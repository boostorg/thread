#ifndef BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP
#define BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>

namespace boost
{
    class read_write_mutex
    {
    private:
        boost::mutex guard;
        boost::condition cond;
        long reader_count;
        
    public:
        read_write_mutex():
            reader_count(0)
        {}
        
        class scoped_read_lock
        {
            read_write_mutex& m;
        public:
            scoped_read_lock(read_write_mutex& m_):
                m(m_)
            {
                boost::mutex::scoped_lock lock(m.guard);
                ++m.reader_count;
            }
            ~scoped_read_lock()
            {
                boost::mutex::scoped_lock lock(m.guard);
                --m.reader_count;
                m.cond.notify_one();
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
                while(m.reader_count)
                {
                    m.cond.wait(lock);
                }
            }
            void unlock()
            {
                m.cond.notify_one();
                lock.unlock();
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
