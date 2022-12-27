#ifndef CHESS_MOVE_ORDERING_H
#define CHESS_MOVE_ORDERING_H

#include <cstdint>
#include "./Board.hpp"
#include "./move.hpp"

void reorder_mvv_lva(const Board& b, MoveList& moveList);

void reorder_moves(
    Board &b, MoveList &moveList ,
    int current_depth, int remaining_depth,
    const MoveList &previousPv, KillerMoves &killers);


int32_t compute_see(Board &b, const Move &m);

#endif // CHESS_MOVE_ORDERING_H
