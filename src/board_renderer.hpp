#ifndef CHESS_BOARD_RENDERER_H
#define CHESS_BOARD_RENDERER_H

#include "SDL.h"
#include "./board.hpp"
#include "./types.hpp"

class BoardRenderer
{
private:
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SDL_Texture *m_piece_tex_white[P_NUM_PIECE];
    SDL_Texture* m_piece_tex_black[P_NUM_PIECE];

    int m_move_event;

    std::vector<Board> m_history;
    bool m_history_mode;
    uint64_t m_current_history_pos;

    MoveList m_candidates_moves;
    bool m_need_redraw;

    void draw_squares() const;
    void draw_pieces(const Board& b) const;
    void draw_candidates_moves() const;


    void prepare_player_move(Board& b, SDL_Event& e);
    void do_player_move(Board &b, SDL_Event&);

public:
    void init();
    BoardRenderer();
    ~BoardRenderer();
    void draw(const Board& board) const;

    void console_draw(const Board& board) const;
    void main_loop(Board &b);

}; // class Board_Renderer

#endif // CHESS_BOARD_RENDERER_H
