
#include <vector>
#include <random>
#include <iostream>

#include "fmt/format.h"

#include "./transposition_table.hpp"
#include "./board.hpp"
#include "./move.hpp"

uint64_t HashParams::piece[HASH_PARAM_SIZE];

void HashParams::init_params()
{
    uint64_t seed = 0XDEADBEEFCAFEBABEULL;
    std::mt19937_64 gen64{ seed };
    std::uniform_int_distribution<uint64_t> rand{ 0, std::numeric_limits<uint64_t>::max() };

    for (int s = 0; s < HASH_PARAM_SIZE; ++s) {
        HashParams::piece[s] = rand(gen64);
    }
    //for (int s = 0; s < NUM_SQUARE; ++s) {
    //    for (int p = 0; p < NUM_PIECE; ++p) {
    //        std::cout << "data=" << HashParams::piece[s * NUM_PIECE + p] << "\n";
    //    }
    //}
}

uint64_t Hash::full_hash(const Board& b)
{
    uint64_t hash = 0;
    for (int8_t s = 0; s < 64; ++s) {
        Piece p = b.get_piece_at(s);
        Color c = b.get_color_at(s);
        if (c != C_EMPTY) {
            int8_t p_idx = (p - 1) + (6 * c);
            hash ^= HashParams::piece[PIECE_OFFSET + p_idx];
        }
    }
    Color c = b.get_next_move();
    if (c == C_WHITE) {
        hash ^= HashParams::piece[SIDE_TO_MOVE_OFFSET];
    }
    uint8_t rights = b.get_castle_rights();
    for (uint8_t i = 0; i < 4; ++i) {
        if (rights & (1 << i)) {
            hash ^= HashParams::piece[CASTLING_OFFSET + i];
        }
    }
    Pos ep_pos = b.get_en_passant_pos();
    if (ep_pos.row != 0) {
        hash ^= HashParams::piece[EN_PASSANT_OFFSET + ep_pos.column];
    }
    return hash;
}

void Hash::make_move(const Board& b, uint64_t &hash, const Move &m, uint8_t castle_diff)
{
    // remove taken piece at dst
    {
        Piece dp = b.get_piece_at(m.dst);
        Color dc = b.get_color_at(m.dst);
        if (dc != C_EMPTY) {
            int y = m.dst.row * 8 + m.dst.column;
            auto ix = (y * NUM_PIECE) + ((dp - 1) + (6 * dc));
            hash ^= HashParams::piece[PIECE_OFFSET + ix];
        }
    }

    // add moved piece at dst
    {
        Piece dp2 = m.promote ? m.piece : m.promote_piece;
        Color dc2 = m.color;
        int dy = m.dst.row * 8 + m.dst.column;
        auto dix = (dy * NUM_PIECE) + ((dp2 - 1) + (6 * dc2));
        hash ^= HashParams::piece[PIECE_OFFSET + dix];
    }


    // remove moved piece at src
    {
        Piece sp = b.get_piece_at(m.src);
        Color sc = b.get_color_at(m.src);
        int sy = m.src.row * 8 + m.src.column;
        auto six = (sy * NUM_PIECE) + ((sp - 1) + (6 * sc));
        hash ^= HashParams::piece[PIECE_OFFSET + six];
    }

    if (m.en_passant)
    {
        // remove en passant taken pawn
        Pos pos = b.get_en_passant_pos();
        Piece ep = P_PAWN;
        Color ec = b.get_color_at(pos);
        int ey = pos.row * 8 + pos.column;
        auto eix = (ey * NUM_PIECE) + ((ep - 1) + (6 * ec));
        hash ^= HashParams::piece[PIECE_OFFSET + eix];
    }

    if (m.castling)
    {
        uint8_t dstCol = m.dst.column;
        uint8_t row = m.src.row;
        Pos rook_src{ row, uc(dstCol == 6 ? 7 : 0) };
        Pos rook_dst{ row, uc(dstCol == 6 ? 5 : 3) };

        {
            // add rook at dst
            Piece dp2 = P_ROOK;
            Color dc2 = m.color;
            uint8_t dy = rook_dst.row * 8 + rook_dst.column;
            auto dix = (dy * NUM_PIECE) + ((dp2 - 1) + (6 * dc2));
            hash ^= HashParams::piece[PIECE_OFFSET + dix];
        }

        {
            // remove rook at src
            Piece sp = P_ROOK;
            Color sc = m.color;
            uint8_t sy = rook_src.row * 8 + rook_src.column;
            auto six = (sy * NUM_PIECE) + ((sp - 1) + (6 * sc));
            hash ^= HashParams::piece[PIECE_OFFSET + six];
        }
    }

    if (b.has_en_passant()
        || (m.piece==P_PAWN && std::abs(m.src.row-m.dst.row)==2))
    {
        // en passant file state remove old state
        Pos p = b.get_en_passant_pos();
        hash ^= HashParams::piece[EN_PASSANT_OFFSET + p.column];
    }

    if (m.piece == P_PAWN && std::abs(m.src.row-m.dst.row)==2)
    {
        // en passant file state add new state
        hash ^= HashParams::piece[EN_PASSANT_OFFSET + m.dst.column];
    }

    // update side to move
    hash ^= HashParams::piece[SIDE_TO_MOVE_OFFSET];

    // castling rights
    for (uint8_t i = 0; i < 4; ++i) {
        if (castle_diff & (1 << i)) {
            hash ^= HashParams::piece[CASTLING_OFFSET+i];
        }
    }

}

std::string Hash::to_string(uint64_t hash)
{
    return fmt::format("{:016X}", hash);
}
