#include <iostream>
#include "./evaluation.hpp"
#include "./move_generation.hpp"

int32_t piece_value(Piece p)
{
    switch (p) {
        case P_PAWN: return 100;
        case P_ROOK: return 550;
        case P_BISHOP: return 300;
        case P_KNIGHT: return 300;
        case P_QUEEN: return 900;
        case P_KING: return 15000;
        default: return 0;
    }
}

void reorder_mvv_lva(const Board& b, MoveList& moveList)
{
    std::sort(moveList.begin(), moveList.end(), [](auto& a, auto& b) {
        if (!a.takes || !b.takes) {
            throw std::exception("some move are not takes here");
        }
        int aa = piece_value(a.taken_piece) - piece_value(a.piece);
        int bb = piece_value(b.taken_piece) - piece_value(b.piece);
        return aa > bb;
    });
}


int32_t quiescence(Board& b, int color, int32_t alpha, int32_t beta)
{
    int32_t standing_pat = color * evaluate_board(b);
    if (standing_pat >= beta) {
        return beta;
    }
    if (alpha < standing_pat) {
        alpha = standing_pat;
    }

    MoveList moveList = enumerate_moves(b , true);
    reorder_mvv_lva(b, moveList);

    for (auto& move : moveList) {
        b.make_move(move);
        int32_t val = -quiescence(b, -color, -beta, -alpha);
        b.unmake_move(move);

        if (val >= beta) {
            return beta;
        }
        if (val > alpha) {
            alpha = val;
        }
    }
    return alpha;
}

int32_t evaluate_board(const Board& b)
{
    uint32_t value = 0;
    Pos center{ 3,3 };

    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i / 8, i % 8 };

        Color c = b.get_color_at(pos);
        Piece p = b.get_piece_at(pos);
        int32_t coeff = c == C_WHITE ? +1 : -1;

        int32_t pvalue = piece_value(p);
        value += coeff * pvalue;
        if (p == P_PAWN) {
            value += 3*pos.row;
        }
        //if (p != P_EMPTY) {
        //    float x = pos.row - center.row;
        //    float y = pos.column - center.column;
        //    float dis = x * x + y * y;
        //    value -= coeff * (int32_t)dis;
        //}
    }
    auto attackListWhite = enumerate_attacks(b, C_WHITE);
    auto attackListBlack = enumerate_attacks(b, C_BLACK);
    for (const auto& m : attackListWhite) {
        if (3 <= m.dst.column && m.dst.column <= 4 && 3 <= m.dst.row && m.dst.row <= 4) {
            value += 8;
        }
        else if (2 <= m.dst.column && m.dst.column <= 4 && 2 <= m.dst.row && m.dst.row <= 5) {
            value += 2;
        }
        else {
            value += 1;
        }
    }
    for (const auto& m : attackListBlack) {
        if (3 <= m.dst.column && m.dst.column <= 4 && 3 <= m.dst.row && m.dst.row <= 4) {
            value -= 3;
        }
        else if (2 <= m.dst.column && m.dst.column <= 4 && 2 <= m.dst.row && m.dst.row <= 5) {
            value -= 2;
        }
        else {
            value -= 1;
        }
    }
    return value;
}
