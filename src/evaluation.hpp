#ifndef CHESS_EVALUATION_H
#define CHESS_EVALUATION_H

#include "./Board.hpp"

constexpr inline int32_t piece_value(Piece p)
{
    switch (p) {
    case P_PAWN: return 100;
    case P_ROOK: return 550;
    case P_BISHOP: return 300;
    case P_KNIGHT: return 300;
    case P_QUEEN: return 900;
    case P_KING: return 15000;
    default: return 0;
    }
}

int32_t evaluate_board(const Board &b);

#endif // CHESS_EVALUATION_H
