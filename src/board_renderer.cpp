#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"

#include "./board_renderer.hpp"
#include "./move_generation.hpp"
#include "./engine.hpp"
#include "./dialog.hpp"
#include "./pgn.hpp"


void BoardRenderer::init_text(SDL_Renderer *renderer)
{
    TTF_Init();
    //this opens a font style and sets a size

    std::string path_ttf = std::string("../assets/fonts/VeraMono.ttf");
#ifdef WIN32
    path_ttf = "../"+ path_ttf;
#endif

    const char *font_file = path_ttf.c_str();
    TTF_Font* sansFont = TTF_OpenFont(font_file, 24);
    if (sansFont == nullptr) {
        std::cerr<<"could not find font "<< font_file<<"\n";
        exit(1);
    }

    // this is the color in rgb format,
    // maxing out all would give you the color white,
    // and it will be your text's color

    // as TTF_RenderText_Solid could only be used on
    // SDL_Surface then you have to create the surface first

    const char *mytext =  "abcdefgh12345678";
    TTF_SizeText(sansFont, mytext, &m_text_width, &m_text_height);

    SDL_Color white = {0xea, 0xe9, 0xd2};
    SDL_Surface *surfaceMessage =  TTF_RenderText_Solid(sansFont, mytext, white);
    m_alphanum_texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    SDL_FreeSurface(surfaceMessage);

    SDL_Color blue = {0x4b, 0x73, 0x99};
    surfaceMessage =  TTF_RenderText_Solid(sansFont, mytext, blue);
    m_alphanum_black_texture = SDL_CreateTextureFromSurface(renderer, surfaceMessage);
    SDL_FreeSurface(surfaceMessage);
}


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

int piece_to_atlaspos(Piece p)
{
    switch (p) {
    case P_PAWN:   return 0;
    case P_BISHOP: return 1;
    case P_QUEEN:  return 2;
    case P_KING:   return 3;
    case P_KNIGHT: return 4;
    case P_ROOK:   return 5;
    default:       return 0;
    }
}



BoardRenderer::BoardRenderer() :
    m_window(nullptr),
    m_renderer(nullptr),
    m_piece_tex{ nullptr },
    m_move_event{ 0 },
    m_history_mode{ false },
    m_current_history_pos{ 0 },
    m_need_redraw{ true },
    m_flipped_board{false}
{
    SDL_Init(SDL_INIT_VIDEO);
    m_window = SDL_CreateWindow("chesspp", 100, 100, 640, 640, 0);
    m_renderer = SDL_CreateRenderer(m_window, -1, SDL_RENDERER_ACCELERATED);
    IMG_Init(IMG_INIT_PNG);

    std::memset(m_piece_pos, 0, sizeof(m_piece_pos));
    for (int i = 0; i < 12; ++i) {
        m_piece_pos[i].w = 600/6;
        m_piece_pos[i].h = 237/2;

        m_piece_pos[i].x = (i % 6) * (600/6);
        m_piece_pos[i].y = (i / 6) * (237/2);

        if (i%6 == piece_to_atlaspos(P_PAWN)) {
            m_piece_pos[i].w -= 25;
        } else if (i%6 == piece_to_atlaspos(P_QUEEN)) {
            m_piece_pos[i].x -= 5;
            m_piece_pos[i].w += 10;
        } else if (i%6 == piece_to_atlaspos(P_KING)) {
            m_piece_pos[i].x += 15;
        } else if (i%6 == piece_to_atlaspos(P_BISHOP)) {
            m_piece_pos[i].x -= 15;
        } else if (i%6 == piece_to_atlaspos(P_KNIGHT)) {
            m_piece_pos[i].x += 15;
        } else if (i%6 == piece_to_atlaspos(P_ROOK)) {
            m_piece_pos[i].x += 15;
        }
        if (i/6 == 0) {
            // blackpiece
            m_piece_pos[i].h -= 25;
            //m_piece_pos[i].y -= 25;
        } else {
            m_piece_pos[i].y += 25;
        }
    }


    init_text(m_renderer);

    // custom events
    m_move_event = SDL_RegisterEvents(1);

    SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "2" );
    std::string path_tex = std::string("../assets/pngs/piece-atlas.png");
#ifdef WIN32
    path_tex = "../"+path_tex;
#endif
    m_piece_tex = IMG_LoadTexture(m_renderer, path_tex.c_str());



}

BoardRenderer::~BoardRenderer()
{
    SDL_DestroyTexture(m_piece_tex);
    m_piece_tex = nullptr;

    SDL_DestroyTexture(m_alphanum_texture);
    SDL_DestroyTexture(m_alphanum_black_texture);
    m_alphanum_texture = nullptr;
    m_alphanum_black_texture = nullptr;

    SDL_DestroyRenderer(m_renderer);
    m_renderer = nullptr;

    SDL_DestroyWindow(m_window);

    m_window = nullptr;
    SDL_Quit();
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
            // #eae9d2
            SDL_SetRenderDrawColor(m_renderer, 0xea, 0xe9, 0xd2, 0xFF);
        } else {
            // blue
            SDL_SetRenderDrawColor(m_renderer, 0x4B, 0x73, 0x99, 0xFF);
        }
        SDL_RenderFillRect(m_renderer, &rect);
    }
}

void BoardRenderer::draw_pieces(const Board &b) const
{
    for (uint8_t i = 0; i < 64; ++i) {
        Pos pos{ i };
        Piece p = b.get_piece_at(pos);
        Color clr = b.get_color_at(pos);

        if (p != P_EMPTY) {
            if (m_flipped_board) {
                pos.column = 7 - pos.column;
                pos.row = 7 - pos.row;
            }

            const SDL_Rect *src = &m_piece_pos[piece_to_atlaspos(p) + 6 * clr];

            SDL_Rect rect;
            rect.x = pos.column * 80 + 10;
            rect.y = (7-pos.row) * 80 + 10;
            rect.h = 60;
            rect.w = 60;
            // if (clr == C_BLACK) {
                rect.h -= (int) ((double)(25/(237/2))  * 60) ;
            // }

            SDL_RenderCopy(m_renderer, m_piece_tex, src, &rect);
        }
    }
}

void BoardRenderer::draw_letters() const
{
    int tw = m_text_width/16;
    int th = m_text_height;
    int margin = 5;

    for (int i = 0; i < 8; ++i) {
        //file letters
        int k = i;
        if (m_flipped_board) {
            k = 7 - k;
        }


        SDL_Rect rect_src;
        rect_src.x = k * tw;
        rect_src.y = 0;
        rect_src.w = tw;
        rect_src.h = th;

        SDL_Rect rect_dst;
        rect_dst.w = tw;
        rect_dst.h = th;
        rect_dst.x = (i+1) * 80 - margin - tw;
        rect_dst.y = 7*80 + 80 - margin - th;

        SDL_Texture *tex = k % 2 == m_flipped_board ? m_alphanum_texture : m_alphanum_black_texture;

        SDL_RenderCopy(m_renderer, tex, &rect_src, &rect_dst);
    }

    for (int i = 0; i < 8; ++i) {
        int k = i;
        if (m_flipped_board) {
            k = 7 - k;
        }
        //row numbers

        SDL_Rect rect_src; //create a rect
        rect_src.x = (8 + k) * tw;
        rect_src.y = 0;
        rect_src.w = tw;
        rect_src.h = th;

        SDL_Rect rect_dst;
        rect_dst.w = tw;
        rect_dst.h = th;
        rect_dst.x = 10;
        rect_dst.y = (7-i) * 80 + margin;

        SDL_Texture *tex = k % 2 == m_flipped_board ? m_alphanum_texture : m_alphanum_black_texture;
        SDL_RenderCopy(m_renderer, tex, &rect_src, &rect_dst);

    }
}

void BoardRenderer::draw_candidates_moves() const
{
    SDL_SetRenderDrawColor(m_renderer, 100, 240, 100, 0xFF);
    for (const auto& m : m_candidates_moves) {
        if (!m.legal) {
            continue;
        }
        Pos p = m.dst;
        if (m_flipped_board) {
            p.column = 7 - p.column;
            p.row = 7 - p.row;
        }
        int x = p.column * 80 + 40;
        int y = (7 - p.row) * 80 + 40;

        draw_circle(m_renderer, x, y, 21);
        draw_circle(m_renderer, x, y, 20);
        draw_circle(m_renderer, x, y, 19);

    }
}

void BoardRenderer::draw(const Board& board) const
{
    SDL_SetRenderDrawColor(m_renderer, 255, 255, 255, 255);
    SDL_RenderClear(m_renderer);

    draw_squares();
    draw_pieces(board);
    draw_candidates_moves();
    draw_letters();

    SDL_RenderPresent(m_renderer);
}

void BoardRenderer::prepare_player_move(Board &b, SDL_Event &e)
{
    if (m_history_mode) {
        std::cout << "in history mode!\n";
        return;
    }
    uint8_t col = e.button.x / 80;
    uint8_t row = 7 - (e.button.y / 80);
    if (m_flipped_board) {
        row = 7 - row;
        col = 7 - col;
    }

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
    uint8_t col = e.button.x / 80;
    uint8_t row = 7 - (e.button.y / 80);
    if (m_flipped_board) {
        row = 7 - row;
        col = 7 - col;
    }
    Pos pos{ row, col };

    bool player_move_done = false;
    for (const auto & m : m_candidates_moves) {
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
        SDL_Event e = {0};
        while (SDL_WaitEvent(&e) != 0)
        {
            // User requests quit
            if (e.type == SDL_QUIT || e.type == SDL_APP_TERMINATING)
            {
                std::cout << "quitting UI!\n";
                quit = true;
                break;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_q)
            {
                std::cout << "quitting UI!\n";
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
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_f)
            {
                flip_board();
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_r)
            {
                b.load_initial_position();
                m_candidates_moves.clear();
                m_history.clear();
                m_history.push_back(b);
                m_current_history_pos = 0;
                m_history_mode = false;
                m_need_redraw = true;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_s)
            {
                std::cout << "\n\nFen:  " << b.get_pos_string() << "\n";
                std::cout << "Key:  " << b.get_key_string() << "\n";
                std::cout << "Number of moves: " << b.get_full_move() << "\n";
                std::cout << "50 move rules counter: " << (int)b.get_half_move() << "\n\n";
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_e)
            {
                std::cout << "Evaluation: " << evaluate_board(b) << "\n";
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
                    m_current_history_pos = std::max(m_current_history_pos - 1, (uint64_t)0);
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
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_o)
            {
                std::string file_path;
                if (find_file_path_dialog(file_path))
                {
                    std::cout << "file_path = " << file_path << "\n";
                    std::ifstream file{ file_path};
                    std::stringstream buf;
                    buf << file.rdbuf();
                    std::string content = buf.str();
                    MoveList move_history;

                    try
                    {
                        pgn::load_pgn_file(content, b, move_history);

                    }
                    catch (chess_exception& e)
                    {
                        std::cout << "error while loading pgn file=" << e.what() << "\n";
                    }

                    for (auto& m : move_history)
                    {
                        std::cout << "move=" << move_to_string(m) << "\n";
                    }

                }
            }
            else if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.scancode >=  SDL_SCANCODE_KP_1
                    && e.key.keysym.scancode <=  SDL_SCANCODE_KP_9) {
                    int q  = e.key.keysym.scancode - SDL_SCANCODE_KP_1 + 1;
                    std::cout << "q="<<q<<"\n";
                    load_test_position(b, q);
                    m_need_redraw = true;
                    m_history.clear();
                    m_history.push_back(b);
                    m_current_history_pos = 0;
                    m_history_mode = false;

                }
            }
            //else if (e.type == SDL_EventType::SDL_VIDEOEXPOSE)
            else if (e.type == SDL_MOUSEMOTION)
            {
            }
            else if (e.type == SDL_WINDOWEVENT || e.type == SDL_DISPLAYEVENT)
            {
                m_need_redraw = true;
            }

            if (m_need_redraw)
            {
                m_need_redraw = false;
                m_history_mode = false;
                m_current_history_pos = m_history.size() - 1;
                draw(b);
            }
        }
    }
}



