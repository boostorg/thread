#include <boost/thread/detail/named.hpp>

namespace {

std::string encode(const char* str)
{
	const char* digits="0123456789abcdef";
	std::string result;
	static char buf[100];
	char* ebuf = buf + 100;
	char* p = buf;
	while (*str)
	{
		if (((*str >= '0') && (*str <= '9')) ||
			((*str >= 'a') && (*str <= 'z')) ||
			((*str >= 'A') && (*str <= 'Z')) ||
			(*str == '/') || (*str == '.') || (*str == '_'))
		{
			*p = *str;
		}
		else if (*str == ' ')
		{
			*p = '+';
		}
		else
		{
			if (p + 3 >= ebuf)
			{
				*p = 0;
				result += buf;
				p = buf;
			}
			*p = '%';
			char* e = p + 2;
			int v = *str;
			while (e > p)
			{
				*e-- = digits[v % 16];
				v /= 16;
			}
			p += 2;
		}
		if (++p == ebuf)
		{
			*p = 0;
			result += buf;
			p = buf;
		}
		++str;
	}
	*p = 0;
	result += buf;
	return result;
}

std::string get_root()
{
#if defined(BOOST_HAS_WINTHREADS)
	return "";
#elif defined(BOOST_HAS_PTHREADS)
    return "/";
#else
	return "";
#endif
}

} // namespace

namespace boost {
namespace detail {

named_object::named_object()
{
}

named_object::named_object(const char* name)
	: m_name(name)
{
}

named_object::~named_object()
{
}

std::string named_object::name() const
{
	return m_name;
}

std::string named_object::effective_name() const
{
	if (m_name.empty())
		return m_name;
	if (m_name[0] == '%')
		return m_name.substr(1);
	return get_root() + encode(m_name.c_str());
}

} // namespace detail
} // namespace boost

