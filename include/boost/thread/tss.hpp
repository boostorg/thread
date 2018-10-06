#ifndef BOOST_THREAD_TSS_HPP
#define BOOST_THREAD_TSS_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-8 Anthony Williams

#include <boost/thread/detail/config.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/thread/detail/thread_heap_alloc.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
    namespace detail
    {
        BOOST_THREAD_DECL void update_tss_data(void const* key,boost::shared_ptr<void> data);
        BOOST_THREAD_DECL void remove_tss_data(void const* key);
        BOOST_THREAD_DECL boost::shared_ptr<void> release_tss_data(void const* key);
        BOOST_THREAD_DECL void* get_tss_data(void const* key);
    }

    template <typename T>
    class thread_specific_ptr
    {
    private:
        thread_specific_ptr(thread_specific_ptr&);
        thread_specific_ptr& operator=(thread_specific_ptr&);

        struct tss_cleanup_functor
        {
            tss_cleanup_functor():
                func_(default_deleter)
            {}

            explicit tss_cleanup_functor(void (*func)(T*)):
                func_(func)
            {}

            void operator()(T* data) const
            {
                if(func_)
                {
                    (*func_)(data);
                }
            }

            void clear()
            {
                func_ = NULL;
            }

        private:
            static void default_deleter(T* data)
            {
                delete data;
            }

            void (*func_)(T*);
        } cleanup;

    public:
        typedef T element_type;

        thread_specific_ptr():
            cleanup()
        {}
        explicit thread_specific_ptr(void (*func_)(T*)):
            cleanup(func_)
        {
        }
        ~thread_specific_ptr()
        {
            detail::remove_tss_data(this);
        }

        T* get() const
        {
            return static_cast<T*>(detail::get_tss_data(this));
        }
        T* operator->() const
        {
            return get();
        }
        typename boost::detail::sp_dereference< T >::type operator*() const
        {
            return *get();
        }
        T* release()
        {
            boost::shared_ptr<void> tmp=detail::release_tss_data(this);
            if(tmp)
            {
              boost::get_deleter<tss_cleanup_functor>(tmp)->clear();
              return static_cast<T*>(tmp.get());
            }
            return NULL;
        }
        void reset(T* new_value=NULL)
        {
            T* const current_value=get();
            if(current_value!=new_value)
            {
                detail::update_tss_data(this,boost::shared_ptr<void>(new_value,cleanup));
            }
        }
    };
}

#include <boost/config/abi_suffix.hpp>

#endif
