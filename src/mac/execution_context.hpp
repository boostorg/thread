// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_EXECUTION_CONTEXT_MJM012402_HPP
#define BOOST_EXECUTION_CONTEXT_MJM012402_HPP

namespace boost {

namespace threads {

namespace mac {


// utility functions for figuring out what context your code is executing in.
//    Bear in mind that at_mp and in_blue are the only functions guarenteed by
//    Apple to work.  There is simply no way of being sure that you will not get
//    false readings about task level at interrupt time in blue.

typedef enum {
    k_eExecutionContextSystemTask,
    k_eExecutionContextDeferredTask,
    k_eExecutionContextMPTask,
    k_eExecutionContextOther
} execution_context_t;

execution_context_t execution_context();

inline bool at_st()
    {    return(execution_context() == k_eExecutionContextSystemTask);    }

inline bool at_mp()
    {    return(execution_context() == k_eExecutionContextMPTask);        }
inline bool in_blue()
    {    return(!at_mp());                                                }


} // namespace mac

} // namespace threads

} // namespace boost

#endif // BOOST_EXECUTION_CONTEXT_MJM012402_HPP
