#ifndef CHESS_MOVE_H
#define CHESS_MOVE_H

struct Move;

#include <vector>


#include "./types.hpp"
#include "./board.hpp"

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


    unsigned en_passant : 1;
    unsigned killer:1;
    unsigned mate_killer:1;
    unsigned hash_move:1;
    unsigned takes:1;
    unsigned castling:1;
    unsigned legal : 1;
    unsigned legal_checked : 1;
    unsigned promote : 1;

    //bool promote;
    Piece promote_piece;

    // remember some of state of board before move
    uint32_t m_board_state_before;
    uint64_t m_board_key_before;

    uint8_t half_move_before;

    Move():
        evaluation{-999999},
        see_value{0},
        killer{false},
        hash_move{false},
        en_passant{false},
        piece{P_INVALID_PIECE},
        color{C_BLACK},
        taken_piece{P_EMPTY},
        takes{false},
        castling{false},
        half_move_before(0),
        promote(false),
        promote_piece(P_INVALID_PIECE),
        m_board_state_before{ 0 },
        m_board_key_before{ 0 },
        legal{false},
        legal_checked{false}
    {
    }

    Move(const Board& b) : Move()
    {
        m_board_state_before = b.get_flags();
        m_board_key_before = b.get_key();
        half_move_before = b.get_half_move();
    }

    bool operator==(const Move& o)
    {
        return dst == o.dst && src==o.src && piece == o.piece;
    }

    Move reverse() const
    {
        if (!takes) {
            throw chess_exception("cannot reverse if not capture");
        }
        Move m;
        m.dst = src;
        m.src = dst;
        m.takes = true;
        m.taken_piece = piece;
        m.piece = taken_piece;
        m.color = other_color(color);


        m.castling = castling;

        m.promote = promote;
        m.promote_piece = promote_piece;

        m.m_board_state_before = m_board_state_before;
        m.m_board_key_before = m_board_key_before;
        m.half_move_before = half_move_before;

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
