/*
 *
 * Copyright (C) 2001
 * William E. Kempf
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appear in all copies and
 * that both that copyright notice and this permission notice appear
 * in supporting documentation.  William E. Kempf makes no representations
 * about the suitability of this software for any purpose.  
 * It is provided "as is" without express or implied warranty.
 *
 * Revision History (excluding minor changes for specific compilers)
 *   8 Feb 01  Initial version.
 */
 
#ifndef BOOST_ATOMIC_HPP
#define BOOST_ATOMIC_HPP

#include <boost/config.hpp>
#ifndef BOOST_HAS_THREADS
#   error	Thread support is unavailable!
#endif

#if !defined(BOOST_HAS_WINTHREADS)
#   include <boost/thread/mutex.hpp>
#endif

namespace boost {
    class atomic_t
    {
    public:
        typedef long value_type;
        
        friend value_type read(const atomic_t&);
        friend value_type increment(atomic_t&);
        friend value_type decrement(atomic_t&);
        friend value_type swap(atomic_t&, value_type);
        friend value_type compare_swap(atomic_t&, value_type, value_type);
        
        explicit atomic_t(value_type val=0)
            : _value(val)
        {
        }
        
    private:
        volatile value_type _value;
#if !defined(BOOST_HAS_WINTHREADS)
        mutex _mutex;
#endif
    };
    
    extern atomic_t::value_type read(const atomic_t&);
    extern atomic_t::value_type increment(atomic_t&);
    extern atomic_t::value_type decrement(atomic_t&);
    extern atomic_t::value_type swap(atomic_t&, atomic_t::value_type);
    extern atomic_t::value_type compare_swap(atomic_t&, atomic_t::value_type, atomic_t::value_type);
} // namespace boost

#endif // BOOST_ATOMIC_HPP
