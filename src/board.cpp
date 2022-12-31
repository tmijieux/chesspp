#include <vector>
#include <iostream>

#include "./FenReader.hpp"
#include "move_generation.hpp"
#include "./board.hpp"

void Board::load_position(const std::string& fen_position)
{
    FenReader r;
    r.load_position(*this, fen_position);
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
    auto x = pos.column + pos.row * 8;
    uint8_t q = x >> 1;
    uint8_t n = x & 1;
    uint8_t val = (m_board[q] >> (4 * n)) & P_PIECE_MASK;
    return (Piece)val;
}

Color Board::get_color_at(const Pos& pos) const {

    if (pos.column > 7 || pos.column < 0 || pos.row > 7 || pos.row < 0) {
        std::abort();
    }
    uint8_t x = pos.column + pos.row * 8;
    uint8_t q = x >> 1;
    uint8_t n = x & 1;
    uint8_t val = (m_board[q] >> (4 * n)) & 0x0F;
    if (val == 0) {
        return C_EMPTY;
    }
    return (Color) (val >> 3);
}

void Board::set_piece_at(const Pos& pos, Piece p, Color c) {

    if (pos.column > 7 || pos.column < 0 || pos.row > 7 || pos.row < 0) {
        std::abort();
    }
    uint8_t x = pos.column + pos.row * 8;
    uint8_t q = x >> 1;
    uint8_t n = x & 1;

    uint8_t store = m_board[q];
    uint8_t newval = ((uint8_t)p) | (((uint8_t)c)<<3);

    m_board[q] = (store & (0x0F << (4 * (1-n))))  | (newval << (4 * (n)));

    if (p == P_KING) {
        set_king_pos(pos, c);
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
    Pos p = get_king_pos(clr);
    MoveList ml;
    find_move_to_position(*this, p, ml, other_color(clr), 1, true, false);
    return ml.size() == 1;
}

void Board::make_move(const Move& move)
{
    if (move.color != get_next_move()) {
        throw std::exception("invalid move for next_to_move state");
    }
    if (move.promote) {
        set_piece_at(move.dst, move.promote_piece, move.color);
    }
    else {
        set_piece_at(move.dst, move.piece, move.color);
    }
    set_piece_at(move.src, P_EMPTY, C_BLACK);
    if (move.en_passant) {
        set_piece_at(get_en_passant_pos(), P_EMPTY, C_BLACK);
    }

    bool isWhite = move.color == C_WHITE;
    if (move.castling) {
        int dstCol = move.dst.column;
        int8_t row = move.src.row;
        Pos rook_src{ row, dstCol == 6 ? 7 : 0 };
        Pos rook_dst{ row, dstCol == 6 ? 5 : 3 };

        // update rook position
        set_piece_at(rook_src, P_EMPTY, C_BLACK);
        set_piece_at(rook_dst, P_ROOK, move.color);
    }

    // remove castling rights
    if (move.piece == P_ROOK) {
        if (move.src.column == 0) {
            set_castle_rights(isWhite ? CR_QUEEN_WHITE : CR_QUEEN_BLACK, false);
        }
        else if (move.src.column == 7) {
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
        set_en_passant_pos(move.dst.column | CAN_EN_PASSANT);
    }
    else {
        set_en_passant_pos(0);
    }
    set_king_checked(compute_king_checked(C_BLACK) | (compute_king_checked(C_WHITE) << 1));
    set_next_move(other_color(get_next_move()));
}

void Board::unmake_move(const Move& move)
{
    set_piece_at(move.src, move.piece, move.color);
    Color clr = move.takes ? other_color(move.color) : C_BLACK;
    m_flags = move.m_flags_before;

    if (move.en_passant) {
        set_piece_at(get_en_passant_pos(), P_PAWN, clr);
        set_piece_at(move.dst, P_EMPTY, C_BLACK);
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
        set_piece_at(rook_dst, P_EMPTY, C_BLACK);
    }
    if (move.color == C_BLACK) {
        --m_full_move_counter;
    }
}
