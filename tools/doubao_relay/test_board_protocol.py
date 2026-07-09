import unittest

from tools.doubao_relay.board_protocol import BoardMessageError, parse_control_message


class BoardProtocolTest(unittest.TestCase):
    def test_start_turn_requires_bounded_home_state(self):
        message = parse_control_message(
            '{"type":"start_turn","protocol":1,"home":{"temp_c":27.0,"humidity":65}}'
        )

        self.assertEqual(message["type"], "start_turn")
        self.assertEqual(message["home"]["temp_c"], 27.0)

    def test_unknown_type_is_rejected(self):
        with self.assertRaisesRegex(BoardMessageError, "type"):
            parse_control_message('{"type":"execute_shell"}')

    def test_end_audio_control_is_allowed(self):
        message = parse_control_message('{"type":"end_audio","protocol":1}')

        self.assertEqual(message["type"], "end_audio")

    def test_rejects_unknown_top_level_fields(self):
        with self.assertRaisesRegex(BoardMessageError, "field"):
            parse_control_message('{"type":"hello","protocol":1,"extra":true}')

    def test_rejects_unsupported_protocol_version(self):
        with self.assertRaisesRegex(BoardMessageError, "protocol"):
            parse_control_message('{"type":"hello","protocol":2}')

    def test_rejects_oversized_home_snapshot(self):
        large_value = "x" * 900

        with self.assertRaisesRegex(BoardMessageError, "home"):
            parse_control_message(
                '{"type":"start_turn","protocol":1,"home":{"note":"%s"}}' % large_value
            )


if __name__ == "__main__":
    unittest.main()
