// Copyright (C) 2013,2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// 2013/09 Vicente J. Botet Escriba
//    Adapt to boost from CCIA C++11 implementation

#ifndef BOOST_THREAD_EXECUTORS_EXECUTOR_ADAPTOR_HPP
#define BOOST_THREAD_EXECUTORS_EXECUTOR_ADAPTOR_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/executors/executor.hpp>
#include <boost/thread/detail/move.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
namespace executors
{
  /**
   * Polymorphic adaptor of a model of Executor to an executor.
   */
  template <typename Executor>
  class executor_adaptor : public executor
  {
    Executor ex;
  public:
    /// type-erasure to store the works to do
    typedef  executor::work work;

    /**
     * executor_adaptor constructor
     */
    //executor_adaptor(executor_adaptor const&) = default;
    //executor_adaptor(executor_adaptor &&) = default;
    BOOST_THREAD_COPYABLE_AND_MOVABLE(executor_adaptor)

    executor_adaptor(executor_adaptor const& x) : ex(x.ex) {}
    executor_adaptor(BOOST_THREAD_RV_REF(executor_adaptor) x)  : ex(boost::move(x.ex)) {}

    executor_adaptor& operator=(BOOST_THREAD_COPY_ASSIGN_REF(executor_adaptor) x)
    {
      if (this != &ex)
      {
        ex = x.ex;
      }
      return *this;
    }
    executor_adaptor& operator=(BOOST_THREAD_RV_REF(executor_adaptor) x)
    {
      if (this != &ex)
      {
        ex = boost::move(x.ex);
      }
      return *this;
    }

    executor_adaptor(Executor const& ex) : ex(ex) {}
    executor_adaptor(BOOST_THREAD_RV_REF(Executor) ex)  : ex(boost::move(ex)) {}

    executor_adaptor() : ex() {}

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES)
    template <typename Arg, typename ...Args>
    executor_adaptor(BOOST_THREAD_FWD_REF(Arg) arg, BOOST_THREAD_FWD_REF(Args) ... args
        , typename disable_if_c<
          is_same<typename decay<Arg>::type, executor_adaptor>::value
            ||
          is_same<typename decay<Arg>::type, Executor>::value
    , int* >::type=0
        )
    : ex(boost::forward<Arg>(arg), boost::forward<Args>(args)...) {}
#else
    /**
     * executor_adaptor constructor
     */
    template <typename A1>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1
        ) :
      ex(
          boost::forward<A1>(a1)
          ) {}
    template <typename A1, typename A2>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1,
        BOOST_THREAD_FWD_REF(A2) a2
        ) :
      ex(
          boost::forward<A1>(a1),
          boost::forward<A2>(a2)
          ) {}
    template <typename A1, typename A2, typename A3>
    executor_adaptor(
        BOOST_THREAD_FWD_REF(A1) a1,
        BOOST_THREAD_FWD_REF(A2) a2,
        BOOST_THREAD_FWD_REF(A3) a3
        ) :
      ex(
          boost::forward<A1>(a1),
          boost::forward<A2>(a2),
          boost::forward<A3>(a3)
          ) {}
#endif
    Executor& underlying_executor() { return ex; }

    /**
     * \b Effects: close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close() { ex.close(); }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed() { return ex.closed(); }

    /**
     * \b Effects: The specified closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the executor will call std::terminate, as is the case with threads.
     *
     * \b Synchronization: completion of closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \b Throws: \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */
    void submit(BOOST_THREAD_RV_REF(work) closure)  {
      return ex.submit(boost::move(closure));
    }

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      submit(work(closure));
    }
#endif
    void submit(void (*closure)())
    {
      submit(work(closure));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_FWD_REF(Closure) closure)
    {
      //submit(work(boost::forward<Closure>(closure)));
      work w((boost::forward<Closure>(closure)));
      submit(boost::move(w));
    }

    /**
     * Effects: try to execute one task.
     * Returns: whether a task has been executed.
     * Throws: whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one() { return ex.try_executing_one(); }

  };
}
using executors::executor_adaptor;

BOOST_THREAD_DCL_MOVABLE_BEG(T) executor_adaptor<T> BOOST_THREAD_DCL_MOVABLE_END

}

#include <boost/config/abi_suffix.hpp>

#endif
