#ifndef CHESS_TYPES_H
#define CHESS_TYPES_H

enum Piece : uint8_t {
    P_EMPTY  = 0,
    P_PAWN   = 1,
    P_BISHOP = 2,
    P_KNIGHT = 3,
    P_ROOK   = 4,
    P_QUEEN  = 5,
    P_KING   = 6,
    P_INVALID_PIECE = 7,
    P_PIECE_MASK = 0x07,

    P_MIN_PIECE = 1,
    P_MAX_PIECE = 6,
    P_NUM_PIECE = 7,
};

enum Color : uint8_t {
    C_EMPTY = 0x00,
    C_BLACK = 0x08,
    C_WHITE = 0x10,
    C_INVALID_COLOR = 0x18,
    C_COLOR_MASK = 0x18,
};
inline Color other_color(Color c) { return (Color)(0x18 - (uint8_t)c); }

enum CasleRightIndex: int8_t {
    CR_QUEEN_BLACK = 0,
    CR_KING_BLACK = 1,
    CR_QUEEN_WHITE = 2,
    CR_KING_WHITE = 3,
};

struct Pos {
public:
    int8_t row;
    int8_t column;
    Pos(int row_, int column_) :row(row_), column(column_) {}
    Pos() :Pos(-1, -1) {}

    bool operator==(const Pos& o) {
        return row == o.row && column == o.column;
    }
};


#endif // CHESS_TYPES_H
