// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba
#ifndef BOOST_THREAD_LOCKS_HPP
#define BOOST_THREAD_LOCKS_HPP
#include <boost/thread/detail/config.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/detail/move.hpp>
#include <algorithm>
#include <iterator>
#include <boost/thread/thread_time.hpp>
#include <boost/detail/workaround.hpp>
#include <boost/type_traits/is_class.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/time_point.hpp>
#include <boost/chrono/duration.hpp>
#endif

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    struct xtime;

#if defined(BOOST_NO_SFINAE) ||                           \
    BOOST_WORKAROUND(__IBMCPP__, BOOST_TESTED_AT(600)) || \
    BOOST_WORKAROUND(__SUNPRO_CC, BOOST_TESTED_AT(0x590))
#define BOOST_THREAD_NO_AUTO_DETECT_MUTEX_TYPES
#endif

#ifndef BOOST_THREAD_NO_AUTO_DETECT_MUTEX_TYPES
    namespace detail
    {
#define BOOST_DEFINE_HAS_MEMBER_CALLED(member_name)                     \
        template<typename T, bool=boost::is_class<T>::value>            \
        struct has_member_called_##member_name                          \
        {                                                               \
            BOOST_STATIC_CONSTANT(bool, value=false);                   \
        };                                                              \
                                                                        \
        template<typename T>                                            \
        struct has_member_called_##member_name<T,true>                  \
        {                                                               \
            typedef char true_type;                                     \
            struct false_type                                           \
            {                                                           \
                true_type dummy[2];                                     \
            };                                                          \
                                                                        \
            struct fallback { int member_name; };                       \
            struct derived:                                             \
                T, fallback                                             \
            {                                                           \
                derived();                                              \
            };                                                          \
                                                                        \
            template<int fallback::*> struct tester;                    \
                                                                        \
            template<typename U>                                        \
                static false_type has_member(tester<&U::member_name>*); \
            template<typename U>                                        \
                static true_type has_member(...);                       \
                                                                        \
            BOOST_STATIC_CONSTANT(                                      \
                bool, value=sizeof(has_member<derived>(0))==sizeof(true_type)); \
        }

        BOOST_DEFINE_HAS_MEMBER_CALLED(lock);
        BOOST_DEFINE_HAS_MEMBER_CALLED(unlock);
        BOOST_DEFINE_HAS_MEMBER_CALLED(try_lock);

        template<typename T,bool=has_member_called_lock<T>::value >
        struct has_member_lock
        {
            BOOST_STATIC_CONSTANT(bool, value=false);
        };

        template<typename T>
        struct has_member_lock<T,true>
        {
            typedef char true_type;
            struct false_type
            {
                true_type dummy[2];
            };

            template<typename U,typename V>
            static true_type has_member(V (U::*)());
            template<typename U>
            static false_type has_member(U);

            BOOST_STATIC_CONSTANT(
                bool,value=sizeof(has_member_lock<T>::has_member(&T::lock))==sizeof(true_type));
        };

        template<typename T,bool=has_member_called_unlock<T>::value >
        struct has_member_unlock
        {
            BOOST_STATIC_CONSTANT(bool, value=false);
        };

        template<typename T>
        struct has_member_unlock<T,true>
        {
            typedef char true_type;
            struct false_type
            {
                true_type dummy[2];
            };

            template<typename U,typename V>
            static true_type has_member(V (U::*)());
            template<typename U>
            static false_type has_member(U);

            BOOST_STATIC_CONSTANT(
                bool,value=sizeof(has_member_unlock<T>::has_member(&T::unlock))==sizeof(true_type));
        };

        template<typename T,bool=has_member_called_try_lock<T>::value >
        struct has_member_try_lock
        {
            BOOST_STATIC_CONSTANT(bool, value=false);
        };

        template<typename T>
        struct has_member_try_lock<T,true>
        {
            typedef char true_type;
            struct false_type
            {
                true_type dummy[2];
            };

            template<typename U>
            static true_type has_member(bool (U::*)());
            template<typename U>
            static false_type has_member(U);

            BOOST_STATIC_CONSTANT(
                bool,value=sizeof(has_member_try_lock<T>::has_member(&T::try_lock))==sizeof(true_type));
        };

    }


    template<typename T>
    struct is_mutex_type
    {
        BOOST_STATIC_CONSTANT(bool, value = detail::has_member_lock<T>::value &&
                              detail::has_member_unlock<T>::value &&
                              detail::has_member_try_lock<T>::value);

    };
#else
    template<typename T>
    struct is_mutex_type
    {
        BOOST_STATIC_CONSTANT(bool, value = false);
    };
#endif

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
    class shared_lock;

    template<typename Mutex>
    class upgrade_lock;

    template<typename Mutex>
    class unique_lock;

    namespace detail
    {
        template<typename Mutex>
        class try_lock_wrapper;
    }

#ifdef BOOST_THREAD_NO_AUTO_DETECT_MUTEX_TYPES
    template<typename T>
    struct is_mutex_type<unique_lock<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template<typename T>
    struct is_mutex_type<shared_lock<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template<typename T>
    struct is_mutex_type<upgrade_lock<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    template<typename T>
    struct is_mutex_type<detail::try_lock_wrapper<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

    class mutex;
    class timed_mutex;
    class recursive_mutex;
    class recursive_timed_mutex;
    class shared_mutex;

    template<>
    struct is_mutex_type<mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_mutex_type<timed_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_mutex_type<recursive_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_mutex_type<recursive_timed_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };
    template<>
    struct is_mutex_type<shared_mutex>
    {
        BOOST_STATIC_CONSTANT(bool, value = true);
    };

#endif

    template<typename Mutex>
    class lock_guard
    {
    private:
        Mutex& m;

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        lock_guard(lock_guard const&) = delete;
        lock_guard& operator=(lock_guard const&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        lock_guard(lock_guard&);
        lock_guard& operator=(lock_guard&);
#endif // BOOST_NO_DELETED_FUNCTIONS
    public:
        typedef Mutex mutex_type;
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

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        unique_lock(unique_lock const&) = delete;
        unique_lock& operator=(unique_lock const&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        unique_lock(unique_lock&);
        unique_lock& operator=(unique_lock &);
#endif // BOOST_NO_DELETED_FUNCTIONS

    private:
        explicit unique_lock(upgrade_lock<Mutex>&);
        unique_lock& operator=(upgrade_lock<Mutex>& other);
    public:
        typedef Mutex mutex_type;
#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100)
        unique_lock(const volatile unique_lock&);
#endif
        unique_lock() BOOST_NOEXCEPT :
            m(0),is_locked(false)
        {}

        explicit unique_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        unique_lock(Mutex& m_,adopt_lock_t):
            m(&m_),is_locked(true)
        {}
        unique_lock(Mutex& m_,defer_lock_t) BOOST_NOEXCEPT:
            m(&m_),is_locked(false)
        {}
        unique_lock(Mutex& m_,try_to_lock_t):
            m(&m_),is_locked(false)
        {
            try_lock();
        }
        template<typename TimeDuration>
        unique_lock(Mutex& m_,TimeDuration const& target_time):
            m(&m_),is_locked(false)
        {
            timed_lock(target_time);
        }
        unique_lock(Mutex& m_,system_time const& target_time):
            m(&m_),is_locked(false)
        {
            timed_lock(target_time);
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(Mutex& mtx, const chrono::time_point<Clock, Duration>& t)
                : m(&mtx), is_locked(mtx.try_lock_until(t))
        {
        }
        template <class Rep, class Period>
        unique_lock(Mutex& mtx, const chrono::duration<Rep, Period>& d)
                : m(&mtx), is_locked(mtx.try_lock_for(d))
        {
        }
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
        unique_lock(unique_lock&& other) BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }
        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION unique_lock(upgrade_lock<Mutex>&& other);


        unique_lock& operator=(unique_lock&& other)  BOOST_NOEXCEPT
        {
            unique_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        unique_lock& operator=(upgrade_lock<Mutex>&& other)  BOOST_NOEXCEPT
        {
            unique_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }
#endif
#else
#if defined BOOST_THREAD_USES_MOVE
        unique_lock(boost::rv<unique_lock<Mutex> >& other)  BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }
        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION unique_lock(boost::rv<upgrade_lock<Mutex> >& other);

        operator ::boost::rv<unique_lock<Mutex> >&()
        {
          return *static_cast< ::boost::rv<unique_lock<Mutex> >* >(this);
        }
        operator const ::boost::rv<unique_lock<Mutex> >&() const
        {
          return *static_cast<const ::boost::rv<unique_lock<Mutex> >* >(this);
        }
        ::boost::rv<unique_lock<Mutex> >& move()
        {
          return *static_cast< ::boost::rv<unique_lock<Mutex> >* >(this);
        }
        const ::boost::rv<unique_lock<Mutex> >& move() const
        {
          return *static_cast<const ::boost::rv<unique_lock<Mutex> >* >(this);
        }
#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100)
        unique_lock& operator=(unique_lock<Mutex> other)
        {
            swap(other);
            return *this;
        }
#else
        unique_lock& operator=(boost::rv<unique_lock<Mutex> >& other)  BOOST_NOEXCEPT
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }
#endif

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        unique_lock& operator=(boost::rv<upgrade_lock<Mutex> >& other)  BOOST_NOEXCEPT
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }
#endif
#else
        unique_lock(detail::thread_move_t<unique_lock<Mutex> > other)  BOOST_NOEXCEPT:
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            other->m=0;
        }
        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION unique_lock(detail::thread_move_t<upgrade_lock<Mutex> > other);

        operator detail::thread_move_t<unique_lock<Mutex> >()
        {
            return move();
        }

        detail::thread_move_t<unique_lock<Mutex> > move()
        {
            return detail::thread_move_t<unique_lock<Mutex> >(*this);
        }

#if BOOST_WORKAROUND(__SUNPRO_CC, < 0x5100)
        unique_lock& operator=(unique_lock<Mutex> other)
        {
            swap(other);
            return *this;
        }
#else
        unique_lock& operator=(detail::thread_move_t<unique_lock<Mutex> > other)  BOOST_NOEXCEPT
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }
#endif

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        unique_lock& operator=(detail::thread_move_t<upgrade_lock<Mutex> > other)  BOOST_NOEXCEPT
        {
            unique_lock temp(other);
            swap(temp);
            return *this;
        }
#endif

        void swap(detail::thread_move_t<unique_lock<Mutex> > other)  BOOST_NOEXCEPT
        {
            std::swap(m,other->m);
            std::swap(is_locked,other->is_locked);
        }
#endif
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
        // Conversion from upgrade locking
        unique_lock(upgrade_lock<mutex_type>&& ul, try_to_lock_t)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock())
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(upgrade_lock<mutex_type>&& ul,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock_until(abs_time))
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(upgrade_lock<mutex_type>&& ul,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (ul.owns_lock()) {
            if (ul.mutex()->try_unlock_upgrade_and_lock_for(rel_time))
            {
              m = ul.release();
              is_locked = true;
            }
          }
          else
          {
            m = ul.release();
          }
        }
#endif
#else

#if defined BOOST_THREAD_USES_MOVE
        // Conversion from upgrade locking
        unique_lock(boost::rv<upgrade_lock<mutex_type> > &ul, try_to_lock_t)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock())
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(boost::rv<upgrade_lock<mutex_type> > &ul,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock_until(abs_time))
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(boost::rv<upgrade_lock<mutex_type> > &ul,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (ul.owns_lock()) {
            if (ul.mutex()->try_unlock_upgrade_and_lock_for(rel_time))
            {
              m = ul.release();
              is_locked = true;
            }
          }
          else
          {
            m = ul.release();
          }
        }
#endif
#else
        // Conversion from upgrade locking
        unique_lock(detail::thread_move_t<upgrade_lock<mutex_type> > ul, try_to_lock_t)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock())
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(detail::thread_move_t<upgrade_lock<mutex_type> > ul,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (ul.owns_lock()) {
              if (ul.mutex()->try_unlock_upgrade_and_lock_until(abs_time))
              {
                  m = ul.release();
                  is_locked = true;
              }
            }
            else
            {
              m = ul.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(detail::thread_move_t<upgrade_lock<mutex_type> > ul,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (ul.owns_lock()) {
            if (ul.mutex()->try_unlock_upgrade_and_lock_for(rel_time))
            {
              m = ul.release();
              is_locked = true;
            }
          }
          else
          {
            m = ul.release();
          }
        }
#endif
#endif
#endif

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONS
#ifndef BOOST_NO_RVALUE_REFERENCES
        // Conversion from shared locking
        unique_lock(shared_lock<mutex_type>&& sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(shared_lock<mutex_type>&& sl,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (sl.owns_lock()) {
              if (sl.mutex()->try_unlock_shared_and_lock_until(abs_time))
              {
                  m = sl.release();
                  is_locked = true;
              }
            }
            else
            {
              m = sl.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(shared_lock<mutex_type>&& sl,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
              if (sl.owns_lock()) {
                if (sl.mutex()->try_unlock_shared_and_lock_for(rel_time))
                {
                    m = sl.release();
                    is_locked = true;
                }
              }
              else
              {
                m = sl.release();
              }
        }
#endif
#else

#if defined BOOST_THREAD_USES_MOVE

        // Conversion from shared locking
        unique_lock(boost::rv<shared_lock<mutex_type> >& sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(boost::rv<shared_lock<mutex_type> >& sl,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (sl.owns_lock()) {
              if (sl.mutex()->try_unlock_shared_and_lock_until(abs_time))
              {
                  m = sl.release();
                  is_locked = true;
              }
            }
            else
            {
              m = sl.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(boost::rv<shared_lock<mutex_type> >& sl,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
              if (sl.owns_lock()) {
                if (sl.mutex()->try_unlock_shared_and_lock_for(rel_time))
                {
                    m = sl.release();
                    is_locked = true;
                }
              }
              else
              {
                m = sl.release();
              }
        }
#endif
#else

        // Conversion from shared locking
        unique_lock(detail::thread_move_t<shared_lock<mutex_type> > sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        unique_lock(detail::thread_move_t<shared_lock<mutex_type> > sl,
                    const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
            if (sl.owns_lock()) {
              if (sl.mutex()->try_unlock_shared_and_lock_until(abs_time))
              {
                  m = sl.release();
                  is_locked = true;
              }
            }
            else
            {
              m = sl.release();
            }
        }

        template <class Rep, class Period>
        unique_lock(detail::thread_move_t<shared_lock<mutex_type> > sl,
                    const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
              if (sl.owns_lock()) {
                if (sl.mutex()->try_unlock_shared_and_lock_for(rel_time))
                {
                    m = sl.release();
                    is_locked = true;
                }
              }
              else
              {
                m = sl.release();
              }
        }
#endif
#endif
#endif
#endif


        void swap(unique_lock& other) BOOST_NOEXCEPT
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
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
            if(m==0)
            {
                boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
            }
            if(owns_lock())
            {
                boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
            }
            m->lock();
            is_locked=true;
        }
        bool try_lock()
        {
            if(m==0)
            {
                boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
            }
            if(owns_lock())
            {
                boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
            }
            is_locked=m->try_lock();
            return is_locked;
        }
        template<typename TimeDuration>
        bool timed_lock(TimeDuration const& relative_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
          }
            is_locked=m->timed_lock(relative_time);
            return is_locked;
        }

        bool timed_lock(::boost::system_time const& absolute_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
          }
            is_locked=m->timed_lock(absolute_time);
            return is_locked;
        }
        bool timed_lock(::boost::xtime const& absolute_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
          }
            is_locked=m->timed_lock(absolute_time);
            return is_locked;
        }

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
          }
          is_locked=m->try_lock_for(rel_time);
          return is_locked;
        }
        template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost unique_lock owns already the mutex"));
          }
          is_locked=m->try_lock_until(abs_time);
          return is_locked;
        }
#endif

        void unlock()
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock has no mutex"));
          }
            if(!owns_lock())
            {
                boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost unique_lock doesn't own the mutex"));
            }
            m->unlock();
            is_locked=false;
        }

#if defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)
        typedef void (unique_lock::*bool_type)();
        operator bool_type() const BOOST_NOEXCEPT
        {
            return is_locked?&unique_lock::lock:0;
        }
        bool operator!() const BOOST_NOEXCEPT
        {
            return !owns_lock();
        }
#else
        explicit operator bool() const BOOST_NOEXCEPT
        {
            return owns_lock();
        }
#endif
        bool owns_lock() const BOOST_NOEXCEPT
        {
            return is_locked;
        }

        Mutex* mutex() const BOOST_NOEXCEPT
        {
            return m;
        }

        Mutex* release() BOOST_NOEXCEPT
        {
            Mutex* const res=m;
            m=0;
            is_locked=false;
            return res;
        }

        friend class shared_lock<Mutex>;
        friend class upgrade_lock<Mutex>;
    };

    template<typename Mutex>
    void swap(unique_lock<Mutex>& lhs,unique_lock<Mutex>& rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }

#if defined BOOST_NO_RVALUE_REFERENCES && ! defined BOOST_THREAD_USES_MOVE
    template <typename Mutex>
    struct has_move_emulation_enabled_aux<unique_lock<Mutex> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif

    template<typename Mutex>
    class shared_lock
    {
    protected:
        Mutex* m;
        bool is_locked;
#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        shared_lock(shared_lock const&) = delete;
        shared_lock& operator=(shared_lock const&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        shared_lock(shared_lock &);
        shared_lock& operator=(shared_lock &);
#endif // BOOST_NO_DELETED_FUNCTIONS
    public:
        typedef Mutex mutex_type;

        shared_lock() BOOST_NOEXCEPT:
            m(0),is_locked(false)
        {}

        explicit shared_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        shared_lock(Mutex& m_,adopt_lock_t):
            m(&m_),is_locked(true)
        {}
        shared_lock(Mutex& m_,defer_lock_t) BOOST_NOEXCEPT:
            m(&m_),is_locked(false)
        {}
        shared_lock(Mutex& m_,try_to_lock_t):
            m(&m_),is_locked(false)
        {
            try_lock();
        }
        shared_lock(Mutex& m_,system_time const& target_time):
            m(&m_),is_locked(false)
        {
            timed_lock(target_time);
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        shared_lock(Mutex& mtx, const chrono::time_point<Clock, Duration>& t)
                : m(&mtx), is_locked(mtx.try_lock_shared_until(t))
        {
        }
        template <class Rep, class Period>
        shared_lock(Mutex& mtx, const chrono::duration<Rep, Period>& d)
                : m(&mtx), is_locked(mtx.try_lock_shared_for(d))
        {
        }
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
        shared_lock(shared_lock<Mutex> && other) BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(unique_lock<Mutex> && other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_shared();
            }
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(upgrade_lock<Mutex> && other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_upgrade_and_lock_shared();
            }
            other.is_locked=false;
            other.m=0;
        }


        shared_lock& operator=(shared_lock<Mutex> && other) BOOST_NOEXCEPT
        {
            shared_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        shared_lock& operator=(unique_lock<Mutex> && other)
        {
            shared_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }

        shared_lock& operator=(upgrade_lock<Mutex> && other)
        {
            shared_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }
#endif
#else
#if defined BOOST_THREAD_USES_MOVE
        shared_lock(boost::rv<shared_lock<Mutex> >& other) BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(boost::rv<unique_lock<Mutex> >& other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_shared();
            }
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(boost::rv<upgrade_lock<Mutex> >& other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_upgrade_and_lock_shared();
            }
            other.is_locked=false;
            other.m=0;
        }

        operator ::boost::rv<shared_lock<Mutex> >&() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<shared_lock<Mutex> >* >(this);
        }
        operator const ::boost::rv<shared_lock<Mutex> >&() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<shared_lock<Mutex> >* >(this);
        }
        ::boost::rv<shared_lock<Mutex> >& move() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<shared_lock<Mutex> >* >(this);
        }
        const ::boost::rv<shared_lock<Mutex> >& move() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<shared_lock<Mutex> >* >(this);
        }

        shared_lock& operator=(::boost::rv<shared_lock<Mutex> >& other) BOOST_NOEXCEPT
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        shared_lock& operator=(::boost::rv<unique_lock<Mutex> >& other)
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }

        shared_lock& operator=(::boost::rv<upgrade_lock<Mutex> >& other)
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }
#endif
#else

        shared_lock(detail::thread_move_t<shared_lock<Mutex> > other) BOOST_NOEXCEPT:
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            other->m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(detail::thread_move_t<unique_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_shared();
            }
            other->is_locked=false;
            other->m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION shared_lock(detail::thread_move_t<upgrade_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            if(is_locked)
            {
                m->unlock_upgrade_and_lock_shared();
            }
            other->is_locked=false;
            other->m=0;
        }

        operator detail::thread_move_t<shared_lock<Mutex> >() BOOST_NOEXCEPT
        {
            return move();
        }

        detail::thread_move_t<shared_lock<Mutex> > move() BOOST_NOEXCEPT
        {
            return detail::thread_move_t<shared_lock<Mutex> >(*this);
        }


        shared_lock& operator=(detail::thread_move_t<shared_lock<Mutex> > other) BOOST_NOEXCEPT
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        shared_lock& operator=(detail::thread_move_t<unique_lock<Mutex> > other)
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }

        shared_lock& operator=(detail::thread_move_t<upgrade_lock<Mutex> > other)
        {
            shared_lock temp(other);
            swap(temp);
            return *this;
        }
#endif
#endif
#endif

        void swap(shared_lock& other) BOOST_NOEXCEPT
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }

        Mutex* mutex() const BOOST_NOEXCEPT
        {
            return m;
        }

        Mutex* release() BOOST_NOEXCEPT
        {
            Mutex* const res=m;
            m=0;
            is_locked=false;
            return res;
        }

        ~shared_lock()
        {
            if(owns_lock())
            {
                m->unlock_shared();
            }
        }
        void lock()
        {
            if(m==0)
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
            }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
            }
            m->lock_shared();
            is_locked=true;
        }
        bool try_lock()
        {
            if(m==0)
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
            }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
            }
            is_locked=m->try_lock_shared();
            return is_locked;
        }
        bool timed_lock(boost::system_time const& target_time)
        {
            if(m==0)
            {
                boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
            }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
            }
            is_locked=m->timed_lock_shared(target_time);
            return is_locked;
        }
        template<typename Duration>
        bool timed_lock(Duration const& target_time)
        {
            if(m==0)
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
            }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
            }
            is_locked=m->timed_lock_shared(target_time);
            return is_locked;
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
          }
          is_locked=m->try_lock_shared_for(rel_time);
          return is_locked;
        }
        template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
          }
          is_locked=m->try_lock_shared_until(abs_time);
          return is_locked;
        }
#endif
        void unlock()
        {
            if(m==0)
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
            }
            if(!owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock doesn't own the mutex"));
            }
            m->unlock_shared();
            is_locked=false;
        }

#if defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)
        typedef void (shared_lock<Mutex>::*bool_type)();
        operator bool_type() const BOOST_NOEXCEPT
        {
            return is_locked?&shared_lock::lock:0;
        }
        bool operator!() const BOOST_NOEXCEPT
        {
            return !owns_lock();
        }
#else
        explicit operator bool() const BOOST_NOEXCEPT
        {
            return owns_lock();
        }
#endif
        bool owns_lock() const BOOST_NOEXCEPT
        {
            return is_locked;
        }

    };

#if defined BOOST_NO_RVALUE_REFERENCES && ! defined BOOST_THREAD_USES_MOVE
  template <typename Mutex>
  struct has_move_emulation_enabled_aux<shared_lock<Mutex> >
  : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
  {};
#endif


    template<typename Mutex>
    void swap(shared_lock<Mutex>& lhs,shared_lock<Mutex>& rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }

    template<typename Mutex>
    class upgrade_lock
    {
    protected:
        Mutex* m;
        bool is_locked;
#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        upgrade_lock(upgrade_lock const&) = delete;
        upgrade_lock& operator=(upgrade_lock const&) = delete;
#else
    private:
        explicit upgrade_lock(upgrade_lock&);
        upgrade_lock& operator=(upgrade_lock&);
#endif
    public:
        typedef Mutex mutex_type;
        upgrade_lock() BOOST_NOEXCEPT:
            m(0),is_locked(false)
        {}

        explicit upgrade_lock(Mutex& m_):
            m(&m_),is_locked(false)
        {
            lock();
        }
        upgrade_lock(Mutex& m_,adopt_lock_t):
            m(&m_),is_locked(true)
        {}
        upgrade_lock(Mutex& m_,defer_lock_t) BOOST_NOEXCEPT:
            m(&m_),is_locked(false)
        {}
        upgrade_lock(Mutex& m_,try_to_lock_t):
            m(&m_),is_locked(false)
        {
            try_lock();
        }

#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
        upgrade_lock(Mutex& mtx, const chrono::time_point<Clock, Duration>& t)
                : m(&mtx), is_locked(mtx.try_lock_upgrade_until(t))
        {
        }
        template <class Rep, class Period>
        upgrade_lock(Mutex& mtx, const chrono::duration<Rep, Period>& d)
                : m(&mtx), is_locked(mtx.try_lock_upgrade_for(d))
        {
        }
#endif

#ifndef BOOST_NO_RVALUE_REFERENCES
        upgrade_lock(upgrade_lock<Mutex>&& other) BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION upgrade_lock(unique_lock<Mutex>&& other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_upgrade();
            }
            other.is_locked=false;
            other.m=0;
        }

        upgrade_lock& operator=(upgrade_lock<Mutex>&& other) BOOST_NOEXCEPT
        {
            upgrade_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        upgrade_lock& operator=(unique_lock<Mutex>&& other)
        {
            upgrade_lock temp(::boost::move(other));
            swap(temp);
            return *this;
        }
#endif
#else
#if defined BOOST_THREAD_USES_MOVE
        upgrade_lock(boost::rv<upgrade_lock<Mutex> >& other) BOOST_NOEXCEPT:
            m(other.m),is_locked(other.is_locked)
        {
            other.is_locked=false;
            other.m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION upgrade_lock(boost::rv<unique_lock<Mutex> >& other):
            m(other.m),is_locked(other.is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_upgrade();
            }
            other.is_locked=false;
            other.m=0;
        }

        operator ::boost::rv<upgrade_lock>&() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<upgrade_lock>* >(this);
        }
        operator const ::boost::rv<upgrade_lock>&() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<upgrade_lock>* >(this);
        }
        ::boost::rv<upgrade_lock>& move() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<upgrade_lock>* >(this);
        }
        const ::boost::rv<upgrade_lock>& move() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<upgrade_lock>* >(this);
        }
        upgrade_lock& operator=(boost::rv<upgrade_lock<Mutex> >& other) BOOST_NOEXCEPT
        {
            upgrade_lock temp(other);
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        upgrade_lock& operator=(boost::rv<unique_lock<Mutex> >& other)
        {
            upgrade_lock temp(other);
            swap(temp);
            return *this;
        }
#endif
#else
        upgrade_lock(detail::thread_move_t<upgrade_lock<Mutex> > other) BOOST_NOEXCEPT:
            m(other->m),is_locked(other->is_locked)
        {
            other->is_locked=false;
            other->m=0;
        }

        BOOST_THREAD_EXPLICIT_LOCK_CONVERSION upgrade_lock(detail::thread_move_t<unique_lock<Mutex> > other):
            m(other->m),is_locked(other->is_locked)
        {
            if(is_locked)
            {
                m->unlock_and_lock_upgrade();
            }
            other->is_locked=false;
            other->m=0;
        }

        operator detail::thread_move_t<upgrade_lock<Mutex> >() BOOST_NOEXCEPT
        {
            return move();
        }

        detail::thread_move_t<upgrade_lock<Mutex> > move() BOOST_NOEXCEPT
        {
            return detail::thread_move_t<upgrade_lock<Mutex> >(*this);
        }


        upgrade_lock& operator=(detail::thread_move_t<upgrade_lock<Mutex> > other) BOOST_NOEXCEPT
        {
            upgrade_lock temp(other);
            swap(temp);
            return *this;
        }

#ifndef BOOST_THREAD_PROVIDES_EXPLICIT_LOCK_CONVERSION
        upgrade_lock& operator=(detail::thread_move_t<unique_lock<Mutex> > other)
        {
            upgrade_lock temp(other);
            swap(temp);
            return *this;
        }
#endif
#endif
#endif

#ifdef BOOST_THREAD_PROVIDES_SHARED_MUTEX_UPWARDS_CONVERSIONSS
#ifndef BOOST_NO_RVALUE_REFERENCES
        // Conversion from shared locking

        upgrade_lock(shared_lock<mutex_type>&& sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
            upgrade_lock(shared_lock<mutex_type>&& sl,
                         const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_until(abs_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

        template <class Rep, class Period>
            upgrade_lock(shared_lock<mutex_type>&& sl,
                         const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_for(rel_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#endif
#else


#if defined BOOST_THREAD_USES_MOVE
        // Conversion from shared locking

        upgrade_lock(boost::rv<shared_lock<mutex_type> > &sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
            upgrade_lock(boost::rv<shared_lock<mutex_type> > &sl,
                         const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_until(abs_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

        template <class Rep, class Period>
            upgrade_lock(boost::rv<shared_lock<mutex_type> > &sl,
                         const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_for(rel_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#endif
#else
        // Conversion from shared locking

        upgrade_lock(detail::thread_move_t<shared_lock<mutex_type> > sl, try_to_lock_t)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade())
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Clock, class Duration>
            upgrade_lock(detail::thread_move_t<shared_lock<mutex_type> > sl,
                         const chrono::time_point<Clock, Duration>& abs_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_until(abs_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }

        template <class Rep, class Period>
            upgrade_lock(detail::thread_move_t<shared_lock<mutex_type> >& sl,
                         const chrono::duration<Rep, Period>& rel_time)
        : m(0),is_locked(false)
        {
          if (sl.owns_lock()) {
            if (sl.mutex()->try_unlock_shared_and_lock_upgrade_for(rel_time))
            {
                m = sl.release();
                is_locked = true;
            }
          }
          else
          {
            m = sl.release();
          }
        }
#endif
#endif
#endif
#endif

        void swap(upgrade_lock& other) BOOST_NOEXCEPT
        {
            std::swap(m,other.m);
            std::swap(is_locked,other.is_locked);
        }
        Mutex* mutex() const BOOST_NOEXCEPT
        {
            return m;
        }

        Mutex* release() BOOST_NOEXCEPT
        {
            Mutex* const res=m;
            m=0;
            is_locked=false;
            return res;
        }
        ~upgrade_lock()
        {
            if(owns_lock())
            {
                m->unlock_upgrade();
            }
        }
        void lock()
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost upgrade_lock owns already the mutex"));
            }
            m->lock_upgrade();
            is_locked=true;
        }
        bool try_lock()
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
            if(owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost upgrade_lock owns already the mutex"));
            }
            is_locked=m->try_lock_upgrade();
            return is_locked;
        }
        void unlock()
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
            if(!owns_lock())
            {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost upgrade_lock doesn't own the mutex"));
            }
            m->unlock_upgrade();
            is_locked=false;
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        bool try_lock_for(const chrono::duration<Rep, Period>& rel_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
          }
          is_locked=m->try_lock_upgrade_for(rel_time);
          return is_locked;
        }
        template <class Clock, class Duration>
        bool try_lock_until(const chrono::time_point<Clock, Duration>& abs_time)
        {
          if(m==0)
          {
              boost::throw_exception(boost::lock_error(system::errc::operation_not_permitted, "boost shared_lock has no mutex"));
          }
          if(owns_lock())
          {
              boost::throw_exception(boost::lock_error(system::errc::resource_deadlock_would_occur, "boost shared_lock owns already the mutex"));
          }
          is_locked=m->try_lock_upgrade_until(abs_time);
          return is_locked;
        }
#endif
#if defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)
        typedef void (upgrade_lock::*bool_type)();
        operator bool_type() const BOOST_NOEXCEPT
        {
            return is_locked?&upgrade_lock::lock:0;
        }
        bool operator!() const BOOST_NOEXCEPT
        {
            return !owns_lock();
        }
#else
        explicit operator bool() const BOOST_NOEXCEPT
        {
            return owns_lock();
        }
#endif
        bool owns_lock() const BOOST_NOEXCEPT
        {
            return is_locked;
        }
        friend class shared_lock<Mutex>;
        friend class unique_lock<Mutex>;
    };

    template<typename Mutex>
    void swap(upgrade_lock<Mutex>& lhs,upgrade_lock<Mutex>& rhs) BOOST_NOEXCEPT
    {
        lhs.swap(rhs);
    }

#if defined BOOST_NO_RVALUE_REFERENCES && ! defined BOOST_THREAD_USES_MOVE
    template <typename Mutex>
    struct has_move_emulation_enabled_aux<upgrade_lock<Mutex> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif


#ifndef BOOST_NO_RVALUE_REFERENCES
    template<typename Mutex>
    unique_lock<Mutex>::unique_lock(upgrade_lock<Mutex>&& other):
        m(other.m),is_locked(other.is_locked)
    {
        if(is_locked)
        {
            m->unlock_upgrade_and_lock();
        }
        other.release();
    }
#else
#if defined BOOST_THREAD_USES_MOVE
    template<typename Mutex>
    unique_lock<Mutex>::unique_lock(boost::rv<upgrade_lock<Mutex> >& other):
        m(other.m),is_locked(other.is_locked)
    {
        if(is_locked)
        {
            m->unlock_upgrade_and_lock();
        }
        other.release();
    }
#else
    template<typename Mutex>
    unique_lock<Mutex>::unique_lock(detail::thread_move_t<upgrade_lock<Mutex> > other):
        m(other->m),is_locked(other->is_locked)
    {
        if(is_locked)
        {
            m->unlock_upgrade_and_lock();
        }
        other.release();
    }
#endif
#endif
    template <class Mutex>
    class upgrade_to_unique_lock
    {
    private:
        upgrade_lock<Mutex>* source;
        unique_lock<Mutex> exclusive;

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        upgrade_to_unique_lock(upgrade_to_unique_lock const&) = delete;
        upgrade_to_unique_lock& operator=(upgrade_to_unique_lock const&) = delete;
#else
    private:
        explicit upgrade_to_unique_lock(upgrade_to_unique_lock&);
        upgrade_to_unique_lock& operator=(upgrade_to_unique_lock&);
#endif
    public:
        typedef Mutex mutex_type;
        explicit upgrade_to_unique_lock(upgrade_lock<Mutex>& m_):
            source(&m_),exclusive(::boost::move(*source))
        {}
        ~upgrade_to_unique_lock()
        {
            if(source)
            {
                *source=BOOST_EXPLICIT_MOVE(upgrade_lock<Mutex>(::boost::move(exclusive)));
            }
        }

#ifndef BOOST_NO_RVALUE_REFERENCES
        upgrade_to_unique_lock(upgrade_to_unique_lock<Mutex>&& other) BOOST_NOEXCEPT:
            source(other.source),exclusive(::boost::move(other.exclusive))
        {
            other.source=0;
        }

        upgrade_to_unique_lock& operator=(upgrade_to_unique_lock<Mutex>&& other) BOOST_NOEXCEPT
        {
            upgrade_to_unique_lock temp(other);
            swap(temp);
            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        upgrade_to_unique_lock(boost::rv<upgrade_to_unique_lock<Mutex> >& other) BOOST_NOEXCEPT:
            source(other.source),exclusive(::boost::move(other.exclusive))
        {
            other.source=0;
        }

        upgrade_to_unique_lock& operator=(boost::rv<upgrade_to_unique_lock<Mutex> >& other) BOOST_NOEXCEPT
        {
            upgrade_to_unique_lock temp(other);
            swap(temp);
            return *this;
        }
        operator ::boost::rv<upgrade_to_unique_lock>&() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<upgrade_to_unique_lock>* >(this);
        }
        operator const ::boost::rv<upgrade_to_unique_lock>&() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<upgrade_to_unique_lock>* >(this);
        }
        ::boost::rv<upgrade_to_unique_lock>& move() BOOST_NOEXCEPT
        {
          return *static_cast< ::boost::rv<upgrade_to_unique_lock>* >(this);
        }
        const ::boost::rv<upgrade_to_unique_lock>& move() const BOOST_NOEXCEPT
        {
          return *static_cast<const ::boost::rv<upgrade_to_unique_lock>* >(this);
        }
#else
        upgrade_to_unique_lock(detail::thread_move_t<upgrade_to_unique_lock<Mutex> > other) BOOST_NOEXCEPT:
            source(other->source),exclusive(::boost::move(other->exclusive))
        {
            other->source=0;
        }

        upgrade_to_unique_lock& operator=(detail::thread_move_t<upgrade_to_unique_lock<Mutex> > other) BOOST_NOEXCEPT
        {
            upgrade_to_unique_lock temp(other);
            swap(temp);
            return *this;
        }
        operator detail::thread_move_t<upgrade_to_unique_lock<Mutex> >() BOOST_NOEXCEPT
        {
            return move();
        }

        detail::thread_move_t<upgrade_to_unique_lock<Mutex> > move() BOOST_NOEXCEPT
        {
            return detail::thread_move_t<upgrade_to_unique_lock<Mutex> >(*this);
        }
#endif
#endif
        void swap(upgrade_to_unique_lock& other) BOOST_NOEXCEPT
        {
            std::swap(source,other.source);
            exclusive.swap(other.exclusive);
        }

#if defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)
        typedef void (upgrade_to_unique_lock::*bool_type)(upgrade_to_unique_lock&);
        operator bool_type() const BOOST_NOEXCEPT
        {
            return exclusive.owns_lock()?&upgrade_to_unique_lock::swap:0;
        }
        bool operator!() const BOOST_NOEXCEPT
        {
            return !owns_lock();
        }
#else
        explicit operator bool() const BOOST_NOEXCEPT
        {
            return owns_lock();
        }
#endif

        bool owns_lock() const BOOST_NOEXCEPT
        {
            return exclusive.owns_lock();
        }
    };

#if defined BOOST_NO_RVALUE_REFERENCES && ! defined BOOST_THREAD_USES_MOVE
    template <typename Mutex>
    struct has_move_emulation_enabled_aux<upgrade_to_unique_lock<Mutex> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif

    namespace detail
    {
        template<typename Mutex>
        class try_lock_wrapper:
            private unique_lock<Mutex>
        {
            typedef unique_lock<Mutex> base;
        public:
            try_lock_wrapper()
            {}

            explicit try_lock_wrapper(Mutex& m):
                base(m,try_to_lock)
            {}

            try_lock_wrapper(Mutex& m_,adopt_lock_t):
                base(m_,adopt_lock)
            {}
            try_lock_wrapper(Mutex& m_,defer_lock_t):
                base(m_,defer_lock)
            {}
            try_lock_wrapper(Mutex& m_,try_to_lock_t):
                base(m_,try_to_lock)
            {}
#ifndef BOOST_NO_RVALUE_REFERENCES
            try_lock_wrapper(try_lock_wrapper&& other):
                base(::boost::move(other))
            {}

            try_lock_wrapper& operator=(try_lock_wrapper<Mutex>&& other)
            {
                try_lock_wrapper temp(::boost::move(other));
                swap(temp);
                return *this;
            }

#else
#if defined BOOST_THREAD_USES_MOVE
            try_lock_wrapper(boost::rv<try_lock_wrapper<Mutex> >& other):
                base(::boost::move(static_cast<base&>(other)))
            {}

            operator ::boost::rv<try_lock_wrapper>&()
            {
              return *static_cast< ::boost::rv<try_lock_wrapper>* >(this);
            }
            operator const ::boost::rv<try_lock_wrapper>&() const
            {
              return *static_cast<const ::boost::rv<try_lock_wrapper>* >(this);
            }
            ::boost::rv<try_lock_wrapper>& move()
            {
              return *static_cast< ::boost::rv<try_lock_wrapper>* >(this);
            }
            const ::boost::rv<try_lock_wrapper>& move() const
            {
              return *static_cast<const ::boost::rv<try_lock_wrapper>* >(this);
            }

            try_lock_wrapper& operator=(boost::rv<try_lock_wrapper<Mutex> >& other)
            {
                try_lock_wrapper temp(other);
                swap(temp);
                return *this;
            }

#else
            try_lock_wrapper(detail::thread_move_t<try_lock_wrapper<Mutex> > other):
                base(detail::thread_move_t<base>(*other))
            {}

            operator detail::thread_move_t<try_lock_wrapper<Mutex> >()
            {
                return move();
            }

            detail::thread_move_t<try_lock_wrapper<Mutex> > move()
            {
                return detail::thread_move_t<try_lock_wrapper<Mutex> >(*this);
            }

            try_lock_wrapper& operator=(detail::thread_move_t<try_lock_wrapper<Mutex> > other)
            {
                try_lock_wrapper temp(other);
                swap(temp);
                return *this;
            }

            void swap(detail::thread_move_t<try_lock_wrapper<Mutex> > other)
            {
                base::swap(*other);
            }
#endif
#endif
            void swap(try_lock_wrapper& other)
            {
                base::swap(other);
            }
            void lock()
            {
                base::lock();
            }
            bool try_lock()
            {
                return base::try_lock();
            }
            void unlock()
            {
                base::unlock();
            }
            bool owns_lock() const
            {
                return base::owns_lock();
            }
            Mutex* mutex() const
            {
                return base::mutex();
            }
            Mutex* release()
            {
                return base::release();
            }

#if defined(BOOST_NO_EXPLICIT_CONVERSION_OPERATORS)
            typedef typename base::bool_type bool_type;
            operator bool_type() const
            {
                return base::operator bool_type();
            }
            bool operator!() const
            {
                return !this->owns_lock();
            }
#else
            explicit operator bool() const
            {
                return owns_lock();
            }
#endif
        };

        template<typename Mutex>
        void swap(try_lock_wrapper<Mutex>& lhs,try_lock_wrapper<Mutex>& rhs)
        {
            lhs.swap(rhs);
        }

        template<typename MutexType1,typename MutexType2>
        unsigned try_lock_internal(MutexType1& m1,MutexType2& m2)
        {
            boost::unique_lock<MutexType1> l1(m1,boost::try_to_lock);
            if(!l1)
            {
                return 1;
            }
            if(!m2.try_lock())
            {
                return 2;
            }
            l1.release();
            return 0;
        }

        template<typename MutexType1,typename MutexType2,typename MutexType3>
        unsigned try_lock_internal(MutexType1& m1,MutexType2& m2,MutexType3& m3)
        {
            boost::unique_lock<MutexType1> l1(m1,boost::try_to_lock);
            if(!l1)
            {
                return 1;
            }
            if(unsigned const failed_lock=try_lock_internal(m2,m3))
            {
                return failed_lock+1;
            }
            l1.release();
            return 0;
        }


        template<typename MutexType1,typename MutexType2,typename MutexType3,
                 typename MutexType4>
        unsigned try_lock_internal(MutexType1& m1,MutexType2& m2,MutexType3& m3,
                                   MutexType4& m4)
        {
            boost::unique_lock<MutexType1> l1(m1,boost::try_to_lock);
            if(!l1)
            {
                return 1;
            }
            if(unsigned const failed_lock=try_lock_internal(m2,m3,m4))
            {
                return failed_lock+1;
            }
            l1.release();
            return 0;
        }

        template<typename MutexType1,typename MutexType2,typename MutexType3,
                 typename MutexType4,typename MutexType5>
        unsigned try_lock_internal(MutexType1& m1,MutexType2& m2,MutexType3& m3,
                                   MutexType4& m4,MutexType5& m5)
        {
            boost::unique_lock<MutexType1> l1(m1,boost::try_to_lock);
            if(!l1)
            {
                return 1;
            }
            if(unsigned const failed_lock=try_lock_internal(m2,m3,m4,m5))
            {
                return failed_lock+1;
            }
            l1.release();
            return 0;
        }


        template<typename MutexType1,typename MutexType2>
        unsigned lock_helper(MutexType1& m1,MutexType2& m2)
        {
            boost::unique_lock<MutexType1> l1(m1);
            if(!m2.try_lock())
            {
                return 1;
            }
            l1.release();
            return 0;
        }

        template<typename MutexType1,typename MutexType2,typename MutexType3>
        unsigned lock_helper(MutexType1& m1,MutexType2& m2,MutexType3& m3)
        {
            boost::unique_lock<MutexType1> l1(m1);
            if(unsigned const failed_lock=try_lock_internal(m2,m3))
            {
                return failed_lock;
            }
            l1.release();
            return 0;
        }

        template<typename MutexType1,typename MutexType2,typename MutexType3,
                 typename MutexType4>
        unsigned lock_helper(MutexType1& m1,MutexType2& m2,MutexType3& m3,
                             MutexType4& m4)
        {
            boost::unique_lock<MutexType1> l1(m1);
            if(unsigned const failed_lock=try_lock_internal(m2,m3,m4))
            {
                return failed_lock;
            }
            l1.release();
            return 0;
        }

        template<typename MutexType1,typename MutexType2,typename MutexType3,
                 typename MutexType4,typename MutexType5>
        unsigned lock_helper(MutexType1& m1,MutexType2& m2,MutexType3& m3,
                             MutexType4& m4,MutexType5& m5)
        {
            boost::unique_lock<MutexType1> l1(m1);
            if(unsigned const failed_lock=try_lock_internal(m2,m3,m4,m5))
            {
                return failed_lock;
            }
            l1.release();
            return 0;
        }
    }

    namespace detail
    {
        template<bool x>
        struct is_mutex_type_wrapper
        {};

        template<typename MutexType1,typename MutexType2>
        void lock_impl(MutexType1& m1,MutexType2& m2,is_mutex_type_wrapper<true>)
        {
            unsigned const lock_count=2;
            unsigned lock_first=0;
            for(;;)
            {
                switch(lock_first)
                {
                case 0:
                    lock_first=detail::lock_helper(m1,m2);
                    if(!lock_first)
                        return;
                    break;
                case 1:
                    lock_first=detail::lock_helper(m2,m1);
                    if(!lock_first)
                        return;
                    lock_first=(lock_first+1)%lock_count;
                    break;
                }
            }
        }

        template<typename Iterator>
        void lock_impl(Iterator begin,Iterator end,is_mutex_type_wrapper<false>);
    }


    template<typename MutexType1,typename MutexType2>
    void lock(MutexType1& m1,MutexType2& m2)
    {
        detail::lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    void lock(const MutexType1& m1,MutexType2& m2)
    {
        detail::lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    void lock(MutexType1& m1,const MutexType2& m2)
    {
        detail::lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    void lock(const MutexType1& m1,const MutexType2& m2)
    {
        detail::lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3>
    void lock(MutexType1& m1,MutexType2& m2,MutexType3& m3)
    {
        unsigned const lock_count=3;
        unsigned lock_first=0;
        for(;;)
        {
            switch(lock_first)
            {
            case 0:
                lock_first=detail::lock_helper(m1,m2,m3);
                if(!lock_first)
                    return;
                break;
            case 1:
                lock_first=detail::lock_helper(m2,m3,m1);
                if(!lock_first)
                    return;
                lock_first=(lock_first+1)%lock_count;
                break;
            case 2:
                lock_first=detail::lock_helper(m3,m1,m2);
                if(!lock_first)
                    return;
                lock_first=(lock_first+2)%lock_count;
                break;
            }
        }
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3,
             typename MutexType4>
    void lock(MutexType1& m1,MutexType2& m2,MutexType3& m3,
              MutexType4& m4)
    {
        unsigned const lock_count=4;
        unsigned lock_first=0;
        for(;;)
        {
            switch(lock_first)
            {
            case 0:
                lock_first=detail::lock_helper(m1,m2,m3,m4);
                if(!lock_first)
                    return;
                break;
            case 1:
                lock_first=detail::lock_helper(m2,m3,m4,m1);
                if(!lock_first)
                    return;
                lock_first=(lock_first+1)%lock_count;
                break;
            case 2:
                lock_first=detail::lock_helper(m3,m4,m1,m2);
                if(!lock_first)
                    return;
                lock_first=(lock_first+2)%lock_count;
                break;
            case 3:
                lock_first=detail::lock_helper(m4,m1,m2,m3);
                if(!lock_first)
                    return;
                lock_first=(lock_first+3)%lock_count;
                break;
            }
        }
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3,
             typename MutexType4,typename MutexType5>
    void lock(MutexType1& m1,MutexType2& m2,MutexType3& m3,
              MutexType4& m4,MutexType5& m5)
    {
        unsigned const lock_count=5;
        unsigned lock_first=0;
        for(;;)
        {
            switch(lock_first)
            {
            case 0:
                lock_first=detail::lock_helper(m1,m2,m3,m4,m5);
                if(!lock_first)
                    return;
                break;
            case 1:
                lock_first=detail::lock_helper(m2,m3,m4,m5,m1);
                if(!lock_first)
                    return;
                lock_first=(lock_first+1)%lock_count;
                break;
            case 2:
                lock_first=detail::lock_helper(m3,m4,m5,m1,m2);
                if(!lock_first)
                    return;
                lock_first=(lock_first+2)%lock_count;
                break;
            case 3:
                lock_first=detail::lock_helper(m4,m5,m1,m2,m3);
                if(!lock_first)
                    return;
                lock_first=(lock_first+3)%lock_count;
                break;
            case 4:
                lock_first=detail::lock_helper(m5,m1,m2,m3,m4);
                if(!lock_first)
                    return;
                lock_first=(lock_first+4)%lock_count;
                break;
            }
        }
    }

    namespace detail
    {
        template<typename Mutex,bool x=is_mutex_type<Mutex>::value>
        struct try_lock_impl_return
        {
            typedef int type;
        };

        template<typename Iterator>
        struct try_lock_impl_return<Iterator,false>
        {
            typedef Iterator type;
        };

        template<typename MutexType1,typename MutexType2>
        int try_lock_impl(MutexType1& m1,MutexType2& m2,is_mutex_type_wrapper<true>)
        {
            return ((int)detail::try_lock_internal(m1,m2))-1;
        }

        template<typename Iterator>
        Iterator try_lock_impl(Iterator begin,Iterator end,is_mutex_type_wrapper<false>);
    }

    template<typename MutexType1,typename MutexType2>
    typename detail::try_lock_impl_return<MutexType1>::type try_lock(MutexType1& m1,MutexType2& m2)
    {
        return detail::try_lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    typename detail::try_lock_impl_return<MutexType1>::type try_lock(const MutexType1& m1,MutexType2& m2)
    {
        return detail::try_lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    typename detail::try_lock_impl_return<MutexType1>::type try_lock(MutexType1& m1,const MutexType2& m2)
    {
        return detail::try_lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2>
    typename detail::try_lock_impl_return<MutexType1>::type try_lock(const MutexType1& m1,const MutexType2& m2)
    {
        return detail::try_lock_impl(m1,m2,detail::is_mutex_type_wrapper<is_mutex_type<MutexType1>::value>());
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3>
    int try_lock(MutexType1& m1,MutexType2& m2,MutexType3& m3)
    {
        return ((int)detail::try_lock_internal(m1,m2,m3))-1;
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3,typename MutexType4>
    int try_lock(MutexType1& m1,MutexType2& m2,MutexType3& m3,MutexType4& m4)
    {
        return ((int)detail::try_lock_internal(m1,m2,m3,m4))-1;
    }

    template<typename MutexType1,typename MutexType2,typename MutexType3,typename MutexType4,typename MutexType5>
    int try_lock(MutexType1& m1,MutexType2& m2,MutexType3& m3,MutexType4& m4,MutexType5& m5)
    {
        return ((int)detail::try_lock_internal(m1,m2,m3,m4,m5))-1;
    }


    namespace detail
    {
        template<typename Iterator>
        struct range_lock_guard
        {
            Iterator begin;
            Iterator end;

            range_lock_guard(Iterator begin_,Iterator end_):
                begin(begin_),end(end_)
            {
                boost::lock(begin,end);
            }

            void release()
            {
                begin=end;
            }

            ~range_lock_guard()
            {
                for(;begin!=end;++begin)
                {
                    begin->unlock();
                }
            }
        };

        template<typename Iterator>
        Iterator try_lock_impl(Iterator begin,Iterator end,is_mutex_type_wrapper<false>)

        {
            if(begin==end)
            {
                return end;
            }
            typedef typename std::iterator_traits<Iterator>::value_type lock_type;
            unique_lock<lock_type> guard(*begin,try_to_lock);

            if(!guard.owns_lock())
            {
                return begin;
            }
            Iterator const failed=boost::try_lock(++begin,end);
            if(failed==end)
            {
                guard.release();
            }

            return failed;
        }
    }


    namespace detail
    {
        template<typename Iterator>
        void lock_impl(Iterator begin,Iterator end,is_mutex_type_wrapper<false>)
        {
            typedef typename std::iterator_traits<Iterator>::value_type lock_type;

            if(begin==end)
            {
                return;
            }
            bool start_with_begin=true;
            Iterator second=begin;
            ++second;
            Iterator next=second;

            for(;;)
            {
                unique_lock<lock_type> begin_lock(*begin,defer_lock);
                if(start_with_begin)
                {
                    begin_lock.lock();
                    Iterator const failed_lock=boost::try_lock(next,end);
                    if(failed_lock==end)
                    {
                        begin_lock.release();
                        return;
                    }
                    start_with_begin=false;
                    next=failed_lock;
                }
                else
                {
                    detail::range_lock_guard<Iterator> guard(next,end);
                    if(begin_lock.try_lock())
                    {
                        Iterator const failed_lock=boost::try_lock(second,next);
                        if(failed_lock==next)
                        {
                            begin_lock.release();
                            guard.release();
                            return;
                        }
                        start_with_begin=false;
                        next=failed_lock;
                    }
                    else
                    {
                        start_with_begin=true;
                        next=second;
                    }
                }
            }
        }

    }

}

#include <boost/config/abi_suffix.hpp>

#endif
