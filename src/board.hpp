#ifndef CHESS_BOARD_H
#define CHESS_BOARD_H

#include <algorithm>
#include <cinttypes>
#include <string>

class Board;
struct Move;
struct NullMove;

#include "./types.hpp"
#include "./transposition_table.hpp"

std::string write_fen_position(const Board&);
enum {
    CAN_EN_PASSANT = 8,
};

class Board
{
private:
    uint8_t m_board[32];
    //std::string m_position;
    uint16_t m_ply_count;
    uint8_t m_half_move_counter; // for 50 moves rule

    uint32_t m_flags;
    uint64_t m_key;

    enum IDX {
        KING_POS_I = 0,
        //KING_POS_LENGTH = 12,
        CASTLE_I = 12,
        //CASTLE_LENGTH = 4,
        EN_PASSANT_I = 16,
        //EN_PASSANT_LENGTH = 4,
        KING_CHECKED_I = 20,
        //KING_CHECKED_LENGTH = 2,
        NEXT_COLOR_I = 22,
        //NEXT_COLOR_LENGTH = 1,

    };
    //unsigned m_king_pos : 12; // 6 black + 6 white
    //unsigned m_castle_rights : 4;
    //unsigned m_en_passant_file : 4; //3(file) + 1 (boolean)
    //unsigned m_king_checked : 2; // 1 black + 1 white
    //unsigned m_next_color_to_move : 1;


    //  12 + 4 + 4 + 2 + 1= 23

    int8_t compute_king_checked(Color);

public:
    Board():
        m_ply_count{0},
        m_half_move_counter{0},
        m_flags{ 0 },
        m_key{ 0 }
  /*      m_en_passant_file{ 0 },
        m_castle_rights{0},
        m_king_checked{ 0 },
        m_next_color_to_move{ C_WHITE }*/
    {
        std::fill(std::begin(m_board), std::end(m_board), 0);
    }

    /* read board state */
    bool check_valid_state();
    Piece get_piece_at(const Pos& pos) const;
    Color get_color_at(const Pos& pos) const;
    Color get_next_move() const { return (Color)((m_flags >> NEXT_COLOR_I) & 1); }
    uint8_t get_castle_rights() const { return (uint8_t)(m_flags >> CASTLE_I) & (0x0F); }

    Pos get_en_passant_pos() const {
        if (has_en_passant()) {
            uint8_t col = (m_flags >> EN_PASSANT_I) & (0x07);
            return col + 8 * (3 + get_next_move());
        }
        return 0;
    }

    bool has_en_passant() const { return ((m_flags >> EN_PASSANT_I) & 0x08) != 0; }
    uint8_t get_half_move() const { return m_half_move_counter ; }
    uint16_t get_full_move() const { return m_ply_count+2/2 ; }
    uint32_t get_flags() const { return m_flags;  }
    std::string get_key_string() const { return HashMethods::to_string(get_key()); }
    uint64_t get_key() const { return m_key; }

    bool is_king_checked(Color clr) const {
        auto val = (m_flags >> KING_CHECKED_I) & 0x3;
        return (val & (1<<clr)) != 0;
    }

    Pos get_king_pos(Color clr) const {
        auto val = (m_flags >> KING_POS_I) & 0xFFF;
        return (val >> (6 * clr)) & 0x3F;
    }
    std::string get_pos_string() const { return write_fen_position(*this); }
    std::string get_fen_string() const { return write_fen_position(*this); }

    bool is_square_attacked(const Pos& pos, Color clr);


    /* update board state */
    void set_king_checked(uint8_t val) {
        m_flags = (m_flags & ~(0x3u << KING_CHECKED_I)) ^ (val << KING_CHECKED_I);
    }
    void set_king_pos(const Pos &p, Color c) {
        uint8_t x = p.column + p.row * 8;

        int val = (m_flags >> KING_POS_I) & 0xFFFu;
        int newval = (val & (0x3Fu << (6 * (1 - c)))) | ((x & 0x3Fu) << (6 * c));
        m_flags = (m_flags & ~(0xFFFu << KING_POS_I)) ^ (newval << KING_POS_I);
    }
    void set_piece_at(const Pos& pos, Piece piece, Color color);
    void set_next_move(Color color) {
        //            remove color               and   set it to new value
        m_flags = (m_flags & ~(1u << NEXT_COLOR_I)) | (color << NEXT_COLOR_I);
    }
    void set_castle_rights(uint32_t val) {
        auto mask = ~(0x0Fu << CASTLE_I);
        m_flags = (m_flags & mask) | (val << CASTLE_I);
    }

    void set_en_passant_pos(uint8_t val) {
        auto mask = ~(0x0Fu << EN_PASSANT_I);
        m_flags = (m_flags & mask) ^ (val << EN_PASSANT_I);
    }
    void set_half_move(uint8_t counter) { m_half_move_counter = counter; }
    void set_ply_count(uint16_t counter) { m_ply_count = counter; }

    /* load positions */
    void load_initial_position();
    void load_position(const std::string& fen_position);

    /* make a move ! */
    void make_move(const Move& move);
    void unmake_move(const Move& move);

    /* dont make a move ! */
    void make_null_move(NullMove& m);
    void unmake_null_move(NullMove& m);
}; // class Board


void load_test_position(Board &b, int position);
#endif // CHESS_BOARD_H
