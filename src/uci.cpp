#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <thread>
#include <type_traits>

#include "./move.hpp"
#include "./engine.hpp"
#include "./move_generation.hpp"
#include "./board_renderer.hpp"

#include "./uci.hpp"

using StringList = std::vector<std::string>;


inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {
        return !std::isspace(c);
    }));
    return s;
}

inline std::string &rtrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int c) {
        return !std::isspace(c);
    }));
    return s;
}

inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}


void tokenize(const std::string  &str, const std::string &delim,
              std::vector<std::string> &out)
{
    size_t start;
    size_t end = 0;

    while ((start = str.find_first_not_of(delim, end)) != std::string::npos)
    {
        end = str.find(delim, start);
        auto str2 = str.substr(start, end - start);
        out.push_back(trim(str2));
    }
}

inline std::vector<std::string> splittrim(const std::string& s, const std::string &delim)
{
    std::vector<std::string> res;
    tokenize(s, delim, res);
    return res;
}

void send_id()
{
    uci_send("id name Tistou Chess 0.0.1 (c)\n");
    uci_send("id author Thomas Mijieux\n");
}

void send_options()
{
    uci_send("option name UCI_Opponent type string\n");
    uci_send("option name UCI_EngineAbout type string default Tistou Chess by Thomas Mijieux. see https://github.com/tmijieux/chesspp\n");
    uci_send("option name UCI_AnalyseMode type check default false\n");
}

void send_uciok()
{
    uci_send("uciok\n");
}

void send_readyok()
{
    uci_send("readyok\n");
}


inline constexpr char get_char_by_piece(Piece p)
{
    switch (p) {
    case P_PAWN: return 'p';
    case P_ROOK: return 'r';
    case P_BISHOP: return 'b';
    case P_KNIGHT: return 'n';
    case P_QUEEN: return 'q';
    case P_KING: return 'k';
    case P_EMPTY: return ' ';
    default: return 'X';
    }
}

void console_draw(const Board& board)
{
    std::cout << "\n\n";
    for (uint8_t row = 0; row < 8; ++row)
    {
        for (uint8_t col = 0; col < 8; ++col)
        {
            Pos pos{ uc(7-row), col };
            Piece p = board.get_piece_at(pos);
            Color clr = board.get_color_at(pos);

            char c = get_char_by_piece(p);
            if (clr == C_WHITE) {
                c = c + ('A' - 'a');
            }
            std::cout << c << " ";
        }
        std::cout << "\n";
    }

    std::cout << "\nFen: " << board.get_pos_string() << "\n";
    std::cout << "Key: " << board.get_key_string() << "\n\n";

}

void uci_send_bestmove(const Move &m)
{
    uci_send("bestmove {}\n", move_to_uci_string(m));
}

void send_nullmove()
{
    uci_send("bestmove 0000\n");
}

MoveList parse_moves(Board& b, const StringList& tokens, size_t begin, size_t end, bool apply_to_board)
{
    MoveList collected_moves;

    for (auto j = begin; j < end; ++j)
    {
        const auto& move = tokens[j];
        if (move.size() != 4 && move.size() != 5) {
            auto msg = "invalid move '" + move + "'";
            throw chess_exception(msg);
        }
        std::string s_src = move.substr(0, 2);
        std::string s_dst = move.substr(2, 2);
        bool promote = move.size() == 5;
        std::string s_promote = move.substr(4, 1);

        Pos src = square_name_to_pos(s_src);
        Pos dst = square_name_to_pos(s_dst);
        MoveList ml;
        add_move_from_position(b, src, ml, false, false);
        bool move_found = false;
        for (auto& m : ml) {
            if (m.dst == dst
                && m.promote == promote
                && (!promote || get_char_by_piece(m.promote_piece)==s_promote[0])) {
                move_found = true;
                collected_moves.push_back(m);
                if (apply_to_board) {
                    b.make_move(m);
                }
                break;
            }
        }
        if (!move_found) {
            auto msg = "invalid move for position '" + move + "'";
            throw chess_exception(msg);
        }
    }
    return collected_moves;
}

template<typename I>
I read_integer(StringList&tokens, size_t &i)
{
    if (i >= tokens.size() - 1) {
        throw chess_exception("invalid params");
    }
    size_t pos;
    I val{ 0 };
    if constexpr (std::is_same<I, uint32_t>::value) {
        val = std::stoul(tokens[i + 1], &pos);
    }
    else {
        val = std::stoull(tokens[i + 1], &pos);
    }
    if (pos == 0) {
        val = 0;
    }
    i += 2;
    return val;
}

GoParams parse_go_params(Board &b, StringList& tokens)
{
    GoParams p;

    size_t i = 1;
    while (i < tokens.size())
    {
        auto& cmd = tokens[i];
        if (cmd == "infinite")
        {
            p.infinite = true;
            p.depth = 1000;
            ++i;
        }
        else if (cmd == "ponder")
        {
            p.ponder = true;
            ++i;
        }
        else if (cmd == "mate")
        {
            p.mate = read_integer<uint32_t>(tokens, i);
        }
        else if (cmd == "depth")
        {
            p.depth = read_integer<uint32_t>(tokens, i);
        }
        else if (cmd == "movestogo")
        {
            p.movestogo = read_integer<uint32_t>(tokens, i);
        }
        else if (cmd == "movetime")
        {
            p.movetime = read_integer<uint64_t>(tokens, i);
        }
        else if (cmd == "wtime")
        {
            p.wtime = read_integer<uint64_t>(tokens, i);
        }
        else if (cmd == "btime")
        {
            p.btime = read_integer<uint64_t>(tokens, i);
        }
        else if (cmd == "winc")
        {
            p.wincrement = read_integer<uint64_t>(tokens, i);
        }
        else if (cmd == "binc")
        {
            p.bincrement = read_integer<uint64_t>(tokens, i);
        }
        else if (cmd == "searchmoves")
        {
            p.searchmoves = parse_moves(b, tokens, i+1, tokens.size(), false);
            i = tokens.size();
        }
        else
        {
            ++i;
        }
    }
    return p;
}


void handle_position_cmd(Board &b,const StringList &tokens)
{
    if (tokens.size() < 2) {
        throw chess_exception("invalid position cmd ???");
    }
    const auto &cmd = tokens[1];
    size_t i = 2;
    if (cmd == "fen") {
        StringList fenpos_tokens;
        while (true)
        {
            if (i >= tokens.size() || tokens[i] == "moves") {
                break;
            }
            fenpos_tokens.push_back(tokens[i]);
            ++i;
        }
        auto fenpos = fmt::format("{}", fmt::join(fenpos_tokens, " "));
        b.load_position(fenpos);
    } else if (cmd == "startpos") {
        b.load_initial_position();
    } else {
        std::string msg = "invalid subcmd for position '" + cmd + "'\n";
        throw chess_exception(msg);
    }

    if (i >= tokens.size() - 1 || tokens[i] != "moves") {
        return;
    }
    parse_moves(b, tokens, i + 1, tokens.size(), true);
}

void print_help()
{
    std::cout << fmt::format(
        "List of commands \n\n"
        " UCI:  \n\n"
        " - uci \n"
        " - ucinewgame \n"
        " - setoption \n"
        " - position \n"
        " - stop \n"
        " - go \n"
        " - isready \n"
        " - quit \n"

        "\n\n "
        " Other commands:  \n\n"

        " - perft [n]\n"
        " - display \n"
        " - evaluate\n"
        " - init  : Load initial position\n"
        " - ui  : open SDL UI\n"
        " - help  : display this help message\n"
        "\n"
    ) << std::flush;
}

int try_handle_one_command(
    StringList& tokens, bool& debug, bool &move_found,
    Move &best_move, Board &b, NegamaxEngine &engine)
{
    if (tokens.size() == 0) {
        return 0;
    }
    const auto &cmd = tokens[0];

    if (cmd == "uci")
    {
        send_id();
        send_options();
        send_uciok();
        engine.init_hash();

    }
    else if (cmd == "debug")
    {
        if (!debug && tokens.size() >= 2 && tokens[1] == "on") {
            uci_send_info_string("debug mode ON!");
            debug = true;
        }
        else if (debug && tokens.size() >= 2 && tokens[1] == "off") {
            uci_send_info_string("debug mode OFF!");
            debug = false;
        }
        // set debug mode
    }
    else if (cmd == "isready")
    {
        send_readyok();
    }
    else if (cmd == "setoption")
    {
        if (tokens.size() < 5) {
            return 0;
        }
        if (tokens[1] != "name" ){
            return 0;
        }
        StringList var_name_tokens;
        StringList var_value_tokens;
        int i = 2;
        while (i < tokens.size() && tokens[i] != "value") {
            var_name_tokens.push_back(tokens[i]);
            ++i;
        }
        if (i >= tokens.size()) {
            return 0;
        }
        ++i;
        while (i < tokens.size()) {
            var_value_tokens.push_back(tokens[i]);
            ++i;
        }
        std::string varname = fmt::format("{}",fmt::join(var_name_tokens, " "));
        std::string val = fmt::format("{}",fmt::join(var_value_tokens, " "));
        uci_send_info_string(fmt::format("option '{}' set to '{}'", varname, val));
    }
    else if (cmd == "ucinewgame")
    {

    }
    else if (cmd == "position")
    {
        handle_position_cmd(b, tokens);
    }
    else if (cmd == "go")
    {
        if (engine.is_running()) {
            // invalid go while already running
            engine.stop();
            return 0;
        }
        GoParams params = parse_go_params(b, tokens);
        engine.set_uci_mode(true, params);
        engine.start_uci_background(b);
    }
    else if (cmd == "stop")
    {
        engine.stop();
    }
    else if (cmd == "ponderhit")
    {

    }
    else if (cmd == "quit" || cmd == "q")
    {
        exit(0);
    }
    else if (cmd == "d" || cmd == "display")
    {
        console_draw(b);
    }
    else if (cmd == "e" || cmd == "eval" || cmd == "evaluate")
    {
        int32_t val = evaluate_board(b);
        std::cout << "Evaluation=" << val <<"\n";
    }
    else if (cmd == "perft")
    {
        size_t i = 0;
        int num = read_integer<uint32_t>(tokens, i);
        if (num > 0) {
            engine.do_perft(b, num);
        }
    }
    else if (cmd == "ui")
    {
        BoardRenderer r;
        r.main_loop(b);
    }
    else if (cmd == "init")
    {
        b.load_initial_position();
    }
    else if (cmd == "clearhash")
    {
        engine.clear_hash();
    }
    else if (cmd == "help" || cmd == "h" || cmd == "commands")
    {
        print_help();
    }
    else
    {
        if (debug) {
            uci_send_info_string(fmt::format("unrecognized command `{}`", cmd));
        }
        return 1;
    }
    return 0;
}

void uci_main_loop()
{
    Board b;
    NegamaxEngine engine;
    b.load_initial_position(); // not supposed to do this in uci but for now idontcare

    bool debug = false;
    Move best_move;
    bool move_found = false;

    while (true)
    {
        std::string line;
        std::getline(std::cin, line);
        auto tokens = splittrim(line, " ");
        bool cmd_handled = false;
        while (!cmd_handled)
        {
            int res = try_handle_one_command(
                tokens, debug, move_found,
                best_move, b, engine
            );
            if (res == 0) {
                cmd_handled = true;
            }
            else {
                tokens.erase(tokens.begin());
            }
        }
    }
}
