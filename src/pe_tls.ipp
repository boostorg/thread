// (C) Copyright Aaron W. LaFramboise 2004.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

extern "C" void tss_cleanup_implemented(void)
{
    //Don't need to do anything; this function's 
    //sole purpose is to cause a link error in cases
    //where tss cleanup is not implemented by Boost.Threads
    //as a reminder that user code is responsible for calling
    //on_process_enter(), on_thread_exit(), and
    //on_process_exit() at the appropriate times
    //and implementing an empty tss_cleanup_implemented()
    //function to eliminate the link error.
}

//Force a TLS directory to be generated even when static TLS is not used.

extern "C" int _tls_used;

int dummy(void)
{
    return _tls_used;
}

//Report thread and process detach events.

void NTAPI tls_callback(PVOID, DWORD Reason, PVOID) 
{
    if (Reason == DLL_THREAD_DETACH)
        on_thread_exit();
    else if (Reason == DLL_PROCESS_DETACH) 
        on_thread_exit();
}

// Add callback to the TLS callback list in TLS directory.

#pragma data_seg(push, old_seg)
#pragma data_seg(".CRT$XLB")
DWORD tls_callback_ptr = (DWORD)tls_callback;
#pragma data_seg(pop, old_seg)