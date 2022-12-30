#ifndef CHESS_UCI_H
#define CHESS_UCI_H


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

void uci_main_loop();
struct Move;
void uci_send_bestmove(const Move&);

#endif // CHESS_UCI_H
