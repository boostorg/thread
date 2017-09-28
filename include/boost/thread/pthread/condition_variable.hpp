#ifndef BOOST_THREAD_CONDITION_VARIABLE_PTHREAD_HPP
#define BOOST_THREAD_CONDITION_VARIABLE_PTHREAD_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-10 Anthony Williams
// (C) Copyright 2011-2012 Vicente J. Botet Escriba

#include <boost/thread/pthread/timespec.hpp>
#include <boost/thread/pthread/pthread_mutex_scoped_lock.hpp>
#include <boost/thread/pthread/pthread_helpers.hpp>

#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
#include <boost/thread/pthread/thread_data.hpp>
#endif
#include <boost/thread/pthread/condition_variable_fwd.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/thread/detail/internal_clock.hpp>
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/detail/delete.hpp>

#include <algorithm>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
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
            void deactivate()
            {
                if (m)
                {
                    m->lock();
                }
                m = 0;
            }
            ~lock_on_exit() BOOST_NOEXCEPT_IF(false)
            {
                if (m)
                {
                    m->lock();
                }
           }
        };
    }

    inline void condition_variable::wait(unique_lock<mutex>& m)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if(! m.owns_lock())
        {
            boost::throw_exception(condition_error(-1, "boost::condition_variable::wait() failed precondition mutex not owned"));
        }
#endif
        int res=0;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            thread_cv_detail::lock_on_exit<unique_lock<mutex> > guard;
            detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
            pthread_mutex_t* the_mutex = &internal_mutex;
            guard.activate(m);
            res = pthread_cond_wait(&cond,the_mutex);
            check_for_interruption.check();
            guard.deactivate();
#else
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            res = pthread_cond_wait(&cond,the_mutex);
#endif
        }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        this_thread::interruption_point();
#endif
        if(res && res != EINTR)
        {
            boost::throw_exception(condition_error(res, "boost::condition_variable::wait failed in pthread_cond_wait"));
        }
    }

    inline bool condition_variable::do_wait_until(
                unique_lock<mutex>& m,
                detail::internal_timespec_timepoint const &timeout)
    {
#if defined BOOST_THREAD_THROW_IF_PRECONDITION_NOT_SATISFIED
        if (!m.owns_lock())
        {
            boost::throw_exception(condition_error(EPERM, "boost::condition_variable::do_wait_until() failed precondition mutex not owned"));
        }
#endif
        int cond_res;
        {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            thread_cv_detail::lock_on_exit<unique_lock<mutex> > guard;
            detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
            pthread_mutex_t* the_mutex = &internal_mutex;
            guard.activate(m);
            cond_res=pthread_cond_timedwait(&cond,the_mutex,&timeout.get());
            check_for_interruption.check();
            guard.deactivate();
#else
            pthread_mutex_t* the_mutex = m.mutex()->native_handle();
            cond_res=pthread_cond_timedwait(&cond,the_mutex,&timeout.get());
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
            boost::throw_exception(condition_error(cond_res, "boost::condition_variable::do_wait_until failed in pthread_cond_timedwait"));
        }
        return true;
    }

    inline void condition_variable::notify_one() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_cond_signal(&cond));
    }

    inline void condition_variable::notify_all() BOOST_NOEXCEPT
    {
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
        boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
#endif
        BOOST_VERIFY(!pthread_cond_broadcast(&cond));
    }

    class condition_variable_any
    {
        pthread_mutex_t internal_mutex;
        pthread_cond_t cond;

    public:
        BOOST_THREAD_NO_COPYABLE(condition_variable_any)
        condition_variable_any()
        {
            int const res=pthread_mutex_init(&internal_mutex,NULL);
            if(res)
            {
                boost::throw_exception(thread_resource_error(res, "boost::condition_variable_any::condition_variable_any() failed in pthread_mutex_init"));
            }
            int const res2 = pthread::cond_init(cond);
            if(res2)
            {
                BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
                boost::throw_exception(thread_resource_error(res2, "boost::condition_variable_any::condition_variable_any() failed in pthread::cond_init"));
            }
        }
        ~condition_variable_any()
        {
            BOOST_VERIFY(!pthread_mutex_destroy(&internal_mutex));
            BOOST_VERIFY(!pthread_cond_destroy(&cond));
        }

        template<typename lock_type>
        void wait(lock_type& m)
        {
            int res=0;
            {
                thread_cv_detail::lock_on_exit<lock_type> guard;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
                detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
#else
                boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
#endif
                guard.activate(m);
                res=pthread_cond_wait(&cond,&internal_mutex);
                check_for_interruption.check();
                guard.deactivate();
            }
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
            this_thread::interruption_point();
#endif
            if(res)
            {
                boost::throw_exception(condition_error(res, "boost::condition_variable_any::wait() failed in pthread_cond_wait"));
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
        template<typename lock_type>
        bool timed_wait(lock_type& m,xtime const& abs_time)
        {
            return timed_wait(m,system_time(abs_time));
        }

        template<typename lock_type,typename duration_type>
        bool timed_wait(lock_type& m,duration_type const& wait_duration)
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
        template <class lock_type,class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<thread_detail::internal_clock_t, Duration>& t)
        {
          const boost::detail::internal_timespec_timepoint& ts = t;
          if (do_wait_until(lock, ts)) return cv_status::no_timeout;
          else return cv_status::timeout;
        }

        template <class lock_type, class Clock, class Duration>
        cv_status
        wait_until(
                lock_type& lock,
                const chrono::time_point<Clock, Duration>& t)
        {
          using namespace chrono;
          typedef typename common_type<Duration, typename Clock::duration>::type CD;
          CD d = t - Clock::now();
          if ( d <= CD::zero() ) return cv_status::timeout;
          d = (std::min)(d, CD(milliseconds(100)));
          while (cv_status::timeout == wait_until(lock, thread_detail::internal_clock_t::now() + d))
          {
              d = t - Clock::now();
              if ( d <= CD::zero() ) return cv_status::timeout;
              d = (std::min)(d, CD(milliseconds(100)));
          }
          return cv_status::no_timeout;
        }

        template <class lock_type, class Rep, class Period>
        cv_status
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d)
        {
          return wait_until(lock, chrono::steady_clock::now() + d);
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

        template <class lock_type, class Rep, class Period, class Predicate>
        bool
        wait_for(
                lock_type& lock,
                const chrono::duration<Rep, Period>& d,
                Predicate pred)
        {
          return wait_until(lock, chrono::steady_clock::now() + d, boost::move(pred));
        }
#endif

        void notify_one() BOOST_NOEXCEPT
        {
            boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
            BOOST_VERIFY(!pthread_cond_signal(&cond));
        }

        void notify_all() BOOST_NOEXCEPT
        {
            boost::pthread::pthread_mutex_scoped_lock internal_lock(&internal_mutex);
            BOOST_VERIFY(!pthread_cond_broadcast(&cond));
        }
    private:

        template <class lock_type>
        bool do_wait_until(
          lock_type& m,
          detail::internal_timespec_timepoint const &timeout)
        {
          int res=0;
          {
              thread_cv_detail::lock_on_exit<lock_type> guard;
#if defined BOOST_THREAD_PROVIDES_INTERRUPTIONS
              detail::interruption_checker check_for_interruption(&internal_mutex,&cond);
#else
              boost::pthread::pthread_mutex_scoped_lock check_for_interruption(&internal_mutex);
#endif
              guard.activate(m);
              res=pthread_cond_timedwait(&cond,&internal_mutex,&timeout.get());
              check_for_interruption.check();
              guard.deactivate();
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
              boost::throw_exception(condition_error(res, "boost::condition_variable_any::do_wait_until() failed in pthread_cond_timedwait"));
          }
          return true;
        }
        template <class lock_type, class Predicate>
        bool do_wait_until(
                lock_type& lock,
                detail::internal_timespec_timepoint const& t,
                Predicate pred)
        {
            while (!pred())
            {
                if ( ! do_wait_until(lock, t) )
                    return pred();
            }
            return true;
        }
    };
}

#include <boost/config/abi_suffix.hpp>

#endif
