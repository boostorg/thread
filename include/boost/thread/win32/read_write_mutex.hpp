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
        void* writer_sem;
        void* upgrader_sem;

        BOOST_STATIC_CONSTANT(long,shared_count_shift=0);
        BOOST_STATIC_CONSTANT(long,shared_count_mask=0x3ff);
        BOOST_STATIC_CONSTANT(long,exclusive_count_shift=10);
        BOOST_STATIC_CONSTANT(long,exclusive_count_mask=0xffc00);
        BOOST_STATIC_CONSTANT(long,shared_mask=0x20000000);
        BOOST_STATIC_CONSTANT(long,upgrading_mask=0x40000000);
        BOOST_STATIC_CONSTANT(long,exclusive_mask=0x80000000);

        template<long lock_mask,long count_mask,long count_increment,long loop_mask,bool test_first>
        long update_state(long old_state)
        {
            if(test_first && (old_state&loop_mask))
            {
                return old_state;
            }
            do
            {
                long const new_count=((old_state&count_mask)+count_increment);
                BOOST_ASSERT(new_count<=count_mask);
                long const new_state=(old_state&~count_mask)|new_count|lock_mask;
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

    public:
        read_write_mutex():
            state(0),
            shared_event(::boost::detail::CreateEventA(NULL,true,false,NULL)),
            writer_sem(::boost::detail::CreateSemaphoreA(NULL,0,1,NULL)),
            upgrader_sem(::boost::detail::CreateSemaphoreA(NULL,0,1,NULL))
        {}

        ~read_write_mutex()
        {
            ::boost::detail::CloseHandle(shared_event);
            ::boost::detail::CloseHandle(writer_sem);
            ::boost::detail::CloseHandle(upgrader_sem);
        }

        void lock_shareable()
        {
            // try and add ourselves to the current readers
            long old_state=update_state<shared_mask,shared_count_mask,1,exclusive_mask,false>(0);
            
            if(!(old_state&shared_mask))
            {
                // someone else has exclusive access
                // mark that we're interested
                old_state=update_state<0,shared_count_mask,1,0,false>(old_state);
                // now we're marked as waiting, if the exclusive lock is now free, try and claim
                // loop until we can claim shared lock
                while(true)
                {
                    old_state=update_state<shared_mask,0,0,exclusive_mask|shared_mask,true>(old_state);
                    if(old_state&shared_mask)
                    {
                        break;
                    }
                    
                    bool const success=::boost::detail::WaitForSingleObject(shared_event,BOOST_INFINITE)==0;
                    BOOST_ASSERT(success);
                }
            }
        }

        void unlock_shareable()
        {
            long old_state=update_state<0,shared_count_mask,-1,0,false>(old_state); // decrement shared count
            if((old_state&shared_count_mask)==1) // if it was just us sharing
            {
                old_state=update_state<0,shared_mask,-shared_mask,0,false>(old_state);
                if(old_state&exclusive_count_mask)
                {
                    bool const res=::boost::detail::ReleaseSemaphore(writer_sem)!=0;
                    BOOST_ASSERT(res);
                }
            }
        }

        void lock()
        {
            // try and acquire an exclusive lock
            long old_state=update_state<exclusive_mask,exclusive_count_mask,1<<exclusive_count_shift,exclusive_mask|shared_mask>(old_state);
            if(old_state&(exclusive_mask|shared_mask))
            {
                // someone else has the lock
                // mark that we're interested
                old_state=update_state<0,exclusive_count_mask,1,0,false>(old_state);
                // now we're marked as waiting, if the lock is now free, try and claim
                // loop until we can claim exclusive lock
                while(true)
                {
                    old_state=update_state<exclusive_mask,0,0,exclusive_mask|shared_mask,true>(old_state);
                    if(!(old_state&(shared_mask|exclusive_mask)))
                    {
                        break;
                    }
                    
                    bool const success=::boost::detail::WaitForSingleObject(writer_sem,BOOST_INFINITE)==0;
                    BOOST_ASSERT(success);
                }
            }
            bool const success=::boost::detail::ResetEvent(shared_event);
            BOOST_ASSERT(success);
        }

        void unlock()
        {
            long old_state=update_state<0,exclusive_mask|exclusive_count_mask,-(exclusive_mask | (1<<exclusive_count_shift)),0,false>(old_state);
            if(old_state&shared_count_mask)
            {
                bool const success=::boost::detail::SetEvent(shared_event)!=0;
                BOOST_ASSERT(success);
            }
            
            if(old_state&exclusive_count_mask)
            {
                bool const success=::boost::detail::ReleaseSemaphore(writer_sem)!=0;
                BOOST_ASSERT(success);
            }
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
            boost::mutex::scoped_lock lock;
            
        public:
            scoped_write_lock(read_write_mutex& m_):
                m(m_),
                lock(m.guard)
            {
                m.lock();
            }
            void unlock()
            {
                m.unlock();
            }
            ~scoped_write_lock()
            {
                if(lock.locked())
                {
                    unlock();
                }
            }
        };
        
    };
}


#endif
