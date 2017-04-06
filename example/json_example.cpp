#include <iguana/json.hpp>

namespace client
{
	struct person
	{
		std::string	name;
		int		    age;
	};

	REFLECTION(person, name, age);
}

int main(void)
{
	client::person p = { "zombie chow", 31 };

	iguana::string_stream ss;
	iguana::json::to_json(ss, p);

	auto json_str = ss.str();
	std::cout << json_str << std::endl;

	client::person p2;
	iguana::json::from_json(p2, json_str.data(), json_str.length());

	std::cout << p2.name << " - " << p2.age << std::endl;
	return 0;
}