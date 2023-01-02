#include <iostream>
#include "./evaluation.hpp"
#include "./move_generation.hpp"


int32_t evaluate_board(const Board& b)
{
    int32_t value = 0;
    int32_t material = 0;
    int32_t pawn_position = 0;

    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i };

        Color c = b.get_color_at(pos);
        Piece p = b.get_piece_at(pos);
        int32_t sign = 2 * c -1;// BLACK=-1 WHITE=-1
        int32_t pvalue = piece_value(p);

        material += sign * pvalue;

        int32_t r0 = 7 * (1 - c);

        double coeff = 2.5 - std::abs(3.5 - pos.column);
        pawn_position += coeff * (p == P_PAWN) * (pos.row - r0) * 10;

        int32_t r1 = 3+c;
        int r2 = r1 + sign;
        int q = sign * (pos.row - r1) >= 0 &&  (c != 2);
        pawn_position += q * (pos.row - r2) * 20;
    }
    value += material;
    value += pawn_position;
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
