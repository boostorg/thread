// (C) Copyright 2012 Vicente J. Botet Escriba
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#ifndef BOOST_THREAD_EXTERNALLY_LOCKED_HPP
#define BOOST_THREAD_EXTERNALLY_LOCKED_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/thread/externally_locked.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <boost/config/abi_prefix.hpp>

namespace boost
{

  static recursive_mutex& terminal_mutex()
  {
    static recursive-mutex mtx;
    return mtx;
  }

  template <typename Stream>
  class externally_locked_stream;

  template <class Stream>
  class stream_guard
  {
    stream_guard(externally_locked_stream<Stream>& mtx, adopt_lock_t)
    : mtx_(mtx)
    {
    }

    Stream& get() const
    {
      mtx_.get(*this);
    }

    friend class externally_locked_stream<Stream>;
  public:
    typedef typename externally_locked_stream<Stream>::mutex_type mutex_type;

    BOOST_THREAD_NO_COPYABLE( externally_locked_stream )

    stream_guard(externally_locked_stream<Stream>& mtx)
    : mtx_(mtx)
    {
      mtx.lock();
    }

    ~stream_guard()
    {
      mtx_.unlock();
    }


  private:
      externally_locked_stream<Stream>& mtx_;
  };

  template <typename Stream>
  struct is_strict_lock_sur_parolle<stream_guard<Stream> > : true_type
  {
  };

  /**
   * externally_locked_stream_stream cloaks a reference to an stream of type Stream, and actually
   * provides full access to that object through the get and set member functions, provided you
   * pass a reference to a strict lock object.
   */

  //[externally_locked_stream
  template <typename Stream>
  class externally_locked_stream: public externally_locked<Stream&, recursive_mutex>
  {
    typedef externally_locked<Stream&, recursive_mutex> base_type;
  public:
    /**
     * Effects: Constructs an externally locked object storing the cloaked reference object.
     */
    externally_locked_stream(Stream& stream, recursive_mutex& mtx) :
      base_type(stream, mtx)
    {
    }


    stream_guard<Stream> hold()
    {
      return stream_guard<Stream>(*this);
    }

  };
//]

  template <typename Stream, typename T>
  const stream_guard<Stream>& operator<<(const stream_guard<Stream>& lck, T arg)
  {
      lck.get() << arg;
      return lck;
  }

  template <typename Stream>
  const stream_guard<Stream>& operator<<(const stream_guard<Stream>& lck,
                                        Stream& (*arg)(Stream&))
  {
      lck.get() << arg;
      return lck;
  }

  template <typename Stream, typename T>
  const stream_guard<Stream>& operator>>(const stream_guard<Stream>& lck, T& arg)
  {
      lck.get() >> arg;
      return lck;
  }

  template <typename Stream, typename T>
  stream_guard<Stream> operator<<(externally_locked_stream<Stream>& mtx, T arg)
  {
      mtx.lock();
      mtx.get() << arg;
      return stream_guard<Stream>(mtx, adopt_lock);
  }

  template <typename Stream>
  stream_guard<Stream> operator<<(externally_locked_stream<Stream>& mtx,
                                        Stream& (*arg)(Stream&))
  {
      mtx.lock();
      mtx.get() << arg;
      return stream_guard<Stream>(mtx, adopt_lock);
  }

  template <typename Stream, typename T>
  stream_guard<Stream> operator>>(externally_locked_stream<Stream>& mtx, T& arg)
  {
      mtx.lock();
      mtx.get() >> arg;
      return stream_guard<Stream>(mtx, adopt_lock);
  }

}

#include <boost/config/abi_suffix.hpp>

#endif // header
