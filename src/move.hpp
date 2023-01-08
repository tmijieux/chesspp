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
    uint64_t killer_freq;
    int32_t see_value;

    Pos src;
    Pos dst;

    Color color;
    Piece piece;
    Piece taken_piece;
    Piece promote_piece;

    unsigned en_passant : 1;
    unsigned killer : 1;
    unsigned mate_killer : 1;
    unsigned hash_move : 1;
    unsigned takes : 1;
    unsigned castling : 1;
    unsigned legal : 1;
    unsigned legal_checked : 1;
    unsigned promote : 1;
    unsigned checks : 1;
    unsigned mate : 1;

    // remember some of state of board before move
    uint32_t m_board_state_before;
    uint64_t m_board_key_before;
    uint8_t half_move_before;

    Move():
        evaluation{-999999},
        killer_freq{0},
        see_value{0},
        killer{false},
        mate_killer{false},
        hash_move{false},
        en_passant{false},
        piece{P_INVALID_PIECE},
        color{C_BLACK},
        taken_piece{P_EMPTY},
        takes{false},
        castling{false},
        half_move_before(0),
        promote{ false },
        checks{ false },
        mate{false},
        promote_piece{ P_EMPTY },
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

    inline bool operator==(const Move& o)
    {
        return dst == o.dst
            && src ==o.src 
            && piece == o.piece
            && promote == o.promote
            && promote_piece == o.promote_piece;
    }

    Move reverse() const
    {
        if (!takes) {
            throw chess_exception("cannot reverse if not capture");
        }
        Move m = *this;
        m.dst = src;
        m.src = dst;
        m.takes = piece != P_EMPTY;
        m.taken_piece = piece;
        m.piece = taken_piece;
        m.color = other_color(color);
        if (m.takes && m.taken_piece == P_EMPTY) {
            std::abort();
        }


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


inline constexpr std::string piece_to_move_letter(Piece p) {
    switch (p) {
        case P_PAWN: return "";
        case P_ROOK: return "R";
        case P_BISHOP: return "B";
        case P_KNIGHT: return "N";
        case P_QUEEN: return "Q";
        case P_KING: return "K";
        default: return "X";
    }
}

inline constexpr std::string get_char_by_piece_pgn(Piece p)
{
    switch (p) {
        case P_PAWN: return "p";
        case P_ROOK: return "r";
        case P_BISHOP: return "b";
        case P_KNIGHT: return "n";
        case P_QUEEN: return "q";
        case P_KING: return "k";
        case P_EMPTY: return " ";
        default: return "X";
    }
}
inline constexpr Piece get_piece_by_char_pgn(char c)
{
    switch (c) {
        case 'R': return P_ROOK;
        case 'B': return P_BISHOP;
        case 'N': return P_KNIGHT;
        case 'Q': return P_QUEEN;
        case 'K': return P_KING;
        default: return P_PAWN;
    }
}


inline constexpr char get_char_by_piece(Piece p)
{
    switch (p) {
    case P_PAWN: return 'p';
    case P_ROOK: return 'r';
    case P_BISHOP: return 'b';
    case P_KNIGHT: return 'n';
    case P_QUEEN: return 'q';
    case P_KING: return 'k';
    case P_EMPTY: return ' ';
    default: return 'X';
    }
}


inline constexpr char get_fen_char_by_piece(Piece c)
{
    switch (c) {
    case P_PAWN: return'p';
    case P_ROOK: return'r';
    case P_BISHOP: return'b';
    case P_KNIGHT: return'n';
    case P_QUEEN: return'q';
    case P_KING: return  'k';
    default: return ' ';
    }
}


inline constexpr Piece get_piece_by_char_fen(char c)
{
    switch (c) {
    case 'p':
    case 'P':
        return P_PAWN;
    case 'r':
    case 'R':
        return P_ROOK;
    case 'b':
    case 'B':
        return P_BISHOP;
    case 'n':
    case 'N':
        return P_KNIGHT;
    case 'q':
    case 'Q':
        return P_QUEEN;
    case 'k':
    case 'K':
        return P_KING;
    default:
        return P_INVALID_PIECE;
    }
}





#endif // CHESS_MOVE_H
