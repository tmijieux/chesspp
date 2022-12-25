#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

#include <vector>


#include "./types.hpp"
class Board;

class Move
{
public:
    int32_t evaluation;

    Pos src;
    Pos dst;
    bool en_passant;
    Piece piece;
    Color color;
    Piece taken_piece;
    bool takes;
    bool castling;

    bool promote;
    Piece promote_piece;

    // remember some of state of board before move
    Pos en_passant_pos_before;
    uint8_t half_move_before;
    bool checks_before[2];
    bool castles_rights_before[4];

    bool legal;
    bool legal_checked;

    Move():
        evaluation(-999999),
        en_passant{false},
        piece{P_INVALID_PIECE},
        color{C_INVALID_COLOR},
        taken_piece{P_EMPTY},
        takes{false},
        castling{false},
        half_move_before(0),
        promote(false),
        promote_piece(P_INVALID_PIECE),
        checks_before{ false,false },
        castles_rights_before{false,false,false,false},
        legal{false},
        legal_checked{false}
    {
    }

    Move(const Board& b);

    bool operator==(const Move& o) {
        return dst.row == o.dst.row && dst.column == o.dst.column && piece == o.piece;
    }

private:

}; // class Move

using MoveList = std::vector<Move>;



#endif // CHESS_MOVE_H
