#pragma once

#include <string>

#include "iguana/ylt/reflection/user_reflect_macro.hpp"

struct person {
  std::string name;
  int age;
};
YLT_REFL(person, name, age);
