#ifndef CHESS_MOVE_ORDERING_H
#define CHESS_MOVE_ORDERING_H

#include <cstdint>

#include "./board.hpp"
#include "./move.hpp"

void reorder_mvv_lva(const Board& b, MoveList& moveList, size_t begin, size_t end);
void reorder_moves(
    Board &b, MoveList &moveList , int current_depth, int remaining_depth,
    KillerMoves &killers, bool has_hash_move, const Move &hash_move);
void reorder_see(Board& b, MoveList& moveList, size_t begin, size_t end);
int32_t compute_see(Board &b, const Move &m);

#endif // CHESS_MOVE_ORDERING_H
