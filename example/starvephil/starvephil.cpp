#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>
#include <iostream>
#include <time.h>

namespace
{
	boost::mutex iomx;
};

class canteen
{
public:
	canteen() : m_chickens(0) { }

	void get(int id)
	{
		boost::mutex::lock lock(m_mutex);
		while (m_chickens == 0)
		{
			{
				boost::mutex::lock lock(iomx);
				std::cout << "(" << clock() << ") Phil" << id <<
					": wot, no chickens?  I'll WAIT ..." << std::endl;
			}
			m_condition.wait(lock);
		}
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() << ") Phil" << id <<
				": those chickens look good ... one please ..." << std::endl;
		}
		m_chickens--;
	}
	void put(int value)
	{
		boost::mutex::lock lock(m_mutex);
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() <<
				") Chef: ouch ... make room ... this dish is very hot ..." << std::endl;
		}
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 3;
        boost::thread::sleep(xt);
		m_chickens += value;
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() <<
				") Chef: more chickens ... " << m_chickens <<
				" now available ... NOTIFYING ..." << std::endl;
		}
		m_condition.notify_all();
	}

private:
	boost::mutex m_mutex;
	boost::condition m_condition;
	int m_chickens;
};

canteen g_canteen;

void chef(void*)
{
	const int chickens = 4;
	{
		boost::mutex::lock lock(iomx);
		std::cout << "(" << clock() << ") Chef: starting ..." << std::endl;
	}
	for (;;)
	{
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() << ") Chef: cooking ..." << std::endl;
		}
        boost::xtime xt;
        boost::xtime_get(&xt, boost::TIME_UTC);
        xt.sec += 2;
        boost::thread::sleep(xt);
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() << ") Chef: " << chickens
				<< " chickens, ready-to-go ..." << std::endl;
		}
		g_canteen.put(chickens);
	}
}

struct phil
{
	phil(int id) : m_id(id) { }
    void run() {
		{
			boost::mutex::lock lock(iomx);
			std::cout << "(" << clock() << ") Phil" << m_id << ": starting ..." << std::endl;
		}
		for (;;)
		{
			if (m_id > 0)
            {
                boost::xtime xt;
                boost::xtime_get(&xt, boost::TIME_UTC);
                xt.sec += 3;
                boost::thread::sleep(xt);
            }
			{
				boost::mutex::lock lock(iomx);
				std::cout << "(" << clock() << ") Phil" << m_id
					<< ": gotta eat ..." << std::endl;
			}
			g_canteen.get(m_id);
			{
				boost::mutex::lock lock(iomx);
				std::cout << "(" << clock() << ") Phil" << m_id
					<< ": mmm ... that's good ..." << std::endl;
			}
		}
    }
	static void do_thread(void* param) {
        static_cast<phil*>(param)->run();
	}

	int m_id;
};

struct thread_adapt
{
    thread_adapt(void (*func)(void*), void* param) : _func(func), _param(param) { }
    int operator()() const
    {
        _func(_param);
        return 0;
    }

    void (*_func)(void*);
    void* _param;
};

int main(int argc, char* argv[])
{
    boost::thread::create(&chef, 0);
    phil p[] = { phil(0), phil(1), phil(2), phil(3), phil(4) };
    boost::thread::create(&phil::do_thread, &p[0]);
    boost::thread::create(&phil::do_thread, &p[1]);
    boost::thread::create(&phil::do_thread, &p[2]);
    boost::thread::create(&phil::do_thread, &p[3]);
    boost::thread::create(&phil::do_thread, &p[4]);
    boost::thread::join_all();
    return 0;
}
