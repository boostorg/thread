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
#include <boost/function.hpp>

#if defined(BOOST_HAS_PTHREADS)
#   include <pthread.h>
#elif defined(BOOST_HAS_MPTASKS)
#   include <Multiprocessing.h>
#endif

namespace boost {

    namespace detail {
        class tss : private noncopyable
        {
        public:
			tss(boost::function1<void, void*> cleanup);
            ~tss();

            void* get() const;
            void set(void* value);
			void cleanup(void* value);

        private:
        #if defined(BOOST_HAS_WINTHREADS)
            unsigned long m_key;
        #elif defined(BOOST_HAS_PTHREADS)
            pthread_key_t m_key;
        #elif defined(BOOST_HAS_MPTASKS)
            TaskStorageIndex m_key;
            void (*m_cleanup)(void*);
        #endif
			boost::function1<void, void*> m_clean;
        };

    #if defined(BOOST_HAS_MPTASKS)
        void thread_cleanup();
    #endif

		struct tss_adapter
		{
			tss_adapter(boost::function1<void, void*> cleanup) : m_cleanup(cleanup) { }
			void operator()(void* p) { m_cleanup(p); }
			boost::function1<void, void*> m_cleanup;
		};
    }

template <typename T>
class thread_specific_ptr : private noncopyable
{
public:
	thread_specific_ptr() : m_tss(detail::tss_adapter(&thread_specific_ptr<T>::cleanup)) { }
	thread_specific_ptr(void (*clean)(void*)) : m_tss(detail::tss_adapter(clean)) { }

    T* get() const { return static_cast<T*>(m_tss.get()); }
    T* operator->() const { return get(); }
    T& operator*() const { return *get(); }
    T* release() { T* temp = get(); m_tss.set(0); return temp; }
    void reset(T* p=0) { T* cur = get(); if (cur == p) return; m_tss.set(p); if (cur) m_tss.cleanup(cur); }

private:
    static void cleanup(void* p) { delete static_cast<T*>(p); }
    detail::tss m_tss;
};

} // namespace boost

// Change Log:
//   6 Jun 01  WEKEMPF Initial version.
//  30 May 02  WEKEMPF Added interface to set specific cleanup handlers. Removed TLS slot limits
//                     from most implementations.

#endif // BOOST_TSS_WEK070601_HPP

