#ifndef CHESS_MOVE_GENERATION_H
#define CHESS_MOVE_GENERATION_H

#include "./Board.hpp"
#include "./Move.hpp"

MoveList enumerate_moves(const Board& b, bool only_takes=false);
MoveList enumerate_attacks(const Board& b, Color to_move);
void add_move_for_position(
    const Board& b, const Pos& pos,
    MoveList& moveList, bool only_takes, bool attacks = false);

std::string move_to_string(const Move& m);

void reorder_moves(MoveList &moveList , int currentDepth, const MoveList &previousPv);



void reorder_mvv_lva(const Board &b, MoveList& moveList);


void generate_queen_move(const Board& b, const Pos& pos, Color clr,
                         MoveList& moveList, int max_dist, bool only_takes);
void generate_rook_move(const Board& b, const Pos& pos, Color clr,
                        MoveList& moveList, bool only_takes);
void generate_knight_move(const Board& b, const Pos& pos, Color clr,
                          MoveList& moveList, bool only_takes);
void generate_bishop_move(const Board& b, const Pos& pos, Color clr,
                          MoveList& moveList, bool only_takes);
void generate_pawn_move(const Board& b, const Pos& pos, Color clr,
                        MoveList& moveList, bool only_takes, bool attacks);
void generate_castle_move(const Board& b, const Pos& pos, Color clr, MoveList& moveList);


#endif // CHESS_MOVE_GENERATION_H
