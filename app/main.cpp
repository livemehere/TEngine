#include <iostream>
#include <core/Application.h>
#include <Game.h>

int main() {

    try {
        Game game;
        Application app(game);
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << "\n";
        return -1;
    }

    return 0;
}
