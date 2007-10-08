// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
#ifndef BOOST_THREAD_LOCKS_HPP
#define BOOST_THREAD_LOCKS_HPP
#include <boost/thread/detail/config.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/detail/move.hpp>
#include <algorithm>
#include <boost/thread/thread_time.hpp>

namespace boost
{
    struct defer_lock_t
    {};
    struct try_to_lock_t
    {};
    struct adopt_lock_t
    {};
    
    const defer_lock_t defer_lock={};
    const try_to_lock_t try_to_lock={};
    const adopt_lock_t adopt_lock={};

    template<typename Mutex>
    class shareable_lock;

    template<typename Mutex>
    class exclusive_lock;

    template<typename Mutex>
    class upgradeable_lock;

    template<typename Mutex>
    class lock_guard
    {
    private:
        Mutex& m;

        explicit lock_guard(lock_guard&);
        lock_guard& operator=(lock_guard&);
    public:
        explicit lock_guard(Mutex& m_):
            m(m_)
        {
            m.lock();
        }
        lock_guard(Mutex& m_,adopt_lock_t):
            m(m_)
        {}
        ~lock_guard()
        {
            m.unlock();
        }
    };


    template<typename Mutex>
    class unique_lock
    {
    private:
        Mutex* m;
        bool is_locked;
        explicit unique_lock(unique_lock&);
        unique_lock& operator=(unique_lock&);
    public:
        explicit unique_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        unique_lock(Mutex& m_,adopt_lock_t):
            m(&m_),is_locked(true)
        {}
        unique_lock(Mutex& m_,defer_lock_t):
            m(&m_),is_locked(false)
        {}
        unique_lock(Mutex& m_,try_to_lock_t):
            m(&m_),is_locked(false)
        {
            try_lock();
        }
        unique_lock(Mutex& m_,system_time const& target_time):
            m(&m_),is_locked(false)
        {
            timed_lock(target_time);
        }
        unique_lock(boost::move_t<unique_lock> other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
        }
        unique_lock(boost::move_t<upgradeable_lock<Mutex> > other);

        unique_lock& operator=(boost::move_t<unique_lock> other)
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }

        unique_lock& operator=(boost::move_t<upgradeable_lock<Mutex> > other)
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }

        void swap(unique_lock& other)
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }
        void swap(boost::move_t<unique_lock> other)
        {
            std::swap(m,other->m);
            std::swap(is_locked,other->is_locked);
        }
        
        ~unique_lock()
        {
            if(owns_lock())
            {
                m->unlock();
            }
        }
        void lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            m->lock();
            is_locked=true;
        }
        bool try_lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            is_locked=m->try_lock();
            return is_locked;
        }
        template<typename TimeDuration>
        bool timed_lock(TimeDuration const& relative_time)
        {
            is_locked=m->timed_lock(relative_time);
            return is_locked;
        }
        
        bool timed_lock(::boost::system_time const& absolute_time)
        {
            is_locked=m->timed_lock(absolute_time);
            return is_locked;
        }
        void unlock()
        {
            if(!owns_lock())
            {
                throw boost::lock_error();
            }
            m->unlock();
            is_locked=false;
        }
            
        typedef void (unique_lock::*bool_type)();
        operator bool_type() const
        {
            return is_locked?&unique_lock::lock:0;
        }
        bool owns_lock() const
        {
            return is_locked;
        }

        Mutex* mutex() const
        {
            return m;
        }

        Mutex* release()
        {
            Mutex* const res=m;
            m=0;
            is_locked=false;
            return res;
        }

        friend class shareable_lock<Mutex>;
        friend class upgradeable_lock<Mutex>;
    };

    template<typename Mutex>
    class shareable_lock
    {
    protected:
        Mutex* m;
        bool is_locked;
    private:
        explicit shareable_lock(shareable_lock&);
        shareable_lock& operator=(shareable_lock&);
    public:
        explicit shareable_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        shareable_lock(Mutex& m_,bool do_lock):
            m(&m_),is_locked(false)
        {
            if(do_lock)
            {
                lock();
            }
        }
        shareable_lock(boost::move_t<shareable_lock> other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
        }

        shareable_lock(boost::move_t<unique_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            if(is_locked)
            {
                m->unlock_and_lock_shareable();
            }
        }

        shareable_lock(boost::move_t<upgradeable_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            if(is_locked)
            {
                m->unlock_upgradeable_and_lock_shareable();
            }
        }

        shareable_lock& operator=(boost::move_t<shareable_lock> other)
        {
            shareable_lock temp(other);
            swap(temp);
            return *this;
        }

        shareable_lock& operator=(boost::move_t<unique_lock<Mutex> > other)
        {
            shareable_lock temp(other);
            swap(temp);
            return *this;
        }

        shareable_lock& operator=(boost::move_t<upgradeable_lock<Mutex> > other)
        {
            shareable_lock temp(other);
            swap(temp);
            return *this;
        }

        void swap(shareable_lock& other)
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }
        
        ~shareable_lock()
        {
            if(owns_lock())
            {
                m->unlock_shareable();
            }
        }
        void lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            m->lock_shareable();
            is_locked=true;
        }
        bool try_lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            is_locked=m->try_lock_shareable();
            return is_locked;
        }
        void unlock()
        {
            if(!owns_lock())
            {
                throw boost::lock_error();
            }
            m->unlock_shareable();
            is_locked=false;
        }
            
        typedef void (shareable_lock::*bool_type)();
        operator bool_type() const
        {
            return is_locked?&shareable_lock::lock:0;
        }
        bool owns_lock() const
        {
            return is_locked;
        }

    };

    template<typename Mutex>
    class upgradeable_lock
    {
    protected:
        Mutex* m;
        bool is_locked;
    private:
        explicit upgradeable_lock(upgradeable_lock&);
        upgradeable_lock& operator=(upgradeable_lock&);
    public:
        explicit upgradeable_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        upgradeable_lock(Mutex& m_,bool do_lock):
            m(&m_),is_locked(false)
        {
            if(do_lock)
            {
                lock();
            }
        }
        upgradeable_lock(boost::move_t<upgradeable_lock> other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
        }

        upgradeable_lock(boost::move_t<unique_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            if(is_locked)
            {
                m->unlock_and_lock_upgradeable();
            }
        }

        upgradeable_lock& operator=(boost::move_t<upgradeable_lock> other)
        {
            upgradeable_lock temp(other);
            swap(temp);
            return *this;
        }

        upgradeable_lock& operator=(boost::move_t<unique_lock<Mutex> > other)
        {
            upgradeable_lock temp(other);
            swap(temp);
            return *this;
        }

        void swap(upgradeable_lock& other)
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }
        
        ~upgradeable_lock()
        {
            if(owns_lock())
            {
                m->unlock_upgradeable();
            }
        }
        void lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            m->lock_upgradeable();
            is_locked=true;
        }
        bool try_lock()
        {
            if(owns_lock())
            {
                throw boost::lock_error();
            }
            is_locked=m->try_lock_upgradeable();
            return is_locked;
        }
        void unlock()
        {
            if(!owns_lock())
            {
                throw boost::lock_error();
            }
            m->unlock_upgradeable();
            is_locked=false;
        }
            
        typedef void (upgradeable_lock::*bool_type)();
        operator bool_type() const
        {
            return is_locked?&upgradeable_lock::lock:0;
        }
        bool owns_lock() const
        {
            return is_locked;
        }
        friend class shareable_lock<Mutex>;
        friend class unique_lock<Mutex>;
    };

    template<typename Mutex>
    unique_lock<Mutex>::unique_lock(boost::move_t<upgradeable_lock<Mutex> > other):
        m(other->m),is_locked(other->is_locked)
    {
        other->is_locked=false;
        if(is_locked)
        {
            m->unlock_upgradeable_and_lock();
        }
    }

    template <class Mutex>
    class upgrade_to_unique_lock
    {
    private:
        upgradeable_lock<Mutex>* source;
        unique_lock<Mutex> exclusive;

        explicit upgrade_to_unique_lock(upgrade_to_unique_lock&);
        upgrade_to_unique_lock& operator=(upgrade_to_unique_lock&);
    public:
        explicit upgrade_to_unique_lock(upgradeable_lock<Mutex>& m_):
            source(&m_),exclusive(boost::move(*source))
        {}
        ~upgrade_to_unique_lock()
        {
            if(source)
            {
                *source=boost::move(exclusive);
            }
        }

        upgrade_to_unique_lock(boost::move_t<upgrade_to_unique_lock> other):
            source(other->source),exclusive(boost::move(other->exclusive))
        {
            other->source=0;
        }
        
        upgrade_to_unique_lock& operator=(boost::move_t<upgrade_to_unique_lock> other)
        {
            upgrade_to_unique_lock temp(other);
            swap(temp);
            return *this;
        }
        void swap(upgrade_to_unique_lock& other)
        {
            std::swap(source,other.source);
            exclusive.swap(other.exclusive);
        }
        typedef void (upgrade_to_unique_lock::*bool_type)(upgrade_to_unique_lock&);
        operator bool_type() const
        {
            return exclusive.owns_lock()?&upgrade_to_unique_lock::swap:0;
        }
        bool owns_lock() const
        {
            return exclusive.owns_lock();
        }
    };
}

#endif
