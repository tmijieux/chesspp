#include <iostream>
#include <unordered_map>

#include "./move_generation.hpp"
#include "./engine.hpp"
#include "./evaluation.hpp"

Move::Move(const Board &b) : Move()
{
    castles_rights_before[CR_KING_BLACK] = b.get_castle_rights(CR_KING_BLACK);
    castles_rights_before[CR_KING_WHITE] = b.get_castle_rights(CR_KING_WHITE);
    castles_rights_before[CR_QUEEN_BLACK] = b.get_castle_rights(CR_QUEEN_BLACK);
    castles_rights_before[CR_QUEEN_WHITE] = b.get_castle_rights(CR_QUEEN_WHITE);

    checks_before[0] = b.is_king_checked(C_BLACK);
    checks_before[1] = b.is_king_checked(C_WHITE);

    en_passant_pos_before = b.get_en_passant_pos();
    half_move_before = b.get_half_move();
    //position_before = b.get_pos_string();
}

bool can_en_passant(const Board &b, const Pos &src, const Pos &dst) {
    Pos en_passant_pos = b.get_en_passant_pos();
    if (en_passant_pos.column == -1 || en_passant_pos.row == -1)
        return false;

    if (en_passant_pos.column == dst.column && en_passant_pos.row == src.row)
    {
        return true;
    }
    return false;
}

void generate_pawn_move(
    const Board& b, const Pos& pos, Color clr,
    MoveList& moveList, bool only_takes, bool pawn_attacks)
{
    int offset = clr == C_WHITE ? +1 : -1;
    int nextRow = pos.row + offset;

    Pos dst{ nextRow, pos.column };
    bool can_promote = (nextRow == 7 && clr == C_WHITE) || (nextRow == 0 && clr==C_BLACK);

    if (b.get_piece_at(dst) == P_EMPTY && !pawn_attacks && !only_takes)
    {
        Move m{ b };
        m.src = pos;
        m.dst = dst;
        m.takes = false;
        m.piece = b.get_piece_at(pos);
        m.color = clr;

        m.promote = can_promote;
        m.promote_piece = can_promote ? P_QUEEN : P_EMPTY;
        moveList.emplace_back(m);

        // initial pawn 2 square move
        Pos dst2{ nextRow+offset, pos.column };
        if (((pos.row == 1 && clr == C_WHITE)
             || (pos.row == 6 && clr == C_BLACK))
            && b.get_piece_at(dst2) == P_EMPTY) {
            Move m2{ b };
            m2.src = pos;
            m2.dst = dst2;
            m2.takes = false;
            m2.piece = b.get_piece_at(pos);
            m2.color = clr;

            moveList.emplace_back(m2);
        }
    }

    Pos dsts[2];
    dsts[0] = Pos(nextRow, pos.column - 1);
    dsts[1] = Pos(nextRow, pos.column + 1);
    for (const auto &dst : dsts) {
        if (dst.row < 0 || dst.row > 7 || dst.column < 0 || dst.column > 7) {
            continue;
        }
        Color dst_color = b.get_color_at(dst);
        bool en_passant = can_en_passant(b, pos, dst);
        if (dst_color == other_color(clr)
            || (pawn_attacks && dst_color == P_EMPTY && !only_takes)
            || en_passant)
        {

            Move m{ b };
            m.src = pos;
            m.dst = dst;
            m.takes = true;
            m.piece = b.get_piece_at(pos);
            m.taken_piece = b.get_piece_at(dst);
            if (en_passant) {
                m.taken_piece = P_PAWN;
            }
            m.color = clr;
            m.en_passant = en_passant;
            m.promote = can_promote;
            m.promote_piece = can_promote ? P_QUEEN : P_EMPTY;
            moveList.emplace_back(m);
        }
    }
}

struct Direction {
    Pos offset;
    int maxrange;
};

void generate_move_for_direction(
    const Board& b, const Pos& initial_pos, Color clr,
    const Direction& dir, MoveList& moveList, bool only_takes)
{
    Pos pos = initial_pos;
    Piece piece = b.get_piece_at(initial_pos);

    for (int distance = 1; distance <= dir.maxrange; ++distance) {
        bool takes = false;
        pos.row = initial_pos.row + distance * dir.offset.row;
        pos.column = initial_pos.column + distance * dir.offset.column;
        if (pos.row < 0 || pos.row >7 || pos.column < 0 || pos.column > 7) {
            break;
        }

        Color dstClr = b.get_color_at(pos);
        if (dstClr == clr) {
            break; // cannot jump / go to over own pieces
        }
        else if (dstClr != C_EMPTY) {
            takes = true;
        }
        if (!takes && only_takes)
        {
            continue;
        }
        Move m{ b };

        m.piece = piece;
        m.color = clr;
        m.src = initial_pos;
        m.dst = pos;
        m.takes = takes;
        m.taken_piece = b.get_piece_at(pos);

        moveList.emplace_back(m);
        if (takes) {
            break;
        }
    }
}

void generate_bishop_move(
    const Board& b, const Pos& pos, Color clr,
    MoveList& moveList, bool only_takes)
{
    Direction dirs[4] = {
        { {-1, -1}, 8 },
        { {-1, +1}, 8 },
        { {+1, -1}, 8 },
        { {+1, +1}, 8 },
    };
    for (const auto& dir : dirs) {
        generate_move_for_direction(b, pos, clr, dir, moveList, only_takes);
    }
}

void generate_knight_move(const Board& b, const Pos& pos, Color clr, MoveList& moveList, bool only_takes)
{
    Direction dirs[8] = {
        { {-1, -2}, 1 },
        { {-1, +2}, 1 },
        { {-2, +1}, 1 },
        { {-2, -1}, 1 },
        { {+1, -2}, 1 },
        { {+1, +2}, 1 },
        { {+2, +1}, 1 },
        { {+2, -1}, 1 },
    };
    for (auto& dir : dirs) {
        generate_move_for_direction(b, pos, clr, dir, moveList, only_takes);
    }
}

void generate_rook_move(const Board& b, const Pos& pos, Color clr, MoveList& moveList, bool only_takes)
{
    Direction dirs[4] = {
        { {-1, +0}, 8 },
        { {+1, +0}, 8 },
        { {+0, +1}, 8 },
        { {+0, -1}, 8 },
    };
    for (auto& dir : dirs) {
        generate_move_for_direction(b, pos, clr, dir, moveList, only_takes);
    }
}


void generate_queen_move(const Board& b, const Pos& pos, Color clr, MoveList& moveList, int max_dist, bool only_takes)
{
    Direction dirs[8] = {
        // like rook
        { {-1, +0}, max_dist },
        { {+1, +0}, max_dist },
        { {+0, +1}, max_dist },
        { {+0, -1}, max_dist },
        // like bishop
        { {-1, -1}, max_dist },
        { {-1, +1}, max_dist },
        { {+1, -1}, max_dist },
        { {+1, +1}, max_dist },
    };
    for (auto& dir : dirs) {
        generate_move_for_direction(b, pos, clr, dir, moveList, only_takes);
    }
}


void castle_king_side(
    const Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    auto idxKing = clr == C_WHITE ? CR_KING_WHITE : CR_KING_BLACK;
    if (!b.get_castle_rights(idxKing)) {
        return;
    }
    for (int i = 5; i <= 6; ++i) {
        if (b.get_piece_at(Pos{ pos.row, i }) != P_EMPTY)
        {
            return;
        }
    }

    for (int i = 4; i <= 6; ++i) {
        if (b.is_square_attacked(Pos{ pos.row, i }, other_color(clr)))
        {
            return;
        }
    }
    //if ((pos.row != 7 && clr == C_BLACK)
    //    || (pos.row != 0 && clr == C_WHITE)) {
    //    throw std::exception("invalid castle state (5)");
    //}

    Move OO{ b };
    OO.src = pos;
    OO.dst = Pos{ pos.row, pos.column + 2 };
    OO.piece = P_KING;
    OO.color = clr;
    OO.castling = true;

    moveList.emplace_back(OO);
}

void castle_queen_side(
    const Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    auto idxQueen = clr == C_WHITE ? CR_QUEEN_WHITE : CR_QUEEN_BLACK;
    if (!b.get_castle_rights(idxQueen)) {
        return;
    }
    for (int i = 3; i >= 1; --i) {
        if (b.get_piece_at(Pos{ pos.row, i }) != P_EMPTY) {
            return;
        }
    }
    for (int i = 4; i >= 2; --i) {
        if (b.is_square_attacked( Pos{ pos.row, i }, other_color(clr)))
        {
            return;
        }
    }

    Move OOO{ b };
    OOO.src = pos;
    OOO.dst = Pos{ pos.row, pos.column - 2 };
    OOO.piece = P_KING;
    OOO.color = clr;
    OOO.castling = true;
    moveList.emplace_back(OOO);
}

void generate_castle_move(
    const Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    castle_king_side(b, pos, clr, moveList);
    castle_queen_side(b, pos, clr, moveList);
}


void find_move_to_position(
    const Board& b, const Pos& pos, MoveList& moveList,
    Color move_clr, int16_t max_move, bool only_takes, bool pawn_attacks)
{
    // generate "reversed move" starting from destination and other color
    // and then "reverse" the moves

    Color enemy_clr = other_color(move_clr);

    {
        MoveList tmpList;
        generate_bishop_move(b, pos, enemy_clr, tmpList, only_takes);
        for (const auto& m : tmpList)
        {
            if (m.taken_piece == P_BISHOP || m.taken_piece == P_QUEEN)
            {
                moveList.emplace_back(m.reverse());
                if (max_move > 0 && moveList.size() >= max_move) {
                    return;
                }
            }
        }
    }

    {
        MoveList tmpList;
        generate_rook_move(b, pos, enemy_clr, tmpList, only_takes);
        for (const auto& m : tmpList) {
            if (m.taken_piece == P_ROOK || m.taken_piece == P_QUEEN) {
                moveList.emplace_back(m.reverse());
                if (max_move > 0 && moveList.size() >= max_move) {
                    return;
                }
            }
        }
    }

    {
        MoveList tmpList;
        generate_knight_move(b, pos, enemy_clr, tmpList, only_takes);
        for (const auto& m : tmpList) {
            if (m.taken_piece == P_KNIGHT)
                moveList.emplace_back(m.reverse());
        }
    }

    {
        MoveList tmpList;
        generate_pawn_move(b, pos, enemy_clr, tmpList, only_takes, pawn_attacks);
        for (const auto& m : tmpList)
        {
            if (m.taken_piece == P_PAWN)
            {
                moveList.emplace_back(m.reverse());
                if (max_move > 0 && moveList.size() >= max_move) {
                    return;
                }
            }
        }
    }

    {
        MoveList tmpList;
        // maxdist = 1 for enemy king
        generate_queen_move(b, pos, enemy_clr, tmpList, 1, true);
        for (const auto& m : tmpList)
        {
            if (m.taken_piece == P_KING)
            {
                moveList.emplace_back(m.reverse());
                if (max_move > 0 && moveList.size() >= max_move) {
                    return;
                }
            }
        }
    }
}

void add_move_from_position(
    const Board &b, const Pos & pos,
    MoveList &moveList, bool only_takes, bool pawn_attacks)
{

    Piece piece = b.get_piece_at(pos);
    Color clr = b.get_color_at(pos);
    if (piece == P_EMPTY) {
        return;
    }
    switch (piece) {
        case P_PAWN:
            generate_pawn_move(b, pos, clr, moveList, only_takes, pawn_attacks);
            break;
        case P_BISHOP:
            generate_bishop_move(b, pos, clr, moveList, only_takes);
            break;
        case P_KNIGHT:
            generate_knight_move(b, pos, clr, moveList, only_takes);
            break;
        case P_ROOK:
            generate_rook_move(b, pos, clr, moveList, only_takes);
            break;
        case P_QUEEN:
            generate_queen_move(b, pos, clr, moveList, 8, only_takes);
            break;
        case P_KING:
            // like queen_move but max dist of 1
            generate_queen_move(b, pos, clr, moveList, 1, only_takes);
            if (!only_takes) {
                generate_castle_move(b, pos, clr, moveList);
            }
            break;
        default:
            break;
    }
}

MoveList enumerate_moves(const Board& b, bool only_takes)
{
    Color to_move = b.get_next_move();

    MoveList moveList;
    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i / 8,  i % 8 };
        Color clr = b.get_color_at(pos);
        if (clr != to_move) {
            continue;
        }
        add_move_from_position(b, pos, moveList, only_takes, false);
    }
    return moveList;
}

MoveList enumerate_attacks(const Board& b, Color to_move)
{
    MoveList moveList;
    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i / 8,  i % 8 };
        Color clr = b.get_color_at(pos);
        if (clr != to_move) {
            continue;
        }
        add_move_from_position(b, pos, moveList, /*attacks*/true);
    }
    return moveList;
}

std::string piece_to_move_letter(Piece p) {
    switch (p) {
        case P_PAWN: return "";
        case P_ROOK: return "R";
        case P_BISHOP: return "B";
        case P_KNIGHT: return "N";
        case P_QUEEN: return "Q";
        case P_KING: return "K";
        default: return "X";
    }
}


std::string pos_to_square_name(const Pos& p) {
    char s[2] = { 0 };
    s[0] = ('a' + p.column);
    return std::string(s) + std::to_string((int)(p.row+1));
}
Pos square_name_to_pos(const std::string& s) {
    return Pos{
        s[1] - '1',
        s[0] - 'a'
    };
}
std::string get_char_by_piece_pgn(Piece p)
{
    switch (p) {
        case P_PAWN: return "p";
        case P_ROOK: return "r";
        case P_BISHOP: return "b";
        case P_KNIGHT: return "n";
        case P_QUEEN: return "q";
        case P_KING: return "k";
        case P_EMPTY: return " ";
        default: return "X";
    }
}

std::string move_to_string(const Move& m)
{
    std::string res = "";

    res += piece_to_move_letter(m.piece);
    if (m.takes) {
        res += "x";
    }
    res += pos_to_square_name(m.dst);
    if (m.takes) {
        auto p = get_char_by_piece_pgn(m.taken_piece);
        res += fmt::format(" (takes {})", p);
    }
    if (m.takes && m.piece == P_PAWN) {
        res = pos_to_square_name(m.src)[0] + res;
    }
    return res;
}

std::string move_to_uci_string(const Move& m)
{
    return fmt::format(
        "{}{}{}",
        pos_to_square_name(m.src),
        pos_to_square_name(m.dst) ,
        m.promote ? get_char_by_piece_pgn(m.promote_piece) : ""
    );
}

