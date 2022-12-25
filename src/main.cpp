#include <iostream>
#include <algorithm>

#include "./board.hpp"
#include "./board_renderer.hpp"

using namespace std;

#undef main // we use console app
int main(int argc, char *argv[])
{
    try {
        Board board;
        BoardRenderer renderer;

        renderer.init();
        board.load_initial_position();
        // Move bestMove = find_best_move(board);
        //std::cout << "Final best move = " << move_to_string(bestMove) << "\n\n___\n\n\n\n";

        renderer.draw(board);
        renderer.main_loop(board);
        std::cout << "quit!!" << "\n";
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
