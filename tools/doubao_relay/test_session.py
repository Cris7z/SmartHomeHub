import json
import unittest

from tools.doubao_relay.doubao_client import RealtimeSession, SessionError, SessionPhase, build_headers


class RealtimeSessionTest(unittest.TestCase):
    def test_session_accepts_expected_server_order(self):
        session = RealtimeSession("session-1")

        self.assertEqual(session.on_event(150, b'{"dialog_id":"dialog-1"}'), [{"type": "phase", "value": "listening"}])
        self.assertIs(session.phase, SessionPhase.STREAMING)
        session.on_event(451, '{"results":[{"text":"你好","is_interim":false}]}'.encode())
        self.assertEqual(session.last_asr, "你好")
        session.on_event(550, '{"content":"你好呀"}'.encode())
        self.assertEqual(session.reply_text, "你好呀")
        self.assertEqual(session.on_event(359, b"{}"), [{"type": "done"}])
        self.assertIs(session.phase, SessionPhase.COMPLETE)

    def test_audio_before_session_started_is_rejected(self):
        session = RealtimeSession("session-1")

        with self.assertRaisesRegex(SessionError, "session"):
            session.on_audio(b"\x00\x00")

    def test_start_payload_contains_required_realtime_settings(self):
        payload = RealtimeSession("session-1").start_payload({"temp_c": 27.0, "humidity": 65})
        body = json.loads(payload.decode("utf-8"))

        self.assertEqual(body["asr"]["audio_info"], {"format": "pcm", "sample_rate": 16000, "channel": 1})
        self.assertEqual(body["asr"]["extra"]["end_smooth_window_ms"], 900)
        self.assertEqual(body["dialog"]["extra"]["model"], "1.2.1.1")
        self.assertIn("27.0", body["dialog"]["system_role"])
        self.assertIn("65", body["dialog"]["system_role"])
        self.assertEqual(body["tts"]["speaker"], "zh_female_vv_jupiter_bigtts")
        self.assertEqual(body["tts"]["audio_config"]["format"], "pcm_s16le")
        self.assertEqual(body["tts"]["audio_config"]["sample_rate"], 24000)
        self.assertIsInstance(body["tts"]["extra"], dict)

    def test_build_headers_prefers_single_api_key(self):
        headers = build_headers({"DOUBAO_API_KEY": "key-1"}, "connect-1")

        self.assertEqual(headers["X-Api-Key"], "key-1")
        self.assertEqual(headers["X-Api-Resource-Id"], "volc.speech.dialog")
        self.assertNotIn("X-Api-App-ID", headers)

    def test_build_headers_falls_back_to_app_credentials(self):
        headers = build_headers(
            {"DOUBAO_APP_ID": "app-1", "DOUBAO_ACCESS_KEY": "access-1"},
            "connect-1",
        )

        self.assertEqual(headers["X-Api-App-ID"], "app-1")
        self.assertEqual(headers["X-Api-Access-Key"], "access-1")
        self.assertEqual(headers["X-Api-Connect-Id"], "connect-1")


if __name__ == "__main__":
    unittest.main()
