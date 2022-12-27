#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <algorithm>

#include <cinttypes>
#include <string>

#include "./types.hpp"
#include "./move.hpp"


class invalid_fen_string : public std::exception {
    const char* what() const override {
        return "invalid fen string";
    }
};

class Board
{
private:
    uint8_t m_board[64];
    Color m_next_to_move;
    bool m_castle_rights[4];
    uint8_t m_half_move_counter; // for 50 moves rule
    uint32_t m_full_move_counter;

    Pos m_en_passant_pos;
    bool m_king_checked[2]; // 0 black,  1 white
    Pos m_king_pos[2]; // 0 black,  1 white

    bool compute_king_checked(Color) const;
    std::string m_position;

public:
    Board():
        m_next_to_move{C_INVALID_COLOR},
        m_half_move_counter{0},
        m_full_move_counter{1},
        m_en_passant_pos{-1,-1},
        m_position{}
    {
        std::fill(std::begin(m_board), std::end(m_board), 0);
        std::fill(std::begin(m_castle_rights), std::end(m_castle_rights), false);
        std::fill(std::begin(m_king_checked), std::end(m_king_checked), false);
    }

    /* read board state */
    Piece get_piece_at(const Pos& pos) const;
    Color get_color_at(const Pos& pos) const;
    Color get_next_move() const { return m_next_to_move; }
    bool get_castle_rights(CasleRightIndex idx) const  { return m_castle_rights[(uint8_t)idx]; }
    Pos get_en_passant_pos() const  { return m_en_passant_pos; }
    uint8_t get_half_move() const { return m_half_move_counter ; }
    uint32_t get_full_move() const { return m_full_move_counter ; }
    bool is_king_checked(Color clr) const { return m_king_checked[clr >> 4]; }
    const Pos& get_king_pos(Color clr) const { return m_king_pos[clr >> 4]; }
    const std::string& get_pos_string() const { return m_position; }
    bool is_square_attacked(const Pos& pos, Color clr) const;


    /* update board state */
    void set_piece_at(const Pos&pos, Piece piece, Color color);
    void set_next_move(Color color) { m_next_to_move = color; }
    void set_castle_rights(CasleRightIndex idx, bool right) { m_castle_rights[(uint8_t)idx] = right; }
    void set_en_passant_pos(const Pos &pos) { m_en_passant_pos = pos; }
    void set_half_move(uint8_t counter) { m_half_move_counter = counter; }
    void set_full_move(uint8_t counter) { m_full_move_counter = counter; }

    /* load positions */
    void load_initial_position();
    void load_position(const std::string& fen_position);

    /* make a move ! */
    void make_move(const Move& move);
    void unmake_move(const Move& move);


}; // class Board



#endif // CHESS_BOARD_H
