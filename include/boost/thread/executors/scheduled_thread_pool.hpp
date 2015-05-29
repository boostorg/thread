// Copyright (C) 2014 Ian Forbed
// Copyright (C) 2014 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#ifndef BOOST_THREAD_EXECUTORS_SCHEDULED_THREAD_POOL_HPP
#define BOOST_THREAD_EXECUTORS_SCHEDULED_THREAD_POOL_HPP

#include <boost/thread/executors/detail/scheduled_executor_base.hpp>
#include <boost/thread/executors/work.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/scoped_thread.hpp>
#include <boost/thread/csbl/vector.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared.hpp>

namespace boost
{
namespace executors
{

  template <class Clock = chrono::steady_clock>
  class scheduled_thread_pool
  {
  private:

    struct shared_state : public detail::scheduled_executor_base<> {

      /// basic_thread_pool is not copyable.
      BOOST_THREAD_NO_COPYABLE(shared_state)

      typedef detail::scheduled_executor_base<> super;
      typedef typename super::work work;

      typedef scoped_thread<> thread_t;
      typedef csbl::vector<thread_t> thread_vector;
      thread_vector threads;

      shared_state(unsigned const thread_count = thread::hardware_concurrency()+1) : super()
      {

        try
        {
          threads.reserve(thread_count);
          for (unsigned i = 0; i < thread_count; ++i)
          {
  #if 1
            thread th (&shared_state::loop, this);
            threads.push_back(thread_t(boost::move(th)));
  #else
            threads.push_back(thread_t(&shared_state::loop, this)); // do not compile
  #endif
          }
        }
        catch (...)
        {
          close();
          throw;
        }
      }

      /**
       * \b Effects: Destroys the thread pool.
       *
       * \b Synchronization: The completion of all the closures happen before the completion of the \c basic_thread_pool destructor.
       */
      ~shared_state()
      {
        this->close();
      }
    }; //end class

  public:
    typedef typename shared_state::work work;
    typedef Clock clock;
    typedef typename clock::duration duration;
    typedef typename clock::time_point time_point;


    /**
     * \b Effects: creates a thread pool that runs closures on \c thread_count threads.
     *
     * \b Throws: Whatever exception is thrown while initializing the needed resources.
     */
    scheduled_thread_pool(unsigned const thread_count = thread::hardware_concurrency()+1)
    : pimpl(make_shared<shared_state>(thread_count))
    {
    }

    /**
     * \b Effects: Destroys the thread pool.
     *
     * \b Synchronization: The completion of all the closures happen before the completion of the \c basic_thread_pool destructor.
     */
    ~scheduled_thread_pool()
    {
    }
    /**
     * \b Effects: close the \c serial_executor for submissions.
     * The loop will work until there is no more closures to run.
     */
    void close()
    {
      pimpl->close();
    }

    /**
     * \b Returns: whether the pool is closed for submissions.
     */
    bool closed()
    {
      return pimpl->closed();
    }

    void submit_at(work w, const time_point& tp)
    {
      return pimpl->submit_at(boost::move(w), tp);
    }

    void submit_after(work w, const duration& d)
    {
      return pimpl->submit_after(boost::move(w), d);
    }
  private:
    shared_ptr<shared_state> pimpl;
  };
} //end executors namespace

using executors::scheduled_thread_pool;

} //end boost
#endif

