/*
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *    8 Feb 01  Initial version.
 *    1 Jun 01  Added boost::thread initial implementation.
 */
 
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/semaphore.hpp>
#include <boost/thread/tss.hpp>
#include <cassert>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#   include <process.h>
#elif defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#endif

#include "timeconv.inl"

namespace
{
    class thread_counter
    {
    public:
        thread_counter() : _threads(0) { }

        void start()
        {
            boost::mutex::lock lock(_mutex);
            ++_threads;
        }

        void stop()
        {
            boost::mutex::lock lock(_mutex);
            if (--_threads == 0)
                _cond.notify_all();
        }

        void wait()
        {
            boost::mutex::lock lock(_mutex);
            while (_threads != 0)
                _cond.wait(lock);
        }

    private:
        unsigned long _threads;
        boost::mutex _mutex;
        boost::condition _cond;
    };

    thread_counter threads;
    boost::tss tss_state;
}

namespace boost
{
    namespace detail
    {
        class thread_state
        {
            enum
            {
                creating,
                created,
                started,
                finished
            };

        public:
            thread_state();
            ~thread_state();
         
            void add_ref();
            void release();
            bool is_alive();
            void join();

#if defined(BOOST_HAS_WINTHREADS)
            static unsigned __stdcall thread_proc(void* param);
#elif defined(BOOST_HAS_PTHREADS)
            static void* thread_proc(void* param);
#endif

            static thread_state* create(const boost::detail::threadfunc& func, void* param);

        private:
            unsigned long _refs;
            mutex _mutex;
            condition _cond;
            int _state;
            threadfunc _func;
            void* _param;
#if defined(BOOST_HAS_WINTHREADS)
            HANDLE _thread;
#elif defined(BOOST_HAS_PTHREADS)
            pthread_t _thread;
#endif

            // This line illustrate the internal compiler error encountered on MSVC
//            boost::function<int, void*> _function;
        };

        thread_state::thread_state()
            : _state(creating), _refs(2)  // Both created thread and creating thread own at first
        {
        }

        thread_state::~thread_state()
        {
            if (_state == finished)
            {
#if defined(BOOST_HAS_WINTHREADS)
                int res = CloseHandle(_thread);
                assert(res);
#elif defined(BOOST_HAS_PTHREADS)
                int res = pthread_detach(_thread);
                assert(res == 0);
#endif
            }
        }

        void thread_state::add_ref()
        {
            mutex::lock lock(_mutex);
            ++_refs;
        }

        void thread_state::release()
        {
            bool del = false;
            {
                mutex::lock lock(_mutex);
                del = (--_refs == 0);
            }
            if (del) delete this;
        }

        bool thread_state::is_alive()
        {
            mutex::lock lock(_mutex);
            return _state == started;
        }

        void thread_state::join()
        {
            mutex::lock lock(_mutex);
            while (_state != finished)
                _cond.wait(lock);
        }

#if defined(BOOST_HAS_WINTHREADS)
        unsigned __stdcall thread_state::thread_proc(void* param)
#elif defined(BOOST_HAS_PTHREADS)
        void* thread_state::thread_proc(void* param)
#endif
        {
            thread_state* state = static_cast<thread_state*>(param);
            assert(state);

            tss_state.set(state);

            {
                mutex::lock lock(state->_mutex);

                while (state->_state != created)
                    state->_cond.wait(lock);

                state->_state = started;
                state->_cond.notify_all();
                threads.start();
            }

            try
            {
                state->_func(state->_param);
            }
            catch (...)
            {
            }

            {
                mutex::lock lock(state->_mutex);
                state->_state = finished;
                state->_cond.notify_all();
            }

            state->release();
            threads.stop();

            return 0;
        }

        thread_state* thread_state::create(const boost::detail::threadfunc& func, void* param)
        {
            thread_state* state = new thread_state();
            mutex::lock lock(state->_mutex);

            assert(func);
            state->_func = func;
            state->_param = param;

#if defined(BOOST_HAS_WINTHREADS)
            unsigned id;
            state->_thread = (HANDLE)_beginthreadex(0, 0, &thread_proc, state, 0, &id);
            assert(state->_thread);

            if (state->_thread == 0)
            {
                delete state;
                return 0;
            }
#elif defined(BOOST_HAS_PTHREADS)
            int res = pthread_create(&state->_thread, 0, &thread_proc, state);
            assert(res == 0);

            if (res != 0)
            {
                delete state;
                return 0;
            }
#endif

            state->_state = created;
            state->_cond.notify_all();

            while (state->_state != started)
                state->_cond.wait(lock);

            return state;
        }
    }

    lock_error::lock_error() : std::runtime_error("thread lock error")
	{
	}

    thread::thread(const thread& other)
        : _state(other._state)
    {
        if (_state)
            _state->add_ref();
    }

    thread::~thread()
    {
        if (_state)
            _state->release();
    }

    bool thread::is_alive() const
    {
        if (_state)
            return _state->is_alive();
        return false;
    }

    void thread::join()
    {
        if (_state)
            _state->join();
    }

    thread thread::create(const detail::threadfunc& func, void* param)
    {
        thread temp;
        temp._state = detail::thread_state::create(func, param);
        return temp;
    }

    thread thread::self()
    {
        thread temp;
        temp._state = static_cast<detail::thread_state*>(tss_state.get());
        if (temp._state)
            temp._state->add_ref();
        return temp;
    }

    void thread::join_all()
    {
        threads.wait();
    }

    void thread::sleep(const xtime& xt)
    {
#if defined(BOOST_HAS_WINTHREADS)
        unsigned milliseconds;
        to_duration(xt, milliseconds);
        Sleep(milliseconds);
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(BOOST_HAS_PTHREAD_DELAY_NP)
        timespec ts;
        to_timespec(xt, ts);
        int res = pthread_delay_np(&ts);
        assert(res == 0);
#   elif defined(BOOST_HAS_NANOSLEEP)
        timespec ts;
        to_timespec(xt, ts);
        nanosleep(&ts, 0);
#   else
        boost::semaphore sema;
        sema.down(xt);
#   endif
#endif
    }

    void thread::yield()
    {
#if defined(BOOST_HAS_WINTHREADS)
        Sleep(0);
#elif defined(BOOST_HAS_PTHREADS)
#   if defined(BOOST_HAS_SCHED_YIELD)
        int res = sched_yield();
        assert(res == 0);
#   elif defined(BOOST_HAS_PTHREAD_YIELD)
        int res = pthread_yield();
        assert(res == 0);
#   else
        xtime xt;
        xtime_get(&xt, boost::TIME_UTC);
        sleep(xt);
#   endif
#endif
    }
}