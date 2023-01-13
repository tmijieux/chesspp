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
    std::sort(
        moveList.begin() + begin,
        moveList.begin() + end,
        // moveList.begin(),
        // moveList.end(),
        [](const auto& a, const auto& b) {
            auto ta = piece_value(a.taken_piece);
            auto tb = piece_value(b.taken_piece);
            if (ta != tb) {
                return ta > tb;
            }
            auto pa = piece_value(a.piece);
            auto pb = piece_value(b.piece);
            return pa < pb;
        });
}

void reorder_see(Board &b, MoveList &moveList, size_t begin, size_t end)
{
    for (auto i = begin; i < end; ++i) {
       auto& m = moveList[i];
       if (m.takes) {
           m.see_value = compute_see(b, m);
       }
       else {
           m.see_value = 0;
       }
    }

    std::sort(
        moveList.begin() + begin,
        moveList.begin() + end,
        [](const auto& a, const auto& b) {
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
        if (m.takes) {
            m.see_value = compute_see(b, m);
            //m.see_value = piece_value(m.taken_piece)-piece_value(m.piece);
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
            return history[idx1] > history[idx2];
        }
    );
}



/**
 * SEE: Static Exchange Evaluation
 */
int32_t compute_see(Board &b, const Move &m)
{
    if (m.taken_piece == P_EMPTY) {
        std::cout << "invalid see(1)\n";
    }
    int32_t val = piece_value(m.taken_piece);
    const Pos &dst = m.dst;
    b.make_move(m);

    // find enemy move
    MoveList moveList;
    find_move_to_position(b, dst, moveList, b.get_next_move(), -1, true);
    if (moveList.size() > 0)
    {
        Move smallest;
        int32_t min_piece_value = 9999999;
        bool found = false;
        for (const auto &MM : moveList) {
            int32_t value = piece_value(MM.piece);
            if (value < min_piece_value) {
                min_piece_value = value;
                smallest = MM;
                found = true;
            }
        }
        if (found) {
            val -= compute_see(b, smallest);
        } // if not found means there is no takes to this square
    }
    b.unmake_move(m);
    return val;
}
