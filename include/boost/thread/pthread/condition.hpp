// Copyright 2006 Roland Schwarz.
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#ifndef BOOST_THREAD_RS06040707_HPP
#define BOOST_THREAD_RS06040707_HPP

#include <boost/thread/pthread/config.hpp>

#include <boost/thread/exceptions.hpp>
#include <boost/utility.hpp>
#include <boost/thread/pthread/lock.hpp>

#include <pthread.h>

namespace boost {

struct xtime;

// disable warnings about non dll import
// see: http://www.boost.org/more/separate_compilation.html#dlls
#ifdef BOOST_MSVC
#	pragma warning(push)
#	pragma warning(disable: 4251 4231 4660 4275)
#endif

namespace detail {

class BOOST_THREAD_DECL condition_impl : private noncopyable
{
    friend class condition;

public:
    condition_impl();
    ~condition_impl();

    void notify_one();
    void notify_all();

    void do_wait(pthread_mutex_t* pmutex);
    bool do_timed_wait(const xtime& xt, pthread_mutex_t* pmutex);

    pthread_cond_t m_condition;
};

} // namespace detail

class condition : private noncopyable
{
public:
    condition() { }
    ~condition() { }

    void notify_one() { m_impl.notify_one(); }
    void notify_all() { m_impl.notify_all(); }

    template <typename L>
    void wait(L& lock)
    {
        if (!lock)
            throw lock_error();

        do_wait(lock.m_mutex);
    }

    template <typename L, typename Pr>
    void wait(L& lock, Pr pred)
    {
        if (!lock)
            throw lock_error();

        while (!pred())
            do_wait(lock.m_mutex);
    }

    template <typename L>
    bool timed_wait(L& lock, const xtime& xt)
    {
        if (!lock)
            throw lock_error();

        return do_timed_wait(lock.m_mutex, xt);
    }

    template <typename L, typename Pr>
    bool timed_wait(L& lock, const xtime& xt, Pr pred)
    {
        if (!lock)
            throw lock_error();

        while (!pred())
        {
            if (!do_timed_wait(lock.m_mutex, xt))
                return false;
        }

        return true;
    }

private:
    detail::condition_impl m_impl;

    template <typename M>
    void do_wait(M& mutex)
    {

        typedef detail::thread::lock_ops<M>
#if defined(__HP_aCC) && __HP_aCC <= 33900 && !defined(BOOST_STRICT_CONFIG)
# define lock_ops lock_ops_  // HP confuses lock_ops witht the template
#endif
            lock_ops;

        typename lock_ops::lock_state state;
        lock_ops::unlock(mutex, state);

        m_impl.do_wait(state.pmutex);

        lock_ops::lock(mutex, state);
#undef lock_ops
    }

    template <typename M>
    bool do_timed_wait(M& mutex, const xtime& xt)
    {

        typedef detail::thread::lock_ops<M>
#if defined(__HP_aCC) && __HP_aCC <= 33900 && !defined(BOOST_STRICT_CONFIG)
# define lock_ops lock_ops_  // HP confuses lock_ops witht the template
#endif
            lock_ops;

        typename lock_ops::lock_state state;
        lock_ops::unlock(mutex, state);

        bool ret = false;

        ret = m_impl.do_timed_wait(xt, state.pmutex);

        lock_ops::lock(mutex, state);
#undef lock_ops

        return ret;
    }
};

#ifdef BOOST_MSVC
#	pragma warning(pop)
#endif

} // namespace boost

#endif // BOOST_CONDITION_RS06040707_HPP
