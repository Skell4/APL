# APL - Audio Playback Library

![APL - Audio Playback Library](assets/apl-banner.png)

[![C++: 17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)](#1-build-and-run)
[![Header-only](https://img.shields.io/badge/header--only-7B68EE)](apl.hpp)
[![Version: 1.1](https://img.shields.io/badge/version-1.1-7B68EE)](apl.hpp)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platforms: Windows | Linux](https://img.shields.io/badge/platforms-Windows%20%7C%20Linux-0078D4)](#9-compatibility-and-implementation)

APL is a C++ library for playing WAV, MP3, and FLAC audio files on Windows and
Linux. The whole library is in [apl.hpp](apl.hpp). It needs C++17 or later and
implements its own decoders, mixer, and resampler.

APL provides:

- WAV, MP3, and native FLAC playback with automatic format detection.
- Full decoding into memory or background streaming.
- Play, pause, stop, seek, and playback status.
- Per-sound volume and master volume.
- Infinite loops or a specified number of additional repetitions.
- Up to 128 loaded sounds per player, mixed automatically.
- Movable sound handles and automatic resource cleanup.

## Files

| File | Purpose |
| --- | --- |
| [apl.hpp](apl.hpp) | The complete library. Copy this file into your project. |
| [example.cpp](example.cpp) | A small example that loads a file and plays it. |
| [README.md](README.md) | This guide and the public API reference. |
| [assets/](assets/) | The project banner in PNG and SVG formats. |
| [LICENSE](LICENSE) | The MIT license. |

All public types are in the `apl` namespace. Use names such as `apl::Player`,
`apl::Sound`, and `apl::LoadMode` directly. The library selects its Windows or
Linux audio backend internally, so application code uses the same API on both
systems.

## Contents

- [1. Build and run](#1-build-and-run)
- [2. First example](#2-first-example)
- [3. Player](#3-player)
- [4. Sounds and playback](#4-sounds-and-playback)
- [5. Loading modes](#5-loading-modes)
- [6. Volume and loops](#6-volume-and-loops)
- [7. Supported formats](#7-supported-formats)
- [8. Errors and object lifetime](#8-errors-and-object-lifetime)
- [9. Compatibility and implementation](#9-compatibility-and-implementation)
- [10. License](#10-license)

## 1. Build and run

Copy `apl.hpp` into your project and include it:

```cpp
#include "apl.hpp"
```

Use a compiler with **C++17 or later**. The header can be included in multiple
source files. Compile your application with the platform flags below.

Run these commands from the repository directory. The example loads `audio.mp3`
from the working directory; see [First example](#2-first-example).

### Windows with GCC or MinGW

Use a MinGW-w64 distribution with `std::thread` support. In PowerShell:

```powershell
g++ -std=c++17 -O2 -Wall -Wextra -pthread example.cpp -o example.exe -lole32
.\example.exe
```

Link `ole32` for the Windows audio backend. A Clang installation targeting
MinGW can use `clang++` with the same flags.

### Windows with MSVC

In a Visual Studio Developer Command Prompt:

```bat
cl /nologo /std:c++17 /EHsc /W4 /permissive- /O2 example.cpp /Fe:example.exe
example.exe
```

The header automatically links `ole32.lib` with MSVC and clang-cl.

### Linux

```sh
g++ -std=c++17 -O2 -Wall -Wextra -pthread example.cpp -o example -ldl
./example
```

Linux uses the system ALSA runtime, `libasound.so.2`, loaded during initialization.
ALSA development headers and `-lasound` are not required. `-ldl` links the system
dynamic loader and supports glibc versions older than 2.34.

For your own application, replace `example.cpp` with your source files and keep
the platform flags. APL's decoders are included in the header.

## 2. First example

The included [example.cpp](example.cpp) loads `audio.mp3`, starts playback, and
waits for Enter before closing. Put your audio file in the directory you run
from, or change `"audio.mp3"` in the code to your WAV, MP3, or FLAC filename.

This is the full example:

```cpp
#include "apl.hpp"
#include <iostream>

int main() {
    apl::Player player;
    if (!player.init()) return 1;

    auto sound = player.load("audio.mp3");
    if (!sound.isValid()) return 1;

    sound.play();
    std::cout<<"Press Enter to close.\n";
    std::cin.get();
}
```

`init()` opens the output device, `load()` creates a sound, and `play()` starts
it. `std::cin.get()` keeps the program open while the sound plays. Objects
release their resources when they leave scope.

The example returns `1` if initialization or loading fails.
`player.lastError()` provides the error description.

The short fragments below assume an initialized `apl::Player player` and a
valid `apl::Sound sound` unless stated otherwise.

## 3. Player

A `Player` owns an audio device, the mixer, and its loaded sounds. Create it,
call `init()`, then load the files you need.

| Method | Result and use |
| --- | --- |
| `bool init()` | Opens the default output device. Returns true if already initialized. |
| `apl::Sound load(const std::filesystem::path& path, apl::LoadMode mode = apl::LoadMode::Memory)` | Loads a file and returns its sound handle. Requires initialization. |
| `void shutdown() noexcept` | Stops playback, joins workers, closes the device and files, and invalidates existing sounds. Safe to call again. |
| `void setMasterVolume(float volume)` | Sets the gain applied to the combined mix. Call after initialization. |
| `std::string lastError() const` | Returns the latest loading, streaming, or device error. |

A player can hold **128 loaded sounds** at once, including paused and stopped
sounds. Loading another file when all slots are occupied returns an invalid
sound with an error. Release a sound with `reset()` or let it leave scope.

The `Player` destructor calls `shutdown()`. A player cannot be copied or moved;
pass it by reference when several functions need it.

## 4. Sounds and playback

`load()` returns an `apl::Sound`. Check `isValid()` before using a loaded sound.

| Method | Result and use |
| --- | --- |
| `bool isValid() const noexcept` | Checks whether the sound still belongs to an active player. |
| `void play()` | Starts or resumes playback. Restarts after natural completion. |
| `void pause()` | Pauses and preserves the position. |
| `void stop()` | Stops and resets the position to the beginning. |
| `void seek(double seconds)` | Seeks within the original file and renews the repetition budget. |
| `bool isPlaying() const noexcept` | Reports requested playback and natural completion. |
| `void setVolume(float volume)` | Sets this sound's gain. |
| `void setLoop(bool enabled)` | Enables infinite repetition or disables additional repetitions. |
| `void setLoopCount(std::uint32_t repetitions)` | Sets the number of additional repetitions. |
| `void reset() noexcept` | Releases the sound and invalidates this handle. |

Calling `play()` while a sound is already playing keeps its current position.
After natural completion, `play()` starts from the beginning. A seek requested
with play is respected regardless of command order.

Seek positions are measured in seconds and clamped to the file's duration.
Seeking preserves the playing or paused state. Non-finite positions are ignored.

Commands are asynchronous and may be combined before the next audio block.
`isPlaying()` reflects the requested playing state, including initial streaming
buffering, and becomes false after natural completion or a pause/stop request.
It is a status query rather than a playback clock. Duration and current-position
getters are not exposed.

A default-constructed `Sound` is empty and invalid. Sounds can be moved using
`Sound(Sound&&)` and move assignment; they cannot be copied. The destructor
releases the sound. Calls on an invalid sound are harmless.

## 5. Loading modes

The same loading modes support WAV, MP3, and native FLAC. Format detection uses
file contents rather than the filename extension.

| Mode | Behavior | Typical use |
| --- | --- | --- |
| `apl::LoadMode::Memory` | Decodes the whole file during loading and stores stereo float samples in RAM. This is the default. | Short sounds or files you want fully decoded before playback. |
| `apl::LoadMode::Stream` | Keeps the file open and decodes on a background worker with limited audio buffers. | Music and longer files. |

```cpp
auto effect = player.load("effect.wav");
auto music = player.load("music.flac", apl::LoadMode::Stream);
```

Paths use `std::filesystem::path`, including wide paths on Windows.

MP3 and FLAC loading scan the compressed frames to validate the structure and
build a seek index. Startup cost depends on file size even in streaming mode,
and index memory grows with the number of frames. Streaming keeps the index and
work buffers without retaining the entire decoded track.

MP3 seeking reconstructs the bit reservoir and decodes preceding frames to
restore synthesis state. FLAC seeking uses independently decodable frames.
File access, decoding, resampling, and loop prefetching run on loading or
streaming threads.

## 6. Volume and loops

Both the master gain and each sound's gain start at `1.0f`.

| Value | Meaning |
| --- | --- |
| `0.0f` | Silence; playback continues to advance. |
| `0.5f` | Half the original amplitude. |
| `1.0f` | Original gain. |
| Greater than `1.0f` | Amplification. |

Gains must be finite and nonnegative. Invalid values are ignored. High gains or
many simultaneous sounds can reach the final output clamp and cause distortion.

Configure repetition before starting playback when possible:

```cpp
sound.setLoop(true);       // Repeat indefinitely.
sound.setLoop(false);      // Disable additional repetitions.
sound.setLoopCount(3);     // Play once, then repeat three times.
```

Each call replaces the previous loop policy. `setLoopCount(0)` plays once.
Changing the policy during playback applies the new repetition budget after
the current position. Changing it after natural completion does not restart
playback. Seeking renews the repetition budget.

Play, pause, stop, seek, volume changes, and sound release use short internal
ramps of about **5 ms** to reduce clicks. A pause or stop may have a short audible
tail. These ramps are automatic.

At a loop boundary, APL overlaps the end with the beginning by up to **3 ms**,
limited to one quarter of the file length. Samples used in that overlap are not
played twice. Each additional repetition therefore adds `duration - overlap`
to the timeline. This reduces endpoint discontinuities but does not guarantee
an imperceptible musical loop for every recording.

## 7. Supported formats

Source files can be mono or stereo.

### WAV

- Little-endian RIFF/WAVE.
- Unsigned PCM 8-bit and signed PCM 16-, 24-, or 32-bit.
- IEEE float32.
- `WAVE_FORMAT_EXTENSIBLE` with PCM/float subtypes and valid PCM bits aligned
  to the most significant bits of the container.
- Unknown chunks are skipped, and `data` may precede `fmt`. Non-finite float
  samples are replaced with zero.

RF64, big-endian RIFX, compressed WAV, multiple `data` chunks, and source files
with more than two channels are not supported. Headers, chunk bounds,
alignment, byte rate, and channel masks are validated.

### MP3

- Raw MPEG Audio Layer III, with constant or variable bitrate.
- MPEG-1: 32, 44.1, and 48 kHz.
- MPEG-2 LSF: 16, 22.05, and 24 kHz.
- MPEG-2.5: 8, 11.025, and 12 kHz.
- Mono, stereo, dual channel, and MS/intensity joint stereo.
- Long, short, and mixed blocks, bit reservoirs, and CRC validation.
- Initial ID3v2.2/2.3/2.4 tags, trailing ID3v2.4 tags with footers, ID3v1, and
  APE tags are skipped.
- Xing/Info metadata and valid LAME/Lavc/Lavf delay/padding fields enable gapless
  trimming. Without valid metadata, complete decoded frames are played.

Free-format, Layers I/II, emphasis/de-emphasis, changes in sample rate or channel
count within one file, and MP3 inside a WAV container are not supported.

### FLAC

- Native FLAC with the `fLaC` signature.
- Signed integer audio from 4 to 32 bits, mono or stereo.
- Sample rates from 1 to 1048575 Hz.
- Constant/verbatim subframes, fixed predictors of order 0-4, and LPC predictors
  of order 1-32.
- Wasted bits, Rice coding with 4/5-bit parameters, and escaped partitions.
- Independent, left-side, side-right, and mid-side stereo, including 33-bit
  side channels.
- Fixed or variable blocks up to 65535 samples.
- Unknown STREAMINFO duration, determined by scanning the frames.
- STREAMINFO, frame numbering, bounds, header CRC-8, and frame CRC-16 validation.
- Metadata and pictures are skipped; an internal index supports seeking even
  without a seek table.

The global MD5 checksum is not verified. FLAC inside Ogg/MP4 containers, ID3 tags
before the signature, and source files with more than two channels are not
supported. FLAC reconstruction uses integer arithmetic; conversion to APL's
float32 pipeline may round samples with more than 24 significant bits.

## 8. Errors and object lifetime

`Player::init()` returns false on failure. `load()` returns an invalid sound if
the file cannot be opened, is unsupported, or contains invalid data. Read
`Player::lastError()` for the description.

A successful initialization or load clears the previous stored error before
playback starts. Streaming and device failures may be reported asynchronously,
so check `lastError()` if playback stops unexpectedly.

`reset()` and sound destruction release the sound. A service thread reclaims
retired voices after the audio thread has acknowledged them, typically on a
5 ms check or during the next load/shutdown. Resource cleanup stays outside the
render function.

Sounds may outlive their player, but become invalid after player shutdown.
They do not keep the output device or streaming workers alive. Destroying a
player with active sounds stops them and releases its resources.

The device is chosen during initialization. A disconnection or unrecoverable
device error stops rendering. To reopen it, call `shutdown()`, call `init()`,
and reload the sounds. Automatic reconnection is not implemented.

## 9. Compatibility and implementation

| Platform | Audio backend | Requirements |
| --- | --- | --- |
| Windows 10/11 | WASAPI, shared mode with event-driven rendering. | C++17 compiler, Windows SDK or MinGW-w64 headers, and `ole32`. |
| Linux | ALSA, nonblocking interleaved playback. | C++17 compiler, threads, dynamic loader, and the system `libasound.so.2` runtime. |

MSVC, GCC/MinGW-w64, and Clang are compiler targets. Native lock-free atomics and
IEEE-754 32-bit `float` are required and checked at compile time. Other platforms,
including macOS, have no playback backend and `init()` reports an error.

### Linux audio device

APL opens the ALSA PCM named `default`, respecting the system configuration.
This can include PipeWire or PulseAudio through configured ALSA plugins.

Select a different PCM for one process with `APL_ALSA_DEVICE`:

```sh
APL_ALSA_DEVICE=plughw:0,0 ./example
```

A hardware PCM may be occupied by another application. For shared output, use
the distribution's configured `default`. With `APL_ALSA_DEVICE=null`, audio is
processed at playback speed and discarded.

### Audio processing

APL mixes stereo float32 samples at the output device's selected rate and uses
four-point cubic Hermite resampling. Strong downsampling of high-frequency
content can alias because this resampler has no sinc anti-alias filter.

Mono sources are duplicated into left/right. Mono output uses their average.
On multichannel output devices, only front-left/front-right receive audio.
The final mix is clamped to `[-1, 1]`. Output with 16 valid bits uses TPDF dither;
digital silence remains zero.

WASAPI uses the format returned by `GetMixFormat`: float32/64 or supported PCM
8/16/24/32 with valid bits. ALSA negotiates a rate near 48 kHz and a channel count
near two, preferring native-endian float32, then PCM32, then PCM16. Resampling
stays inside APL; a system audio server/plugin may perform further conversion.

### Threads

The audio render function does not allocate, destroy objects, acquire mutexes,
read files, or wait. Its voice scan is bounded by the 128-sound limit.
Operating-system waits and buffer transfers happen outside that function.

Each streaming sound has a worker, an 8192-frame stereo ring at the device rate,
and four source-cache pages. If data arrives late, the mixer ramps briefly to
silence and retries without waiting for the worker.

Player methods are serialized internally. Controls on the same valid sound can
be called from multiple application threads. Moving, resetting, or destroying
the same C++ handle requires external synchronization against other calls on
that handle. Normal cleanup is automatic; no periodic update call is required.

The public API does not include pan, separate channel-volume controls, effects,
speed/pitch controls, 3D audio, recording, or MIDI.

### Format and backend references

- [MP3 bitstream syntax and decoding](https://mp3guessenc.sourceforge.io/MPEG%20Layer3%20Bitstream%20Syntax%20and%20Decoding.pdf), used alongside ISO/IEC 11172-3 and ISO/IEC 13818-3.
- [FLAC format, RFC 9639](https://www.rfc-editor.org/rfc/rfc9639.html), sections 8 and 9.
- [WASAPI initialization](https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nf-audioclient-iaudioclient-initialize) and [render-buffer management](https://learn.microsoft.com/en-us/windows/win32/coreaudio/rendering-a-stream).
- [ALSA PCM documentation](https://www.alsa-project.org/alsa-doc/alsa-lib/pcm.html).

## 10. License

APL is available under the [MIT license](LICENSE).

Copyright © 2026 allekks_b.
