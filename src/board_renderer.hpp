#ifndef CHESS_BOARD_RENDERER_H
#define CHESS_BOARD_RENDERER_H

#ifdef CHESS_ENABLE_SDL

#include <vector>

#include "SDL.h"

#include "./board.hpp"
#include "./types.hpp"
#include "./move.hpp"

class BoardRenderer
{
private:
    SDL_Window *m_window;
    SDL_Renderer *m_renderer;
    SDL_Rect m_piece_pos[2*P_NUM_PIECE];
    SDL_Texture* m_piece_tex;
    SDL_Texture* m_alphanum_texture;
    SDL_Texture* m_alphanum_black_texture;
    int m_text_width;
    int m_text_height;

    // custom events
    int m_move_event;

    std::vector<Board> m_history;
    bool m_history_mode;
    uint64_t m_current_history_pos;

    MoveList m_candidates_moves;
    bool m_need_redraw;

    void init_text(SDL_Renderer *renderer);

    void draw_squares() const;
    void draw_pieces(const Board& b) const;
    void draw_letters() const;
    void draw_candidates_moves() const;

    void prepare_player_move(Board& b, SDL_Event& e);
    void do_player_move(Board &b, SDL_Event&);

    bool m_flipped_board;

public:
    BoardRenderer();
    ~BoardRenderer();
    void draw(const Board& board) const;

    void main_loop(Board &b);

    void flip_board() { m_flipped_board = !m_flipped_board; m_need_redraw = true;}

}; // class Board_Renderer


#else // CHESS_ENABLE_SDL
class Board;
struct BoardRenderer {
    void init() {}
    void main_loop(Board&) {
        std::cout << "SDL disabled\n";
    }
};

#endif // CHESS_ENABLE_SDL


#endif // CHESS_BOARD_RENDERER_H
