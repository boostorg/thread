// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

//
// includes
//

#include <cstdlib>

#include <Multiprocessing.h>


//
// using declarations
//

using std::size_t;


extern "C" {

//
// prototypes
//

void *malloc(size_t ulSize);
void free(void *pBlock);

}


//
// MSL function replacements
//

// all allocation/deallocation currently goes through MPAllocateAligned/MPFree.  This
//    solution is sub-optimal at best, but will have to do for now.
void *malloc(size_t ulSize)
{
    static bool bIgnored = MPLibraryIsLoaded();
    return(MPAllocateAligned(ulSize, kMPAllocateDefaultAligned, 0UL));
}

void free(void *pBlock)
{
    if(pBlock == NULL) return;
    MPFree(pBlock);
}
