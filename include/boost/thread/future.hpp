//  (C) Copyright 2008-10 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See
//  accompanying file LICENSE_1_0.txt or copy at
//  http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_THREAD_FUTURE_HPP
#define BOOST_THREAD_FUTURE_HPP

#include <boost/thread/detail/config.hpp>
#include <boost/thread/detail/scoped_enum.hpp>
#include <stdexcept>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/type_traits/is_fundamental.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/remove_reference.hpp>
#include <boost/mpl/if.hpp>
#include <boost/config.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <boost/scoped_array.hpp>
#include <boost/utility/enable_if.hpp>
#include <list>
#include <boost/next_prior.hpp>
#include <vector>
#include <boost/system/error_code.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#include <boost/thread/detail/memory.hpp>
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_FUTURE future
#else
#define BOOST_THREAD_FUTURE unique_future
#endif

namespace boost
{

  //enum class future_errc
  BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_errc)
  {
      broken_promise,
      future_already_retrieved,
      promise_already_satisfied,
      no_state
  }
  BOOST_SCOPED_ENUM_DECLARE_END(future_errc)

  namespace system
  {
    template <>
    struct BOOST_SYMBOL_VISIBLE is_error_code_enum<future_errc> : public true_type {};

    #ifdef BOOST_NO_SCOPED_ENUMS
    template <>
    struct BOOST_SYMBOL_VISIBLE is_error_code_enum<future_errc::enum_type> : public true_type { };
    #endif
  }

  //enum class launch
  BOOST_SCOPED_ENUM_DECLARE_BEGIN(launch)
  {
      async = 1,
      deferred = 2,
      any = async | deferred
  }
  BOOST_SCOPED_ENUM_DECLARE_END(launch)

  //enum class future_status
  BOOST_SCOPED_ENUM_DECLARE_BEGIN(future_status)
  {
      ready,
      timeout,
      deferred
  }
  BOOST_SCOPED_ENUM_DECLARE_END(future_status)

  BOOST_THREAD_DECL
  const system::error_category& future_category();

  namespace system
  {
    inline BOOST_THREAD_DECL
    error_code
    make_error_code(future_errc e)
    {
        return error_code(underlying_cast<int>(e), boost::future_category());
    }

    inline BOOST_THREAD_DECL
    error_condition
    make_error_condition(future_errc e)
    {
        return error_condition(underlying_cast<int>(e), future_category());
    }
  }

  class BOOST_SYMBOL_VISIBLE future_error
      : public std::logic_error
  {
      system::error_code ec_;
  public:
      future_error(system::error_code ec)
      : logic_error(ec.message()),
        ec_(ec)
      {
      }

      const system::error_code& code() const BOOST_NOEXCEPT
      {
        return ec_;
      }

      //virtual ~future_error() BOOST_NOEXCEPT;
  };

    class BOOST_SYMBOL_VISIBLE future_uninitialized:
        public future_error
    {
    public:
        future_uninitialized():
          future_error(system::make_error_code(future_errc::no_state))
        {}
    };
    class BOOST_SYMBOL_VISIBLE broken_promise:
        public future_error
    {
    public:
        broken_promise():
          future_error(system::make_error_code(future_errc::broken_promise))
        {}
    };
    class BOOST_SYMBOL_VISIBLE future_already_retrieved:
        public future_error
    {
    public:
        future_already_retrieved():
          future_error(system::make_error_code(future_errc::future_already_retrieved))
        {}
    };
    class BOOST_SYMBOL_VISIBLE promise_already_satisfied:
        public future_error
    {
    public:
        promise_already_satisfied():
          future_error(system::make_error_code(future_errc::promise_already_satisfied))
        {}
    };

    class BOOST_SYMBOL_VISIBLE task_already_started:
        public future_error
    {
    public:
        task_already_started():
        future_error(system::make_error_code(future_errc::promise_already_satisfied))
            //std::logic_error("Task already started")
        {}
    };

        class BOOST_SYMBOL_VISIBLE task_moved:
            public future_error
        {
        public:
            task_moved():
              future_error(system::make_error_code(future_errc::no_state))
                //std::logic_error("Task moved")
            {}
        };

            class promise_moved:
                public future_error
            {
            public:
                  promise_moved():
                  future_error(system::make_error_code(future_errc::no_state))
                    //std::logic_error("Promise moved")
                {}
            };

    namespace future_state
    {
        enum state { uninitialized, waiting, ready, moved };
    }

    namespace detail
    {
        struct future_object_base
        {
            boost::exception_ptr exception;
            bool done;
            bool thread_was_interrupted;
            boost::mutex mutex;
            boost::condition_variable waiters;
            typedef std::list<boost::condition_variable_any*> waiter_list;
            waiter_list external_waiters;
            boost::function<void()> callback;

            future_object_base():
                done(false),
                thread_was_interrupted(false)
            {}
            virtual ~future_object_base()
            {}

            waiter_list::iterator register_external_waiter(boost::condition_variable_any& cv)
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                do_callback(lock);
                return external_waiters.insert(external_waiters.end(),&cv);
            }

            void remove_external_waiter(waiter_list::iterator it)
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                external_waiters.erase(it);
            }

            void mark_finished_internal()
            {
                done=true;
                waiters.notify_all();
                for(waiter_list::const_iterator it=external_waiters.begin(),
                        end=external_waiters.end();it!=end;++it)
                {
                    (*it)->notify_all();
                }
            }

            struct relocker
            {
                boost::unique_lock<boost::mutex>& lock;

                relocker(boost::unique_lock<boost::mutex>& lock_):
                    lock(lock_)
                {
                    lock.unlock();
                }
                ~relocker()
                {
                    lock.lock();
                }
            private:
                relocker& operator=(relocker const&);
            };

            void do_callback(boost::unique_lock<boost::mutex>& lock)
            {
                if(callback && !done)
                {
                    boost::function<void()> local_callback=callback;
                    relocker relock(lock);
                    local_callback();
                }
            }


            void wait(bool rethrow=true)
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                do_callback(lock);
                while(!done)
                {
                    waiters.wait(lock);
                }
                if(rethrow && thread_was_interrupted)
                {
                    throw boost::thread_interrupted();
                }
                if(rethrow && exception)
                {
                    boost::rethrow_exception(exception);
                }
            }

            bool timed_wait_until(boost::system_time const& target_time)
            {
                boost::unique_lock<boost::mutex> lock(mutex);
                do_callback(lock);
                while(!done)
                {
                    bool const success=waiters.timed_wait(lock,target_time);
                    if(!success && !done)
                    {
                        return false;
                    }
                }
                return true;
            }

#ifdef BOOST_THREAD_USES_CHRONO

            template <class Clock, class Duration>
            future_status
            wait_until(const chrono::time_point<Clock, Duration>& abs_time)
            {
              boost::unique_lock<boost::mutex> lock(mutex);
              do_callback(lock);
              while(!done)
              {
                  cv_status const st=waiters.wait_until(lock,abs_time);
                  if(st==cv_status::timeout && !done)
                  {
                    return future_status::timeout;
                  }
              }
              return future_status::ready;
            }
#endif
            void mark_exceptional_finish_internal(boost::exception_ptr const& e)
            {
                exception=e;
                mark_finished_internal();
            }
            void mark_exceptional_finish()
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                mark_exceptional_finish_internal(boost::current_exception());
            }
            void mark_interrupted_finish()
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                thread_was_interrupted=true;
                mark_finished_internal();
            }
            bool has_value()
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                return done && !(exception || thread_was_interrupted);
            }
            bool has_exception()
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                return done && (exception || thread_was_interrupted);
            }

            template<typename F,typename U>
            void set_wait_callback(F f,U* u)
            {
                callback=boost::bind(f,boost::ref(*u));
            }

        private:
            future_object_base(future_object_base const&);
            future_object_base& operator=(future_object_base const&);
        };

        template<typename T>
        struct future_traits
        {
            typedef boost::scoped_ptr<T> storage_type;
#ifndef BOOST_NO_RVALUE_REFERENCES
            typedef T const& source_reference_type;
            struct dummy;
            typedef typename boost::mpl::if_<boost::is_fundamental<T>,dummy&,T&&>::type rvalue_source_type;
            typedef typename boost::mpl::if_<boost::is_fundamental<T>,T,T&&>::type move_dest_type;
#else
#if defined BOOST_THREAD_USES_MOVE
            typedef T& source_reference_type;
            typedef typename boost::mpl::if_<boost::has_move_emulation_enabled<T>,::boost::rv<T>&,T const&>::type rvalue_source_type;
            typedef typename boost::mpl::if_<boost::has_move_emulation_enabled<T>,::boost::rv<T>&,T>::type move_dest_type;
#else
            typedef T& source_reference_type;
            typedef typename boost::mpl::if_<boost::is_convertible<T&,boost::detail::thread_move_t<T> >,boost::detail::thread_move_t<T>,T const&>::type rvalue_source_type;
            typedef typename boost::mpl::if_<boost::is_convertible<T&,boost::detail::thread_move_t<T> >,boost::detail::thread_move_t<T>,T>::type move_dest_type;
#endif
#endif
            typedef const T& shared_future_get_result_type;

            static void init(storage_type& storage,source_reference_type t)
            {
                storage.reset(new T(t));
            }

            static void init(storage_type& storage,rvalue_source_type t)
            {
              storage.reset(new T(static_cast<rvalue_source_type>(t)));
            }

            static void cleanup(storage_type& storage)
            {
                storage.reset();
            }
        };

        template<typename T>
        struct future_traits<T&>
        {
            typedef T* storage_type;
            typedef T& source_reference_type;
            struct rvalue_source_type
            {};
            typedef T& move_dest_type;
            typedef T& shared_future_get_result_type;

            static void init(storage_type& storage,T& t)
            {
                storage=&t;
            }

            static void cleanup(storage_type& storage)
            {
                storage=0;
            }
        };

        template<>
        struct future_traits<void>
        {
            typedef bool storage_type;
            typedef void move_dest_type;
            typedef void shared_future_get_result_type;

            static void init(storage_type& storage)
            {
                storage=true;
            }

            static void cleanup(storage_type& storage)
            {
                storage=false;
            }

        };

        template<typename T>
        struct future_object:
            detail::future_object_base
        {
            typedef typename future_traits<T>::storage_type storage_type;
            typedef typename future_traits<T>::source_reference_type source_reference_type;
            typedef typename future_traits<T>::rvalue_source_type rvalue_source_type;
            typedef typename future_traits<T>::move_dest_type move_dest_type;
            typedef typename future_traits<T>::shared_future_get_result_type shared_future_get_result_type;

            storage_type result;

            future_object():
                result(0)
            {}

            void mark_finished_with_result_internal(source_reference_type result_)
            {
                future_traits<T>::init(result,result_);
                mark_finished_internal();
            }
            void mark_finished_with_result_internal(rvalue_source_type result_)
            {
                future_traits<T>::init(result,static_cast<rvalue_source_type>(result_));
                mark_finished_internal();
            }

            void mark_finished_with_result(source_reference_type result_)
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                mark_finished_with_result_internal(result_);
            }
            void mark_finished_with_result(rvalue_source_type result_)
            {
                boost::lock_guard<boost::mutex> lock(mutex);
#if defined BOOST_THREAD_USES_MOVEXX
                mark_finished_with_result_internal(static_cast<rvalue_source_type>(result_));
#else
                mark_finished_with_result_internal(result_);
#endif
            }

#if defined BOOST_THREAD_USES_MOVEXX
            T& get()
            {
                wait();
                return *result;             }
#else
            move_dest_type get()
            {
                wait();
                return static_cast<move_dest_type>(*result);
            }
#endif

            shared_future_get_result_type get_sh()
            {
                wait();
                return static_cast<shared_future_get_result_type>(*result);
            }

            future_state::state get_state() // DEPRECATED V2
            {
                boost::lock_guard<boost::mutex> guard(mutex);
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }

        private:
            future_object(future_object const&);
            future_object& operator=(future_object const&);
        };

        template<>
        struct future_object<void>:
            detail::future_object_base
        {
          typedef void shared_future_get_result_type;

            future_object()
            {}

            void mark_finished_with_result_internal()
            {
                mark_finished_internal();
            }

            void mark_finished_with_result()
            {
                boost::lock_guard<boost::mutex> lock(mutex);
                mark_finished_with_result_internal();
            }

            void get()
            {
                wait();
            }
            void get_sh()
            {
                wait();
            }
            future_state::state get_state() // DEPRECATED V2
            {
                boost::lock_guard<boost::mutex> guard(mutex);
                if(!done)
                {
                    return future_state::waiting;
                }
                else
                {
                    return future_state::ready;
                }
            }

        private:
            future_object(future_object const&);
            future_object& operator=(future_object const&);
        };

//        template<typename T, typename Allocator>
//        struct future_object_alloc: public future_object<T>
//        {
//          typedef future_object<T> base;
//          Allocator alloc_;
//
//        public:
//          explicit future_object_alloc(const Allocator& a)
//              : alloc_(a) {}
//
//        };
        class future_waiter
        {
            struct registered_waiter;
            typedef std::vector<registered_waiter>::size_type count_type;

            struct registered_waiter
            {
                boost::shared_ptr<detail::future_object_base> future_;
                detail::future_object_base::waiter_list::iterator wait_iterator;
                count_type index;

                registered_waiter(boost::shared_ptr<detail::future_object_base> const& a_future,
                                  detail::future_object_base::waiter_list::iterator wait_iterator_,
                                  count_type index_):
                    future_(a_future),wait_iterator(wait_iterator_),index(index_)
                {}

            };

            struct all_futures_lock
            {
                count_type count;
                boost::scoped_array<boost::unique_lock<boost::mutex> > locks;

                all_futures_lock(std::vector<registered_waiter>& futures):
                    count(futures.size()),locks(new boost::unique_lock<boost::mutex>[count])
                {
                    for(count_type i=0;i<count;++i)
                    {
#if defined __DECCXX || defined __SUNPRO_CC
                        locks[i]=boost::unique_lock<boost::mutex>(futures[i].future_->mutex).move();
#else
                        locks[i]=boost::unique_lock<boost::mutex>(futures[i].future_->mutex);
#endif
                    }
                }

                void lock()
                {
                    boost::lock(locks.get(),locks.get()+count);
                }

                void unlock()
                {
                    for(count_type i=0;i<count;++i)
                    {
                        locks[i].unlock();
                    }
                }
            };

            boost::condition_variable_any cv;
            std::vector<registered_waiter> futures;
            count_type future_count;

        public:
            future_waiter():
                future_count(0)
            {}

            template<typename F>
            void add(F& f)
            {
                if(f.future_)
                {
                    futures.push_back(registered_waiter(f.future_,f.future_->register_external_waiter(cv),future_count));
                }
                ++future_count;
            }

            count_type wait()
            {
                all_futures_lock lk(futures);
                for(;;)
                {
                    for(count_type i=0;i<futures.size();++i)
                    {
                        if(futures[i].future_->done)
                        {
                            return futures[i].index;
                        }
                    }
                    cv.wait(lk);
                }
            }

            ~future_waiter()
            {
                for(count_type i=0;i<futures.size();++i)
                {
                    futures[i].future_->remove_external_waiter(futures[i].wait_iterator);
                }
            }

        };

    }

    template <typename R>
    class BOOST_THREAD_FUTURE;

    template <typename R>
    class shared_future;

    template<typename T>
    struct is_future_type
    {
        BOOST_STATIC_CONSTANT(bool, value=false);
    };

    template<typename T>
    struct is_future_type<BOOST_THREAD_FUTURE<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value=true);
    };

    template<typename T>
    struct is_future_type<shared_future<T> >
    {
        BOOST_STATIC_CONSTANT(bool, value=true);
    };

    template<typename Iterator>
    typename boost::disable_if<is_future_type<Iterator>,void>::type wait_for_all(Iterator begin,Iterator end)
    {
        for(Iterator current=begin;current!=end;++current)
        {
            current->wait();
        }
    }

    template<typename F1,typename F2>
    typename boost::enable_if<is_future_type<F1>,void>::type wait_for_all(F1& f1,F2& f2)
    {
        f1.wait();
        f2.wait();
    }

    template<typename F1,typename F2,typename F3>
    void wait_for_all(F1& f1,F2& f2,F3& f3)
    {
        f1.wait();
        f2.wait();
        f3.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4>
    void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4)
    {
        f1.wait();
        f2.wait();
        f3.wait();
        f4.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4,typename F5>
    void wait_for_all(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
    {
        f1.wait();
        f2.wait();
        f3.wait();
        f4.wait();
        f5.wait();
    }

    template<typename Iterator>
    typename boost::disable_if<is_future_type<Iterator>,Iterator>::type wait_for_any(Iterator begin,Iterator end)
    {
        if(begin==end)
            return end;

        detail::future_waiter waiter;
        for(Iterator current=begin;current!=end;++current)
        {
            waiter.add(*current);
        }
        return boost::next(begin,waiter.wait());
    }

    template<typename F1,typename F2>
    typename boost::enable_if<is_future_type<F1>,unsigned>::type wait_for_any(F1& f1,F2& f2)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3>
    unsigned wait_for_any(F1& f1,F2& f2,F3& f3)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4>
    unsigned wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        waiter.add(f4);
        return waiter.wait();
    }

    template<typename F1,typename F2,typename F3,typename F4,typename F5>
    unsigned wait_for_any(F1& f1,F2& f2,F3& f3,F4& f4,F5& f5)
    {
        detail::future_waiter waiter;
        waiter.add(f1);
        waiter.add(f2);
        waiter.add(f3);
        waiter.add(f4);
        waiter.add(f5);
        return waiter.wait();
    }

    template <typename R>
    class promise;

    template <typename R>
    class packaged_task;

    template <typename R>
    class BOOST_THREAD_FUTURE
    {

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
      BOOST_THREAD_FUTURE(BOOST_THREAD_FUTURE const& rhs) = delete;
      BOOST_THREAD_FUTURE& operator=(BOOST_THREAD_FUTURE const& rhs) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
      BOOST_THREAD_FUTURE(BOOST_THREAD_FUTURE & rhs);// = delete;
      BOOST_THREAD_FUTURE& operator=(BOOST_THREAD_FUTURE& rhs);// = delete;
#endif // BOOST_NO_DELETED_FUNCTIONS

    private:

        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

        future_ptr future_;

        friend class shared_future<R>;
        friend class promise<R>;
        friend class packaged_task<R>;
        friend class detail::future_waiter;

        typedef typename detail::future_traits<R>::move_dest_type move_dest_type;

        BOOST_THREAD_FUTURE(future_ptr a_future):
            future_(a_future)
        {}

    public:
        typedef future_state::state state;

        BOOST_THREAD_FUTURE()
        {}

        ~BOOST_THREAD_FUTURE()
        {}

#ifndef BOOST_NO_RVALUE_REFERENCES
        BOOST_THREAD_FUTURE(BOOST_THREAD_FUTURE && other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
        }
        BOOST_THREAD_FUTURE& operator=(BOOST_THREAD_FUTURE && other) BOOST_NOEXCEPT
        {
            future_=other.future_;
            other.future_.reset();
            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        BOOST_THREAD_FUTURE(boost::rv<BOOST_THREAD_FUTURE>& other) BOOST_NOEXCEPT:
            future_(other.future_)
        {
            other.future_.reset();
        }

        BOOST_THREAD_FUTURE& operator=(boost::rv<BOOST_THREAD_FUTURE>& other) BOOST_NOEXCEPT
        {
            future_=other.future_;
            other.future_.reset();
            return *this;
        }
        operator ::boost::rv<BOOST_THREAD_FUTURE>&()
        {
          return *static_cast< ::boost::rv<BOOST_THREAD_FUTURE>* >(this);
        }
        operator const ::boost::rv<BOOST_THREAD_FUTURE>&() const
        {
          return *static_cast<const ::boost::rv<BOOST_THREAD_FUTURE>* >(this);
        }
        ::boost::rv<BOOST_THREAD_FUTURE>& move()
        {
          return *static_cast< ::boost::rv<BOOST_THREAD_FUTURE>* >(this);
        }
        const ::boost::rv<BOOST_THREAD_FUTURE>& move() const
        {
          return *static_cast<const ::boost::rv<BOOST_THREAD_FUTURE>* >(this);
        }
#else
        BOOST_THREAD_FUTURE(boost::detail::thread_move_t<BOOST_THREAD_FUTURE> other) BOOST_NOEXCEPT:
            future_(other->future_)
        {
            other->future_.reset();
        }

        BOOST_THREAD_FUTURE& operator=(boost::detail::thread_move_t<BOOST_THREAD_FUTURE> other) BOOST_NOEXCEPT
        {
            future_=other->future_;
            other->future_.reset();
            return *this;
        }

        operator boost::detail::thread_move_t<BOOST_THREAD_FUTURE>()
        {
            return boost::detail::thread_move_t<BOOST_THREAD_FUTURE>(*this);
        }
        boost::detail::thread_move_t<BOOST_THREAD_FUTURE> move()
        {
            return boost::detail::thread_move_t<BOOST_THREAD_FUTURE>(*this);
        }
#endif
#endif
        shared_future<R> share()
        {
          return shared_future<R>(::boost::move(*this));
        }

        void swap(BOOST_THREAD_FUTURE& other)
        {
            future_.swap(other.future_);
        }

        // retrieving the value
        move_dest_type get()
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }

            return future_->get();
        }

        // functions to check state, and wait for ready
        state get_state() const BOOST_NOEXCEPT // DEPRECATED V2
        {
            if(!future_)
            {
                return future_state::uninitialized;
            }
            return future_->get_state();
        }


        bool is_ready() const BOOST_NOEXCEPT
        {
            return get_state()==future_state::ready;
        }

        bool has_exception() const BOOST_NOEXCEPT
        {
            return future_ && future_->has_exception();
        }

        bool has_value() const BOOST_NOEXCEPT
        {
            return future_ && future_->has_value();
        }

        bool valid() const BOOST_NOEXCEPT
        {
            return future_ != 0;
        }


        void wait() const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            future_->wait(false);
        }

        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(boost::get_system_time()+rel_time);
        }

        bool timed_wait_until(boost::system_time const& abs_time) const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future_->timed_wait_until(abs_time);
        }
#ifdef BOOST_THREAD_USES_CHRONO
        template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const
        {
          return wait_until(chrono::steady_clock::now() + rel_time);

        }
        template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const
        {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->wait_until(abs_time);
        }
#endif
    };

#ifdef BOOST_NO_RVALUE_REFERENCES
    template <typename T>
    struct has_move_emulation_enabled_aux<BOOST_THREAD_FUTURE<T> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif

    template <typename R>
    class shared_future
    {
        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

        future_ptr future_;

//         shared_future(const BOOST_THREAD_FUTURE<R>& other);
//         shared_future& operator=(const BOOST_THREAD_FUTURE<R>& other);

        friend class detail::future_waiter;
        friend class promise<R>;
        friend class packaged_task<R>;

        shared_future(future_ptr a_future):
            future_(a_future)
        {}

    public:
        shared_future(shared_future const& other):
            future_(other.future_)
        {}

        typedef future_state::state state;

        shared_future()
        {}

        ~shared_future()
        {}

        shared_future& operator=(shared_future const& other)
        {
            future_=other.future_;
            return *this;
        }
#ifndef BOOST_NO_RVALUE_REFERENCES
        shared_future(shared_future && other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
        }
        shared_future(BOOST_THREAD_FUTURE<R> && other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
        }
        shared_future& operator=(shared_future && other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
            other.future_.reset();
            return *this;
        }
        shared_future& operator=(BOOST_THREAD_FUTURE<R> && other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
            other.future_.reset();
            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        shared_future(boost::rv<shared_future>& other) BOOST_NOEXCEPT :
            future_(other.future_)
        {
            other.future_.reset();
        }
//         shared_future(const BOOST_THREAD_FUTURE<R> &) = delete;
        shared_future(boost::rv<BOOST_THREAD_FUTURE<R> >& other) BOOST_NOEXCEPT :
            future_(other.future_)
        {
            other.future_.reset();
        }
        shared_future& operator=(boost::rv<shared_future>& other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
            other.future_.reset();
            return *this;
        }
        shared_future& operator=(boost::rv<BOOST_THREAD_FUTURE<R> >& other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
            other.future_.reset();
            return *this;
        }
        operator ::boost::rv<shared_future>&()
        {
          return *static_cast< ::boost::rv<shared_future>* >(this);
        }
        operator const ::boost::rv<shared_future>&() const
        {
          return *static_cast<const ::boost::rv<shared_future>* >(this);
        }
        ::boost::rv<shared_future>& move()
        {
          return *static_cast< ::boost::rv<shared_future>* >(this);
        }
        const ::boost::rv<shared_future>& move() const
        {
          return *static_cast<const ::boost::rv<shared_future>* >(this);
        }

#else

        shared_future(boost::detail::thread_move_t<shared_future> other) BOOST_NOEXCEPT :
            future_(other->future_)
        {
            other->future_.reset();
        }
//         shared_future(const BOOST_THREAD_FUTURE<R> &) = delete;
        shared_future(boost::detail::thread_move_t<BOOST_THREAD_FUTURE<R> > other) BOOST_NOEXCEPT :
            future_(other->future_)
        {
            other->future_.reset();
        }
        shared_future& operator=(boost::detail::thread_move_t<shared_future> other) BOOST_NOEXCEPT
        {
            future_.swap(other->future_);
            other->future_.reset();
            return *this;
        }
        shared_future& operator=(boost::detail::thread_move_t<BOOST_THREAD_FUTURE<R> > other) BOOST_NOEXCEPT
        {
            future_.swap(other->future_);
            other->future_.reset();
            return *this;
        }

        operator boost::detail::thread_move_t<shared_future>()
        {
            return boost::detail::thread_move_t<shared_future>(*this);
        }
        boost::detail::thread_move_t<shared_future> move()
        {
            return boost::detail::thread_move_t<shared_future>(*this);
        }
#endif
#endif

        void swap(shared_future& other) BOOST_NOEXCEPT
        {
            future_.swap(other.future_);
        }

        // retrieving the value
        typename detail::future_object<R>::shared_future_get_result_type get()
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }

            return future_->get_sh();
        }

        // functions to check state, and wait for ready
        state get_state() const  BOOST_NOEXCEPT // DEPRECATED V2
        {
            if(!future_)
            {
                return future_state::uninitialized;
            }
            return future_->get_state();
        }

        bool valid() const  BOOST_NOEXCEPT
        {
            return future_ != 0;
        }

        bool is_ready() const  BOOST_NOEXCEPT
        {
            return get_state()==future_state::ready;
        }

        bool has_exception() const BOOST_NOEXCEPT
        {
            return future_ && future_->has_exception();
        }

        bool has_value() const BOOST_NOEXCEPT
        {
            return future_ && future_->has_value();
        }

        void wait() const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            future_->wait(false);
        }

        template<typename Duration>
        bool timed_wait(Duration const& rel_time) const
        {
            return timed_wait_until(boost::get_system_time()+rel_time);
        }

        bool timed_wait_until(boost::system_time const& abs_time) const
        {
            if(!future_)
            {
                boost::throw_exception(future_uninitialized());
            }
            return future_->timed_wait_until(abs_time);
        }
#ifdef BOOST_THREAD_USES_CHRONO

        template <class Rep, class Period>
        future_status
        wait_for(const chrono::duration<Rep, Period>& rel_time) const
        {
          return wait_until(chrono::steady_clock::now() + rel_time);

        }
        template <class Clock, class Duration>
        future_status
        wait_until(const chrono::time_point<Clock, Duration>& abs_time) const
        {
          if(!future_)
          {
              boost::throw_exception(future_uninitialized());
          }
          return future_->wait_until(abs_time);
        }
#endif
    };

#ifdef BOOST_NO_RVALUE_REFERENCES
    template <typename T>
    struct has_move_emulation_enabled_aux<shared_future<T> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif

    template <typename R>
    class promise
    {
        typedef boost::shared_ptr<detail::future_object<R> > future_ptr;

        future_ptr future_;
        bool future_obtained;

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        promise(promise const&  rhs) = delete;
        promise & operator=(promise const & rhs) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        promise(promise & rhs);// = delete;
        promise & operator=(promise & rhs);// = delete;
#endif // BOOST_NO_DELETED_FUNCTIONS
    private:

        void lazy_init()
        {
#if defined BOOST_THREAD_PROMISE_LAZY
            if(!atomic_load(&future_))
            {
                future_ptr blank;
                atomic_compare_exchange(&future_,&blank,future_ptr(new detail::future_object<R>));
            }
#endif
        }

    public:
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
        template <class Allocator>
        promise(boost::allocator_arg_t, Allocator a)
        {
          typedef typename Allocator::template rebind<detail::future_object<R> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          future_ = future_ptr(::new(a2.allocate(1)) detail::future_object<R>(), D(a2, 1) );
          future_obtained = false;
        }
#endif
        promise():
#if defined BOOST_THREAD_PROMISE_LAZY
            future_(),
#else
            future_(new detail::future_object<R>()),
#endif
            future_obtained(false)
        {}

        ~promise()
        {
            if(future_)
            {
                boost::lock_guard<boost::mutex> lock(future_->mutex);

                if(!future_->done)
                {
                    future_->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()));
                }
            }
        }

        // Assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        promise(promise && rhs) BOOST_NOEXCEPT:
            future_obtained(rhs.future_obtained)
        {
            future_.swap(rhs.future_);
            rhs.future_.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(promise&& rhs) BOOST_NOEXCEPT
        {
#if defined BOOST_THREAD_PROMISE_LAZY
            future_.swap(rhs.future_);
            future_obtained=rhs.future_obtained;
            rhs.future_.reset();
            rhs.future_obtained=false;
#else
            promise(boost::move(rhs)).swap(*this);
#endif

            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        promise(boost::rv<promise>& rhs) BOOST_NOEXCEPT :
            future_(rhs.future_),future_obtained(rhs.future_obtained)
        {
            rhs.future_.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(boost::rv<promise>& rhs) BOOST_NOEXCEPT
        {
            future_=rhs.future_;
            future_obtained=rhs.future_obtained;
            rhs.future_.reset();
            rhs.future_obtained=false;
            return *this;
        }
        operator ::boost::rv<promise>&()
        {
          return *static_cast< ::boost::rv<promise>* >(this);
        }
        operator const ::boost::rv<promise>&() const
        {
          return *static_cast<const ::boost::rv<promise>* >(this);
        }
        ::boost::rv<promise>& move()
        {
          return *static_cast< ::boost::rv<promise>* >(this);
        }
        const ::boost::rv<promise>& move() const
        {
          return *static_cast<const ::boost::rv<promise>* >(this);
        }
#else
        promise(boost::detail::thread_move_t<promise> rhs) BOOST_NOEXCEPT:
            future_(rhs->future_),future_obtained(rhs->future_obtained)
        {
            rhs->future_.reset();
            rhs->future_obtained=false;
        }
        promise & operator=(boost::detail::thread_move_t<promise> rhs) BOOST_NOEXCEPT
        {
            future_=rhs->future_;
            future_obtained=rhs->future_obtained;
            rhs->future_.reset();
            rhs->future_obtained=false;
            return *this;
        }

        operator boost::detail::thread_move_t<promise>()
        {
            return boost::detail::thread_move_t<promise>(*this);
        }
#endif
#endif

        void swap(promise& other)
        {
            future_.swap(other.future_);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        BOOST_THREAD_FUTURE<R> get_future()
        {
            lazy_init();
            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            if (future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return BOOST_THREAD_FUTURE<R>(future_);
        }

        void set_value(typename detail::future_traits<R>::source_reference_type r)
        {
            lazy_init();
            boost::lock_guard<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(r);
        }

//         void set_value(R && r);
        void set_value(typename detail::future_traits<R>::rvalue_source_type r)
        {
            lazy_init();
            boost::lock_guard<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal(static_cast<typename detail::future_traits<R>::rvalue_source_type>(r));
        }

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::lock_guard<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_exceptional_finish_internal(p);
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future_->set_wait_callback(f,this);
        }

    };

    template <>
    class promise<void>
    {
        typedef boost::shared_ptr<detail::future_object<void> > future_ptr;

        future_ptr future_;
        bool future_obtained;

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        promise(promise const&  rhs) = delete;
        promise & operator=(promise const & rhs) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        promise(promise & rhs);// = delete;
        promise & operator=(promise & rhs);// = delete;
#endif // BOOST_NO_DELETED_FUNCTIONS
    private:

        void lazy_init()
        {
#if defined BOOST_THREAD_PROMISE_LAZY
            if(!atomic_load(&future_))
            {
                future_ptr blank;
                atomic_compare_exchange(&future_,&blank,future_ptr(new detail::future_object<void>));
            }
#endif
        }
    public:
#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
        template <class Allocator>
        promise(boost::allocator_arg_t, Allocator a)
        {
          typedef typename Allocator::template rebind<detail::future_object<void> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          future_ = future_ptr(::new(a2.allocate(1)) detail::future_object<void>(), D(a2, 1) );
          future_obtained = false;
        }
#endif
        promise():
#if defined BOOST_THREAD_PROMISE_LAZY
            future_(),
#else
            future_(new detail::future_object<void>),
#endif
            future_obtained(false)
        {}

        ~promise()
        {
            if(future_)
            {
                boost::lock_guard<boost::mutex> lock(future_->mutex);

                if(!future_->done)
                {
                    future_->mark_exceptional_finish_internal(boost::copy_exception(broken_promise()));
                }
            }
        }

        // Assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        promise(promise && rhs) BOOST_NOEXCEPT :
            future_obtained(rhs.future_obtained)
        {
            future_.swap(rhs.future_);
            // we need to release the future as shared_ptr doesn't implements move semantics
            rhs.future_.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(promise&& rhs) BOOST_NOEXCEPT
        {
            future_.swap(rhs.future_);
            future_obtained=rhs.future_obtained;
            rhs.future_.reset();
            rhs.future_obtained=false;
            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        promise(boost::rv<promise>& rhs) BOOST_NOEXCEPT :
            future_(rhs.future_),future_obtained(rhs.future_obtained)
        {
            rhs.future_.reset();
            rhs.future_obtained=false;
        }
        promise & operator=(boost::rv<promise>& rhs) BOOST_NOEXCEPT
        {
            future_=rhs.future_;
            future_obtained=rhs.future_obtained;
            rhs.future_.reset();
            rhs.future_obtained=false;
            return *this;
        }
        operator ::boost::rv<promise>&()
        {
          return *static_cast< ::boost::rv<promise>* >(this);
        }
        operator const ::boost::rv<promise>&() const
        {
          return *static_cast<const ::boost::rv<promise>* >(this);
        }
        ::boost::rv<promise>& move()
        {
          return *static_cast< ::boost::rv<promise>* >(this);
        }
        const ::boost::rv<promise>& move() const
        {
          return *static_cast<const ::boost::rv<promise>* >(this);
        }
#else
        promise(boost::detail::thread_move_t<promise> rhs) BOOST_NOEXCEPT :
            future_(rhs->future_),future_obtained(rhs->future_obtained)
        {
            rhs->future_.reset();
            rhs->future_obtained=false;
        }
        promise & operator=(boost::detail::thread_move_t<promise> rhs) BOOST_NOEXCEPT
        {
            future_=rhs->future_;
            future_obtained=rhs->future_obtained;
            rhs->future_.reset();
            rhs->future_obtained=false;
            return *this;
        }

        operator boost::detail::thread_move_t<promise>()
        {
            return boost::detail::thread_move_t<promise>(*this);
        }
        boost::detail::thread_move_t<promise> move()
        {
            return boost::detail::thread_move_t<promise>(*this);
        }
#endif
#endif

        void swap(promise& other)
        {
            future_.swap(other.future_);
            std::swap(future_obtained,other.future_obtained);
        }

        // Result retrieval
        BOOST_THREAD_FUTURE<void> get_future()
        {
            lazy_init();

            if (future_.get()==0)
            {
                boost::throw_exception(promise_moved());
            }
            if(future_obtained)
            {
                boost::throw_exception(future_already_retrieved());
            }
            future_obtained=true;
            return BOOST_THREAD_FUTURE<void>(future_);
        }

        void set_value()
        {
            lazy_init();
            boost::lock_guard<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_finished_with_result_internal();
        }

        void set_exception(boost::exception_ptr p)
        {
            lazy_init();
            boost::lock_guard<boost::mutex> lock(future_->mutex);
            if(future_->done)
            {
                boost::throw_exception(promise_already_satisfied());
            }
            future_->mark_exceptional_finish_internal(p);
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            lazy_init();
            future_->set_wait_callback(f,this);
        }

    };

#ifdef BOOST_NO_RVALUE_REFERENCES
    template <typename T>
    struct has_move_emulation_enabled_aux<promise<T> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif

    namespace detail
    {
        template<typename R>
        struct task_base:
            detail::future_object<R>
        {
            bool started;

            task_base():
                started(false)
            {}

            void reset()
            {
              started=false;
            }
            void run()
            {
                {
                    boost::lock_guard<boost::mutex> lk(this->mutex);
                    if(started)
                    {
                        boost::throw_exception(task_already_started());
                    }
                    started=true;
                }
                do_run();
            }

            void owner_destroyed()
            {
                boost::lock_guard<boost::mutex> lk(this->mutex);
                if(!started)
                {
                    started=true;
                    this->mark_exceptional_finish_internal(boost::copy_exception(boost::broken_promise()));
                }
            }


            virtual void do_run()=0;
        };


        template<typename R,typename F>
        struct task_object:
            task_base<R>
        {
        private:
          task_object(task_object&);
        public:
            F f;
            task_object(F const& f_):
                f(f_)
            {}
#ifndef BOOST_NO_RVALUE_REFERENCES
            task_object(F&& f_):
              f(boost::forward<F>(f_))
            {}
#elif defined BOOST_THREAD_USES_MOVE
            task_object(boost::rv<F>& f_):
                f(boost::move(f_))
            {}
#else
            task_object(boost::detail::thread_move_t<F> f_):
                f(f_)
            {}
#endif
            void do_run()
            {
                try
                {
                    this->mark_finished_with_result(f());
                }
                catch(thread_interrupted& )
                {
                    this->mark_interrupted_finish();
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

        template<typename F>
        struct task_object<void,F>:
            task_base<void>
        {
        private:
          task_object(task_object&);
        public:
            F f;
            task_object(F const& f_):
                f(f_)
            {}
#ifndef BOOST_NO_RVALUE_REFERENCES
            task_object(F&& f_):
              f(boost::forward<F>(f_))
            {}
#elif defined BOOST_THREAD_USES_MOVE
            task_object(boost::rv<F>& f_):
                f(boost::forward<F>(f_))
            {}
#else
            task_object(boost::detail::thread_move_t<F> f_):
                f(f_)
            {}
#endif

            void do_run()
            {
                try
                {
                    f();
                    this->mark_finished_with_result();
                }
                catch(thread_interrupted& )
                {
                    this->mark_interrupted_finish();
                }
                catch(...)
                {
                    this->mark_exceptional_finish();
                }
            }
        };

    }

    template<typename R>
    class packaged_task
    {
        typedef boost::shared_ptr<detail::task_base<R> > task_ptr;
        boost::shared_ptr<detail::task_base<R> > task;
        bool future_obtained;

#ifndef BOOST_NO_DELETED_FUNCTIONS
    public:
        packaged_task(packaged_task const&) = delete;
        packaged_task& operator=(packaged_task const&) = delete;
#else // BOOST_NO_DELETED_FUNCTIONS
    private:
        packaged_task(packaged_task&);// = delete;
        packaged_task& operator=(packaged_task&);// = delete;
#endif // BOOST_NO_DELETED_FUNCTIONS
    public:
        typedef R result_type;
        packaged_task():
            future_obtained(false)
        {}

        // construction and destruction

        explicit packaged_task(R(*f)()):
            task(new detail::task_object<R,R(*)()>(f)),future_obtained(false)
        {}
#ifndef BOOST_NO_RVALUE_REFERENCES
        template <class F>
        explicit packaged_task(F&& f):
            task(new detail::task_object<R,typename remove_reference<F>::type>(boost::forward<F>(f))),future_obtained(false)
        {}
#else
        template <class F>
        explicit packaged_task(F const& f):
            task(new detail::task_object<R,F>(f)),future_obtained(false)
        {}
#if defined BOOST_THREAD_USES_MOVE
        template <class F>
        explicit packaged_task(boost::rv<F>& f):
            task(new detail::task_object<R,F>(boost::move(f))),future_obtained(false)
        {}
#else
        template <class F>
        explicit packaged_task(boost::detail::thread_move_t<F> f):
            task(new detail::task_object<R,F>(f)),future_obtained(false)
        {}
#endif
#endif

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
#ifndef BOOST_NO_RVALUE_REFERENCES
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, F&& f)
        {
          typedef typename remove_reference<F>::type FR;
          typedef typename Allocator::template rebind<detail::task_object<R,FR> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) detail::task_object<R,FR>(boost::forward<F>(f)), D(a2, 1) );
          future_obtained = false;
        }
#else
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, const F& f)
        {
          typedef typename Allocator::template rebind<detail::task_object<R,F> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) detail::task_object<R,F>(f), D(a2, 1) );
          std::cout << __FILE__ ":"<<__LINE__<<std::endl;
          future_obtained = false;
        }
#if defined BOOST_THREAD_USES_MOVE
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, boost::rv<F>& f)
        {
          typedef typename Allocator::template rebind<detail::task_object<R,F> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) detail::task_object<R,F>(boost::move(f)), D(a2, 1) );
          future_obtained = false;
        }
#else
        template <class F, class Allocator>
        packaged_task(boost::allocator_arg_t, Allocator a, boost::detail::thread_move_t<F> f)
        {
          typedef typename Allocator::template rebind<detail::task_object<R,F> >::other A2;
          A2 a2(a);
          typedef thread_detail::allocator_destructor<A2> D;

          task = task_ptr(::new(a2.allocate(1)) detail::task_object<R,F>(boost::forward<F>(f)), D(a2, 1) );
          future_obtained = false;
        }
#endif // BOOST_THREAD_USES_MOVE
#endif //BOOST_NO_RVALUE_REFERENCES
#endif // BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS

        ~packaged_task()
        {
            if(task)
            {
                task->owner_destroyed();
            }
        }

        // assignment
#ifndef BOOST_NO_RVALUE_REFERENCES
        packaged_task(packaged_task&& other) BOOST_NOEXCEPT :
            future_obtained(other.future_obtained)
        {
            task.swap(other.task);
            other.future_obtained=false;
        }
        packaged_task& operator=(packaged_task&& other) BOOST_NOEXCEPT
        {
            packaged_task temp(static_cast<packaged_task&&>(other));
            swap(temp);
            return *this;
        }
#else
#if defined BOOST_THREAD_USES_MOVE
        packaged_task(boost::rv<packaged_task>& other) BOOST_NOEXCEPT :
            future_obtained(other.future_obtained)
        {
            task.swap(other.task);
            other.future_obtained=false;
        }
        packaged_task& operator=(boost::rv<packaged_task>& other) BOOST_NOEXCEPT
        {
            packaged_task temp(other);
            swap(temp);
            return *this;
        }
        operator ::boost::rv<packaged_task>&()
        {
          return *static_cast< ::boost::rv<packaged_task>* >(this);
        }
        operator const ::boost::rv<packaged_task>&() const
        {
          return *static_cast<const ::boost::rv<packaged_task>* >(this);
        }
        ::boost::rv<packaged_task>& move()
        {
          return *static_cast< ::boost::rv<packaged_task>* >(this);
        }
        const ::boost::rv<packaged_task>& move() const
        {
          return *static_cast<const ::boost::rv<packaged_task>* >(this);
        }
#else
        packaged_task(boost::detail::thread_move_t<packaged_task> other) BOOST_NOEXCEPT:
            future_obtained(other->future_obtained)
        {
            task.swap(other->task);
            other->future_obtained=false;
        }
        packaged_task& operator=(boost::detail::thread_move_t<packaged_task> other) BOOST_NOEXCEPT
        {
            packaged_task temp(other);
            swap(temp);
            return *this;
        }
        operator boost::detail::thread_move_t<packaged_task>()
        {
            return boost::detail::thread_move_t<packaged_task>(*this);
        }
        boost::detail::thread_move_t<packaged_task> move()
        {
            return boost::detail::thread_move_t<packaged_task>(*this);
        }
#endif
#endif

        void reset()
        {
            if (!valid())
                throw future_error(system::make_error_code(future_errc::no_state));
            task->reset();
            future_obtained=false;
        }

        void swap(packaged_task& other) BOOST_NOEXCEPT
        {
            task.swap(other.task);
            std::swap(future_obtained,other.future_obtained);
        }
        bool valid() const BOOST_NOEXCEPT
        {
          return task.get()!=0;
        }

        // result retrieval
        BOOST_THREAD_FUTURE<R> get_future()
        {
            if(!task)
            {
                boost::throw_exception(task_moved());
            }
            else if(!future_obtained)
            {
                future_obtained=true;
                return BOOST_THREAD_FUTURE<R>(task);
            }
            else
            {
                boost::throw_exception(future_already_retrieved());
            }
            return BOOST_THREAD_FUTURE<R>();

        }


        // execution
        void operator()()
        {
            if(!task)
            {
                boost::throw_exception(task_moved());
            }
            task->run();
        }

        template<typename F>
        void set_wait_callback(F f)
        {
            task->set_wait_callback(f,this);
        }

    };

#if defined BOOST_THREAD_PROVIDES_FUTURE_CTOR_ALLOCATORS
      template <class R, class Alloc>
      struct uses_allocator<packaged_task<R>, Alloc>
        : public true_type {};
#endif

#ifdef BOOST_NO_RVALUE_REFERENCES
#if ! defined BOOST_THREAD_USES_MOVE
    template <typename T>
    struct has_move_emulation_enabled_aux<packaged_task<T> >
      : BOOST_MOVE_BOOST_NS::integral_constant<bool, true>
    {};
#endif
#endif

}


#endif
