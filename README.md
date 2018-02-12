# A Universal Serialization Engine Based on compile-time Reflection #

*iguana* is a modern, universal and easy-to-use serialization engine developed in c++17.
### Motivation ###
Serialize an object to any other format data with compile-time reflection, such as json, xml, binary, table and so on.
This library is designed to unify and simplify serialization in a portable cross-platform manner. This library is also easy to extend, and you can serialize any format of data with the library.
This library provides a portable cross-platform way of: 

- serialization of json
- serialization of xml
- serialization of any customized format

### Tutorial ###
This Tutorial is provided to give you a view of how *iguana* works for serialization. 

### Serialization of json

The first thing to do when you serialize an object is to define meta data.  There is an example of defining meta data.

	struct person
	{
		std::string  name;
		int          age;
	};

	REFLECTION(person, name, age) //define meta data
Defining meta data is very simple, and just needs to define in a `REFLECTION` macro.

Now let's serialize `person` to `json` string.

	person p = { "tom", 28 };

	iguana::string_stream ss;
	iguana::json::to_json(ss, p);

	std::cout << ss.str() << std::endl; 
This example will output:

	{"name":"tom","age":28}
Serializing person to `json` string is also very simple, just need to call `to_json` method, there is nothing more.

How about deserialization of `json`? Look at the follow example.

	const char * json = "{ \"name\" : \"tom\", \"age\" : 28}";

	person p;
	iguana::json::from_json(p, json);
It's as simple as serialization, just need to call `from_json` method.

### Serialization of xml
Serialization of `xml` is similar to `json`. The first step is also defining meta data as above. This is a complete example.

	person p = {"admin", 20};

	iguana::string_stream ss;
	iguana::xml::to_xml(ss, p);

	std::cout << ss.str() << std::endl;

	std::string xml = "<?xml version=\"1.0\" encoding=\"UTF-8\">  <name>buke</name> <id>1</id>";
	iguana::xml::from_xml(p, xml.data(), xml.length());

### A complicated example
*iguana* can deal with objects which contain another objects and containers. Here is the example:

At first, we define the meta data:

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

Then call the simple interface:

	one_t one = { 2 };
	composit_t composit = { 1,{ "tom", "jack" }, 3,{ { 2,3 } },{ { 5,6 } }, 5.3,{ one } };
	iguana::string_stream ss;
	iguana::json::to_json(ss, composit);
	std::cout << ss.str() << std::endl;

	const char* str_comp = R"({"a":1, "b":["tom", "jack"], "c":3, "d":{"2":3,"5":6},"e":{"3":4},"f":5.3,"g":[{"id":1},{"id":2}])";
	composit_t comp;
	iguana::json::from_json(comp, str_comp);
	
### Full sources:


- https://github.com/qicosmos/iguana/blob/master/example/example.cpp

### F.A.Q

- **Question**: Why is the library called *iguana*?

	- **Answer**: I think serialization is like an iguana, because the only difference is the displaying format, however the meta data is never changed. With changeless meta data and reflection, you can serialize an object to any format, which is like how an iguana does.
	
- **Question**: Does *iguana* support raw pointer?

	- **Answer**: No. *iguana* doesn't support raw pointer, but it will support smart pointer in the future.
	

- **Question**: Is iguana thread-safe?

	- **Answer**: Not yet, but it's not a problem, you can use `lock` before calling `from_json` or `to_json`.


- **Question**: Is *iguana* high performance?

	- **Answer**: Yes, it is, because *iguana* is based on compile-time reflection.

- **Question**: I found a bug, how could I report?
    - **Answer**: Create an issue on [GitHub](https://github.com/qicosmos/iguana) with a detailed description. 


### Update
1. Support C++17
2. Support disorderly parse json, a new interface from_json0 do this, however it is slower than from_json.
