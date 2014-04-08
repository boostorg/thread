#ifndef BOOST_THREAD_PERMIT_PTHREAD_HPP
#define BOOST_THREAD_PERMIT_PTHREAD_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-10 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba
// (C) Copyright 2014 Niall Douglas

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <pthread.h>
#include <boost/thread/cv_status.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/pthread/timespec.hpp>
#include <boost/thread/detail/delete.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
#include <boost/thread/pthread/thread_data.hpp>
#endif

// Set up the C11 pthreads permit implementation to use an internal C++ namespace
// to prevent it colliding with anything else.
#ifdef PTHREAD_PERMIT_H
#error The C11 pthreads permit implementation is already known to this compilation unit. I cannot proceed.
#endif
#define PTHREAD_PERMIT_USE_BOOST
#define PTHREAD_PERMIT_CXX_NAMESPACE_BEGIN namespace boost { namespace c_permit {
#define PTHREAD_PERMIT_CXX_NAMESPACE_END } }
#define PTHREAD_PERMIT_MANGLEAPI(api) pthread_##api
#define PTHREAD_PERMIT_HEADER_ONLY 1
#include "../permit/pthread_permit.h"

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        template<bool consuming> struct permit_impl_selector
        {          
            typedef boost::c_permit::pthread_permit1_t pthread_permit_t;
            static  int pthread_permit_init     (pthread_permit_t *permit, bool initial)                                    { return boost::c_permit::pthread_permit1_init     (permit, initial); }
            static void pthread_permit_destroy  (pthread_permit_t *permit)                                                  {        boost::c_permit::pthread_permit1_destroy  (permit); }
            static  int pthread_permit_grant    (pthread_permit_t *permit)                                                  { return boost::c_permit::pthread_permit1_grant    (permit); }
            static void pthread_permit_revoke   (pthread_permit_t *permit)                                                  {        boost::c_permit::pthread_permit1_revoke   (permit); }
            static  int pthread_permit_wait     (pthread_permit_t *permit, pthread_mutex_t *mtx)                            { return boost::c_permit::pthread_permit1_wait     (permit, mtx); }
            static  int pthread_permit_timedwait(pthread_permit_t *permit, pthread_mutex_t *mtx, const struct timespec *ts) { return boost::c_permit::pthread_permit1_timedwait(permit, mtx, ts); }
        };
        template<> struct permit_impl_selector<true>
        {          
            typedef boost::c_permit::pthread_permitnc_t pthread_permit_t;
            static  int pthread_permit_init     (pthread_permit_t *permit, bool initial)                                    { return boost::c_permit::pthread_permitnc_init     (permit, initial); }
            static void pthread_permit_destroy  (pthread_permit_t *permit)                                                  {        boost::c_permit::pthread_permitnc_destroy  (permit); }
            static  int pthread_permit_grant    (pthread_permit_t *permit)                                                  { return boost::c_permit::pthread_permitnc_grant    (permit); }
            static void pthread_permit_revoke   (pthread_permit_t *permit)                                                  {        boost::c_permit::pthread_permitnc_revoke   (permit); }
            static  int pthread_permit_wait     (pthread_permit_t *permit, pthread_mutex_t *mtx)                            { return boost::c_permit::pthread_permitnc_wait     (permit, mtx); }
            static  int pthread_permit_timedwait(pthread_permit_t *permit, pthread_mutex_t *mtx, const struct timespec *ts) { return boost::c_permit::pthread_permitnc_timedwait(permit, mtx, ts); }
        };
    }

    template<bool consuming=true> class permit : private detail::permit_impl_selector<consuming>
    {
      typedef typename detail::permit_impl_selector<consuming>::pthread_permit_t pthread_permit_t;
    private:
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        pthread_mutex_t internal_mutex;
#endif
        pthread_permit_t perm;

    public:
    //private: // used by boost::thread::try_join_until

        inline bool do_wait_until(
            unique_lock<mutex>& lock,
            struct timespec const &timeout);

        bool do_wait_for(
            unique_lock<mutex>& lock,
            struct timespec const &timeout)
        {
          return do_wait_until(lock, boost::detail::timespec_plus(timeout, boost::detail::timespec_now()));
        }

    public:
      BOOST_THREAD_NO_COPYABLE(permit)
        permit(bool initial_state=false)
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            int const res=pthread_mutex_init(&internal_mutex,NULL);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost::permit::permit() constructor failed in pthread_mutex_init"));
            }
#endif
            int const res2=pthread_permit_init(&perm,initial_state);
            if(res2)
            {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
#endif
                boost::throw_exception(thread_resource_error(res2, "boost::permit::permit() constructor failed in pthread_permit_init"));
            }
        }
        ~permit()
        {
            int ret;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            do {
              ret = pthread_mutex_destroy(&internal_mutex);
            } while (ret == EINTR);
            BOOST_ASSERT(!ret);
#endif
            pthread_permit_destroy(&perm);
        }

        void wait(unique_lock<mutex>& m);

        template<typename predicate_type>
        void wait(unique_lock<mutex>& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }


#if defined BOOST_THREAD_USES_DATETIME
        inline bool timed_wait(
            unique_lock<mutex>& m,
            boost::system_time const& abs_time)
        {
#if defined BOOST_THREAD_WAIT_BUG
            struct timespec const timeout=detail::to_timespec(abs_time + BOOST_THREAD_WAIT_BUG);
            return do_wait_until(m, timeout);
#else
            struct timespec const timeout=detail::to_timespec(abs_time);
            return do_wait_until(m, timeout);
#endif
        }
        bool timed_wait(
            unique_lock<mutex>& m,
            xtime const& abs_time)
        {
            return timed_wait(m,system_time(abs_time));
        }

        template<typename duration_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            duration_type const& wait_duration)
        {
            return timed_wait(m,get_system_time()+wait_duration);
        }

        template<typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            boost::system_time const& abs_time,predicate_type pred)
        {
            while (!pred())
            {
                if(!timed_wait(m, abs_time))
                    return pred();
            }
            return true;
        }

        template<typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            xtime const& abs_time,predicate_type pred)
        {
            return timed_wait(m,system_time(abs_time),pred);
        }

        template<typename duration_type,typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            duration_type const& wait_duration,predicate_type pred)
        {
            return timed_wait(m,get_system_time()+wait_duration,pred);
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<chrono::system_clock, Duration>& t)
        {
          using namespace chrono;
          typedef time_point<system_clock, nanoseconds> nano_sys_tmpt;
          wait_until(lock,
                        nano_sys_tmpt(ceil<nanoseconds>(t.time_since_epoch())));
          return system_clock::now() < t ? cv_status::no_timeout :
                                             cv_status::timeout;
        }

        template <class Clock, class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
          using namespace chrono;
          system_clock::time_point     s_now = system_clock::now();
          typename Clock::time_point  c_now = Clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(t - c_now));
          return Clock::now() < t ? cv_status::no_timeout : cv_status::timeout;
        }

        template <class Clock, class Duration, class Predicate>
        bool
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            while (!pred())
            {
                if (wait_until(lock, t) == cv_status::timeout)
                    return pred();
            }
            return true;
        }


        template <class Rep, class Period>
        cv_status
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d)
        {
          using namespace chrono;
          system_clock::time_point s_now = system_clock::now();
          steady_clock::time_point c_now = steady_clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(d));
          return steady_clock::now() - c_now < d ? cv_status::no_timeout :
                                                   cv_status::timeout;

        }


        template <class Rep, class Period, class Predicate>
        bool
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));

//          while (!pred())
//          {
//              if (wait_for(lock, d) == cv_status::timeout)
//                  return pred();
//          }
//          return true;
        }
#endif

#define BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE
        typedef pthread_permit_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &perm;
        }

        void grant() BOOST_NOEXCEPT;
        void revoke() BOOST_NOEXCEPT;

        void notify_one() BOOST_NOEXCEPT { grant(); }
        void notify_all() BOOST_NOEXCEPT { grant(); }
        
#ifdef BOOST_THREAD_USES_CHRONO
        inline cv_status wait_until(
            unique_lock<mutex>& lk,
            chrono::time_point<chrono::system_clock, chrono::nanoseconds> tp)
        {
            using namespace chrono;
            nanoseconds d = tp.time_since_epoch();
            timespec ts = boost::detail::to_timespec(d);
            if (do_wait_until(lk, ts)) return cv_status::no_timeout;
            else return cv_status::timeout;
        }
#endif
    };



#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
    namespace this_thread
    {
        void BOOST_THREAD_DECL interruption_point();
    }
#endif

    namespace thread_cv_detail
    {
        template<typename MutexType>
        struct lock_on_exit
        {
            MutexType* m;

            lock_on_exit():
                m(0)
            {}

            void activate(MutexType& m_)
            {
                m_.unlock();
                m=&m_;
            }
            ~lock_on_exit()
            {
                if(m)
                {
                    m->lock();
                }
           }
        };
    }

    template<bool consuming> inline void permit<consuming>::wait(unique_lock<mutex>& m)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if(! m.owns_lock())
        {
            boost::throw_exception(condition_error(-1, "boost::permit::wait() failed precondition mutex not owned"));
        }
#endif
        int res=0;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            thread_cv_detail::lock_on_exit<unique_lock<mutex> > guard;
            detail::interruption_checker check_for_interruption(&internal_mutex,pthread_permit_get_internal_cond(&perm));
            guard.activate(m);
            do {
              res = pthread_permit_wait(&perm,&internal_mutex);
            } while (res == EINTR);
#else
            //boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            do {
              res = pthread_permit_wait(&perm,the_mutex);
            } while (res == EINTR);
#endif
        }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        this_thread::interruption_point();
#endif
        if(res)
        {
            boost::throw_exception(condition_error(res, "boost::permit::wait failed in pthread_permit_wait"));
        }
    }

    template<bool consuming> inline bool permit<consuming>::do_wait_until(
                unique_lock<mutex>& m,
                struct timespec const &timeout)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if (!m.owns_lock())
        {
            boost::throw_exception(condition_error(EPERM, "boost::permit::do_wait_until() failed precondition mutex not owned"));
        }
#endif
        thread_cv_detail::lock_on_exit<unique_lock<mutex> > guard;
        int cond_res;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            detail::interruption_checker check_for_interruption(&internal_mutex,pthread_permit_get_internal_cond(&perm));
            guard.activate(m);
            cond_res=pthread_permit_timedwait(&perm,&internal_mutex,&timeout);
#else
            //boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            cond_res=pthread_permit_timedwait(&perm,the_mutex,&timeout);
#endif
        }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        this_thread::interruption_point();
#endif
        if(cond_res==ETIMEDOUT)
        {
            return false;
        }
        if(cond_res)
        {
            boost::throw_exception(condition_error(cond_res, "boost::permit::do_wait_until failed in pthread_permit_timedwait"));
        }
        return true;
    }

    template<bool consuming> inline void permit<consuming>::grant() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_permit_grant(&perm));
    }

    template<bool consuming> inline void permit<consuming>::revoke() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_permit_revoke(&perm));
    }

    template<bool consuming=true> class permit_any : private detail::permit_impl_selector<consuming>
    {
        typedef typename detail::permit_impl_selector<consuming>::pthread_permit_t pthread_permit_t;
        pthread_mutex_t internal_mutex;
        pthread_permit_t perm;

    public:
        BOOST_THREAD_NO_COPYABLE(permit_any)
        permit_any(bool initial_state=false)
        {
            int const res=pthread_mutex_init(&internal_mutex,NULL);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost::permit_any::permit_any() failed in pthread_mutex_init"));
            }
            int const res2=pthread_permit_init(&perm, initial_state);
            if(res2)
            {
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
                boost::throw_exception(thread_resource_error(res, "boost::permit_any::permit_any() failed in pthread_permit_init"));
            }
        }
        ~permit_any()
        {
            BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
            BOOST_VERIFY(!pthread_permit_destroy(&perm));
        }

        template<typename lock_type>
        void wait(lock_type& m)
        {
            int res=0;
            {
                thread_cv_detail::lock_on_exit<lock_type> guard;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                detail::interruption_checker check_for_interruption(&internal_mutex,pthread_permit_get_internal_cond(&perm));
#else
            boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
#endif
                guard.activate(m);
                res=pthread_permit_wait(&perm,&internal_mutex);
            }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            this_thread::interruption_point();
#endif
            if(res)
            {
                boost::throw_exception(condition_error(res, "boost::permit_any::wait() failed in pthread_permit_wait"));
            }
        }

        template<typename lock_type,typename predicate_type>
        void wait(lock_type& m,predicate_type pred)
        {
            while(!pred()) wait(m);
        }

#if defined BOOST_THREAD_USES_DATETIME
        template<typename lock_type>
        bool timed_wait(lock_type& m,boost::system_time const& abs_time)
        {
            struct timespec const timeout=detail::to_timespec(abs_time);
            return do_wait_until(m, timeout);
        }
        template<typename lock_type>
        bool timed_wait(lock_type& m,xtime const& abs_time)
        {
            return timed_wait(m,system_time(abs_time));
        }

        template<typename lock_type,typename duration_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration)
        {
            return timed_wait(m,get_system_time()+wait_duration);
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,boost::system_time const& abs_time, predicate_type pred)
        {
            while (!pred())
            {
                if(!timed_wait(m, abs_time))
                    return pred();
            }
            return true;
        }

        template<typename lock_type,typename predicate_type>
        bool timed_wait(lock_type& m,xtime const& abs_time, predicate_type pred)
        {
            return timed_wait(m,system_time(abs_time),pred);
        }

        template<typename lock_type,typename duration_type,typename predicate_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration,predicate_type pred)
        {
            return timed_wait(m,get_system_time()+wait_duration,pred);
        }
#endif
#ifdef BOOST_THREAD_USES_CHRONO
        template <class lock_type,class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<chrono::system_clock, Duration>& t)
        {
          using namespace chrono;
          typedef time_point<system_clock, nanoseconds> nano_sys_tmpt;
          wait_until(lock,
                        nano_sys_tmpt(ceil<nanoseconds>(t.time_since_epoch())));
          return system_clock::now() < t ? cv_status::no_timeout :
                                             cv_status::timeout;
        }

        template <class lock_type, class Clock, class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
          using namespace chrono;
          system_clock::time_point     s_now = system_clock::now();
          typename Clock::time_point  c_now = Clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(t - c_now));
          return Clock::now() < t ? cv_status::no_timeout : cv_status::timeout;
        }

        template <class lock_type, class Clock, class Duration, class Predicate>
        bool
        wait_until(
                lock_type& lock,
                const chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            while (!pred())
            {
                if (wait_until(lock, t) == cv_status::timeout)
                    return pred();
            }
            return true;
        }


        template <class lock_type, class Rep, class Period>
        cv_status
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d)
        {
          using namespace chrono;
          system_clock::time_point s_now = system_clock::now();
          steady_clock::time_point c_now = steady_clock::now();
          wait_until(lock, s_now + ceil<nanoseconds>(d));
          return steady_clock::now() - c_now < d ? cv_status::no_timeout :
                                                   cv_status::timeout;

        }


        template <class lock_type, class Rep, class Period, class Predicate>
        bool
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));

//          while (!pred())
//          {
//              if (wait_for(lock, d) == cv_status::timeout)
//                  return pred();
//          }
//          return true;
        }

        template <class lock_type>
        cv_status wait_until(
            lock_type& lk,
            chrono::time_point<chrono::system_clock, chrono::nanoseconds> tp)
        {
            using namespace chrono;
            nanoseconds d = tp.time_since_epoch();
            timespec ts = boost::detail::to_timespec(d);
            if (do_wait_until(lk, ts)) return cv_status::no_timeout;
            else return cv_status::timeout;
        }
#endif

        void grant() BOOST_NOEXCEPT
        {
            boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
            BOOST_VERIFY(!pthread_permit_grant(&perm));
        }

        void revoke() BOOST_NOEXCEPT
        {
            boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
            BOOST_VERIFY(!pthread_permit_revoke(&perm));
        }
    private: // used by boost::thread::try_join_until

        template <class lock_type>
        inline bool do_wait_until(
          lock_type& m,
          struct timespec const &timeout)
        {
          int res=0;
          {
              thread_cv_detail::lock_on_exit<lock_type> guard;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
              detail::interruption_checker check_for_interruption(&internal_mutex,pthread_permit_get_internal_cond(&perm));
#else
            boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
#endif
              guard.activate(m);
              res=pthread_permit_timedwait(&perm,&internal_mutex,&timeout);
          }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
          this_thread::interruption_point();
#endif
          if(res==ETIMEDOUT)
          {
              return false;
          }
          if(res)
          {
              boost::throw_exception(condition_error(res, "boost::permit_any::do_wait_until() failed in pthread_permit_timedwait"));
          }
          return true;
        }


    };
    
    typedef permit<true> permit_c;
    typedef permit_any<true> permit_any_c;
    typedef permit<false> permit_nc;
    typedef permit_any<false> permit_any_nc;

}

#include <boost/config/abi_suffix.hpp>

#endif
