#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include "./board.hpp"
#include "./move_generation.hpp"
#include "./evaluation.hpp"

int32_t negamax(
    Board& b,
    int max_depth, int remaining_depth, int current_depth,
    int color,
    int32_t alpha, int32_t beta,
    MoveList& parentPv,
    const MoveList& previousPv,
    MoveList* topLevelOrdering
);

void iterative_deepening(
    Board& b, int max_depth, Move *bestMove, bool *moveFound);

std::pair<bool,Move> find_best_move(Board& b);

#endif // CHESS_ENGINE_H
