#include <iostream>
#include <algorithm>

#include "./board.hpp"
#include "./board_renderer.hpp"
#include "./uci.hpp"

using namespace std;

#undef main // we use console app

void UI_mode()
{
    Board board;
    BoardRenderer renderer;

    renderer.init();
    board.load_initial_position();

    renderer.main_loop(board);
}

int main(int argc, char *argv[])
{
    try {
        uci_main_loop();
    }
    catch (std::exception& e) {
        std::cerr << "Exception=" << e.what() << "\n";
        return 1;
    }
    return 0;
}

// int WinMain(void) {
//     return main(0, nullptr);
// }
