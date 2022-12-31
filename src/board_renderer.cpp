#ifdef CHESS_ENABLE_SDL

#include <iostream>
#include <filesystem>

#include "SDL.h"
#include "SDL_image.h"

#include "./board_renderer.hpp"
#include "./move_generation.hpp"
#include "./engine.hpp"


void draw_circle(SDL_Renderer* renderer, int32_t centreX, int32_t centreY, int32_t radius)
{
    const int32_t diameter = (radius * 2);

    int32_t x = (radius - 1);
    int32_t y = 0;
    int32_t tx = 1;
    int32_t ty = 1;
    int32_t error = (tx - diameter);

    while (x >= y)
    {
        //  Each of the following renders an octant of the circle
        SDL_RenderDrawPoint(renderer, centreX + x, centreY - y);
        SDL_RenderDrawPoint(renderer, centreX + x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX - x, centreY - y);
        SDL_RenderDrawPoint(renderer, centreX - x, centreY + y);
        SDL_RenderDrawPoint(renderer, centreX + y, centreY - x);
        SDL_RenderDrawPoint(renderer, centreX + y, centreY + x);
        SDL_RenderDrawPoint(renderer, centreX - y, centreY - x);
        SDL_RenderDrawPoint(renderer, centreX - y, centreY + x);

        if (error <= 0)
        {
            ++y;
            error += ty;
            ty += 2;
        }

        if (error > 0)
        {
            --x;
            tx += 2;
            error += (tx - diameter);
        }
    }
}

//P_EMPTY = 0,
//P_PAWN = 1,
//P_BISHOP = 2,
//P_KNIGHT = 3,
//P_ROOK = 4,
//P_QUEEN = 5,
//P_KING = 6,

const char* piece_path_black[P_NUM_PIECE] = {
    nullptr,
    "chess-pawn-black.png",
    "chess-bishop-black.png",
    "chess-knight-black.png",
    "chess-rook-black.png",
    "chess-queen-black.png",
    "chess-king-black.png",
};
const char* piece_path_white[P_NUM_PIECE] = {
    nullptr,
    "chess-pawn-white.png",
    "chess-bishop-white.png",
    "chess-knight-white.png",
    "chess-rook-white.png",
    "chess-queen-white.png",
    "chess-king-white.png",
};

BoardRenderer::BoardRenderer() :
    m_window(nullptr),
    m_renderer(nullptr),
    m_piece_tex_white{ nullptr },
    m_piece_tex_black{ nullptr },
    m_move_event{ 0 },
    m_history_mode{ false },
    m_current_history_pos{ 0 },
    m_need_redraw{ true }
{
    std::fill(std::begin(m_piece_tex_white), std::end(m_piece_tex_white), nullptr);
    std::fill(std::begin(m_piece_tex_black), std::end(m_piece_tex_black), nullptr);
}

void BoardRenderer::init()
{
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("chesspp", 100, 100, 640, 640, 0);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    IMG_Init(IMG_INIT_PNG);

    m_move_event = SDL_RegisterEvents(1);
    std::cout << "move_event="<<m_move_event<<"\n";

    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "2" );

    for (uint8_t t = P_MIN_PIECE; t <= P_MAX_PIECE; ++t) {
        std::string path_white = std::string("../../assets/pngs/") + piece_path_white[t];
        std::string path_black = std::string("../../assets/pngs/") + piece_path_black[t];
        m_piece_tex_black[t] = IMG_LoadTexture(m_renderer, path_black.c_str());
        m_piece_tex_white[t] = IMG_LoadTexture(m_renderer, path_white.c_str());
    }
}

BoardRenderer::~BoardRenderer()
{
    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;

    SDL_DestroyWindow(m_window);
    m_window = nullptr;
}

void BoardRenderer::draw_squares() const
{
    for (int i = 0; i < 64; ++i) {
        int col = i % 8;
        int row = i / 8;
        SDL_Rect rect;
        rect.x = col * 80;
        rect.y = row * 80;
        rect.h = 80;
        rect.w = 80;
        if (row % 2 == col % 2) {
            SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
        } else {
            SDL_SetRenderDrawColor(m_renderer, 100, 100, 100, 255);
        }
        SDL_RenderFillRect(m_renderer, &rect);
    }
}
void BoardRenderer::draw_pieces(const Board &b) const
{
    for (int i = 0; i < 64; ++i) {
        int col = i % 8;
        int row = i / 8;

        Pos pos{ row, col };
        Piece p = b.get_piece_at(pos);
        Color clr = b.get_color_at(pos);

        SDL_Rect rect;
        rect.x = col * 80 + 10;
        rect.y = (7-row) * 80 + 10;
        rect.h = 60;
        rect.w = 60;

        if (p != P_EMPTY) {
            SDL_Texture* tex;
            tex = (clr == C_WHITE) ? m_piece_tex_white[p] : m_piece_tex_black[p];
            SDL_RenderCopy(m_renderer, tex, NULL, &rect);
        }
    }
}

void BoardRenderer::draw_candidates_moves() const
{
    SDL_SetRenderDrawColor(m_renderer, 200, 200, 200, 255);
    for (const auto& m : m_candidates_moves) {
        if (!m.legal) {
            continue;
        }
        int x = m.dst.column * 80 + 40;
        int y = (7 - m.dst.row) * 80 + 40;

        draw_circle(m_renderer, x, y, 20);
    }
}

void BoardRenderer::draw(const Board& board) const
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);

    draw_squares();
    draw_pieces(board);
    draw_candidates_moves();

    SDL_RenderPresent(m_renderer);
}

void BoardRenderer::prepare_player_move(Board &b, SDL_Event &e)
{
    if (m_history_mode) {
        std::cout << "in history mode!\n";
        return;
    }
    int col = e.button.x / 80;
    int row = 7 - (e.button.y / 80);
    Pos pos{ row, col };
    Color clr = b.get_color_at(pos);
    Color to_move = b.get_next_move();

    if (clr == to_move) {
        m_candidates_moves.clear();
        add_move_from_position(b, pos, m_candidates_moves, false);

        for (auto& m : m_candidates_moves)
        {
            b.make_move(m);
            m.legal = !b.is_king_checked(to_move);
            b.unmake_move(m);
        }
        m_need_redraw = true;
    }
}

void BoardRenderer::do_player_move(Board &b, SDL_Event &e)
{
    if (m_history_mode) {
        std::cout << "in history mode!\n";
        return;
    }
    int col = e.button.x / 80;
    int row = 7 - (e.button.y / 80);
    Pos pos{ row, col };

    bool player_move_done = false;
    for (const auto& m : m_candidates_moves) {
        if (m.legal && pos == m.dst) {
            if (m.promote && m.promote_piece != P_QUEEN) {
                continue;
            }
            b.make_move(m);
            m_history.push_back(b);

            player_move_done = true;
            break;
        }
    }
    m_candidates_moves.clear();

    //if (player_move_done) {
    //    draw(b);

    //    auto [found, move] = find_best_move(b);
    //    if (!found) {
    //        std::cout << "no move was found\n";
    //        if (b.is_king_checked(b.get_next_move())) {
    //            std::cout << "because is checkmated\n";
    //        }
    //        else {
    //            std::cout << "pat!\n";
    //        }
    //    }
    //    else {
    //        b.make_move(move);
    //        m_history.push_back(b);
    //    }
    //}
    m_need_redraw = true;
}

void BoardRenderer::main_loop(Board &b)
{
    bool quit = false;
    bool down = false;
    m_history.push_back(b);

    while (!quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e) != 0)
        {
            // User requests quit
            if (e.type == SDL_QUIT)
            {
                quit = true;
                break;
            }

            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                prepare_player_move(b, e);
            }
            else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT)
            {
                do_player_move(b, e);
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r)
            {
                b.load_initial_position();
                m_candidates_moves.clear();
                m_need_redraw = true;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s)
            {
                std::cout << "current position= " << b.get_pos_string() << "\n";
                std::cout << "current number of moves = " << b.get_full_move() << "\n";
                std::cout << "current 50move rules counter = " << b.get_half_move() << "\n";
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_e)
            {
                std::cout << "current eval= " << evaluate_board(b) << "\n";
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_p)
            {
                NegamaxEngine engine;
                engine.do_perft(b, 4);
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_LEFT)
            {
                if (!m_history_mode && m_history.size() > 1) {
                    m_history_mode = true;
                    m_current_history_pos = m_history.size() - 2;
                }
                else if (m_history_mode && m_current_history_pos >= 1) {
                    m_current_history_pos = std::max(m_current_history_pos - 1ull, (uint64_t)0);
                }
                draw(m_history[m_current_history_pos]);
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_RIGHT)
            {
                if (m_history_mode) {
                    m_current_history_pos = std::min(m_current_history_pos + 1, m_history.size()-1);

                    if (m_current_history_pos == m_history.size() - 1) {
                        m_history_mode = false;
                    }
                    draw(m_history[m_current_history_pos]);
                }
            }

            if (m_need_redraw) {
                m_need_redraw = false;
                m_history_mode = false;
                m_current_history_pos = m_history.size() - 1;
                draw(b);
            }
        }
    }
}


#endif // CHESS_ENABLE_SDL
