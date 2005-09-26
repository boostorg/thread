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

// (C) Copyright 2005 Anthony Williams

#include <boost/thread/detail/config.hpp>

#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>
#include <boost/thread/read_write_mutex.hpp>

#include <boost/test/unit_test.hpp>
#include <libs/thread/test/util.inl>

#include <iostream>

#define TS_CHECK(pred) \
    do { if (!(pred)) BOOST_ERROR (#pred); } while (0)
#define TS_CHECK_MSG(pred, msg) \
    do { if (!(pred)) BOOST_ERROR (msg); } while (0)

namespace {

int shared_val = 0;

boost::xtime xsecs(int secs)
{
    //Create an xtime that is secs seconds from now
    boost::xtime ret;
    TS_CHECK (boost::TIME_UTC == boost::xtime_get(&ret, boost::TIME_UTC));
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
        , wait_for_lock_secs_(wait_for_lock_secs)
        , sleep_with_lock_secs_(sleep_with_lock_secs)
        , test_promotion_and_demotion_(demote_after_write)
        , value_(k_data_init)
        , rw_(m)
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
    try
    {
        data<RW>* pdata = (data<RW>*) arg;
        TS_CHECK_MSG(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

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
    catch(...)
    {
        TS_CHECK_MSG(false, "plain_writer() exception!");
        throw;
    }
}

template<typename RW>
void plain_reader(void* arg, RW& rw)
{
    try
    {
        data<RW>* pdata = (data<RW>*)arg;
        TS_CHECK(!pdata->test_promotion_and_demotion_);
        TS_CHECK_MSG(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

        typename RW::scoped_read_lock l(rw);

        if (pdata->sleep_with_lock_secs_ > 0)
            boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

        pdata->value_ = shared_val;
    }
    catch(...)
    {
        TS_CHECK_MSG(false, "plain_reader() exception!");
        throw;
    }
}

template<typename RW>
void try_writer(void* arg, RW& rw)
{
    try
    {
        data<RW>* pdata = (data<RW>*) arg;
        TS_CHECK_MSG(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

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
    catch(...)
    {
        TS_CHECK_MSG(false, "try_writer() exception!");
        throw;
    }
}

template<typename RW>
void try_reader(void*arg, RW& rw)
{
    try
    {
        data<RW>* pdata = (data<RW>*)arg;
        TS_CHECK(!pdata->test_promotion_and_demotion_);
        TS_CHECK_MSG(pdata->wait_for_lock_secs_ == 0, "pdata->wait_for_lock_secs_: " << pdata->wait_for_lock_secs_);

        typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::unlocked);

        if (l.try_read_lock())
        {
            if (pdata->sleep_with_lock_secs_ > 0)
                boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

            pdata->value_ = shared_val;
        }
    }
    catch(...)
    {
        TS_CHECK_MSG(false, "try_reader() exception!");
        throw;
    }
}

template<typename RW>
void timed_writer(void* arg, RW& rw)
{
    try
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
    catch(...)
    {
        TS_CHECK_MSG(false, "timed_writer() exception!");
        throw;
    }
}

template<typename RW>
void timed_reader(void* arg, RW& rw)
{
    try
    {
        data<RW>* pdata = (data<RW>*)arg;
        TS_CHECK(!pdata->test_promotion_and_demotion_);

        typename RW::scoped_timed_read_write_lock l(rw,boost::read_write_lock_state::unlocked);

        boost::xtime xt = xsecs(pdata->wait_for_lock_secs_);
        if (l.timed_read_lock(xt))
        {
            if (pdata->sleep_with_lock_secs_ > 0)
                boost::thread::sleep(xsecs(pdata->sleep_with_lock_secs_));

            pdata->value_ = shared_val;
        }
    }
    catch(...)
    {
        TS_CHECK_MSG(false, "timed_reader() exception!");
        throw;
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

bool shared_test_writelocked = false;
bool shared_test_readlocked = false;
bool shared_test_unlocked = false;

template<typename RW>
void run_try_tests(void* arg, RW& rw)
{
    try
    {
        TS_CHECK(shared_test_writelocked || shared_test_readlocked || shared_test_unlocked);

        typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::unlocked);

        if (shared_test_writelocked)
        {
            //Verify that write lock blocks other write locks
            TS_CHECK(!l.try_write_lock());

            //Verify that write lock blocks read locks
            TS_CHECK(!l.try_read_lock());
        }
        else if (shared_test_readlocked)
        {
            //Verify that read lock blocks write locks
            TS_CHECK(!l.try_write_lock());

            //Verify that read lock does not block other read locks
            TS_CHECK(l.try_read_lock());

            //Verify that read lock blocks promotion
            TS_CHECK(!l.try_promote());
        }
        else if (shared_test_unlocked)
        {
            //Verify that unlocked does not blocks write locks
            TS_CHECK(l.try_write_lock());

            //Verify that unlocked does not block demotion
            TS_CHECK(l.try_demote());

            l.unlock();

            //Verify that unlocked does not block read locks
            TS_CHECK(l.try_read_lock());

            //Verify that unlocked does not block promotion
            TS_CHECK(l.try_promote());
            
            l.unlock();
        }
    }
    catch(...)
    {
        TS_CHECK_MSG(false, "run_try_tests() exception!");
        throw;
    }
}

template<typename RW>
void test_plain_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    //Verify that a write lock prevents both readers and writers from obtaining a lock
    {
        shared_val = 0;
        data<RW> r1(1, rw, 0, 0);
        data<RW> r2(2, rw, 0, 0);
        data<RW> w1(3, rw, 0, 0);
        data<RW> w2(4, rw, 0, 0);

        //Write-lock the mutex and queue up other readers and writers

        typename RW::scoped_read_write_lock l(rw, boost::read_write_lock_state::write_locked);

        boost::thread tr1(thread_adapter<RW>(plain_reader, &r1, rw));
        boost::thread tr2(thread_adapter<RW>(plain_reader, &r2, rw));
        boost::thread tw1(thread_adapter<RW>(plain_writer, &w1, rw));
        boost::thread tw2(thread_adapter<RW>(plain_writer, &w2, rw));

        boost::thread::sleep(xsecs(1));

        //At this point, neither queued readers nor queued writers should have obtained access

        TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
        TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
        TS_CHECK_MSG(r1.value_ == k_data_init, MESSAGE);
        TS_CHECK_MSG(r2.value_ == k_data_init, MESSAGE);

        if (test_promotion_and_demotion)
        {
            l.demote();
            boost::thread::sleep(xsecs(1));
            //:boost::thread tr3(thread_adapter<RW>(plain_reader, &r3, rw));

//             if (rw.policy() == boost::read_write_scheduling_policy::writer_priority)
//             {
//                 //Expected result:
//                 //Since writers have priority, demotion doesn't release any readers.
//                 TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == k_data_init, MESSAGE);
//             }
//             else if (rw.policy() == boost::read_write_scheduling_policy::reader_priority)
//             {
//                 //Expected result:
//                 //Since readers have priority, demotion releases all readers.
//                 TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//             }
//             else if (rw.policy() == boost::read_write_scheduling_policy::alternating_many_reads)
//             {
//                 //Expected result:
//                 //Since readers can be released many at a time, demotion releases all queued readers.
//                 TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//                 //:TS_CHECK_MSG(r3.value_ == k_data_init, MESSAGE);
//             }
//             else if (rw.policy() == boost::read_write_scheduling_policy::alternating_single_read)
//             {
//                 //Expected result:
//                 //Since readers can be released only one at a time, demotion releases one queued reader.
//                 TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == k_data_init || r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == k_data_init || r2.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ != r2.value_, MESSAGE);
//             }
        }
        
        l.unlock();

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();

//         if (rw.policy() == boost::read_write_scheduling_policy::writer_priority)
//         {
//             if (!test_promotion_and_demotion)
//             {
//                 //Expected result: 
//                 //1) either w1 or w2 obtains and releases the lock
//                 //2) the other of w1 and w2 obtains and releases the lock
//                 //3) r1 and r2 obtain and release the lock "simultaneously"
//                 TS_CHECK_MSG(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 20, MESSAGE);
//             }
//             else
//             {
//                 //Expected result: 
//                 //The same, except that either w1 or w2 (but not both) may
//                 //fail to promote to a write lock,
//                 //and r1, r2, or both may "sneak in" ahead of w1 and/or w2
//                 //by obtaining a read lock before w1 or w2 can promote
//                 //their initial read lock to a write lock.
//                 TS_CHECK_MSG(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == k_data_init || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == k_data_init || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
//             }
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::reader_priority)
//         {
//             if (!test_promotion_and_demotion)
//             {
//                 //Expected result: 
//                 //1) r1 and r2 obtain and release the lock "simultaneously"
//                 //2) either w1 or w2 obtains and releases the lock
//                 //3) the other of w1 and w2 obtains and releases the lock
//                 TS_CHECK_MSG(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//             }
//             else
//             {
//                 //Expected result: 
//                 //The same, except that either w1 or w2 (but not both) may
//                 //fail to promote to a write lock.
//                 TS_CHECK_MSG(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//             }
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::alternating_many_reads)
//         {
//             if (!test_promotion_and_demotion)
//             {
//                 //Expected result: 
//                 //1) r1 and r2 obtain and release the lock "simultaneously"
//                 //2) either w1 or w2 obtains and releases the lock
//                 //3) the other of w1 and w2 obtains and releases the lock
//                 TS_CHECK_MSG(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//             }
//             else
//             {
//                 //Expected result: 
//                 //The same, except that either w1 or w2 (but not both) may
//                 //fail to promote to a write lock.
//                 TS_CHECK_MSG(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//             }
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::alternating_single_read)
//         {
//             if (!test_promotion_and_demotion)
//             {
//                 //Expected result:
//                 //1) either r1 or r2 obtains and releases the lock
//                 //2) either w1 or w2 obtains and releases the lock
//                 //3) the other of r1 and r2 obtains and releases the lock
//                 //4) the other of w1 and w2 obtains and release the lock
//                 TS_CHECK_MSG(w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0 || r1.value_ == 10, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0 || r2.value_ == 10, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ != r2.value_, MESSAGE);
//             }
//             else
//             {
//                 //Expected result:
//                 //Since w1 and w2 start as read locks, r1, r2, w1, and w2 
//                 //obtain read locks "simultaneously". Each of w1 and w2, 
//                 //after it obtain a read lock, attempts to promote to a
//                 //write lock; this attempt fails if the other has
//                 //already done so and currently holds the write lock;
//                 //otherwise it will succeed as soon as any other 
//                 //read locks have been released.
//                 //In other words, any ordering is possible, and either
//                 //w1 or w2 (but not both) may fail to obtain the lock
//                 //altogether.
//                 TS_CHECK_MSG(w1.value_ == k_data_init || w1.value_ == 10 || w1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w2.value_ == k_data_init || w2.value_ == 10 || w2.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(w1.value_ != w2.value_, MESSAGE);
//                 TS_CHECK_MSG(r1.value_ == 0 || r1.value_ == 10 || r1.value_ == 20, MESSAGE);
//                 TS_CHECK_MSG(r2.value_ == 0 || r2.value_ == 10 || r2.value_ == 20, MESSAGE);
//             }
//         }
    }

    //Verify that a read lock prevents readers but not writers from obtaining a lock
    {
        shared_val = 0;
        data<RW> r1(1, rw, 0, 0);
        data<RW> r2(2, rw, 0, 0);
        data<RW> w1(3, rw, 0, 0);
        data<RW> w2(4, rw, 0, 0);

        //Read-lock the mutex and queue up other readers and writers

        typename RW::scoped_read_write_lock l(rw, boost::read_write_lock_state::read_locked);

        boost::thread tr1(thread_adapter<RW>(plain_reader, &r1, rw));
        boost::thread tr2(thread_adapter<RW>(plain_reader, &r2, rw));

        boost::thread::sleep(xsecs(1));

        boost::thread tw1(thread_adapter<RW>(plain_writer, &w1, rw));
        boost::thread tw2(thread_adapter<RW>(plain_writer, &w2, rw));

        boost::thread::sleep(xsecs(1));

        //Expected result: all readers passed through before the writers entered
        TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
        TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
        TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
        TS_CHECK_MSG(r2.value_ == 0, MESSAGE);

        if (test_promotion_and_demotion)
        {
            l.promote();
        }
        
        l.unlock();

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();
    }

    //Verify that a read lock prevents readers but not writers from obtaining a lock
    {
        shared_val = 0;
        data<RW> r1(1, rw, 0, 0);
        data<RW> r2(2, rw, 0, 0);
        data<RW> w1(3, rw, 0, 0);
        data<RW> w2(4, rw, 0, 0);

        //Read-lock the mutex and queue up other readers and writers

        typename RW::scoped_read_write_lock l(rw, boost::read_write_lock_state::read_locked);

        boost::thread tw1(thread_adapter<RW>(plain_writer, &w1, rw));
        boost::thread tw2(thread_adapter<RW>(plain_writer, &w2, rw));

        boost::thread::sleep(xsecs(1));

        boost::thread tr1(thread_adapter<RW>(plain_reader, &r1, rw));
        boost::thread tr2(thread_adapter<RW>(plain_reader, &r2, rw));

        boost::thread::sleep(xsecs(1));

//         if (rw.policy() == boost::read_write_scheduling_policy::writer_priority)
//         {
//             //Expected result: 
//             //Writers have priority, so no readers have been released
//             TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r2.value_ == k_data_init, MESSAGE);
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::reader_priority)
//         {
//             //Expected result: 
//             //Readers have priority, so all readers have been released
//             TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r1.value_ == 0, MESSAGE);
//             TS_CHECK_MSG(r2.value_ == 0, MESSAGE);
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::alternating_many_reads)
//         {
//             //Expected result: 
//             //It's the writers' turn, so no readers have been released
//             TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r2.value_ == k_data_init, MESSAGE);
//         }
//         else if (rw.policy() == boost::read_write_scheduling_policy::alternating_single_read)
//         {
//             //Expected result: 
//             //It's the writers' turn, so no readers have been released
//             TS_CHECK_MSG(w1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(w2.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r1.value_ == k_data_init, MESSAGE);
//             TS_CHECK_MSG(r2.value_ == k_data_init, MESSAGE);
//         }

        if (test_promotion_and_demotion)
        {
            l.promote();
        }
        
        l.unlock();

        tr2.join();
        tr1.join();
        tw2.join();
        tw1.join();
    }
}

template<typename RW>
void test_try_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    //Repeat the plain tests with the try lock.
    //This is important to verify that try locks are proper
    //read_write_mutexes as well.

    test_plain_read_write_mutex(rw, test_promotion_and_demotion);

    //Verify try_* operations with write-locked mutex
    {
        typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::write_locked);

        shared_test_writelocked = true;
        shared_test_readlocked = false;
        shared_test_unlocked = false;
        
        boost::thread test_thread(thread_adapter<RW>(run_try_tests, NULL, rw));
        test_thread.join();
    }

    //Verify try_* operations with read-locked mutex
    {
        typename RW::scoped_try_read_write_lock l(rw, boost::read_write_lock_state::read_locked);
        
        shared_test_writelocked = false;
        shared_test_readlocked = true;
        shared_test_unlocked = false;
        
        boost::thread test_thread(thread_adapter<RW>(run_try_tests, NULL, rw));
        test_thread.join();
    }

    //Verify try_* operations with unlocked mutex
    {
        shared_test_writelocked = false;
        shared_test_readlocked = false;
        shared_test_unlocked = true;
        
        boost::thread test_thread(thread_adapter<RW>(run_try_tests, NULL, rw));
        test_thread.join();
    }
}

template<typename RW>
void test_timed_read_write_mutex(RW& rw, bool test_promotion_and_demotion)
{
    //Repeat the try tests with the timed lock.
    //This is important to verify that timed locks are proper
    //try locks as well.

    test_try_read_write_mutex(rw, test_promotion_and_demotion);

    //:More tests here
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

//         std::cout << "try test, sp=" << i 
//             << (test_promotion_and_demotion ? " with promotion & demotion" : " without promotion & demotion")
//             << "\n";
//         std::cout.flush();

//         {
//             boost::try_read_write_mutex try_rw(static_cast<boost::read_write_scheduling_policy::read_write_scheduling_policy_enum>(i));
//             test_try_read_write_mutex(try_rw, test_promotion_and_demotion);
//         }

//         std::cout << "timed test, sp=" << i 
//             << (test_promotion_and_demotion ? " with promotion & demotion" : " without promotion & demotion") 
//             << "\n";
//         std::cout.flush();

//         {
//             boost::timed_read_write_mutex timed_rw(static_cast<boost::read_write_scheduling_policy::read_write_scheduling_policy_enum>(i));
//             test_timed_read_write_mutex(timed_rw, test_promotion_and_demotion);
//         }
    }
}

void test_read_write_mutex()
{
    do_test_read_write_mutex(false);
    do_test_read_write_mutex(true);
}

namespace
{
    template<typename lock_type>
    class locking_thread
    {
        boost::read_write_mutex& rw_mutex;
        unsigned& unblocked_count;
        boost::mutex& unblocked_count_mutex;
        boost::mutex& finish_mutex;
    public:
        locking_thread(boost::read_write_mutex& rw_mutex_,
                       unsigned& unblocked_count_,
                       boost::mutex& unblocked_count_mutex_,
                       boost::mutex& finish_mutex_):
            rw_mutex(rw_mutex_),
            unblocked_count(unblocked_count_),
            unblocked_count_mutex(unblocked_count_mutex_),
            finish_mutex(finish_mutex_)
        {}
        
        void operator()()
        {
            // acquire lock
            lock_type lock(rw_mutex);
            
            // increment count to show we're unblocked
            {
                boost::mutex::scoped_lock ublock(unblocked_count_mutex);
                ++unblocked_count;
            }
            
            // wait to finish
            boost::mutex::scoped_lock finish_lock(finish_mutex);
        }
    };
    
}


void test_multiple_readers()
{
    unsigned const number_of_threads=100;
    
    boost::thread_group pool;

    boost::read_write_mutex rw_mutex;
    unsigned unblocked_count=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    for(unsigned i=0;i<number_of_threads;++i)
    {
        pool.create_thread(locking_thread<boost::read_write_mutex::scoped_read_lock>(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex));
    }

    boost::thread::sleep(delay(1));

    BOOST_CHECK_EQUAL(unblocked_count,number_of_threads);

    finish_lock.unlock();

    pool.join_all();
}

void test_reader_blocks_writer()
{
    boost::thread_group pool;

    boost::read_write_mutex rw_mutex;
    unsigned unblocked_count=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    pool.create_thread(locking_thread<boost::read_write_mutex::scoped_read_lock>(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex));
    boost::thread::sleep(delay(1));
    BOOST_CHECK_EQUAL(unblocked_count,1U);
    pool.create_thread(locking_thread<boost::read_write_mutex::scoped_write_lock>(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex));
    boost::thread::sleep(delay(1));
    BOOST_CHECK_EQUAL(unblocked_count,1U);

    finish_lock.unlock();

    pool.join_all();

    BOOST_CHECK_EQUAL(unblocked_count,2U);
}

void test_only_one_writer_permitted()
{
    unsigned const number_of_threads=100;
    
    boost::thread_group pool;

    boost::read_write_mutex rw_mutex;
    unsigned unblocked_count=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);
    
    for(unsigned i=0;i<number_of_threads;++i)
    {
        pool.create_thread(locking_thread<boost::read_write_mutex::scoped_write_lock>(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex));
    }

    boost::thread::sleep(delay(1));

    BOOST_CHECK_EQUAL(unblocked_count,1);

    finish_lock.unlock();

    pool.join_all();

    BOOST_CHECK_EQUAL(unblocked_count,number_of_threads);
}

void test_unlocking_writer_unblocks_all_readers()
{
    boost::thread_group pool;

    boost::read_write_mutex rw_mutex;
    boost::read_write_mutex::scoped_write_lock write_lock(rw_mutex);
    unsigned unblocked_count=0;
    boost::mutex unblocked_count_mutex;
    boost::mutex finish_mutex;
    boost::mutex::scoped_lock finish_lock(finish_mutex);

    unsigned const reader_count=100;

    for(unsigned i=0;i<reader_count;++i)
    {
        pool.create_thread(locking_thread<boost::read_write_mutex::scoped_read_lock>(rw_mutex,unblocked_count,unblocked_count_mutex,finish_mutex));
    }
    boost::thread::sleep(delay(1));
    BOOST_CHECK_EQUAL(unblocked_count,0U);

    write_lock.unlock();
    
    boost::thread::sleep(delay(1));
    BOOST_CHECK_EQUAL(unblocked_count,reader_count);

    finish_lock.unlock();
    pool.join_all();
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: read_write_mutex test suite");

    test->add(BOOST_TEST_CASE(&test_read_write_mutex));
    test->add(BOOST_TEST_CASE(&test_multiple_readers));
    test->add(BOOST_TEST_CASE(&test_reader_blocks_writer));
    test->add(BOOST_TEST_CASE(&test_only_one_writer_permitted));
    test->add(BOOST_TEST_CASE(&test_unlocking_writer_unblocks_all_readers));

    return test;
}
