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
#include <boost/thread/executors/basic_thread_pool.hpp>
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
    virtual const char* what() const BOOST_NOEXCEPT
    { return "task_canceled_exception";}
  };
  class task_region_handle;

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

  class task_region_handle
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

    task_region_handle()
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    : canceled(false)
#endif
    {
    }
    ~task_region_handle()
    {
      //wait_all();
    }

#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
    bool canceled;
#endif
    basic_thread_pool tp;
    exception_list exs;
    csbl::vector<future<void>> group;
    mutable mutex mtx;


  public:
    task_region_handle(const task_region_handle&) = delete;
    task_region_handle& operator=(const task_region_handle&) = delete;
    task_region_handle* operator&() const = delete;

    template<typename F>
    void run(F&& f)
    {
      lock_guard<mutex> lk(mtx);
#if defined BOOST_THREAD_TASK_REGION_HAS_SHARED_CANCELED
      if (canceled) {
        boost::throw_exception(task_canceled_exception());
        //throw task_canceled_exception();
      }
      group.push_back(async(tp, detail::wrapped<task_region_handle, F>(*this, forward<F>(f))));
#else
      group.push_back(async(tp, forward<F>(f)));
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
