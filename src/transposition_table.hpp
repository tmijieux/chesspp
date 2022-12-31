#ifndef CHESS_TRANSPOSITION_TABLE_H
#define CHESS_TRANSPOSITION_TABLE_H

#include <unordered_map>

#include "./types.hpp"

class Board;
struct Move;

enum HashParamsValues {
    NUM_PIECE = 6+6, // 0->5 black pieces, 6-11 white pieces
    NUM_SQUARE = 64,
    TOTAL_PIECES = NUM_PIECE * NUM_SQUARE,
    NUM_EN_PASSANT_FILE = 8,

    NUM_SIDE_TO_MOVE = 1,
    NUM_CASTLING_RIGHT = 4,

    PIECE_OFFSET = 0,
    EN_PASSANT_OFFSET = PIECE_OFFSET + TOTAL_PIECES,
    SIDE_TO_MOVE_OFFSET = EN_PASSANT_OFFSET + NUM_EN_PASSANT_FILE,
    CASTLING_OFFSET = SIDE_TO_MOVE_OFFSET + NUM_SIDE_TO_MOVE,

    HASH_PARAM_SIZE =
        TOTAL_PIECES
        + NUM_EN_PASSANT_FILE
        + NUM_SIDE_TO_MOVE
        + NUM_CASTLING_RIGHT
    ,

};
struct HashParams
{
    static uint64_t piece[HASH_PARAM_SIZE];
    static void init_params();
};

struct HashEntry {
    uint64_t key;
    uint16_t depth;
    int32_t score;
    int8_t hashmove_src;
    int8_t hashmove_dst;
    Piece promote_piece;

    unsigned lower_bound :1;
    unsigned upper_bound :1;
    unsigned exact_score :1;

    HashEntry() :
        key{ 0 },
        depth{ 0 },
        score{ 0 },
        hashmove_src{ 0 },
        hashmove_dst{ 0 },
        promote_piece{ Piece::P_EMPTY },
        lower_bound{ 0 },
        upper_bound{ 0 },
        exact_score{ 0 }
    {
    }

};

struct Hash
{
private:
    std::unordered_map<uint64_t,HashEntry> m_entries;
public:
    Hash(){}

    void init(size_t size)
    {
        m_entries.reserve(size);
    }

    void clear()
    {
        m_entries.clear();
    }

    inline HashEntry& get(uint64_t k) {
        auto it = m_entries.find(k);
        if (it == m_entries.end()) {
            auto res = m_entries.emplace(k, HashEntry{});
            return res.first->second;
        }
        return m_entries[k];
    }

    static uint64_t full_hash(const Board& b);
    static void make_move(const Board& b, uint64_t& hash, const Move& move, uint8_t castle_diff);
    static std::string to_string(uint64_t hash);

};

#endif // CHESS_TRANSPOSITION_TABLE_H
