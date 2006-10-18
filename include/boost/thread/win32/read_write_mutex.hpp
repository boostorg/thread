#ifndef BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP
#define BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP

#include <boost/assert.hpp>
#include <boost/detail/interlocked.hpp>
#include <boost/thread/win32/thread_primitives.hpp>
#include <boost/thread/win32/interlocked_read.hpp>

namespace boost
{
    class read_write_mutex
    {
    private:
        long state;
        void* shared_event;
        void* exclusive_event;
        void* upgradeable_event;

        BOOST_STATIC_CONSTANT(long,shared_count_increment=1);
        BOOST_STATIC_CONSTANT(long,shared_count_mask=0x7fff);
        BOOST_STATIC_CONSTANT(long,exclusive_count_mask=0x1fff8000);
        BOOST_STATIC_CONSTANT(long,exclusive_count_increment=0x8000);
        BOOST_STATIC_CONSTANT(long,shared=0x20000000);
        BOOST_STATIC_CONSTANT(long,upgradeable=0x40000000);
        BOOST_STATIC_CONSTANT(long,exclusive=0x80000000);

        template<long lock_flags,long unlock_flags,long count_mask,long count_increment,long loop_mask,bool test_first>
        long update_state(long old_state)
        {
            if(test_first && (old_state&loop_mask))
            {
                return old_state;
            }
            do
            {
                long const masked_state=old_state&~unlock_flags;
                long const new_count=((masked_state&count_mask)+count_increment);
                BOOST_ASSERT(new_count<=count_mask);
                long const new_state=(masked_state&~count_mask)|new_count|lock_flags;
                long const current_state=BOOST_INTERLOCKED_COMPARE_EXCHANGE(&state,new_state,old_state);
                if(current_state==old_state)
                {
                    break;
                }
                old_state=current_state;
            }
            while(!(old_state&loop_mask));
            return old_state;
        }

        enum event_type
        {
            auto_reset=false,
            manual_reset=true
        };
        enum initial_event_state
        {
            initially_set=true,
            initially_reset=false
        };

        void* create_anonymous_event(event_type type,initial_event_state initial_state)
        {
            return ::boost::detail::CreateEventA(NULL,type,initial_state,NULL);
        }

    public:
        read_write_mutex():
            state(0),
            shared_event(create_anonymous_event(manual_reset,initially_set)),
            exclusive_event(create_anonymous_event(auto_reset,initially_reset)),
            upgradeable_event(create_anonymous_event(auto_reset,initially_set)) 
        {}

        ~read_write_mutex()
        {
            ::boost::detail::CloseHandle(shared_event);
            ::boost::detail::CloseHandle(exclusive_event);
            ::boost::detail::CloseHandle(upgradeable_event);
        }

        void lock_shareable()
        {
            // try and add ourselves to the current readers
            long old_state=update_state<shared,0,shared_count_mask,shared_count_increment,exclusive,false>(0);
            
            if(old_state&exclusive)
            {
                // someone else has exclusive access
                // mark that we're interested
                old_state=update_state<0,0,shared_count_mask,shared_count_increment,0,false>(old_state);
                // now we're marked as waiting, if the exclusive lock is now free, try and claim
                // loop until we can claim shared lock
                while(true)
                {
                    old_state=update_state<shared,0,0,0,exclusive|shared,true>(old_state);
                    if(old_state&shared)
                    {
                        break;
                    }
                    old_state&=~(exclusive|shared);
                    
                    bool const success=::boost::detail::WaitForSingleObject(shared_event,BOOST_INFINITE)==0;
                    BOOST_ASSERT(success);
                }
            }
        }

        void unlock_shareable()
        {
            long old_state=update_state<0,0,shared_count_mask,-shared_count_increment,0,false>(shared|shared_count_increment); // decrement shared count
            if((old_state&shared_count_mask)==shared_count_increment) // if it was just us sharing
            {
                old_state-=shared_count_increment;
                old_state=update_state<0,shared,0,0,shared_count_mask,true>(old_state);
                if((old_state&exclusive_count_mask) && !(old_state&shared_count_mask))
                {
                    bool const res=::boost::detail::SetEvent(exclusive_event)!=0;
                    BOOST_ASSERT(res);
                }
            }
        }

        void lock()
        {
            // try and acquire an exclusive lock
            long old_state=update_state<exclusive,0,exclusive_count_mask,exclusive_count_increment,exclusive|shared|upgradeable,false>(0);
            if(old_state&(exclusive|shared|upgradeable))
            {
                // someone else has the lock
                // mark that we're interested
                old_state=update_state<0,0,exclusive_count_mask,exclusive_count_increment,0,false>(old_state);
                // now we're marked as waiting, if the lock is now free, try and claim
                // loop until we can claim exclusive lock
                while(true)
                {
                    old_state=update_state<exclusive,0,0,0,exclusive|shared|upgradeable,true>(old_state);
                    if(!(old_state&(shared|exclusive)))
                    {
                        break;
                    }
                    old_state&=~(exclusive|shared|upgradeable);
                    
                    bool const success=::boost::detail::WaitForSingleObject(exclusive_event,BOOST_INFINITE)==0;
                    BOOST_ASSERT(success);
                }
            }
            bool const success=::boost::detail::ResetEvent(shared_event);
            BOOST_ASSERT(success);
        }

        void unlock()
        {
            long old_state=update_state<0,exclusive,exclusive_count_mask,-exclusive_count_increment,0,false>(exclusive|exclusive_count_increment);
            if(old_state&shared_count_mask)
            {
                bool const success=::boost::detail::SetEvent(shared_event)!=0;
                BOOST_ASSERT(success);
            }
            
            if(old_state&exclusive_count_mask)
            {
                bool const success=::boost::detail::SetEvent(exclusive_event)!=0;
                BOOST_ASSERT(success);
            }
        }

        void lock_upgradeable()
        {
            // try and acquire an upgrading lock
            long old_state=update_state<upgradeable|shared,0,shared_count_mask,shared_count_increment,exclusive|upgradeable,false>(0);
            if(old_state&(exclusive|upgradeable))
            {
                // someone else has the lock
                // mark that we're interested
                old_state=update_state<0,0,shared_count_mask,shared_count_increment,0,false>(old_state);
                // now we're marked as waiting, if the lock is now free, try and claim
                // loop until we can claim lock

                void* const handles[2]={upgradeable_event,shared_event};
                
                while(true)
                {
                    old_state=update_state<upgradeable|shared,0,0,0,exclusive|upgradeable,true>(old_state);
                    if(!(old_state&(upgradeable|exclusive)))
                    {
                        break;
                    }
                    if(old_state&exclusive) // someone has exclusive lock
                    {
                        // so we can take lock when they release it
                        bool const success=::boost::detail::SetEvent(upgradeable_event)!=0;
                        BOOST_ASSERT(success);
                    }
                    old_state&=~(exclusive|upgradeable);
                    
                    bool const success=::boost::detail::WaitForMultipleObjects(2,handles,true,BOOST_INFINITE)<2;
                    BOOST_ASSERT(success);
                }
            }
        }

        void unlock_upgradeable()
        {
            update_state<0,upgradeable,0,0,0,false>(upgradeable|shared|shared_count_increment); // we're not upgrading
            unlock_shareable(); // and we're not sharing either
            bool const success=::boost::detail::SetEvent(upgradeable_event)!=0;
            BOOST_ASSERT(success);
        }

        void unlock_upgradeable_and_lock()
        {
            // try and just change "1 upgrading, shared lock" to "1 exclusive lock"
            // we can't do it if there's more than 1 shared lock
            long old_state=update_state<exclusive,
                upgradeable|shared,
                shared_count_mask|exclusive_count_mask,
                exclusive_count_increment-shared_count_increment,
                shared_count_mask-shared_count_increment,
                true>(upgradeable|shared|shared_count_increment);
            if((old_state&shared_count_mask)!=shared_count_increment)
            {
                // We can't upgrade, because there's another shared lock
                // we're not a shared lock any more, but an exclusive-lock-to-be
                old_state=update_state<0,0,exclusive_count_mask|shared_count_mask,exclusive_count_increment-shared_count_increment,0,false>(old_state);
                old_state+=exclusive_count_increment-shared_count_increment;
                // if there now aren't any more shared locks, take off the mask
                old_state=update_state<0,shared,0,0,shared_count_mask,true>(old_state);
                if(!(old_state&shared_count_mask))
                {
                    old_state&=~shared;
                }
                // now we're marked as waiting, if the lock is now free, try and claim
                // loop until we can claim exclusive lock
                while(true)
                {
                    old_state=update_state<exclusive,upgradeable,0,0,shared,true>(old_state);
                    if(!(old_state&(shared)))
                    {
                        break;
                    }
                    old_state&=~shared;
                    
                    bool const success=::boost::detail::WaitForSingleObject(exclusive_event,BOOST_INFINITE)==0;
                    BOOST_ASSERT(success);
                }
            }
            bool const success=::boost::detail::ResetEvent(shared_event);
            BOOST_ASSERT(success);
        }
        
        void unlock_and_lock_upgradeable()
        {
            long old_state=update_state<upgradeable|shared,exclusive,
                shared_count_mask|exclusive_count_mask,shared_count_increment-exclusive_count_increment,
                0,false>(exclusive|exclusive_count_increment);
            bool const success=::boost::detail::SetEvent(shared_event)!=0;
            BOOST_ASSERT(success);
        }
        
        void unlock_and_lock_shareable()
        {
            long old_state=update_state<shared,exclusive,
                shared_count_mask|exclusive_count_mask,shared_count_increment-exclusive_count_increment,
                0,false>(exclusive|exclusive_count_increment);
            bool const success=::boost::detail::SetEvent(shared_event)!=0;
            BOOST_ASSERT(success);
        }

        void unlock_upgradeable_and_lock_shareable()
        {
            long old_state=update_state<0,upgradeable,0,0,0,false>(upgradeable|shared|shared_count_increment);
            bool const success=::boost::detail::SetEvent(upgradeable_event)!=0;
            BOOST_ASSERT(success);
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
