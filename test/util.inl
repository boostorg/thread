#if !defined(UTIL_INL_WEK01242003)
#define UTIL_INL_WEK01242003

#include <boost/thread/xtime.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition.hpp>
#include <boost/thread/thread.hpp>

namespace
{
	inline boost::xtime delay(int secs)
	{
		boost::xtime xt;
        BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC),
            static_cast<int>(boost::TIME_UTC));
		xt.sec += secs;
		return xt;
	}

    class execution_monitor
    {
    public:
		enum wait_type { use_sleep_only, use_mutex, use_condition };

        execution_monitor(wait_type type, int secs)
			: done(false), type(type), secs(secs) { }
		void start() {
			if (type != use_sleep_only) {
				boost::mutex::scoped_lock lock(mutex); done = false;
			} else {
				done = false;
			}
		}
        void finish() {
			if (type != use_sleep_only) {
				boost::mutex::scoped_lock lock(mutex);
				done = true;
				if (type == use_condition)
					cond.notify_one();
			} else {
				done = true;
			}
		}
		bool wait() {
			boost::xtime xt = delay(secs);
			if (type != use_condition)
				boost::thread::sleep(xt);
			if (type != use_sleep_only) {
				boost::mutex::scoped_lock lock(mutex);
				while (type == use_condition && !done) {
					if (!cond.timed_wait(lock, xt))
						break;
				}
				return done;
			}
			return done;
		}

    private:
        boost::mutex mutex;
		boost::condition cond;
        bool done;
		wait_type type;
		int secs;
    };

	template <typename F>
    class indirect_adapter
    {
    public:
        indirect_adapter(F func, execution_monitor& monitor)
            : func(func), monitor(monitor) { }
        void operator()() const
        {
			try
			{
				boost::thread thrd(func);
				thrd.join();
			}
			catch (...)
			{
				monitor.finish();
				throw;
			}
			monitor.finish();
        }

    private:
		F func;
        execution_monitor& monitor;
    };

	template <typename F>
	void timed_test(F func, int secs,
		execution_monitor::wait_type type=execution_monitor::use_condition)
	{
		execution_monitor monitor(type, secs);
		indirect_adapter<F> ifunc(func, monitor);
		monitor.start();
		boost::thread thrd(ifunc);
		BOOST_REQUIRE_MESSAGE(monitor.wait(),
			"Timed test didn't complete in time, possible deadlock.");
	}

	template <typename F, typename T>
    class binder
    {
    public:
        binder(const F& func, const T& param)
            : func(func), param(param) { }
        void operator()() const { func(param); }

    private:
		F func;
        T param;
    };

	template <typename F, typename T>
	binder<F, T> bind(const F& func, const T& param)
	{
		return binder<F, T>(func, param);
	}
} // namespace

#endif
