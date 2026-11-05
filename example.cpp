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
