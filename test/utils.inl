#include <boost/thread/xtime.hpp>

namespace {

#if defined(BOOST_NO_INT64_T)
typedef boost::int_fast32_t sec_type;
#else
typedef boost::int_fast64_t sec_type;
#endif
typedef boost::int_fast32_t nsec_type;

static void xtime_get(boost::xtime& xt, sec_type secs, nsec_type nsecs=0)
{
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.sec += secs;
	xt.nsec += nsecs;
}

static int xtime_cmp(const boost::xtime& xt1, const boost::xtime& xt2)
{
	int cmp = (int)(xt1.sec - xt2.sec);
	if (cmp == 0)
		cmp = (int)(xt1.nsec - xt2.nsec);
	return cmp;
}

static bool xtime_in_range(const boost::xtime& xt, sec_type min, sec_type max)
{
	boost::xtime xt_min, xt_max;
	boost::xtime_get(&xt_min, boost::TIME_UTC);
	xt_max = xt_min;
	xt_min.sec += min;
	xt_max.sec += max;
	return (xtime_cmp(xt, xt_min) >= 0) && (xtime_cmp(xt, xt_max) <= 0);
}

} // namespace