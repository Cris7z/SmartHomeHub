import unittest

from tools.doubao_relay.doubao_protocol import MAX_FRAME_BYTES
from tools.doubao_relay.server import (
    BOARD_AUDIO_CHUNK_BYTES,
    board_server_options,
    iter_board_audio_chunks,
)


class ServerConfigTest(unittest.TestCase):
    def test_board_server_does_not_ping_esp32_clients(self):
        options = board_server_options()

        self.assertEqual(options["max_size"], MAX_FRAME_BYTES)
        self.assertIsNone(options["ping_interval"])

    def test_board_tts_audio_is_chunked_for_esp32_websocket_client(self):
        pcm = bytes(range(250)) * 8

        chunks = list(iter_board_audio_chunks(pcm))

        self.assertEqual(BOARD_AUDIO_CHUNK_BYTES, 960)
        self.assertEqual(b"".join(chunks), pcm)
        self.assertGreater(len(chunks), 1)
        self.assertTrue(all(len(chunk) <= BOARD_AUDIO_CHUNK_BYTES for chunk in chunks))
        self.assertTrue(all(len(chunk) % 2 == 0 for chunk in chunks))


if __name__ == "__main__":
    unittest.main()
