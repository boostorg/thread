// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#include <cassert>
#include <cstdio>

#include <MacTypes.h>

#include "remote_calls.hpp"

// this function will be called when an assertion fails.  We redirect the assertion
//    to DebugStr (MacsBug under Mac OS 1.x-9.x, Console under Mac OS X).
void __assertion_failed(char const *pszAssertion, char const *pszFile, int nLine)
{
    using std::snprintf;
    unsigned char strlDebug[sizeof(Str255) + 1];
    char *pszDebug = reinterpret_cast<char *>(&strlDebug[1]);
    strlDebug[0] = snprintf(pszDebug, sizeof(Str255), "assertion failed: \"%s\", %s, line %d", pszAssertion, pszFile, nLine);
    boost::threads::mac::dt_remote_call(DebugStr, static_cast<ConstStringPtr>(strlDebug));
}
