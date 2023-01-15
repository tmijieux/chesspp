#ifndef CHESS_TRANSPOSITION_TABLE_H
#define CHESS_TRANSPOSITION_TABLE_H

#include <unordered_map>

#include "./types.hpp"

class Board;
struct Move;
struct NullMove;

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

struct PerftHashEntry {
    uint64_t key;
    uint16_t depth;
    uint64_t value;
    uint64_t nummoves;
    PerftHashEntry() :
        key{ 0 },
        depth{ 0 },
        value{ 0 },
        nummoves{ 0 }
    {

    }
};
struct HashEntry {
    uint64_t key;
    uint16_t depth;
    int32_t score;
    int8_t hashmove_src;
    int8_t hashmove_dst;
    Piece promote_piece;

    #ifdef CHESS_DEBUG
    std::string fen;
    #endif
    NodeType node_type;
    unsigned is_null_window : 1;

    HashEntry() :
        key{ 0 },
        depth{ 0 },
        score{ 0 },
        hashmove_src{ 0 },
        hashmove_dst{ 0 },
        promote_piece{ Piece::P_EMPTY },
        node_type{NodeType::UNDEFINED},
        is_null_window { 0 }
    {
    }

};

template<typename E = HashEntry>
struct Hash
{
private:
    std::unordered_map<uint64_t,E> m_entries;
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

    inline E& get(uint64_t bkey) {

        //uint64_t mask = (1 << 27) - 1;
        //uint32_t key = (uint32_t)(bkey & mask);
        uint64_t key = bkey;

        auto it = m_entries.find(key);
        if (it == m_entries.end()) {
            auto res = m_entries.emplace(key, E{});
            return res.first->second;
        }
        return it->second;
    }


};

struct HashMethods {
    static uint64_t full_hash(const Board& b);
    static void make_move(const Board& b, uint64_t& hash, const Move& move, uint8_t castle_diff);
    static std::string to_string(uint64_t hash);

    static void make_null_move(const Board& b, uint64_t& hash, const NullMove &m);

};

#endif // CHESS_TRANSPOSITION_TABLE_H
