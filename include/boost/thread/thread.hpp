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

#include <boost/thread/detail/config.hpp>

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>
#include <iostream>
#include <list>
#include <memory>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#   include <boost/thread/condition.hpp>
#elif defined(BOOST_HAS_MPTASKS)
#   include <Multiprocessing.h>
#endif

namespace boost {

struct xtime;

class BOOST_THREAD_DECL thread_cancel
{
public:
    thread_cancel();
	~thread_cancel();
};

class BOOST_THREAD_DECL cancellation_guard
{
public:
	cancellation_guard();
	~cancellation_guard();

private:
	void* m_handle;
};

#if defined(BOOST_HAS_WINTHREADS)

struct sched_param
{
	int priority;
};

enum { sched_fifo, sched_round_robin, sched_other };
enum { scope_process, scope_system };

#elif defined(BOOST_HAS_PTHREADS)

using ::sched_param;

enum
{
	sched_fifo = SCHED_FIFO,
	sched_round_robin = SCHED_RR,
	sched_other = SCHED_OTHER
};

enum
{
	scope_process = PTHREAD_SCOPE_PROCESS,
	scope_system = PTHREAD_SCOPE_SYSTEM
};

#endif

class BOOST_THREAD_DECL thread
{
public:
	class BOOST_THREAD_DECL attributes
	{
	public:
		attributes();
		~attributes();

		attributes& set_stack_size(size_t size);
		size_t get_stack_size() const;

		attributes& set_stack_address(void* addr);
		void* get_stack_address() const;

		attributes& inherit_scheduling(bool inherit);
		bool inherit_scheduling() const;
		attributes& set_schedule(int policy, const sched_param& param);
		void get_schedule(int& policy, sched_param& param);
		attributes& scope(int scope);
		int scope() const;

	private:
		friend class thread;

#if defined(BOOST_HAS_WINTHREADS)
		size_t m_stacksize;
		bool m_schedinherit;
		sched_param m_schedparam;
#elif defined(BOOST_HAS_PTHREADS)
		pthread_attr_t m_attr;
#endif
	};

    thread();
    explicit thread(const function0<void>& threadfunc, attributes attr=attributes());
	thread(const thread& other);
    ~thread();

	thread& operator=(const thread& other);

    bool operator==(const thread& other) const;
    bool operator!=(const thread& other) const;
	bool operator<(const thread& other) const;

    void join();
    void cancel();

	void set_scheduling_parameter(int policy, const sched_param& param);
	void get_scheduling_parameter(int& policy, sched_param& param) const;

	static int max_priority(int policy);
	static int min_priority(int policy);

    static void test_cancel();
    static void sleep(const xtime& xt);
    static void yield();

	static const int stack_min;

#if defined(BOOST_HAS_WINTHREADS)
	typedef unsigned int id_type;
#else
	typedef void* id_type;
#endif
	
	id_type id() const;

private:
	void* m_handle;
};

template <typename charT, typename Traits>
std::basic_ostream<charT, Traits>& operator<<(std::basic_ostream<charT, Traits>& os, const thread& thrd)
{
	if (!os.good()) return os;

	typename std::basic_ostream<charT, Traits>::sentry opfx(os);

	if (opfx)
		os << thrd.id();

	return os;
}

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
