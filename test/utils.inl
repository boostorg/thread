#if !defined(BOOST_UTILS_INL_WEK01212003)
#define BOOST_UTILS_INL_WEK01212003

#include <boost/thread/xtime.hpp>

namespace {

#if defined(BOOST_NO_INT64_T)
typedef boost::int_fast32_t sec_type;
#else
typedef boost::int_fast64_t sec_type;
#endif
typedef boost::int_fast32_t nsec_type;

inline boost::xtime xtime_get_future(sec_type secs, nsec_type nsecs=0)
{
	boost::xtime xt;
    BOOST_CHECK_EQUAL(boost::xtime_get(&xt, boost::TIME_UTC),
        static_cast<int>(boost::TIME_UTC));
	xt.sec += secs;
	xt.nsec += nsecs;
	return xt;
}

inline void xtime_get(boost::xtime& xt, sec_type secs, nsec_type nsecs=0)
{
	boost::xtime_get(&xt, boost::TIME_UTC);
	xt.sec += secs;
	xt.nsec += nsecs;
}

inline bool xtime_in_range(const boost::xtime& xt, sec_type less_secs,
    sec_type greater_secs)
{
    boost::xtime cur;
    BOOST_CHECK_EQUAL(boost::xtime_get(&cur, boost::TIME_UTC),
        static_cast<int>(boost::TIME_UTC));

    boost::xtime less = cur;
    less.sec += less_secs;

    boost::xtime greater = cur;
    greater.sec += greater_secs;

    return (boost::xtime_cmp(xt, less) >= 0) &&
        (boost::xtime_cmp(xt, greater) <= 0);
}

} // namespace

#endif
