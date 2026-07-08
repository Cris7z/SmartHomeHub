from dataclasses import dataclass
import struct


FULL_CLIENT_REQUEST = 0x1
AUDIO_REQUEST = 0x2
FULL_SERVER_RESPONSE = 0x9
AUDIO_RESPONSE = 0xB
ERROR_RESPONSE = 0xF
EVENT_FLAG = 0x4

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
    serialization_nibble = 1 if serialization == "json" else 0
    header = bytes(
        [
            0x11,
            (message_type << 4) | EVENT_FLAG,
            serialization_nibble << 4,
            0x00,
        ]
    )
    optional = struct.pack(">I", event)
    if session_id is not None:
        encoded_id = session_id.encode("utf-8")
        optional += struct.pack(">I", len(encoded_id)) + encoded_id
    return header + optional + struct.pack(">I", len(payload)) + payload


def decode_frame(data: bytes) -> Frame:
    if len(data) < 12:
        raise ProtocolError("truncated frame header")
    if data[0] >> 4 != 1 or data[0] & 0x0F != 1:
        raise ProtocolError("unsupported protocol header")

    message_type = data[1] >> 4
    flags = data[1] & 0x0F
    serialization = "json" if data[2] >> 4 == 1 else "raw"
    offset = 4

    if message_type == ERROR_RESPONSE:
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
        if len(data) < offset + session_size:
            raise ProtocolError("truncated session id")
        try:
            session_id = data[offset : offset + session_size].decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ProtocolError("invalid session id") from exc
        offset += session_size

    if len(data) < offset + 4:
        raise ProtocolError("truncated payload length")
    payload_size = struct.unpack_from(">I", data, offset)[0]
    offset += 4
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
