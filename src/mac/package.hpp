// (C) Copyright Mac Murrett 2001.
// Use, modification and distribution are subject to the 
// Boost Software License, Version 1.0. (See accompanying file 
// LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// See http://www.boost.org for most recent version.

#ifndef BOOST_PACKAGE_MJM012402_HPP
#define BOOST_PACKAGE_MJM012402_HPP


namespace boost {

namespace threads {

namespace mac {

namespace detail {


class base_package: private noncopyable
{
  public:
    virtual void accept() = 0;
};

template<class R>
class package: public base_package
{
  public:
    inline package(function<R> &rFunctor):
        m_rFunctor(rFunctor)
        {    /* no-op */                }
    inline ~package()
        {    /* no-op */                }

    virtual void accept()
        {    m_oR = m_rFunctor();    }
    inline R return_value()
        {    return(m_oR);            }

  private:
    function<R> &m_rFunctor;
    R m_oR;
};

template<>
class package<void>: public base_package
{
  public:
    inline package(function<void> &rFunctor):
        m_rFunctor(rFunctor)
        {    /* no-op */                }
    inline ~package()
        {    /* no-op */                }

    virtual void accept()
        {    m_rFunctor();            }
    inline void return_value()
        {    return;                    }

  private:
    function<void> &m_rFunctor;
};


} // namespace detail

} // namespace mac

} // namespace threads

} // namespace boost


#endif // BOOST_PACKAGE_MJM012402_HPP
