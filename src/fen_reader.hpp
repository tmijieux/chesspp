#ifndef CHESS_FEN_READER_H
#define CHESS_FEN_READER_H

#include <string>

#include "./board.hpp"

class FenReader
{
public:
    void load_position(Board &b, const std::string&fen_position) const;
private:

}; // class Fenreader


#endif // CHESS_FEN_READER_H
