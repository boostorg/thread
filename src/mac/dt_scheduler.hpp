// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_DT_SCHEDULER_MJM012402_HPP
#define BOOST_DT_SCHEDULER_MJM012402_HPP


#include "periodical.hpp"

#include <OpenTransport.h>


namespace boost {

namespace threads {

namespace mac {

namespace detail {


// class dt_scheduler calls its pure-virtual periodic_function method periodically at
//    deferred task time.  This is generally 1kHz under Mac OS 9.

class dt_scheduler
{
  public:
    dt_scheduler();
    virtual ~dt_scheduler();

  protected:
    void start_polling();
    void stop_polling();

  private:
    virtual void periodic_function() = 0;

  private:
    void schedule_task();
    static pascal void task_entry(void *pRefCon);
    void task();

  private:
    bool m_bReschedule;
    OTProcessUPP m_uppTask;
    long m_lTask;
};


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_DT_SCHEDULER_MJM012402_HPP
