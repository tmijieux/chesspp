#ifndef CHESS_TIMER_H
#define CHESS_TIMER_H


#ifndef _WIN32
#include <sys/time.h>

#endif // #ifndef _WIN32

#include <cstring>
#include <iostream>

#include "./types.hpp"


#define TIMEVAL_DIFF(a,b) ( ((a).tv_sec - (b).tv_sec) + ( ((a).tv_usec - (b).tv_usec) / 1000000.0) )


#ifdef _WIN32


//// MSVC defines this in winsock2.h!?
typedef struct _timeval {
    long tv_sec;
    long tv_usec;
} my_timeval_t;

int gettimeofday(my_timeval_t* tp, struct timezone* tzp);
//
#else
using  my_timeval_t = timeval ;


#endif


class Timer
{
private:
    bool m_running;
    double m_length;
    my_timeval_t m_last_start;
public:
    Timer():
        m_running(false),
        m_length(0.0)
    {
    }

    void reset()
    {
        memset(&m_last_start, 0, sizeof m_last_start);
        m_running = false;
        m_length = 0.0;
    }

    void start()
    {
        if (m_running)
            throw chess_exception("timer is already started");

        gettimeofday(&m_last_start, NULL);
        m_running = true;
    }

    void stop()
    {
        if (!m_running)
            throw chess_exception("timer is not running");
        my_timeval_t tsp;
        gettimeofday(&tsp, NULL);
        m_length += TIMEVAL_DIFF(tsp, m_last_start);
        m_running = false;
    }

    double get_length()
    {
        if (m_running) {
            my_timeval_t tsp;
            gettimeofday(&tsp, NULL);
            double length = m_length + TIMEVAL_DIFF(tsp, m_last_start);
            return length;
        }
        return m_length;
    }

    double get_micro_length()
    {
        return get_length() * 1000000.0;
    }


    void print_and_restart(const std::string &label)
    {
        stop();
        std::cerr  << label <<" =\t" << get_length() << "\n";
        reset();
        start();
    }

}; // class Timer


class SmartTime
{
private:
    Timer &m_timer;
public:
    SmartTime(Timer &timer):
        m_timer(timer)
    {
        m_timer.start();
    }

    ~SmartTime()
    {
        m_timer.stop();
    }
}; // class SmartTime



#endif // CHESS_TIMER_H

