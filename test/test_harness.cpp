#define BOOST_INCLUDE_MAIN
#include <boost/test/test_tools.hpp>

#include <iostream>
#include <process.h>

extern void test_xtime_get();
extern void test_thread();
extern void test_thread_group();
extern void test_mutex();
extern void test_try_mutex();
extern void test_timed_mutex();
extern void test_recursive_mutex();
extern void test_recursive_try_mutex();
extern void test_recursive_timed_mutex();
extern void test_condition();
extern void test_thread_specific_ptr();
extern void test_call_once();
extern void test_barrier();
extern void test_thread_pool();
extern void test_rw_mutex();

namespace {

void run_test(void (*func)())
{
	// Indicate testing progress...
	std::cout << '.' << std::flush;
	(*func)();
}

} // namespace

int test_main(int, char*[])
{
	run_test(&test_xtime_get);
	run_test(&test_thread);
	run_test(&test_thread_group);
    run_test(&test_mutex);
    run_test(&test_try_mutex);
    run_test(&test_timed_mutex);
    run_test(&test_recursive_mutex);
    run_test(&test_recursive_try_mutex);
    run_test(&test_recursive_timed_mutex);
    run_test(&test_condition);
    run_test(&test_thread_specific_ptr);
    run_test(&test_call_once);
	run_test(&test_barrier);
	run_test(&test_thread_pool);
//	run_test(&test_rw_mutex);
//	_endthreadex(0);
    return 0;
}
