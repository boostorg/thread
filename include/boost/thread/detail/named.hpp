// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_NAMED_WEK031703_HPP
#define BOOST_NAMED_WEK031703_HPP

#include <boost/thread/detail/config.hpp>

namespace boost {
namespace detail {

class named_object
{
protected:
    named_object(const char* name=0);
    ~named_object();

public:
    const char* name() const;
    const char* effective_name() const;

protected:
    char* m_name;
    char* m_ename;
};

} // namespace detail
} // namespace boost

#endif // BOOST_NAMED_WEK031703_HPP
