#ifndef CHESS_EVALUATION_H
#define CHESS_EVALUATION_H

#include "./Board.hpp"

int32_t piece_value(Piece p);

int32_t evaluate_board(const Board &b);
int32_t quiescence(Board& b, int color, int32_t alpha, int32_t beta);

#endif // CHESS_EVALUATION_H
