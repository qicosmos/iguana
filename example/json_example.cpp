#include <iguana/json_reader.hpp>
#include <iguana/json_writer.hpp>

//#include <boost/timer.hpp>

namespace client {
struct person {
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
} // namespace client

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

// void test_tuple()
//{
//	client::person p = { "zombie chow", -311 };
//	std::tuple<int, std::string, double, client::person> tp(20, "tom", 2.5,
// p); 	iguana::string_stream ss; 	iguana::to_json(tp, ss);
//
//	auto json_str = ss.str();
//	std::cout << json_str << std::endl;
//
//	std::tuple<int, std::string, double, client::person> tp1;
//	iguana::from_json(tp1, json_str.data(), json_str.length());
//	std::cout << std::get<1>(tp1) << '\n';
//
//	{
//		std::tuple<int, std::string, std::vector<int>> tp(20, "tom",
//{1,2,3}); 		iguana::string_stream ss;
// iguana::to_json(tp, ss);
//
//		auto json_str = ss.str();
//		std::cout << json_str << std::endl;
//
//		std::tuple<int, std::string, std::vector<int>> tp1;
//		iguana::from_json(tp1, json_str.data(),
// json_str.length());
//	}
// }

// void compare_speed(){
//	std::string s1 = "{\"name\":\"zombie chow\",\"age\":-311}";
//	std::string s2 = "{\"age\":-311,\"name\":\"zombie chow\"}";
//
//	client::person p2;
//	const size_t Size = 1000000;
//
//	boost::timer t;
//	for (auto i = 0; i < Size; ++i) {
//		iguana::from_json(p2, s1.data(), s1.length()); //the
// sequence must be limited
//	}
//	std::cout<<t.elapsed()<<'\n';
//	t.restart();
//
//	for (auto i = 0; i < Size; ++i) {
//		iguana::from_json(p2, s2.data(), s2.length()); //no
// limitation, but slower
//	}
//	std::cout<<t.elapsed()<<'\n';
// }

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

  return 0;
}
