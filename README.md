# A Universal Serialization Engine Based on compile-time Reflection #

*iguana* is a modern, universal and easy-to-use serialization engine developed in c++20.

| OS (Compiler Version)                          | Status                                                                                                   |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| Ubuntu 22.04 (clang 14.0.0)                    | ![win](https://github.com/qicosmos/iguana/actions/workflows/linux-clang.yml/badge.svg?branch=master) |
| Ubuntu 22.04 (gcc 11.2.0)                      | ![win](https://github.com/qicosmos/iguana/actions/workflows/linux-gcc.yml/badge.svg?branch=master)   |
| macOS Monterey 12 (AppleClang 14.0.0.14000029) | ![win](https://github.com/qicosmos/iguana/actions/workflows/mac.yml/badge.svg?branch=master)         |
| Windows Server 2022 (MSVC 19.33.31630.0)       | ![win](https://github.com/qicosmos/iguana/actions/workflows/windows.yml/badge.svg?branch=master)     |

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

	iguana::string_stream ss; // here use std::string is also ok
	iguana::to_json(p, ss);

	std::cout << ss.str() << std::endl; 
This example will output:

	{"name":"tom","age":28}
Serializing person to `json` string is also very simple, just need to call `to_json` method, there is nothing more.

How about deserialization of `json`? Look at the follow example.

	std::string json = "{ \"name\" : \"tom\", \"age\" : 28}";

	person p;
	iguana::from_json(p, json);
It's as simple as serialization, just need to call `from_json` method. 

You can also use parse interface to do dom parsing:
```c++
    std::string_view str = R"(false)";
    iguana::jvalue val;
    iguana::parse(val, str.begin(), str.end());

    std::error_code ec;
    auto b = val.get<bool>(ec);
    CHECK(!ec);
    CHECK(!b);
    
    // or
    b = val.get<bool>(); // this interface maybe throw exception
    CHECK(!b);
```

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
	iguana::to_json(composit, ss);
	std::cout << ss.str() << std::endl;

	std::string str_comp = R"({"a":1, "b":["tom", "jack"], "c":3, "d":{"2":3,"5":6},"e":{"3":4},"f":5.3,"g":[{"id":1},{"id":2}])";
	composit_t comp;
	iguana::from_json(comp, str_comp);
	
### How to solve the problem of unicode path in a json file?

If there is an unicode string as a path in a json file, however iguana parse the file as utf-8, so maybe you can see some strange characters after parse.

It's ok, because you see the utf-8 strings. The problem is you can't use the string directly, such as use std::ifstream to open the file with the unicode string path.

We can slove the problem1 easily with c++17:

```
	//the p.path is a unicode string path
	std::ifstream in(std::filesystem::u8path(p.path)); //std::filesystem::u8path help us
	//now you can operate the file
```		
	
### Full sources:


- https://github.com/qicosmos/iguana/blob/master/example/example.cpp

### Scripts

Automatically generate `REFLECTION` macros based by struct.

To get a list of basic options and switches use:
```
python automatic_macro_generator.py -h
```

basic example:

The content of the test_macro_generator.cpp is as follows:

	struct person {
		std::string name;
		int age;
	};

	char *iguana = NULL;

	struct composit_t { int a; std::vector<std::string> b; int c; std::map<int, int> d; std::unordered_map<int, int> e; double f;};

	char *iguana_test = NULL;

	struct composit_t2
	{
		int a;
		std::vector<std::string> b;
		int iguana;
		std::map<int, int> example_test;
		std::unordered_map<int, int> random_name__;
		double __f__number__complex;
	};


execute script:
```
python automatic_macro_generator.py -i test_macro_generator.cpp
```

After processing by the automatic_macro_generator.py script,test_macro_generator.cpp change into:

	struct person {
		std::string name;
		int age;
	};
	REFLECTION(person, name, age);
	char *iguana = NULL;

	struct composit_t { int a; std::vector<std::string> b; int c; std::map<int, int> d; std::unordered_map<int, int> e; double f;};
	REFLECTION(composit_t, a, b, c, d, e, f);

	struct composit_t2
	{
		int a;
		std::vector<std::string> b;
		int iguana;
		std::map<int, int> example_test;
		std::unordered_map<int, int> random_name__;
		double __f__number__complex;
	};
	REFLECTION(composit_t2, a, b, iguana, example_test, random_name__, __f__number__complex);

other example:
```
python automatic_macro_generator.py -i test_macro_generator.cpp -o have_macro.cpp
```
test_macro_generator.cpp will be unchanged, have_macro.cpp will be changed to source file with REFLECTION macro.

scripts works out of the box with Python version 2.7 and 3.x on any platform.

Notes: In Python3,Will prompt `DeprecationWarning: 'U' mode is deprecated`.Ignore it.


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
1. Support C++20
2. Refactor json reader, modification based on glaze  [json/read.hpp](https://github.com/stephenberry/glaze/blob/main/include/glaze/json/read.hpp)
