#ifndef BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
#define BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
#include <pthread.h>
#include <boost/assert.hpp>

namespace boost
{
    namespace pthread
    {
        class pthread_mutex_scoped_lock
        {
            pthread_mutex_t* m;
        public:
            explicit pthread_mutex_scoped_lock(pthread_mutex_t* m_):
                m(m_)
            {
                int const res=pthread_mutex_lock(m);
                BOOST_ASSERT(!res);
            }
            ~pthread_mutex_scoped_lock()
            {
                int const res=pthread_mutex_unlock(m);
                BOOST_ASSERT(!res);
            }
            
        };
    }
}

#endif
