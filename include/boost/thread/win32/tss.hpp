#ifndef BOOST_THREAD_WIN32_TSS_HPP
#define BOOST_THREAD_WIN32_TSS_HPP

namespace boost
{
    namespace detail
    {
        typedef void(*tss_cleanup_function)(void const* key,void* value);
        
        BOOST_THREAD_DECL void set_tss_data(void const* key,tss_cleanup_function func,void* tss_data,bool cleanup_existing);
        BOOST_THREAD_DECL void* get_tss_data(void const* key);
    }

    template <typename T>
    class thread_specific_ptr
    {
    private:
        thread_specific_ptr(thread_specific_ptr&);
        thread_specific_ptr& operator=(thread_specific_ptr&);

        static void delete_data(void const* self,void* value)
        {
            static_cast<thread_specific_ptr const*>(self)->cleanup((T*)value);
        }
        
        void cleanup(T* data) const
        {
            if(func)
            {
                func(data);
            }
            else
            {
                delete data;
            }
        }

        void (*func)(T*);
        
    public:
        thread_specific_ptr():
            func(0)
        {}
        explicit thread_specific_ptr(void (*func_)(T*)):
            func(func_)
        {}
        ~thread_specific_ptr()
        {
            reset();
        }

        T* get() const
        {
            return static_cast<T*>(detail::get_tss_data(this));
        }
        T* operator->() const
        {
            return get();
        }
        T& operator*() const
        {
            return *get();
        }
        T* release()
        {
            T* const temp=get();
            detail::set_tss_data(this,0,0,false);
            return temp;
        }
        void reset(T* new_value=0)
        {
            T* const current_value=get();
            if(current_value!=new_value)
            {
                detail::set_tss_data(this,delete_data,new_value,true);
            }
        }
    };
}


#endif
