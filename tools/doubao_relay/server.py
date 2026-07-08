import argparse
import asyncio
import json
import os
import wave
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
BOARD_AUDIO_CHUNK_BYTES = 960


def board_server_options() -> dict:
    return {"max_size": MAX_FRAME_BYTES, "ping_interval": None}


def iter_board_audio_chunks(payload: bytes, chunk_bytes: int = BOARD_AUDIO_CHUNK_BYTES):
    if chunk_bytes < 2:
        raise ValueError("chunk size must fit at least one int16 sample")
    chunk_bytes -= chunk_bytes % 2
    for offset in range(0, len(payload), chunk_bytes):
        yield payload[offset : offset + chunk_bytes]


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


def log(message: str) -> None:
    print(f"[relay] {message}", flush=True)


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
        self.audio_frames = 0
        self.audio_bytes = 0
        self.audio_peak = 0
        self.audio_capture = bytearray()
        self.tts_audio_chunks = 0
        self.tts_audio_bytes = 0

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
        log(f"start_session id={self.session.session_id}")

    async def send_audio(self, pcm: bytes) -> None:
        self.session.on_audio(pcm)
        self.audio_frames += 1
        self.audio_bytes += len(pcm)
        if len(self.audio_capture) < 512000:
            remaining = 512000 - len(self.audio_capture)
            self.audio_capture.extend(pcm[:remaining])
        for i in range(0, len(pcm) - 1, 2):
            sample = int.from_bytes(pcm[i : i + 2], "little", signed=True)
            self.audio_peak = max(self.audio_peak, abs(sample))
        if self.audio_frames <= 3 or self.audio_frames % 50 == 0:
            log(f"audio frames={self.audio_frames} bytes={self.audio_bytes} peak={self.audio_peak}")
        await self._write_queue.put(
            encode_event(
                message_type=AUDIO_REQUEST,
                event=200,
                session_id=self.session.session_id,
                payload=pcm,
                serialization="raw",
            )
        )

    async def end_audio(self) -> None:
        if self.session.phase is not SessionPhase.STREAMING:
            log(f"end_audio ignored phase={self.session.phase.name}")
            return
        self.session.phase = SessionPhase.THINKING
        log(f"end_audio frames={self.audio_frames} bytes={self.audio_bytes} peak={self.audio_peak}")
        self._write_debug_wav()
        await self._write_queue.put(
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=400,
                session_id=self.session.session_id,
                payload=b"{}",
                serialization="json",
            )
        )

    def _write_debug_wav(self) -> None:
        if not self.audio_capture:
            return
        path = Path(".pio") / "logs" / "last_turn.wav"
        path.parent.mkdir(parents=True, exist_ok=True)
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(16000)
            wav.writeframes(bytes(self.audio_capture))
        log(f"wrote_audio path={path} bytes={len(self.audio_capture)}")

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
                log(f"doubao_error code={frame.event} payload={frame.payload[:160]!r}")
                for event in self.session.on_event(599, frame.payload):
                    await self.board.send(json_bytes(event))
                continue
            if frame.event == 50:
                log("connection_started")
                self._connection_started.set()
                continue
            if frame.message_type == AUDIO_RESPONSE:
                if self.session.phase is not SessionPhase.SPEAKING:
                    log("audio_response_begin")
                    self.session.phase = SessionPhase.SPEAKING
                    await self.board.send(json_bytes({"type": "phase", "value": "speaking"}))
                for chunk in iter_board_audio_chunks(frame.payload):
                    await self.board.send(chunk)
                    self.tts_audio_chunks += 1
                    self.tts_audio_bytes += len(chunk)
                    if self.tts_audio_chunks <= 3 or self.tts_audio_chunks % 50 == 0:
                        log(
                            f"tts_audio chunks={self.tts_audio_chunks} "
                            f"bytes={self.tts_audio_bytes}"
                        )
                continue
            if frame.message_type == FULL_SERVER_RESPONSE:
                log(f"doubao_event event={frame.event} payload={frame.payload[:160]!r}")
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
    log("board_connected")
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
                        log("board_start_turn")
                        await websocket.send(json_bytes({"type": "phase", "value": "connecting"}))
                        await turn.start()
                    elif control["type"] == "end_audio" and turn:
                        log("board_end_audio")
                        await turn.end_audio()
                        await websocket.send(json_bytes({"type": "phase", "value": "thinking"}))
                    elif control["type"] == "cancel" and turn:
                        log("board_cancel")
                        await turn.close()
                        turn = None
                        await websocket.send(json_bytes({"type": "done"}))
                except (BoardMessageError, SessionError, TimeoutError) as exc:
                    await websocket.send(json_bytes({"type": "error", "message": str(exc)[:191]}))
            elif isinstance(message, (bytes, bytearray)):
                if not turn or turn.session.phase is not SessionPhase.STREAMING:
                    log(f"audio_rejected bytes={len(message)} phase={turn.session.phase.name if turn else 'NONE'}")
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
    async with serve(lambda ws: handle_board(ws, env), host, port, **board_server_options()):
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
