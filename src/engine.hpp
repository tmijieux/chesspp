#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <map>
#include <unordered_map>
#include <algorithm>
#include <thread>

#include "./timer.hpp"
#include "./board.hpp"
#include "./move_generation.hpp"
#include "./evaluation.hpp"
#include "./uci.hpp"

enum NodeType {
    UNDEFINED = 0,
    PV_NODE,
    CUT_NODE,
    ALL_NODE,
    NO_MOVE,
};

struct Stats {
    uint32_t num_cutoffs;
    uint32_t num_cut_by_killer;
    uint32_t num_cut_by_hash_move;
    uint32_t num_leaf_nodes;

    uint32_t num_faillow_node;
    uint32_t num_pvnode;

    uint32_t num_nodes;

    uint32_t num_move_maked;//including illegal moves
    uint32_t num_move_skipped;
    uint32_t num_move_generated;
    uint32_t num_hash_hits;
    uint32_t num_hash_conflicts;

    uint32_t num_aspiration_tries;
    uint32_t num_aspiration_failures;



    Stats() :
        num_cutoffs{ 0 },
        num_cut_by_killer{ 0 },
        num_cut_by_hash_move{ 0 },
        num_faillow_node{0},
        num_pvnode{ 0 },
        num_nodes{ 0 },
        num_leaf_nodes{ 0 },
        num_move_maked{0},
        num_move_skipped{ 0 },
        num_move_generated{ 0 },
        num_hash_hits{ 0 },
        num_hash_conflicts{ 0 },
        num_aspiration_tries{ 0 },
        num_aspiration_failures{ 0 }
    {
    }
};

struct NegamaxEngine
{
private:
    KillerMoves m_killers;
    uint32_t m_max_depth;
    uint32_t m_current_max_depth; // iterative deepening;


    //  current_max_depth, current_depth
    std::map<uint32_t, std::map<uint32_t, Stats>> m_stats;

    Timer m_move_ordering_timer;
    Timer m_move_ordering_mvv_lva_timer;
    Timer m_make_move_timer;
    Timer m_unmake_move_timer;
    Timer m_move_generation_timer;

    Timer m_quiescence_timer;
    Timer m_evaluation_timer;
    Timer m_make_move2_timer;
    Timer m_unmake_move2_timer;
    Timer m_move_generation2_timer;

    uint64_t m_total_nodes_prev;
    uint64_t m_total_nodes_prev_prev;
    uint64_t m_regular_nodes;
    uint64_t m_leaf_nodes;
    uint64_t m_quiescence_nodes;

    uint64_t m_run_id;
    bool m_uci_mode;
    GoParams m_uci_go_params;
    std::thread m_thread;
    bool m_stop_required;
    bool m_stop_required_by_timeout;
    bool m_running;

    void _start_uci_background(Board& b);
    void reset_timers();

    void extract_pv_from_tt(Board& b, MoveList& pv, int depth);

    std::vector<uint64_t> m_positions_sequence;//store Zkey

public:
    NegamaxEngine():
        m_max_depth{ 0 },
        m_current_max_depth{ 0 },
        m_total_nodes_prev{ 0 },
        m_total_nodes_prev_prev{ 0 },
        m_regular_nodes{ 0 },
        m_leaf_nodes{ 0 },
        m_quiescence_nodes{ 0 },
        m_run_id{ 0 },
        m_uci_mode{ false },
        m_stop_required{ false },
        m_stop_required_by_timeout{ false },
        m_running{ false }
    {
    }
    Hash<HashEntry> m_hash;

    void init_hash() { m_hash.init(1000); }
    void clear_hash() { m_hash.clear(); init_hash(); }

    void stop();
    bool is_running() const { return m_running; }
    void start_uci_background(Board& b);

    void set_uci_mode(bool uci_mode, GoParams& params)
    {
        m_uci_mode = uci_mode;
        m_uci_go_params = params;
    }
    void display_timers(Timer&);
    void display_stats();
    void display_stats(int current_maxdepth);
    void display_node_infos(Timer&);
    void display_readable_pv(Board& b, const MoveList& pvLine);

    int32_t quiesce(Board& b, int color, int32_t alpha, int32_t beta, int depth);
    int32_t negamax(
        Board& b,
        int max_depth, int remaining_depth, int ply,
        int color,
        int32_t alpha, int32_t beta,
        MoveList* topLevelOrdering,
        bool internal,
        NodeType &node_type,
        MoveList &parentPvLine
        // TranspositionTable &tt,
    );
    bool iterative_deepening(
        Board& b, int max_depth, Move* bestMove, bool* moveFound,
        uint64_t max_time
    );

    void set_max_depth(int maxdepth);
    void set_current_maxdepth(int maxdepth) { m_current_max_depth = maxdepth; }
    uint64_t perft(Board &b, int max_depth, int remaining_depth, 
        std::vector<uint64_t> &res, Hash<PerftHashEntry>&);
    void do_perft(Board &b, int depth);
};



std::pair<bool,Move> find_best_move(Board& b);

#endif // CHESS_ENGINE_H
