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
#include <boost/thread/detail/timespec.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>

#if defined BOOST_THREAD_USES_DATETIME
#include <boost/thread/xtime.hpp>
#endif

#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

#include <algorithm>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    class condition_variable
    {
    private:
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        pthread_mutex_t internal_mutex;
//#endif
        pthread_cond_t cond;

    public:
    //private: // used by boost::thread::try_join_until

        inline bool do_wait_until(
            unique_lock<mutex>& lock,
            detail::internal_timespec_timepoint const &timeout);
        template <class Predicate>
        bool do_wait_until(
            unique_lock<mutex>& lock,
            detail::internal_timespec_timepoint const &timeout,
            Predicate pred)
        {
          while (!pred())
          {
              if ( ! do_wait_until(lock, timeout) )
                  return pred();
          }
          return true;
        }

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
            res = pthread::cond_init(cond);
            if (res)
            {
//#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                // ditto
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
//#endif
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable::condition_variable() constructor failed in pthread::cond_init"));
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
            boost::system_time const& abs_time_fixed = abs_time + BOOST_THREAD_WAIT_BUG;
#else
            boost::system_time const& abs_time_fixed = abs_time;
#endif
#if defined BOOST_THREAD_INTERNAL_CLOCK_IS_MONO
            const detail::real_timespec_timepoint ts(abs_time_fixed);
            detail::timespec_duration d = ts - detail::real_timespec_clock::now();
            d = (std::min)(d, detail::timespec_milliseconds(100));
            while ( ! do_wait_until(m, detail::internal_timespec_clock::now() + d) )
            {
              d = ts - detail::real_timespec_clock::now();
              if ( d <= detail::timespec_duration::zero() ) return false;
              d = (std::min)(d, detail::timespec_milliseconds(100));
            }
            return true;
#else
            return do_wait_until(m, detail::internal_timespec_timepoint(abs_time_fixed));
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
                wait(m);
                return true;
            }
            if (wait_duration.is_special())
            {
                return true;
            }
            detail::timespec_duration d(wait_duration);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
            const detail::mono_timespec_timepoint& ts = detail::mono_timespec_clock::now() + d;
            d = (std::min)(d, detail::timespec_milliseconds(100));
            while ( ! do_wait_until(m, detail::internal_timespec_clock::now() + d) )
            {
              d = ts - detail::mono_timespec_clock::now();
              if ( d <= detail::timespec_duration::zero() ) return false;
              d = (std::min)(d, detail::timespec_milliseconds(100));
            }
            return true;
#else
            return do_wait_until(m, detail::internal_timespec_clock::now() + d);
#endif
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
            duration_type const& wait_duration, predicate_type pred)
        {
            if (wait_duration.is_pos_infinity())
            {
                while (!pred())
                {
                  wait(m);
                }
                return true;
            }
            if (wait_duration.is_special())
            {
                return pred();
            }
            detail::timespec_duration d(wait_duration);
#if defined(BOOST_THREAD_HAS_MONO_CLOCK) && !defined(BOOST_THREAD_INTERNAL_CLOCK_IS_MONO)
            const detail::mono_timespec_timepoint& ts = detail::mono_timespec_clock::now() + d;
            d = (std::min)(d, detail::timespec_milliseconds(100));
            while ( ! pred() && ! do_wait_until(m, detail::internal_timespec_clock::now() + d) )
            {
              d = ts - detail::mono_timespec_clock::now();
              if ( d <= detail::timespec_duration::zero() ) return pred();
              d = (std::min)(d, detail::timespec_milliseconds(100));
            }
            return pred();
#else
            return do_wait_until(m, detail::internal_timespec_clock::now() + d, move(pred));
#endif
        }
#endif

#ifdef BOOST_THREAD_USES_CHRONO

        template <class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<detail::internal_chrono_clock, Duration>& t)
        {
          const detail::internal_timespec_timepoint& ts = t;
          if (do_wait_until(lock, ts)) return cv_status::no_timeout;
          else return cv_status::timeout;
        }

        template <class Clock, class Duration>
        cv_status
        wait_until(
                unique_lock<mutex>& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
          typedef typename common_type<Duration, typename Clock::duration>::type CD;
          CD d = t - Clock::now();
          d = (std::min)(d, CD(chrono::milliseconds(100)));
          while (cv_status::timeout == wait_until(lock, detail::internal_chrono_clock::now() + d))
          {
              d = t - Clock::now();
              if ( d <= CD::zero() ) return cv_status::timeout;
              d = (std::min)(d, CD(chrono::milliseconds(100)));
          }
          return cv_status::no_timeout;
        }

        template <class Rep, class Period>
        cv_status
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d)
        {
          return wait_until(lock, chrono::steady_clock::now() + d);
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

        template <class Rep, class Period, class Predicate>
        bool
        wait_for(
                unique_lock<mutex>& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));
        }
#endif

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
