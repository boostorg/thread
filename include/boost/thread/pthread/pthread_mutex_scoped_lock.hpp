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
                BOOST_VERIFY(!pthread_mutex_lock(m));
            }
            ~pthread_mutex_scoped_lock()
            {
                BOOST_VERIFY(!pthread_mutex_unlock(m));
            }
            
        };
    }
}

#endif
