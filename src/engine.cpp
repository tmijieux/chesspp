#include <iostream>

#include "./engine.hpp"


int32_t negamax(
    Board& b,
    int max_depth, int remaining_depth, int current_depth,
    int color,
    int32_t alpha, int32_t beta,
    MoveList& parentPv,
    const MoveList& previousPv,
    MoveList* topLevelOrdering
) {
    MoveList currentPv;
    Color clr = b.get_next_move();
    currentPv.reserve(remaining_depth+1);

    if (remaining_depth == 0) {
        int32_t val = quiescence(b, color, alpha, beta);
        return val ;
    }
    MoveList moveList;
    if (topLevelOrdering !=  nullptr && topLevelOrdering->size() > 0) {
        moveList = *topLevelOrdering;
    } else {
        moveList = enumerate_moves(b);
        reorder_moves(moveList, current_depth, previousPv);
    }
    uint32_t num_legal_move = 0;

    bool cutoff = false;
    for (auto& move : moveList) {
        b.make_move(move);
        if (b.is_king_checked(clr)) {
            // illegal move
            move.legal_checked = true;
            move.legal = false;
            move.evaluation = std::numeric_limits<int32_t>::min();
            b.unmake_move(move);
            continue;
        }
        move.legal_checked = true;
        move.legal = true;
        ++num_legal_move;
        int32_t val = -negamax(
            b, max_depth, remaining_depth - 1, current_depth +1,
            -color, -beta, -alpha,
            currentPv, previousPv, nullptr
        );
        b.unmake_move(move);

        move.evaluation = val;
        if (val >= beta) {
            alpha = beta;
            break;
        }
        if (val > alpha) {
            alpha = val;
            // new pV node ???
            // collect PV :
            if (parentPv.size() == 0) {
                parentPv.push_back(move);
            }
            else {
                parentPv[0] = move;
            }
            parentPv.resize(1);
            parentPv.insert(parentPv.begin() + 1, currentPv.begin(), currentPv.end());
            // std::cout << "alpha increase\n";
        }
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
        for (auto &m : moveList) {
            if (!m.legal) {
                continue;
            }
            std::cout << move_to_string(m) << " "<<m.evaluation<<"\n";
        }
    }
    return alpha;
}

void iterative_deepening(
    Board& b, int max_depth, Move *bestMove, bool *moveFound)
{
    *moveFound = false;

    int color = b.get_next_move() == C_WHITE ? +1 : -1;
    MoveList previousPv;
    MoveList topLevelOrdering;

     for (int depth = 1; depth <= max_depth; ++depth) {
         MoveList newPv;
         newPv.reserve(depth);

         negamax(
             b, max_depth, depth, 0, color,
             -999999, // alpha
             +999999, // beta
             newPv, previousPv, &topLevelOrdering
         );
         if (newPv.size() > 0) {
             std::cout << "Best move for depth="
                 << depth << "  = " << move_to_string(newPv[0])
                 << "\n\n\n";
         }
         else {
             *moveFound = false;
             return;
         }


         std::cout << "PV = ";
         for (const auto& m : newPv) {
             std::cout << move_to_string(m) << " ";
         }
         std::cout << "\n";
         std::cout << "-----------------\n\n";

         *bestMove = newPv[0];
         *moveFound = true;

         previousPv = std::move(newPv);
     }
}

std::pair<bool,Move> find_best_move(Board& b)
{
    Move move;
    bool found;
    iterative_deepening(b, 4, &move, &found);
    if (!found) {
        std::cout << "WARNING no move found !!" << std::endl;
    }

    return std::make_pair(found,move);
}
