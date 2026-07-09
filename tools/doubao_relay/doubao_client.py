import json
import re
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


def _bool_text(value: object, yes: str, no: str) -> str:
    return yes if bool(value) else no


def _home_summary(home: Mapping) -> str:
    room = "有人" if home.get("presence") else "无人"
    if home.get("force_security"):
        room = "强制安防显示无人"
    security = "已布防" if home.get("security_armed") else "关闭"
    alarm = "报警中" if home.get("alarm") else "无报警"
    lamp = "开启" if home.get("lamp_on") else "关闭"
    ac = "制冷" if home.get("ac_cooling") else "待机"
    ir = "收到红外" if home.get("ir_rx") else "未收到红外"
    risk = f"{home.get('risk_score', 0)}/{home.get('risk', 'LOW')}"
    weather = home.get("weather", "--")
    location = home.get("location", "Xiamen")
    return (
        f"室内{float(home.get('temp_c', 0)):.1f}度，湿度{float(home.get('humidity', 0)):.0f}%，"
        f"房间{room}，安防{security}，{alarm}，灯光{lamp}，空调{ac}，"
        f"红外{ir}，风险{risk}，位置{location}，天气{weather}。"
    )


def _compact(text: str) -> str:
    return re.sub(r"[\s，。！？、,.!?;；：:（）()【】\[\]\"'“”‘’]+", "", text or "").lower()


def classify_board_action(text: str) -> dict | None:
    q = _compact(text)
    if not q:
        return None

    if any(word in q for word in ("回家模式", "我回来了", "到家了", "回家了")):
        return {"type": "action", "name": "macro_home", "text": "已切换回家模式。"}
    if any(word in q for word in ("离家模式", "出门模式", "我要出门", "我出门了", "离开家")):
        return {"type": "action", "name": "macro_away", "text": "已切换离家模式，系统进入布防。"}
    if any(word in q for word in ("夜间模式", "睡眠模式", "睡觉模式", "晚安模式", "我要睡觉", "准备睡觉")):
        return {"type": "action", "name": "macro_night", "text": "已切换夜间模式，保留安防监测。"}

    if any(word in q for word in ("清除报警", "解除报警", "取消报警", "复位", "恢复正常")):
        return {"type": "action", "name": "clear_alarm", "text": "已清除报警并恢复自动安防。"}
    if any(word in q for word in ("关闭安防", "取消安防", "撤防", "解除布防")):
        return {"type": "action", "name": "security_off", "text": "已关闭强制安防。"}
    if any(word in q for word in ("打开安防", "开启安防", "进入安防", "启动安防", "布防")):
        return {"type": "action", "name": "security_on", "text": "已进入安防模式，房间按无人状态监测。"}

    if any(word in q for word in ("关灯", "关闭灯", "关闭灯光", "灯带关闭")):
        return {"type": "action", "name": "lamp_off", "text": "已关闭灯带。"}
    if any(word in q for word in ("开灯", "打开灯", "打开灯光", "灯带打开")):
        return {"type": "action", "name": "lamp_on", "text": "已打开灯带。"}

    if any(word in q for word in ("关空调", "关闭空调", "空调待机", "停止制冷")):
        return {"type": "action", "name": "ac_off", "text": "已关闭空调演示。"}
    if any(word in q for word in ("开空调", "打开空调", "空调制冷", "开启制冷", "打开制冷", "有点热", "太热")):
        return {"type": "action", "name": "ac_on", "text": "已打开空调制冷演示，并发送红外测试信号。"}

    return None


class RealtimeSession:
    def __init__(self, session_id: str):
        if not session_id:
            raise SessionError("session id is required")
        self.session_id = session_id
        self.phase = SessionPhase.STARTING
        self.dialog_id = ""
        self.last_asr = ""
        self.reply_text = ""
        self.action_sent = False

    def start_payload(self, home: dict) -> bytes:
        summary = _home_summary(home)
        role = (
            "你是 SmartHomeHub 的小智语音助手。"
            "你理解这块 ESP32-S3 智能家居中控板的固定规则："
            "HOME 页面显示温度、湿度、ROOM、SEC、AC、LAMP；"
            "SYSTEM 页面显示声音、蓝牙、IR 和 IR TEST；"
            "强制安防时 ROOM 显示 EMPTY；"
            "温度高于28度自动制冷，低于25度自动待机；"
            "无人一段时间自动布防，布防时异常声音会触发蜂鸣器和红色灯带报警。"
            "你只能确认这些可执行动作：打开安防、关闭安防、开灯、关灯、打开空调演示、关闭空调演示、清除报警、回家模式、离家模式、夜间模式、报告当前状态。"
            "如果用户要求其他硬件能力，必须说明当前作品未接入该功能。"
            "回答用简短中文，不超过两句话。"
            f"当前板子状态：{summary}"
        )
        body = {
            "asr": {
                "audio_info": {"format": "pcm", "sample_rate": 16000, "channel": 1},
                "extra": {"end_smooth_window_ms": 900},
            },
            "dialog": {
                "bot_name": "小智",
                "system_role": role,
                "speaking_style": "语速稍快，语气自然亲切，适合比赛演示。",
                "extra": {"model": "1.2.1.1", "input_mod": "push_to_talk"},
            },
            "tts": {
                "speaker": "zh_female_vv_jupiter_bigtts",
                "audio_config": {
                    "channel": 1,
                    "format": "pcm_s16le",
                    "sample_rate": 24000,
                    "speech_rate": 15,
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
            if body.get("no_content") is True:
                self.phase = SessionPhase.COMPLETE
                return [{"type": "reply", "text": "没有听到有效人声"}, {"type": "done"}]
            self.phase = SessionPhase.THINKING
            events = []
            action = classify_board_action(self.last_asr)
            if action and not self.action_sent:
                self.action_sent = True
                events.append(action)
            events.append({"type": "phase", "value": "thinking"})
            return events
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
