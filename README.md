# APL - Audio Playback Library

[![APL — Audio Playback Library](assets/apl-banner.png)](assets/apl-banner.svg)

[![C++: 17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](#requirements)
[![Header-only](https://img.shields.io/badge/header--only-7B68EE)](apl.hpp)
[![Version: 1.0.1](https://img.shields.io/badge/version-1.0.1-7B68EE)](#version-101-scope)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platforms: Windows | Linux](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-0078D4)](#requirements)

**Audio Playback Library** is a C++17, single-header library for playing WAV and
MP3 files on Windows and Linux. Copy `apl.hpp` into your project and include it.
No implementation macro, separate library source file, or build system is needed.

APL implements its own decoders, mixer, and resampler. It uses the operating
system's audio APIs: WASAPI on Windows and ALSA on Linux. No third-party codec
libraries are required.

## Changes in 1.0.1

This patch fixes playback control handling without changing the public API:

- A seek requested together with play after natural completion keeps the requested
  position, in both memory and streaming modes, regardless of command order.
- A new loop count is preserved when natural completion occurs while a seek is
  still waiting for the internal ramp.

## Features

- WAV and MP3 playback, with automatic detection from the file contents.
- Full decoding into memory or background streaming for longer files.
- Play, pause, stop, seek, and playback status.
- Per-sound volume and master volume, including amplification above `1.0`.
- Infinite loops or a specified number of additional repetitions.
- Up to 128 loaded sounds per player, mixed automatically.
- Float32 stereo processing, cubic Hermite resampling, short internal ramps,
  output clamping, and dither for 16-bit output.
- Movable, non-copyable sound handles with automatic resource cleanup.
- No allocation, object destruction, file access, mutex acquisition, or waiting
  in the audio render function.

## Requirements

| Platform | Audio backend | Build requirements |
| --- | --- | --- |
| Windows 10/11 | WASAPI, shared mode with event-driven rendering | C++17 compiler and Windows SDK or MinGW-w64 headers; link `ole32` |
| Linux | ALSA, nonblocking interleaved playback | C++17 compiler, threads, dynamic loader, and the system `libasound.so.2` runtime |

MSVC, GCC/MinGW-w64, and Clang are supported compiler targets. Native lock-free
atomics and IEEE-754 32-bit `float` are required and checked at compile time.
macOS and other platforms do not have an audio backend in version 1.0.1;
`Player::init()` reports an error on those platforms.

On Linux, ALSA is loaded dynamically. ALSA development headers and `-lasound`
are not needed. The system ALSA configuration determines how playback reaches
the hardware, including PipeWire or PulseAudio when the appropriate plugins
are configured.

## Getting started

Place `apl.hpp` next to your source file:

```cpp
#include "apl.hpp"
```

The included [example.cpp](example.cpp) loads `audio.mp3`, plays it, and waits for
Enter before closing. Put your own `audio.mp3` in the directory you run from, or
change the filename in the example to your WAV or MP3 file. Audio files are not
bundled with this repository. Run the commands below from the repository directory.

The example returns `1` if initialization or loading fails. Use
`Player::lastError()` to get an error description; see [Errors](#errors).

### Windows: g++ / MinGW-w64

Use a MinGW-w64 distribution with `std::thread` support:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pthread example.cpp -o example.exe -lole32
.\example.exe
```

To use a WAV file, change `"audio.mp3"` to your WAV filename in `example.cpp`.

Only `example.cpp` is compiled: it includes `apl.hpp` automatically. For your
own application, replace `example.cpp` with your source files and keep the
platform flags. A Clang installation targeting MinGW can use `clang++` with
the same flags.

### Windows: MSVC

Run this in a Developer Command Prompt:

```bat
cl /nologo /std:c++17 /EHsc /W4 /permissive- /O2 example.cpp /Fe:example.exe
example.exe
```

The header links `ole32.lib` automatically through an MSVC pragma, also used by
`clang-cl`.

### Linux: g++

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pthread example.cpp -o example -ldl
./example
```

Use `clang++` in place of `g++` for Clang. `-ldl` is included for compatibility
with systems using glibc versions earlier than 2.34.

### Choosing a Linux output

APL opens the ALSA PCM named `default`. Override it for a process with the
`APL_ALSA_DEVICE` environment variable:

```sh
APL_ALSA_DEVICE=plughw:0,0 ./example
```

A hardware PCM can be occupied by another application. The distribution's
configured `default` is generally the appropriate choice for shared playback.

WSL, containers, and servers may have no default ALSA output. Errors such as
`cannot find card '0'` or `Unknown PCM default` indicate an unavailable audio
output. To run playback through ALSA's silent output:

```sh
APL_ALSA_DEVICE=null ./example
```

This processes and discards the audio in real time; it produces no audible
sound. Audible playback requires an ALSA output configured for that environment.
On WSL, you can also run the Windows build to hear the result through WASAPI.

## Basic playback

```cpp
#include "apl.hpp"
#include <iostream>

int main() {
    apl::Player player;
    if (!player.init()) return 1;

    auto sound = player.load("audio.mp3"); // Use your own WAV or MP3 file.
    if (!sound.isValid()) return 1;

    sound.play();
    std::cout << "Press Enter to close.\n";
    std::cin.get(); // Keep the program open while the sound plays.
}
```

The remaining snippets assume an initialized `apl::Player player` and valid
sound handles. Always check `isValid()` after loading a file.

## Loading audio

```cpp
auto effect = player.load("click.wav"); // Memory is the default.
auto another = player.load("voice.mp3", apl::LoadMode::Memory);
auto music = player.load("music.mp3", apl::LoadMode::Stream);

if (!music.isValid()) {
    std::cerr << player.lastError() << '\n';
}
```

| Mode | Behavior | Typical use |
| --- | --- | --- |
| `apl::LoadMode::Memory` | Decodes the whole file during `load()` and keeps stereo float samples in RAM | Short sounds or files you want fully decoded before playback |
| `apl::LoadMode::Stream` | Uses a background worker and a ring buffer; the audio thread never reads the file | Music and longer recordings |

Both modes support WAV, MP3, seeking, and looping. Detection uses file contents,
so the extension does not select the decoder. `load()` accepts a
`std::filesystem::path`, including wide paths on Windows:

```cpp
auto sound = player.load(std::filesystem::path(L"C:\\Audio\\music.mp3"));
```

MP3 loading scans the compressed frames once to validate the structure, calculate
duration, and build a seek index. In streaming mode this index grows with the
number of frames, but the entire decoded track is not retained in RAM. Initial
buffering may take a few milliseconds. Errors found later during streaming are
available through `Player::lastError()`.

## Playback controls

```cpp
sound.play();       // Start, or resume from the paused position.
sound.pause();      // Pause at the current position.
sound.play();       // Resume.
sound.seek(12.5);   // Seek to 12.5 seconds in the original file.
sound.stop();       // Stop and return to the beginning.
sound.play();       // Start from the beginning.

bool playing = sound.isPlaying();
```

Calling `play()` while already playing does not restart the sound. After natural
completion, `play()` starts it again from the beginning. Seeking preserves the
requested playing or paused state and resets the repetition budget. Positions
are clamped to the file's duration; non-finite values are ignored.

Commands are asynchronous and may be combined before the next audio block.
`isPlaying()` reflects the requested playing state, including initial streaming
buffering, and becomes false after natural completion or a pause/stop request.
It is a status query, not a sample-accurate playback clock. Version 1.0.1 does not
expose a duration or current-position getter.

## Volume

```cpp
sound.setVolume(0.5f);        // Half the original amplitude.
player.setMasterVolume(0.8f);

sound.setVolume(0.0f);        // Mute this sound; its position keeps advancing.
sound.setVolume(1.0f);        // Original amplitude.
sound.setVolume(1.5f);        // Amplify this sound.
```

Both volumes start at `1.0`. Gains must be finite and nonnegative; invalid values
are ignored. Set the master volume after initializing the player. Its value
applies to the combined mix. High gains or many simultaneous sounds can reach
the final output clamp and cause saturation distortion.

Play, pause, stop, seek, volume changes, and sound retirement use short internal
ramps of about 5 ms to reduce clicks. A pause or stop can therefore have a short
audible tail. These ramps are automatic; there is no public fade-effect API.

## Looping

```cpp
music.setLoop(true);       // Repeat indefinitely.
music.play();

effect.setLoopCount(3);    // Play once, then repeat three times: four plays total.
effect.play();

music.setLoop(false);      // Disable additional repetitions.
effect.setLoopCount(0);    // Play once without repetition.
```

Configure the loop before calling `play()` when possible. `setLoop()` and
`setLoopCount()` replace the previous loop policy. Changing it during playback
uses a brief ramp and applies the new repetition budget after the current
position. Changing it after natural completion does not restart playback.

To smooth the boundary, APL overlaps the end with the beginning by up to **3 ms**,
limited to one quarter of the file's length. The beginning samples already used
in that overlap are not played twice. Each additional repetition therefore adds
`file duration - overlap duration` to the timeline. For example, a 1-second file
with a 3 ms overlap adds about 0.997 seconds per repetition.

This reduces discontinuities between different endpoints. It changes exact loop
timing and does not guarantee a musically seamless result for every recording.

## Playing several sounds

```cpp
auto music = player.load("music.mp3", apl::LoadMode::Stream);
auto click = player.load("click.wav");
if (!music.isValid() || !click.isValid()) {
    std::cerr << player.lastError() << '\n';
    return 1;
}

music.setLoop(true);
music.setVolume(0.4f);
music.play();

click.play(); // Plays over the music; APL mixes both sounds automatically.
```

Keep both handles and the player alive while they are needed. A handle represents
one playback voice: calling `play()` repeatedly on the same handle does not
create overlapping copies. Load separate handles to play overlapping instances.

Each player has 128 slots for loaded sounds, including stopped and paused
sounds. When all slots are occupied, `load()` returns an invalid handle and sets
an error. Releasing a playing sound includes its stop ramp and asynchronous
cleanup, so its slot may not become available immediately.

## Ownership and cleanup

```cpp
apl::Sound sound = player.load("click.wav");
apl::Sound moved = std::move(sound); // The original handle becomes invalid.

moved.reset();     // Release this sound; it becomes invalid immediately.
player.shutdown(); // Close the device and invalidate all remaining sounds.
```

`Sound` is movable and non-copyable. Destruction and `reset()` stop the sound
through the internal ramp. The player retains the audio data until the render
thread acknowledges retirement; memory and files are then released by a service
thread. No periodic `update()` call is needed.

`Player` is neither copyable nor movable. Its destructor calls `shutdown()`.
Initialization and shutdown are idempotent. A sound can outlive its player, but
it becomes invalid and does not keep the output device or workers alive.
Operations on an invalid sound are harmless.

To restart a player after shutdown, call `init()` and load new sounds. Existing
handles remain invalid; the master volume starts again at `1.0`.

## Errors

```cpp
if (!player.init()) {
    std::cerr << player.lastError() << '\n';
    return 1;
}

auto sound = player.load("missing.mp3");
if (!sound.isValid()) {
    std::cerr << player.lastError() << '\n';
    return 1;
}
```

`init()` returns `false` on failure. `load()` returns an invalid `Sound` on
failure. `lastError()` returns a copy of the latest error description, including
file validation, streaming, and backend errors. A successful initialization or
load clears the previous stored error before playback starts. Errors may be
reported asynchronously, so check `lastError()` if playback stops unexpectedly.

The output device is selected during initialization. A disconnected device or an
unrecoverable backend failure stops rendering; automatic reconnection is not
implemented. Read the error, call `shutdown()`, call `init()`, and reload sounds.
No exception escapes the audio render callback.

## Public API reference

### `apl::Player`

| Method | Description |
| --- | --- |
| `bool init()` | Initialize the default output device; safe to call again while initialized |
| `void shutdown() noexcept` | Stop all playback, join workers, close the device, and invalidate handles |
| `Sound load(const std::filesystem::path& path, LoadMode mode = LoadMode::Memory)` | Load a file; return an invalid sound on failure; requires initialization |
| `void setMasterVolume(float volume)` | Set the nonnegative master gain after initialization |
| `std::string lastError() const` | Get the latest file, streaming, or device error |

### `apl::Sound`

| Method | Description |
| --- | --- |
| `Sound() noexcept` | Construct an empty, invalid handle |
| `Sound(Sound&&) noexcept` | Move ownership from another handle |
| `Sound& operator=(Sound&&) noexcept` | Release the previous sound and move another handle into this one |
| `bool isValid() const noexcept` | Check whether the handle still belongs to an active player |
| `void play()` | Start or resume; restart after natural completion |
| `void pause()` | Ramp down and preserve the position |
| `void stop()` | Ramp down and reset the position to zero |
| `void seek(double seconds)` | Seek within the original file and renew the repetition budget |
| `bool isPlaying() const noexcept` | Query requested playback and natural completion |
| `void setVolume(float volume)` | Set the nonnegative gain for this sound |
| `void setLoop(bool enabled)` | Select infinite repetition or no additional repetitions |
| `void setLoopCount(std::uint32_t repetitions)` | Select the number of additional repetitions |
| `void reset() noexcept` | Release the sound and invalidate this handle |

## Supported audio formats

### WAV

- Little-endian RIFF/WAVE, mono or stereo.
- Unsigned PCM 8-bit and signed PCM 16-, 24-, or 32-bit.
- IEEE float32.
- `WAVE_FORMAT_EXTENSIBLE` with PCM or float subtypes and valid PCM bits aligned
  to the most significant bits of the container.
- Header, chunk, size, alignment, byte-rate, and channel-mask validation.
  Unknown chunks are skipped; `data` may precede `fmt`. Non-finite float samples
  are replaced with zero.

RF64, big-endian RIFX, compressed WAV, multiple `data` chunks, and source files
with more than two channels are not supported.

### MP3

- Raw MPEG Audio Layer III, CBR or VBR.
- MPEG-1: 32, 44.1, and 48 kHz.
- MPEG-2 LSF: 16, 22.05, and 24 kHz.
- MPEG-2.5: 8, 11.025, and 12 kHz.
- Mono, stereo, dual channel, and joint stereo with MS/intensity processing.
- Long, short, and mixed blocks, bit reservoirs, and CRC validation when present.
- Skipping of initial ID3v2.2/2.3/2.4 tags, trailing ID3v2.4 tags with footers,
  ID3v1 tags, and APE tags.
- Xing/Info metadata and LAME/Lavc/Lavf delay/padding information for gapless
  trimming when valid metadata is available. Without valid trimming metadata,
  complete decoded audio frames are played.

Free-format MP3, MPEG Layers I/II, emphasis/de-emphasis, changes in sample rate or
channel count within one stream, and MP3 stored inside a WAV container are not
supported. The MP3 decoder does not call an operating-system codec or an external
program.

## Audio processing and threading

Audio is mixed as stereo float32 at the output device's selected rate. Mono
sources are duplicated to both channels. Mono output averages left and right;
on multichannel Windows devices, only front-left and front-right carry audio.

Resampling uses four-point cubic Hermite interpolation. It is not a band-limited
sinc resampler: large downsampling ratios can alias high-frequency content.
The final mix is clamped to `[-1, 1]`. Conversion to output with 16 valid bits
uses TPDF dither while preserving exact digital silence.

Windows uses WASAPI's shared, event-driven mode and the format returned by
`GetMixFormat()`. Linux negotiates an ALSA rate near 48 kHz and a channel count
near two, preferring native-endian float32, then PCM32, then PCM16. APL resamples
sources to the negotiated rate. System audio servers or plugins may perform
additional conversion. Linux requests periods around 10 ms and a buffer of
four periods; these are not end-to-end latency guarantees.

Each streamed sound has its own worker, four source-cache pages, and an
8192-frame stereo ring buffer at the device rate. At 48 kHz, the ring holds about
171 ms of audio. If the worker cannot keep up, rendering briefly ramps toward
silence and retries without waiting for disk access. MP3 seeks reconstruct the
bit reservoir and decode two preceding frames to restore synthesis state.

Audio rendering does not allocate, free objects, acquire mutexes, read files,
or block. Device waits and OS buffer transfers happen outside the render
function. Application-side controls and loading may use locks or allocate.

Player methods are serialized internally. Controls on the same valid sound can
be called from multiple application threads. Moving, resetting, or destroying
the same C++ handle requires external synchronization against other calls on
that handle. Retiring a sound while the mixer uses it, or shutting down a player
with active sounds, is supported.

## Version 1.0.1 scope

The public API does not yet include pan, separate channel-volume controls,
effects, speed/pitch controls, OGG/FLAC decoding, macOS playback, 3D audio,
recording, or MIDI. The internal backend and decoder interfaces separate those
components from the mixer. Independent channel gains and an effect insertion
point are reserved internally for future work.

Version 1.0.1 passed nine offline test groups on Windows with GCC/MinGW, including
regressions for seek/play ordering and loop counts at natural completion. Rendering
allocated no memory during those checks. Additional public-control checks passed
200 repeated cases and 72 edge cases with different sample rates and block sizes.
The MP3 suite passed 17 reference files, streaming/seek checks, and 400 mutated files.

Earlier version 1.0 validation also covered MSVC on Windows and GCC on Linux,
including virtual ALSA outputs and address/undefined-behavior sanitizers. Those
platform checks were not repeated for this patch. A full Clang build and physical
Linux audio-device latency remain unvalidated. Test tools and generated binaries
are not part of this release folder.

## Format references

The MP3 decoder was independently implemented using ISO/IEC 11172-3,
ISO/IEC 13818-3, and Martin Sieler and Ralph Sperschneider's
[MPEG-Layer3 Bitstream Syntax and Decoding, issue 1.2 (1997)](https://mp3guessenc.sourceforge.io/MPEG%20Layer3%20Bitstream%20Syntax%20and%20Decoding.pdf).
Numeric Huffman, synthesis-window, and scalefactor-band tables are transcribed
from that format reference. Decoder algorithms and processing code are
implemented in APL.

## License

APL is released under the [MIT License](LICENSE).

Artificial intelligence was used to help create this project, including its code
and documentation.

Copyright © 2026 allekks_b.
