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

#include <boost/thread/shared_memory.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/xtime.hpp>

#include <boost/test/test_tools.hpp>
#include <boost/bind.hpp>


namespace {

struct test_struct
{
    char buf[128];
    double val;

    test_struct(const char *msg="",double v=0.0)
    {
        strcpy(buf,msg);
        val = v;
    }
};

}

struct CreateDefault
{
    test_struct *operator()(void *place,size_t)
    {
        return new(place) test_struct;
    }
};


struct CreateSlow
{
    test_struct *operator()(void *place,size_t)
    {
        boost::xtime xt;
        boost::xtime_get(&xt,boost::TIME_UTC);

        xt.sec++;

        boost::thread::sleep(xt);
        return new(place) test_struct;
    }
};

struct CreateWithParams
{
    CreateWithParams(const char *msg, double v) : m_p_msg(msg),m_v(v)
    {}

    test_struct *operator()(void *place,size_t)
    {
        return new(place) test_struct(m_p_msg,m_v);
    }
    const char *m_p_msg;
    double      m_v;
};


struct CreateSlowWithParams
{
    CreateSlowWithParams(const char *msg, double v) : m_p_msg(msg),m_v(v)
    {}

    test_struct *operator()(void *place,size_t)
    {
        boost::xtime xt;
        boost::xtime_get(&xt,boost::TIME_UTC);

        xt.sec++;

        boost::thread::sleep(xt);
        return new(place) test_struct(m_p_msg,m_v);
    }
    const char *m_p_msg;
    double      m_v;
};

void test_default_ctor()
{
    const char *shared_name = "TestStruct";

    typedef boost::shared_memory so;

    so creator(shared_name,sizeof(test_struct),CreateDefault());

    test_struct *pts = (test_struct *)creator.get();
    BOOST_TEST(pts->buf[0] == 0);
    BOOST_TEST(pts->val == 0.0);

    strcpy(pts->buf,shared_name);
    pts->val = 7.0;

    so user(shared_name,sizeof(test_struct));

    test_struct *pts2 = (test_struct *)user.get();
    BOOST_TEST(strcmp(pts->buf,shared_name)==0);
    BOOST_TEST(pts->val == 7.0);
}

void test_slow_create_thread()
{
    const char *shared_name = "SlowStruct";

    boost::shared_memory creator(shared_name, sizeof(test_struct),
        CreateSlowWithParams(shared_name,8.0));

    test_struct *pts = (test_struct *)creator.get();
    BOOST_TEST(strcmp(pts->buf,shared_name)==0);
    BOOST_TEST(pts->val == 8.0);
}

void test_slow_user_thread()
{
    const char *shared_name = "SlowStruct";

    boost::shared_memory user(shared_name,sizeof(test_struct));

    test_struct *pts2 = (test_struct *)user.get();
    BOOST_TEST(strcmp(pts2->buf,shared_name)==0);
    BOOST_TEST(pts2->val == 8.0);
}

void test_slow_ctor()
{
    boost::thread t1(&test_slow_create_thread);

    // Give the creator a chance to get moving.
    boost::xtime xt;
    boost::xtime_get(&xt,boost::TIME_UTC);
    xt.nsec += 250000000;
    boost::thread::sleep(xt);

    boost::thread t2(&test_slow_user_thread);

    t2.join();
    t1.join();
}

void test_shared_memory()
{
    test_default_ctor();
    test_slow_ctor();
}
