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
 
#include <boost/thread/atomic.hpp>

#if defined(BOOST_HAS_WINTHREADS)
#   include <windows.h>
#endif

namespace boost {
	atomic_t::value_type read(const atomic_t& x)
	{
		return x._value;
	}

#if defined(BOOST_HAS_WINTHREADS)
	atomic_t::value_type increment(atomic_t& x)
	{
		return InterlockedIncrement(const_cast<long*>(&x._value));
	}

	atomic_t::value_type decrement(atomic_t& x)
	{
		return InterlockedDecrement(const_cast<long*>(&x._value));
	}

	atomic_t::value_type swap(atomic_t& x, atomic_t::value_type y)
	{
		return InterlockedExchange(const_cast<long*>(&x._value), y);
	}

	atomic_t::value_type compare_swap(atomic_t& x, atomic_t::value_type y, atomic_t::value_type z)
	{
		return InterlockedCompareExchange(const_cast<long*>(&x._value), y, z);
	}
#else
	atomic_t::value_type increment(atomic_t& x)
	{
		mutex::lock lock(x._mutex);
		return ++x._value;
	}

	atomic_t::value_type decrement(atomic_t& x)
	{
		mutex::lock lock(x._mutex);
		return --x._value;
	}

	atomic_t::value_type swap(atomic_t& x, atomic_t::value_type y)
	{
		mutex::lock lock(x._mutex);
		atomic_t::value_type temp = x._value;
		x._value = y;
		return temp;
	}

	atomic_t::value_type compare_swap(atomic_t& x, atomic_t::value_type y, atomic_t::value_type z)
	{
		mutex::lock lock(x._mutex);
		atomic_t::value_type temp = x._value;
		if (temp == z)
			x._value = y;
		return temp;
	}
#endif
} // namespace boost
