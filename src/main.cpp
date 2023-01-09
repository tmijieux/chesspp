#include <iostream>
#include <algorithm>

#include "./board.hpp"
#include "./board_renderer.hpp"
#include "./uci.hpp"
#include "./transposition_table.hpp"

using namespace std;

#undef main // we use console app

int main(int argc, char *argv[])
{
    std::cout << "Tistou Chess by Thomas Mijieux\n"<<std::flush;
    HashParams::init_params();
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
