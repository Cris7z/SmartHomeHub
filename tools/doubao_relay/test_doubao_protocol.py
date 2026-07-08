import struct
import unittest
from dataclasses import FrozenInstanceError

from tools.doubao_relay.doubao_protocol import (
    AUDIO_REQUEST,
    AUDIO_RESPONSE,
    ERROR_RESPONSE,
    EVENT_FLAG,
    FULL_CLIENT_REQUEST,
    FULL_SERVER_RESPONSE,
    Frame,
    ProtocolError,
    decode_frame,
    encode_event,
)


class DoubaoProtocolTest(unittest.TestCase):
    def test_public_constants_match_protocol_values(self):
        self.assertEqual(AUDIO_REQUEST, 0x2)
        self.assertEqual(AUDIO_RESPONSE, 0xB)
        self.assertEqual(FULL_CLIENT_REQUEST, 0x1)
        self.assertEqual(FULL_SERVER_RESPONSE, 0x9)
        self.assertEqual(ERROR_RESPONSE, 0xF)
        self.assertEqual(EVENT_FLAG, 0x4)

    def test_frame_is_frozen(self):
        frame = Frame(FULL_CLIENT_REQUEST, 1, None, b"{}", "json")

        with self.assertRaises(FrozenInstanceError):
            frame.event = 2

    def test_start_connection_matches_official_fixture(self):
        encoded = encode_event(
            message_type=FULL_CLIENT_REQUEST,
            event=1,
            payload=b"{}",
            serialization="json",
        )

        self.assertEqual(
            encoded,
            bytes([17, 20, 16, 0, 0, 0, 0, 1, 0, 0, 0, 2, 123, 125]),
        )

    def test_task_request_carries_session_and_raw_pcm(self):
        pcm = b"\x01\x00\xff\x7f"
        encoded = encode_event(
            message_type=AUDIO_REQUEST,
            event=200,
            session_id="session-1",
            payload=pcm,
            serialization="raw",
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(AUDIO_REQUEST, 200, "session-1", pcm, "raw"),
        )

    def test_all_project_session_events_carry_session_ids(self):
        session_events = {
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

        for event in session_events:
            with self.subTest(event=event):
                encoded = encode_event(
                    message_type=FULL_CLIENT_REQUEST,
                    event=event,
                    session_id="session-1",
                    payload=b"{}",
                )
                self.assertEqual(decode_frame(encoded).session_id, "session-1")

    def test_decode_rejects_truncated_frame_header(self):
        with self.assertRaisesRegex(ProtocolError, "truncated"):
            decode_frame(b"\x11\x14\x10\x00")

    def test_decode_rejects_truncated_payload(self):
        bad = bytes([0x11, 0x94, 0x10, 0x00, 0, 0, 0, 1, 0, 0, 0, 5, 1, 2])

        with self.assertRaisesRegex(ProtocolError, "truncated"):
            decode_frame(bad)

    def test_decode_rejects_trailing_payload_bytes(self):
        encoded = encode_event(
            message_type=FULL_CLIENT_REQUEST,
            event=1,
            payload=b"{}",
        )

        with self.assertRaisesRegex(ProtocolError, "trailing"):
            decode_frame(encoded + b"\x00")

    def test_decode_rejects_invalid_protocol_header(self):
        bad_version = bytes([0x21, 0x14, 0x10, 0x00]) + struct.pack(">II", 1, 0)
        bad_header_size = bytes([0x12, 0x14, 0x10, 0x00]) + struct.pack(">II", 1, 0)

        for frame in (bad_version, bad_header_size):
            with self.subTest(frame=frame):
                with self.assertRaisesRegex(ProtocolError, "header"):
                    decode_frame(frame)

    def test_decode_rejects_missing_event_flag(self):
        bad = bytes([0x11, 0x10, 0x10, 0x00]) + struct.pack(">II", 1, 0)

        with self.assertRaisesRegex(ProtocolError, "event flag"):
            decode_frame(bad)

    def test_decode_rejects_invalid_utf8_session_id(self):
        bad = (
            bytes([0x11, (AUDIO_REQUEST << 4) | EVENT_FLAG, 0x00, 0x00])
            + struct.pack(">II", 200, 1)
            + b"\xff"
            + struct.pack(">I", 0)
        )

        with self.assertRaisesRegex(ProtocolError, "invalid session id"):
            decode_frame(bad)

    def test_decode_error_frame_uses_error_code_as_event(self):
        payload = b'{"message":"bad request"}'
        encoded = (
            bytes([0x11, ERROR_RESPONSE << 4, 0x10, 0x00])
            + struct.pack(">II", 40000001, len(payload))
            + payload
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(ERROR_RESPONSE, 40000001, None, payload, "json"),
        )

    def test_error_code_is_not_reinterpreted_as_session_event(self):
        encoded = (
            bytes([0x11, ERROR_RESPONSE << 4, 0x10, 0x00])
            + struct.pack(">II", 200, 0)
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(ERROR_RESPONSE, 200, None, b"", "json"),
        )


if __name__ == "__main__":
    unittest.main()
