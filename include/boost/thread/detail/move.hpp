// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#ifndef BOOST_THREAD_MOVE_HPP
#define BOOST_THREAD_MOVE_HPP

namespace boost
{
    namespace detail
    {
        template<typename T>
        struct thread_move_t
        {
            T& t;
            thread_move_t(T& t_):
                t(t_)
            {}

            T* operator->() const
            {
                return &t;
            }
        };

        template<typename T>
        thread_move_t<T> thread_move(T& t)
        {
            return thread_move_t<T>(t);
        }
    }
    
}


#endif
