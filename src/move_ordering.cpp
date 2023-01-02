#include <iostream>
#include <unordered_map>
#include <algorithm>

#include "./move_generation.hpp"
#include "./move_ordering.hpp"
#include "./evaluation.hpp"
#include "./engine.hpp"


void reorder_mvv_lva(
    const Board& b, MoveList& moveList,
    size_t begin, size_t end)
{
    std::sort(
      /*  moveList.begin() + begin,
        moveList.begin() + end,*/
        moveList.begin(),
        moveList.end(),
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
    Board &b,
    MoveList &moveList,
    int current_depth,
    int remaining_depth,
    const MoveList& previousPv,
    KillerMoves &killers,
    const Move &hash_move, bool has_hash_move)
{
    bool has_best_move = false;
    size_t offset = 0;
    //if (current_depth < previousPv.size()) {
    //    const Move& bestMove = previousPv[current_depth];

    //    for (size_t i = offset+1; i < moveList.size(); ++i) {
    //        if (moveList[i] == bestMove ) {
    //            std::swap(moveList[offset], moveList[i]);
    //            has_best_move = true;
    //            moveList[offset].best_from_pv = true;
    //            ++offset;
    //            break;
    //        }
    //    }
    //}
    if (has_hash_move) {
        for (size_t i = offset+1; i < moveList.size(); ++i) {
            if (moveList[i] == hash_move ) {
                std::swap(moveList[offset], moveList[i]);
                has_best_move = true;
                moveList[offset].hash_move = true;
                ++offset;
                break;
            }
        }
    }

    bool some_killers = false;
    size_t startOfKillers = offset;
    if (current_depth < killers.size() && killers[current_depth].size() > 0) {
        auto& mykillers = killers[current_depth];
        for (const auto& killer : mykillers) {
            for (size_t i = offset+1; i < moveList.size(); ++i) {
                if (moveList[i] == killer) {
                    std::swap(moveList[offset], moveList[i]);
                    moveList[offset].killer = true;
                    ++offset;
                    some_killers = true;
                    break;
                }
            }
        }
    }

    if (some_killers) {
        // put mate killers before other mates
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

    // size_t startAt = offset;

    // std::sort(
    //     moveList.begin()+startAt, moveList.end(),
    //     [](const auto& a, const auto& b) {
    //         auto ta = piece_value(a.taken_piece);
    //         auto tb = piece_value(b.taken_piece);
    //         if (ta != tb) {
    //             return ta > tb;
    //         }
    //         auto pa = piece_value(a.piece);
    //         auto pb = piece_value(b.piece);
    //         return pa < pb;
    //     }
    // );

    if (has_best_move || remaining_depth <= 4) {

        auto size = moveList.size();
        for (auto i = offset; i < size; ++i) {
            auto& m = moveList[i];
            if (m.takes) {
                m.see_value = compute_see(b, m);
            }
            else {
                m.see_value = 0;
            }
        }

        std::sort(
            moveList.begin() + offset, moveList.end(),
            [](auto& a, auto& b) {
                return a.see_value > b.see_value;
            }
        );
    } else {
        // INTERNAL ITERATIVE DEEPENING
        int color = b.get_next_move() == C_WHITE ? +1 : -1;
        size_t length_before = moveList.size();

        MoveList previousPv;
        NegamaxEngine engine;
        engine.set_max_depth(2);

        int max_depth = std::max(remaining_depth/3, 2);
        for (int depth = 1; depth <= max_depth; ++depth) {
            MoveList newPv;
            engine.negamax(
                b, depth, depth, 0, color,
                -999999, // alpha
                +999999, // beta
                newPv, previousPv, &moveList,
                true
            );
            previousPv = std::move(newPv);
        }
    }
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
        } // if not found means there is no takes to this square 
    }
    b.unmake_move(m);
    return val;
}
