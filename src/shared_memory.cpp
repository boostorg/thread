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
#include <boost/thread/once.hpp>
#include <boost/thread/mutex.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#include <windows.h>
#include <winbase.h>

// Next line should really be BOOST_HAS_POSIX_xxx
#elif defined(BOOST_HAS_PTHREADS)

//#include <sys/shm.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>

#endif

namespace {

std::string encode(const char* str)
{
	const char* digits="0123456789abcdef";
	std::string result;
	while (str)
	{
		if (((*str >= '0') && (*str <= '9')) ||
			((*str >= 'a') && (*str <= 'z')) ||
			((*str >= 'A') && (*str <= 'Z')) ||
			(*str == '/') || (*str == '.') || (*str == '_'))
		{
			result += *str;
		}
		else if (*str == ' ')
		{
			result += '+';
		}
		else
		{
			result += '%';
			int v = *str;
			std::string hex;
			while (v)
			{
				hex = std::string() + digits[v % 16] + hex;
				v /= 16;
			}
			result += hex;
		}
	}
	return result;
}

std::string get_root()
{

#if defined(BOOST_HAS_WINTHREADS)
	return "";
#elif defined(BOOST_HAS_PTHREADS)
    return "/";
#else
	return "";
#endif
}

} // namespace

namespace boost {

shared_memory::shared_memory(const char *name, size_t len, int flags)
{
    init(name, len, flags, 0);
}

shared_memory::shared_memory(const char *name, size_t len, int flags,
    const boost::function1<void, void *>& initfunc)
{
    init(name, len, flags & create, &initfunc);
}

shared_memory::~shared_memory()
{
    if (m_ptr)
    {
        int res = 0;
#if defined(BOOST_HAS_WINTHREADS)
        res = UnmapViewOfFile(m_ptr);
        assert(res);
        res = CloseHandle(reinterpret_cast<HANDLE>(m_hmap));
        assert(res);
#elif defined(BOOST_HAS_PTHREADS)
        res = munmap(reinterpret_cast<char*>(m_ptr), m_len);
        assert(res == 0);
        res = close(m_hmap);
        assert(res == 0);
        res = shm_unlink(m_name.c_str());
        assert(res == 0);
#endif
    }
}

void shared_memory::init(const char *name, size_t len, int flags,
    const boost::function1<void,void *>* initfunc)
{
    int res = 0;
    bool should_init = false;

    std::string sname;
    if (name[0] == '%')
        sname = &name[1];
    else
        sname = get_root() + encode(name);
    std::string mxname = sname + "mx94543CBD1523443dB128451E51B5103E";

#if defined(BOOST_HAS_WINTHREADS)
    HANDLE mutex = CreateMutexA(0, FALSE, mxname.c_str());
    if (mutex == INVALID_HANDLE_VALUE)
        throw thread_resource_error();
    res = WaitForSingleObject(mutex, INFINITE);
    assert(res == WAIT_OBJECT_0);

    if (flags & create)
    {
        DWORD protect = (flags & write) ? PAGE_READWRITE : PAGE_READONLY;
        m_hmap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, protect,
            0, len, sname.c_str());
        if (m_hmap == INVALID_HANDLE_VALUE ||
            ((flags & exclusive) && GetLastError() == ERROR_ALREADY_EXISTS))
        {
            res = ReleaseMutex(mutex);
            assert(res);
            res = CloseHandle(mutex);
            assert(res);
            if (m_hmap != INVALID_HANDLE_VALUE)
            {
                res = CloseHandle(m_hmap);
                assert(res);
            }
            throw thread_resource_error();
        }
        should_init = GetLastError() != ERROR_ALREADY_EXISTS;
    }
    else
    {
        DWORD protect = (flags & write) ? FILE_MAP_WRITE : FILE_MAP_READ;
        m_hmap = OpenFileMapping(protect, FALSE, sname.c_str());
        if (m_hmap == INVALID_HANDLE_VALUE)
        {
            res = ReleaseMutex(mutex);
            assert(res);
            res = CloseHandle(mutex);
            assert(res);
            throw thread_resource_error();
        }
    }
    m_ptr = MapViewOfFile(m_hmap, FILE_MAP_WRITE, 0, 0, 0);
    assert(m_ptr);
#elif defined(BOOST_HAS_PTHREADS)
    m_name = sname;
    m_len = len;

	sem_t* sem = sem_open(mxname.c_str(), O_CREAT);
	if (sem == SEM_FAILED)
		throw thread_resource_error();
	res = sem_wait(sem);
	assert(res == 0);

	int oflag = (flags & write) ? O_RDWR : O_RDONLY;
	int cflag = (flags & create) ? O_CREAT|O_TRUNC|O_EXCL : 0;
	for (;;)
	{
		m_hmap = shm_open(name, oflag|cflag, 0);
		if (m_hmap == -1)
		{
			if (errno != EEXIST || (flags & exclusive))
			{
				res = sem_post(sem);
				assert(res == 0);
				res = sem_close(sem);
				assert(res == 0);
				res = sem_unlink(mxname.c_str());
				assert(res);
				throw thread_resource_error();
			}
			m_hmap = shm_open(name, oflag, 0);
			if (m_hmap == -1)
			{
				if (errno == ENOENT)
					continue;
				res = sem_post(sem);
				assert(res == 0);
				res = sem_close(sem);
				assert(res);
				res = sem_unlink(mxname.c_str());
				assert(res);
				throw thread_resource_error();
			}
		}
		else
			should_init = true;
		break;
	}
		
	ftruncate(m_hmap, len);
	int prot = (flags & write) ? PROT_READ|PROT_WRITE : PROT_READ;
	m_ptr = mmap(0, m_len, prot, MAP_SHARED, m_hmap, 0);
#endif

    if (should_init && initfunc)
        (*initfunc)(m_ptr);

#if defined(BOOST_HAS_WINTHREADS)
    res = ReleaseMutex(mutex);
    assert(res);
    res = CloseHandle(mutex);
    assert(res);
#elif defined(BOOST_HAS_PTHREADS)
    res = sem_post(sem);
    assert(res == 0);
    res = sem_close(sem);
    assert(res == 0);
    res = sem_unlink(mxname.c_str());
    assert(res == 0);
#endif
}

}   // namespace boost
