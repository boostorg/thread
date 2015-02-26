// Copyright (C) 2015 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_GENERIC_EXECUTOR_HPP
#define BOOST_THREAD_EXECUTORS_GENERIC_EXECUTOR_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/detail/delete.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/executors/executor_adaptor.hpp>

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  namespace executors
  {

  class generic_executor
  {
    shared_ptr<executor> ex;
  public:
    /// type-erasure to store the works to do
    typedef executors::work work;

    //generic_executor(generic_executor const&) = default;
    //generic_executor(generic_executor &&) = default;

    template<typename Executor>
    generic_executor(Executor& ex)
    //: ex(make_shared<executor_ref<Executor> >(ex)) // todo check why this doesn't works with C++03
    : ex( new executor_adaptor<Executor>(ex) )
    {
    }

    //generic_executor(generic_executor const& other) noexcept    {}
    //generic_executor& operator=(generic_executor const& other) noexcept    {}


    /**
     * \par Effects
     * Close the \c executor for submissions.
     * The worker threads will work until there is no more closures to run.
     */
    void close() { ex->close(); }

    /**
     * \par Returns
     * Whether the pool is closed for submissions.
     */
    bool closed() { return ex->closed(); }

    void submit(BOOST_THREAD_RV_REF(work) closure)
    {
      ex->submit(boost::forward<work>(closure));
    }

    /**
     * \par Requires
     * \c Closure is a model of Callable(void()) and a model of CopyConstructible/MoveConstructible.
     *
     * \par Effects
     * The specified closure will be scheduled for execution at some point in the future.
     * If invoked closure throws an exception the thread pool will call std::terminate, as is the case with threads.
     *
     * \par Synchronization
     * Completion of closure on a particular thread happens before destruction of thread's thread local variables.
     *
     * \par Throws
     * \c sync_queue_is_closed if the thread pool is closed.
     * Whatever exception that can be throw while storing the closure.
     */

#if defined(BOOST_NO_CXX11_RVALUE_REFERENCES)
    template <typename Closure>
    void submit(Closure & closure)
    {
      work w ((closure));
      submit(boost::move(w));
    }
#endif
    void submit(void (*closure)())
    {
      work w ((closure));
      submit(boost::move(w));
    }

    template <typename Closure>
    void submit(BOOST_THREAD_RV_REF(Closure) closure)
    {
      work w = boost::move(closure);
      submit(boost::move(w));
    }

//    size_t num_pending_closures() const
//    {
//      return ex->num_pending_closures();
//    }

    /**
     * \par Effects
     * Try to execute one task.
     *
     * \par Returns
     * Whether a task has been executed.
     *
     * \par Throws
     * Whatever the current task constructor throws or the task() throws.
     */
    bool try_executing_one() { return ex->try_executing_one(); }

    /**
     * \par Requires
     * This must be called from an scheduled task.
     *
     * \par Effects
     * reschedule functions until pred()
     */
    template <typename Pred>
    bool reschedule_until(Pred const& pred)
    {
      do {
        //schedule_one_or_yield();
        if ( ! try_executing_one())
        {
          return false;
        }
      } while (! pred());
      return true;
    }

  };
  }
  using executors::generic_executor;
}

#include <boost/config/abi_suffix.hpp>

#endif
