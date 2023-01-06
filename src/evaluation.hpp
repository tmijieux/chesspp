#ifndef CHESS_EVALUATION_H
#define CHESS_EVALUATION_H

#include "./board.hpp"
#include "./move_generation.hpp"


constexpr inline int32_t piece_value(Piece p)
{
    switch (p) {
    case P_PAWN: return 100;
    case P_ROOK: return 550;
    case P_BISHOP: return 300;
    case P_KNIGHT: return 300;
    case P_QUEEN: return 900;
    case P_KING: return 15000;
    default: return 0; // covers P_EMPTY
    }
}

int32_t evaluate_position(const Board &b);
int32_t evaluate_material_only(const Board& b);
int32_t evaluate_position_only(const Board& b, MoveList &out);

#endif // CHESS_EVALUATION_H
