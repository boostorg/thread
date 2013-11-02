// Copyright (C) 2013 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// futtest.cpp
#include <iostream>
#define BOOST_THREAD_VERSION 4

#include <boost/thread/future.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/chrono.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>

using namespace boost;

typedef shared_ptr< promise<int> > IntPromise;

void foo(IntPromise p)
{
    std::cout << "foo" << std::endl;
    p->set_value(123); // This line locks the future's mutex, then calls the continuation with the mutex already locked.
}

void bar(future<int> fooResult)
{
    std::cout << "bar" << std::endl;
    int i = fooResult.get(); // Code hangs on this line (Due to future already being locked by the set_value call)
    std::cout << "i: " << i << std::endl;
}

int main()
{
    IntPromise p(new promise<int>());
    thread t(boost::bind(foo, p));
    future<int> f1 = p->get_future();
    //f1.then(launch::deferred, boost::bind(bar, _1));
    f1.then(launch::deferred, &bar);
    t.join();
}

