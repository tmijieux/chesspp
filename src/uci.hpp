#ifndef CHESS_UCI_H
#define CHESS_UCI_H

#include <fmt/format.h>
#include "./move.hpp"

struct GoParams {
    bool infinite;
    bool ponder;
    MoveList searchmoves;
    uint64_t movetime;
    uint64_t wtime;
    uint64_t btime;
    uint64_t wincrement;
    uint64_t bincrement;
    uint32_t movestogo;
    uint32_t mate;
    uint32_t depth;

    GoParams() :
        infinite{ false },
        ponder{ false },
        movetime{ 0 },
        wtime{ 0 },
        btime{ 0 },
        wincrement{ 0 },
        bincrement{ 0 },
        movestogo{ 0 },
        mate{ 0 },
        depth{ 0 }
    {
    }
};

template<typename T, typename ...K>
inline void uci_send(T&& t, K&&... k)
{
    std::cout << fmt::format(std::forward<T>(t), std::forward<K>(k)...) << std::flush;
}

void uci_main_loop();
struct Move;
void uci_send_bestmove(const Move&);

template<typename T, typename ...K>
inline void uci_send_info_string(T&& info, K&&... k)
{
    uci_send("info string {}\n", fmt::format(std::forward<T>(info), std::forward<K>(k)...));
}

class Board;
void console_draw(const Board& board);


#endif // CHESS_UCI_H
