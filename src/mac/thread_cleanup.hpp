// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_THREAD_CLEANUP_MJM012402_HPP
#define BOOST_THREAD_CLEANUP_MJM012402_HPP


namespace boost {

namespace threads {

namespace mac {

namespace detail {


void do_thread_startup();
void do_thread_cleanup();

void set_thread_cleanup_task();


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_THREAD_CLEANUP_MJM012402_HPP
