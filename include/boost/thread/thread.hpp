// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.  
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_THREAD_WEK070601_HPP
#define BOOST_THREAD_WEK070601_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/thread/exceptions.h>

#include <boost/function.hpp>
#include <boost/utility.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <list>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost {

struct xtime;

class thread : private noncopyable
{
public:
    thread();
    thread(const function0<void>& threadfunc);
    ~thread();

    bool operator==(const thread& other) const;
    bool operator!=(const thread& other) const;

    void join();
    bool try_join();
    bool timed_join(const xtime& xt);

    static void sleep(const xtime& xt);
    static void yield();

private:
#if defined(BOOST_HAS_WINTHREADS)
    unsigned long m_thread;
    unsigned int m_id;
#elif defined(BOOST_HAS_PTHREADS)
public: // pdm: sorry about this, I'm getting an error in libs/boost/src/thread.cpp - you can work out how to best fix it ;)
    class thread_list;
private:
    friend class thread_list;

    pthread_t m_thread;
    mutex m_mutex;
    condition m_condition;
    bool m_alive;
    thread_list* m_state_manager;
#endif
};

class thread_group : private noncopyable
{
public:
    thread_group();
    ~thread_group();

    thread* create_thread(const function0<void>& threadfunc);
    void add_thread(thread* thrd);
    void remove_thread(thread* thrd);
    void join_all();

private:
    std::list<thread*> m_threads;
    mutex m_mutex;
};

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//    1 Jun 01  WEKEMPF Added boost::thread initial implementation.
//    3 Jul 01  WEKEMPF Redesigned boost::thread to be noncopyable.

#endif // BOOST_THREAD_WEK070601_HPP
