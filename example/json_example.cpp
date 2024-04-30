#include <charconv>  // for to_chars
#include <iostream>  // for std::cout

namespace iguana {

template <typename T>
inline char* to_chars_float(T value, char* buffer) {
  std::cout << "call custom to_chars_float with snprintf\n";
  return buffer + snprintf(buffer, 65, "%g", value);
}

}  // namespace iguana

#include <iguana/json_reader.hpp>
#include <iguana/json_writer.hpp>

//#include <boost/timer.hpp>

namespace client {
struct person {
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
}  // namespace client

struct MyStruct {
  uint64_t a;
};
REFLECTION(MyStruct, a);

struct student {
  int id;
  std::string name;
  int age;
};
REFLECTION(student, id, name, age);

void test() {
  MyStruct p = {5566777755311};
  iguana::string_stream ss;
  iguana::to_json(p, ss);

  MyStruct p2;
  iguana::from_json(p2, ss);
  std::cout << p2.a << std::endl;
}

void test_v() {
  client::person p1 = {"tom", 20};
  client::person p2 = {"jack", 19};
  client::person p3 = {"mike", 21};

  std::vector<client::person> v{p1, p2, p3};
  iguana::string_stream ss;
  iguana::to_json(v, ss);
  std::cout << ss << std::endl;

  std::vector<client::person> v1;
  iguana::from_json(v1, ss);
}

void test_disorder() {
  student s{1, "tom", 20};
  iguana::string_stream ss;
  iguana::to_json(s, ss);
  std::cout << ss << std::endl;

  student s1{};
  std::string str = "{\"name\":\"tom\",\"id\":1,\"age\":20}";
  iguana::from_json(s1, str.data(), str.length());
  std::string str1 = "{\"name\":\"tom\",\"age\":20,\"id\":1}";
  iguana::from_json(s1, str1.data(), str1.length());

  std::string str2 = "{ \"id\":1,\"name\" : \"madoka\",\"age\" : 27 }";
  iguana::from_json(s1, str2.data(), str2.length());
}

struct book_t {
  std::string_view title;
  std::string_view edition;
  std::vector<std::string_view> author;
};
REFLECTION(book_t, title, edition, author);

void test_str_view() {
  {
    std::string str = R"({
    "title": "C++ templates",
    "edition": "invalid number",
    "author": [
      "David Vandevoorde",
      "Nicolai M. Josuttis"
    ]})";
    book_t b;
    iguana::from_json(b, str);
    std::cout << b.title << std::endl;
    std::cout << b.edition << std::endl;
    std::cout << b.author[0] << std::endl;
    std::cout << b.author[1] << std::endl;

    std::string ss;
    iguana::to_json(b, ss);
    std::cout << "to_json\n" << ss << "\n";
  }
}

namespace my_space {
struct my_struct {
  int x, y, z;
  bool operator==(const my_struct& o) const {
    return x == o.x && y == o.y && z == o.z;
  }
};

template <bool Is_writing_escape, typename Stream>
inline void to_json_impl(Stream& s, const my_struct& t) {
  iguana::to_json(*(int(*)[3]) & t, s);
}

template <typename It>
IGUANA_INLINE void from_json_impl(my_struct& value, It&& it, It&& end) {
  iguana::from_json(*(int(*)[3]) & value, it, end);
}

}  // namespace my_space

struct nest {
  std::string name;
  my_space::my_struct value;
  bool operator==(const nest& o) const {
    return name == o.name && value == o.value;
  }
};

REFLECTION(nest, name, value);

void user_defined_struct_example() {
  {
    my_space::my_struct v{1, 2, 3}, v2;
    std::string s;
    iguana::to_json(v, s);
    std::cout << s << std::endl;
    iguana::from_json(v2, s);
    assert(v == v2);
  }
  {
    nest v{"Hi", {1, 2, 3}}, v2;
    std::string s;
    iguana::to_json(v, s);
    std::cout << s << std::endl;
    iguana::from_json(v2, s);
    assert(v == v2);
  }
}

struct test_float_t {
  double a;
  float b;
};
REFLECTION(test_float_t, a, b);

void user_defined_tochars_example() {
  test_float_t t{2.011111, 2.54};
  std::string ss;
  iguana::to_json(t, ss);
  std::cout << ss << std::endl;
}

int main(void) {
  test_disorder();
  test_v();
  test();
  client::person p = {"zombie chow", -311};
  iguana::string_stream ss;
  iguana::to_json(p, ss);

  std::cout << ss << std::endl;
  client::person p2;

  iguana::from_json(p2, ss.data(), ss.length());

  std::cout << p2.name << " - " << p2.age << std::endl;
  test_str_view();

  user_defined_struct_example();
  user_defined_tochars_example();
  return 0;
}
