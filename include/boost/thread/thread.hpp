/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *    8 Feb 01  Initial version.
 *    1 Jun 01  Added boost::thread initial implementation.
 */
 
#ifndef BOOST_THREAD_HPP
#define BOOST_THREAD_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/thread/xtime.hpp>
#include <boost/function.hpp>
#include <stdexcept>

#if defined(BOOST_HAS_PTHREADS)
    struct timespec;
#endif

namespace boost
{
    namespace detail
    {
        class thread_state;
//        typedef function<int> threadfunc;
        typedef void (*threadfunc)(void* param);
    }

    class lock_error : public std::runtime_error
    {
    public:
        lock_error();
    };

    class thread
    {
    public:
        thread() : _state(0) { }
        thread(const thread& other);
        ~thread();

        thread& operator=(const thread& other)
        {
            thread temp(other);
            swap(temp);
            return *this;
        }
        thread& swap(thread& other)
        {
            detail::thread_state* temp = other._state;
            other._state = _state;
            _state = temp;
            return *this;
        }

        bool operator==(const thread& other) { return _state == other._state; }
        bool operator!=(const thread& other) { return _state != other._state; }

        bool is_alive() const;
        void join();

        static thread create(const detail::threadfunc& func, void* param=0);
        static thread self();

        static void join_all();
        static void sleep(const xtime& xt);
        static void yield();

    private:
        detail::thread_state* _state;
    };
} // namespace boost

#endif // BOOST_THREAD_HPP
