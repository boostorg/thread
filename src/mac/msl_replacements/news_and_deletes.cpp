// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

//
// includes
//

#include <new>

#include <Multiprocessing.h>


//
// using declarations
//

using std::size_t;
using std::bad_alloc;
using std::nothrow_t;
using std::nothrow;


//
// local utility functions
//

// all allocation/deallocation currently goes through MPAllocateAligned/MPFree.  This
//    solution is sub-optimal at best, but will have to do for now.
inline static void *allocate(size_t ulSize, const nothrow_t &)
{
    static bool bIgnored = MPLibraryIsLoaded();
    return(MPAllocateAligned(ulSize, kMPAllocateDefaultAligned, 0UL));
}

inline static void *allocate(size_t ulSize)
{
    void *pBlock = allocate(ulSize, nothrow);
    if(pBlock == NULL)
        throw(bad_alloc());
    return(pBlock);
}

inline static void deallocate(void *pBlock)
{
    if(pBlock == NULL) return;
    MPFree(pBlock);
}


//
// global operators
//

void *operator new(size_t ulSize)
{
    return(allocate(ulSize));
}

void *operator new[](size_t ulSize)
{
    return(allocate(ulSize));
}


void *operator new(size_t ulSize, const nothrow_t &rNoThrow)
{
    return(allocate(ulSize, rNoThrow));
}

void *operator new[](size_t ulSize, const nothrow_t &rNoThrow)
{
    return(allocate(ulSize, rNoThrow));
}


void operator delete(void *pBlock)
{
    deallocate(pBlock);
}

void operator delete[](void *pBlock)
{
    deallocate(pBlock);
}


void operator delete(void *pBlock, const nothrow_t &)
{
    deallocate(pBlock);
}

void operator delete[](void *pBlock, const nothrow_t &)
{
    deallocate(pBlock);
}
