

#include <boost/thread.hpp>
#include <boost/config.hpp>

#ifndef BOOST_NO_RVALUE_REFERENCES
struct MovableButNonCopyable {
#if ! defined BOOST_NO_DELETED_FUNCTIONS
      MovableButNonCopyable(MovableButNonCopyable const&) = delete;
      MovableButNonCopyable& operator=(MovableButNonCopyable const&) = delete;
#else
private:
    MovableButNonCopyable(MovableButNonCopyable const&);
    MovableButNonCopyable& operator=(MovableButNonCopyable const&);
#endif
public:
    MovableButNonCopyable() {};
    MovableButNonCopyable(MovableButNonCopyable&&) {};
    MovableButNonCopyable& operator=(MovableButNonCopyable&&) {
      return *this;
    };
};
int main()
{
    boost::packaged_task<MovableButNonCopyable>(MovableButNonCopyable());
    return 0;
}
#else
int main()
{
    return 0;
}
#endif
