#ifndef BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_TASK_REGION_HPP
#define BOOST_THREAD_EXPERIMENTAL_PARALLEL_V2_TASK_REGION_HPP

//////////////////////////////////////////////////////////////////////////////
//
// (C) Copyright Vicente J. Botet Escriba 2014. Distributed under the Boost
// Software License, Version 1.0. (See accompanying file
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// See http://www.boost.org/libs/thread for documentation.
//
//////////////////////////////////////////////////////////////////////////////
#if ! defined BOOST_NO_CXX11_RANGE_BASED_FOR
#include <boost/thread/detail/config.hpp>
#include <boost/thread/future.hpp>
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
#include <boost/thread/executors/basic_thread_pool.hpp>
#endif
#include <boost/thread/experimental/exception_list.hpp>
#include <boost/thread/experimental/parallel/v2/inline_namespace.hpp>

#include <boost/config/abi_prefix.hpp>

#define BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED

namespace boost
{
namespace experimental
{
namespace parallel
{
BOOST_THREAD_INLINE_NAMESPACE(v2)
{
  class BOOST_SYMBOL_VISIBLE task_canceled_exception: public std::exception
  {
  public:
    //task_canceled_exception() BOOST_NOEXCEPT {}
    //task_canceled_exception(const task_canceled_exception&) BOOST_NOEXCEPT {}
    //task_canceled_exception& operator=(const task_canceled_exception&) BOOST_NOEXCEPT {}
    virtual const char* what() const BOOST_NOEXCEPT_OR_NOTHROW
    { return "task_canceled_exception";}
  };

  template <class Executor>
  class task_region_handle_gen;

  namespace detail
  {
    void handle_task_region_exceptions(exception_list& errors)
    {
      try {
        boost::rethrow_exception(boost::current_exception());
        //throw boost::current_exception();
      }
      catch (task_canceled_exception& ex)
      {
      }
      catch (exception_list const& el)
      {
        for (boost::exception_ptr const& e: el)
        {
          try {
            rethrow_exception(e);
          }
          catch (...)
          {
            handle_task_region_exceptions(errors);
          }
        }
      }
      catch (...)
      {
        errors.add(boost::current_exception());
      }
    }

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    template <class TRH, class F>
    struct wrapped
    {
      TRH& tr;
      F f;
      wrapped(TRH& tr, F&& f) : tr(tr), f(move(f))
      {}
      void operator()()
      {
        try
        {
          f();
        }
        catch (...)
        {
          lock_guard<mutex> lk(tr.mtx);
          tr.canceled = true;
          handle_task_region_exceptions(tr.exs);
        }
      }
    };
#endif
  }

  template <class Executor>
  class task_region_handle_gen
  {
  private:
    // Private members and friends
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    template <class TRH, class F>
    friend struct detail::wrapped;
#endif
    template <typename F>
    friend void task_region(F&& f);
    template<typename F>
    friend void task_region_final(F&& f);
    template <class Ex, typename F>
    friend void task_region(Ex&, F&& f);
    template<class Ex, typename F>
    friend void task_region_final(Ex&, F&& f);

    void wait_all()
    {
      wait_for_all(group.begin(), group.end());

      #if ! defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED

      for (future<void>& f: group)
      {
        if (f.has_exception())
        {
          try
          {
            boost::rethrow_exception(f.get_exception_ptr());
          }
          catch (...)
          {
            detail::handle_task_region_exceptions(exs);
          }
        }
      }
      #endif
      if (exs.size() != 0)
      {
        boost::throw_exception(exs);
        //throw exs;
      }
    }
protected:
#if ! defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && ! defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    {}
#endif

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : canceled(false)
    , ex(0)
    {}
    task_region_handle_gen(Executor& ex)
    : canceled(false)
    , ex(&ex)
    {}

#endif

#if ! defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : ex(0)
    {}
    task_region_handle_gen(Executor& ex)
    : ex(&ex)
    {}
#endif

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED && ! defined BOOST_THREAD_PROVIDES_EXECUTORS
    task_region_handle_gen()
    : canceled(false)
    {
    }
#endif

    ~task_region_handle_gen()
    {
      //wait_all();
    }

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    bool canceled;
#endif
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
    Executor* ex;
#endif
    exception_list exs;
    csbl::vector<future<void>> group;
    mutable mutex mtx;


  public:
    task_region_handle_gen(const task_region_handle_gen&) = delete;
    task_region_handle_gen& operator=(const task_region_handle_gen&) = delete;
    task_region_handle_gen* operator&() const = delete;

    template<typename F>
    void run(F&& f)
    {
      lock_guard<mutex> lk(mtx);
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      if (canceled) {
        boost::throw_exception(task_canceled_exception());
        //throw task_canceled_exception();
      }
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      group.push_back(async(*ex, detail::wrapped<task_region_handle_gen<Executor>, F>(*this, forward<F>(f))));
#else
      group.push_back(async(detail::wrapped<task_region_handle_gen<Executor>, F>(*this, forward<F>(f))));
#endif
#else
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      group.push_back(async(*ex, forward<F>(f)));
#else
      group.push_back(async(forward<F>(f)));
#endif
#endif
    }

    void wait()
    {
      lock_guard<mutex> lk(mtx);
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      if (canceled) {
        boost::throw_exception(task_canceled_exception());
        //throw task_canceled_exception{};
      }
#endif
      wait_all();
    }
  };
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
  typedef basic_thread_pool default_executor;
#else
  typedef int default_executor;
#endif
  class task_region_handle :
    public task_region_handle_gen<default_executor>
  {
    default_executor tp;
    template <typename F>
    friend void task_region(F&& f);
    template<typename F>
    friend void task_region_final(F&& f);

  protected:
    task_region_handle() : task_region_handle_gen<default_executor>()
    {
#if defined BOOST_THREAD_PROVIDES_EXECUTORS
      ex = &tp;
#endif
    }
    task_region_handle(const task_region_handle&) = delete;
    task_region_handle& operator=(const task_region_handle&) = delete;
    task_region_handle* operator&() const = delete;

  };

  template <typename Executor, typename F>
  void task_region_final(Executor& ex, F&& f)
  {
    task_region_handle_gen<Executor> tr(ex);
    try
    {
      f(tr);
    }
    catch (...)
    {
      lock_guard<mutex> lk(tr.mtx);
      detail::handle_task_region_exceptions(tr.exs);
    }
    tr.wait_all();
  }

  template <typename Executor, typename F>
  void task_region(Executor& ex, F&& f)
  {
    task_region_final(ex, forward<F>(f));
  }

  template <typename F>
  void task_region_final(F&& f)
  {
    task_region_handle tr;
    try
    {
      f(tr);
    }
    catch (...)
    {
      lock_guard<mutex> lk(tr.mtx);
      detail::handle_task_region_exceptions(tr.exs);
    }
    tr.wait_all();
  }

  template <typename F>
  void task_region(F&& f)
  {
    task_region_final(forward<F>(f));
  }

} // v2
} // parallel
} // experimental
} // boost

#include <boost/config/abi_suffix.hpp>

#endif
#endif
