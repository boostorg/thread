// Copyright (C) 2002-2003
// William E. Kempf, David Moore
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_MUTEX_JDM062402_HPP
#define BOOST_MUTEX_JDM062402_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error   Thread support is unavailable!
#endif

#include <boost/utility.hpp>
#include <boost/function.hpp>

#include <boost/thread/exceptions.hpp>

#include <string>

namespace boost {

class shared_memory
{
public:
	enum { read=1, read_write=2, create=4 };
	
    // Obtain a shared memory block len bytes long, zero initialized
    shared_memory(const char *name, size_t len, int flags);
    // Obtain a shared memory block and initialize it with initfunc
    shared_memory(const char *name, size_t len, int flags,
        const boost::function1<void,void *> &initfunc);
    ~shared_memory();

	void* map();
	void unmap();
    
private:
    void init(const char *name, size_t len, int flags,
			  const boost::function1<void, void*>* initfunc);

    void *m_ptr;        // Pointer to shared memory block
#if defined(BOOST_HAS_WINTHREADS)
	void* m_hmap;
#elif defined(BOOST_HAS_PTHREADS)
	int m_hmap;
	int m_len;
	int m_flags;
	std::string m_name;
#endif
};

}  // namespace boost

#endif
