#include <iostream>
#include <unordered_map>

#include "./move_generation.hpp"
#include "./engine.hpp"
#include "./evaluation.hpp"


bool can_en_passant(const Board &b, const Pos &src, const Pos &dst) {
    Pos ep_pos = b.get_en_passant_pos();
    if (ep_pos.row == 0)
        return false;

    if (ep_pos.column == dst.column && ep_pos.row == src.row)
    {
        return true;
    }
    return false;
}

inline constexpr uint8_t promote_row(Color clr) {
    return clr * 7; // WHITE=7 BLACK=0
}
inline constexpr uint8_t start_row(Color clr) {
    return  6 - 5 * clr; // WHITE=1 BLACK=6
}

void generate_pawn_move_dst(
    const Board& b, const Pos& dst, Color clr,  MoveList& moveList)
{
    int8_t offset = clr == C_WHITE ? -1 : +1;
    uint8_t prevRow = dst.row + offset;
    if (prevRow > 7) {
        return;
    }
    if (b.get_piece_at(dst) != P_EMPTY)
    {
        return;
    }

    Pos src{ prevRow, dst.column};
    if (b.get_piece_at(src) == P_PAWN)
    {
        bool can_promote = dst.row == promote_row(clr);
        Move m{ b };
        m.src = src;
        m.dst = dst;
        m.takes = false;
        m.piece = P_PAWN;
        m.color = clr;
        m.promote = can_promote;
        if (can_promote) {
            for (int p = P_BISHOP; p <= P_QUEEN; ++p) {
                m.promote_piece = (Piece)p;
                moveList.emplace_back(m);
            }
        } else {
            m.promote_piece = P_EMPTY;
            moveList.emplace_back(m);
        }
    }
    else if (b.get_piece_at(src) == P_EMPTY)
    {
        // initial pawn 2 square move
        Pos src2{ u8(prevRow+offset), src.column};
        if (src2.row == start_row(clr)
            && b.get_piece_at(src2) == P_PAWN) {
            Move m2{ b };
            m2.src = src2;
            m2.dst = dst;
            m2.takes = false;
            m2.piece = P_PAWN;
            m2.color = clr;

            moveList.emplace_back(m2);
        }
    }
}

void generate_pawn_move(
    Board& b, const Pos& src, Color clr,
    MoveList& moveList, bool only_takes)
{
    int8_t offset = clr == C_WHITE ? +1 : -1;
    uint8_t nextRow = src.row + offset;
    if (nextRow > 7) {
        // this case can happen when generating reverse moves
        // to find attacks on a particular square
        return;
    }

    Pos dst{nextRow, src.column};
    bool can_promote = nextRow == promote_row(clr);

    if (b.get_piece_at(dst) == P_EMPTY && !only_takes)
    {
        Move m{ b };
        m.src = src;
        m.dst = dst;
        m.takes = false;
        m.piece = b.get_piece_at(src);
        m.color = clr;
        m.promote = can_promote;
        if (can_promote) {
            for (int p = P_BISHOP; p <= P_QUEEN; ++p) {
                m.promote_piece = (Piece)p;
                moveList.emplace_back(m);
            }
        } else {
            m.promote_piece = P_EMPTY;
            moveList.emplace_back(m);
        }

        // initial pawn 2 square move
        Pos dst2{ u8(nextRow+offset), src.column};
        if (src.row == start_row(clr)
            && b.get_piece_at(dst2) == P_EMPTY) {
            Move m2{ b };
            m2.src = src;
            m2.dst = dst2;
            m2.takes = false;
            m2.piece = b.get_piece_at(src);
            m2.color = clr;

            moveList.emplace_back(m2);
        }
    }

    Pos dsts[2];
    dsts[0] = Pos(nextRow, src.column - 1);
    dsts[1] = Pos(nextRow, src.column + 1);
    for (const auto &dst : dsts) {
        if (dst.row > 7 || dst.column > 7) {
            continue;
        }
        Color dst_color = b.get_color_at(dst);
        bool en_passant = can_en_passant(b, src, dst);
        if (dst_color == other_color(clr) || en_passant)
        {

            Move m{ b };
            m.src = src;
            m.dst = dst;
            m.takes = true;
            m.piece = b.get_piece_at(src);
            m.taken_piece = en_passant ? P_PAWN : b.get_piece_at(dst);
            if (m.taken_piece == P_EMPTY) {
                std::abort();
            }
            m.color = clr;
            m.en_passant = en_passant;
            m.promote = can_promote;
            if (can_promote) {
                for (int p = P_BISHOP; p <= P_QUEEN; ++p) {
                    m.promote_piece = (Piece)p;
                    moveList.emplace_back(m);
                }
            }
            else {
                m.promote_piece = P_EMPTY;
                moveList.emplace_back(m);
            }
        }
    }
}


struct Direction {
    Vec offset;
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
        if (pos.row >7 || pos.column > 7) {
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
        if (m.takes && m.taken_piece == P_EMPTY)
        {
            std::abort();
        }
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


void gen_castle_king_side(
    Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    auto rights = b.get_castle_rights();
    auto idxKing = clr == C_WHITE ? CR_KING_WHITE : CR_KING_BLACK;
    if ((rights & idxKing) == 0) {
        return;
    }
    for (uint8_t i = 5; i <= 6; ++i) {
        if (b.get_piece_at(Pos{ pos.row, i }) != P_EMPTY)
        {
            return;
        }
    }

    for (uint8_t i = 4; i <= 6; ++i) {
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
    OO.dst = Pos{ u8(pos.to_val()+2) };
    OO.piece = P_KING;
    OO.color = clr;
    OO.castling = true;

    moveList.emplace_back(OO);
}

void gen_castle_queen_side(
    Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    auto rights = b.get_castle_rights();
    auto idxQueen = clr == C_WHITE ? CR_QUEEN_WHITE : CR_QUEEN_BLACK;
    if ((rights & idxQueen) == 0) {
        return;
    }
    for (uint8_t i = 3; i >= 1; --i) {
        if (b.get_piece_at(Pos{ pos.row, i }) != P_EMPTY) {
            return;
        }
    }
    for (uint8_t i = 4; i >= 2; --i) {
        if (b.is_square_attacked( Pos{ pos.row, i}, other_color(clr)))
        {
            return;
        }
    }

    Move OOO{ b };
    OOO.src = pos;
    OOO.dst = Pos{ u8(pos.to_val() - 2) };
    OOO.piece = P_KING;
    OOO.color = clr;
    OOO.castling = true;
    moveList.emplace_back(OOO);
}

void generate_castle_move(
    Board& b, const Pos& pos, Color clr, MoveList& moveList)
{
    gen_castle_king_side(b, pos, clr, moveList);
    gen_castle_queen_side(b, pos, clr, moveList);
}


void find_move_to_position(
    Board& b, const Pos& pos, MoveList& moveList,
    Color move_clr, int16_t max_move, bool only_takes)
{
    // generate "reversed move" starting from destination and other color
    // and then "reverse" the moves
    Piece at_dst = b.get_piece_at(pos);
    if (only_takes &&  at_dst == P_EMPTY)
    {
        return;
    }

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
            {
                moveList.emplace_back(m.reverse());
                if (max_move > 0 && moveList.size() >= max_move) {
                    return;
                }
            }
        }
    }

    {
        if (at_dst != P_EMPTY)
        {
            // only takes here
            MoveList tmpList;
            generate_pawn_move(b, pos, enemy_clr, tmpList, true);
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

        if (!only_takes)
        {
            MoveList tmpList;
            generate_pawn_move_dst(b, pos, move_clr, tmpList);
            for (const auto& m : tmpList)
            {
                moveList.emplace_back(m);
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
    Board &b, const Pos & pos,
    MoveList &moveList, bool only_takes)
{

    Piece piece = b.get_piece_at(pos);
    Color clr = b.get_color_at(pos);
    if (piece == P_EMPTY) {
        return;
    }
    switch (piece) {
        case P_PAWN:
            generate_pawn_move(b, pos, clr, moveList, only_takes);
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

void remove_duplicate_moves(MoveList &ml)
{
    int invalid_count = 0;
    for (size_t i = 1; i < ml.size(); ++i) {

        for (size_t j = 0; j < i; ++j) {
            if (ml[j].legal_checked) {
                continue;
            }
            if (ml[i] == ml[j]) {
                ml[i].legal_checked = true;
                ml[i].legal = false;
                ++invalid_count;
                break;
            }
        }
    }
    // compact
    for (size_t i = 1; i < ml.size(); ++i)
    {
        if (ml[i].legal_checked) {
            continue;
        }
        for (size_t j = 0; j < i; ++j) {
            if (ml[j].legal_checked) {
                std::swap(ml[i], ml[j]);
                continue;
            }
        }
    }
    ml.resize(ml.size() - invalid_count);
}

MoveList generate_check_evading_moves(Board& b)
{
    Color clr = b.get_next_move();
    Pos kpos = b.get_king_pos(clr);
    MoveList king_attacks;
    find_move_to_position(b, kpos, king_attacks, other_color(clr), 2, true);
    bool double_check = king_attacks.size() > 1;
    MoveList res;

    if (double_check)
    {
        // move king out of the way
        generate_queen_move(b, kpos, clr, res, 1, false);
        return res;
    }
    if (king_attacks.size() == 0) {
        throw chess_exception("no checks ??");
    }
    auto& attacker_pos = king_attacks[0].src;
    Piece checker = b.get_piece_at(attacker_pos);
    if (checker == P_KNIGHT)
    {
        // capture the knight
        find_move_to_position(b, attacker_pos, res, clr, -1, true);
        // or move out of the way
        generate_queen_move(b, kpos, clr, res, 1, false);
        return res;
    }
    else
    {
        // capture the piece
        find_move_to_position(b, attacker_pos, res, clr, -1, true);

        int rd = attacker_pos.row - kpos.row;
        int ro = rd / std::max(std::abs(rd), 1);
        int cd = attacker_pos.column - kpos.column;
        int co = cd / std::max(std::abs(cd), 1);

        // block the attack
        for (uint8_t i = kpos.row+ro, j = kpos.column+co;
             i != attacker_pos.row || j != attacker_pos.column;
             i += ro, j += co)
        {
            find_move_to_position(b, Pos{ i,j }, res, clr, -1, false);
        }
        // or move out of the way
        generate_queen_move(b, kpos, clr, res, 1, false);

        remove_duplicate_moves(res);
        return res;
    }
}

MoveList generate_pseudo_moves(Board& b, bool only_takes)
{
    Color to_move = b.get_next_move();

    MoveList moveList;
    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i };
        Color clr = b.get_color_at(pos);
        if (clr != to_move) {
            continue;
        }
        add_move_from_position(b, pos, moveList, only_takes);
    }
    return moveList;
}

Move generate_move_for_squares(
    Board &b,  const Pos &src, const Pos &dst, Piece promote_piece)
{
    MoveList ml;
    add_move_from_position(b, src, ml, false);
    for (const auto& m : ml) {
        if (m.dst == dst && m.promote_piece == promote_piece) {
            return m;
        }
    }
    throw chess_exception("unreachable");
}



MoveList enumerate_attacks(Board& b, Color to_move)
{
    MoveList moveList;
    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i };
        Color clr = b.get_color_at(pos);
        if (clr != to_move) {
            continue;
        }
        add_move_from_position(b, pos, moveList, /*attacks*/true);
    }
    return moveList;
}

std::string col_name(uint8_t col) {
    char s[2] = { 0 };
    s[0] = ('a' + col);
    return std::string(s);
}
std::string row_name(uint8_t row) {
    return std::to_string((int)(row + 1));
}

std::string pos_to_square_name(const Pos& p) {
    return col_name(p.column) + row_name(p.row);
}


Pos square_name_to_pos(const std::string& s) {
    return Pos{
        (uint8_t)(s[1] - '1'),
        (uint8_t)(s[0] - 'a')
    };
}

std::string move_to_string(const Move& m)
{
    std::string res = "";

    res += piece_to_move_letter(m.piece);
    if (m.piece == P_PAWN && m.takes)
    {
        res += col_name(m.src.column);
    }
    if (m.takes) {
        res += "x";
    }
    res += pos_to_square_name(m.dst);

    if (m.promote) {
        res += std::string("=") + piece_to_move_letter(m.promote_piece);
    }

    if (m.mate) {
        res += "#";
    }
    else if (m.checks) {
        res += "+";
    }
    return res;
}

std::string move_to_string_disambiguate(Board &b, const Move& m)
{
    std::string res = move_to_string(m);
    if (m.piece == P_PAWN || m.piece == P_KING) {
        return res;
    }
    MoveList move_candidates;
    find_move_to_position(b, m.dst, move_candidates, m.color, -1, m.takes);
    int num_same_piece = 0;
    for (auto& c : move_candidates) {
        if (c.src != m.src && c.piece == m.piece) {
            ++num_same_piece;
        }
    }
    
    if (num_same_piece >= 1)
    {
        bool has_on_same_row = false;
        bool has_on_same_column = false;
        for (auto& candidate : move_candidates)
        {
            if (candidate.piece == m.piece && candidate.src != m.src)
            {
                if (candidate.src.column == m.src.column) {
                    has_on_same_row = true;
                }
                if (candidate.src.row == m.src.row) {
                    has_on_same_row = true;
                }
            }
        }
        std::string extra;
        if (has_on_same_row || has_on_same_column)
        {
            if (has_on_same_row) {
                extra += col_name(m.src.column);
            }
            if (has_on_same_column) {
                extra += row_name(m.src.row);
            }
        }
        else
        {
            extra += col_name(m.src.column);
        }
        res = res.substr(0, 1) + extra + res.substr(1);
    }
    return res;
}


std::string move_to_uci_string(const Move& m)
{
    return fmt::format(
        "{}{}{}",
        pos_to_square_name(m.src),
        pos_to_square_name(m.dst),
        m.promote ? get_char_by_piece_pgn(m.promote_piece) : ""
    );
}

