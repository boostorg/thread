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
 */
 
#ifndef BOOST_SEMAPHORE_HPP
#define BOOST_SEMAPHORE_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/xtime.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost {
    class semaphore : private noncopyable
    {
    public:
        explicit semaphore(unsigned count=0, unsigned max=0);
        ~semaphore();
        
        bool up(unsigned count=1, unsigned* prev=0);
        void down();
        bool down(const xtime& xt);
        
    private:
#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _sema;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_mutex_t _mutex;
        pthread_cond_t _cond;
        unsigned _available;
        unsigned _max;
#endif
    };
} // namespace boost

#endif // BOOST_SEMAPHORE_HPP
