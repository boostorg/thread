// (C) Copyright 2012 Vicente Botet
//
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//#define BOOST_THREAD_VERSION 4

// There is yet a limitation when BOOST_THREAD_PROVIDES_FUTURE_INVALID_AFTER_GET is defined
#define BOOST_THREAD_DONT_PROVIDE_FUTURE_INVALID_AFTER_GET

#include <iostream>
#include <string>
#include <boost/thread/synchronized_value.hpp>

#if ! defined BOOST_NO_CXX11_RVALUE_REFERENCES && ! defined BOOST_NO_CXX11_AUTO


//class SafePerson {
//public:
//  std::string GetName() const {
//    const_unique_access<std::string> name(nameGuard);
//    return *name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<std::string> name(nameGuard);
//    *name = newName;
//  }
//private:
//  unique_access_guard<std::string> nameGuard;
//};

class SafePerson {
public:
  std::string GetName() const {
    return *name;
  }
  void SetName(const std::string& newName)  {
    *name = newName;
  }

private:
  boost::synchronized_value<std::string> name;
};

class Person {
public:
  std::string GetName() const {
    return name;
  }
  void SetName(const std::string& newName) {
    name = newName;
  }
private:
  std::string name;
};
typedef boost::synchronized_value<Person> Person_ts;


//class SafeMemberPerson {
//public:
//  SafeMemberPerson(unsigned int age) :
//    memberGuard(age)
//  {  }
//  std::string GetName() const  {
//    const_unique_access<Member> member(memberGuard);
//    return member->name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<Member> member(memberGuard);
//    member->name = newName;
//  }
//private:
//  struct Member
//  {
//    Member(unsigned int age) :
//      age(age)
//    {    }
//    std::string name;
//    unsigned int age;
//  };
//  unique_access_guard<Member> memberGuard;
//};

class SafeMemberPerson {
public:
  SafeMemberPerson(unsigned int age) :
    member(Member(age))
  {  }
  std::string GetName() const  {
    return member->name;
  }
  void SetName(const std::string& newName)  {
    member->name = newName;
  }
private:
  struct Member  {
    Member(unsigned int age) :
      age(age)
    {    }
    std::string name;
    unsigned int age;
  };
  boost::synchronized_value<Member> member;
};


class Person2 {
public:
  Person2(unsigned int age) : age_(age)
  {}
  std::string GetName() const  {
    return name_;
  }
  void SetName(const std::string& newName)  {
    name_ = newName;
  }
  unsigned int GetAge() const  {
    return age_;
  }

private:
  std::string name_;
  unsigned int age_;
};
typedef boost::synchronized_value<Person2> Person2_ts;

//===================

//class HelperPerson {
//public:
//  HelperPerson(unsigned int age) :
//    memberGuard(age)
//  {  }
//  std::string GetName() const  {
//    const_unique_access<Member> member(memberGuard);
//    Invariant(member);
//    return member->name;
//  }
//  void SetName(const std::string& newName)  {
//    unique_access<Member> member(memberGuard);
//    Invariant(member);
//    member->name = newName;
//  }
//private:
//  void Invariant(const_unique_access<Member>& member) const  {
//    if (member->age < 0) throw std::runtime_error("Age cannot be negative");
//  }
//  struct Member  {
//    Member(unsigned int age) :
//      age(age)
//    {    }
//    std::string name;
//    unsigned int age;
//  };
//  unique_access_guard<Member> memberGuard;
//};

class HelperPerson {
public:
  HelperPerson(unsigned int age) :
    member(age)
  {  }
  std::string GetName() const  {
    auto&& memberSync = member.synchronize();
    Invariant(memberSync);
    return memberSync->name;
  }
  void SetName(const std::string& newName)  {
    auto&& memberSync = member.synchronize();
    Invariant(memberSync);
    memberSync->name = newName;
  }
private:
  struct Member  {
    Member(unsigned int age) :
      age(age)
    {    }
    std::string name;
    unsigned int age;
  };
  void Invariant(boost::synchronized_value<Member>::const_strict_synchronizer & mbr) const
  {
    if (mbr->age < 1) throw std::runtime_error("Age cannot be negative");
  }
  boost::synchronized_value<Member> member;
};

class Person3 {
public:
  Person3(unsigned int age) :
    age_(age)
  {  }
  std::string GetName() const  {
    Invariant();
    return name_;
  }
  void SetName(const std::string& newName)  {
    Invariant();
    name_ = newName;
  }
private:
  std::string name_;
  unsigned int age_;
  void Invariant() const  {
    if (age_ < 1) throw std::runtime_error("Age cannot be negative");
  }
};

typedef boost::synchronized_value<Person3> Person3_ts;

int main()
{
  {
    SafePerson p;
    p.SetName("Vicente");
  }
  {
    Person_ts p;
    p->SetName("Vicente");
  }
  {
    SafeMemberPerson p(1);
    p.SetName("Vicente");
  }
  {
    Person2_ts p(1);
    p->SetName("Vicente");
  }
  {
    HelperPerson p(1);
    p.SetName("Vicente");
  }
  {
    Person3_ts p(1);
    p->SetName("Vicente");
  }
  return 0;
}

#else

int main()
{
  return 0;
}
#endif
