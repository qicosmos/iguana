#pragma once
#include "iguana/reflection.hpp"

struct MyClass1 {
  double member0;
  double member1;
  double member2;
  double member3;

  bool operator==(MyClass1 const &rhs) const {
    return member0 == rhs.member0 and member1 == rhs.member1 and
           member2 == rhs.member2 and member3 == rhs.member3;
  }
};
REFLECTION(MyClass1, member0, member1, member2, member3);

struct MyClass2 {
  unsigned member_unsigned0;
  unsigned member_unsigned1;
  signed member_signed;

  bool operator==(MyClass2 const &rhs) const {
    return member_unsigned0 == rhs.member_unsigned0 and
           member_unsigned1 == rhs.member_unsigned1 and
           member_signed == rhs.member_signed;
  }
};
REFLECTION(MyClass2, member_unsigned0, member_unsigned1, member_signed);