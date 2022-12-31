#include <iostream>
#include <unordered_map>

#include "./move_generation.hpp"
#include "./move_ordering.hpp"
#include "./evaluation.hpp"


void reorder_mvv_lva(const Board& b, MoveList& moveList)
{
    //std::sort(moveList.begin(), moveList.end(), [](auto& a, auto& b) {
    //    // if (!a.takes || !b.takes) {
    //    //     throw std::exception("some move are not takes here");
    //    // }
    //    int aa = piece_value(a.taken_piece) - piece_value(a.piece);
    //    int bb = piece_value(b.taken_piece) - piece_value(b.piece);
    //    return aa > bb;
    //});

    std::sort(moveList.begin(), moveList.end(), [](auto& a, auto& b) {
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


void reorder_move_see(Board &b, MoveList &moveList, int begin, int end)
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

    std::sort(moveList.begin() + begin, moveList.begin()+end, [](auto& a, auto& b) {
       return a.see_value > b.see_value;
    });
}

void reorder_moves(
    Board &b,
    MoveList &moveList,
    int current_depth,
    int remaining_depth,
    const MoveList& previousPv,
    KillerMoves &killers)
{
    bool has_best_move = false;
    size_t offset = 0;
    if (current_depth < previousPv.size()) {
        const Move& bestMove = previousPv[current_depth];

        for (size_t i = offset+1; i < moveList.size(); ++i) {
            if (moveList[i] == bestMove ) {
                std::swap(moveList[offset], moveList[i]);
                has_best_move = true;
                moveList[offset].best_from_pv = true;
                ++offset;
                break;
            }
        }
    }
    size_t startOfKillers = offset;

    if (current_depth < killers.size() && killers[current_depth].size() > 0) {
        auto& mykillers = killers[current_depth];
        for (const auto& killer : mykillers) {
            for (size_t i = offset+1; i < moveList.size(); ++i) {
                if (moveList[i] == killer) {
                    std::swap(moveList[offset], moveList[i]);
                    moveList[offset].killer = true;
                    ++offset;
                    break;
                }
            }
        }
    }
    if (startOfKillers < offset) {
        std::sort(
            moveList.begin()+startOfKillers, moveList.begin()+offset,
            [](const auto& a, const auto& b) {
                if (a.mate_killer && !b.mate_killer) {
                    return true;
                } else if (b.mate_killer && !a.mate_killer) {
                    return false;
                } else {
                    return &a < &b;
                }
            }
        );
    }

    bool some_see = false;
    size_t startAt = offset;


    std::sort(
        moveList.begin()+startAt, moveList.end(),
        [](const auto& a, const auto& b) {
            auto ta = piece_value(a.taken_piece);
            auto tb = piece_value(b.taken_piece);
            if (ta != tb) {
                return ta > tb;
            }
            auto pa = piece_value(a.piece);
            auto pb = piece_value(b.piece);
            return pa < pb;
        }
    );

    //auto size = moveList.size();
    //for (auto i = offset; i < size; ++i) {
    //    auto& m = moveList[i];
    //    if (m.takes) {
    //        m.see_value = compute_see(b, m);
    //    }
    //    else {
    //        m.see_value = 0;
    //    }
    //    some_see = true;
    //}

    //std::sort(moveList.begin() + offset, moveList.end(), [](auto& a, auto& b) {
    //    return a.see_value > b.see_value;
    ///*    if (a.takes && !b.takes) {
    //        return true;
    //    }
    //    if (b.takes && !a.takes) {
    //        return false;
    //    }
    //    if (a.takes && b.takes) {
    //        auto ta = piece_value(a.taken_piece);
    //        auto tb = piece_value(b.taken_piece);
    //        if (ta != tb) {
    //            return ta > tb;
    //        }
    //        auto pa = piece_value(a.piece);
    //        auto pb = piece_value(b.piece);
    //    }*/
    //    return &a < &b;
    //});


    // if (!has_best_move) {
    //     int color = b.get_next_move() == C_WHITE ? +1 : -1;
    //     // internal iterative deepening
    //     if (remaining_depth >= 4 && !some_see) {
    //         //std::cout << "no best move here ==> GOING DEEP :) \n";
    //       /*  std::cout << "moveList before=\n";
    //         for (auto &m : moveList) {
    //             std::cout << move_to_string(m) << "("<<m.evaluation<<") ";
    //         }
    //         std::cout << "\n";*/
    //         int length_before = moveList.size();

    //         MoveList previousPv;
    //         NegamaxEngine engine;
    //         engine.set_max_depth(2);

    //         int max_depth = 2;
    //         for (int depth = 1; depth <= max_depth; ++depth) {
    //             MoveList newPv;
    //             engine.negamax(
    //                 b, depth, depth, 0, color,
    //                 -999999, // alpha
    //                 +999999, // beta
    //                 newPv, previousPv, &moveList
    //             );
    //             previousPv = std::move(newPv);
    //         }
    //         int length_after = moveList.size();
    //         if (length_before != length_after) {

    //         }

    //         //std::cout << "moveList after=\n";
    //         //for (auto& m : moveList) {
    //         //    std::cout << move_to_string(m) << "("<<m.evaluation<<") ";
    //         //}
    //         //std::cout << "\n\n\n";
    //     }
    // }
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
    find_move_to_position(b, dst, moveList, b.get_next_move(), -1, true, false);
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
        }
    }
    b.unmake_move(m);
    return val;
}
