#include <iostream>
#include <iguana/json.hpp>
#include <iguana/xml.hpp>

struct person
{
	std::string  name;
	int          age;
};
REFLECTION(person, name, age)

struct one_t
{
	int id;
};
REFLECTION(one_t, id);

struct two
{
	std::string name;
	one_t one;
	int age;
};
REFLECTION(two, name, one, age);

struct composit_t
{
	int a;
	std::vector<std::string> b;
	int c;
	std::map<int, int> d;
	std::unordered_map<int, int> e;
	double f;
	std::list<one_t> g;
};
REFLECTION(composit_t, a, b, c, d, e, f, g);

void test_json()
{
	person p;
	const char * json = "{ \"name\" : \"tom\", \"age\" : 20}";
	auto r = iguana::json::from_json0(p, json);

	iguana::string_stream ss;
	iguana::json::to_json(ss, p);
	std::cout << ss.str() << std::endl;

	one_t one = { 2 };
	composit_t composit = { 1,{ "tom", "jack" }, 3,{ { 2,3 } },{ { 5,6 } }, 5.3,{ one } };
	iguana::string_stream sst;
	iguana::json::to_json(sst, composit);
	std::cout << sst.str() << std::endl;

//	const char* str_comp = R"({"a":1, "b":["tom", "jack"], "c":3, "d":{"2":3,"5":6},"e":{"3":4},"f":5.3,"g":[{"id":1},{"id":2}])";
	const char* str_comp = R"({"b":["tom", "jack"], "a":1, "c":3, "e":{"3":4}, "d":{"2":3,"5":6},"f":5.3,"g":[{"id":1},{"id":2}])";
	composit_t comp;
	iguana::json::from_json0(comp, str_comp);
	std::cout<<comp.a<<" "<<comp.f<<std::endl;
}

//void performance()
//{
//	person obj;
//	char * json = "{ \"Name\" : \"Boo\", \"Age\" : 28}";
//	ajson::load_from_buff(obj, json);
//
//	const size_t LEN = 1000000;
//
//	//�����л�
//	std::cout << "ajson deserialize: ";
//	boost::timer t;
//	for (size_t i = 0; i < LEN; i++)
//	{
//		ajson::load_from_buff(obj, json);
//	}
//	std::cout << t.elapsed() << std::endl;
//
//	std::cout << "reflib deserialize: ";
//	t.restart();
//	for (size_t i = 0; i < LEN; i++)
//	{
//		iguana::json::from_json(obj, json);
//	}
//	std::cout << t.elapsed() << std::endl;
//
//	//���л�
//	std::cout << "ajson serialize: ";
//	t.restart();
//	ajson::string_stream ss1;
//
//	for (size_t i = 0; i < LEN; i++)
//	{
//		ss1.clear();
//		ajson::save_to(ss1, obj);
//	}
//	std::cout << t.elapsed() << std::endl;
//
//	std::cout << "reflib serialize: ";
//	t.restart();
//
//	iguana::json::string_stream ss;
//	for (size_t i = 0; i < LEN; i++)
//	{
//		ss.clear();
//		iguana::json::to_json(ss, obj);
//	}
//	std::cout << t.elapsed() << std::endl;
//}

void test_xml()
{
	person p = {"admin", 20};
	iguana::string_stream ss;
	iguana::xml::to_xml(ss, p);
	std::cout << ss.str() << std::endl;

	ss.clear();
	two t = { "test", {2}, 4 };
	iguana::xml::to_xml(ss, t);
	auto result = ss.str();
	std::cout << result << std::endl;

	std::string xml = "			<?xml version=\"1.0\" encoding=\"UTF-8\">  <name>buke</name> <one><id>1</id></one>  <age>2</age>";
	two t1;
	iguana::xml::from_xml(t1, xml.data(), xml.length());
}

int main()
{
	test_json();
	test_xml();
}