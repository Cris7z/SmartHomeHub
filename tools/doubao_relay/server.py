import argparse
import asyncio
import json
import os
import uuid
from pathlib import Path
from typing import Mapping

from tools.doubao_relay.board_protocol import BoardMessageError, PROTOCOL_VERSION, parse_control_message
from tools.doubao_relay.doubao_client import RealtimeSession, SessionError, SessionPhase, build_headers
from tools.doubao_relay.doubao_protocol import (
    AUDIO_REQUEST,
    AUDIO_RESPONSE,
    ERROR_RESPONSE,
    FULL_CLIENT_REQUEST,
    FULL_SERVER_RESPONSE,
    MAX_FRAME_BYTES,
    decode_frame,
    encode_event,
)


REALTIME_URL = "wss://openspeech.bytedance.com/api/v3/realtime/dialogue"
DEFAULT_BIND_HOST = "0.0.0.0"
DEFAULT_BIND_PORT = 8765


def load_env(path: Path | None = None) -> dict[str, str]:
    env = dict(os.environ)
    dotenv = path or Path(__file__).with_name(".env")
    if not dotenv.exists():
        return env
    for raw_line in dotenv.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        env.setdefault(key.strip(), value.strip())
    return env


def json_bytes(payload: dict) -> str:
    return json.dumps(payload, ensure_ascii=False, separators=(",", ":"))


async def probe(env: Mapping[str, str]) -> str:
    from websockets.asyncio.client import connect

    connect_id = uuid.uuid4().hex
    try:
        async with connect(
            REALTIME_URL,
            additional_headers=build_headers(env, connect_id),
            max_size=None,
        ) as doubao:
            await doubao.send(
                encode_event(
                    message_type=FULL_CLIENT_REQUEST,
                    event=1,
                    payload=b"{}",
                    serialization="json",
                )
            )
            raw = await asyncio.wait_for(doubao.recv(), timeout=8)
            frame = decode_frame(raw)
            if frame.message_type == ERROR_RESPONSE:
                return f"AUTH FAIL code={frame.event}"
            if frame.event != 50:
                return f"AUTH FAIL code=event-{frame.event}"
            await doubao.send(
                encode_event(
                    message_type=FULL_CLIENT_REQUEST,
                    event=2,
                    payload=b"{}",
                    serialization="json",
                )
            )
            logid = _response_logid(doubao) or connect_id
            return f"AUTH OK logid={logid}"
    except SessionError:
        raise
    except Exception as exc:
        code = getattr(getattr(exc, "response", None), "status_code", None)
        return f"AUTH FAIL code={code or exc.__class__.__name__}"


def _response_logid(connection) -> str:
    response = getattr(connection, "response", None)
    headers = getattr(response, "headers", {}) if response else {}
    return headers.get("X-Tt-Logid", "") or headers.get("X-Request-Id", "")


class DoubaoTurn:
    def __init__(self, board, env: Mapping[str, str], home: dict):
        self.board = board
        self.env = env
        self.session = RealtimeSession(uuid.uuid4().hex)
        self.home = home
        self.doubao = None
        self._write_queue: asyncio.Queue[bytes | None] = asyncio.Queue()
        self._writer_task: asyncio.Task | None = None
        self._reader_task: asyncio.Task | None = None
        self._connection_started = asyncio.Event()

    async def start(self) -> None:
        from websockets.asyncio.client import connect

        self.doubao = await connect(
            REALTIME_URL,
            additional_headers=build_headers(self.env, uuid.uuid4().hex),
            max_size=MAX_FRAME_BYTES,
        )
        self._writer_task = asyncio.create_task(self._writer())
        self._reader_task = asyncio.create_task(self._reader())
        await self._write_queue.put(
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=1,
                payload=b"{}",
                serialization="json",
            )
        )
        await asyncio.wait_for(self._connection_started.wait(), timeout=8)
        await self._write_queue.put(
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=100,
                session_id=self.session.session_id,
                payload=self.session.start_payload(self.home),
                serialization="json",
            )
        )

    async def send_audio(self, pcm: bytes) -> None:
        self.session.on_audio(pcm)
        await self._write_queue.put(
            encode_event(
                message_type=AUDIO_REQUEST,
                event=200,
                session_id=self.session.session_id,
                payload=pcm,
                serialization="raw",
            )
        )

    async def close(self) -> None:
        await self._write_queue.put(None)
        if self._writer_task:
            await self._writer_task
        if self._reader_task:
            self._reader_task.cancel()
        if self.doubao:
            await self.doubao.close()

    async def _writer(self) -> None:
        while True:
            frame = await self._write_queue.get()
            if frame is None:
                return
            await self.doubao.send(frame)

    async def _reader(self) -> None:
        async for raw in self.doubao:
            if not isinstance(raw, bytes):
                continue
            frame = decode_frame(raw)
            if frame.message_type == ERROR_RESPONSE:
                for event in self.session.on_event(599, frame.payload):
                    await self.board.send(json_bytes(event))
                continue
            if frame.event == 50:
                self._connection_started.set()
                continue
            if frame.message_type == AUDIO_RESPONSE:
                await self.board.send(frame.payload)
                continue
            if frame.message_type == FULL_SERVER_RESPONSE:
                for event in self.session.on_event(frame.event, frame.payload):
                    await self.board.send(json_bytes(event))


def _request_path(websocket) -> str:
    request = getattr(websocket, "request", None)
    return getattr(request, "path", "/voice") if request else "/voice"


async def handle_board(websocket, env: Mapping[str, str]) -> None:
    if _request_path(websocket) != "/voice":
        await websocket.close(1008, "unsupported path")
        return

    turn: DoubaoTurn | None = None
    await websocket.send(json_bytes({"type": "ready", "protocol": PROTOCOL_VERSION}))
    try:
        async for message in websocket:
            if isinstance(message, str):
                try:
                    control = parse_control_message(message)
                    if control["type"] == "hello":
                        await websocket.send(json_bytes({"type": "hello", "protocol": PROTOCOL_VERSION}))
                    elif control["type"] == "start_turn":
                        if turn:
                            await turn.close()
                        turn = DoubaoTurn(websocket, env, control["home"])
                        await websocket.send(json_bytes({"type": "phase", "value": "connecting"}))
                        await turn.start()
                    elif control["type"] == "cancel" and turn:
                        await turn.close()
                        turn = None
                        await websocket.send(json_bytes({"type": "done"}))
                except (BoardMessageError, SessionError, TimeoutError) as exc:
                    await websocket.send(json_bytes({"type": "error", "message": str(exc)[:191]}))
            elif isinstance(message, (bytes, bytearray)):
                if not turn or turn.session.phase is not SessionPhase.STREAMING:
                    await websocket.send(json_bytes({"type": "error", "message": "not listening"}))
                    continue
                try:
                    await turn.send_audio(bytes(message))
                except SessionError as exc:
                    await websocket.send(json_bytes({"type": "error", "message": str(exc)[:191]}))
    finally:
        if turn:
            await turn.close()


async def run_server(env: Mapping[str, str]) -> None:
    from websockets.asyncio.server import serve

    host = env.get("DOUBAO_BIND_HOST", DEFAULT_BIND_HOST)
    port = int(env.get("DOUBAO_BIND_PORT", str(DEFAULT_BIND_PORT)))
    async with serve(lambda ws: handle_board(ws, env), host, port, max_size=MAX_FRAME_BYTES):
        print(f"Doubao relay listening on ws://{host}:{port}/voice")
        await asyncio.Future()


def main() -> None:
    parser = argparse.ArgumentParser(description="Doubao realtime relay for ESP32-S3 XiaoZhi voice")
    parser.add_argument("--probe", action="store_true", help="check Doubao authentication and exit")
    args = parser.parse_args()
    env = load_env()
    if args.probe:
        print(asyncio.run(probe(env)))
    else:
        asyncio.run(run_server(env))


if __name__ == "__main__":
    main()
