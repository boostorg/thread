// Copyright (C) 2007 Anthony Williams
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying 
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#include <boost/thread/thread.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/ref.hpp>
#include <boost/utility.hpp>
#include <string>
#include <vector>

bool normal_function_called=false;

void normal_function()
{
    normal_function_called=true;
}

void test_thread_function_no_arguments()
{
    boost::thread function(normal_function);
    function.join();
    BOOST_CHECK(normal_function_called);
}

struct callable_no_args
{
    static bool called;
    
    void operator()() const
    {
        called=true;
    }
};

bool callable_no_args::called=false;

void test_thread_callable_object_no_arguments()
{
    callable_no_args func;
    boost::thread callable(func);
    callable.join();
    BOOST_CHECK(callable_no_args::called);
}

struct callable_noncopyable_no_args:
    boost::noncopyable
{
    static bool called;
    
    void operator()() const
    {
        called=true;
    }
};

bool callable_noncopyable_no_args::called=false;

void test_thread_callable_object_ref_no_arguments()
{
    callable_noncopyable_no_args func;
    
    boost::thread callable(boost::ref(func));
    callable.join();
    BOOST_CHECK(callable_noncopyable_no_args::called);
}

struct callable_one_arg
{
    static bool called;
    static int called_arg;
    
    void operator()(int arg) const
    {
        called=true;
        called_arg=arg;
    }
};

bool callable_one_arg::called=false;
int callable_one_arg::called_arg=0;

void test_thread_callable_object_one_argument()
{
    boost::thread callable(callable_one_arg(),42);
    callable.join();
    BOOST_CHECK(callable_one_arg::called);
    BOOST_CHECK_EQUAL(callable_one_arg::called_arg,42);
}

struct callable_multiple_arg
{
    static bool called_two;
    static int called_two_arg1;
    static double called_two_arg2;
    static bool called_three;
    static std::string called_three_arg1;
    static std::vector<int> called_three_arg2;
    static int called_three_arg3;
    
    void operator()(int arg1,double arg2) const
    {
        called_two=true;
        called_two_arg1=arg1;
        called_two_arg2=arg2;
    }
    void operator()(std::string const& arg1,std::vector<int> const& arg2,int arg3) const
    {
        called_three=true;
        called_three_arg1=arg1;
        called_three_arg2=arg2;
        called_three_arg3=arg3;
    }
};

bool callable_multiple_arg::called_two=false;
bool callable_multiple_arg::called_three=false;
int callable_multiple_arg::called_two_arg1;
double callable_multiple_arg::called_two_arg2;
std::string callable_multiple_arg::called_three_arg1;
std::vector<int> callable_multiple_arg::called_three_arg2;
int callable_multiple_arg::called_three_arg3;

void test_thread_callable_object_multiple_arguments()
{
    std::vector<int> x;
    for(unsigned i=0;i<7;++i)
    {
        x.push_back(i*i);
    }
    
    boost::thread callable3(callable_multiple_arg(),"hello",x,1.2);
    callable3.join();
    BOOST_CHECK(callable_multiple_arg::called_three);
    BOOST_CHECK_EQUAL(callable_multiple_arg::called_three_arg1,"hello");
    BOOST_CHECK_EQUAL(callable_multiple_arg::called_three_arg2.size(),x.size());
    for(unsigned j=0;j<x.size();++j)
    {
        BOOST_CHECK_EQUAL(callable_multiple_arg::called_three_arg2.at(j),x[j]);
    }
    
    BOOST_CHECK_EQUAL(callable_multiple_arg::called_three_arg3,1);

    double const dbl=1.234;
    
    boost::thread callable2(callable_multiple_arg(),19,dbl);
    callable2.join();
    BOOST_CHECK(callable_multiple_arg::called_two);
    BOOST_CHECK_EQUAL(callable_multiple_arg::called_two_arg1,19);
    BOOST_CHECK_EQUAL(callable_multiple_arg::called_two_arg2,dbl);
}


boost::unit_test_framework::test_suite* init_unit_test_suite(int, char*[])
{
    boost::unit_test_framework::test_suite* test =
        BOOST_TEST_SUITE("Boost.Threads: thread launching test suite");

    test->add(BOOST_TEST_CASE(test_thread_function_no_arguments));
    test->add(BOOST_TEST_CASE(test_thread_callable_object_no_arguments));
    test->add(BOOST_TEST_CASE(test_thread_callable_object_ref_no_arguments));
    test->add(BOOST_TEST_CASE(test_thread_callable_object_one_argument));
    test->add(BOOST_TEST_CASE(test_thread_callable_object_multiple_arguments));
    return test;
}
