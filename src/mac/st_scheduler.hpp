// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_ST_SCHEDULER_MJM012402_HPP
#define BOOST_ST_SCHEDULER_MJM012402_HPP


#include <CarbonEvents.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// class st_scheduler calls its pure-virtual periodic_function method periodically at
//  system task time.  This is generally 40Hz under Mac OS 9.

class st_scheduler
{
  public:
    st_scheduler();
    virtual ~st_scheduler();

  protected:
    void start_polling();
    void stop_polling();

  private:
    virtual void periodic_function() = 0;

#if TARGET_CARBON
// use event loop timers under Carbon
  private:
    static pascal void task_entry(EventLoopTimerRef pTimer, void *pRefCon);
    void task();

  private:
    EventLoopTimerUPP m_uppTask;
    EventLoopTimerRef m_pTimer;
#else
// this can be implemented using OT system tasks.  This would be mostly a copy-and-
//  paste of the dt_scheduler code, replacing DeferredTask with SystemTask and DT
//  with ST.
#   error st_scheduler unimplemented!
#endif
};


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_ST_SCHEDULER_MJM012402_HPP
