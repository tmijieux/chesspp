#ifndef CHESS_FEN_READER_H
#define CHESS_FEN_READER_H

#include "./board.hpp"
#include <string>

class FenReader
{
public:
    void load_position(Board &b, const std::string&fen_position) const;
private:

}; // class Fenreader

std::string write_fen_position(const Board& b);


#endif // CHESS_FEN_READER_H
