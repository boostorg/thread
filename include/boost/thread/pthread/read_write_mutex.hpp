#ifndef BOOST_THREAD_PTHREAD_READ_WRITE_MUTEX_HPP
#define BOOST_THREAD_PTHREAD_READ_WRITE_MUTEX_HPP

//  (C) Copyright 2006-7 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#include <boost/assert.hpp>
#include <boost/static_assert.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/xtime.hpp>

namespace boost
{
    class read_write_mutex
    {
    private:
        struct state_data
        {
            unsigned shared_count:10;
            unsigned exclusive:1;
            unsigned upgradeable:1;
            unsigned exclusive_waiting_blocked:1;
        };
        


        state_data state;
        boost::mutex state_change;
        boost::condition shared_cond;
        boost::condition exclusive_cond;
        boost::condition upgradeable_cond;

        void release_waiters()
        {
            exclusive_cond.notify_one();
            shared_cond.notify_all();
        }
        

    public:
        read_write_mutex()
        {
            state_data state_={0};
            state=state_;
        }

        ~read_write_mutex()
        {
        }

        void lock_shareable()
        {
            boost::mutex::scoped_lock lock(state_change);
                
            while(true)
            {
                if(!state.exclusive && !state.exclusive_waiting_blocked)
                {
                    ++state.shared_count;
                    return;
                }
                
                shared_cond.wait(lock);
            }
        }

        bool try_lock_shareable()
        {
            boost::mutex::scoped_lock lock(state_change);
                
            if(state.exclusive || state.exclusive_waiting_blocked)
            {
                return false;
            }
            else
            {
                ++state.shared_count;
                return true;
            }
        }

        bool timed_lock_shareable(xtime const& timeout)
        {
            boost::mutex::scoped_lock lock(state_change);
                
            while(true)
            {
                if(!state.exclusive && !state.exclusive_waiting_blocked)
                {
                    ++state.shared_count;
                    return true;
                }
                
                if(!shared_cond.timed_wait(lock,timeout))
                {
                    return false;
                }
            }
        }

        void unlock_shareable()
        {
            boost::mutex::scoped_lock lock(state_change);
            bool const last_reader=!--state.shared_count;
                
            if(last_reader)
            {
                if(state.upgradeable)
                {
                    state.upgradeable=false;
                    state.exclusive=true;
                    upgradeable_cond.notify_one();
                }
                else
                {
                    state.exclusive_waiting_blocked=false;
                }
                release_waiters();
            }
        }

        void lock()
        {
            boost::mutex::scoped_lock lock(state_change);
                
            while(true)
            {
                if(state.shared_count || state.exclusive)
                {
                    state.exclusive_waiting_blocked=true;
                }
                else
                {
                    state.exclusive=true;
                    return;
                }
                exclusive_cond.wait(lock);
            }
        }

        bool timed_lock(xtime const& timeout)
        {
            boost::mutex::scoped_lock lock(state_change);
                
            while(true)
            {
                if(state.shared_count || state.exclusive)
                {
                    state.exclusive_waiting_blocked=true;
                }
                else
                {
                    state.exclusive=true;
                    return true;
                }
                if(!exclusive_cond.timed_wait(lock,timeout))
                {
                    return false;
                }
            }
        }

        bool try_lock()
        {
            boost::mutex::scoped_lock lock(state_change);
                
            if(state.shared_count || state.exclusive)
            {
                return false;
            }
            else
            {
                state.exclusive=true;
                return true;
            }
                
        }

        void unlock()
        {
            boost::mutex::scoped_lock lock(state_change);
            state.exclusive=false;
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }

        void lock_upgradeable()
        {
            boost::mutex::scoped_lock lock(state_change);
            while(true)
            {
                if(!state.exclusive && !state.exclusive_waiting_blocked && !state.upgradeable)
                {
                    ++state.shared_count;
                    state.upgradeable=true;
                    return;
                }
                
                shared_cond.wait(lock);
            }
        }

        bool timed_lock_upgradeable(xtime const& timeout)
        {
            boost::mutex::scoped_lock lock(state_change);
            while(true)
            {
                if(!state.exclusive && !state.exclusive_waiting_blocked && !state.upgradeable)
                {
                    ++state.shared_count;
                    state.upgradeable=true;
                    return true;
                }
                
                if(!shared_cond.timed_wait(lock,timeout))
                {
                    return false;
                }
            }
        }

        bool try_lock_upgradeable()
        {
            boost::mutex::scoped_lock lock(state_change);
            if(state.exclusive || state.exclusive_waiting_blocked || state.upgradeable)
            {
                return false;
            }
            else
            {
                ++state.shared_count;
                state.upgradeable=true;
                return true;
            }
        }

        void unlock_upgradeable()
        {
            boost::mutex::scoped_lock lock(state_change);
            state.upgradeable=false;
            bool const last_reader=!--state.shared_count;
                
            if(last_reader)
            {
                state.exclusive_waiting_blocked=false;
                release_waiters();
            }
        }

        void unlock_upgradeable_and_lock()
        {
            boost::mutex::scoped_lock lock(state_change);
            --state.shared_count;
            while(true)
            {
                if(!state.shared_count)
                {
                    state.upgradeable=false;
                    state.exclusive=true;
                    break;
                }
                upgradeable_cond.wait(lock);
            }
        }

        void unlock_and_lock_upgradeable()
        {
            boost::mutex::scoped_lock lock(state_change);
            state.exclusive=false;
            state.upgradeable=true;
            ++state.shared_count;
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }
        
        void unlock_and_lock_shareable()
        {
            boost::mutex::scoped_lock lock(state_change);
            state.exclusive=false;
            ++state.shared_count;
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }
        
        void unlock_upgradeable_and_lock_shareable()
        {
            boost::mutex::scoped_lock lock(state_change);
            state.upgradeable=false;
            state.exclusive_waiting_blocked=false;
            release_waiters();
        }
        
        class scoped_read_lock
        {
            read_write_mutex& m;
        public:
            scoped_read_lock(read_write_mutex& m_):
                m(m_)
            {
                m.lock_shareable();
            }
            ~scoped_read_lock()
            {
                m.unlock_shareable();
            }
        };

        class scoped_write_lock
        {
            read_write_mutex& m;
            bool locked;
            
        public:
            scoped_write_lock(read_write_mutex& m_):
                m(m_),locked(false)
            {
                lock();
            }
            void lock()
            {
                m.lock();
                locked=true;
            }
            
            void unlock()
            {
                m.unlock();
                locked=false;
            }
            ~scoped_write_lock()
            {
                if(locked)
                {
                    unlock();
                }
            }
        };

        class scoped_upgradeable_lock
        {
            read_write_mutex& m;
            bool locked;
            bool upgraded;
             
        public:
            scoped_upgradeable_lock(read_write_mutex& m_):
                m(m_),
                locked(false),upgraded(false)
            {
                lock();
            }
            void lock()
            {
                m.lock_upgradeable();
                locked=true;
            }
            void upgrade()
            {
                m.unlock_upgradeable_and_lock();
                upgraded=true;
            }
            void unlock()
            {
                if(upgraded)
                {
                    m.unlock();
                }
                else
                {
                    m.unlock_upgradeable();
                }
            }
            ~scoped_upgradeable_lock()
            {
                if(locked)
                {
                    unlock();
                }
            }
        };
    };
}


#endif
