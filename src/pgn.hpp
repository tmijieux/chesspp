/**
 * chess Portable Game Notation (abbreviated as PGN)
 */

#ifndef CHESS_PGN_H
#define CHESS_PGN_H

#include <string>
#include "./move.hpp"

namespace pgn {

void load_pgn_file(const std::string& body, Board& b, MoveList& moves);

} // end namespace pgn



#endif // CHESS_PGN_H
