#ifndef BOOST_THREAD_CONDITION_HPP
#define BOOST_THREAD_CONDITION_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <boost/thread/detail/platform.hpp>
#include BOOST_THREAD_PLATFORM(condition_state.hpp)

namespace boost
{
    template <class Mutex>
    class condition:
        private boost::detail::thread::condition_state<Mutex>
    {
    public:
        typedef Mutex mutex_type;
    
        condition();
        explicit condition(mutex_type& m);
        ~condition();

#ifdef BOOST_HAS_DELETED_FUNCTIONS    
        condition(const condition&) = delete;
        condition& operator=(const condition&) = delete;
#endif

        void notify_one();
        void notify_all();

        template <class Lock>
        void wait(Lock& lock);

        template <class Lock, class Predicate>
        void wait(Lock& lock, Predicate pred);

        template <class Lock>
        bool timed_wait(Lock& lock, const utc_time& abs_time);

        template <class Lock, class Predicate>
        bool timed_wait(Lock& lock, const utc_time& abs_time, Predicate pred);

#ifndef BOOST_HAS_DELETED_FUNCTIONS    
    private:
        explicit condition(condition&);
        condition& operator=(condition&);
#endif

    };
}

#include BOOST_THREAD_PLATFORM(condition_impl.hpp)
#include <boost/thread/utc_time.hpp>

#endif

