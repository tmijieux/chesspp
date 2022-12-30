#include <iostream>

#include <fmt/format.h>

#include "./move_ordering.hpp"
#include "./engine.hpp"
#include "./move_generation.hpp"
#include "./move.hpp"
#include "./timer.hpp"
#include "./evaluation.hpp"


int32_t NegamaxEngine::quiesce(
    Board& b, int color, int32_t alpha, int32_t beta, int current_depth)
{
    if (m_required_stop) {
        return beta; // fail-high immediately
    }

    int32_t standing_pat;
    //const auto &pos_string = b.get_pos_string();
    //auto it = m_evaluation.find(pos_string);
    // if (it != m_evaluation.end())
    // {
    //     standing_pat = it->second;
    // }
    // else
    {
        SmartTime st{ m_evaluation_timer };
        standing_pat = color * evaluate_board(b);
        //m_evaluation[pos_string] = standing_pat;
    }
    if (standing_pat + 4000 < alpha) {
        return alpha;
    }

    m_total_quiescence_nodes += 1;

    if (standing_pat >= beta) {
        return beta;
    }
    if (alpha < standing_pat) {
        alpha = standing_pat;
    }

    MoveList moveList;
    {
        SmartTime st{ m_move_generation2_timer };
        moveList = enumerate_moves(b, true);
    }
 /*   MoveList pv;
    KillerMoves kl;
    reorder_moves(b, moveList, 1000, 1000, pv, kl);*/
    reorder_mvv_lva(b, moveList);

    int num_legal_move = 0;
    for (auto& move : moveList) {
        {
            SmartTime st{ m_make_move2_timer };
            b.make_move(move);
        }
        if (b.is_king_checked(move.color))
        {
            {
                SmartTime st{ m_unmake_move2_timer };
                b.unmake_move(move);
            }
            move.legal_checked = true;
            move.legal = false;
            continue;
        }

        ++num_legal_move;
        move.legal_checked = true;
        move.legal = true;

        int32_t BIG_DELTA = 975;
        if (move.promote) {
            BIG_DELTA += 775;
        }
        int32_t pval = piece_value(move.taken_piece);
        if (pval + BIG_DELTA < alpha) {
            {
                SmartTime st{ m_unmake_move2_timer };
                b.unmake_move(move);
            }
            return alpha;
        }
        int32_t val = -quiesce(b, -color, -beta, -alpha, current_depth+1);
        {
            SmartTime st{ m_unmake_move2_timer };
            b.unmake_move(move);
        }

        if (val >= beta) {
            return beta;
        }
        if (val > alpha) {
            alpha = val;
        }
    }
    if (num_legal_move == 0) {
        Color clr = b.get_next_move();
        if (b.is_king_checked(clr)) {
            // deep trouble here (checkmate) , but the more moves to get there,
            // the less deep trouble because adversary could make a mistake ;)
            // (this is to select quickest forced mate)
            return -20000 + 5*current_depth;
        }
        else {
            // pat
            return 0;
        }
    }
    return alpha;
}

int32_t NegamaxEngine::negamax(
    Board& b,
    int max_depth, int remaining_depth, int current_depth,
    int color,
    int32_t alpha, int32_t beta,
    MoveList& parentPv,
    const MoveList& previousPv,
    MoveList* topLevelOrdering
) {
    if (m_required_stop) {
        return beta; // fail-high immediately
    }

    MoveList currentPv;
    Color clr = b.get_next_move();
    currentPv.reserve(remaining_depth+1);

    if (remaining_depth == 0) {
        auto& stats = m_stats[max_depth][current_depth];
        stats.num_leaf_nodes += 1;
        SmartTime st{ m_quiescence_timer };
        int32_t val = quiesce(b, color, alpha, beta, current_depth);
        return val ;
    }
    m_total_nodes += 1;

    MoveList moveList;
    if (topLevelOrdering !=  nullptr && topLevelOrdering->size() > 0) {
        moveList = *topLevelOrdering;
    } else {
        {
            SmartTime st{ m_move_generation_timer };
            moveList = enumerate_moves(b);
        }
        {
            SmartTime st{ m_move_ordering_timer };
            reorder_moves(b, moveList, current_depth, remaining_depth, previousPv, m_killers);
        }
    }
    uint32_t num_legal_move = 0;

    bool cutoff = false;
    bool raise_alpha = false;
    int num_move_visited = 0;
    auto& stats = m_stats[max_depth][current_depth];
    bool use_aspiration = false;

    for (auto& move : moveList) {
        if (move.legal_checked && !move.legal) {
            continue;
        }
        ++num_move_visited;
        {
            SmartTime st{ m_make_move_timer };
            b.make_move(move);
        }

        if (b.is_king_checked(clr)) {
            // illegal move
            move.legal_checked = true;
            move.legal = false;
            move.evaluation = std::numeric_limits<int32_t>::min();
            {
                SmartTime st{ m_unmake_move_timer };
                b.unmake_move(move);
            }
            continue;
        }
        move.legal_checked = true;
        move.legal = true;
        ++num_legal_move;
        int32_t val = 0;
        if (use_aspiration && remaining_depth >= 2) {
            val = -negamax(
                b, max_depth, remaining_depth - 1, current_depth + 1,
                -color, -alpha-1, -alpha,
                currentPv, previousPv, nullptr
            );

            if (val > alpha && val < beta) {
                int32_t lower = -alpha - 1;
                int32_t initial_window_size = beta - alpha;

                int k = 0;
                while (val > alpha && val < beta && lower > -beta && k <= 3) {
                    // while search window failed high we increment window
                    double coeff = 1.0 / (8 >> k);
                    if (k < 3) {
                        lower = std::min(-alpha - (int32_t)(coeff * initial_window_size), lower-1);
                    }
                    else {
                        lower = -beta;
                    }
                    // 1/8 , 1/4, 1/2, and eventually 1/1
                    val = -negamax(
                        b, max_depth, remaining_depth - 1, current_depth + 1,
                        -color, lower, -alpha,
                        currentPv, previousPv, nullptr
                    );
                    ++k;
                }
            }

        }
        else {
            val = -negamax(
                b, max_depth, remaining_depth - 1, current_depth + 1,
                -color, -beta, -alpha,
                currentPv, previousPv, nullptr
            );
        }
        {
            SmartTime st{ m_unmake_move_timer };
            b.unmake_move(move);
        }
        move.evaluation = val;
        if (val >= beta) {
            alpha = beta; // cut node ! yay !
            if (m_required_stop) {
                return beta;
            }
            cutoff = true;
            if (move.best_from_pv) {
                stats.num_cut_by_best_pv += 1;
            }
            else if (move.killer) {
                stats.num_cut_by_killer += 1;
            }
            if (!move.takes && !move.killer) {
                move.killer = true;
                move.mate_killer = val >= 20000 - (max_depth+1) * 5;
                bool already_in = false;
                auto &killers = m_killers[current_depth];
                //replace killer
                for (const auto &m : killers) {
                    if (m == move) {
                        already_in = true;
                        break;
                    }
                }

                if (!already_in) {
                    killers.push_back(move);
                    if (killers.size() > 10) {
                        // pop front
                        killers.erase(killers.begin());
                    }
                }
            } else {

            }
            // TODO: store refutation move in TT
            break;
        }
        if (val > alpha) {
            alpha = val; // pv node
            // collect PV :
            if (parentPv.size() == 0) {
                parentPv.push_back(move);
            }
            else {
                parentPv[0] = move;
            }
            parentPv.resize(1);
            parentPv.insert(parentPv.begin() + 1, currentPv.begin(), currentPv.end());
            raise_alpha = true;
            // std::cerr << "alpha increase\n";
            use_aspiration = true;
        }
    }

    stats.num_move_visited += num_move_visited;
    stats.num_move_skipped += moveList.size() - num_move_visited;
    stats.num_move_generated += moveList.size();
    stats.num_nodes += 1;

    if (cutoff) {
        stats.num_cutoffs += 1;

        // cut-node (fail-high)
        // "A fail-high indicates that the search found something that was "too good".
        // What this means is that the opponent has some way, already found by the search,
        // of avoiding this position, so you have to assume that they'll do this.
        // If they can avoid this position, there is no longer any need to search successors,
        // since this position won't happen."
    } else if (raise_alpha) {
        stats.num_pvnode += 1;

        // pv-node (new best move)
    } else {
        stats.num_faillow_node += 1;
        // all-node (fail-low)
        // "A fail-low indicates that this position was not good enough for us.
        // We will not reach this position,
        // because we have some other means of reaching a position that is better.
        // We will not make the move that allowed the opponent to put us in this position."
    }

    if (num_legal_move == 0) {
        if (b.is_king_checked(clr)) {
            // deep trouble here (checkmate) , but the more moves to get there,
            // the less deep trouble because adversary could make a mistake ;)
            // (this is to select quickest forced mate)
            return -20000 + 5*current_depth;
        }
        else {
            // pat
            return 0;
        }
    }
    if (current_depth == 0) {
        std::sort(moveList.begin(), moveList.end(), [](auto& a, auto& b) {
            return a.evaluation > b.evaluation;
        });
        *topLevelOrdering = moveList;
        //for (auto &m : moveList) {
        //    if (!m.legal) {
        //        continue;
        //    }
        //    std::cerr << move_to_string(m) << " "<<m.evaluation<<"\n";
        //}
    }
    return alpha;
}

void NegamaxEngine::iterative_deepening(
    Board& b, int max_depth, Move *bestMove, bool *moveFound)
{
    m_running = true;
    *moveFound = false;

    int color = b.get_next_move() == C_WHITE ? +1 : -1;
    MoveList previousPv;
    MoveList topLevelOrdering;

    this->set_max_depth(max_depth);

    for (int depth = 1; depth <= max_depth; ++depth) {
         MoveList newPv;
         newPv.reserve(depth);
         Timer t;
         t.reset();
         t.start();

         reset_timers();
         m_total_nodes = 0;
         m_total_quiescence_nodes = 0;

         int32_t score = this->negamax(
             b, depth, depth, 0, color,
             -999999, // alpha
             +999999, // beta
             newPv, previousPv, &topLevelOrdering
         );
         //std::cerr << "\n\n-----------------\n";
         if (m_required_stop) {
             m_running = false;
             m_required_stop = false;
             return;
         }
         if (newPv.size() > 0) {
             //std::cerr << "Best move for depth="
             //    << depth << "  = " << move_to_string(newPv[0])
             //    << "\n";
         }
         else {
             *moveFound = false;
             return;
         }

         t.stop();

         std::vector<std::string> moves_str;
         // moves_str.reserve(newPv.size());
         // for (const auto& m : newPv) {
         //     moves_str.emplace_back(move_to_string(m));
         // }
         // std::cout << fmt::format("info string PV = {}\n", fmt::join(moves_str, " "));


         if (depth >= 4) {

             moves_str.clear();
             moves_str.reserve(newPv.size());
             for (const auto& m : newPv) {
                 moves_str.emplace_back(move_to_uci_string(m));
             }
             uint64_t total_nodes = m_total_nodes + m_total_quiescence_nodes;
             double duration = std::max(t.get_length(), 0.001); // cap at 1ms
             uint64_t nps = (uint64_t)(total_nodes / duration);
             uint64_t time = t.get_micro_length() / 1000;

             std::cout << fmt::format(
                 "info depth {} score cp {} nodes {} nps {} pv {} time {}\n",
                 depth, score, total_nodes, nps, fmt::join(moves_str, " "), time
             );
         }

         *bestMove = newPv[0];
         *moveFound = true;

         previousPv = std::move(newPv);
         /*  this->display_cutoffs(depth);*/

         //std::cerr << "duration="<<t.get_length()<<"\n";
         //display_timers();
         //std::cerr << "total_nodes="<<m_total_nodes<<"\n";
         //std::cerr << "total_quiescence_nodes="<<m_total_quiescence_nodes<<"\n";
         //std::cerr << "\n-----------------\n\n";

     }
}

std::pair<bool,Move> find_best_move(Board& b)
{
    Move move;
    bool found;
    NegamaxEngine engine;

    engine.iterative_deepening(b, 7, &move, &found);
    if (!found) {
        std::cerr << "WARNING no move found !!" << std::endl;
    }
    return std::make_pair(found,move);
}
