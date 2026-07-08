import json
from enum import Enum, auto
from typing import Mapping


class SessionError(RuntimeError):
    pass


class SessionPhase(Enum):
    STARTING = auto()
    STREAMING = auto()
    THINKING = auto()
    SPEAKING = auto()
    COMPLETE = auto()
    FAILED = auto()


def build_headers(env: Mapping[str, str], connect_id: str) -> dict[str, str]:
    common = {
        "X-Api-Resource-Id": "volc.speech.dialog",
        "X-Api-App-Key": "PlgvMymc7f3tQnJ6",
        "X-Api-Connect-Id": connect_id,
    }
    api_key = env.get("DOUBAO_API_KEY", "")
    if api_key:
        return common | {"X-Api-Key": api_key}

    app_id = env.get("DOUBAO_APP_ID", "")
    access_key = env.get("DOUBAO_ACCESS_KEY", "")
    if not app_id or not access_key:
        raise SessionError("missing Doubao credentials")
    return common | {"X-Api-App-ID": app_id, "X-Api-Access-Key": access_key}


class RealtimeSession:
    def __init__(self, session_id: str):
        if not session_id:
            raise SessionError("session id is required")
        self.session_id = session_id
        self.phase = SessionPhase.STARTING
        self.dialog_id = ""
        self.last_asr = ""
        self.reply_text = ""

    def start_payload(self, home: dict) -> bytes:
        role = (
            "你是小智智能家居助手。请用简短自然的中文回答。"
            f"当前室内温度{float(home.get('temp_c', 0)):.1f}度，"
            f"湿度{float(home.get('humidity', 0)):.0f}%。"
        )
        body = {
            "asr": {
                "audio_info": {"format": "pcm", "sample_rate": 16000, "channel": 1},
                "extra": {"end_smooth_window_ms": 900},
            },
            "dialog": {
                "bot_name": "小智",
                "system_role": role,
                "speaking_style": "语速稍快，语气自然亲切，回答不超过三句话。",
                "extra": {"model": "1.2.1.1"},
            },
            "tts": {
                "speaker": "zh_female_vv_jupiter_bigtts",
                "audio_config": {
                    "channel": 1,
                    "format": "pcm_s16le",
                    "sample_rate": 24000,
                    "speech_rate": 5,
                },
                "extra": {},
            },
        }
        return json.dumps(body, ensure_ascii=False, separators=(",", ":")).encode("utf-8")

    def on_event(self, event: int, payload: bytes) -> list[dict]:
        try:
            body = json.loads(payload.decode("utf-8") or "{}")
        except (UnicodeDecodeError, json.JSONDecodeError) as exc:
            raise SessionError("invalid JSON event payload") from exc

        if event == 150:
            self.dialog_id = str(body.get("dialog_id", ""))
            self.phase = SessionPhase.STREAMING
            return [{"type": "phase", "value": "listening"}]
        if event == 451:
            results = body.get("results", [])
            if results:
                self.last_asr = str(results[-1].get("text", ""))[:191]
                return [{"type": "asr", "text": self.last_asr}]
            return []
        if event == 459:
            self.phase = SessionPhase.THINKING
            return [{"type": "phase", "value": "thinking"}]
        if event == 350:
            self.phase = SessionPhase.SPEAKING
            return [{"type": "phase", "value": "speaking"}]
        if event == 550:
            self.reply_text = (self.reply_text + str(body.get("content", "")))[:191]
            return [{"type": "reply", "text": self.reply_text}]
        if event == 359:
            self.phase = SessionPhase.COMPLETE
            return [{"type": "done"}]
        if event in {153, 599}:
            self.phase = SessionPhase.FAILED
            return [{"type": "error", "code": str(body.get("status_code", event))}]
        return []

    def on_audio(self, pcm: bytes) -> None:
        if self.phase is not SessionPhase.STREAMING:
            raise SessionError("session is not accepting audio")
        if not pcm or len(pcm) % 2:
            raise SessionError("PCM must contain complete int16 samples")
