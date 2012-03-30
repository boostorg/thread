#ifndef BOOST_THREAD_DETAIL_THREAD_INTERRUPTION_HPP
#define BOOST_THREAD_DETAIL_THREAD_INTERRUPTION_HPP
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2007-9 Anthony Williams
// (C) Copyright 2012 Vicente J. Botet Escriba

#include <boost/thread/detail/config.hpp>

namespace boost
{
    namespace this_thread
    {
        class BOOST_THREAD_DECL disable_interruption
        {
          bool interruption_was_enabled;
          friend class restore_interruption;
        public:
#ifndef BOOST_NO_DELETED_FUNCTIONS
            disable_interruption(const disable_interruption&) = delete;
            disable_interruption& operator=(const disable_interruption&) = delete;
#else
        private:
            disable_interruption(const disable_interruption&);
            disable_interruption& operator=(const disable_interruption&);
        public:
#endif
            disable_interruption() BOOST_NOEXCEPT;
            ~disable_interruption() BOOST_NOEXCEPT;
        };

        class BOOST_THREAD_DECL restore_interruption
        {
        public:
#ifndef BOOST_NO_DELETED_FUNCTIONS
            restore_interruption(const restore_interruption&) = delete;
            restore_interruption& operator=(const restore_interruption&) = delete;
#else
        private:
            restore_interruption(const restore_interruption&);
            restore_interruption& operator=(const restore_interruption&);
        public:
#endif

            explicit restore_interruption(disable_interruption& d) BOOST_NOEXCEPT;
            ~restore_interruption() BOOST_NOEXCEPT;
        };
    }
}

#endif
