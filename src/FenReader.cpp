#include <istream>
#include <sstream>

#include "./FenReader.hpp"
#include "./types.hpp"

Piece get_piece_by_char(char c)
{
    switch (c) {
    case 'p':
    case 'P':
        return P_PAWN;
    case 'r':
    case 'R':
        return P_ROOK;
    case 'b':
    case 'B':
        return P_BISHOP;
    case 'n':
    case 'N':
        return P_KNIGHT;
    case 'q':
    case 'Q':
        return P_QUEEN;
    case 'k':
    case 'K':
        return P_KING;
    default:
        return P_INVALID_PIECE;
    }
}


Color get_color_by_char(char c)
{
    if ('a' <= c && c <= 'z') {
        return C_BLACK;
    }
    else if ('A' <= c && c <= 'Z') {
        return C_WHITE;
    }
    else {
        return C_INVALID_COLOR;
    }
}

void remove_whitespace(std::istream& input)
{
    char c;
    do {
        c = input.get();
    } while (c == ' ');
    input.putback(c);
}

void fen_read_position(Board &b, std::istream& input)
{
    Pos cur_pos{ 7, 0 };
    for (;;) {
        char c = input.get();
        if (c == ' ') {

            break;
        }
        if (c == '/') {
            if (cur_pos.column != 0 || cur_pos.row >= 8) {
                throw invalid_fen_string();
            }
            continue;
        }
        if (cur_pos.row < 0) {
            break;
        }

        Piece p = get_piece_by_char(c);
        Color clr = get_color_by_char(c);
   
        if (p != P_INVALID_PIECE)
        {
            b.set_piece_at(cur_pos, p, clr);
            ++cur_pos.column;
            if (cur_pos.column == 8) {
                cur_pos.column = 0;
                --cur_pos.row;
            }
        }
        else if ('1' <= c && c <= '8')
        {
            int length = c - '0';
            for (int i = 0; i < length; ++i) {
                b.set_piece_at(cur_pos, P_EMPTY, C_EMPTY);
                ++cur_pos.column;
                if (cur_pos.column == 8) {
                    cur_pos.column = 0;
                    --cur_pos.row;
                }
            }
        }
        else {
            throw invalid_fen_string();
        }
    }
    remove_whitespace(input);
}

void fen_read_next_to_move(Board& b, std::istream& input)
{
    for (;;) {
        char c = input.get();
        if (c == ' ') {
            break;
        }
        else if (c == 'w') {
            b.set_next_move(C_WHITE);
            break;
        }
        else if (c == 'b') {
            b.set_next_move(C_BLACK);
            break;
        }
        else {
            throw invalid_fen_string();
        }
    }
    remove_whitespace(input);
}

void fen_read_castle_rights(Board& b, std::istream& input)
{
    b.set_castle_rights(CR_KING_BLACK, false);
    b.set_castle_rights(CR_QUEEN_BLACK, false);
    b.set_castle_rights(CR_KING_WHITE, false);
    b.set_castle_rights(CR_QUEEN_WHITE, false);

    for (;;) {
        char c = input.get();
        if (c == ' ' || c == '-') {
            break;
        }
        else if (c == 'k') {
            b.set_castle_rights(CR_KING_BLACK, true);
        }
        else if (c == 'q') {
            b.set_castle_rights(CR_QUEEN_BLACK, true);
        }
        else if (c == 'K') {
            b.set_castle_rights(CR_KING_WHITE, true);
        }
        else if (c == 'Q') {
            b.set_castle_rights(CR_QUEEN_WHITE, true);
        }
        else {
            throw invalid_fen_string();
        }
    }
    remove_whitespace(input);
}

void fen_read_en_passant_position(Board& b, std::istream& input)
{
    Pos en_passant_pos{ -1,-1 };
    for (;;) {
        char c = input.get();
        if (c == '-') {
            en_passant_pos = Pos{ -1,-1 };
            break;
        }
        if ('a' <= c && c <= 'h') {
            en_passant_pos.column = c - 'a';
        }
        else if ('1' <= c && c <= '8') {
            en_passant_pos.row = c - '1';
        }
        else {
            throw invalid_fen_string();
        }
    }

    if ((en_passant_pos.column == -1 && en_passant_pos.row != -1)
        || (en_passant_pos.row == -1 && en_passant_pos.column != -1)) {
        // if half of position is missing
        throw invalid_fen_string();
    }
    b.set_en_passant_pos(en_passant_pos);
    remove_whitespace(input);
}


void FenReader::load_position(Board &b, const std::string&fen_position) const
{
    std::istringstream reader(fen_position);
    remove_whitespace(reader);

    fen_read_position(b, reader);
    fen_read_next_to_move(b, reader);
    fen_read_castle_rights(b, reader);
    fen_read_en_passant_position(b, reader);

    int half_move = -1;
    reader >> half_move;
    b.set_half_move(half_move);

    int full_move = -1;
    reader >> full_move;
    b.set_full_move(full_move);
}
