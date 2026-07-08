from dataclasses import dataclass
import struct


FULL_CLIENT_REQUEST = 0x1
AUDIO_REQUEST = 0x2
FULL_SERVER_RESPONSE = 0x9
AUDIO_RESPONSE = 0xB
ERROR_RESPONSE = 0xF
EVENT_FLAG = 0x4
MAX_SESSION_ID_BYTES = 128
MAX_CONNECT_ID_BYTES = 128
MAX_PAYLOAD_BYTES = 2 * 1024 * 1024
MAX_FRAME_BYTES = MAX_PAYLOAD_BYTES + 1024

_KNOWN_MESSAGE_TYPES = frozenset(
    {
        FULL_CLIENT_REQUEST,
        AUDIO_REQUEST,
        FULL_SERVER_RESPONSE,
        AUDIO_RESPONSE,
        ERROR_RESPONSE,
    }
)
_SERIALIZATION_NIBBLES = {
    "raw": 0,
    "json": 1,
}
_SERIALIZATION_NAMES = {value: key for key, value in _SERIALIZATION_NIBBLES.items()}

_SESSION_EVENTS = frozenset(
    {
        100,
        102,
        150,
        152,
        153,
        154,
        200,
        251,
        350,
        351,
        352,
        359,
        450,
        451,
        459,
        550,
        559,
        599,
    }
)
_CONNECT_EVENTS = frozenset({1, 2, 50, 51, 52})


class ProtocolError(ValueError):
    pass


@dataclass(frozen=True)
class Frame:
    message_type: int
    event: int
    session_id: str | None
    payload: bytes
    serialization: str


def encode_event(
    *,
    message_type: int,
    event: int,
    payload: bytes,
    session_id: str | None = None,
    serialization: str = "json",
) -> bytes:
    if message_type not in _KNOWN_MESSAGE_TYPES or message_type == ERROR_RESPONSE:
        raise ProtocolError("unsupported message type")
    if serialization not in _SERIALIZATION_NIBBLES:
        raise ProtocolError("unsupported serialization")
    if len(payload) > MAX_PAYLOAD_BYTES:
        raise ProtocolError("payload too large")
    if event in _SESSION_EVENTS:
        if not session_id:
            raise ProtocolError("session id required")
    elif session_id is not None:
        raise ProtocolError("unexpected session id")

    encoded_id = None
    if session_id is not None:
        encoded_id = session_id.encode("utf-8")
        if len(encoded_id) > MAX_SESSION_ID_BYTES:
            raise ProtocolError("session id too large")

    serialization_nibble = _SERIALIZATION_NIBBLES[serialization]
    header = bytes(
        [
            0x11,
            (message_type << 4) | EVENT_FLAG,
            serialization_nibble << 4,
            0x00,
        ]
    )
    optional = struct.pack(">I", event)
    if encoded_id is not None:
        optional += struct.pack(">I", len(encoded_id)) + encoded_id
    frame = header + optional + struct.pack(">I", len(payload)) + payload
    if len(frame) > MAX_FRAME_BYTES:
        raise ProtocolError("frame too large")
    return frame


def decode_frame(data: bytes) -> Frame:
    if len(data) > MAX_FRAME_BYTES:
        raise ProtocolError("frame too large")
    if len(data) < 12:
        raise ProtocolError("truncated frame header")
    if data[0] >> 4 != 1 or data[0] & 0x0F != 1:
        raise ProtocolError("unsupported protocol header")
    if data[3] != 0:
        raise ProtocolError("unsupported reserved header")

    message_type = data[1] >> 4
    flags = data[1] & 0x0F
    if message_type not in _KNOWN_MESSAGE_TYPES:
        raise ProtocolError("unsupported message type")

    serialization_nibble = data[2] >> 4
    if serialization_nibble not in _SERIALIZATION_NAMES:
        raise ProtocolError("unsupported serialization")
    if data[2] & 0x0F:
        raise ProtocolError("unsupported compression")
    serialization = _SERIALIZATION_NAMES[serialization_nibble]
    offset = 4

    if message_type == ERROR_RESPONSE:
        if flags != 0x0F:
            raise ProtocolError("unsupported error frame flags")
        event = struct.unpack_from(">I", data, offset)[0]
        offset += 4
    else:
        if flags != EVENT_FLAG:
            raise ProtocolError("event flag is required")
        event = struct.unpack_from(">I", data, offset)[0]
        offset += 4

    session_id = None
    if message_type != ERROR_RESPONSE and event in _SESSION_EVENTS:
        if len(data) < offset + 4:
            raise ProtocolError("truncated session id length")
        session_size = struct.unpack_from(">I", data, offset)[0]
        offset += 4
        if session_size == 0:
            raise ProtocolError("session id required")
        if session_size > MAX_SESSION_ID_BYTES:
            raise ProtocolError("session id too large")
        if len(data) < offset + session_size:
            raise ProtocolError("truncated session id")
        try:
            session_id = data[offset : offset + session_size].decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ProtocolError("invalid session id") from exc
        offset += session_size
    elif message_type != ERROR_RESPONSE and event in _CONNECT_EVENTS:
        if len(data) >= offset + 4:
            connect_size = struct.unpack_from(">I", data, offset)[0]
            next_offset = offset + 4 + connect_size
            if 0 < connect_size <= MAX_CONNECT_ID_BYTES and len(data) >= next_offset + 4:
                next_payload_size = struct.unpack_from(">I", data, next_offset)[0]
                if (
                    next_payload_size <= MAX_PAYLOAD_BYTES
                    and len(data) == next_offset + 4 + next_payload_size
                ):
                    offset = next_offset

    if len(data) < offset + 4:
        raise ProtocolError("truncated payload length")
    payload_size = struct.unpack_from(">I", data, offset)[0]
    offset += 4
    if payload_size > MAX_PAYLOAD_BYTES:
        raise ProtocolError("payload too large")
    if len(data) < offset + payload_size:
        raise ProtocolError("truncated payload")
    if len(data) > offset + payload_size:
        raise ProtocolError("trailing payload bytes")

    return Frame(
        message_type=message_type,
        event=event,
        session_id=session_id,
        payload=data[offset : offset + payload_size],
        serialization=serialization,
    )
