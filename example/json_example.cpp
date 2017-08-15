#include <iguana/json.hpp>

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

void test_tuple()
{
	client::person p = { "zombie chow", -311 };
	std::tuple<int, std::string, double, client::person> tp(20, "tom", 2.5, p);
	iguana::string_stream ss;
	iguana::json::to_json(ss, tp);

	auto json_str = ss.str();
	std::cout << json_str << std::endl;

	std::tuple<int, std::string, double, client::person> tp1;
	iguana::json::from_json(tp1, json_str.data(), json_str.length());
	std::cout << std::get<1>(tp1) << '\n';

	{
		std::tuple<int, std::string, std::vector<int>> tp(20, "tom", {1,2,3});
		iguana::string_stream ss;
		iguana::json::to_json(ss, tp);

		auto json_str = ss.str();
		std::cout << json_str << std::endl;

		std::tuple<int, std::string, std::vector<int>> tp1;
		iguana::json::from_json(tp1, json_str.data(), json_str.length());
	}
}

int main(void)
{
	test_tuple();
	test();
	client::person p = { "zombie chow", -311 };
	iguana::string_stream ss;
	iguana::json::to_json(ss, p);

	auto json_str = ss.str();
	std::cout << json_str << std::endl;

	client::person p2;
	iguana::json::from_json(p2, json_str.data(), json_str.length());

	std::cout << p2.name << " - " << p2.age << std::endl;
	return 0;
}
