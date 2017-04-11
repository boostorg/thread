#ifndef BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
#define BOOST_PTHREAD_MUTEX_SCOPED_LOCK_HPP
//  (C) Copyright 2007-8 Anthony Williams 
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <pthread.h>
#include <boost/assert.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace pthread
    {
        class pthread_mutex_scoped_lock
        {
            pthread_mutex_t* m;
            bool locked;
        public:
            explicit pthread_mutex_scoped_lock(pthread_mutex_t* m_):
                m(m_),locked(true)
            {
                int error_code = pthread_mutex_lock(m);
                BOOST_VERIFY(!error_code);
            }
            void unlock()
            {
                int error_code = pthread_mutex_unlock(m);
                BOOST_VERIFY(!error_code);
                locked=false;
            }
            
            ~pthread_mutex_scoped_lock()
            {
                if(locked)
                {
                    unlock();
                }
            }
            
        };

        class pthread_mutex_scoped_unlock
        {
            pthread_mutex_t* m;
        public:
            explicit pthread_mutex_scoped_unlock(pthread_mutex_t* m_):
                m(m_)
            {
                int error_code = pthread_mutex_unlock(m);
                BOOST_VERIFY(!error_code);
            }
            ~pthread_mutex_scoped_unlock()
            {
                int error_code = pthread_mutex_lock(m);
                BOOST_VERIFY(!error_code);
            }
            
        };
    }
}

#include <boost/config/abi_suffix.hpp>

#endif
