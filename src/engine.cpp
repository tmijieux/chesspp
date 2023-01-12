#include <iostream>
#include <functional>
#include <cstdint>

#include <fmt/format.h>

#include "./move_ordering.hpp"
#include "./engine.hpp"
#include "./move_generation.hpp"
#include "./move.hpp"
#include "./timer.hpp"
#include "./evaluation.hpp"
#include "./uci.hpp"


void extract_pv(const Move& m, const MoveList& currentPvLine, MoveList& parentPvLine)
{
    parentPvLine.resize(currentPvLine.size() + 1);

    parentPvLine[0] = m;
    auto size = currentPvLine.size();
    for (size_t i = 0; i < size; ++i) {
        parentPvLine[i + 1] = currentPvLine[i];
    }
}


int32_t NegamaxEngine::quiesce(
    Board& b, int color, int32_t alpha, int32_t beta, uint32_t ply)
{
    int32_t standing_pat = 0;
    {
        // SmartTime st{ m_evaluation_timer };
        standing_pat = color * evaluate_board(b);
        //m_evaluation[pos_string] = standing_pat;
    }
    if (m_stop_required) {
        return std::max(standing_pat, beta); // fail-high immediately
        // return beta
    }
    if (standing_pat + 4000 < alpha) {
        return standing_pat;
        //return alpha;
    }

    m_quiescence_nodes += 1;
    if (standing_pat >= beta) {
        return standing_pat;
        //return beta;
    }
    if (standing_pat > alpha) {
        alpha = standing_pat;
    }

    MoveList moveList;
    {
        // SmartTime st{ m_move_generation2_timer };
        moveList = generate_pseudo_moves(b, true);
    }

    {
        // SmartTime st{ m_move_ordering_mvv_lva_timer };
        reorder_mvv_lva(moveList, 0, moveList.size());
    }

    int32_t nodeval = standing_pat;
    for (auto& move : moveList) {
        {
            // SmartTime st{ m_make_move2_timer };
            b.make_move(move);
        }
        if (b.is_king_checked(move.color))
        {
            {
                // SmartTime st{ m_unmake_move2_timer };
                b.unmake_move(move);
            }
            move.legal_checked = true;
            move.legal = false;
            continue;
        }

        move.legal_checked = true;
        move.legal = true;

        int32_t BIG_DELTA = 975;
        if (move.promote) {
            BIG_DELTA += 775;
        }
        int32_t pval = piece_value(move.taken_piece);
        if (pval + BIG_DELTA < alpha) {
            {
                // SmartTime st{ m_unmake_move2_timer };
                b.unmake_move(move);
            }
            //return alpha;
            return standing_pat;
        }
        int32_t val = -quiesce(b, -color, -beta, -alpha, ply+1);
        {
            // SmartTime st{ m_unmake_move2_timer };
             b.unmake_move(move);
        }

        if (val >= beta) {
            //return beta;
            return val;
        }
        if (val > nodeval) {
            nodeval = val;
        }
        if (val > alpha) {
            alpha = val;
        }
    }
    return nodeval;
}


int32_t NegamaxEngine::negamax(
    Board& b,
    int max_depth, int remaining_depth, uint32_t ply,
    int color,
    int32_t alpha, int32_t beta,
    bool internal,
    NodeType &node_type,
    MoveList &parentPvLine
) {
    if (m_stop_required) {
        return beta; // fail-high immediately
    }

    bool null_window = beta == alpha+1;
    bool found_best_move = false;
    bool has_hash_move = false;
    Move best_move;
    Move hash_move;

    Color clr = b.get_next_move();

    MoveList currentPvLine;
    auto &stats = m_stats[max_depth][ply];

    // check for 50moves
    if (b.get_half_move() >= 100) {
        return 0;
    }

    // checks for repetition
    uint64_t bkey = b.get_key();

    auto psize = m_positions_sequence.size();
    if (psize < ply + 1) {
        m_positions_sequence.resize(ply + 1);
    }
    m_positions_sequence[ply] = bkey;
    for (size_t i = 0; i < ply; ++i) {
        if (m_positions_sequence[i] == bkey) {
            // repetition
            //std::cout  <<"repetition!!\n";
            return 0; // 0 for draw
        }
    }

    auto &hashentry = m_hash.get(bkey);

    if (hashentry.key == bkey) {
        has_hash_move = hashentry.hashmove_src != hashentry.hashmove_dst;
        if (has_hash_move)
        {
            hash_move = generate_move_for_squares(
                b, hashentry.hashmove_src,
                hashentry.hashmove_dst,
                hashentry.promote_piece
            );
        }

        if (hashentry.depth >= remaining_depth) {
            if (hashentry.exact_score) {
                //std::cout << "hash hit!\n";
                stats.num_hash_hits++;


                if (hashentry.score >= beta) {
                    return hashentry.score;
                    //return beta;
                }
                else if (hashentry.score <= alpha) {
                    //return alpha;
                    return hashentry.score;
                }
                else {

                    if (has_hash_move) {
                        currentPvLine.clear();
                        extract_pv_from_tt(b, currentPvLine, hashentry.depth);
                        parentPvLine = currentPvLine;
                        // if (currentPvLine.size() > 0){
                        //     currentPvLine.erase(currentPvLine.begin());
                        // }
                        // extract_pv(hash_move, currentPvLine, parentPvLine);
                    }
//                    currentPvLine.clear();
//                    extract_pv_from_tt(b, currentPvLine, hashentry.depth);
//                    extract_pv(hash_move, currentPvLine, parentPvLine);
                    return hashentry.score;
                }
            }
            else if (hashentry.lower_bound) {
                if (hashentry.score >= beta) {
                    //std::cout << "hash hit!\n";
                    stats.num_hash_hits++;

                    return hashentry.score;
                    // return beta
                }
                else if (hashentry.score >= alpha) {
                    //alpha = hashentry.score;
                }
            }
            else if (hashentry.upper_bound) {
                if (hashentry.score <= alpha) {
                    stats.num_hash_hits++;

                    //std::cout << "hash hit!\n";
                    return hashentry.score;
                    //return alpha;
                } else if (hashentry.score <= beta) {

                }
            }
        }
    } else if (hashentry.key != 0 && hashentry.key != bkey) {
        stats.num_hash_conflicts++;
    }


    if (remaining_depth <= 0 && !b.is_king_checked(clr) ) {
        // TODO ... && !b.is_king_checked(clr)
        // we do not want to go into quiescence if is in check
        stats.num_leaf_nodes += 1;
        m_leaf_nodes += 1;
        // SmartTime st{ m_quiescence_timer };
        int32_t nodeval = quiesce(b, color, alpha, beta, ply);
        return nodeval;
    }
    // if (remaining_depth <= 0)
    // {
    //     std::cout << "check extension!\n";
    // }


    m_regular_nodes += 1;

    uint32_t num_legal_move = 0;
    bool cutoff = false;
    bool raise_alpha = false;
    bool use_aspiration = false;
    int num_move_maked = 0;
    int32_t nodeval = -999999;
    int32_t val  = 0;

    if (has_hash_move) {
        NodeType children = NodeType::UNDEFINED;
        b.make_move(hash_move);
        hash_move.checks = b.is_king_checked(other_color(clr));
        val = -negamax(
            b, max_depth, remaining_depth - 1, ply + 1,
            -color, -beta, -alpha,
            internal, children, currentPvLine
        );
        hash_move.mate = (!!hash_move.checks) && children == NO_MOVE;

        if (val > nodeval) {
            nodeval = val;
            best_move = hash_move;
            found_best_move = true;
        }
        if (val > alpha) {
            alpha = val; // pv-node
            raise_alpha = true;
            use_aspiration = true;
            extract_pv(hash_move, currentPvLine, parentPvLine);
        }
        if (val >= beta) {
            // cut node ! yay !
            cutoff = true;
            stats.num_cut_by_hash_move += 1;
        }
        b.unmake_move(hash_move);
        ++num_legal_move;
        ++num_move_maked;
    }


    MoveList moveList;
    if (!cutoff)
    {
        {
            // SmartTime st{ m_move_generation_timer };
            moveList = generate_pseudo_moves(b);
        }

        {
            // SmartTime st{ m_move_ordering_timer };
            reorder_moves(*this, b, moveList, ply, remaining_depth,
                          m_killers, has_hash_move, hash_move, m_history);
        }

        for (auto &move : moveList) {
            NodeType children = NodeType::UNDEFINED;
            if (has_hash_move && move == hash_move) {
                continue;
            }
            if (move.legal_checked && !move.legal) {
                continue;
            }
            {
                // SmartTime st{ m_make_move_timer };
                b.make_move(move);
                ++num_move_maked;
            }

            if (b.is_king_checked(clr)) {
                // illegal move
                move.legal_checked = true;
                move.legal = false;
                move.evaluation = std::numeric_limits<int32_t>::min();
                {
                    // SmartTime st{ m_unmake_move_timer };
                    b.unmake_move(move);
                }
                continue;
            }
            move.checks = b.is_king_checked(other_color(clr));
            move.legal_checked = true;
            move.legal = true;
            ++num_legal_move;
            int32_t val = 0;
            int32_t r = 0;
            if (!move.checks && move.see_value <= 0 && !move.killer && num_legal_move > 5) {
                auto MLsize = moveList.size();
                r = 1;
                if (MLsize >= 30 && num_legal_move >= MLsize - 10) {
                    r = 2;
                    if (MLsize >= 40
                        && move.see_value < 0
                        && num_legal_move >= MLsize - 5) {
                        r = 3;
                    }

                }
            }

            if (use_aspiration && remaining_depth >= 2) {
                stats.num_aspiration_tries += 1;
                val = -negamax(
                    b, max_depth, remaining_depth - 1 - r, ply + 1,
                    -color, -alpha-1, -alpha,
                    internal, children, currentPvLine
                );
                move.mate = (!!move.checks) && children == NO_MOVE;

                if (val > alpha && val < beta) {
                    int32_t lower = -alpha - 1;
                    int32_t initial_window_size = beta - alpha;

                    int k = 0;
                    while (val > alpha && val < beta && lower > -beta && k <= 3) {
                        // while search window failed high we increment window
                        stats.num_aspiration_failures += 1;
                        double coeff = 1.0 / (8 >> k);
                        if (k < 3) {
                            stats.num_aspiration_tries += 1;
                            lower = std::min(-alpha - (int32_t)(coeff * initial_window_size), lower-1);
                        }
                        else {
                            lower = -beta;
                        }
                        // 1/8 , 1/4, 1/2, and eventually 1/1
                        val = -negamax(
                            b, max_depth, remaining_depth - 1 - r, ply + 1,
                            -color, lower, -alpha,
                            internal, children, currentPvLine
                        );
                        ++k;
                    }
                }
            }
            else {
                val = -negamax(
                    b, max_depth, remaining_depth - 1 - r, ply + 1,
                    -color, -beta, -alpha,
                    internal, children, currentPvLine
                );
                move.mate = (!!move.checks) && children == NO_MOVE;
            }
            move.evaluation = val;

            {
                // SmartTime st{ m_unmake_move_timer };
                b.unmake_move(move);
            }
            if (m_stop_required) {
                // INSTA-FAIL-HIGH
                return std::max(nodeval, beta);
                // return beta
            }

            if (val > nodeval) {
                nodeval = val;
                best_move = move;
                found_best_move = true;
            }
            if (val >= beta) {
                // cut node ! yay !
                cutoff = true;
                if (move.killer) {
                    stats.num_cut_by_killer += 1;
                }

                if (!move.takes) {
                    // update killers
                    move.killer = true;
                    move.mate_killer = val >= 20000 - (max_depth+1);
                    bool already_in = false;
                    if (m_killers.size() < ply+1) {
                        m_killers.resize(ply+1);
                    }
                    auto &killers = m_killers[ply];
                    //replace killer
                    for (auto &m : killers) {
                        if (m == move) {
                            already_in = true;
                            m.killer_freq += ply*ply;
                            break;
                        }
                    }

                    if (!already_in) {
                        move.killer_freq = ply * ply;
                        killers.push_back(move);
                        std::sort(
                            killers.begin(), killers.end(),
                            [](const auto&a, const auto&b) {
                                return a.killer_freq > b.killer_freq;
                            });
                        while (killers.size() > 10) {
                            // pop back
                            killers.erase(killers.end()-1);
                        }
                    }

                    // update history
                    auto clr = move.color;
                    size_t idx = clr*64*64 + move.src.to_val()*64 + move.dst.to_val();
                    m_history[idx] += ply*ply;
                }
                break;
            }
            if (val > alpha) {
                alpha = val; // pv-node
                raise_alpha = true;
                use_aspiration = true;
                extract_pv(move, currentPvLine, parentPvLine);
            }
        }
    }
    stats.num_move_skipped += (uint32_t)std::max(moveList.size(),1_u64) - num_move_maked;
    stats.num_move_generated += (uint32_t)moveList.size();
    stats.num_move_maked += num_move_maked;
    stats.num_nodes += 1;


    if (num_legal_move == 0) {
        node_type = NO_MOVE;
        if (b.is_king_checked(clr)) {
            // deep trouble here (checkmate) , but the more moves to get there,
            // the less deep trouble because adversary could make a mistake ;)
            // ( is to select quickest forced mate)
            nodeval = -20000 + ply;
        }
        else {
            // pat
            nodeval = 0;
        }
    }

    bool replace = hashentry.key == 0 // FIXME must not replace pv
        || remaining_depth > hashentry.depth
        || (!hashentry.exact_score && !cutoff && raise_alpha);
    // if depth is lower but we have an pv-node
    // while the previous was not a pv-node then replace!
    if (null_window) {
        // if in a null windows
        // we replace if is empty or if the entry is that of a null window
        // and we would otherwise replace
        replace = (hashentry.is_null_window && replace) || hashentry.key == 0;
    } else if (hashentry.is_null_window) {
        replace = true;
    }
    if (replace) {
        hashentry.score = nodeval;
        hashentry.depth = remaining_depth;
        hashentry.key = bkey;
        hashentry.is_null_window = null_window;
        if (found_best_move) {
            hashentry.hashmove_src = best_move.src.to_val();
            hashentry.hashmove_dst = best_move.dst.to_val();
            hashentry.promote_piece = best_move.promote_piece;
        }
        else {
            hashentry.hashmove_src = 0;
            hashentry.hashmove_dst = 0;
        }
    }

    if (cutoff) {
        // cut-node (fail-high)
        // "A fail-high indicates that the search found something that was "too good".
        // What this means is that the opponent has some way, already found by the search,
        // of avoiding this position, so you have to assume that they'll do this.
        // If they can avoid this position, there is no longer any need to search successors,
        // since this position won't happen."
        stats.num_cutoffs += 1;
        node_type = CUT_NODE;
        if (replace){
            hashentry.exact_score = false;
            hashentry.lower_bound = true;
            hashentry.upper_bound = false;
        }
    } else if (raise_alpha) {
        // pv-node (new best move)
        stats.num_pvnode += 1;
        node_type = PV_NODE;
        if (replace){
            hashentry.exact_score = true;
            hashentry.lower_bound = false;
            hashentry.upper_bound = false;
        }
    } else {
        // all-node (fail-low)
        // "A fail-low indicates that this position was not good enough for us.
        // We will not reach this position,
        // because we have some other means of reaching a position that is better.
        // We will not make the move that allowed the opponent to put us in this position."
        stats.num_faillow_node += 1;
        if (node_type != NO_MOVE) {
            node_type = ALL_NODE;
        }
        if (replace){
            hashentry.exact_score = false;
            hashentry.lower_bound = false;
            hashentry.upper_bound = true;
        }
    }

    // if (ply == 0) {
    //     std::sort(moveList.begin(), moveList.end(), [](auto& a, auto& b) {
    //         return a.evaluation > b.evaluation;
    //     });
    //     *topLevelOrdering = moveList;
        // full sort for next level of iterative deepening

        // if (!internal) {
        //     for (auto &m : moveList) {
        //         if (!m.legal) {
        //             continue;
        //         }
        //         std::cout << move_to_string(m) << " "<< m.evaluation<< " \n";
        //     }
        // }
    // }
    return nodeval;
}

void NegamaxEngine::_start_uci_background(Board &b)
{
    bool move_found = false;
    Move best_move;
    GoParams p = m_uci_go_params;
    if (p.depth == 0) {
        p.depth = 200;
    }
    uint64_t time = 0;
    if (p.movetime > 0) {
        time = p.movetime;
    }
    else if (p.wtime > 0 || p.btime > 0) {
        uint64_t basetime = b.get_next_move() == C_WHITE ? p.wtime : p.btime;
        if (p.movestogo > 0) {
            time = basetime / p.movestogo;
        }
        else {
            int move_count_wanted = std::max(60u - b.get_full_move(), 10u);
            time = basetime / move_count_wanted;
        }
        // remove 200ms to have time finishing and returning move
        time = std::min(time, std::max(15_u64, basetime - 200_u64));
    }
    auto id = ++m_run_id;

    if (time > 0) {
        std::cout << "sleeping for interrupt!!\nms=" << time << "\n";
         // schedule thread to interrupt
        std::thread([this,time, id]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(time));
            if (this->is_running() && this->m_run_id == id)
            {
                this->m_stop_required_by_timeout = true;
                this->m_stop_required = true;
            }
        })
            .detach();
    }

    m_running = true;
    bool interrupted = iterative_deepening(
        b, p.depth, &best_move, &move_found, time
    );
    m_running = false;

    if (move_found) {
        uci_send_bestmove(best_move);
        move_found = false;
    }
    if (!interrupted || m_stop_required_by_timeout) {
        m_thread.detach();
    }
    m_stop_required = false;
    m_stop_required_by_timeout = false;
}

void NegamaxEngine::start_uci_background(Board &b)
{
    if (m_running) {
        throw chess_exception("engine already running");
    }
    m_thread = std::thread{
        std::bind( &NegamaxEngine::_start_uci_background,this, b)
    };
}

void NegamaxEngine::stop()
{
    if (m_thread.joinable())
    {
        m_stop_required = true;
        m_thread.join();
    }
    m_stop_required = false;
    m_running = false;
}

void NegamaxEngine::extract_pv_from_tt(Board& b, MoveList& pv, int depth)
{
    // extract PV  from TT
    // std::cout << "extract PV from TT\n" << std::flush;

    pv.reserve(depth);
    auto current_depth = depth;
    while (current_depth > 0)
    {
        uint64_t bkey = b.get_key();
        auto& hashentry = m_hash.get(bkey);
        if (hashentry.key == 0) {
            //std::cerr << "did not found entry for pv move in TT == 0\n";
            break;
        }
        if (hashentry.key != bkey) {
            //std::cerr << "did not found entry for pv move in TT => KEY CONFLICT\n";
            break;
        }
        if (!hashentry.exact_score) {
            //std::cerr << "entry in TT is not PV-node\n";
            break;
        }
        bool has_hash_move = hashentry.hashmove_src != hashentry.hashmove_dst;
        if (!has_hash_move) {
            // std::cerr << "no hashmove in TT for PV-node\n";
            break;
        }
        Move hash_move = generate_move_for_squares(
            b,
            hashentry.hashmove_src,
            hashentry.hashmove_dst,
            hashentry.promote_piece
        );
        b.make_move(hash_move);
        hash_move.checks = b.is_king_checked(other_color(hash_move.color));
        hash_move.mate = (hashentry.exact_score!=0) && hashentry.score == 20000 - 1;
        pv.push_back(hash_move);
        --current_depth;
        if (hash_move.mate)
        {
            break;
        }
    }
    for (auto i = pv.size(); i > 0; --i) {
        b.unmake_move(pv[i - 1]);
    }
}


/**
 * return  true if search was interrupted by m_stop_required,
 * and false otherwise
 */
bool NegamaxEngine::iterative_deepening(
    Board& b, int max_depth,
    Move *best_move, bool *move_found,
    uint64_t max_time_ms)
{
    *move_found = false;

    int color = b.get_next_move() == C_WHITE ? +1 : -1;

    this->set_max_depth(max_depth);
    Timer total_timer;
    total_timer.start();
    MoveList previousPvLine;
    m_total_nodes_prev = 0;
    m_total_nodes_prev_prev = 0;

    for (int depth = 1; depth <= max_depth; ++depth) {
        Timer t;
        t.start();

        reset_timers();
        m_regular_nodes = 0;
        m_leaf_nodes = 0;
        m_quiescence_nodes = 0;
        NodeType node_type = NodeType::UNDEFINED;
        MoveList pvLine;

        int32_t score = this->negamax(
            b, depth, depth, 0, color,
            -999999, // alpha
            +999999, // beta
            false, node_type, pvLine
        );

        if (m_stop_required_by_timeout || max_time_ms > 0) {
            uint64_t total_duration = (uint64_t)(total_timer.get_micro_length() / 1000.0);
            if (total_duration > max_time_ms) {
                uci_send_info_string(
                    "EXIT ON TIME total_duration={} max_time_ms={} move_found={}",
                    total_duration, max_time_ms, *move_found
                );
                return false;
            }
        }
        if (m_stop_required) {
            return true;
        }
        t.stop();

        //extract_pv_from_tt(b, pvLine, depth);

        if (pvLine.size() == 0) {
            // only possibility is the position is already mated
            uci_send_info_string("no move was found! score={}", score);
            *move_found = false;
            return false;
        }
        // if (pvLine.size() < depth && !pvLine[pvLine.size()-1].mate)
        // {
        //     std::cout << fmt::format("MISSING {} MOVE IN PV!!!\n", depth - pvLine.size());
        // }
        *best_move = pvLine[0];
        *move_found = true;
    
   
        display_readable_pv(b, pvLine);

     
        uint64_t total_nodes = m_regular_nodes + m_quiescence_nodes;
        double duration = std::max(t.get_length(), 0.001); // cap at 1ms
        uint64_t nps = (uint64_t)(total_nodes / duration);
        uint64_t time = (uint64_t)(t.get_micro_length() / 1000);

        int32_t mate = 0;
        if (score <= -20000+2*depth+1 || score >= 20000-2*depth-1) {
            // need to divide by 2 to have number of move from number of plies
            if (score < 0) {
                mate = - 20000 - score;
                mate = (mate-1) / 2;
            } else {
                mate = 20000 - score;
                mate = (mate+1) / 2;
            }
        }

        std::vector<std::string> moves_str;
        moves_str.reserve(pvLine.size());
        for (const auto& m : pvLine) {
            moves_str.emplace_back(move_to_uci_string(m));
        }
        if (mate != 0) {
            uci_send(
                "info depth {} score mate {} nodes {} nps {} pv {} time {}\n",
                depth, mate, total_nodes, nps, fmt::join(moves_str, " "), time
            );
        } else {
            uci_send(
                "info depth {} score cp {} nodes {} nps {} pv {} time {}\n",
                depth, score, total_nodes, nps, fmt::join(moves_str, " "), time
            );
        }

        display_stats(depth);
        display_timers(t);
        display_node_infos(t);

        if (pvLine[pvLine.size() - 1].mate) {
            break;
            // stop searching here
            // if we found a forced mate
            // even if it is not the best
            // it is either won or lost at
            // this point
        }

        previousPvLine = std::move(pvLine);

    }
    return false;
}

std::pair<bool,Move> find_best_move(Board& b)
{
    Move move;
    bool found;
    NegamaxEngine engine;

    engine.iterative_deepening(b, 7, &move, &found, 0);
    if (!found) {
        std::cerr << "WARNING no move found !!" << std::endl;
    }
    return std::make_pair(found,move);
}

uint64_t NegamaxEngine::perft(
    Board &b,
    uint32_t max_depth, uint32_t remaining_depth,
    std::vector<uint64_t> &res, Hash<PerftHashEntry> &hash)
{
    if (remaining_depth == 0) {
        return 1;
    }
    auto ply = max_depth - remaining_depth;

    //auto key = b.get_key();
    //uint64_t full_key = HashMethods::full_hash(b);
    //if (key != full_key) {
    //    std::abort();
    //}
    //if (remaining_depth >= 1)
    //{
    //    auto& hashentry = hash.get(key);
    //    if (hashentry.key == key && hashentry.depth == remaining_depth)
    //    {
    //        res[ply] += hashentry.nummoves;
    //        return hashentry.value;
    //    }
    //}
    Color clr = b.get_next_move();

    MoveList ml;
    //if (b.is_king_checked(clr))
    //{
    //    ml = generate_check_evading_moves(b);
    //}
    //else
    //{
        ml = generate_pseudo_moves(b);
    //}


    uint64_t total = 0;
    int num_legal_move = 0;
    for (auto &move : ml)
    {
        //if (move.legal_checked && !move.legal) {
        //    continue;
        //}
        b.make_move(move);
        if (b.is_king_checked(clr))
        {
            b.unmake_move(move);
            continue;
        }
        ++num_legal_move;
        uint64_t val = perft(b, max_depth, remaining_depth-1, res, hash);

        total += val;
        b.unmake_move(move);
        if (max_depth == remaining_depth)
        {
            std::cout << move_to_uci_string(move) <<": "<<val<< " "<<move_to_string(move) << "\n";
        }
    }

    res[ply] += num_legal_move;
    //if (remaining_depth>=1 ) {
    //    auto& hashentry = hash.get(key);
    //    if (remaining_depth > hashentry.depth)
    //    {
    //        // save only position with most savings potential
    //        hashentry.key = key;
    //        hashentry.depth = remaining_depth;
    //        hashentry.value = total;
    //        hashentry.nummoves = num_legal_move;
    //    }

    //}
    return total;
}

void NegamaxEngine::set_max_depth(int maxdepth)
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

std::string human_readable(double value)
{
    if (value < 1000) {
        return fmt::format("{:g}", value);
    }
    else if (value < 1000000) {
        value /= 1000.0;
        return fmt::format("{:.3g}K", value);
    } else if (value < 1000000000) {
        value /= 1000000.0;
        return fmt::format("{:.3g}M", value);
    } else  {
        value /= 1000000000.0;
        return fmt::format("{:.3g}B", value);
    }
}

void NegamaxEngine::do_perft(Board &b, uint32_t depth)
{
    Hash<PerftHashEntry> hash;
    std::vector<uint64_t> res;
    res.resize(depth);
    std::fill(res.begin(), res.end(), 0);
    Timer t;
    t.start();
    uint64_t total = perft(b, depth, depth, res, hash);
    t.stop();
    double duration = t.get_length();
    std::cout << "total=" << total << "\n";
    if (duration != 0) {
        std::cout << fmt::format(
            "{} moves in {} seconds ({} moves per seconds)\n",
            human_readable(total), duration, human_readable(total / duration));
    }
    else {
        std::cout << fmt::format(
            "{} moves in ~0 seconds (time window to small for precise measurement)\n",
            total);
    }

    for (uint32_t i = 0; i < depth; ++i) {
        uci_send_info_string(
            "num_move for depth {} = {}",
            i, res[i]
        );
    }
}


// --------------------------------------------------------
// --- STATS AND REPORTS ----------------------------------

void NegamaxEngine::display_stats()
{
    for (auto& [maxdepth, stats_at_depth] : m_stats) {
        display_stats(maxdepth);
    }
}

void NegamaxEngine::display_stats(int current_maxdepth)
{
    std::cerr << "cutoffs for current_maxdepth=" << current_maxdepth << "\n";
    for (auto& [depth, stats] : m_stats[current_maxdepth]) {

        double percent_maked = (double)stats.num_move_maked / std::max(stats.num_move_generated, 1u);
        double percent_skipped = (double)stats.num_move_skipped / std::max(stats.num_move_generated, 1u);

        double percent_cutoff = (double)stats.num_cutoffs / std::max(stats.num_nodes, 1u);
        double percent_faillow = (double)stats.num_faillow_node / std::max(stats.num_nodes, 1u);

        std::cerr << "d=" << depth
            << "\n   NODES total=" << stats.num_nodes
            << " leaf=" << stats.num_leaf_nodes
            << " cutoffs=" << stats.num_cutoffs
            << " (" << (int)(percent_cutoff * 100.0) << "%) "

            << " pv=" << stats.num_pvnode
            << " faillow= " << stats.num_faillow_node
            << " (" << (int)(percent_faillow * 100.0) << "%) "
            << " cut_by_hash_move= " << stats.num_cut_by_hash_move
            << " cut_by_killer= " << stats.num_cut_by_killer

            << "\n   MOVES generated=" << stats.num_move_generated
            << " maked=" << stats.num_move_maked
            << " (" << (int)(percent_maked * 100.0) << "%) "
            << " skipped=" << stats.num_move_skipped
            << " (" << (int)(percent_skipped * 100.0) << "%) "
            << "\n   HASH hits=" << stats.num_hash_hits
            << " conflicts=" << stats.num_hash_conflicts
            << "\n   ASPIRATION TOTAL=" << stats.num_aspiration_tries
            << " failure=" << stats.num_aspiration_failures
            << " success=" << stats.num_aspiration_tries-stats.num_aspiration_failures
            << "\n\n";

    }
}

void NegamaxEngine::reset_timers()
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

void NegamaxEngine::display_timers(Timer &t)
{
    double dur = t.get_length();

    std::cerr << "T(total)="<<dur<<"\n";
    std::cerr << "---\n" << std::flush;
    std::cerr << "T(move ordering)=" << m_move_ordering_timer.get_length() << "\n";
    std::cerr << "T(move generation)=" << m_move_generation_timer.get_length() << "\n";
    std::cerr << "T(make move)=" << m_make_move_timer.get_length() << "\n";
    std::cerr << "T(unmake move)=" << m_unmake_move_timer.get_length() << "\n";
    std::cerr << "---\n";
    std::cerr << "T(quiescence)=" << m_quiescence_timer.get_length() << "\n";
    std::cerr << "---\n";
    std::cerr << "T(evaluation)=" << m_evaluation_timer.get_length() << "\n";
    std::cerr << "T(Q move generation)=" << m_move_generation2_timer.get_length() << "\n";
    std::cerr << "T(Q move ordering mvv lva)=" << m_move_ordering_mvv_lva_timer.get_length() << "\n";
    std::cerr << "T(Q make move)=" << m_make_move2_timer.get_length() << "\n";
    std::cerr << "T(Q unmake move)" << m_unmake_move2_timer.get_length() << "\n";
    std::cerr << "---\n" << std::flush;;
}

void NegamaxEngine::display_node_infos(Timer &t)
{
    std::cerr << "Leaf Nodes="<<m_leaf_nodes<<"\n";
    std::cerr << "Regular Nodes="<<m_regular_nodes<<"\n";
    std::cerr << "Quiescence Nodes="<<m_quiescence_nodes<<"\n";

    auto total = m_regular_nodes + m_quiescence_nodes;
    std::cerr << "TOTAL_NODES="<<total<<"\n";
    double dur = t.get_length();
    std::cerr << "NPS (TOTAL NODES)=" << (total/dur) << "\n";

    double br_fac_1 = (double)(m_leaf_nodes+m_regular_nodes-1)/m_regular_nodes;
    double br_fac_2 = (double)(m_quiescence_nodes+m_regular_nodes-1)/m_regular_nodes;
    std::cerr << "AVG Branching Factor((LEAF+REGULAR)/REGULAR)="<<br_fac_1<<"\n";
    std::cerr << "AVG Branching Factor((QUIESCENCE+REGULAR)/REGULAR)="<<br_fac_2<<"\n";

    if (m_total_nodes_prev != 0) {
        double ebf_n_1 = (double)total/m_total_nodes_prev;
        std::cerr << "EBF(N/N-1) = " << ebf_n_1 << "\n";
    }
    if (m_total_nodes_prev_prev != 0) {
        double ebf_n_2 = (double)total/m_total_nodes_prev_prev;
        std::cerr << "sqrt(EBF(N/N-2)) = " << std::sqrt(ebf_n_2) << "\n";
    }
    m_total_nodes_prev_prev = m_total_nodes_prev;
    m_total_nodes_prev = total;

    std::cerr << "\n-----------------\n\n";

}
void NegamaxEngine::display_readable_pv(Board &b, const MoveList &pvLine)
{
    std::vector<std::string> moves_str;
    moves_str.reserve(pvLine.size());
    for (const auto& m : pvLine) {
        moves_str.emplace_back(move_to_string_disambiguate(b, m));
        b.make_move(m);
    }
    for (auto i = pvLine.size(); i > 0; --i)
    {
        b.unmake_move(pvLine[i - 1]);
    }
    uci_send("info string PV = {}\n", fmt::join(moves_str, " "));

}
