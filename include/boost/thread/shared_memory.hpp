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

// Yuck, but... 
#include <windows.h>

namespace boost {

    
// Example construct templates, with helper functions so that template args 
//   don't have to be specified, except for the type being created.

template<typename T>
class construct_0
{
public:
    explicit construct_0(){}
    T *operator()(void *placement){return new(placement) T;}
};

template<typename T> inline
construct_0<T> construct0(){return construct_0<T>();}

template<typename T,typename A1>
class construct_1
{
public:
    explicit construct_1(A1 &a1) : m_a1(a1){}
    T *operator()(void *placement){return new(placement) T(m_a1);}
private:
    A1 &m_a1;
};

template<typename T,typename _A1> inline
    construct_1<T,_A1> __cdecl construct1( _A1& _a1)
    {return (construct_1<T,_A1>(_a1));}


template<typename T,typename A1,typename A2>
class construct_2
{
public:
    explicit construct_2( A1 &a1, A2 &a2) : m_a1(a1),m_a2(a2){}
    T *operator()(void *placement){return new(placement) T(m_a1,m_a2);}
private:
    A1 &m_a1;
    A2 &m_a2;
};

template<typename T,typename A1,typename A2> inline
    construct_2<T,A1,A2> __cdecl construct2( A1 &a1, A2 &a2)
    {return (construct_2<T,A1,A2>(a1,a2));}

    const int HEADER_ALIGN=16;

    template<typename T> class shared_object
    {
        public:
            // 1st form creates the object.
            explicit shared_object(const char *name,
                                   const char *path,
                                   const boost::function1<T*,void *>& creatorfunc);
            ~shared_object();

            T   *get();
            
        private:
            struct hdr
            {
                size_t len;
                unsigned int count;
            };

            // Pointer to memory
            T   *m_ptr;    
            //const char *  m_p_name;
            //const char *  m_p_path;
            boost::function1<T*,void *> m_creatorfunc;

            HANDLE m_h_mutex;
            HANDLE m_h_map;
            char *m_p_buf;

    };

    template<typename T>
    shared_object<T>::
    shared_object(const char *name,const char *path, const function1<T*,void*>& creatorfunc) : 
        m_creatorfunc(creatorfunc), m_ptr(NULL)
    {
        HANDLE h_file = INVALID_HANDLE_VALUE;
        m_h_map = NULL;
        m_h_mutex = NULL;
        HANDLE h_event = NULL;

        DWORD  ret;
        bool b_creator = false;
        
        std::string obj_name = "_MTX_";
        obj_name += name;
        m_h_mutex = CreateMutex(NULL,FALSE,obj_name.c_str());

        if(m_h_mutex == NULL)
            throw thread_resource_error();

        obj_name.replace(0,5,"_EVT_");
        h_event = CreateEvent(NULL,TRUE,FALSE,obj_name.c_str());

        if(h_event == NULL)
        {
            CloseHandle(m_h_mutex);
            throw thread_resource_error();
        }

        // If path, create the file mapping in a file in the file system
        if(path)
        {
            h_file = CreateFile(path,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
            if(h_file != INVALID_HANDLE_VALUE)
            {
                b_creator = (GetLastError() != ERROR_ALREADY_EXISTS);
            }
            m_h_map = CreateFileMapping(h_file,NULL,PAGE_READWRITE,0,sizeof(T)+HEADER_ALIGN,name);
            CloseHandle(h_file);
        }
        // If no path, mapping is created in the pagefile.
        else
        {
            m_h_map = CreateFileMapping(h_file,NULL,PAGE_READWRITE,0,sizeof(T)+HEADER_ALIGN,name);
            b_creator = (GetLastError() != ERROR_ALREADY_EXISTS);
        }

        if(m_h_map)
        {
            m_p_buf = (char *)MapViewOfFile(m_h_map,FILE_MAP_WRITE,0,0,0);
            if(m_p_buf)
            {
                hdr *p_hdr = (hdr *) m_p_buf;
                ret = WaitForSingleObject(m_h_mutex,INFINITE);
                if(ret == WAIT_OBJECT_0)
                {
                    if(b_creator)
                    {
                        m_ptr = m_creatorfunc((char *)m_p_buf + HEADER_ALIGN);
                        p_hdr->len = sizeof(T);
                        p_hdr->count = 1;
                        ReleaseMutex(m_h_mutex);
                        SetEvent(h_event);
                    }
                    else
                    {
                        if(p_hdr->len == 0)
                        {   // We "won" the race and grabbed the mutex even though
                            // we didn't create the file.  Release the mutex and wait
                            //   on both the mutex and the event so we can know the
                            //   creator is done.
                            ReleaseMutex(m_h_mutex);
                            HANDLE h[2];
                            h[0] = m_h_mutex;
                            h[1] = h_event;
                            ret = WaitForMultipleObjects(2,h,TRUE,INFINITE);

                            // This check makes me nervous.  Can a race occur where
                            //   we get an abandoned mutex or something?
                            if(ret != WAIT_OBJECT_0 && ret != WAIT_OBJECT_0 + 1)
                            {
                                CloseHandle(m_h_mutex);
                                CloseHandle(h_event);
                                throw thread_resource_error();
                            }
                        }
                            // Set our pointer to the previously created object.
                        m_ptr = (T*) ((char *)m_p_buf + HEADER_ALIGN);
                        (p_hdr->count)++;
                        ReleaseMutex(m_h_mutex);
                        
                    }
                }
                else
                {
                    CloseHandle(m_h_mutex);
                    CloseHandle(h_event);

                    throw thread_resource_error();
                }
            }
        }
        CloseHandle(h_event);
    }

    

    template<typename T>
    shared_object<T>::
    ~shared_object()
    {
        // Question:  who destroys T?
        // Creator seems reasonable at first, but users may outlive creator...
        //
        // This is becoming a shared_ptr problem, but a nasty one since the 
        //   lifetime of creators and users won't necessarily overlap.
        //
        hdr *p_hdr = (hdr *)m_p_buf;

        if(p_hdr)
        {
            p_hdr->count--;
            // VC++ 6 skips right over this check, doesn't even generate code.
            if(p_hdr->count == 0)
            {
                if(m_ptr)
                {
                    m_ptr->~T();
                }   
            }
        }
        if(m_p_buf)
        {
            UnmapViewOfFile(m_p_buf);
        }
        if(m_h_map)
        {
            CloseHandle(m_h_map);
        }
        CloseHandle(m_h_mutex);

    }


    template<typename T>
    T   *
    shared_object<T>::
    get()
    {
        return m_ptr;
    }


};  // namespace boost

#endif
