/*
 *
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
 *   22 May 01  Modified to use xtime for time outs.
 *   23 May 01  Removed "duration" timed_waits, as they are too difficult
 *                  to use with spurious wakeups.
 */
 
#ifndef BOOST_CONDITION_HPP
#define BOOST_CONDITION_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/xtime.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost
{
    class condition : private noncopyable
    {
    public:
        condition();
        ~condition();
        
        void notify_one();
        void notify_all();

        template <typename L>
        void wait(L& lock)
        {
            if (!lock)
                throw lock_error();
            
            do_wait(lock._mutex);
        }

        template <typename L, typename Pr>
        void wait(L& lock, Pr pred)
        {
            if (!lock)
                throw lock_error();
            
            while (!pred())
                do_wait(lock._mutex);
        }

        template <typename L>
        bool timed_wait(L& lock, const xtime& xt)
        {
            if (!lock)
                throw lock_error();
            
            return do_timed_wait(lock._mutex, xt);
        }

        template <typename L, typename Pr>
        bool timed_wait(L& lock, const xtime& xt, Pr pred)
        {
            if (!lock)
                throw lock_error();
            
            while (!pred())
            {
                if (!do_timed_wait(lock._mutex, xt))
                    return false;
            }

            return true;
        }

    private:
        template <typename M>
        void do_wait(M& mutex)
        {
#if defined(BOOST_HAS_WINTHREADS)
            enter_wait();
#endif

            typename M::cv_state state;
            mutex.do_unlock(state);

#if defined(BOOST_HAS_PTHREADS)
            do_wait(state.pmutex);
#elif defined(BOOST_HAS_WINTHREADS)
            do_wait();
#endif

            mutex.do_lock(state);
        }

        template <typename M>
        bool do_timed_wait(M& mutex, const xtime& xt)
        {
#if defined(BOOST_HAS_WINTHREADS)
            enter_wait();
#endif

            typename M::cv_state state;
            mutex.do_unlock(state);

            bool ret = false;

#if defined(BOOST_HAS_PTHREADS)
            ret = do_timed_wait(xt, state.pmutex);
#elif defined(BOOST_HAS_WINTHREADS)
            ret = do_timed_wait(xt);
#endif

            mutex.do_lock(state);

            return ret;
        }

#if defined(BOOST_HAS_WINTHREADS)
        void enter_wait();
        void do_wait();
        bool do_timed_wait(const xtime& xt);
#elif defined(BOOST_HAS_PTHREADS)
        void do_wait(pthread_mutex_t* pmutex);
        bool do_timed_wait(const xtime& xt, pthread_mutex_t* pmutex);
#endif

#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _gate;
        unsigned long _queue;
        unsigned long _mutex;
        unsigned _gone; // # threads that timed out and never made it to the _queue
        long _blocked; // # threads _blocked _waiting for the condition
        unsigned _waiting; // # threads _waiting no longer _waiting for the condition but still
                          //   _waiting to be removed from the _queue
#elif defined(BOOST_HAS_PTHREADS)
        pthread_cond_t _cond;
#endif
    };
} // namespace boost

#endif // BOOST_CONDITION_HPP
