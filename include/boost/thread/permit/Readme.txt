pthreads Permit Object
-=-=-=-=-=-=-=-=-=-=-=
(C) 2010-2014 Niall Douglas http://www.nedproductions.biz/

Herein is a reference implementation for a proposed safe permit object for POSIX
threads. Its development came out of work done internally to ISO WG14 when making
the C11 release of the C programming language, but it was decided to submit the
proposal to the Austin Working Group for inclusion into POSIX after the 2012 TC1.

This reference implementation has been tested on:
 * Microsoft Windows 7 with Visual Studio 2010
 * Clang v3.1
 * GCC v4.6 (Mingw)
 * GCC v4.x (Ubuntu)

Build notes:
-=-=-=-=-=-=
There is an almighty hack of C11 atomics and threading support for Windows in
c11_compat.h. It is semantically correct however, but implements just enough
to get the reference object working.

On POSIX, C11's stdatomic.h and threads.h are not typically available as yet. As a
result, macro hackery is done to map C11's atomic and thread support to C++11's
atomics and POSIX threads.

The unit tests will make use of parallel multithreaded unit testing if you compile
it with OpenMP support enabled.
