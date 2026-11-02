#include "apl.hpp"

#include <chrono>
#include <iostream>
#include <thread>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <audio.wav|audio.mp3>\n";
        return 0;
    }

    // Open the default audio output device.
    apl::Player player;
    if (!player.init()) {
        std::cerr << player.lastError() << '\n';
        return 1;
    }

    // Load a WAV or MP3 file into memory.
    auto sound = player.load(argv[1]);
    if (!sound.isValid()) {
        std::cerr << player.lastError() << '\n';
        return 1;
    }

    sound.setVolume(0.5f);
    sound.play();
    std::cout << "Playing " << argv[1] << "...\n";

    // Keep both objects alive until playback ends or an audio error occurs.
    while (sound.isPlaying() && player.lastError().empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    if (const auto error = player.lastError(); !error.empty()) {
        std::cerr << error << '\n';
        return 1;
    }

    // The sound and player release their resources automatically.
    return 0;
}
