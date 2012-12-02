// (C) Copyright 2010 Just Software Solutions Ltd http://www.justsoftwaresolutions.co.uk
// (C) Copyright 2012 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_SYNCHRONIZED_VALUE_HPP
#define BOOST_THREAD_SYNCHRONIZED_VALUE_HPP

#include <boost/thread/detail/config.hpp>
#if ! defined BOOST_NO_CXX11_RVALUE_REFERENCES

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_types.hpp>
#include <boost/thread/lock_guard.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{
  /**
   *
   */
  template <typename T, typename Lockable = mutex>
  class synchronized_value
  {
  public:
    typedef T value_type;
    typedef Lockable lockable_type;
  private:
    T value_;
    mutable lockable_type mtx_;
  public:
    synchronized_value()
    : value_()
    {
    }

    synchronized_value(T other)
    : value_(other)
    {
    }

    /**
     *
     */
    struct const_strict_synchronizer
    {
    protected:
      friend class synchronized_value;

      boost::unique_lock<lockable_type> lk_;
      T const& value_;

      explicit const_strict_synchronizer(synchronized_value const& outer) :
        lk_(outer.mtx_), value_(outer.value_)
      {
      }
    public:
      BOOST_THREAD_NO_COPYABLE( const_strict_synchronizer )

      const_strict_synchronizer(const_strict_synchronizer&& other)
      : lk_(boost::move(other.lk_)),value_(other.value_)
      {
      }

      ~const_strict_synchronizer()
      {
      }

      const T* operator->() const
      {
        return &value_;
      }

      const T& operator*() const
      {
        return value_;
      }

    };

    /**
     *
     */
    struct strict_synchronizer : const_strict_synchronizer
    {
    protected:
      friend class synchronized_value;

      explicit strict_synchronizer(synchronized_value& outer) :
        const_strict_synchronizer(const_cast<synchronized_value&>(outer))
      {
      }
    public:
      BOOST_THREAD_NO_COPYABLE( strict_synchronizer )

      strict_synchronizer(strict_synchronizer&& other)
      : const_strict_synchronizer(boost::move(other))
      {
      }

      ~strict_synchronizer()
      {
      }

      T* operator->()
      {
        return const_cast<T*>(&this->value_);
      }

      T& operator*()
      {
        return const_cast<T&>(this->value_);
      }

    };


    strict_synchronizer operator->()
    {
      return BOOST_THREAD_MAKE_RV_REF(strict_synchronizer(*this));
    }
    const_strict_synchronizer operator->() const
    {
      return BOOST_THREAD_MAKE_RV_REF(const_strict_synchronizer(*this));
    }

    strict_synchronizer synchronize()
    {
      return BOOST_THREAD_MAKE_RV_REF(strict_synchronizer(*this));
    }
    const_strict_synchronizer synchronize() const
    {
      return BOOST_THREAD_MAKE_RV_REF(const_strict_synchronizer(*this));
    }

    /**
     *
     */
    struct unique_synchronizer : unique_lock<lockable_type>
    {
    private:
      friend class synchronized_value;
      typedef unique_lock<lockable_type> base_type;

      T& value_;

      explicit unique_synchronizer(synchronized_value& outer)
      : base_type(outer.mtx_), value_(outer.value_)
      {
      }
    public:
      BOOST_THREAD_NO_COPYABLE(unique_synchronizer)

      unique_synchronizer(unique_synchronizer&& other):
      base_type(static_cast<base_type&&>(other)),value_(other.value_)
      {
      }

      ~unique_synchronizer()
      {
      }

      T* operator->()
      {
        if (this->owns_lock())
        return &value_;
        else
        return 0;
      }

      const T* operator->() const
      {
        if (this->owns_lock())
        return &value_;
        else
        return 0;
      }

      T& operator*()
      {
        BOOST_ASSERT (this->owns_lock());
        return value_;
      }

      const T& operator*() const
      {
        BOOST_ASSERT (this->owns_lock());
        return value_;
      }

    };

  private:
    class deref_value
    {
    private:
      friend class synchronized_value;

      boost::unique_lock<lockable_type> lk_;
      T& value_;

      explicit deref_value(synchronized_value& outer):
      lk_(outer.mtx_),value_(outer.value_)
      {}

    public:
      BOOST_THREAD_NO_COPYABLE(deref_value)
      deref_value(deref_value&& other):
      lk_(boost::move(other.lk_)),value_(other.value_)
      {}
      operator T()
      {
        return value_;
      }

      deref_value& operator=(T const& newVal)
      {
        value_=newVal;
        return *this;
      }
    };
    class const_deref_value
    {
    private:
      friend class synchronized_value;

      boost::unique_lock<lockable_type> lk_;
      const T& value_;

      explicit const_deref_value(synchronized_value const& outer):
      lk_(outer.mtx_), value_(outer.value_)
      {}

    public:
      BOOST_THREAD_NO_COPYABLE(const_deref_value)
      const_deref_value(const_deref_value&& other):
      lk_(boost::move(other.lk_)), value_(other.value_)
      {}

      operator T()
      {
        return value_;
      }
    };

  public:
    deref_value operator*()
    {
      return BOOST_THREAD_MAKE_RV_REF(deref_value(*this));
    }

    const_deref_value operator*() const
    {
      return BOOST_THREAD_MAKE_RV_REF(const_deref_value(*this));
    }

  };

}

#include <boost/config/abi_suffix.hpp>

#endif // BOOST_NO_CXX11_RVALUE_REFERENCES
#endif // header
