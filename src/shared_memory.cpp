// Copyright (C) 2002
// William E. Kempf, David Moore
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/shared_memory.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#include <windows.h>

// Next line should really be BOOST_HAS_POSIX_xxx
#elif defined(BOOST_HAS_PTHREADS)

//#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>

#endif

namespace boost {

shared_memory::shared_memory(const char *name, size_t len, int flags)
    : m_ptr(NULL)
#if defined(BOOST_HAS_WINTHREADS)
	, m_hmap(INVALID_HANDLE_VALUE)
#elif defined(BOOST_HAS_PTHREADS)
    , m_hmap(0), m_len(len), m_flags(flags), m_name(name)
#endif 
{
    init(name, len, flags, 0);
}

shared_memory::shared_memory(const char *name, size_t len, int flags,
    const boost::function1<void, void *>& initfunc)
    : m_ptr(NULL)
#if defined(BOOST_HAS_WINTHREADS)
	, m_hmap(INVALID_HANDLE_VALUE)
#elif defined(BOOST_HAS_PTHREADS) 
    , m_hmap(0), m_len(len), m_flags(flags), m_name(name)
#endif 
{
	init(name, len, flags & create, &initfunc);
}

shared_memory::~shared_memory()
{
	unmap();

#if defined(BOOST_HAS_WINTHREADS)
	CloseHandle(reinterpret_cast<HANDLE>(m_hmap));
#elif defined(BOOST_HAS_PTHREADS)
	close(m_hmap);
	shm_unlink(m_name);
#endif
}

void shared_memory::init(const char *name, size_t len, int flags,
	const boost::function1<void,void *>* initfunc)
{
	std::string mxname = std::string(name) +
		"mx94543CBD1523443dB128451E51B5103E";
#if defined(BOOST_HAS_WINTHREADS)
	HANDLE mutex = CreateMutexA(0, FALSE, mxname.c_str());
	WaitForSingleObject(mutex, INFINITE);

	DWORD protect = (flags & read_write) ? PAGE_READWRITE : PAGE_READONLY;
    m_hmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, protect,
		0, len, name);
#elif defined(BOOST_HAS_PTHREADS)
	sem_t sem = sem_open(mxname.c_str(), O_CREAT);
	sem_wait(&sem);

	int oflag = (flags & read_write) ? O_RDWR : O_RDONLY;
	if (flags & create)
		oflag |= O_CREAT|O_TRUNC;
	m_hmap = shm_open(name, oflag, 0);
	ftruncate(m_hmap, len);
#endif

	if (initfunc)
	{
		(*initfunc)(map());
		unmap();
	}

#if defined(BOOST_HAS_WINTHREADS)
	ReleaseMutex(mutex);
	CloseHandle(mutex);
#elif defined(BOOST_HAS_PTHREADS)
	sem_post(&sem);
	sem_close(&sem);
	sem_unlink(&sem);
#endif 
}

void* shared_memory::map()
{
	if (!m_ptr)
	{
#if defined(BOOST_HAS_WINTHREADS)
        m_ptr = static_cast<char*>(
            MapViewOfFile(m_hmap, FILE_MAP_WRITE, 0, 0, 0));
#elif defined(BOOST_HAS_PTHREADS)
		int prot = (m_flags & read_write) ? PROT_READ|PROT_WRITE : PROT_READ;
		m_ptr = mmap(0, m_len, prot, MAP_SHARED, m_hmap, 0);
#endif 
	}
	return m_ptr;
}

void shared_memory::unmap()
{
	if (m_ptr)
	{
#if defined(BOOST_HAS_WINTHREADS)
        UnmapViewOfFile(m_ptr);
#elif defined(BOOST_HAS_PTHREADS)
		munmap(reinterpret_cast<char*>(m_ptr), m_len);
#endif 
	}
}

}   // namespace boost
