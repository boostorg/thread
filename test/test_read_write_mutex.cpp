// Copyright (C) 2001-2003
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  William E. Kempf makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/read_write_mutex.hpp>

#include <boost/test/unit_test.hpp>

#include <iostream>

namespace {

int shared_val = 0;

boost::xtime xsecs(int secs)
{
    //Create an xtime that is secs seconds from now
    boost::xtime ret;
    BOOST_CHECK(boost::TIME_UTC == boost::xtime_get(&ret, boost::TIME_UTC));
    ret.sec += secs;
    return ret;
}

#define MESSAGE "w1=" << w1.value_ << ", w2=" << w2.value_ << ", r1=" << r1.value_ << ", r2=" << r2.value_

template <typename RW>
class thread_adapter
{
public:

    thread_adapter(
        void (*func)(void*, RW&),
        void* param1,
        RW &param2
        )
        : func_(func)
        , param1_(param1)
        , param2_(param2)
    {}

    void operator()() const
    {
        func_(param1_, param2_);
    }

private:

    void (*func_)(void*, RW&);
    void* param1_;
    RW& param2_;
};

const int k_data_init = -1;

template <typename RW>
struct data
{
    data(
        int id,
        RW& m,
        int wait_for_lock_secs,
        int sleep_with_lock_secs,
        bool demote_after_write = false
        )
        : id_(id)
        , rw_(m)
        , wait_for_lock_secs_(wait_for_lock_secs)
        , sleep_with_lock_secs_(sleep_with_lock_secs)
        , test_promotion_and_demotion_(demote_after_write)
        , value_(k_data_init)
    {}

    int id_;
    int wait_for_lock_secs_;
    int sleep_with_lock_secs_;
    bool test_promotion_and_demotion_;
    int value_;

    RW& rw_;
};

template<typename RW>
void plain_writer(void* arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*) arg;
    BOOST_CHECK_MESSAGE(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

    typename RW::scoped_read_write_lock l(
        rw, 
        pdata->test_promotion_and_demotion_ 
            ? boost::read_write_lock_state::read_locked 
            : boost::read_write_lock_state::write_locked
        );

    bool succeeded = true;

    if (pdata->test_promotion_and_demotion_)
    {
        try
        {
            l.promote();
        }
        catch(const boost::lock_error&)
        {
            succeeded = false;
        }
    }

    if (succeeded)
    {
        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        shared_val += 10;

        if (pdata->test_promotion_and_demotion_)
            l.demote();

        pdata->value_ = shared_val;
    }
}

template<typename RW>
void plain_reader(void* arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*)arg;
    BOOST_CHECK(!pdata->test_promotion_and_demotion_);
    BOOST_CHECK_MESSAGE(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

    typename RW::scoped_read_write_lock l(rw, boost::read_write_lock_state::read_locked);

    if (pdata->sleep_with_lock_secs_ > 0)
        boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

    pdata->value_ = shared_val;
}

template<typename RW>
void try_writer(void* arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*) arg;
    BOOST_CHECK_MESSAGE(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

    typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::unlocked);

    bool succeeded = false;

    if (pdata->test_promotion_and_demotion_)
        succeeded = l.try_read_lock() && l.try_promote();
    else
        succeeded = l.try_write_lock();

    if (succeeded)
    {
        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        shared_val += 10;

        if (pdata->test_promotion_and_demotion_)
            l.demote();

        pdata->value_ = shared_val;
    }
}

template<typename RW>
void try_reader(void*arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*)arg;
    BOOST_CHECK(!pdata->test_promotion_and_demotion_);
    BOOST_CHECK_MESSAGE(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

    typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::unlocked);

    if (l.try_read_lock())
    {
        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        pdata->value_ = shared_val;
    }
}

template<typename RW>
void timed_writer(void* arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*)arg;

    typename RW::scoped_timed_read_write_lock l(rw, boost::read_write_lock_state::unlocked);

    bool succeeded = false;

    boost::xtime xt = xsecs(pdata->wait_for_lock_secs_);
    if (pdata->test_promotion_and_demotion_)
        succeeded = l.timed_read_lock(xt) && l.timed_promote(xt);
    else
        succeeded = l.timed_write_lock(xt);

    if (succeeded)
    {
        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        shared_val += 10;

        if (pdata->test_promotion_and_demotion_)
            l.demote();

        pdata->value_ = shared_val;
    }
}

template<typename RW>
void timed_reader(void* arg, RW& rw)
{
    data<RW>* pdata = (data<RW>*)arg;
    BOOST_CHECK(!pdata->test_promotion_and_demotion_);

    typename RW::scoped_timed_read_write_lock l(rw,boost::read_write_lock_state::unlocked);

    boost::xtime xt = xsecs(pdata->wait_for_lock_secs_);
    if (l.timed_read_lock(xt))
    {
        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        pdata->value_ = shared_val;
    }
}

template<typename RW>
void clear_data(data<RW>& data1, data<RW>& data2, data<RW>& data3, data<RW>& data4)
{
    shared_val = 0;
    data1.value_ = k_data_init;
    data2.value_ = k_data_init;
    data3.value_ = k_data_init;
    data4.value_ = k_data_init;
}

template<typename RW>
void test_read_write_mutex(RW& rw, void (*reader_func)(void*, RW&), void (*writer_func)(void*, RW&), bool test_promotion_and_demotion, int sleep_seconds, bool can_timeout)
{
    //Test for an ordering of threads that causes a
    //scheduling problem (deadlock, etc.)

    data<RW> r1(1, rw, 0, 1);
    data<RW> r2(2, rw, 0, 1);
    data<RW> w1(3, rw, 0, 1, test_promotion_and_demotion);
    data<RW> w2(4, rw, 0, 1, test_promotion_and_demotion);

    {
        //One writer

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));

        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << "\n";
        std::cout.flush();

        BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
    }

    {
        //One reader

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));

        tr1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " r1=" << r1.value_ 
            << "\n";
        std::cout.flush();

        BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
    }

    {
        //Two writers

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));

        tw2.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
        }
    }

    {
        //Two readers

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));

        tr2.join();
        tr1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
        }
        else
        {
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(!(r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //One of each, writer first

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));

        tr1.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " r1=" << r1.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 10, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10, MESSAGE);
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(!(r1.value_ == k_data_init && w1.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //One of each, reader first

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));

        tw1.join();
        tr1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " r1=" << r1.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10, MESSAGE);
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(!(r1.value_ == k_data_init && w1.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, writers first

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 20, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, readers first

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));

        tw2.join();
        tw1.join();
        tr2.join();
        tr1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, alternating, writers first

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));

        tr2.join();
        tw2.join();
        tr1.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 20, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, alternating, readers first

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));

        tw2.join();
        tr2.join();
        tw1.join();
        tr1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 10, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, readers in the middle

        clear_data(r1, r2, w1, w2);
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));

        tw2.join();
        tr2.join();
        tr1.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 10, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }

    {
        //Two of each, writers in the middle

        clear_data(r1, r2, w1, w2);
        boost::thread tr1(thread_adapter<RW>(reader_func, &r1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw1(thread_adapter<RW>(writer_func, &w1, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tw2(thread_adapter<RW>(writer_func, &w2, rw));
        if (sleep_seconds > 0) boost::thread::sleep(xsecs(sleep_seconds));
        boost::thread tr2(thread_adapter<RW>(reader_func, &r2, rw));

        tw2.join();
        tr2.join();
        tr1.join();
        tw1.join();

        std::cout << "test"
            << (test_promotion_and_demotion ? " with" : " without") << " promotion" 
            << "," << (can_timeout ? " can" : " cannot") << " timeout" 
            << ", sleep=" << sleep_seconds << " seconds:"
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (sleep_seconds > 0 && !can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 20, MESSAGE);
        }
        else if (!can_timeout && !test_promotion_and_demotion)
        {
            BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            //r1 may == r2
        }
        else
        {
            BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE((w1.value_ != w2.value_) || (w1.value_ == k_data_init && w2.value_ == k_data_init), MESSAGE);
            BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            BOOST_CHECK_MESSAGE(!(w1.value_ == k_data_init && w2.value_ == k_data_init && r1.value_ == k_data_init && r2.value_ == k_data_init), MESSAGE);
        }
    }
}

template<typename RW>
void test_plain_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    test_read_write_mutex<RW>(rw, plain_reader, plain_writer, test_promotion_and_demotion, 0, false);
    test_read_write_mutex<RW>(rw, plain_reader, plain_writer, test_promotion_and_demotion, 1, false);

    //Test read lock vs write lock ordering
    {
        shared_val = 0;
        data<RW> r1(1, rw, 0, 0);
        data<RW> r2(2, rw, 0, 0);
        data<RW> w1(1, rw, 0, 3, test_promotion_and_demotion);
        data<RW> w2(2, rw, 0, 3, test_promotion_and_demotion);

        //Writer one launches, acquires the lock, and holds it for 3 seconds.

        boost::thread tw1(thread_adapter<RW>(plain_writer, &w1, rw));

        //Sleep for one second

        boost::thread::sleep(xsecs(1));

        //Writer two launches and tries to acquire the lock "clearly"
        //after writer one has acquired it and before it has released it.
        //It holds the lock for three seconds after it gets it.

        boost::thread tw2(thread_adapter<RW>(plain_writer, &w2, rw));

        //Sleep for one second

        boost::thread::sleep(xsecs(1));

        //Readers launch and try to acquire the lock "clearly" after writer
        //two has tried to acquire it and while writer one still holds it.

        boost::thread tr1(thread_adapter<RW>(plain_reader, &r1, rw));
        boost::thread tr2(thread_adapter<RW>(plain_reader, &r2, rw));

        //Wait until all threads finish, then check results

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();

        std::cout << "plain:" 
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (rw.policy() == boost::read_write_scheduling_policy::writer_priority)
        {
            if (!test_promotion_and_demotion)
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 20, MESSAGE);    //Readers get in after 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 20, MESSAGE);
            }
            else
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);    //w2 is also a reader at first, and may or may not convert to a writer before r1 and r2 run
                BOOST_CHECK_MESSAGE(r1.value_ == 10 || r1.value_ == 20, MESSAGE);    //Readers get in after 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::reader_priority)
        {
            if (!test_promotion_and_demotion)
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 10, MESSAGE);    //Readers get in before 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 10, MESSAGE);
            }
            else
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);    //w2 is also a reader at first, and may or may not convert to a writer before r1 and r2 run
                BOOST_CHECK_MESSAGE(r1.value_ == 10 || r1.value_ == 20, MESSAGE);    //Readers get in after 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::alternating_many_reads)
        {
            if (!test_promotion_and_demotion)
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 10, MESSAGE);    //Readers get in before 2nd writer //:xxxxxx == 10, sp=2, -p&d, 0 sleep
                BOOST_CHECK_MESSAGE(r2.value_ == 10, MESSAGE);
            }
            else
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);    //w2 is also a reader at first, and may or may not convert to a writer before r1 and r2 run
                BOOST_CHECK_MESSAGE(r1.value_ == 10 || r1.value_ == 20, MESSAGE);    //Readers get in after 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::alternating_single_read)
        {
            if (!test_promotion_and_demotion)
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 10 || r1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 10 || r2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ != r2.value_, MESSAGE);
            }
            else
            {
                BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);    //w2 is also a reader at first, and may or may not convert to a writer before r1 and r2 run
                BOOST_CHECK_MESSAGE(r1.value_ == 10 || r1.value_ == 20, MESSAGE);    //Readers get in after 2nd writer
                BOOST_CHECK_MESSAGE(r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
    }

    //A different ordering test (not as timing-dependent as most of the others)
    {
        shared_val = 0;
        data<RW> r1(1, rw, 0, 0);
        data<RW> r2(2, rw, 0, 0);
        data<RW> w1(1, rw, 0, 0);
        data<RW> w2(2, rw, 0, 0);

        //Write-lock the mutex and queue up other readers and writers

        typename RW::scoped_read_write_lock l(rw, boost::read_write_lock_state::write_locked);

        boost::thread tw1(thread_adapter<RW>(plain_writer, &w1, rw));
        boost::thread tw2(thread_adapter<RW>(plain_writer, &w2, rw));
        boost::thread tr1(thread_adapter<RW>(plain_reader, &r1, rw));
        boost::thread tr2(thread_adapter<RW>(plain_reader, &r2, rw));

        boost::thread::sleep(xsecs(5));

        l.unlock();

        //Release them all at once and make sure they get the lock in the correct order

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();

        std::cout << "plain:" 
            << " w1=" << w1.value_ 
            << " w2=" << w2.value_ 
            << " r1=" << r1.value_ 
            << " r2=" << r2.value_ 
            << "\n";
        std::cout.flush();

        if (rw.policy() == boost::read_write_scheduling_policy::writer_priority)
        {
            if (!test_promotion_and_demotion)
            {
                //Expected result: 
                //1) either w1 or w2 obtains and releases the lock
                //2) the other of w1 and w2 obtains and releases the lock
                //3) r1 and r2 obtain and release the lock "simultaneously"
                BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 20, MESSAGE);
            }
            else
            {
                //Expected result: 
                //The same, except that either w1 or w2 (but not both) may
                //fail to promote to a write lock,
                //and r1, r2, or both may "sneak in" ahead of w1 and/or w2
                //by obtaining a read lock before they can promote
                //their initial read lock to a write lock.
                BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == k_data_init || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == k_data_init || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::reader_priority)
        {
            if (!test_promotion_and_demotion)
            {
                //Expected result: 
                //1) r1 and r2 obtain and release the lock "simultaneously"
                //2) either w1 or w2 obtains and releases the lock
                //3) the other of w1 and w2 obtains and releases the lock
                BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
            }
            else
            {
                //Expected result: 
                //The same, except that either w1 or w2 (but not both) may
                //fail to promote to a write lock.
                BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::alternating_many_reads)
        {
            if (!test_promotion_and_demotion)
            {
                //Expected result: 
                //1) r1 and r2 obtain and release the lock "simultaneously"
                //2) either w1 or w2 obtains and releases the lock
                //3) the other of w1 and w2 obtains and releases the lock
                BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE); //:xxxxx r1.value_ == 10, plain test, sp=3, -p&d, 0 sleep; sp=2, -p&d, -timeout, 1 sleep
                BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE);
            }
            else
            {
                //Expected result: 
                //The same, except that either w1 or w2 (but not both) may
                //fail to promote to a write lock.
                BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0, MESSAGE); //:xxxxx == 10, with promotion, cannot timeout, sp=2
                BOOST_CHECK_MESSAGE(r2.value_ == 0, MESSAGE); //:xxxxx == 10, with promotion, cannot timeout, sp=2
            }
        }
        else if (rw.policy() == boost::read_write_scheduling_policy::alternating_single_read)
        {
            if (!test_promotion_and_demotion)
            {
                //Expected result:
                //1) either r1 or r2 obtains and releases the lock
                //2) either w1 or w2 obtains and releases the lock
                //3) the other of r1 and r2 obtains and releases the lock
                //4) the other of w1 and w2 obtains and release the lock
                BOOST_CHECK_MESSAGE(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ != r2.value_, MESSAGE);
            }
            else
            {
                //Expected result:
                //Since w1 and w2 start as read locks, r1, r2, w1, and w2 
                //obtain read locks "simultaneously". Each of w1 and w2, 
                //after it obtain a read lock, attempts to promote to a
                //write lock; this attempt fails if the other has
                //already done so and currently holds the write lock;
                //otherwise it will succeed as soon as any other 
                //read locks have been released.
                //In other words, any ordering is possible, and either
                //w1 or w2 (but not both) may fail to obtain the lock
                //altogether.
                BOOST_CHECK_MESSAGE(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(w1.value_ != w2.value_, MESSAGE);
                BOOST_CHECK_MESSAGE(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
                BOOST_CHECK_MESSAGE(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
            }
        }
    }
}

template<typename RW>
void test_try_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    test_read_write_mutex<RW>(rw, try_reader, try_writer, test_promotion_and_demotion, 0, true);
    test_read_write_mutex<RW>(rw, try_reader, try_writer, test_promotion_and_demotion, 1, true);

    shared_val = 0;

    data<RW> r1(1, rw, 0, 0);
    data<RW> r2(2, rw, 0, 0);
    data<RW> w1(3, rw, 0, 3, test_promotion_and_demotion);
    data<RW> w2(4, rw, 0, 3, test_promotion_and_demotion);

    //Writer one launches, acquires the lock, and holds it for 3 seconds.

    boost::thread tw1(thread_adapter<RW>(try_writer, &w1, rw));

    //Sleep for one second

    boost::thread::sleep(xsecs(1));

    //Reader one launches and tries to acquire the lock "clearly" 
    //after writer one has acquired it and before it has released it.

    boost::thread tr1(thread_adapter<RW>(try_reader, &r1, rw));

    //Writer two launches in the same timeframe.

    boost::thread tw2(thread_adapter<RW>(try_writer, &w2, rw));

    //Wait until all threads finish, then check results

    tw2.join();
    tr1.join();
    tw1.join();

    std::cout << "try:" 
        << " w1=" << w1.value_ 
        << " w2=" << w2.value_ 
        << " r1=" << r1.value_ 
        << "\n";
    std::cout.flush();

    if (!test_promotion_and_demotion || test_promotion_and_demotion)
    {
        BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
        BOOST_CHECK_MESSAGE(r1.value_ == k_data_init, MESSAGE);    //Try returns w/o waiting
        BOOST_CHECK_MESSAGE(w2.value_ == k_data_init, MESSAGE);    //Try returns w/o waiting
    }

    //Finish by repeating the plain tests with the try lock.
    //This is important to verify that try locks are proper
    //read_write_mutexes as well.

    test_plain_read_write_mutex(rw, test_promotion_and_demotion);
}

template<typename RW>
void test_timed_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    test_read_write_mutex<RW>(rw, timed_reader, timed_writer, test_promotion_and_demotion, 0, true);
    test_read_write_mutex<RW>(rw, timed_reader, timed_writer, test_promotion_and_demotion, 1, true);

    shared_val = 0;
    data<RW> r1(1, rw, 1, 0);
    data<RW> r2(2, rw, 3, 0);
    data<RW> w1(3, rw, 3, 3, test_promotion_and_demotion);
    data<RW> w2(4, rw, 1, 0, test_promotion_and_demotion);

    //Writer one launches, acquires the lock, and holds it for 3 seconds.

    boost::thread tw1(thread_adapter<RW>(timed_writer, &w1, rw));

    //Sleep for one second

    boost::thread::sleep(xsecs(1));

    //Writer two will try to acquire the lock "clearly" after 
    //writer one has acquired it and before it has released it.  
    //It will wait 1 second for the lock, then fail.

    boost::thread tw2(thread_adapter<RW>(timed_writer, &w2, rw));

    //Readers one and two will launch and try to acquire the lock "clearly" 
    //after writer one has acquired it and before it has released it.
    //Reader one will wait 1 second and then fail.
    //Reader 2 will wait 3 seconds and then succeed.

    boost::thread tr1(thread_adapter<RW>(timed_reader, &r1, rw));
    boost::thread tr2(thread_adapter<RW>(timed_reader, &r2, rw));

    //Wait until all threads finish, then check results

    tw1.join();
    tr1.join();
    tr2.join();
    tw2.join();

    std::cout << "timed:" 
        << " w1=" << w1.value_ 
        << " w2=" << w2.value_ 
        << " r1=" << r1.value_ 
        << " r2=" << r2.value_ 
        << "\n";
    std::cout.flush();

    if (!test_promotion_and_demotion || test_promotion_and_demotion)
    {
        BOOST_CHECK_MESSAGE(w1.value_ == 10, MESSAGE);
        BOOST_CHECK_MESSAGE(w2.value_ == k_data_init, MESSAGE);    //Times out
        BOOST_CHECK_MESSAGE(r1.value_ == k_data_init, MESSAGE);    //Times out
        BOOST_CHECK_MESSAGE(r2.value_ == 10, MESSAGE);
    }

    //Finish by repeating the try tests with the timed lock.
    //This is important to verify that timed locks are proper
    //try locks as well.

    test_try_read_write_mutex(rw, test_promotion_and_demotion);
}

} // namespace

void do_test_read_write_mutex(bool test_promotion_and_demotion)
{
    //Run every test for each scheduling policy

    for(int i = (int) boost::read_write_scheduling_policy::writer_priority;
        i <= (int) boost::read_write_scheduling_policy::alternating_single_read;
        i++)
    {
        std::cout << "plain test, sp=" << i 
            << (test_promotion_and_demotion ? " with promotion & demotion" : " without promotion & demotion") 
            << "\n";
        std::cout.flush();

        {
            boost::read_write_mutex plain_rw(static_cast<boost::read_write_scheduling_policy::read_write_scheduling_policy_enum>(i));
            test_plain_read_write_mutex(plain_rw, test_promotion_and_demotion);
        }

        std::cout << "try test, sp=" << i 
            << (test_promotion_and_demotion ? " with promotion & demotion" : " without promotion & demotion")
            << "\n";
        std::cout.flush();

        {
            boost::try_read_write_mutex try_rw(static_cast<boost::read_write_scheduling_policy::read_write_scheduling_policy_enum>(i));
            test_try_read_write_mutex(try_rw, test_promotion_and_demotion);
        }

        std::cout << "timed test, sp=" << i 
            << (test_promotion_and_demotion ? " with promotion & demotion" : " without promotion & demotion") 
            << "\n";
        std::cout.flush();

        {
            boost::timed_read_write_mutex timed_rw(static_cast<boost::read_write_scheduling_policy::read_write_scheduling_policy_enum>(i));
            test_timed_read_write_mutex(timed_rw, test_promotion_and_demotion);
        }
    }
}

void test_read_write_mutex()
{
    do_test_read_write_mutex(false);
    do_test_read_write_mutex(true);
}

boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: read_write_mutex test suite");

    test->add(BOOST_TEST_CASE(&test_read_write_mutex));

    return test;
}
