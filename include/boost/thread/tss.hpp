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
 *   6 Jun 01  Initial version.
 */
 
#ifndef BOOST_TSS_HPP
#define BOOST_TSS_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost
{
    class tss
    {
    public:
        tss();
        ~tss();

        void* get() const;
        bool set(void* value);

    private:
#if defined(BOOST_HAS_WINTHREADS)
        unsigned long _key;
#elif defined(BOOST_HAS_PTHREADS)
        pthread_key_t _key;
#endif
    };
}

#endif // BOOST_TSS_HPP