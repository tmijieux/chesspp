#include <vector>
#include <string>
#include <sstream>
#include <iostream>

#include <fmt/format.h>

#include "./types.hpp"
#include "./pgn.hpp"
#include "./board.hpp"
#include "./move.hpp"
#include "./move_generation.hpp"


namespace pgn
{


enum TokenType {
    T_STRING = 1,
    T_OPENING_BRACKET = 2,
    T_CLOSING_BRACKET = 3,
    T_OPENING_PAREN = 4,
    T_CLOSING_PAREN = 5,
    T_OPENING_ANGLE_BRACKET = 6,
    T_CLOSING_ANGLE_BRACKET = 7,
    T_STAR = 8,
    T_PERIOD = 9,
    T_COMMENT = 10,
    T_INLINE_COMMENT = 11,
    T_EXTENSION = 12,
    T_NAG = 13, //numeric annotation glyph
    T_INTEGER = 14,
    T_SYMBOL = 15,
};

struct Token {
    TokenType type;
    std::string value;
};
using StrVec = std::vector<std::string>;
using TokenVec = std::vector<Token>;



void find_optional_src(const std::string &san, Pos &src)
{
    src.row = 0xFF;
    src.column = 0xFF;
    if (san.size() == 0)
    {
        return;
    }
    if (san.size() > 2)
    {
        throw chess_exception("invalid src position");
    }
    if (san.size() == 2)
    {
        src.column = san[0] - 'a';
        src.row = san[1] - '1';
    }
    else if (san.size() == 1) {
        if ('a' <= san[0] && san[0] <= 'h') {
            src.column = san[0] - 'a';
        }
        else if ('1' <= san[0] && san[0] <= '8')
        {
            src.row = san[0] - '1';
        }
    }
}

void find_move_src_for_attacking_piece(const Board &b, Move &move)
{
    if (move.src.row != 0xFF && move.src.column != 0xFF)
    {
        return;
    }

    MoveList ml;
    find_move_to_position(b, move.dst, ml, move.color, -1, false, false);
    if (move.src.row != 0xFF)
    {
        // find on given row
        for (auto& m : ml)
        {
            if (m.src.row == move.src.row && m.piece == move.piece)
            {
                move.src.column = m.src.column;
                break;
            }
        }
    }
    else if (move.src.column != 0xFF)
    {
        // find on given column
        for (auto& m : ml)
        {
            if (m.src.column == move.src.column && m.piece == move.piece)
            {
                move.src.row = m.src.row;
                break;
            }
        }
    }
    else
    {
        // find among all move
        for (auto& m : ml)
        {
            if (m.piece == move.piece)
            {
                move.src = m.src;
                break;
            }
        }
    }
}


/**
 * compute the move the the standart algebraic notation (abbreviated as `san`)
 */
Move compute_move_from_san(const std::string& san, Color clr, const Board& b)
{
    //Move generate_move_for_squares(
    //    const Board & b, const Pos & src, const Pos & dst, Piece promote_piece);
    if (san == "O-O") {
        if (clr == C_BLACK)
            return generate_move_for_squares(b, Pos{ 4,7 }, Pos{ 6,7 }, Piece::P_EMPTY);
        else if (clr == C_WHITE)
            return generate_move_for_squares(b, Pos{ 4,0 }, Pos{ 6,0 }, Piece::P_EMPTY);
    }
    else if (san == "O-O-O") {
        if (clr == C_BLACK)
            return generate_move_for_squares(b, Pos{ 4,7 }, Pos{ 2,7 }, Piece::P_EMPTY);
        else if (clr == C_WHITE)
            return generate_move_for_squares(b, Pos{ 4,0 }, Pos{ 2,0 }, Piece::P_EMPTY);
    }

    char first_letter = san[0];
    
    Piece piece = get_piece_by_char_pgn(first_letter);

    Move move;
    move.color = clr;

    const std::string columns = "abcdefgh";
    const std::string rows = "12345678";

    char c = san[san.size() - 1];
    bool checkMate = (c == '#');
    bool check = checkMate || (c == '+');
    size_t i = san.size()-1;
    Piece promote = get_piece_by_char_pgn(san[i]);
    if (promote != P_PAWN || san[i - 1] == '=')
    {
        i -= 2;
    }
    else
    {
        promote = P_EMPTY;
    }


    bool takes = false;
    move.dst.column = san[i] - '1';
    move.dst.row = san[i - 1] - 'a';
    auto sub1 = san.substr(0, i - 1);

    if (sub1 == piece_to_move_letter(piece)) {
        find_move_src_for_attacking_piece(b, move);
    }
    else if (san[i - 2] == 'x') {
        move.takes = 1;
        move.taken_piece = b.get_piece_at(move.dst);

        std::string sub = san.substr(std::max((size_t)0, i - 4), i - 2);
        find_optional_src(sub, move.src);
        find_move_src_for_attacking_piece(b, move);    }
    else {
        std::string sub = san.substr(std::max((size_t)0, i - 3), i - 1);
        find_optional_src(sub, move.src);
        find_move_src_for_attacking_piece(b, move);
    }
    return move;
}

std::string read_next_string(std::istream &s)
{
    int pos = 0;
    bool seen_end_quote  = 0;
    bool prev_was_backslash = false;
    std::string output = "";
    const auto len = s.rdbuf()->in_avail();

    while (!seen_end_quote) {
        char c = s.get();
        if (prev_was_backslash) {
            prev_was_backslash = false;
            if (c == '"' || c == '\\') {
                output += c;
            } else {
                throw chess_exception("invalid escape sequence \\"+c);
            }
        } else {
            prev_was_backslash = c == '\\';
            if (c == '"')
            {
                seen_end_quote = true;
            } else if (!prev_was_backslash) {
                output += c;
            }
        }
        ++pos;
        if (pos > len) {
            throw chess_exception("invalid string: unterminated: "+output);
        }
    }
    return output;
}

std::string read_comment(char first, std::istream& s)
{
    char last = 0;
    if (first == ';') {
        last = '\n';
    }
    else if (first == '{') {
        last = '}';
    }
    else {
        throw chess_exception("invalid comment start");
    }

    std::string res = "";
    int i = 0;

    auto len = s.rdbuf()->in_avail();
    char c = s.get();
    while (c != last) {
        res += c;
        ++i;
        if (i > len) {
            break;
        }
        c = s.get();
    }
    if (last == '}' && c != last) {
        throw chess_exception(fmt::format("invalid unterminated comment {}", res));
    }
    return res;
}

std::string read_numeric_annotation_glyph(std::istream& s)
{
    return "";
}

std::string read_escape_extension(std::istream& s)
{
    return "";
}

std::string read_symbol_token(std::istream& s)
{
    int pos = 0;
    std::string output = "";
    size_t len = s.rdbuf()->in_avail();

    const char glyph[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_+#=:-";
    while (true) 
    {
        char c = s.get();
        if ((c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9')
            || c == '_' || c == '+' 
            || c=='#' || c == '='
            || c == ':' || c == '-') 
        {
            output += c;
            pos++;
            if (pos > len) {
                break;
            }
        }
        else {
            s.putback(c);
            break;

        }
    }
    return output;
}

bool is_all_numeric(const std::string &s)
{
    for (const char &c : s) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

TokenVec tokenize(const std::string &body)
{
    TokenVec tokens;
    bool beginning_of_line = true;
    int line_count = 0;
    int column_count = 0;

    std::istringstream ss{body};
    while (ss.rdbuf()->in_avail() > 0) {
        char c = ss.get();
        int code = (int)c;

        if (c  == '"') {
            tokens.emplace_back(T_STRING, read_next_string(ss));
        } else if (std::isspace(c)) {
            if (c == '\n') {
                ++line_count;
                column_count = 0;
            }
        } else if (c == '[') {
            tokens.emplace_back(T_OPENING_BRACKET,"[");
        } else if (c == ']') {
            tokens.emplace_back(T_CLOSING_BRACKET,"]");
        } else if (c == '*') {
            tokens.emplace_back(T_STAR, "*");
        } else if (c == '.') {
            tokens.emplace_back(T_PERIOD, ".");
        } else if (c == ';' || c == '{') {
            auto comment = read_comment(c, ss);
            TokenType token_type = c == ';' ? T_COMMENT : T_INLINE_COMMENT;
            tokens.emplace_back(token_type, comment);
        } else if (c == '%' && beginning_of_line) {
            auto extension = read_escape_extension(ss);
            tokens.emplace_back(T_EXTENSION, extension);
        } else if (c == '(') {
            tokens.emplace_back(T_OPENING_PAREN, "(");
        } else if (c == ')') {
            tokens.emplace_back(T_CLOSING_PAREN, ")");
        } else if (c == '<') {
            tokens.emplace_back(T_OPENING_ANGLE_BRACKET, "<");
        } else if (c == '>') {
            tokens.emplace_back(T_CLOSING_ANGLE_BRACKET, ">");
        } else if (c == '$') {
            auto nag = read_numeric_annotation_glyph(ss);
            tokens.emplace_back(T_NAG, nag);
        } else if ((c >= '0' && c <= '9')
                   || (c >= 'A' && c <= 'Z')
                   || (c >= 'a' && c <= 'z')) {
            ss.putback(c);
            auto symbol = read_symbol_token(ss);
            if (is_all_numeric(symbol)) {


                tokens.emplace_back(T_INTEGER, symbol);
            } else {
                tokens.emplace_back(T_SYMBOL, symbol);
            }
        } else {
            throw chess_exception(
                fmt::format(
                    "unexpected character '{}' at line {} column {}",
                    c, line_count, column_count
                )
            );
        }
        beginning_of_line = c == '\n';
        if (!beginning_of_line) {
            ++column_count;
        }
    }
    return tokens;
}

void load_pgn_file(const std::string &body, Board &b, MoveList &moves)
{
    TokenVec tokens = tokenize(body);

    b.load_initial_position();

    bool in_tag_pair = false;
    std::string current_move_number;
    std::string current_symbol;

    int seen_period_count = 0;
    bool next_is_black = false;


    for (::pgn::Token &token : tokens)
    {
        if (token.type == T_OPENING_BRACKET) {
           in_tag_pair = true;
        } else if (in_tag_pair && token.type == T_CLOSING_BRACKET) {
           in_tag_pair = false;
        } else if (in_tag_pair) {
           if (token.type == T_SYMBOL) {
               current_symbol = token.value;
           } else if (token.type == T_STRING) {
               if (current_symbol == "Black") {
                   std::cout << "black player is "<<token.value<<"\n";
               } else if (current_symbol == "White") {
                   std::cout << "white player is "<<token.value<<"\n";
               }
           }
        } else if (!in_tag_pair) {
           if (token.type != T_PERIOD) {
               next_is_black = seen_period_count == 3;
               seen_period_count = 0;
           }
           if (token.type == T_PERIOD) {
               ++seen_period_count;
           } else if (token.type == T_INTEGER) {
               current_move_number = token.value;
           } else if (token.type == T_STAR) {
               std::cout<< " result = *\n";
               // update_board_information()
           } else if (token.type == T_SYMBOL) {
               if (token.value == "1-0"
                   || token.value == "0-1"
                   || token.value == "1/2-1/2")
               {
                   std::cout<< fmt::format(" result = {}\n", token.value);
                   break;
               }
               Color clr = next_is_black ? C_BLACK : C_WHITE;
               Move move = compute_move_from_san(token.value, clr, b);
               b.make_move(move);
               moves.push_back(move);

               next_is_black = !next_is_black;
           }
        }
    }
}


} // end namespace pgn
