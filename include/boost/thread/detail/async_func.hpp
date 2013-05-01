// Copyright (C) 2012 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// 2013/04 Vicente J. Botet Escriba
//    Provide implementation up to 9 parameters when BOOST_NO_CXX11_VARIADIC_TEMPLATES is defined.
//    Make use of Boost.Move
//    Make use of Boost.Tuple (movable)
// 2012/11 Vicente J. Botet Escriba
//    Adapt to boost libc++ implementation

//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
// The async_func code is based on the one from libcxx.
//===----------------------------------------------------------------------===//

#ifndef BOOST_THREAD_DETAIL_ASYNC_FUNCT_HPP
#define BOOST_THREAD_DETAIL_ASYNC_FUNCT_HPP

#include <boost/config.hpp>

#include <boost/utility/result_of.hpp>
#include <boost/thread/detail/move.hpp>
#include <boost/thread/detail/invoke.hpp>
#include <boost/thread/detail/make_tuple_indices.hpp>


#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined(BOOST_NO_CXX11_HDR_TUPLE)
#include <tuple>
#else
#include <boost/tuple/tuple.hpp>
#endif

namespace boost
{
  namespace detail
  {

#if ! defined(BOOST_NO_CXX11_VARIADIC_TEMPLATES) && \
    ! defined(BOOST_NO_CXX11_HDR_TUPLE)

    template <class Fp, class... Args>
    class async_func
    {
        std::tuple<Fp, Args...> f_;

    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        //typedef typename invoke_of<_Fp, _Args...>::type Rp;
        typedef typename result_of<Fp(Args...)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f, BOOST_THREAD_RV_REF(Args)... args)
            : f_(boost::move(f), boost::move(args)...) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<1+sizeof...(Args), 1>::type Index;
            return execute(Index());
        }
    private:
        template <size_t ...Indices>
        result_type
        execute(tuple_indices<Indices...>)
        {
            return invoke(boost::move(std::get<0>(f_)), boost::move(std::get<Indices>(f_))...);
        }
    };
    //BOOST_THREAD_DCL_MOVABLE_BEG(X) async_func<Fp> BOOST_THREAD_DCL_MOVABLE_END
#else
    template <class Fp,
      class T0 = tuples::null_type, class T1 = tuples::null_type, class T2 = tuples::null_type,
      class T3 = tuples::null_type, class T4 = tuples::null_type, class T5 = tuples::null_type,
      class T6 = tuples::null_type, class T7 = tuples::null_type, class T8 = tuples::null_type
      ,  class T9 = tuples::null_type>
    class async_func;

    template <class Fp,
      class T0 , class T1 , class T2 ,
      class T3 , class T4 , class T5 ,
      class T6 , class T7 , class T8 >
    class async_func<Fp, T0, T1, T2, T3, T4, T5, T6, T7, T8>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3, T4, T5, T6, T7, T8> f_;

    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6, T7, T8)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            , BOOST_THREAD_RV_REF(T4) a4
            , BOOST_THREAD_RV_REF(T5) a5
            , BOOST_THREAD_RV_REF(T6) a6
            , BOOST_THREAD_RV_REF(T7) a7
            , BOOST_THREAD_RV_REF(T8) a8
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        , boost::move(a4)
        , boost::move(a5)
        , boost::move(a6)
        , boost::move(a7)
        , boost::move(a8)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<10, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2,
        std::size_t I3, std::size_t I4, std::size_t I5,
        std::size_t I6, std::size_t I7, std::size_t I8
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7, I8>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
            , boost::move(boost::get<I4>(f_))
            , boost::move(boost::get<I5>(f_))
            , boost::move(boost::get<I6>(f_))
            , boost::move(boost::get<I7>(f_))
            , boost::move(boost::get<I8>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5, class T6, class T7 >
    class async_func<Fp, T0, T1, T2, T3, T4, T5, T6, T7>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3, T4, T5, T6, T7> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6, T7)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            , BOOST_THREAD_RV_REF(T4) a4
            , BOOST_THREAD_RV_REF(T5) a5
            , BOOST_THREAD_RV_REF(T6) a6
            , BOOST_THREAD_RV_REF(T7) a7
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        , boost::move(a4)
        , boost::move(a5)
        , boost::move(a6)
        , boost::move(a7)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<9, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2,
        std::size_t I3, std::size_t I4, std::size_t I5,
        std::size_t I6, std::size_t I7
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3, I4, I5, I6, I7>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
            , boost::move(boost::get<I4>(f_))
            , boost::move(boost::get<I5>(f_))
            , boost::move(boost::get<I6>(f_))
            , boost::move(boost::get<I7>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5, class T6>
    class async_func<Fp, T0, T1, T2, T3, T4, T5, T6>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3, T4, T5, T6> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5, T6)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            , BOOST_THREAD_RV_REF(T4) a4
            , BOOST_THREAD_RV_REF(T5) a5
            , BOOST_THREAD_RV_REF(T6) a6
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        , boost::move(a4)
        , boost::move(a5)
        , boost::move(a6)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<8, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5, std::size_t I6
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3, I4, I5, I6>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
            , boost::move(boost::get<I4>(f_))
            , boost::move(boost::get<I5>(f_))
            , boost::move(boost::get<I6>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4, class T5>
    class async_func<Fp, T0, T1, T2, T3, T4, T5>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3, T4, T5> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3, T4, T5)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            , BOOST_THREAD_RV_REF(T4) a4
            , BOOST_THREAD_RV_REF(T5) a5
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        , boost::move(a4)
        , boost::move(a5)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<7, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4, std::size_t I5
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3, I4, I5>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
            , boost::move(boost::get<I4>(f_))
            , boost::move(boost::get<I5>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2, class T3, class T4>
    class async_func<Fp, T0, T1, T2, T3, T4>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3, T4> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3, T4)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            , BOOST_THREAD_RV_REF(T4) a4
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        , boost::move(a4)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<6, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3, std::size_t I4
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3, I4>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
            , boost::move(boost::get<I4>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2, class T3>
    class async_func<Fp, T0, T1, T2, T3>
    {
        ::boost::tuple<Fp, T0, T1, T2, T3> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2, T3)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            , BOOST_THREAD_RV_REF(T3) a3
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        , boost::move(a3)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<5, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2, std::size_t I3
        >
        result_type
        execute(tuple_indices<I0, I1, I2, I3>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
            , boost::move(boost::get<I3>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1, class T2>
    class async_func<Fp, T0, T1, T2>
    {
        ::boost::tuple<Fp, T0, T1, T2> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1, T2)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            , BOOST_THREAD_RV_REF(T2) a2
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        , boost::move(a2)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<4, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1, std::size_t I2
        >
        result_type
        execute(tuple_indices<I0, I1, I2>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
            , boost::move(boost::get<I2>(f_))
                );
        }
    };
    template <class Fp, class T0, class T1>
    class async_func<Fp, T0, T1>
    {
        ::boost::tuple<Fp, T0, T1> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0, T1)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            , BOOST_THREAD_RV_REF(T1) a1
            )
            : f_(boost::move(f)
        , boost::move(a0)
        , boost::move(a1)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<3, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0, std::size_t I1
        >
        result_type
        execute(tuple_indices<I0, I1>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
            , boost::move(boost::get<I1>(f_))
                );
        }
    };
    template <class Fp, class T0>
    class async_func<Fp, T0>
    {
        ::boost::tuple<Fp, T0> f_;
    public:
        BOOST_THREAD_MOVABLE_ONLY(async_func)
        typedef typename result_of<Fp(T0)>::type result_type;

        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_RV_REF(Fp) f
            , BOOST_THREAD_RV_REF(T0) a0
            )
            : f_(boost::move(f)
        , boost::move(a0)
        ) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_RV_REF(async_func) f) : f_(boost::move(f.f_)) {}

        result_type operator()()
        {
            typedef typename make_tuple_indices<2, 1>::type Index;
            return execute(Index());
        }
    private:
        template <
        std::size_t I0
        >
        result_type
        execute(tuple_indices<I0>)
        {
            return invoke(boost::move(boost::get<0>(f_))
            , boost::move(boost::get<I0>(f_))
                );
        }
    };
    template <class Fp>
    class async_func<Fp>
    {
        Fp f_;
    public:
        BOOST_THREAD_COPYABLE_AND_MOVABLE(async_func)
        typedef typename result_of<Fp()>::type result_type;
        BOOST_SYMBOL_VISIBLE
        explicit async_func(BOOST_THREAD_FWD_REF(Fp) f)
            : f_(boost::move(f)) {}

        BOOST_SYMBOL_VISIBLE
        async_func(BOOST_THREAD_FWD_REF(async_func) f) : f_(boost::move(f.f_)) {}
        result_type operator()()
        {
            return execute();
        }
    private:
        result_type
        execute()
        {
            return f_();
        }
    };
#endif
//#else
//    template <class Fp>
//    class async_func
//    {
//        Fp f_;
//    public:
//        BOOST_THREAD_COPYABLE_AND_MOVABLE(async_func)
//        typedef typename result_of<Fp()>::type result_type;
//        BOOST_SYMBOL_VISIBLE
//        explicit async_func(BOOST_THREAD_FWD_REF(Fp) f)
//            : f_(boost::move(f)) {}
//
//        BOOST_SYMBOL_VISIBLE
//        async_func(BOOST_THREAD_FWD_REF(async_func) f) : f_(boost::move(f.f_)) {}
//        result_type operator()()
//        {
//            return execute();
//        }
//    private:
//        result_type
//        execute()
//        {
//            return f_();
//        }
//    };
//    //BOOST_THREAD_DCL_MOVABLE_BEG(Fp) async_func<Fp> BOOST_THREAD_DCL_MOVABLE_END
//#endif

  }
}

#endif // header
