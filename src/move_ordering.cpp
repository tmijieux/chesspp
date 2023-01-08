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
    Board &b, MoveList &moveList, int ply, int remaining_depth,
    KillerMoves &killers, bool has_hash_move, const Move &hash_move)
{
    size_t offset = 0;

    bool some_killers = false;
    size_t startOfKillers = offset;
    if (!has_hash_move && remaining_depth > 4) {
        // INTERNAL ITERATIVE DEEPENING
        int color = b.get_next_move() == C_WHITE ? +1 : -1;
        size_t length_before = moveList.size();

        NegamaxEngine engine;
        engine.set_max_depth(2);
        NodeType children;
        MoveList pvLine;

        int max_depth = std::max(remaining_depth/3, 2);
        for (int depth = 1; depth <= max_depth; ++depth) {
            engine.negamax(
                b, depth, depth, 0, color,
                -999999, // alpha
                +999999, // beta
                &moveList,
                true,
                children,
                pvLine
            );
        }
        return;
    }
    if (ply < killers.size() && killers[ply].size() > 0) {
        auto& mykillers = killers[ply];
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

    if (ply >= 2 && ply < killers.size() && killers[ply-2].size() > 0) {
        Move& killer = killers[ply-2][0];
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

    // if (some_killers) {
    //     // put mate killers before other mates
    //     std::sort(
    //         moveList.begin()+startOfKillers, moveList.begin()+offset,
    //         [](const auto& a, const auto& b) {
    //             if (a.mate_killer && !b.mate_killer) {
    //                 return true;
    //             } else if (b.mate_killer && !a.mate_killer) {
    //                 return false;
    //             } else {
    //                 return &a < &b;
    //             }
    //         }
    //     );
    // }

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

    auto size = moveList.size();
    for (auto i = offset; i < size; ++i) {
        auto& m = moveList[i];
        if (m.takes) {
            //m.see_value = compute_see(b, m);
            m.see_value = piece_value(m.taken_piece)-piece_value(m.piece);
        }
        else {
            m.see_value = 0;
        }
    }

    std::sort(
        moveList.begin() + offset, moveList.end(),
        [](auto& a, auto& b) {
            if (a.see_value > b.see_value) {
                return true;
            }
            if (a.see_value < b.see_value) {
                return false;
            }
            if (a.killer && !b.killer) {
                return true;
            }
            if (!a.killer && b.killer) {
                return false;
            }
            if (a.mate_killer && !b.mate_killer) {
                return true;
            }
            if (!a.mate_killer && b.mate_killer) {
                return false;
            }
            return &a < &b;
        }
    );

    // if (has_best_move || remaining_depth <= 4) {

    //     auto size = moveList.size();
    //     for (auto i = offset; i < size; ++i) {
    //         auto& m = moveList[i];
    //         if (m.takes) {
    //             m.see_value = compute_see(b, m);
    //         }
    //         else {
    //             m.see_value = 0;
    //         }
    //     }

    //     std::sort(
    //         moveList.begin() + offset, moveList.end(),
    //         [](auto& a, auto& b) {
    //             return a.see_value > b.see_value;
    //         }
    //     );
    // } else {
    //     // INTERNAL ITERATIVE DEEPENING
    //     int color = b.get_next_move() == C_WHITE ? +1 : -1;
    //     size_t length_before = moveList.size();

    //     NegamaxEngine engine;
    //     engine.set_max_depth(2);

    //     int max_depth = std::max(remaining_depth/3, 2);
    //     for (int depth = 1; depth <= max_depth; ++depth) {
    //         engine.negamax(
    //             b, depth, depth, 0, color,
    //             -999999, // alpha
    //             +999999, // beta
    //             &moveList,
    //             true
    //         );
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
