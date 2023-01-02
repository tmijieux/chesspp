#include <iostream>
#include "./evaluation.hpp"
#include "./move_generation.hpp"


int32_t evaluate_board(const Board& b)
{
    uint32_t value = 0;

    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i };

        Color c = b.get_color_at(pos);
        Piece p = b.get_piece_at(pos);
        int32_t coeff = c == C_WHITE ? +1 : -1;

        int32_t pvalue = piece_value(p);
        value += coeff * pvalue;
        if (p == P_PAWN) {
            if (c == C_WHITE)
            {
                value += pos.row - 0;
            }
            else
            {
                value -= 7 - pos.row;
            }

            if (pos.row >= 5 && c == C_WHITE)
            {
                value += (pos.row - 4) * 80;
            }
            else if (pos.row <= 2 && c == C_BLACK)
            {
                value -= (3 - pos.row) * 80;
            }
        }
    }
    //auto attackListWhite = enumerate_attacks(b, C_WHITE);
    //auto attackListBlack = enumerate_attacks(b, C_BLACK);
    //for (const auto& m : attackListWhite) {
    //    if (3 <= m.dst.column && m.dst.column <= 4
    //        && 3 <= m.dst.row && m.dst.row <= 4) {
    //        // center
    //        value += 8;
    //    }
    //    else if (2 <= m.dst.column && m.dst.column <= 4
    //             && 2 <= m.dst.row && m.dst.row <= 5) {
    //        // large center
    //        value += 2;
    //    }
    //    else {
    //        // else
    //        value += 1;
    //    }
    //    // add points for square controlled around enemy king
    //    value += 3 * (
    //        std::abs(b.get_king_pos(C_BLACK).row - m.dst.row) <= 1
    //        && std::abs(b.get_king_pos(C_BLACK).column - m.dst.column) <= 1);
    //}
    //for (const auto& m : attackListBlack) {
    //    if (3 <= m.dst.column && m.dst.column <= 4 && 3 <= m.dst.row && m.dst.row <= 4) {
    //        value -= 3;
    //    }
    //    else if (2 <= m.dst.column && m.dst.column <= 4 && 2 <= m.dst.row && m.dst.row <= 5) {
    //        value -= 2;
    //    }
    //    else {
    //        value -= 1;
    //    }
    //    value -= 3 * (
    //        std::abs(b.get_king_pos(C_WHITE).row - m.dst.row) <= 1
    //        && std::abs(b.get_king_pos(C_WHITE).column - m.dst.column) <= 1);
    //}
    return value;
}
