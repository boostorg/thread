// test_barrier.cpp
//
// A simple test for a barrier implementation.

#include <iostream>
#include <boost/thread/thread.hpp>
#define BOOST_INCLUDE_MAIN

#include <boost/test/test_tools.hpp>

#include <boost/thread/barrier.hpp>

const int N_THREADS=10;

// Shared variables for simple barrier test.
boost::one_shot_barrier start_barrier(N_THREADS+1);
boost::one_shot_barrier proceed_barrier(N_THREADS);
boost::one_shot_barrier finish_barrier(N_THREADS+1);

// Shared variables for generation barrier test
boost::barrier gen_barrier(N_THREADS);


boost::mutex io_mutex;
boost::mutex calc_mutex;

long global_parameter;

static void worker1()
{
    start_barrier.wait();
    {
        boost::mutex::scoped_lock lock(io_mutex);
        std::cout << "In worker...  init global=" << global_parameter << "\n";
    }

    // Barrier to make sure everyone samples the same value before proceeding
    //  to start changing it.

    proceed_barrier.wait();

    // Perform our calculation
    {
        boost::mutex::scoped_lock lock(calc_mutex);
        global_parameter++;
    
        boost::mutex::scoped_lock lock2(io_mutex);
        std::cout << "In worker...  calc global=" << global_parameter << "\n";
    }

    finish_barrier.wait();
}



static void worker2()
{
    gen_barrier.wait();
    {
        boost::mutex::scoped_lock lock(io_mutex);
        std::cout << "In worker2...  init global=" << global_parameter << "\n";
    }

    // We have logged our output
    gen_barrier.wait();


    // Barrier to make sure everyone samples the same value before proceeding
    //  to start changing it.
    gen_barrier.wait();

    // Perform our calculation
    {
        boost::mutex::scoped_lock lock(calc_mutex);
        global_parameter++;
    
        boost::mutex::scoped_lock lock2(io_mutex);
        std::cout << "In worker2...  calc global=" << global_parameter << "\n";
    }
    // We have logged our output
    gen_barrier.wait();

    // Wait to exit until told to do so.
    gen_barrier.wait();
}


void prompt(const char *s)
{
    char dummy[25];
    boost::mutex::scoped_lock iolock(io_mutex);
    std::cout << s << std::endl;
    iolock.unlock();
    std::cin.getline(dummy,24,'\n');
}

    
void test_simple_barrier()
{
    
    int i;
    boost::thread_group g;

    global_parameter = -99;

    for(i = 0; i < N_THREADS; i++)
        g.create_thread(&worker1);

    // Simulate some info passing to workers
    global_parameter = 0;

    prompt("Press any key to allow workers to start");
    // Allow the workers to proceed
    start_barrier.wait();       // Sync point with workers.
   

    prompt("Press any key to allow workers to finish");
    finish_barrier.wait();
    {
        boost::mutex::scoped_lock lock(io_mutex);
        std::cout << "All workers exited...\n";
    }
    g.join_all();

}


void test_generational_barrier()
{
    
    int i;
    boost::thread_group g;

    global_parameter = -99;

    for(i = 0; i < N_THREADS; i++)
        g.create_thread(&worker2);

    // Simulate some info passing to workers
    global_parameter = 0;

    prompt("Press any key to allow workers to start");
    // Allow the workers to proceed
    gen_barrier.wait();       
    // Allow them to display their logs
    gen_barrier.wait();
   
    prompt("Press a key to allow workers to proceed with calc");
    // Allow the workers to proceed
    gen_barrier.wait();
    // Allow them to display their information
    gen_barrier.wait();

    
    prompt("Press a key to allow workers to exit");

    gen_barrier.wait();

    std::cout << "Final calculation : " << global_parameter << std::endl;
    g.join_all();

}


static void worker()
{
    int i;
    for(i = 0; i < 5; i++)
    {
        if(gen_barrier.wait() == boost::BOOST_SERIAL_THREAD)
        {
            global_parameter++;
        }
    }
    // Let one worker "report" the results
    if(gen_barrier.wait() == boost::BOOST_SERIAL_THREAD)
        std::cout << "Global Parameter=" << global_parameter << "\n";
}

void test_barrier()
{
    int i;
    boost::thread_group g;
    global_parameter = 0;

    for(i = 0; i < N_THREADS; i++)
        g.create_thread(&worker);

    g.join_all();

    BOOST_TEST(global_parameter == 5);
}


int test_main(int argc,char *argv[])
{
    test_barrier();

    return 0;
};

