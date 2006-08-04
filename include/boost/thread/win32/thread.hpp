// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06041007_HPP
#define BOOST_THREAD_RS06041007_HPP

#include <boost/thread/win32/config.hpp>


#include <boost/utility.hpp>
#include <boost/function.hpp>

#include <boost/thread/win32/mutex.hpp>
#include <list>
#include <memory>

namespace boost {

struct xtime;

// disable warnings about non dll import
// see: http://www.boost.org/more/separate_compilation.html#dlls
#ifdef BOOST_MSVC
#	pragma warning(push)
#	pragma warning(disable: 4251 4231 4660)
#endif

class BOOST_THREAD_DECL thread : private noncopyable
{
public:
    thread();
    explicit thread(const function0<void>& threadfunc);
    ~thread();

    bool operator==(const thread& other) const;
    bool operator!=(const thread& other) const;

    void join();

    static void sleep(const xtime& xt);
    static void yield();

private:
    void* m_thread;
    unsigned int m_id;
    bool m_joinable;
};

class BOOST_THREAD_DECL thread_group : private noncopyable
{
public:
    thread_group();
    ~thread_group();

    thread* create_thread(const function0<void>& threadfunc);
    void add_thread(thread* thrd);
    void remove_thread(thread* thrd);
    void join_all();
        int size();

private:
    std::list<thread*> m_threads;
    mutex m_mutex;
};

#ifdef BOOST_MSVC
#	pragma waring(pop)
#endif

} // namespace boost

#endif // BOOST_THREAD_RS06041007_HPP
