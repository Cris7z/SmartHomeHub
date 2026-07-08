# Doubao Realtime Voice Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make INMP441 speech trigger a Doubao Realtime conversation whose generated audio is played only through the board's MAX98357A speaker.

**Architecture:** The ESP32-S3 streams 16 kHz PCM over a LAN WebSocket to a lightweight Python bridge. The bridge translates the simple board protocol to Doubao's binary Realtime WebSocket protocol and returns recognized text, reply text, and 24 kHz PCM to the board. Pure framing, PCM conversion, ring-buffer, and state logic stay separately testable.

**Tech Stack:** ESP32-S3 Arduino/PlatformIO, legacy ESP-IDF I2S driver, links2004/WebSockets, Python 3.13, websockets 16.0, C++17 host tests, Python unittest.

---

## File Structure

### PC bridge

- Create `tools/doubao_relay/__init__.py`: make bridge modules and tests importable from the repository root.
- Create `tools/doubao_relay/doubao_protocol.py`: encode and decode Doubao big-endian event frames.
- Create `tools/doubao_relay/doubao_client.py`: authenticate, manage connection/session lifecycle, and expose normalized events.
- Create `tools/doubao_relay/board_protocol.py`: validate the small board-to-bridge JSON protocol.
- Create `tools/doubao_relay/server.py`: bridge one board WebSocket to one Doubao session.
- Create `tools/doubao_relay/test_doubao_protocol.py`: exact binary frame fixtures and malformed-frame tests.
- Create `tools/doubao_relay/test_board_protocol.py`: control-message validation tests.
- Create `tools/doubao_relay/test_session.py`: state-order and normalized-event tests with an in-memory fake upstream.
- Create `tools/doubao_relay/requirements.txt`: pin `websockets==16.0`.
- Create `tools/doubao_relay/.env.example`: document API-key and legacy authentication fields without credentials.
- Create `tools/doubao_relay/setup.ps1`: create a small D:-resident virtual environment.
- Create `tools/doubao_relay/run.ps1`: load ignored local environment and start the bridge.
- Modify `.gitignore`: ignore bridge `.env`, bytecode, and local virtual environments.

### Firmware

- Create `src/io/voice_capture_core.h/.cpp`: portable slot selection, PCM conversion, and pre-roll ring.
- Create `src/io/voice_capture.h/.cpp`: I2S0 packet acquisition and turn capture.
- Create `src/board/pcm_ring_buffer.h/.cpp`: portable bounded PCM queue.
- Create `src/board/streaming_speaker.h/.cpp`: PSRAM queue and 24 kHz I2S1 output.
- Create `src/net/voice_stream_protocol.h/.cpp`: parse bounded bridge control JSON.
- Create `src/net/voice_stream_client.h/.cpp`: board WebSocket lifecycle and binary streaming.
- Modify `src/board/config.h`: packet, pre-roll, timeout, cooldown, and sample-rate constants.
- Modify `src/board/hardware.h/.cpp`: expose I2S microphone reads and speaker clock switching without duplicating drivers.
- Modify `src/io/sensors.h/.cpp`: allow normal level sensing to yield I2S0 ownership during active streaming.
- Modify `src/app/hub_state.h`: track relay readiness, recognized text, reply text, and stream errors.
- Modify `src/app/xiaozhi_core.h/.cpp`: define deterministic realtime phase transitions.
- Modify `src/app/xiaozhi_ai.h/.cpp`: orchestrate automatic trigger, stream, playback, failure, and cooldown.
- Modify `src/main.cpp`: call stream and audio updates in the required order.
- Modify `src/app/display.cpp`, `src/net/web_dashboard.cpp`, and `src/app/diagnostics.cpp`: show Realtime status and remove browser speech playback from the primary path.
- Modify `src/net/secrets_config.h` and `src/net/secrets_example.h`: add the ignored relay URL setting.
- Modify `platformio.ini`: add the WebSockets dependency.

### Tests and docs

- Create `test/voice_capture_core_test.cpp`.
- Create `test/pcm_ring_buffer_test.cpp`.
- Create `test/voice_stream_protocol_test.cpp`.
- Create `test/xiaozhi_realtime_core_test.cpp`.
- Modify `test/web_voice_prompt_test.cpp`: assert the dashboard no longer speaks cloud replies through the browser.
- Create `test/doubao_realtime_integration_test.cpp`: source-level wiring and secret-safety checks.
- Modify `README.md` and `docs/competition.md`: describe setup, run, and acceptance commands.

---

### Task 1: Doubao Binary Protocol Codec

**Files:**
- Create: `tools/doubao_relay/__init__.py`
- Create: `tools/doubao_relay/doubao_protocol.py`
- Create: `tools/doubao_relay/test_doubao_protocol.py`
- Create: `tools/doubao_relay/requirements.txt`

- [ ] **Step 1: Write exact failing frame tests**

Create tests that require these public definitions:

```python
from tools.doubao_relay.doubao_protocol import (
    AUDIO_REQUEST,
    AUDIO_RESPONSE,
    FULL_CLIENT_REQUEST,
    FULL_SERVER_RESPONSE,
    Frame,
    ProtocolError,
    decode_frame,
    encode_event,
)


def test_start_connection_matches_official_fixture():
    encoded = encode_event(
        message_type=FULL_CLIENT_REQUEST,
        event=1,
        payload=b"{}",
        serialization="json",
    )
    assert list(encoded) == [17, 20, 16, 0, 0, 0, 0, 1, 0, 0, 0, 2, 123, 125]


def test_task_request_carries_session_and_raw_pcm():
    encoded = encode_event(
        message_type=AUDIO_REQUEST,
        event=200,
        session_id="session-1",
        payload=b"\x01\x00\xff\x7f",
        serialization="raw",
    )
    frame = decode_frame(encoded)
    assert frame.event == 200
    assert frame.session_id == "session-1"
    assert frame.payload == b"\x01\x00\xff\x7f"
    assert frame.message_type == AUDIO_REQUEST


def test_decode_rejects_truncated_payload():
    bad = bytes([0x11, 0x94, 0x10, 0x00, 0, 0, 1, 0, 0, 0, 5, 1, 2])
    try:
        decode_frame(bad)
    except ProtocolError as exc:
        assert "truncated" in str(exc).lower()
    else:
        raise AssertionError("truncated frame accepted")
```

- [ ] **Step 2: Run the test and verify RED**

Run:

```powershell
python -m unittest tools.doubao_relay.test_doubao_protocol -v
```

Expected: `ModuleNotFoundError: No module named 'doubao_protocol'` or missing exported names.

- [ ] **Step 3: Implement the minimal codec**

Implement these exact interfaces:

```python
from dataclasses import dataclass
import struct

FULL_CLIENT_REQUEST = 0x1
AUDIO_REQUEST = 0x2
FULL_SERVER_RESPONSE = 0x9
AUDIO_RESPONSE = 0xB
ERROR_RESPONSE = 0xF
EVENT_FLAG = 0x4


class ProtocolError(ValueError):
    pass


@dataclass(frozen=True)
class Frame:
    message_type: int
    event: int
    session_id: str | None
    payload: bytes
    serialization: str


def encode_event(*, message_type: int, event: int, payload: bytes,
                 session_id: str | None = None,
                 serialization: str = "json") -> bytes:
    serialization_nibble = 1 if serialization == "json" else 0
    header = bytes([0x11, (message_type << 4) | EVENT_FLAG,
                    serialization_nibble << 4, 0x00])
    optional = struct.pack(">I", event)
    if session_id is not None:
        encoded_id = session_id.encode("utf-8")
        optional += struct.pack(">I", len(encoded_id)) + encoded_id
    return header + optional + struct.pack(">I", len(payload)) + payload


def decode_frame(data: bytes) -> Frame:
    session_events = {
        100, 102, 150, 152, 153, 154, 200, 251, 350, 351,
        352, 359, 450, 451, 459, 550, 559, 599,
    }
    if len(data) < 12:
        raise ProtocolError("truncated frame header")
    if data[0] >> 4 != 1 or data[0] & 0x0F != 1:
        raise ProtocolError("unsupported protocol header")
    message_type = data[1] >> 4
    flags = data[1] & 0x0F
    serialization = "json" if data[2] >> 4 == 1 else "raw"
    offset = 4
    if message_type == ERROR_RESPONSE:
        code = struct.unpack_from(">I", data, offset)[0]
        offset += 4
        event = code
    else:
        if flags != EVENT_FLAG:
            raise ProtocolError("event flag is required")
        event = struct.unpack_from(">I", data, offset)[0]
        offset += 4
    session_id = None
    if event in session_events:
        if len(data) < offset + 4:
            raise ProtocolError("truncated session id length")
        session_size = struct.unpack_from(">I", data, offset)[0]
        offset += 4
        if len(data) < offset + session_size:
            raise ProtocolError("truncated session id")
        try:
            session_id = data[offset:offset + session_size].decode("utf-8")
        except UnicodeDecodeError as exc:
            raise ProtocolError("invalid session id") from exc
        offset += session_size
    if len(data) < offset + 4:
        raise ProtocolError("truncated payload length")
    payload_size = struct.unpack_from(">I", data, offset)[0]
    offset += 4
    if len(data) != offset + payload_size:
        raise ProtocolError("truncated or trailing payload")
    return Frame(message_type, event, session_id,
                 data[offset:offset + payload_size], serialization)
```

The completed `decode_frame` must determine session-level events from the explicit set used by this project: `{100, 102, 150, 152, 153, 154, 200, 251, 350, 351, 352, 359, 450, 451, 459, 550, 559, 599}`. It must use `struct.unpack_from(">I", data, offset)`, reject invalid UTF-8 IDs, and reject trailing or truncated payload bytes.

- [ ] **Step 4: Run protocol tests and verify GREEN**

Run:

```powershell
python -m unittest tools.doubao_relay.test_doubao_protocol -v
```

Expected: all three tests pass.

- [ ] **Step 5: Pin the only Python dependency**

Create an empty `__init__.py` and create `requirements.txt` containing:

```text
websockets==16.0
```

- [ ] **Step 6: Commit the codec**

```powershell
git add tools/doubao_relay/__init__.py tools/doubao_relay/doubao_protocol.py tools/doubao_relay/test_doubao_protocol.py tools/doubao_relay/requirements.txt
git commit -m "test: define Doubao realtime frame protocol"
```

---

### Task 2: Bridge Authentication Probe and Session State

**Files:**
- Create: `tools/doubao_relay/board_protocol.py`
- Create: `tools/doubao_relay/doubao_client.py`
- Create: `tools/doubao_relay/server.py`
- Create: `tools/doubao_relay/test_board_protocol.py`
- Create: `tools/doubao_relay/test_session.py`
- Create: `tools/doubao_relay/.env.example`
- Modify: `.gitignore`

- [ ] **Step 1: Write failing board-protocol tests**

```python
from tools.doubao_relay.board_protocol import BoardMessageError, parse_control_message


def test_start_turn_requires_bounded_home_state():
    message = parse_control_message(
        '{"type":"start_turn","protocol":1,"home":{"temp_c":27.0,"humidity":65}}'
    )
    assert message["type"] == "start_turn"
    assert message["home"]["temp_c"] == 27.0


def test_unknown_type_is_rejected():
    try:
        parse_control_message('{"type":"execute_shell"}')
    except BoardMessageError as exc:
        assert "type" in str(exc).lower()
    else:
        raise AssertionError("unknown board message accepted")
```

- [ ] **Step 2: Write failing session-order tests**

```python
from tools.doubao_relay.doubao_client import RealtimeSession, SessionError, SessionPhase


def test_session_accepts_expected_server_order():
    session = RealtimeSession("session-1")
    session.on_event(150, b'{"dialog_id":"dialog-1"}')
    assert session.phase is SessionPhase.STREAMING
    session.on_event(451, b'{"results":[{"text":"你好","is_interim":false}]}')
    assert session.last_asr == "你好"
    session.on_event(550, b'{"content":"你好呀"}')
    assert session.reply_text == "你好呀"
    session.on_event(359, b'{}')
    assert session.phase is SessionPhase.COMPLETE


def test_audio_before_session_started_is_rejected():
    session = RealtimeSession("session-1")
    try:
        session.on_audio(b"\x00\x00")
    except SessionError as exc:
        assert "session" in str(exc).lower()
    else:
        raise AssertionError("audio accepted before SessionStarted")
```

- [ ] **Step 3: Run both test modules and verify RED**

```powershell
python -m unittest tools.doubao_relay.test_board_protocol tools.doubao_relay.test_session -v
```

Expected: imports fail because bridge modules do not exist.

- [ ] **Step 4: Implement strict board control parsing**

`parse_control_message(text: str) -> dict` must:

```python
ALLOWED_TYPES = {"hello", "start_turn", "cancel"}
MAX_CONTROL_BYTES = 2048
PROTOCOL_VERSION = 1
```

It must reject oversized input, malformed JSON, unknown fields at the top level, unsupported protocol versions, and a `home` object whose JSON encoding exceeds 512 bytes.

- [ ] **Step 5: Implement normalized Realtime session state**

Define:

```python
import json
from enum import Enum, auto


class SessionError(RuntimeError):
    pass


class SessionPhase(Enum):
    STARTING = auto()
    STREAMING = auto()
    THINKING = auto()
    SPEAKING = auto()
    COMPLETE = auto()
    FAILED = auto()


class RealtimeSession:
    def __init__(self, session_id: str):
        self.session_id = session_id
        self.phase = SessionPhase.STARTING
        self.dialog_id = ""
        self.last_asr = ""
        self.reply_text = ""

    def start_payload(self, home: dict) -> bytes:
        role = (
            "你是小智智能家居助手。请用简短自然的中文回答。"
            f"当前室内温度{home.get('temp_c', 0):.1f}度，"
            f"湿度{home.get('humidity', 0):.0f}%。"
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
        body = json.loads(payload.decode("utf-8") or "{}")
        if event == 150:
            self.dialog_id = body.get("dialog_id", "")
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
```

`start_payload` must select model `1.2.1.1`, speaker `zh_female_vv_jupiter_bigtts`, input PCM S16LE mono 16 kHz, output `pcm_s16le` mono 24 kHz, `end_smooth_window_ms: 900`, non-null `asr.extra` and `tts.extra`, and a concise Chinese home-assistant role containing the supplied sensor snapshot.

- [ ] **Step 6: Implement authentication header selection and live probe**

Define:

```python
def build_headers(env: Mapping[str, str], connect_id: str) -> dict[str, str]:
    common = {
        "X-Api-Resource-Id": "volc.speech.dialog",
        "X-Api-App-Key": "PlgvMymc7f3tQnJ6",
        "X-Api-Connect-Id": connect_id,
    }
    if env.get("DOUBAO_API_KEY"):
        return common | {"X-Api-Key": env["DOUBAO_API_KEY"]}
    return common | {
        "X-Api-App-ID": env["DOUBAO_APP_ID"],
        "X-Api-Access-Key": env["DOUBAO_ACCESS_KEY"],
    }
```

Add `python -m tools.doubao_relay.server --probe` that calls `websockets.asyncio.client.connect(REALTIME_URL, additional_headers=headers, max_size=None)`, sends `StartConnection`, requires event 50 within eight seconds, sends `FinishConnection`, and prints only an outcome such as `AUTH OK logid=20260709023000123456` or `AUTH FAIL code=401`.

- [ ] **Step 7: Implement the board WebSocket server**

Use `websockets.asyncio.server.serve` on `0.0.0.0:8765`, path `/voice`, and `max_size=2 * 1024 * 1024`. One coroutine owns all writes to the Doubao socket. Binary board messages are accepted only during `STREAMING`; normalized JSON and TTS binary frames are sent back to the board.

- [ ] **Step 8: Add secret-safe environment files**

`.env.example` must contain empty values:

```dotenv
DOUBAO_API_KEY=
DOUBAO_APP_ID=
DOUBAO_ACCESS_KEY=
DOUBAO_BIND_HOST=0.0.0.0
DOUBAO_BIND_PORT=8765
```

Append these ignore rules:

```gitignore
tools/doubao_relay/.env
tools/doubao_relay/__pycache__/
tools/doubao_relay/.venv/
```

- [ ] **Step 9: Run bridge unit tests and verify GREEN**

```powershell
python -m unittest discover -s tools/doubao_relay -t . -p "test_*.py" -v
```

Expected: all bridge tests pass without opening a network connection.

- [ ] **Step 10: Commit bridge behavior**

```powershell
git add .gitignore tools/doubao_relay
git commit -m "feat: add Doubao realtime protocol bridge"
```

---

### Task 3: Microphone PCM Conversion and Pre-Roll

**Files:**
- Create: `src/io/voice_capture_core.h`
- Create: `src/io/voice_capture_core.cpp`
- Create: `test/voice_capture_core_test.cpp`
- Modify: `src/board/config.h`

- [ ] **Step 1: Write failing PCM and pre-roll tests**

```cpp
#include <cassert>
#include "../src/io/voice_capture_core.h"

int main() {
  const int32_t stereo[] = {
      0, 0x00100000, 0, 0x00200000,
      0, (int32_t)0xffe00000, 0, (int32_t)0xfff00000};
  assert(selectVoiceI2sSlot(stereo, 8) == 1);

  int16_t pcm[4] = {};
  assert(convertVoiceI2sToPcm16(stereo, 8, 1, 8, pcm, 4) == 4);
  assert(pcm[0] > 0);
  assert(pcm[2] < 0);

  int16_t storage[5] = {};
  VoicePreRoll ring(storage, 5);
  const int16_t first[] = {1, 2, 3};
  const int16_t second[] = {4, 5, 6, 7};
  ring.push(first, 3);
  ring.push(second, 4);
  int16_t copied[5] = {};
  assert(ring.copyChronological(copied, 5) == 5);
  const int16_t expected[] = {3, 4, 5, 6, 7};
  for (int i = 0; i < 5; ++i) assert(copied[i] == expected[i]);
}
```

- [ ] **Step 2: Compile and verify RED**

```powershell
g++ -std=c++17 test\voice_capture_core_test.cpp src\io\voice_capture_core.cpp -o .pio\voice_capture_core_test.exe
```

Expected: compilation fails because the new files and symbols do not exist.

- [ ] **Step 3: Implement the portable capture core**

Expose:

```cpp
int selectVoiceI2sSlot(const int32_t *samples, size_t sampleCount);
size_t convertVoiceI2sToPcm16(const int32_t *samples, size_t sampleCount,
                              uint8_t slot, uint8_t rightShift,
                              int16_t *out, size_t outCapacity);

class VoicePreRoll {
 public:
  VoicePreRoll(int16_t *storage, size_t capacity);
  void clear();
  void push(const int16_t *samples, size_t count);
  size_t copyChronological(int16_t *out, size_t outCapacity) const;
  size_t size() const;
};
```

Slot selection compares 64-bit accumulated absolute magnitudes. Conversion clamps to `INT16_MIN..INT16_MAX`. The ring never allocates memory and overwrites oldest samples only after capacity is full.

- [ ] **Step 4: Add exact audio constants**

```cpp
constexpr uint32_t VOICE_INPUT_SAMPLE_RATE = 16000;
constexpr uint32_t VOICE_OUTPUT_SAMPLE_RATE = 24000;
constexpr uint16_t VOICE_PACKET_MS = 20;
constexpr size_t VOICE_PACKET_SAMPLES = 320;
constexpr size_t VOICE_PACKET_BYTES = 640;
constexpr uint16_t VOICE_PREROLL_MS = 300;
constexpr size_t VOICE_PREROLL_SAMPLES = 4800;
constexpr uint8_t VOICE_PCM_RIGHT_SHIFT = 8;
```

- [ ] **Step 5: Compile, run, and verify GREEN**

```powershell
g++ -std=c++17 test\voice_capture_core_test.cpp src\io\voice_capture_core.cpp -o .pio\voice_capture_core_test.exe
.\.pio\voice_capture_core_test.exe
```

Expected: exit code 0.

- [ ] **Step 6: Commit capture core**

```powershell
git add src/io/voice_capture_core.* src/board/config.h test/voice_capture_core_test.cpp
git commit -m "feat: add voice PCM capture core"
```

---

### Task 4: Bounded Streaming Speaker Queue

**Files:**
- Create: `src/board/pcm_ring_buffer.h`
- Create: `src/board/pcm_ring_buffer.cpp`
- Create: `src/board/streaming_speaker.h`
- Create: `src/board/streaming_speaker.cpp`
- Create: `test/pcm_ring_buffer_test.cpp`
- Modify: `src/board/hardware.h`
- Modify: `src/board/hardware.cpp:114`

- [ ] **Step 1: Write the failing bounded-queue test**

```cpp
#include <cassert>
#include "../src/board/pcm_ring_buffer.h"

int main() {
  int16_t storage[5] = {};
  PcmRingBuffer queue(storage, 5);
  const int16_t a[] = {1, 2, 3, 4};
  const int16_t b[] = {5, 6};
  assert(queue.push(a, 4) == 4);
  assert(queue.push(b, 2) == 1);
  assert(queue.overflowed());
  int16_t out[5] = {};
  assert(queue.pop(out, 5) == 5);
  for (int i = 0; i < 5; ++i) assert(out[i] == i + 1);
  assert(queue.empty());
}
```

- [ ] **Step 2: Compile and verify RED**

```powershell
g++ -std=c++17 test\pcm_ring_buffer_test.cpp src\board\pcm_ring_buffer.cpp -o .pio\pcm_ring_buffer_test.exe
```

Expected: compilation fails because the ring buffer does not exist.

- [ ] **Step 3: Implement the no-allocation ring**

Expose `push`, `pop`, `size`, `free`, `empty`, `clear`, and `overflowed`. `push` returns the accepted count and never overwrites queued response audio.

- [ ] **Step 4: Compile, run, and verify GREEN**

```powershell
g++ -std=c++17 test\pcm_ring_buffer_test.cpp src\board\pcm_ring_buffer.cpp -o .pio\pcm_ring_buffer_test.exe
.\.pio\pcm_ring_buffer_test.exe
```

Expected: exit code 0.

- [ ] **Step 5: Implement the ESP32 streaming wrapper**

Expose:

```cpp
bool setupStreamingSpeaker(size_t sampleCapacity);
bool beginStreamingSpeaker(uint32_t sampleRate);
size_t queueStreamingSpeakerPcm(const uint8_t *littleEndianBytes, size_t byteCount);
void updateStreamingSpeaker();
void endStreamingSpeaker();
bool streamingSpeakerActive();
bool streamingSpeakerOverflowed();
```

Allocate the queue with `heap_caps_malloc(sampleCapacity * sizeof(int16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT)`. Switch I2S1 using `i2s_set_clk(I2S_NUM_1, sampleRate, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_STEREO)`. Duplicate each mono sample to the existing `SpeakerToneFrame` output chunk. Restore `SPEAKER_SAMPLE_RATE` when streaming ends so local tones still work.

- [ ] **Step 6: Integrate speaker ownership**

`queueSpeakerTone` and `queueSpeakerPcmClip` must call `endStreamingSpeaker()` first. `beginStreamingSpeaker` must cancel tone/clip playback first. `isSpeakerTonePlaying()` must include streaming playback so existing microphone suppression stays correct.

- [ ] **Step 7: Build firmware and verify the wrapper compiles**

```powershell
platformio run
```

Expected: `[SUCCESS]` for `esp32-s3-devkitc-1-n16r8`.

- [ ] **Step 8: Commit streaming output**

```powershell
git add src/board/pcm_ring_buffer.* src/board/streaming_speaker.* src/board/hardware.* test/pcm_ring_buffer_test.cpp
git commit -m "feat: stream PCM replies to MAX98357A"
```

---

### Task 5: Firmware Voice Stream Protocol and WebSocket Client

**Files:**
- Create: `src/net/voice_stream_protocol.h`
- Create: `src/net/voice_stream_protocol.cpp`
- Create: `src/net/voice_stream_client.h`
- Create: `src/net/voice_stream_client.cpp`
- Create: `src/io/voice_capture.h`
- Create: `src/io/voice_capture.cpp`
- Create: `test/voice_stream_protocol_test.cpp`
- Modify: `platformio.ini`
- Modify: `src/net/secrets_config.h`
- Modify: `src/net/secrets_example.h`
- Modify: `src/board/hardware.h/.cpp`
- Modify: `src/io/sensors.h/.cpp:57`

- [ ] **Step 1: Write failing control-message parser tests**

```cpp
#include <cassert>
#include <cstring>
#include "../src/net/voice_stream_protocol.h"

int main() {
  VoiceRelayMessage message{};
  assert(parseVoiceRelayMessage(
      "{\"type\":\"asr\",\"text\":\"ni hao\"}", message));
  assert(message.type == VoiceRelayMessageType::Asr);
  assert(std::strcmp(message.text, "ni hao") == 0);
  assert(!parseVoiceRelayMessage("{\"type\":\"unknown\"}", message));
  assert(!parseVoiceRelayMessage("not-json", message));
}
```

- [ ] **Step 2: Compile and verify RED**

```powershell
g++ -std=c++17 test\voice_stream_protocol_test.cpp src\net\voice_stream_protocol.cpp -o .pio\voice_stream_protocol_test.exe
```

Expected: compilation fails because the protocol files do not exist.

- [ ] **Step 3: Implement a portable bounded parser**

Define message types `Ready`, `Phase`, `Asr`, `Reply`, `Done`, and `Error`; store text in a fixed 192-byte buffer. Keep host parsing dependency-free by validating the exact flat JSON shapes generated by the bridge and decoding JSON string escapes. Reject text longer than the destination and unknown types.

- [ ] **Step 4: Compile, run, and verify GREEN**

```powershell
g++ -std=c++17 test\voice_stream_protocol_test.cpp src\net\voice_stream_protocol.cpp -o .pio\voice_stream_protocol_test.exe
.\.pio\voice_stream_protocol_test.exe
```

Expected: exit code 0.

- [ ] **Step 5: Add board dependencies and relay configuration**

Append to `lib_deps`:

```ini
    links2004/WebSockets
```

Add this ignored setting with an empty tracked default:

```cpp
#ifndef SMART_HOME_DOUBAO_RELAY_URL
#define SMART_HOME_DOUBAO_RELAY_URL ""
#endif
```

- [ ] **Step 6: Implement 20 ms I2S capture ownership**

Expose:

```cpp
bool setupVoiceCapture();
bool pollVoiceCapturePacket(int16_t *out, size_t outCapacity, size_t &sampleCount);
size_t copyVoicePreRoll(int16_t *out, size_t outCapacity);
void beginVoiceCaptureTurn();
void endVoiceCaptureTurn();
bool voiceCaptureOwnsMic();
```

`voice_capture.cpp` must allocate the 4,800-sample pre-roll in PSRAM and continuously read I2S0 through a new `readI2sMicFrames` hardware helper, including while XiaoZhi is idle. Each successful poll analyzes the frame with the existing microphone-processing functions, updates `state.micLevel`, `state.micThreshold`, and `state.soundTriggered`, then pushes converted samples into pre-roll. It locks the selected I2S slot for an active turn and emits exactly 320 mono samples per normal packet. `voice_stream_client.cpp` calls `pollVoiceCapturePacket` on every update, sends packets only during an active turn, and otherwise keeps them only for level detection and pre-roll. `readMicrophone()` returns immediately while `voiceCaptureOwnsMic()` is true, retaining its old implementation as a fallback if voice capture setup fails.

- [ ] **Step 7: Implement the board WebSocket client**

Expose:

```cpp
void setupVoiceStreamClient();
void updateVoiceStreamClient();
bool startVoiceStreamTurn();
void cancelVoiceStreamTurn(const char *reason);
bool voiceStreamReady();
bool voiceStreamTurnActive();
```

Parse `ws://host:port/path` once at setup. Use `WebSocketsClient::begin`, enable a five-second reconnect interval, send one JSON `hello` after connection, send `start_turn` with the current sensor snapshot, then send pre-roll and live PCM via `sendBIN`. Route binary server frames to `queueStreamingSpeakerPcm`. Route text frames through `parseVoiceRelayMessage` and copy only bounded text into `HubState`.

- [ ] **Step 8: Build firmware and verify client APIs**

```powershell
platformio run
```

Expected: successful compile and link with no undefined WebSocket or audio symbols.

- [ ] **Step 9: Commit the stream client**

```powershell
git add platformio.ini src/net/voice_stream_* src/io/voice_capture.* src/io/sensors.* src/board/hardware.* src/net/secrets_config.h src/net/secrets_example.h test/voice_stream_protocol_test.cpp
git commit -m "feat: stream board audio through relay"
```

---

### Task 6: XiaoZhi Realtime State Integration

**Files:**
- Create: `test/xiaozhi_realtime_core_test.cpp`
- Create: `test/doubao_realtime_integration_test.cpp`
- Modify: `src/app/xiaozhi_core.h`
- Modify: `src/app/xiaozhi_core.cpp`
- Modify: `src/app/xiaozhi_ai.h`
- Modify: `src/app/xiaozhi_ai.cpp:82`
- Modify: `src/app/hub_state.h:8`
- Modify: `src/main.cpp:56`
- Modify: `src/app/display.cpp`
- Modify: `src/app/diagnostics.cpp`
- Modify: `src/net/web_dashboard.cpp:93`
- Modify: `test/web_voice_prompt_test.cpp`

- [ ] **Step 1: Write failing deterministic transition tests**

```cpp
#include <cassert>
#include "../src/app/xiaozhi_core.h"

int main() {
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnStarted) ==
         XiaozhiPhase::Listening);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AsrEnded) ==
         XiaozhiPhase::Thinking);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::AudioStarted) ==
         XiaozhiPhase::Speaking);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::TurnDone) ==
         XiaozhiPhase::Idle);
  assert(xiaozhiPhaseForRelayEvent(XiaozhiRelayEvent::Failed) ==
         XiaozhiPhase::Idle);
}
```

- [ ] **Step 2: Compile and verify RED**

```powershell
g++ -std=c++17 test\xiaozhi_realtime_core_test.cpp src\app\xiaozhi_core.cpp -o .pio\xiaozhi_realtime_core_test.exe
```

Expected: compilation fails because `XiaozhiRelayEvent` and the transition function do not exist.

- [ ] **Step 3: Implement the minimal transition API**

Add `XiaozhiRelayEvent` with `TurnStarted`, `AsrEnded`, `AudioStarted`, `TurnDone`, and `Failed`. Implement a total switch returning the phases asserted above.

- [ ] **Step 4: Compile, run, and verify GREEN**

```powershell
g++ -std=c++17 test\xiaozhi_realtime_core_test.cpp src\app\xiaozhi_core.cpp -o .pio\xiaozhi_realtime_core_test.exe
.\.pio\xiaozhi_realtime_core_test.exe
```

Expected: exit code 0.

- [ ] **Step 5: Write the failing source-integration test**

The test must read source files and assert all of these strings exist:

```cpp
assert(ai.find("startVoiceStreamTurn") != std::string::npos);
assert(ai.find("voiceStreamTurnActive") != std::string::npos);
assert(mainSource.find("updateVoiceStreamClient") != std::string::npos);
assert(mainSource.find("updateStreamingSpeaker") != std::string::npos);
assert(dashboard.find("speechSynthesis.speak") == std::string::npos);
assert(secrets.find("SMART_HOME_DOUBAO_RELAY_URL") != std::string::npos);
```

- [ ] **Step 6: Run the integration test and verify RED**

```powershell
g++ -std=c++17 test\doubao_realtime_integration_test.cpp -o .pio\doubao_realtime_integration_test.exe
.\.pio\doubao_realtime_integration_test.exe
```

Expected: assertion failure because integration calls and dashboard cleanup are not complete.

- [ ] **Step 7: Replace the simulated XiaoZhi timing path**

`updateXiaozhiAi()` must:

1. Trigger only from `soundTriggered`, idle phase, configured relay, connected Wi-Fi, speaker idle, and cooldown elapsed.
2. Call `startVoiceStreamTurn()` and enter `Listening`.
3. React to relay callbacks for ASR, reply text, audio start, done, and error instead of elapsed `XIAOZHI_LISTEN_MS/XIAOZHI_THINK_MS/XIAOZHI_SPEAK_MS` timers.
4. Queue a short error tone for failures.
5. Set `xiaozhiMicMutedUntilMs` after every success or failure.
6. Leave the manual web text command available only as a visible diagnostic and never call DeepSeek from the automatic voice path.

- [ ] **Step 8: Update state, display, diagnostics, and dashboard**

Add booleans `doubaoRelayConfigured`, `doubaoRelayConnected`, and `doubaoSessionActive`, plus a bounded `xiaozhiErrorText`. Show `DOUBAO READY/OFFLINE` instead of `CLOUD CONFIGURED/LOCAL DEMO`. Add relay fields to `/api/state`. Remove `SpeechRecognition`, `SpeechSynthesisUtterance`, and `speechSynthesis.speak` from the dashboard so the browser cannot masquerade as board audio.

- [ ] **Step 9: Order loop updates for audio flow**

Use this sequence in `loop()`:

```cpp
updateVoiceStreamClient();
updateStreamingSpeaker();
updateSpeakerAudio();
readEnvironment();
readInputs();
updateAutomation();
updateAiGuard();
updateXiaozhiAi();
```

Keep the existing network/UI updates after those calls.

- [ ] **Step 10: Run integration tests and full build**

```powershell
g++ -std=c++17 test\doubao_realtime_integration_test.cpp -o .pio\doubao_realtime_integration_test.exe
.\.pio\doubao_realtime_integration_test.exe
platformio run
```

Expected: integration test exits 0 and firmware build succeeds.

- [ ] **Step 11: Commit state integration**

```powershell
git add src/app/xiaozhi_* src/app/hub_state.h src/main.cpp src/app/display.cpp src/app/diagnostics.cpp src/net/web_dashboard.cpp test/xiaozhi_realtime_core_test.cpp test/doubao_realtime_integration_test.cpp test/web_voice_prompt_test.cpp
git commit -m "feat: drive XiaoZhi from Doubao realtime events"
```

---

### Task 7: D:-Resident Bridge Setup and Documentation

**Files:**
- Create: `tools/doubao_relay/setup.ps1`
- Create: `tools/doubao_relay/run.ps1`
- Modify: `README.md`
- Modify: `docs/competition.md`

- [ ] **Step 1: Write the setup script**

The script must use:

```powershell
$InstallRoot = 'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay'
$Venv = Join-Path $InstallRoot '.venv'
$Python = 'D:\A-Soft\DevTools\Python313\python.exe'
New-Item -ItemType Directory -Force -Path $InstallRoot | Out-Null
& $Python -m venv $Venv
& (Join-Path $Venv 'Scripts\python.exe') -m pip install --upgrade pip
& (Join-Path $Venv 'Scripts\python.exe') -m pip install -r "$PSScriptRoot\requirements.txt"
```

Before installation it must print: `Estimated program/cache use: under 100 MB on D:`. After installation it must print the resolved Python path and `websockets.__version__`.

- [ ] **Step 2: Write the launch script**

`run.ps1` must parse non-empty `KEY=VALUE` lines from `tools/doubao_relay/.env`, set process-only environment variables, print LAN IPv4 addresses, and execute:

```powershell
$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot '..\..')
Push-Location $RepoRoot
try {
    & 'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay\.venv\Scripts\python.exe' -m tools.doubao_relay.server
} finally {
    Pop-Location
}
```

It must fail with a concise message if `.env` or the virtual environment is missing.

- [ ] **Step 3: Document credential rotation and configuration**

Document this exact order:

1. Delete the API key exposed in screenshots.
2. Create a replacement key and put it only in `tools/doubao_relay/.env`.
3. Run `setup.ps1` once.
4. Run `python -m tools.doubao_relay.server --probe` and require `AUTH OK`.
5. Put the printed LAN bridge URL in ignored `src/net/secrets.h`.
6. Start `run.ps1`, then build/upload firmware.

- [ ] **Step 4: Document the physical acceptance path**

Include the unchanged pins:

```text
INMP441: BCLK GPIO15, WS GPIO39, SD GPIO40
MAX98357A: DIN GPIO47, BCLK GPIO42, LRC GPIO38
```

Include expected serial markers: `[VOICE] relay connected`, `[DOUBAO] session started`, `[DOUBAO] asr=`, `[DOUBAO] reply=`, and `[VOICE] playback complete`.

- [ ] **Step 5: Commit setup and docs**

```powershell
git add tools/doubao_relay/setup.ps1 tools/doubao_relay/run.ps1 README.md docs/competition.md
git commit -m "docs: add Doubao voice setup runbook"
```

---

### Task 8: End-to-End Verification, Upload, and Recovery Test

**Files:**
- Modify only if a failing verification exposes a tested defect.

- [ ] **Step 1: Run every Python bridge test**

```powershell
python -m unittest discover -s tools/doubao_relay -t . -p "test_*.py" -v
```

Expected: all tests pass and no credential values appear.

- [ ] **Step 2: Build and run all new host C++ tests**

```powershell
g++ -std=c++17 test\voice_capture_core_test.cpp src\io\voice_capture_core.cpp -o .pio\voice_capture_core_test.exe
g++ -std=c++17 test\pcm_ring_buffer_test.cpp src\board\pcm_ring_buffer.cpp -o .pio\pcm_ring_buffer_test.exe
g++ -std=c++17 test\voice_stream_protocol_test.cpp src\net\voice_stream_protocol.cpp -o .pio\voice_stream_protocol_test.exe
g++ -std=c++17 test\xiaozhi_realtime_core_test.cpp src\app\xiaozhi_core.cpp -o .pio\xiaozhi_realtime_core_test.exe
g++ -std=c++17 test\doubao_realtime_integration_test.cpp -o .pio\doubao_realtime_integration_test.exe
Get-ChildItem .pio\*realtime*test.exe,.pio\voice_capture_core_test.exe,.pio\pcm_ring_buffer_test.exe,.pio\voice_stream_protocol_test.exe | ForEach-Object { & $_.FullName; if ($LASTEXITCODE -ne 0) { throw "failed: $($_.Name)" } }
```

Expected: every executable exits 0.

- [ ] **Step 3: Run existing regression executables**

Recompile the existing tests touched by shared files, then run them:

```powershell
g++ -std=c++17 test\mic_processing_test.cpp src\io\mic_processing.cpp -o .pio\mic_processing_test.exe
g++ -std=c++17 test\speaker_pcm_test.cpp src\board\speaker_tone.cpp -o .pio\speaker_pcm_test.exe
g++ -std=c++17 test\speaker_tone_test.cpp src\board\speaker_tone.cpp -o .pio\speaker_tone_test.exe
g++ -std=c++17 test\xiaozhi_core_test.cpp src\app\xiaozhi_core.cpp -o .pio\xiaozhi_core_test.exe
.\.pio\mic_processing_test.exe
.\.pio\speaker_pcm_test.exe
.\.pio\speaker_tone_test.exe
.\.pio\xiaozhi_core_test.exe
```

Expected: all four exit 0.

- [ ] **Step 4: Run the credential leak scan**

```powershell
git grep -n -E "5716bf92|DOUBAO_API_KEY=.+|DOUBAO_ACCESS_KEY=.+" -- ':!docs/superpowers' ':!tools/doubao_relay/.env.example'
```

Expected: no output.

- [ ] **Step 5: Build release firmware**

```powershell
platformio run
```

Expected: success with flash below 100% and RAM below 100%.

- [ ] **Step 6: Rotate the exposed key and probe Doubao**

After the replacement credential is stored only in ignored `.env`:

```powershell
& 'D:\A-Soft\DevTools\SmartHomeHubVoiceRelay\.venv\Scripts\python.exe' -m tools.doubao_relay.server --probe
```

Expected: `AUTH OK` without printing the key. If new-console `X-Api-Key` auth is rejected, populate the legacy App ID and Access Key shown under the same project's resource details and rerun the same probe.

- [ ] **Step 7: Start the bridge and upload firmware**

Start `tools\doubao_relay\run.ps1` in one terminal. In another:

```powershell
platformio device list
platformio run --target upload
platformio device monitor --baud 115200
```

Expected: upload succeeds to the detected board port, then serial reports relay connected and audio hardware ready.

- [ ] **Step 8: Verify one physical conversation**

Speak one short Chinese sentence near INMP441. Require all of:

```text
[DOUBAO] session started
[DOUBAO] asr=你好小智
[DOUBAO] reply=你好，我在呢。
[VOICE] playback complete
```

The reply must be audible through MAX98357A and silent on the browser/PC speaker.

- [ ] **Step 9: Verify echo suppression and recovery**

During playback, confirm no second turn starts. Stop the bridge, speak once, require an error tone without reboot, restart the bridge, then require the next spoken turn to succeed.

- [ ] **Step 10: Commit verification-only fixes and record final status**

If verification required code fixes, each fix must have a reproducing test and a focused commit. Finish with:

```powershell
git status --short
git log --oneline -8
```

Expected: clean worktree and the Doubao implementation commits visible.
