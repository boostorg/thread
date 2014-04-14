//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
// Copyright (C) 2011 Vicente J. Botet Escriba
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

// <boost/thread/permit>

// class permit_c_any;

// permit_c_any& operator=(const permit_c_any&) = delete;

#include <boost/thread/permit.hpp>

void fail()
{
  boost::permit_c_any cv0;
  boost::permit_c_any cv1;
  cv1 = cv0;
}

#include "../../../remove_error_code_unused_warning.hpp"

