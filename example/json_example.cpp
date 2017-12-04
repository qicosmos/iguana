#include <iguana/json.hpp>
//#include <boost/timer.hpp>

namespace client
{
	struct person
	{
		std::string	name;
		int64_t		 age;
	};

	REFLECTION(person, name, age);
}

struct MyStruct
{
	uint64_t a;
};
REFLECTION(MyStruct, a);

void test()
{
	MyStruct p = { 5566777755311 }; 
	iguana::string_stream ss; 
	iguana::json::to_json(ss, p); 
	auto json_str = ss.str(); 
	std::cout << json_str << std::endl; 
	MyStruct p2; 
	iguana::json::from_json(p2, json_str.data(), json_str.length()); 
	std::cout << p2.a << std::endl;
}

//void test_tuple()
//{
//	client::person p = { "zombie chow", -311 };
//	std::tuple<int, std::string, double, client::person> tp(20, "tom", 2.5, p);
//	iguana::string_stream ss;
//	iguana::json::to_json(ss, tp);
//
//	auto json_str = ss.str();
//	std::cout << json_str << std::endl;
//
//	std::tuple<int, std::string, double, client::person> tp1;
//	iguana::json::from_json(tp1, json_str.data(), json_str.length());
//	std::cout << std::get<1>(tp1) << '\n';
//
//	{
//		std::tuple<int, std::string, std::vector<int>> tp(20, "tom", {1,2,3});
//		iguana::string_stream ss;
//		iguana::json::to_json(ss, tp);
//
//		auto json_str = ss.str();
//		std::cout << json_str << std::endl;
//
//		std::tuple<int, std::string, std::vector<int>> tp1;
//		iguana::json::from_json(tp1, json_str.data(), json_str.length());
//	}
//}

//void compare_speed(){
//	std::string s1 = "{\"name\":\"zombie chow\",\"age\":-311}";
//	std::string s2 = "{\"age\":-311,\"name\":\"zombie chow\"}";
//
//	client::person p2;
//	const size_t Size = 1000000;
//
//	boost::timer t;
//	for (auto i = 0; i < Size; ++i) {
//		iguana::json::from_json(p2, s1.data(), s1.length()); //the sequence must be limited
//	}
//	std::cout<<t.elapsed()<<'\n';
//	t.restart();
//
//	for (auto i = 0; i < Size; ++i) {
//		iguana::json::from_json0(p2, s2.data(), s2.length()); //no limitation, but slower
//	}
//	std::cout<<t.elapsed()<<'\n';
//}

int main(void)
{
	test();
	client::person p = { "zombie chow", -311 };
	iguana::string_stream ss;
	iguana::json::to_json(ss, p);

	auto json_str = ss.str();
	std::cout << json_str << std::endl;

	client::person p2;

	iguana::json::from_json(p2, json_str.data(), json_str.length()); //the sequence must be limited
    iguana::json::from_json0(p2, json_str.data(), json_str.length()); //no limitation, but slower

	std::cout << p2.name << " - " << p2.age << std::endl;
	return 0;
}
