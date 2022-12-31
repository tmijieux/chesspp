#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

struct Move;

#include <vector>


#include "./types.hpp"
#include "./Board.hpp"

struct Move
{
public:
    int32_t evaluation;
    int32_t see_value;

    Pos src;
    Pos dst;
    Piece piece;
    Color color;
    Piece taken_piece;

    //bool en_passant;

    unsigned en_passant : 1;
    // bool killer;
    // bool best_from_pv;
    unsigned killer:1;
    unsigned mate_killer:1;
    unsigned best_from_pv:1;

    //bool takes;
    unsigned takes:1;
    //bool castling;
    unsigned castling:1;

    //bool legal;
    //bool legal_checked;
    bool legal;
    unsigned legal_checked : 1;

    unsigned promote : 1;
    //bool promote;
    Piece promote_piece;

    // remember some of state of board before move
    Pos en_passant_pos_before;
    uint8_t half_move_before;

    bool checks_before[2];
    bool castles_rights_before[4];
    //unsigned checks_before : 2;
    //unsigned castle_rights_before : 4;

    std::string position_before;

    Move():
        evaluation{-999999},
        see_value{0},
        killer{false},
        best_from_pv{false},
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

    Move(const Board& b) : Move()
    {
        castles_rights_before[CR_KING_BLACK] = b.get_castle_rights(CR_KING_BLACK);
        castles_rights_before[CR_KING_WHITE] = b.get_castle_rights(CR_KING_WHITE);
        castles_rights_before[CR_QUEEN_BLACK] = b.get_castle_rights(CR_QUEEN_BLACK);
        castles_rights_before[CR_QUEEN_WHITE] = b.get_castle_rights(CR_QUEEN_WHITE);

        checks_before[0] = b.is_king_checked(C_BLACK);
        checks_before[1] = b.is_king_checked(C_WHITE);

        en_passant_pos_before = b.get_en_passant_pos();
        half_move_before = b.get_half_move();
        position_before = b.get_pos_string();
    }

    bool operator==(const Move& o)
    {
        return dst.row == o.dst.row && dst.column == o.dst.column && piece == o.piece;
    }

    Move reverse() const
    {
        if (!takes) {
            throw std::exception("cannot reverse if not capture");
        }
        Move m;
        m.dst = src;
        m.src = dst;
        m.takes = true;
        m.taken_piece = piece;
        m.piece = taken_piece;
        m.color = other_color(color);


        m.castling = castling;

        m.en_passant_pos_before = en_passant_pos_before;
        m.half_move_before = half_move_before;
        m.promote = promote;
        m.promote_piece = promote_piece;

        m.checks_before[0] = checks_before[0];
        m.checks_before[1] = checks_before[1];

        m.castles_rights_before[0] = castles_rights_before[0];
        m.castles_rights_before[1] = castles_rights_before[1];
        m.castles_rights_before[2] = castles_rights_before[2];
        m.castles_rights_before[3] = castles_rights_before[3];

        m.legal = legal;
        m.legal_checked = legal_checked;

        return m;
    }

private:

}; // class Move

using MoveList = std::vector<Move>;
using MovePtrList = std::vector<Move*>;
using KillerMoves = std::vector<MoveList>;



#endif // CHESS_MOVE_H
