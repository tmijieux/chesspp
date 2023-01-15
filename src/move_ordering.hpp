#ifndef CHESS_MOVE_ORDERING_H
#define CHESS_MOVE_ORDERING_H

#include <cstdint>

#include "./board.hpp"
#include "./move.hpp"

struct NegamaxEngine;

void reorder_mvv_lva(MoveList& moveList, size_t begin, size_t end);
void reorder_moves(
    NegamaxEngine &engine,
    Board &b, MoveList &moveList , int current_depth, int remaining_depth,
    KillerMoves &killers, bool has_hash_move, const Move &hash_move, const HistoryMoves &history);
void reorder_see(Board& b, MoveList& moveList, size_t begin, size_t end);
int32_t see_capture(Board &b, const Move &m);

#endif // CHESS_MOVE_ORDERING_H
