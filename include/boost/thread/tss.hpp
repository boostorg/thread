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

#ifndef BOOST_TSS_WEK070601_HPP
#define BOOST_TSS_WEK070601_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/thread/detail/config.hpp>
#include <boost/function.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#elif defined(BOOST_HAS_MPTASKS)
#   include <Multiprocessing.h>
#endif

namespace boost {

namespace detail {

class BOOST_THREAD_DECL tss_ref
{
public:
    tss_ref();
};

class BOOST_THREAD_DECL tss : private noncopyable
{
public:
	template <typename F>
		tss(const F& cleanup) {
			boost::function1<void, void*>* pcleanup = 0;
			try
			{
				pcleanup = new boost::function1<void, void*>(cleanup);
				init(pcleanup);
			}
			catch (...)
			{
				delete pcleanup;
				throw boost::thread_resource_error();
			}
		}

    void* get() const;
    void set(void* value);
    void cleanup(void* p);

private:
    int m_slot;

	void init(boost::function1<void, void*>* pcleanup);
};

#if defined(BOOST_HAS_MPTASKS)
    void thread_cleanup();
#endif

/*struct tss_adapter
{
    tss_adapter(boost::function1<void, void*> cleanup) : m_cleanup(cleanup) { }
    void operator()(void* p) { m_cleanup(p); }
    boost::function1<void, void*> m_cleanup;
};*/

} // namespace detail

template <typename T>
class thread_specific_ptr : private noncopyable
{
public:
    thread_specific_ptr()
        : m_tss(&thread_specific_ptr<T>::cleanup) { }
    thread_specific_ptr(void (*clean)(void*))
        : m_tss(clean) { }
    ~thread_specific_ptr() { reset(); }

    T* get() const { return static_cast<T*>(m_tss.get()); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    T* release() { T* temp = get(); if (tmp) m_tss.set(0); return temp; }
    void reset(T* p=0)
    {
        T* cur = get();
        if (cur == p) return;
        m_tss.set(p);
        if (cur) m_tss.cleanup(cur);
    }

private:
    static void cleanup(void* p) { delete static_cast<T*>(p); }
    detail::tss m_tss;
};

} // namespace boost

namespace {
    // This injects a tss_ref into every namespace and helps to insure we
    // get a proper value for the "main" thread
    boost::detail::tss_ref _tss_ref__7BAFF4714CFC42ae9C425F60CE3714D8;
}

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
//  30 May 02  WEKEMPF Added interface to set specific cleanup handlers.
//                     Removed TLS slot limits from most implementations.

#endif // BOOST_TSS_WEK070601_HPP
