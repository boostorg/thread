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


namespace boost {

    class shared_memory
    {
    public:
        // Obtain a shared memory block len bytes long, zero initialized
        shared_memory(const char *name,size_t len);
        // Obtain a shared memory block and initialize it with initfunc
        shared_memory(const char *name,size_t len,const boost::function2<void,void *,size_t> &initfunc);
        ~shared_memory();

        void *get(){return m_ptr;}
    private:

        void create(const char *name,
                    size_t len);


        void *m_ptr;        // Pointer to shared memory block
        int   m_mem_obj;    // Platform specific handle to shared memory block
        void *m_h_event;    // Platform specific handle to event saying block initialized.
        size_t  m_len;

        boost::function2<void,void *,size_t> m_initfunc;

    };
};  // namespace boost

#endif
