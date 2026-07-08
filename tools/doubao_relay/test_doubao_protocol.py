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

    def test_end_asr_carries_session_id(self):
        encoded = encode_event(
            message_type=FULL_CLIENT_REQUEST,
            event=400,
            session_id="session-1",
            payload=b"{}",
            serialization="json",
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(FULL_CLIENT_REQUEST, 400, "session-1", b"{}", "json"),
        )

    def test_start_session_matches_exact_fixture(self):
        encoded = encode_event(
            message_type=FULL_CLIENT_REQUEST,
            event=100,
            session_id="session-1",
            payload=b"{}",
            serialization="json",
        )

        self.assertEqual(
            encoded,
            (
                b"\x11\x14\x10\x00"
                + struct.pack(">II", 100, len(b"session-1"))
                + b"session-1"
                + struct.pack(">I", 2)
                + b"{}"
            ),
        )

    def test_audio_request_matches_exact_pcm_fixture(self):
        pcm = b"\x01\x00\xff\x7f"
        encoded = encode_event(
            message_type=AUDIO_REQUEST,
            event=200,
            session_id="session-1",
            payload=pcm,
            serialization="raw",
        )

        self.assertEqual(
            encoded,
            (
                b"\x11\x24\x00\x00"
                + struct.pack(">II", 200, len(b"session-1"))
                + b"session-1"
                + struct.pack(">I", len(pcm))
                + pcm
            ),
        )

    def test_server_json_response_decodes_exact_fixture(self):
        payload = b'{"text":"ok"}'
        encoded = (
            b"\x11\x94\x10\x00"
            + struct.pack(">II", 451, len(b"session-1"))
            + b"session-1"
            + struct.pack(">I", len(payload))
            + payload
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(FULL_SERVER_RESPONSE, 451, "session-1", payload, "json"),
        )

    def test_connection_started_response_skips_connect_id(self):
        payload = b"{}"
        connect_id = b"c5f80e51803e424dafdf63d1786cf588"
        encoded = (
            b"\x11\x94\x10\x00"
            + struct.pack(">II", 50, len(connect_id))
            + connect_id
            + struct.pack(">I", len(payload))
            + payload
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(FULL_SERVER_RESPONSE, 50, None, payload, "json"),
        )

    def test_server_audio_response_decodes_exact_fixture(self):
        pcm = b"\x00\x00\x10\x00"
        encoded = (
            b"\x11\xb4\x00\x00"
            + struct.pack(">II", 352, len(b"session-1"))
            + b"session-1"
            + struct.pack(">I", len(pcm))
            + pcm
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(AUDIO_RESPONSE, 352, "session-1", pcm, "raw"),
        )

    def test_encode_requires_session_for_session_events(self):
        with self.assertRaisesRegex(ProtocolError, "session id"):
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=100,
                payload=b"{}",
            )

    def test_encode_rejects_session_for_connection_events(self):
        with self.assertRaisesRegex(ProtocolError, "session id"):
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=1,
                session_id="session-1",
                payload=b"{}",
            )

    def test_encode_rejects_unknown_serialization(self):
        with self.assertRaisesRegex(ProtocolError, "serialization"):
            encode_event(
                message_type=FULL_CLIENT_REQUEST,
                event=1,
                payload=b"{}",
                serialization="protobuf",
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

    def test_decode_rejects_unsupported_header_fields(self):
        frames = {
            "message type": bytes([0x11, 0xE4, 0x10, 0x00]) + struct.pack(">II", 1, 0),
            "compression": bytes([0x11, 0x14, 0x11, 0x00]) + struct.pack(">II", 1, 0),
            "serialization": bytes([0x11, 0x14, 0x20, 0x00]) + struct.pack(">II", 1, 0),
            "reserved": bytes([0x11, 0x14, 0x10, 0x01]) + struct.pack(">II", 1, 0),
        }

        for name, frame in frames.items():
            with self.subTest(name=name):
                with self.assertRaisesRegex(ProtocolError, name):
                    decode_frame(frame)

    def test_decode_rejects_wrong_error_frame_flag(self):
        bad = bytes([0x11, (ERROR_RESPONSE << 4) | EVENT_FLAG, 0x10, 0x00]) + struct.pack(
            ">II", 40000001, 0
        )

        with self.assertRaisesRegex(ProtocolError, "error frame"):
            decode_frame(bad)

    def test_decode_rejects_oversized_declared_lengths(self):
        oversized_session = (
            bytes([0x11, (AUDIO_REQUEST << 4) | EVENT_FLAG, 0x00, 0x00])
            + struct.pack(">II", 200, 129)
        )
        oversized_payload = (
            bytes([0x11, 0x14, 0x10, 0x00])
            + struct.pack(">II", 1, 2 * 1024 * 1024 + 1)
        )

        with self.assertRaisesRegex(ProtocolError, "session id too large"):
            decode_frame(oversized_session)
        with self.assertRaisesRegex(ProtocolError, "payload too large"):
            decode_frame(oversized_payload)

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
            bytes([0x11, (ERROR_RESPONSE << 4) | 0x0F, 0x10, 0x00])
            + struct.pack(">II", 40000001, len(payload))
            + payload
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(ERROR_RESPONSE, 40000001, None, payload, "json"),
        )

    def test_error_code_is_not_reinterpreted_as_session_event(self):
        encoded = (
            bytes([0x11, (ERROR_RESPONSE << 4) | 0x0F, 0x10, 0x00])
            + struct.pack(">II", 200, 0)
        )

        self.assertEqual(
            decode_frame(encoded),
            Frame(ERROR_RESPONSE, 200, None, b"", "json"),
        )


if __name__ == "__main__":
    unittest.main()
