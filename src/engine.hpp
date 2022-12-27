#ifndef CHESS_ENGINE_H
#define CHESS_ENGINE_H

#include <map>
#include <unordered_map>

#include "./timer.hpp"
#include "./board.hpp"
#include "./move_generation.hpp"
#include "./evaluation.hpp"

struct Stats {
    int32_t num_cutoffs;
    int32_t num_cut_by_killer;
    int32_t num_cut_by_best_pv;
    int32_t num_leaf_nodes;

    int32_t num_faillow_node;
    int32_t num_pvnode;

    int32_t num_nodes;

    int32_t num_move_visited;//including illegal moves
    int32_t num_move_skipped;
    int32_t num_move_generated;



    Stats() :
        num_cutoffs{ 0 },
        num_cut_by_killer{ 0 },
        num_cut_by_best_pv{ 0 },
        num_faillow_node{0},
        num_pvnode{ 0 },
        num_nodes{ 0 },
        num_leaf_nodes{ 0 },

        num_move_visited{0},
        num_move_skipped{ 0 },
        num_move_generated{ 0 }
    {
    }
};

using Evaluation = std::unordered_map<std::string,int32_t>;
struct NegamaxEngine
{
private:
    Evaluation m_evaluation;
    KillerMoves m_killers;
    uint32_t m_max_depth;
    uint32_t m_current_max_depth; // iterative deepening;

    //  current_max_depth, current_depth
    std::map<uint32_t, std::map<uint32_t, Stats>> m_stats;

    Timer m_evaluation_timer;
    Timer m_move_ordering_timer;
    Timer m_move_ordering_mvv_lva_timer;
    Timer m_make_move_timer;
    Timer m_unmake_move_timer;
    Timer m_quiescence_timer;
    Timer m_move_generation_timer;

    Timer m_make_move2_timer;
    Timer m_unmake_move2_timer;
    Timer m_move_generation2_timer;

    uint64_t m_total_nodes;
    uint64_t m_total_quiescence_nodes;

public:
    bool m_required_stop;
    bool m_running;

private:
    void reset_timers()
    {
        m_evaluation_timer.reset();
        m_move_generation_timer.reset();
        m_move_ordering_timer.reset();
        m_move_ordering_mvv_lva_timer.reset();
        m_make_move_timer.reset();
        m_unmake_move_timer.reset();
        m_quiescence_timer.reset();

        m_move_generation2_timer.reset();
        m_make_move2_timer.reset();
        m_unmake_move2_timer.reset();
    }


    void display_timers()
    {
        std::cout << "m_evaluation_time=" << m_evaluation_timer.get_length() << "\n";
        std::cout << "m_move_ordering_time=" << m_move_ordering_timer.get_length() << "\n";
        std::cout << "m_move_ordering_mvv_lva_time=" << m_move_ordering_mvv_lva_timer.get_length() << "\n";

        std::cout << "m_move_generation_time=" << m_move_generation_timer.get_length() << "\n";
        std::cout << "m_make_move_time=" << m_make_move_timer.get_length() << "\n";
        std::cout << "m_unmake_move_time=" << m_unmake_move_timer.get_length() << "\n";
        std::cout << "---\n";
        std::cout << "m_quiescence_timer=" << m_quiescence_timer.get_length() << "\n";
        std::cout << "---\n";
        std::cout << "m_move_generation2_time=" << m_move_generation2_timer.get_length() << "\n";
        std::cout << "m_make_move2_time=" << m_make_move2_timer.get_length() << "\n";
        std::cout << "m_unmake_move2_time=" << m_unmake_move2_timer.get_length() << "\n";
    }

public:
    NegamaxEngine() :
        m_max_depth{ 0 },
        m_current_max_depth{ 0 },
        m_total_nodes{ 0 },
        m_total_quiescence_nodes{ 0 },
        m_required_stop{false},
        m_running{false}
    {

    }

    int32_t quiesce(Board& b, int color, int32_t alpha, int32_t beta, int depth);

    int32_t negamax(
        Board& b,
        int max_depth, int remaining_depth, int current_depth,
        int color,
        int32_t alpha, int32_t beta,
        MoveList& parentPv,
        const MoveList& previousPv,
        MoveList* topLevelOrdering
        // TranspositionTable &tt,
    );

    void iterative_deepening(
        Board& b, int max_depth, Move* bestMove, bool* moveFound );

    void set_max_depth(int maxdepth)
    {
        m_max_depth = maxdepth;
        m_killers.clear();
        m_killers.resize(maxdepth);

        m_stats.clear();
        for (int depth = 1; depth <= maxdepth; ++depth) {
            for (int curdepth = 0; curdepth < depth; ++curdepth)
            {
                m_stats[depth][curdepth] = Stats{};
            }
        }
    }

    void set_current_maxdepth(int maxdepth)
    {
        m_current_max_depth = maxdepth;
    }

    void display_cutoffs()
    {
        for (auto& [maxdepth, stats_at_depth] : m_stats) {
            display_cutoffs(maxdepth);
        }
    }

    void display_cutoffs(int current_maxdepth)
    {
        std::cout << "cutoffs for current_maxdepth=" << current_maxdepth << "\n";
        for (auto& [depth, stats] : m_stats[current_maxdepth]) {

            double percent = (double)stats.num_move_visited / std::max(stats.num_move_generated,1);
            double percent2 = (double)stats.num_cutoffs / std::max(stats.num_nodes,1);

            std::cout << "d=" << depth
                      << "\n   NODES total=" << stats.num_nodes
                      << " leaf="<<stats.num_leaf_nodes
                      << " cutoffs=" << stats.num_cutoffs
                      << " (" << (int)(percent2 * 100.0) << "%) "

                      << " pv=" << stats.num_pvnode
                      << " faillow= " << stats.num_faillow_node
                      << " cut_by_killer= " << stats.num_cut_by_killer
                      << " cut_by_best_pv= " << stats.num_cut_by_best_pv

                      << "\n   MOVES generated=" << stats.num_move_generated
                      << " skipped=" << stats.num_move_skipped
                      << " visited=" << stats.num_move_visited
                      << " ("<< (int)(percent*100.0) <<"%) "
                      << "\n\n";

        }
    }

};



std::pair<bool,Move> find_best_move(Board& b);

#endif // CHESS_ENGINE_H
