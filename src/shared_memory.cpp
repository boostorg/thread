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

#include <sys/shm.h>
#include <sys/mman.h>

// Need to busy-wait on POSIX
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#endif


namespace {

const int HEADER_ALIGN=16;

struct hdr
{
    size_t len;
    unsigned int count;
};

void fillzero(void *ptr, size_t len)
{
    memset(ptr,0,len);
}

void noinit(void *,size_t)
{
}

}

namespace boost {

shared_memory::shared_memory(const char *name, size_t len)
    : m_ptr(NULL), m_mem_obj(0), m_h_event(NULL), m_len(len),
      m_initfunc(&noinit)
{
    create(name,len);
}

// Obtain a shared memory block and initialize it with initfunc
shared_memory::shared_memory(const char *name, size_t len,
    const boost::function2<void,void *,size_t> &initfunc)
    : m_ptr(NULL), m_mem_obj(0), m_h_event(NULL), m_len(len),
      m_initfunc(initfunc)
{
    create(name,len);
}

shared_memory::~shared_memory()
{
    if (m_ptr)
    {
        m_ptr = ((char *) m_ptr - HEADER_ALIGN);
        hdr *p_hdr = (hdr *)m_ptr;

        if (p_hdr)
        {
            p_hdr->count--;
        }

#if defined(BOOST_HAS_WINTHREADS)
        UnmapViewOfFile(m_ptr);

        if (m_mem_obj)
        {
            CloseHandle(reinterpret_cast<HANDLE>(m_mem_obj));
        }
        if (m_h_event)
        {
            CloseHandle(reinterpret_cast<HANDLE>(m_h_event));
        }
#elif defined (BOOST_HAS_PTHREADS)
        if (p_hdr->count == 0)
        {
            shm_unlink(m_name);
        }
        munmap(m_ptr,m_len);
#endif
    }
}

void shared_memory::create(const char *name, size_t len)
{
#if defined(BOOST_HAS_WINTHREADS)
    HANDLE h_map = NULL;
    HANDLE h_event = NULL;

    DWORD  ret;
    bool b_creator = false;

    std::string obj_name = "_EVT_";
    obj_name += name;
    h_event = CreateEvent(NULL,TRUE,FALSE,obj_name.c_str());

    if (h_event == NULL)
    {
        throw boost::thread_resource_error();
    }
    b_creator = (GetLastError() != ERROR_ALREADY_EXISTS);

    h_map = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE,
        0, len + HEADER_ALIGN, name);

    if (h_map)
    {
        m_ptr = static_cast<char*>(
            MapViewOfFile(h_map, FILE_MAP_WRITE, 0, 0, 0));
        if (m_ptr)
        {
            // Get a pointer to our header, and move our real ptr past this.
            hdr *p_hdr = (hdr *)m_ptr;
            m_ptr = ((char *)m_ptr + HEADER_ALIGN);

            if (b_creator)
            {
                // Call the initialization function for the user area.
                m_initfunc(m_ptr,len);
                p_hdr->len = len;
                p_hdr->count = 1;
                SetEvent(h_event);
            }
            else
            {
                ret = WaitForSingleObject(h_event,INFINITE);
                if (ret != WAIT_OBJECT_0)
                {
                    CloseHandle(h_event);
                    CloseHandle(h_map);
                    throw boost::thread_resource_error();
                }

                // We've got a previously constructed object.
                (p_hdr->count)++;
            }
        }
        else
        {
            CloseHandle(h_event);
            throw boost::thread_resource_error();
        }
    }

    m_mem_obj = reinterpret_cast<int>(h_map);
    m_h_event = reinterpret_cast<void *>(h_event);

#elif defined (BOOST_HAS_PTHREADS)
    int fd_smo;      // descriptor to shared memory object
    bool b_creator = true;

    fd_smo = shm_open(name, O_RDWR|O_CREAT|O_EXCL, SHM_MODE);

    if (fd_smo == -1)
    {
        if (errno == EEXIST)
        {
            // We lost the race.  We should just re-open with shared access
            //  below.
            fd_smo = shm_open(name,O_RDWR,SHM_MODE);

            b_creator = false
        }
        else
        {
            throw boost::thread_resource_error();
        }
    }
    else
    {
        // We're the creator.  Use ftrunctate to set the size.
        b_creator = true;
        //
        // Add error check on ftruncate.
        ftruncate(fd_smo,len+HEADER_ALIGN);
    }

    m_ptr = (char *)mmap(NULL, len + HEADER_ALIGN, PROT_READ|PROT_WRITE,
        MAP_SHARED, fd_smo, 0);

    if (m_ptr)
    {
        // Get a pointer to our header, and move our real ptr past this.
        hdr *p_hdr = (hdr *)ptr;
        m_ptr = ((char *)m_ptr + HEADER_ALIGN);

        if(b_creator)
        {
            // Call the initialization function for the user area.
            //flock(fd_smo);
            initfunc(ptr,len);
            p_hdr->len = len;
            p_hdr->count = 1;

            //funlock(fd_sm0);
        }
        else
        {
            // Need an event here.  For now, busy wait.
            while(p_hdr->len == 0)
            {
                //flock(fd_smo);
                //funlock(fd_smo);
                boost::xtime xt;
                boost::xtime_get(&xt,boost::TIME_UTC);
                xt.sec++;

                boost::thread::sleep(xt);
            }

            // We've got a previously constructed object.
            (p_hdr->count)++;
        }
    }
    close(fd_smo);
    mem_obj = NULL;         //reinterpret_cast<int>(h_map);
    event_obj = NULL;       //reinterpret_cast<void *>(h_event);
#endif
}

}   // namespace boost
