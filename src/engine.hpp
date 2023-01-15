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


struct Node {
    NodeType type;
    NodeType expected_type;

    int32_t score;
    uint32_t num_move_maked;
    uint32_t num_legal_move;
    uint64_t zkey;
    bool null_window;
    bool has_hash_move;
    bool found_best_move;
    bool use_aspiration;
    Move hash_move;
    Move best_move;
    MoveList pvLine;

    Node() :
        type{ NodeType::UNDEFINED },
        score{ -999999 },
        num_move_maked{ 0 },
        num_legal_move{ 0 },
        zkey{ 0 },
        null_window{ false },
        has_hash_move{ false },
        found_best_move{ false },
        use_aspiration{ false }
    {
    }
};

struct Stats {
    uint32_t num_cut_by_killer;
    uint32_t num_cut_by_mate_killer;
    uint32_t num_cut_by_hash_move;
    uint32_t num_leaf_nodes;

    uint32_t reduced_by_1;
    uint32_t reduced_by_2;

    uint32_t reduced_by_1_fail;
    uint32_t reduced_by_2_fail;

    uint32_t num_cut_nodes;
    uint32_t num_faillow_nodes;
    uint32_t num_pv_nodes;
    uint32_t num_match_expected;

    uint32_t num_nodes;

    uint32_t num_move_maked;//including illegal moves
    uint32_t num_move_skipped;
    uint32_t num_move_generated;
    uint32_t num_hash_hits;
    uint32_t num_hash_conflicts;

    uint32_t num_aspiration_tries;
    uint32_t num_aspiration_failures;



    Stats() :
        num_cut_by_killer{ 0 },
        num_cut_by_mate_killer{ 0 },
        num_cut_by_hash_move{ 0 },
        num_leaf_nodes{ 0 },
        reduced_by_1{0},
        reduced_by_2{ 0 },
        reduced_by_1_fail{ 0 },
        reduced_by_2_fail{ 0 },
        num_cut_nodes{ 0 },
        num_faillow_nodes{0},
        num_pv_nodes{ 0 },
        num_match_expected{ 0 },
        num_nodes{ 0 },
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
    HistoryMoves m_history;
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
    bool m_has_current_root_evaluation;
    int32_t m_current_root_evaluation;

    uint64_t m_run_id;
    bool m_uci_mode;
    GoParams m_uci_go_params;
    std::thread m_thread;
    bool m_stop_required;
    bool m_stop_required_by_timeout;
    bool m_running;

    std::vector<uint64_t> m_positions_sequence;//store Zkey


    void _start_uci_background(Board& b);
    void reset_timers();

    void extract_pv_from_tt(Board& b, MoveList& pv, int depth);
    void handle_no_move_available(Board &b);

    bool lookup_hash(Board &b, Node &node, Stats& stats,
        int32_t remaining_depth, int32_t ply,
        int32_t alpha, int32_t beta,
        Node &parent_node);
    void update_cut_heuristics(Move& move, Node& node, int32_t ply);
    void update_hash(Node &node, Stats& stats, int remaining_depth);

public:
    NegamaxEngine():
        m_max_depth{ 0 },
        m_current_max_depth{ 0 },
        m_total_nodes_prev{ 0 },
        m_total_nodes_prev_prev{ 0 },
        m_regular_nodes{ 0 },
        m_leaf_nodes{ 0 },
        m_quiescence_nodes{ 0 },
        m_has_current_root_evaluation{ false },
        m_current_root_evaluation{0},
        m_run_id{ 0 },
        m_uci_mode{ false },
        m_stop_required{ false },
        m_stop_required_by_timeout{ false },
        m_running{ false }
    {
        m_history.resize( 2 * 64 * 64);
        std::cout  <<"m_history size() == "<<m_history.size() <<"\n";
        std::fill(m_history.begin(), m_history.end(), 0);
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

    int32_t quiesce(Board& b, int color, int32_t alpha, int32_t beta, uint32_t ply);
    int32_t negamax(
        Node& parent_node, Node &node,
        Board& b,
        int max_depth, int remaining_depth, uint32_t ply,
        int color,
        int32_t alpha, int32_t beta,
        bool internal
    );
    bool iterative_deepening(
        Board& b, int max_depth, Move* bestMove, bool* moveFound,
        uint64_t max_time
    );

    void set_max_depth(int maxdepth);
    void set_current_maxdepth(int maxdepth) { m_current_max_depth = maxdepth; }
    uint64_t perft(Board &b, uint32_t max_depth, uint32_t remaining_depth,
        std::vector<uint64_t> &res, Hash<PerftHashEntry>&);
    void do_perft(Board &b, uint32_t depth);
};



std::pair<bool,Move> find_best_move(Board& b);

#endif // CHESS_ENGINE_H
