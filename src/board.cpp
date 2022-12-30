#include <vector>
#include <iostream>

#include "move_generation.hpp"
#include "./board.hpp"
#include "./FenReader.hpp"

void Board::load_position(const std::string& fen_position)
{
    FenReader r;
    r.load_position(*this, fen_position);
    m_position = fen_position;
}

void Board::load_initial_position()
{
    load_position("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    //load_position("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
}

Piece Board::get_piece_at(const Pos& pos) const {
    if (pos.column > 7 || pos.column < 0 || pos.row > 7 || pos.row < 0) {
        std::abort();
    }
    uint8_t piece = m_board[pos.column + pos.row * 8];
    piece = piece & P_PIECE_MASK;
    Piece p;

    static_assert(sizeof(p) == sizeof(piece), "mismatched types size");
    std::memcpy(&p, &piece, sizeof(p));
    return p;
}

Color Board::get_color_at(const Pos& pos) const {

    if (pos.column > 7 || pos.column < 0 || pos.row > 7 || pos.row < 0) {
        std::abort();
    }
    uint8_t color = m_board[pos.column + pos.row * 8];
    color = color & C_COLOR_MASK;
    Color c;

    static_assert(sizeof(c) == sizeof(color), "mismatched types size");
    std::memcpy(&c, &color, sizeof(c));
    return c;
}
void Board::set_piece_at(const Pos& pos, Piece p, Color c) {

    if (pos.column > 7 || pos.column < 0 || pos.row > 7 || pos.row < 0) {
        std::abort();
    }
    uint8_t piece;
    uint8_t color;
    static_assert(sizeof(p) == sizeof(piece), "mismatched types size");
    static_assert(sizeof(c) == sizeof(color), "mismatched types size");

    std::memcpy(&piece, &p, sizeof(p));
    std::memcpy(&color, &c, sizeof(c));

    m_board[pos.column + pos.row * 8] = piece | color;
    if (p == P_KING) {
        auto idx = c >> 4;
        if (idx < 0 || idx >= 2) {
            std::abort();
        }
        m_king_pos[idx] = pos;
    }
}

bool Board::is_square_attacked(const Pos &p, Color attacked_by_clr) const
{
    MoveList ml;
    if (p.column == 3 && p.row == 6) {
        std::cout << "here\n";
    }
    find_move_to_position(*this, p, ml, attacked_by_clr, 1, false, true);
    return ml.size() == 1;
}

bool Board::compute_king_checked(Color clr) const
{ // start from king position and do 'inversed-move' of all type of piece
  // to see if we land on a threatening piece
    auto idx = clr >> 4;
    if (idx < 0 || idx >= 2) {
        std::abort();
    }
    Pos p = m_king_pos[idx];
    MoveList ml;
    find_move_to_position(*this, p, ml, other_color(clr), 1, true, false);
    return ml.size() == 1;
}

void Board::make_move(const Move& move)
{
    if (move.color != m_next_to_move) {
        throw std::exception("invalid move for next_to_move state");
    }
    if (move.promote) {
        set_piece_at(move.dst, move.promote_piece, move.color);
    }
    else {
        set_piece_at(move.dst, move.piece, move.color);
    }
    set_piece_at(move.src, P_EMPTY, C_EMPTY);
    if (move.en_passant) {
        set_piece_at(move.en_passant_pos_before, P_EMPTY, C_EMPTY);
    }

    bool isWhite = move.color == C_WHITE;
    if (move.castling) {
        int dstCol = move.dst.column;
        int8_t row = move.src.row;
        Pos rook_src{ row, dstCol == 6 ? 7 : 0 };
        Pos rook_dst{ row, dstCol == 6 ? 5 : 3 };

        // update rook position
        set_piece_at(rook_src, P_EMPTY, C_EMPTY);
        set_piece_at(rook_dst, P_ROOK, move.color);
    }

    // remove castling rights
    if (move.piece == P_ROOK) {
        if (move.src.column == 0) {
            set_castle_rights(isWhite ? CR_QUEEN_WHITE : CR_QUEEN_BLACK, false);
        }
        else {
            set_castle_rights(isWhite ? CR_KING_WHITE : CR_KING_BLACK, false);
        }
    }
    else if (move.piece == P_KING) {
        set_castle_rights(isWhite ? CR_KING_WHITE : CR_KING_BLACK, false);
        set_castle_rights(isWhite ? CR_QUEEN_WHITE : CR_QUEEN_BLACK, false);
    }

    if (move.takes && move.taken_piece == P_ROOK) {
        if (move.dst.column == 7) {
            set_castle_rights(isWhite ? CR_KING_BLACK : CR_KING_WHITE, false);
        } else if (move.dst.column == 0) {
            set_castle_rights(isWhite ? CR_QUEEN_BLACK : CR_QUEEN_WHITE, false);
        }
    }

    if (move.takes || move.piece == P_PAWN) {
        m_half_move_counter = 0;
    }
    else {
        ++m_half_move_counter;
    }
    if (move.color == C_BLACK) {
        ++m_full_move_counter;
    }
    if (move.piece == P_PAWN && std::abs(move.src.row - move.dst.row) == 2) {
        m_en_passant_pos = move.dst;
    }
    else {
        m_en_passant_pos = Pos{ -1,-1 };
    }
    m_king_checked[0] = compute_king_checked(C_BLACK);
    m_king_checked[1] = compute_king_checked(C_WHITE);

    m_next_to_move = other_color(m_next_to_move);

    m_position = write_fen_position(*this);
}

void Board::unmake_move(const Move& move)
{
    set_piece_at(move.src, move.piece, move.color);
    Color clr = move.takes ? other_color(move.color) : C_EMPTY;
    if (move.en_passant) {
        set_piece_at(move.en_passant_pos_before, P_PAWN, clr);
        set_piece_at(move.dst, P_EMPTY, C_EMPTY);
    } else {
        set_piece_at(move.dst, move.taken_piece, clr);
    }

    if (move.castling) {
        int dstCol = move.dst.column;
        // update rook position
        int8_t row = move.src.row;
        Pos rook_src{ row, dstCol == 6 ? 7 : 0 };
        Pos rook_dst{ row, dstCol == 6 ? 5 : 3 };

        //if (get_piece_at(rook_dst) != P_ROOK || get_color_at(rook_dst) != move.color) {
        //    throw std::exception("invalid castling state (3)");
        //}
        //if (get_piece_at(rook_src) != P_EMPTY) {
        //    throw std::exception("invalid castling state (4)");
        //}

        set_piece_at(rook_src, P_ROOK, move.color);
        set_piece_at(rook_dst, P_EMPTY, C_EMPTY);
    }
    m_en_passant_pos = move.en_passant_pos_before;
    m_half_move_counter = move.half_move_before;
    m_king_checked[0] = move.checks_before[0];
    m_king_checked[1] = move.checks_before[1];

    m_castle_rights[CR_KING_WHITE] = move.castles_rights_before[CR_KING_WHITE];
    m_castle_rights[CR_QUEEN_WHITE] = move.castles_rights_before[CR_QUEEN_WHITE];
    m_castle_rights[CR_KING_BLACK] = move.castles_rights_before[CR_KING_BLACK];
    m_castle_rights[CR_QUEEN_BLACK] = move.castles_rights_before[CR_QUEEN_BLACK];


    if (move.color == C_BLACK) {
        --m_full_move_counter;
    }
    m_next_to_move = move.color;
    //m_position = move.position_before;
}
