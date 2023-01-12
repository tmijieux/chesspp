
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
    constexpr uint64_t seed = u64(15925555970513767049UL);
    std::mt19937_64 gen64{ seed };
    std::uniform_int_distribution<uint64_t> rand{ 0, std::numeric_limits<uint64_t>::max() };
    std::cout << "rand1=" << rand(gen64) << "\n";
    std::cout << "rand2=" << rand(gen64) << "\n";
    std::cout << "rand3=" << rand(gen64) << "\n";
    std::cout << "seed1=" << seed << "\n";

    for (int s = 0; s < HASH_PARAM_SIZE; ++s) {
        HashParams::piece[s] = rand(gen64);
    }
    //for (int s = 0; s < NUM_SQUARE; ++s) {
    //    for (int p = 0; p < NUM_PIECE; ++p) {
    //        std::cout << "data=" << HashParams::piece[s * NUM_PIECE + p] << "\n";
    //    }
    //}
}

uint64_t HashMethods::full_hash(const Board &b)
{
    uint64_t hash = 0;
    for (uint8_t s = 0; s < 64; ++s) {
        Piece p = b.get_piece_at(s);
        Color c = b.get_color_at(s);
        if (c != C_EMPTY) {
            uint16_t p_idx = (s * NUM_PIECE) + (p - 1) + (6 * c);
            //std::cout << "FULLHASH piece_idx=" << p_idx << "\n";
            hash ^= HashParams::piece[PIECE_OFFSET + p_idx];
        }
    }
    Pos ep_pos = b.get_en_passant_pos();
    if (ep_pos.row != 0) {
        //std::cout << "FULLHASH ep=" << (int)ep_pos.column << "\n";

        hash ^= HashParams::piece[EN_PASSANT_OFFSET + ep_pos.column];
    }
    Color c = b.get_next_move();
    if (c == C_WHITE) {
        //std::cout << "FULLHASH iswhite\n";

        hash ^= HashParams::piece[SIDE_TO_MOVE_OFFSET];
    }
    uint8_t rights = b.get_castle_rights();
    for (uint8_t i = 0; i < 4; ++i) {
        if (rights & (1 << i)) {
            //std::cout << "FULLHASH castle "<< (int)i <<"\n";

            hash ^= HashParams::piece[CASTLING_OFFSET + i];
        }
    }
 
    return hash;
}

void HashMethods::make_move(const Board& b, uint64_t &hash, const Move &m, uint8_t castle_diff)
{
    // remove taken piece at dst
    {
        Piece p = b.get_piece_at(m.dst);
        Color c = b.get_color_at(m.dst);
        if (c != C_EMPTY) {
            int sq = m.dst.to_val();
            auto ix = (sq * NUM_PIECE) + (p - 1) + (6 * c);
            hash ^= HashParams::piece[PIECE_OFFSET + ix];
        }
    }

    // add moved piece at dst
    {
        Piece p = m.promote ? m.promote_piece : m.piece;
        Color c = m.color;
        int sq = m.dst.to_val();
        auto ix = (sq * NUM_PIECE) + (p - 1) + (6 * c);
        hash ^= HashParams::piece[PIECE_OFFSET + ix];
    }

    // remove moved piece at src
    {
        Piece p = b.get_piece_at(m.src);
        Color c = b.get_color_at(m.src);
        int sq = m.src.to_val();
        auto ix = (sq * NUM_PIECE) + (p - 1) + (6 * c);
        hash ^= HashParams::piece[PIECE_OFFSET + ix];
    }

    if (m.en_passant)
    {
        // remove en passant taken pawn
        Pos pos = b.get_en_passant_pos();
        Piece p = P_PAWN;
        Color c = b.get_color_at(pos);
        int sq = pos.to_val();
        auto ix = (sq * NUM_PIECE) + ((p - 1) + (6 * c));
        hash ^= HashParams::piece[PIECE_OFFSET + ix];
    }

    if (m.castling)
    {
        uint8_t dstCol = m.dst.column;
        uint8_t row = m.src.row;
        Pos rook_src{ row, u8(dstCol == 6 ? 7 : 0) };
        Pos rook_dst{ row, u8(dstCol == 6 ? 5 : 3) };

        {
            // add rook at dst
            Piece p = P_ROOK;
            Color c = m.color;
            int sq = rook_dst.to_val();
            auto ix = (sq * NUM_PIECE) + ((p - 1) + (6 * c));
            hash ^= HashParams::piece[PIECE_OFFSET + ix];
        }

        {
            // remove rook at src
            Piece p = P_ROOK;
            Color c = m.color;
            int sq = rook_src.to_val();
            auto ix = (sq * NUM_PIECE) + ((p - 1) + (6 * c));
            hash ^= HashParams::piece[PIECE_OFFSET + ix];
        }
    }

    if (b.has_en_passant()) // <-- previous state as en passant
    {
        // remove old state en passant file state 
        Pos p = b.get_en_passant_pos();
        hash ^= HashParams::piece[EN_PASSANT_OFFSET + p.column];
    }

    // new state has en passant
    if (m.piece == P_PAWN && std::abs(m.src.row-m.dst.row) == 2)
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

std::string HashMethods::to_string(uint64_t hash)
{
    return fmt::format("{:016X}", hash);
}
