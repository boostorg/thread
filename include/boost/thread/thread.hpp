// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_THREAD_WEK070601_HPP
#define BOOST_THREAD_WEK070601_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif
#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>
#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <list>
#include <memory>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#   include <boost/thread/condition.hpp>
#elif defined(BOOST_HAS_MPTASKS)
#   include <Multiprocessing.h>
#endif

// Config macros
// BOOST_THREAD_ATTRIBUTES_STACKSIZE
// BOOST_THREAD_ATTRIBUTES_STACKADDR
// BOOST_THREAD_PRIORITY_SCHEDULING
// 

namespace boost {

struct xtime;

class BOOST_THREAD_DECL thread_cancel
{
public:
    thread_cancel();
	~thread_cancel();
};

class BOOST_THREAD_DECL thread : private noncopyable
{
public:
	class BOOST_THREAD_DECL attributes
	{
	public:
		attributes();
		~attributes();

#if defined(BOOST_THREAD_ATTRIBUTES_STACKSIZE)
		attributes& stack_size(size_t size);
		size_t stack_size() const;
#endif

#if defined(BOOST_THREAD_ATTRIBUTES_STACKADDR)
		attributes& stack_address(void* addr);
		void* stack_address() const;
#endif

#if defined(BOOST_THREAD_PRIORITY_SCHEDULING)
		struct sched_param { int priority; }

		attributes& inherit_scheduling(bool inherit);
		bool inherit_scheduling() const;
		attributes& scheduling_parameter(const sched_param& param);
		sched_param scheduling_parameter() const;
		attributes& scheduling_policy(int policy);
		int scheduling_policy() const;
		attributes& scope(int scope);
		int scope() const;
#endif
	};

    thread();
    explicit thread(const function0<void>& threadfunc, attributes attr=attributes());
    ~thread();

    bool operator==(const thread& other) const;
    bool operator!=(const thread& other) const;

    void join();
    void cancel();

    static void test_cancel();
    static void sleep(const xtime& xt);
    static void yield();

	// This is an implementation detail and should be private,
	// but we need it to be public to access the type in some
	// unnamed namespace free functions in the implementation.
	class data;

private:
	data* m_handle;
};

class BOOST_THREAD_DECL thread_group : private noncopyable
{
public:
    thread_group();
    ~thread_group();

    thread* create_thread(const function0<void>& threadfunc);
    void add_thread(thread* thrd);
    void remove_thread(thread* thrd);
	thread* thread_group::find(thread& thrd);
    void join_all();

private:
    std::list<thread*> m_threads;
    mutex m_mutex;
};

} // namespace boost

// Change Log:
//    8 Feb 01  WEKEMPF Initial version.
//    1 Jun 01  WEKEMPF Added boost::thread initial implementation.
//    3 Jul 01  WEKEMPF Redesigned boost::thread to be noncopyable.

#endif // BOOST_THREAD_WEK070601_HPP
