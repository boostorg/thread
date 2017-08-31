#ifndef BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
#define BOOST_THREAD_PTHREAD_CONDITION_VARIABLE_FWD_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/assert.hpp>
#include <boost/throw_exception.hpp>
#include <pthread.h>
#include <boost/thread/cv_status.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/pthread/timespec.hpp>
#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif

#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace detail {
    inline int monotonic_pthread_cond_init(pthread_cond_t& cond) {

#ifdef BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
            pthread_condattr_t attr;
            int res = pthread_condattr_init(&attr);
            if (res)
            {
              return res;
            }
            pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
            res=pthread_cond_init(&cond,&attr);
            pthread_condattr_destroy(&attr);
            return res;
#else
            return pthread_cond_init(&cond,NULL);
#endif

    }
  }

    class condition_variable
    {
    private:
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        pthread_mutex_t internal_mutex;
//#endif
        pthread_cond_t cond;

#ifdef BOOST_THREAD_USES_CHRONO
#ifdef BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC
        typedef chrono::steady_clock internal_clock_t;
#else
        typedef chrono::system_clock internal_clock_t;
#endif
#endif

    public:
    //private: // used by boost::thread::try_join_until

        inline bool do_wait_until(
            unique_lock<mutex>& lock,
            struct timespec const &timeout);

        template<typename predicate_type>
        bool do_wait_until(
            unique_lock<mutex>& lock,
            struct timespec const &timeout,
            predicate_type pred);

    public:
      BOOST_THREAD_NO_COPYABLE(condition_variable)
        condition_variable()
        {
            int res;
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // Even if it is not used, the internal_mutex exists (see
            // above) and must be initialized (etc) in case some
            // compilation units provide interruptions and others
            // don't.
            res=pthread_mutex_init(&internal_mutex,NULL);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable::condition_variable() constructor failed in pthread_mutex_init"));
            }
//#endif
            res = detail::monotonic_pthread_cond_init(cond);
            if (res)
            {
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                // ditto
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
//#endif
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable::condition_variable() constructor failed in detail::monotonic_pthread_cond_init"));
            }
        }
        ~condition_variable()
        {
            int ret;
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            // ditto
            do {
              ret = pthread_mutex_destroy(&internal_mutex);
            } while (ret == EINTR);
            BOOST_ASSERT(!ret);
//#endif
            do {
              ret = pthread_cond_destroy(&cond);
            } while (ret == EINTR);
            BOOST_ASSERT(!ret);
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
            return do_wait_until(m,detail::timespec_to_internal_clock(abs_time + BOOST_THREAD_WAIT_BUG));
#else
            return do_wait_until(m,detail::timespec_to_internal_clock(abs_time));
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
            if (wait_duration.is_pos_infinity())
            {
                wait(m); // or do_wait(m,detail::timeout::sentinel());
                return true;
            }
            if (wait_duration.is_special())
            {
                return true;
            }
            return do_wait_until(m,detail::timespec_plus_internal_clock(wait_duration));
        }

        template<typename predicate_type>
        bool timed_wait(
            unique_lock<mutex>& m,
            boost::system_time const& abs_time,predicate_type pred)
        {
            return do_wait_until(m,detail::timespec_to_internal_clock(abs_time),pred);
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
            if (wait_duration.is_pos_infinity())
            {
                while (!pred())
                {
                  wait(m); // or do_wait(m,detail::timeout::sentinel());
                }
                return true;
            }
            if (wait_duration.is_special())
            {
                return pred();
            }
            return do_wait_until(m,detail::timespec_plus_internal_clock(wait_duration),pred);
        }
#endif // defined BOOST_THREAD_USES_DATETIME

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<internal_clock_t, Duration>& t)
        {
          return do_wait_until(lock, boost::detail::to_timespec(t.time_since_epoch()))
            ? cv_status::no_timeout : cv_status::timeout;
        }

        template <class Clock, class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
          using namespace chrono;
          internal_clock_t::time_point s_now = internal_clock_t::now();
          typename Clock::time_point  c_now = Clock::now();
          return wait_until(lock, s_now + ceil<nanoseconds>(t - c_now));
        }



        template <class Rep, class Period>
        cv_status
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d)
        {
          return wait_until(lock, internal_clock_t::now() + d);
        }

        template <class Duration, class Predicate>
        bool
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<internal_clock_t, Duration>& t,
                Predicate pred)
        {
            return do_wait_until(lock, boost::detail::to_timespec(t.time_since_epoch()), boost::move(pred));
        }

        template <class Clock, class Duration, class Predicate>
        bool
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t,
                Predicate pred)
        {
            using namespace chrono;
            internal_clock_t::time_point s_now = internal_clock_t::now();
            typename Clock::time_point  c_now = Clock::now();
            return wait_until(lock, s_now + ceil<nanoseconds>(t - c_now), boost::move(pred));
        }

        template <class Rep, class Period, class Predicate>
        bool
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, internal_clock_t::now() + d, boost::move(pred));
        }
#endif // defined BOOST_THREAD_USES_CHRONO

#define BOOST_THREAD_DEFINES_CONDITION_VARIABLE_NATIVE_HANDLE
        typedef pthread_cond_t* native_handle_type;
        native_handle_type native_handle()
        {
            return &cond;
        }

        void notify_one() BOOST_NOEXCEPT;
        void notify_all() BOOST_NOEXCEPT;


    };

    BOOST_THREAD_DECL void notify_all_at_thread_exit(condition_variable& cond, unique_lock<mutex> lk);

}


#include <boost/config/abi_suffix.hpp>

#endif
