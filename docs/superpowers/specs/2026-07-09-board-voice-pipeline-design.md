# Doubao Realtime Board Voice Pipeline Design

## Goal

Build a complete physical voice loop in which the board captures and plays all audio while Doubao Realtime performs speech recognition, reasoning, and speech synthesis:

`INMP441 -> ESP32-S3 -> lightweight LAN bridge -> Doubao Realtime -> ESP32-S3 -> MAX98357A`

The browser speech APIs and DeepSeek are not part of the primary voice conversation. The Windows PC only translates protocols and forwards byte streams; it does not run an AI model or play conversation audio.

## Constraints

- Keep the existing ESP32-S3 N16R8 pin assignments.
- Use the existing INMP441 and MAX98357A hardware for input and output.
- Start a conversation automatically from microphone activity; no button is required.
- Use Doubao Realtime O2.0 for ASR, reasoning, dialogue context, and TTS.
- Run a lightweight bridge on a Windows PC connected to the same LAN for the first competition-ready version.
- Keep credentials in ignored local configuration and never print complete credentials.
- Prefer a robust half-duplex demo over interruption and full-duplex echo cancellation.

## Selected Architecture

### ESP32-S3 firmware

While idle, the firmware continuously analyzes the INMP441 level and keeps about 300 ms of 16 kHz, 16-bit, mono PCM in a PSRAM circular pre-roll buffer. Consecutive samples above the adaptive threshold start a turn. The pre-roll is sent first so the beginning of the first word is retained.

During a turn, the board sends one 640-byte PCM packet every 20 ms to the LAN bridge. The board receives status, recognized text, reply text, and 24 kHz, 16-bit, mono PCM audio from the bridge. Returned PCM is buffered in PSRAM and streamed through I2S1 to MAX98357A.

This version is half-duplex. Microphone upload stops while response audio is playing. A short cooldown after playback prevents speaker output from retriggering the microphone.

### Windows protocol bridge

A small Python server runs on the PC. It accepts one board WebSocket connection and maintains the Doubao Realtime WebSocket connection at:

`wss://openspeech.bytedance.com/api/v3/realtime/dialogue`

The bridge implements Doubao's binary event framing, authentication headers, session lifecycle, and reconnect policy. It forwards the board's PCM as `TaskRequest` event 200 and forwards these server events back to the board:

- `ASRResponse` 451 for recognized text.
- `ASREnded` 459 for end-of-user-speech state.
- `ChatResponse` 550 for reply text.
- `TTSResponse` 352 for generated PCM.
- `TTSEnded` 359 for end-of-playback state.
- Connection, session, usage, and error events for diagnostics.

No PortAudio, Whisper model, local TTS model, or PC speaker is used. The Python environment and package cache stay on D: and are expected to use less than 100 MB.

### Doubao session

Each board turn creates a fresh Doubao session so current home state can be included in the system role. The session uses:

- Model: O2.0, protocol model value `1.2.1.1`.
- Speaker: `zh_female_vv_jupiter_bigtts` (Vivi/vv style).
- Input: PCM S16LE, mono, 16 kHz.
- Output: `pcm_s16le`, mono, 24 kHz.
- Server VAD with a configurable end-of-speech window near 900 ms.
- Concise Chinese smart-home-assistant role and speaking style.

The bridge sends `StartConnection`, waits for `ConnectionStarted`, sends `StartSession`, waits for `SessionStarted`, streams audio, and ends the session cleanly after `TTSEnded`. A later turn may reuse the WebSocket connection with a new session.

## Components

- `voice_capture_core`: portable I2S slot selection, 32-bit-to-16-bit conversion, threshold decisions, and pre-roll indexing.
- `voice_capture`: I2S0 reads, PSRAM pre-roll, and 20 ms board audio packets.
- `voice_stream_protocol`: the small board-to-bridge control and binary-audio protocol.
- `voice_stream_client`: ESP32 LAN WebSocket connection, send queue, receive dispatch, and reconnect.
- `streaming_speaker`: PSRAM-backed 24 kHz PCM queue and I2S1 playback ownership.
- `xiaozhi_ai`: idle, listening, thinking, speaking, error, and cooldown orchestration.
- `tools/doubao_relay`: Python bridge, Doubao binary frame codec, session state, setup, launch, and protocol tests.

Portable conversion and framing logic remain separate from Arduino networking so they can be tested on the host.

## Board-To-Bridge Protocol

The board opens a WebSocket to the bridge and sends:

- JSON `hello` with firmware protocol version and current home state.
- JSON `start_turn` when local sound detection triggers.
- Binary 16 kHz PCM packets, each representing 20 ms where possible.
- JSON `cancel` when the turn times out or local hardware fails.

The bridge sends:

- JSON `ready`, `phase`, `asr`, `reply`, `done`, and `error` messages.
- Binary 24 kHz PCM audio packets only between the response start and `done` message.

Both sides reject unsupported protocol versions and bounded-message violations instead of interpreting ambiguous data.

## Data Flow

1. Idle microphone analysis fills pre-roll and crosses the adaptive threshold.
2. Firmware opens or reuses the bridge connection and sends `start_turn` with home status.
3. The bridge starts a Doubao O2.0 session with the assistant role and PCM TTS configuration.
4. Firmware sends pre-roll followed by live 20 ms PCM packets.
5. Doubao server VAD detects speech and end of speech; ASR text is returned to the board display.
6. Doubao reasons over the speech and role context; reply text is returned to the board display.
7. Doubao sends 24 kHz PCM chunks; the board queues and plays them through MAX98357A.
8. `TTSEnded` completes the turn, frees buffers, starts microphone cooldown, and returns to idle.

## Failure Handling

- Missing microphone, speaker, Wi-Fi, bridge URL, or PSRAM disables automatic voice mode and reports one actionable serial status.
- Bridge connection failure emits an error tone and retries with bounded backoff on the next turn.
- Doubao authentication or session failure returns a sanitized error code; credentials are never echoed.
- Malformed frames, unsupported audio format, queue overflow, and allocation failure cancel the current turn and release all owned buffers.
- A turn has finite connection, first-response, and total-duration timeouts.
- The bridge sends `FinishSession` before disconnecting whenever the service is responsive.
- Speaker playback always mutes microphone upload and is followed by cooldown, including after partial or failed audio output.

## Configuration

Tracked defaults live in `secrets_config.h`. Ignored local settings provide the bridge URL. Doubao credentials remain on the PC bridge rather than in ESP32 flash.

Example board setting:

```cpp
#define SMART_HOME_DOUBAO_RELAY_URL "ws://192.168.1.20:8765/voice"
```

The bridge uses an ignored `.env` containing the current Doubao authentication values required by the console. The API key exposed in screenshots must be revoked and replaced before testing.

## Testing And Acceptance

Tests are written first and observed failing before production implementation. They cover:

- Active I2S slot selection, PCM conversion, clipping, and 20 ms packet size.
- Pre-roll wraparound and chronological extraction.
- Board-to-bridge JSON and binary message validation.
- Doubao big-endian event frame encoding and decoding for all used event types.
- Session state transitions, invalid ordering, timeouts, and reconnect behavior.
- Streaming PCM queue ownership, overflow policy, and playback completion.
- Source integration expectations and a full PlatformIO build.

Live acceptance requires:

1. Serial reports microphone, speaker, PSRAM, Wi-Fi, bridge, and Doubao session readiness.
2. Speaking near INMP441 automatically starts a turn without a browser or button.
3. The first word is preserved and recognized text appears on the display and dashboard.
4. Doubao reply text appears and the complete Vivi/vv response is audible only through MAX98357A.
5. Speaker audio does not trigger a second turn.
6. Disconnecting the bridge produces an error tone and the board recovers without rebooting.
7. No complete API key appears in source control, serial output, dashboard output, or test logs.

## Out Of Scope

- ESP32 direct connection to the Doubao binary WebSocket in the first release.
- Full-duplex interruption and acoustic echo cancellation.
- A trained local wake word.
- Local or browser-based ASR, LLM, and TTS.
- Voice cloning and paid custom speakers.
