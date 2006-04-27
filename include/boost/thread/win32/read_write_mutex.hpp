#ifndef BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP
#define BOOST_THREAD_WIN32_READ_WRITE_MUTEX_HPP

namespace boost
{
    class read_write_mutex
    {
    public:
        class scoped_read_lock
        {
        public:
            scoped_read_lock(read_write_mutex&)
            {}
            
        };
        
    };
}


#endif
