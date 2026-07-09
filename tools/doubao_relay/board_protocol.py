import json


ALLOWED_TYPES = frozenset({"hello", "start_turn", "end_audio", "cancel"})
ALLOWED_FIELDS = frozenset({"type", "protocol", "home"})
MAX_CONTROL_BYTES = 2048
MAX_HOME_BYTES = 768
PROTOCOL_VERSION = 1


class BoardMessageError(ValueError):
    pass


def parse_control_message(text: str) -> dict:
    if len(text.encode("utf-8")) > MAX_CONTROL_BYTES:
        raise BoardMessageError("control message is too large")
    try:
        body = json.loads(text)
    except json.JSONDecodeError as exc:
        raise BoardMessageError("malformed JSON control message") from exc
    if not isinstance(body, dict):
        raise BoardMessageError("control message must be an object")

    extra_fields = set(body) - ALLOWED_FIELDS
    if extra_fields:
        raise BoardMessageError("unknown control field")

    message_type = body.get("type")
    if message_type not in ALLOWED_TYPES:
        raise BoardMessageError("unsupported control type")

    protocol = body.get("protocol", PROTOCOL_VERSION)
    if protocol != PROTOCOL_VERSION:
        raise BoardMessageError("unsupported protocol version")

    home = body.get("home")
    if message_type == "start_turn" and not isinstance(home, dict):
        raise BoardMessageError("start_turn requires home object")
    if home is not None:
        if not isinstance(home, dict):
            raise BoardMessageError("home must be an object")
        encoded_home = json.dumps(home, ensure_ascii=False, separators=(",", ":")).encode("utf-8")
        if len(encoded_home) > MAX_HOME_BYTES:
            raise BoardMessageError("home snapshot is too large")

    result = {"type": message_type, "protocol": PROTOCOL_VERSION}
    if home is not None:
        result["home"] = home
    return result
