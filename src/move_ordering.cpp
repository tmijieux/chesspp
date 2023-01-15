#include <iostream>
#include <unordered_map>
#include <algorithm>

#include "./move_generation.hpp"
#include "./move_ordering.hpp"
#include "./evaluation.hpp"
#include "./engine.hpp"


void reorder_mvv_lva(
    MoveList& moveList,
    size_t begin, size_t end)
{
    for (auto& m : moveList)
    {
        m.mvv_lva_value = piece_value(m.taken_piece) * 100 - piece_value(m.piece);
    }
    std::sort(
        moveList.begin() + begin,
        moveList.begin() + end,
        // moveList.begin(),
        // moveList.end(),
        [](const auto& a, const auto& b) {
            return a.mvv_lva_value > b.mvv_lva_value;
        });
}

void reorder_see(Board &b, MoveList &moveList, size_t begin, size_t end)
{
    for (auto i = begin; i < end; ++i) {
       auto& m = moveList[i];
       if (m.takes) {
           m.see_value = see_capture(b, m);
           m.mvv_lva_value = piece_value(m.taken_piece) * 100 - piece_value(m.piece);
       }
       else {
           m.see_value = 0;
       }
    }

    std::sort(
        moveList.begin() + begin,
        moveList.begin() + end,
        [](const auto& a, const auto& b) {
            if (a.see_value == b.see_value) {
                return a.mvv_lva_value > b.mvv_lva_value;
            }
            return a.see_value > b.see_value;
        });
}

void reorder_moves(
    NegamaxEngine &engine,
    Board &b, MoveList &moveList, int ply, int remaining_depth,
    KillerMoves &killers, bool has_hash_move, const Move &hash_move,
    const HistoryMoves &history)
{
    size_t offset = 0;

    auto size = moveList.size();

    // if (!has_hash_move && remaining_depth > 4) {
    //     // INTERNAL ITERATIVE DEEPENING
    //     Color clr = b.get_next_move();
    //     int color = clr == C_WHITE ? +1 : -1;

    //     //NegamaxEngine engine;
    //     //engine.set_max_depth(2);
    //     NodeType children = NodeType::PV_NODE;
    //     MoveList pvLine;
    //     for (auto &move : moveList) {
    //         int max_depth = std::max((remaining_depth-1)/3, 2);
    //         b.make_move(move);
    //         if (b.is_king_checked(clr)) {
    //             move.legal_checked = true;
    //             move.legal = false;
    //             b.unmake_move(move);
    //             move.evaluation = -999999;
    //             continue;
    //         }
    //         for (int depth = 1; depth <= max_depth; ++depth) {
    //             move.evaluation = - engine.negamax(
    //                 b, depth, depth, 0, color,
    //                 -999999, // alpha
    //                 +999999, // beta
    //                 true,
    //                 children,
    //                 pvLine
    //             );
    //         }
    //         b.unmake_move(move);
    //     }
    //     std::sort(
    //         moveList.begin(),
    //         moveList.end(),
    //         [](const auto &m1, const auto &m2){
    //             return m1.evaluation > m2.evaluation;
    //         });
    //     return;
    // }
    if (ply < killers.size() && killers[ply].size() > 0) {
        auto& mykillers = killers[ply];
        for (const auto& killer : mykillers) {
            for (size_t i = 0; i < size; ++i) {
                if (moveList[i] == killer) {
                    moveList[i].killer = true;
                    if (killer.mate_killer) {
                        moveList[i].mate_killer = true;
                    }
                    break;
                }
            }
        }
    }

    if (ply >= 2 && ply < killers.size() && killers[ply-2].size() > 0) {
        Move& killer = killers[ply-2][0];
        for (size_t i = 0; i < size; ++i) {
            if (moveList[i] == killer) {
                moveList[i].killer = true;
                if (killer.mate_killer) {
                    moveList[i].mate_killer = true;
                }
                break;
            }
        }
    }

    for (auto i = offset; i < size; ++i) {
        auto& m = moveList[i];
        m.mvv_lva_value = piece_value(m.taken_piece) * 100 - piece_value(m.piece);
        if (m.takes) {
            m.see_value = see_capture(b, m);
        }
        else {
            m.see_value = 0;
        }
    }

    std::sort(
        moveList.begin(), moveList.end(),
        [&history](auto& a, auto& b) {
            if (a.see_value > b.see_value) {
                // good captures first
                return true;
            }
            if (a.see_value < b.see_value) {
                // bad captures lasts
                return false;
            }
            if (a.killer && !b.killer) {
                // killer after good captures
                return true;
            }
            if (!a.killer && b.killer) {
                // killer after good captures
                return false;
            }
            if (a.mate_killer && !b.mate_killer) {
                // mate killer before killers
                return true;
            }
            if (!a.mate_killer && b.mate_killer) {
                // mate killer before killers
                return false;
            }
            // rest is ordered by history heuristic
            auto idx1 = a.color*64*64+a.src.to_val()*64+a.dst.to_val();
            auto idx2 = b.color*64*64+b.src.to_val()*64+b.dst.to_val();
            auto h1 = history[idx1];
            auto h2 = history[idx2];
            if (h1 == h2) {
                return a.mvv_lva_value > b.mvv_lva_value;
            }
            return h1 > h2;
        }
    );
}

bool get_smallest_attacker(Board& b, const Pos &dst, Move& move)
{
    MoveList moveList;
    int32_t value = 0;
    Color clr = b.get_next_move();
    find_move_to_position(b, dst, moveList, clr , -1, true);
    if (moveList.size() > 0)
    {
        int32_t min_piece_value = 999999;
        bool found = false;
        for (auto& candidate : moveList) {
            // // check legality here ?
            // b.make_move(candidate);
            // candidate.legal_checked = true;
            // candidate.legal = !b.is_king_checked(clr);
            // b.unmake_move(candidate);
            // if (!candidate.legal) { continue; }
            int32_t value = piece_value(candidate.piece);
            if (value < min_piece_value) {
                min_piece_value = value;
                move = candidate;
                found = true;
            }
        }
        return found;
    }
    return false;
}

int32_t compute_see(Board &b, const Pos& dst)
{
    Piece p = b.get_piece_at(dst);
    if (p == P_EMPTY)
    {
        return 0;
    }
    int32_t value = 0;

    // find enemy move
    Move move;
    bool found = get_smallest_attacker(b, dst, move);
    if (found) {
        int32_t just_captured = piece_value(move.taken_piece);
        b.make_move(move);
        value = std::max(0, just_captured - compute_see(b, dst));
        b.unmake_move(move);
    } // if not found means there is no takes to this square
    return value;
}

/**
 * SEE: Static Exchange Evaluation
 */
int32_t see_capture(Board &b, const Move &capture)
{
    int32_t value = 0;
    b.make_move(capture);
    value = piece_value(capture.taken_piece) - compute_see(b, capture.dst);
    b.unmake_move(capture);
    return value;
}
