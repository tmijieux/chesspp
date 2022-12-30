
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <thread>
#include <type_traits>

#include <fmt/format.h>

#include "./move.hpp"
#include "./engine.hpp"
#include "./move_generation.hpp"


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

void send_command(std::string cmd)
{

}

void send_id()
{
    std::cout << "id name Tistou Chess 0.0.1 (c)\n";
    std::cout << "id author Thomas Mijieux\n";
}

void send_options()
{
    std::cout << "option name UCI_Opponent type string\n";
    std::cout << "option name UCI_EngineAbout type string default Tistou Chess by Thomas Mijieux. see https://github.com/tmijieux/chesspp\n";
    std::cout << "option name UCI_AnalyseMode type check default false\n";
}

void send_uciok()
{
    std::cout << "uciok\n";
}

void send_readyok()
{
    std::cout << "readyok\n";
}


void send_bestmove(const Move &m)
{
    std::cout << "bestmove " << move_to_uci_string(m) << "\n";
}

void send_nullmove()
{
    std::cout << "bestmove 0000\n";
}

struct GoParams {
    bool infinite;
    bool ponder;
    MoveList searchmoves;
    uint64_t wtime;
    uint64_t btime;
    uint64_t wincrement;
    uint64_t bincrement;
    uint32_t movestogo;
    uint32_t mate;
    uint32_t depth;

    GoParams() :
        infinite{ false },
        ponder{ false },
        wtime{ 0 },
        btime{ 0 },
        wincrement{ 0 },
        bincrement{ 0 },
        movestogo{ 0 },
        mate{ 0 },
        depth{ 0 }
    {
    }
};

MoveList parse_moves(Board& b, const StringList& tokens, int begin, int end, bool apply_to_board)
{
    MoveList collected_moves;

    for (int j = begin; j < end; ++j)
    {
        const auto& move = tokens[j];
        if (move.size() != 4) {
            auto msg = "invalid move '" + move + "'";
            throw std::exception(msg.c_str());
        }
        std::string s_src = move.substr(0, 2);
        std::string s_dst = move.substr(2, 2);
        Pos src = square_name_to_pos(s_src);
        Pos dst = square_name_to_pos(s_dst);
        MoveList ml;
        add_move_from_position(b, src, ml, false, false);
        bool move_found = false;
        for (auto& m : ml) {
            if (m.dst == dst) {
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
            throw std::exception(msg.c_str());
        }
    }
    return collected_moves;
}

template<typename I>
I read_integer(StringList&tokens, size_t &i)
{
    if (i >= tokens.size() - 1) {
        throw std::exception("invalid params");
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
        throw std::exception("invalid position cmd ???");
    }
    auto cmd = tokens[1];
    int i = 2;
    if (cmd == "fen") {
        StringList fenpos_tokens;
        while (true)
        {
            if (i >= tokens.size() || tokens[i] == "moves"){
                break;
            }
            fenpos_tokens.push_back(tokens[i]);
            ++i;
        }
        auto fenpos = fmt::format("{}", fmt::join(fenpos_tokens, " "));
        b.load_position(fenpos);
    } else if (cmd == "startpos") {
        b.load_initial_position();
    }
    else {
        std::string msg = "invalid subcmd for position '" + cmd + "'\n";
        throw std::exception(msg.c_str());
    }

    if (i >= tokens.size() - 1 || tokens[i] != "moves") {
        return;
    }
    parse_moves(b, tokens, i + 1, tokens.size(), true);
}

void send_info_string(const std::string& info){
    auto lines = splittrim(info, "\n");
    for (const auto &line : lines) {
        std::cout << fmt::format("info string {}\n", line);
    }
}

int try_handle_one_command(
    StringList& tokens, bool& debug, bool &move_found,
    Move &best_move, Board &b, NegamaxEngine &engine, std::thread &compute_thread)
{
    if (tokens.size() == 0) {
        return 0;
    }
    auto cmd = tokens[0];

    if (cmd == "uci")
    {
        send_id();
        send_options();
        send_uciok();
    }
    else if (cmd == "debug")
    {
        if (!debug && tokens.size() >= 2 && tokens[1] == "on") {
            send_info_string("debug mode ON!");
            debug = true;
        }
        else if (debug && tokens.size() >= 2 && tokens[1] == "off") {
            send_info_string("debug mode OFF!");
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
        if (i >= tokens.size()){
            return 0;
        }
        ++i;
        while (i < tokens.size()) {
            var_value_tokens.push_back(tokens[i]);
            ++i;
        }
        std::string varname = fmt::format("{}",fmt::join(var_name_tokens, " "));
        std::string val = fmt::format("{}",fmt::join(var_value_tokens, " "));
        send_info_string(fmt::format("option '{}' set to '{}'", varname, val));
    }
    else if (cmd == "register")
    {

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
        GoParams params = parse_go_params(b, tokens);
        if (compute_thread.joinable()) { compute_thread.detach(); }

        compute_thread = std::thread{ [&b, &engine, &best_move, &move_found, &compute_thread]() {
            move_found = false;
            engine.iterative_deepening(b, 6, &best_move, &move_found);
            if (move_found) {
                send_bestmove(best_move);
                move_found = false;
            }
            else {
                send_nullmove();
            }
            compute_thread.detach();
            engine.m_required_stop = false;
        } };
    }
    else if (cmd == "stop")
    {
        if (compute_thread.joinable()) {
            engine.m_required_stop = true;
            compute_thread.join();
        }
    }
    else if (cmd == "ponderhit")
    {

    }
    else if (cmd == "quit")
    {
        exit(0);
    }
    else
    {
        if (debug) {
            send_info_string(fmt::format("unrecognized command `{}`", cmd));
        }
        return 1;
    }
    return 0;
}

void uci_main_loop()
{
    Board b;
    NegamaxEngine engine;

    bool debug = false;
    std::thread compute_thread;
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
                best_move, b, engine, compute_thread
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
