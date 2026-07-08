# Board Voice Pipeline Design

## Goal

Turn the existing XiaoZhi demo into a complete physical voice loop:

`INMP441 -> ESP32-S3 -> speech recognition -> DeepSeek -> speech synthesis -> MAX98357A`

The browser speech APIs are no longer part of the primary interaction. A Windows PC on the same Wi-Fi network may run a local speech relay during the competition demo.

## Constraints

- Keep the existing ESP32-S3 N16R8 pin assignments.
- Keep DeepSeek as the text LLM through the existing cloud client.
- Start recording automatically from microphone activity; no push-to-talk button is required.
- Do not require another paid cloud credential for the first working version.
- Keep credentials in ignored local configuration and never expose them in tracked files or logs.
- Prefer a robust competition demo over continuous full-duplex conversation.

## Architecture

### ESP32-S3 firmware

The firmware owns the conversation and hardware state machine. It continuously measures the INMP441 level while idle. A level above the existing adaptive threshold starts a recording. Recording ends after about one second of silence or at an eight-second hard limit.

The recording is 16 kHz, 16-bit, mono PCM stored in PSRAM. The firmware sends it to the relay for ASR, passes the recognized Chinese text to the existing DeepSeek client, then sends the reply text to the relay for TTS. The returned 16 kHz mono WAV is validated, stored in PSRAM, and played through I2S1 and MAX98357A.

Microphone sampling is suspended while recording is consumed by the voice recorder and while speaker audio is active. A post-playback cooldown prevents the speaker from immediately retriggering the microphone.

### Windows speech relay

A small Python HTTP server runs on the PC and listens on port 8765:

- `GET /health` returns relay and model readiness.
- `POST /asr` accepts `audio/wav` and returns `{"text":"..."}`.
- `POST /tts` accepts `{"text":"..."}` and returns `audio/wav`.

ASR uses `faster-whisper` with the multilingual `small` model for useful Chinese accuracy. TTS uses the installed Windows Chinese voice through `System.Speech`, explicitly producing 16 kHz, 16-bit, mono WAV. The relay does not store recordings after each request completes.

The relay environment and model cache live under `D:\A-Soft\DevTools\SmartHomeHubVoice`. Expected disk use is approximately 0.8-1.5 GB, including the virtual environment, native runtime, model, and cache.

## Components

- `voice_capture_core`: portable sample conversion, active I2S slot selection, VAD timing, silence stop, and maximum-duration decisions.
- `voice_capture`: ESP32 I2S reads and PSRAM recording ownership.
- `wav_pcm`: RIFF/WAV validation and location of supported PCM data.
- `voice_relay`: HTTP health, ASR upload, and TTS download operations.
- `hardware`: playback of dynamically owned PCM and release after playback.
- `xiaozhi_ai`: orchestration across listening, recognizing, thinking, synthesizing, speaking, failure, and cooldown states.
- `tools/voice_relay`: PC setup, server, health check, and launch scripts.

Each component exposes a narrow interface so conversion, VAD, protocol parsing, and state transitions can be tested without ESP32 hardware.

## Data Flow

1. Idle microphone analysis crosses the adaptive threshold for consecutive reads.
2. Firmware allocates the bounded recording buffer in PSRAM and enters `LISTENING`.
3. I2S0 frames are converted from the active 32-bit INMP441 slot to clamped 16-bit mono PCM.
4. Silence or the eight-second limit completes the WAV.
5. Firmware posts the WAV to `/asr` and stores the recognized text as the XiaoZhi prompt.
6. Existing cloud code obtains a concise Chinese DeepSeek reply with current home status context.
7. Firmware posts the reply to `/tts`, validates the returned WAV, and queues its PCM for I2S1 playback.
8. Playback completion frees the PSRAM buffer, applies the microphone cooldown, and returns to idle.

## Failure Handling

- Missing microphone, speaker, Wi-Fi, relay URL, or PSRAM prevents automatic voice mode from starting and prints one actionable serial status.
- Relay timeout, empty ASR text, malformed JSON, unsupported WAV, or allocation failure aborts the current turn, emits a short error tone, records a concise event, and returns to idle after cooldown.
- DeepSeek failure retains the current local home-status reply fallback, which is still synthesized and spoken.
- Recording and downloaded speech have strict size limits to prevent memory exhaustion.
- HTTP requests use finite timeouts; no failure leaves the microphone permanently muted or a buffer owned after playback.

## Configuration

Tracked defaults live in `secrets_config.h`; the local ignored `secrets.h` sets only the relay URL, for example:

```cpp
#define SMART_HOME_VOICE_RELAY_URL "http://192.168.1.20:8765"
```

The PC launch script prints the detected LAN addresses and the exact URL to place in local configuration. Existing Wi-Fi and DeepSeek settings remain unchanged.

## Testing And Acceptance

Host tests are written first and must be observed failing before implementation. They cover:

- 32-bit I2S slot-to-mono conversion and clipping.
- VAD start, silence completion, and maximum duration.
- WAV parsing with valid, truncated, stereo, wrong-rate, and non-PCM inputs.
- Relay request formatting and response parsing.
- XiaoZhi integration source expectations and owned playback-buffer lifetime.
- Relay health, short WAV ASR request validation, and deterministic TTS WAV format where the Windows voice is available.

Acceptance on the live board requires:

1. Serial reports microphone, speaker, Wi-Fi, DeepSeek, PSRAM, and relay ready.
2. Speaking near INMP441 automatically enters listening and produces a non-empty recognized Chinese prompt.
3. Serial and display show the recognized prompt and cloud reply.
4. The complete generated reply is audible from MAX98357A, not the browser or PC speaker.
5. Speaker output does not retrigger another conversation during playback or cooldown.
6. Relay disconnection produces an error tone and recovers without rebooting.

## Out Of Scope

- A trained local wake word.
- Full-duplex interruption while XiaoZhi is speaking.
- Running ASR or TTS entirely on the ESP32-S3.
- Production authentication or TLS for the trusted same-LAN relay.
