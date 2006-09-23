// Copyright (C) 2001-2003 William E. Kempf
// Copyright (C) 2006 Roland Schwarz
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// This work is a reimplementation along the design and ideas
// of William E. Kempf.

#include <boost/thread/pthread/config.hpp>

#include <boost/thread/pthread/once.hpp>

namespace {
pthread_key_t key;
pthread_once_t once = PTHREAD_ONCE_INIT;

typedef void (*once_callback)();
}

extern "C" {

    static void key_init()
    {
        pthread_key_create(&key, 0);
    }

    static void do_once()
    {
        once_callback* cb = reinterpret_cast<once_callback*>(
            pthread_getspecific(key));
        (**cb)();
    }

}

namespace boost {

void call_once(void (*func)(), once_flag& flag)
{
    pthread_once(&once, &key_init);
    pthread_setspecific(key, &func);
    pthread_once(&flag, do_once);
}

}
