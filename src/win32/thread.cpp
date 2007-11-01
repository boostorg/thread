// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007 Anthony Williams

#include <boost/thread/thread.hpp>
#include <algorithm>
#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <boost/thread/once.hpp>
#include <boost/assert.hpp>

namespace boost
{
    namespace
    {
#ifdef _MSC_VER
        __declspec(thread) detail::thread_data_base* current_thread_data=0;
        detail::thread_data_base* get_current_thread_data()
        {
            return current_thread_data;
        }
        void set_current_thread_data(detail::thread_data_base* new_data)
        {
            current_thread_data=new_data;
        }
#elif defined(__BORLANDC__)
        detail::thread_data_base* __thread current_thread_data=0;
        detail::thread_data_base* get_current_thread_data()
        {
            return current_thread_data;
        }
        void set_current_thread_data(detail::thread_data_base* new_data)
        {
            current_thread_data=new_data;
        }
#else

        boost::once_flag current_thread_tls_init_flag=BOOST_ONCE_INIT;
        DWORD current_thread_tls_key=0;

        void create_current_thread_tls_key()
        {
            current_thread_tls_key=TlsAlloc();
            BOOST_ASSERT(current_thread_tls_key!=TLS_OUT_OF_INDEXES);
        }

        detail::thread_data_base* get_current_thread_data()
        {
            boost::call_once(current_thread_tls_init_flag,create_current_thread_tls_key);
            return (detail::thread_data_base*)TlsGetValue(current_thread_tls_key);
        }

        void set_current_thread_data(detail::thread_data_base* new_data)
        {
            boost::call_once(current_thread_tls_init_flag,create_current_thread_tls_key);
            BOOL const res=TlsSetValue(current_thread_tls_key,new_data);
            BOOST_ASSERT(res);
        }
#endif
    }

    void thread::yield()
    {
        this_thread::yield();
    }
    
    void thread::sleep(const system_time& target)
    {
        system_time const now(get_system_time());
        
        if(target<=now)
        {
            this_thread::yield();
        }
        else
        {
            this_thread::sleep(target-now);
        }
    }

    namespace detail
    {
        struct thread_exit_callback_node
        {
            boost::detail::thread_exit_function_base* func;
            thread_exit_callback_node* next;

            thread_exit_callback_node(boost::detail::thread_exit_function_base* func_,
                                      thread_exit_callback_node* next_):
                func(func_),next(next_)
            {}
        };

    }

    namespace
    {
        void run_thread_exit_callbacks()
        {
            boost::intrusive_ptr<detail::thread_data_base> current_thread_data(get_current_thread_data(),false);
            if(current_thread_data)
            {
                while(current_thread_data->thread_exit_callbacks)
                {
                    detail::thread_exit_callback_node* const current_node=current_thread_data->thread_exit_callbacks;
                    current_thread_data->thread_exit_callbacks=current_node->next;
                    if(current_node->func)
                    {
                        (*current_node->func)();
                        boost::detail::heap_delete(current_node->func);
                    }
                    boost::detail::heap_delete(current_node);
                }
            }
            set_current_thread_data(0);
        }
        
    }
    

    unsigned __stdcall thread::thread_start_function(void* param)
    {
        detail::thread_data_base* const thread_info(reinterpret_cast<detail::thread_data_base*>(param));
        set_current_thread_data(thread_info);
        try
        {
            thread_info->run();
        }
        catch(thread_cancelled const&)
        {
        }
        catch(...)
        {
            std::terminate();
        }
        run_thread_exit_callbacks();
        return 0;
    }

    thread::thread()
    {}

    void thread::start_thread()
    {
        uintptr_t const new_thread=_beginthreadex(0,0,&thread_start_function,thread_info.get(),CREATE_SUSPENDED,&thread_info->id);
        if(!new_thread)
        {
            throw thread_resource_error();
        }
        intrusive_ptr_add_ref(thread_info.get());
        thread_info->thread_handle=(detail::win32::handle)(new_thread);
        ResumeThread(thread_info->thread_handle);
    }

    thread::thread(boost::intrusive_ptr<detail::thread_data_base> data):
        thread_info(data)
    {}

    namespace
    {
        struct externally_launched_thread:
            detail::thread_data_base
        {
            externally_launched_thread()
            {
                ++count;
                thread_handle=detail::win32::duplicate_handle(detail::win32::GetCurrentThread());
            }
            ~externally_launched_thread()
            {
                OutputDebugString("External thread finished\n");
            }
            
            void run()
            {}
        };
    }

    thread thread::self()
    {
        if(!get_current_thread_data())
        {
            externally_launched_thread* me=detail::heap_new<externally_launched_thread>();
            set_current_thread_data(me);
        }
        return thread(boost::intrusive_ptr<detail::thread_data_base>(get_current_thread_data()));
    }
    
    thread::~thread()
    {
        detach();
    }
    
    thread::thread(boost::move_t<thread> x)
    {
        {
            boost::mutex::scoped_lock l(x->thread_info_mutex);
            thread_info=x->thread_info;
        }
        x->release_handle();
    }
    
    thread& thread::operator=(boost::move_t<thread> x)
    {
        thread new_thread(x);
        swap(new_thread);
        return *this;
    }
        
    thread::operator boost::move_t<thread>()
    {
        return move();
    }

    boost::move_t<thread> thread::move()
    {
        boost::move_t<thread> x(*this);
        return x;
    }

    void thread::swap(thread& x)
    {
        thread_info.swap(x.thread_info);
    }

    thread::id thread::get_id() const
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        return local_thread_info?thread::id(local_thread_info->id):thread::id();
    }

    thread::cancel_handle thread::get_cancel_handle() const
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        return local_thread_info?thread::cancel_handle(local_thread_info->cancel_handle.duplicate()):thread::cancel_handle();
    }
    
    bool thread::joinable() const
    {
        return get_thread_info();
    }

    void thread::join()
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        if(local_thread_info)
        {
            this_thread::cancellable_wait(local_thread_info->thread_handle,detail::win32::infinite);
            release_handle();
        }
    }

    bool thread::timed_join(boost::system_time const& wait_until)
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        if(local_thread_info)
        {
            if(!this_thread::cancellable_wait(local_thread_info->thread_handle,get_milliseconds_until(wait_until)))
            {
                return false;
            }
            release_handle();
        }
        return true;
    }
    
    void thread::detach()
    {
        release_handle();
    }

    void thread::release_handle()
    {
        boost::mutex::scoped_lock l1(thread_info_mutex);
        thread_info=0;
    }
    
    void thread::cancel()
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        if(local_thread_info)
        {
            detail::win32::SetEvent(local_thread_info->cancel_handle);
        }
    }
    
    bool thread::cancellation_requested() const
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        return local_thread_info.get() && (detail::win32::WaitForSingleObject(local_thread_info->cancel_handle,0)==0);
    }
    
    unsigned thread::hardware_concurrency()
    {
        SYSTEM_INFO info={0};
        GetSystemInfo(&info);
        return info.dwNumberOfProcessors;
    }
    
    thread::native_handle_type thread::native_handle()
    {
        boost::intrusive_ptr<detail::thread_data_base> local_thread_info=get_thread_info();
        return local_thread_info?(detail::win32::handle)local_thread_info->thread_handle:detail::win32::invalid_handle_value;
    }

    boost::intrusive_ptr<detail::thread_data_base> thread::get_thread_info() const
    {
        boost::mutex::scoped_lock l(thread_info_mutex);
        return thread_info;
    }

    namespace this_thread
    {
        thread::cancel_handle get_cancel_handle()
        {
            return get_current_thread_data()?thread::cancel_handle(get_current_thread_data()->cancel_handle.duplicate()):thread::cancel_handle();
        }

        bool cancellable_wait(detail::win32::handle handle_to_wait_for,unsigned long milliseconds)
        {
            detail::win32::handle handles[2]={0};
            unsigned handle_count=0;
            unsigned cancel_index=~0U;
            if(handle_to_wait_for!=detail::win32::invalid_handle_value)
            {
                handles[handle_count++]=handle_to_wait_for;
            }
            if(get_current_thread_data() && get_current_thread_data()->cancel_enabled)
            {
                cancel_index=handle_count;
                handles[handle_count++]=get_current_thread_data()->cancel_handle;
            }
        
            if(handle_count)
            {
                unsigned long const notified_index=detail::win32::WaitForMultipleObjects(handle_count,handles,false,milliseconds);
                if((handle_to_wait_for!=detail::win32::invalid_handle_value) && !notified_index)
                {
                    return true;
                }
                else if(notified_index==cancel_index)
                {
                    detail::win32::ResetEvent(get_current_thread_data()->cancel_handle);
                    throw thread_cancelled();
                }
            }
            else
            {
                detail::win32::Sleep(milliseconds);
            }
            return false;
        }

        thread::id get_id()
        {
            return thread::id(detail::win32::GetCurrentThreadId());
        }

        void cancellation_point()
        {
            if(cancellation_enabled() && cancellation_requested())
            {
                detail::win32::ResetEvent(get_current_thread_data()->cancel_handle);
                throw thread_cancelled();
            }
        }
        
        bool cancellation_enabled()
        {
            return get_current_thread_data() && get_current_thread_data()->cancel_enabled;
        }
        
        bool cancellation_requested()
        {
            return get_current_thread_data() && (detail::win32::WaitForSingleObject(get_current_thread_data()->cancel_handle,0)==0);
        }

        void yield()
        {
            detail::win32::Sleep(0);
        }
        
        disable_cancellation::disable_cancellation():
            cancel_was_enabled(cancellation_enabled())
        {
            if(cancel_was_enabled)
            {
                get_current_thread_data()->cancel_enabled=false;
            }
        }
        
        disable_cancellation::~disable_cancellation()
        {
            if(get_current_thread_data())
            {
                get_current_thread_data()->cancel_enabled=cancel_was_enabled;
            }
        }

        restore_cancellation::restore_cancellation(disable_cancellation& d)
        {
            if(d.cancel_was_enabled)
            {
                get_current_thread_data()->cancel_enabled=true;
            }
        }
        
        restore_cancellation::~restore_cancellation()
        {
            if(get_current_thread_data())
            {
                get_current_thread_data()->cancel_enabled=false;
            }
        }
    }

    namespace
    {
        void NTAPI thread_exit_func_callback(HINSTANCE, DWORD, PVOID);
        typedef void (NTAPI* tls_callback)(HINSTANCE, DWORD, PVOID);
        
#ifdef _MSC_VER
        extern "C"
        {
            extern DWORD _tls_used; //the tls directory (located in .rdata segment)
            extern tls_callback __xl_a[], __xl_z[];    //tls initializers */
        }
        
#if (_MSC_VER >= 1300) // 1300 == VC++ 7.0
#   pragma data_seg(push, old_seg)
#endif
        
#pragma data_seg(".CRT$XLB")
        tls_callback p_thread_callback = thread_exit_func_callback;
#pragma data_seg()
        
#if (_MSC_VER >= 1300) // 1300 == VC++ 7.0
#   pragma data_seg(pop, old_seg)
#endif
#endif

        void NTAPI thread_exit_func_callback(HINSTANCE h, DWORD dwReason, PVOID pv)
        {
            if((dwReason==DLL_THREAD_DETACH) || (dwReason==DLL_PROCESS_DETACH))
            {
                run_thread_exit_callbacks();
            }
        }
    }

    namespace detail
    {
        void add_thread_exit_function(thread_exit_function_base* func)
        {
            thread_exit_callback_node* const new_node=
                heap_new<thread_exit_callback_node>(func,
                                                    get_current_thread_data()->thread_exit_callbacks);
            get_current_thread_data()->thread_exit_callbacks=new_node;
        }
    }
}

