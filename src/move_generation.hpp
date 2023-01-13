#ifndef CHESS_MOVE_GENERATION_H
#define CHESS_MOVE_GENERATION_H

struct Pos;
class Board;

#include "./board.hpp"
#include "./move.hpp"

MoveList generate_pseudo_moves(Board& b, bool only_takes=false);
MoveList enumerate_attacks(Board& b, Color to_move);
MoveList generate_check_evading_moves(Board& b);

/**
 * pawn_attacks: also return pseudo-capture of pawn on empty square
 */
void add_move_from_position(
    Board& b, const Pos& pos,
    MoveList& moveList, bool only_takes);

/**
 * pawn_attacks: also return pseudo-capture of pawn on empty square
 */
void find_move_to_position(
    Board& b, const Pos& pos, MoveList& moveList,
    Color clr, int16_t max_move, bool only_takes);

std::string move_to_string(const Move& m);
std::string move_to_uci_string(const Move& m);
std::string move_to_string_disambiguate(Board& b, const Move& m);


void generate_queen_move(const Board& b, const Pos& pos, Color clr,
                         MoveList& moveList, int max_dist, bool only_takes);
void generate_rook_move(const Board& b, const Pos& pos, Color clr,
                        MoveList& moveList, bool only_takes);
void generate_knight_move(const Board& b, const Pos& pos, Color clr,
                          MoveList& moveList, bool only_takes);
void generate_bishop_move(const Board& b, const Pos& pos, Color clr,
                          MoveList& moveList, bool only_takes);


/**
 * pawn_attacks: also return pseudo-capture of pawn on empty square
 */
void generate_pawn_move(
    Board& b, const Pos& pos, Color clr,
    MoveList& moveList, bool only_takes);

void generate_castle_move(const Board& b, const Pos& pos, Color clr, MoveList& moveList);

std::string pos_to_square_name(const Pos& p);
Pos square_name_to_pos(const std::string& squarename);

bool generate_move_for_squares(
    Board &b,  const Pos &src, const Pos &dst, Piece promote_piece, Move &out);


#endif // CHESS_MOVE_GENERATION_H
