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

#ifndef BOOST_TSS_WEK070601_HPP
#define BOOST_TSS_WEK070601_HPP

#include <boost/thread/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#include <boost/utility.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

namespace boost {

class tss : private noncopyable
{
public:
    tss();
    ~tss();

    void* get() const;
    bool set(void* value);

private:
#if defined(BOOST_HAS_WINTHREADS)
    unsigned long m_key;
#elif defined(BOOST_HAS_PTHREADS)
    pthread_key_t m_key;
#endif
};

} // namespace boost

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.

#endif // BOOST_TSS_WEK070601_HPP

