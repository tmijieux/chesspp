
#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>
#include <string>
#include <thread>

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
    std::cout << "id name chesspp 0.0.1 (c)\n";
    std::cout << "id author Thomas Mijieux (c) \n";
}

void send_options()
{
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
    for (int j = i + 1; j < tokens.size(); ++j) {
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
        add_move_from_position(b,  src, ml, false, false);
        bool move_found = false;
        for (auto& m : ml) {
            if (m.dst == dst) {
                move_found = true;
                b.make_move(m);
                break;
            }
        }
        if (!move_found) {
            auto msg = "invalid move for position '" + move + "'";
            throw std::exception(msg.c_str());
        }
    }
}

void uci_main_loop()
{
    Board b;
    NegamaxEngine engine;

    std::thread compute_thread;
    Move best_move;
    bool move_found = false;

    while (true)
    {
        std::string line;
        std::getline(std::cin, line);
        auto tokens = splittrim(line, " ");

        if (tokens.size() == 0) {
            continue;
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
            // set debug mode
        }
        else if (cmd == "isready")
        {
            send_readyok();
        }
        else if (cmd == "setoption")
        {

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
            move_found = false;
            compute_thread = std::thread([&b, &engine, &best_move, &move_found]() {
                engine.iterative_deepening(b, 10, &best_move, &move_found);
            });
        }
        else if (cmd == "stop")
        {
            engine.m_required_stop = true;
            compute_thread.join();
            if (move_found) {
                send_bestmove(best_move);
            }
            else {
                send_nullmove();
            }
        }
        else if (cmd == "ponderhit")
        {

        }
        else if (cmd == "quit")
        {
            exit(0);
        }
    }
}
