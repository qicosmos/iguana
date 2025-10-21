# A Universal Serialization Engine Based on compile-time Reflection #

*iguana* is a modern, universal and easy-to-use serialization engine developed in C++20 and C++17.

| OS (Compiler Version)                          | Status                                                                                                   |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| Ubuntu 22.04 (clang 14.0.0)                    | ![win](https://github.com/qicosmos/iguana/actions/workflows/linux-clang.yml/badge.svg?branch=master) |
| Ubuntu 22.04 (gcc 11.2.0)                      | ![win](https://github.com/qicosmos/iguana/actions/workflows/linux-gcc.yml/badge.svg?branch=master)   |
| macOS Monterey latest (AppleClang latest) | ![win](https://github.com/qicosmos/iguana/actions/workflows/mac.yml/badge.svg?branch=master)         |
| Windows Server 2022 (MSVC 19.33.31630.0)       | ![win](https://github.com/qicosmos/iguana/actions/workflows/windows.yml/badge.svg?branch=master)     |

qq 交流群 701594518

[中文版](lang/iguana%20使用文档.md)

[struct_pb](lang/struct_pb_intro.md)

### Motivation ###
Serialize an object to any other format data with compile-time reflection, such as json, xml, binary, table and so on.
This library is designed to unify and simplify serialization in a portable cross-platform manner. This library is also easy to extend, and you can serialize any format of data with the library.
This library provides a portable cross-platform way of: 

- serialization of json
- serialization of xml
- serialization of yaml
- serialization of protobuf
- serialization of any customized format

### compile time reflection ###

[reflection lib introduction](lang/reflection_introduction.md)

### Tutorial ###
This Tutorial is provided to give you a view of how *iguana* works for serialization. 

### Serialization of json

The first thing to do when you serialize an object is to define meta data.  There is an example of defining meta data.

```c++
struct person
{
    std::string  name;
    int          age;
};
#if __cplusplus < 202002L
YLT_REFL(person, name, age) //define meta data
#endif
```

Defining meta data is very simple, if your compiler is C++20 compiler(gcc11+, clang13+, msvc2022), no need define YLT_REFL, other wise need to define in a `YLT_REFL` macro.

Now let's serialize `person` to `json` string.

```c++
person p = { "tom", 28 };

iguana::string_stream ss; // here use std::string is also ok
iguana::to_json(p, ss);

std::cout << ss.str() << std::endl; 
```

This example will output:

```bash
{"name":"tom","age":28}
```

Serializing person to `json` string is also very simple, just need to call `to_json` method, there is nothing more.

How about deserialization of `json`? Look at the follow example.

```c++
std::string json = "{ \"name\" : \"tom\", \"age\" : 28}";

person p;
iguana::from_json(p, json);
```

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

The serialization of `xml` is similar to `json`. The first step is also defining meta data as above, and then you can call `iguana::to_xml` to serialization  the structure, or call `iguana::from_xml` to deserialization  the structure. The following is a complete example.

```c++
// serialization the structure to the string
person p = {"admin", 20};
iguana::string_stream ss;  // here use std::string is also ok
iguana::to_xml(p, ss);
std::cout << ss << std::endl;

// deserialization the structure from the string
std::string xml = R"(
<?xml version=\"1.0\" encoding=\"UTF-8\">  
<root> 
  <name>buke</name> 
  <age>30</age> 
</root>)";
iguana::from_xml(p, xml);
```
#### Serialization of yaml

The serialization of `yaml` is also as simple as the above interface. Here is a complete example:

```c++
// serialization the structure to the string
person p = {"admin", 20};
iguana::string_stream ss;  // here use std::string is also ok
iguana::to_yaml(ss, p);
std::cout << ss.str() << std::endl;
std::string yaml = R"(
name : buke
age : 30
)";
// deserialization the structure from the string
iguana::from_yaml(p, yaml);
```

### A complicated example

#### json

*iguana* can deal with objects which contain another objects and containers. Here is the example:

At first, we define the meta data:

```c++
struct one_t
{
    int id;
};
YLT_REFL(one_t, id);

struct two
{
    std::string name;
    one_t one;
    int age;
};
YLT_REFL(two, name, one, age);

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
YLT_REFL(composit_t, a, b, c, d, e, f, g);
```

Then call the simple interface:

```c++
one_t one = { 2 };
composit_t composit = { 1,{ "tom", "jack" }, 3,{ { 2,3 } },{ { 5,6 } }, 5.3,{ one } };
iguana::string_stream ss;
iguana::to_json(composit, ss);
std::cout << ss.str() << std::endl;

std::string str_comp = R"({"a":1, "b":["tom", "jack"], "c":3, "d":{"2":3,"5":6},"e":{"3":4},"f":5.3,"g":[{"id":1},{"id":2}])";
composit_t comp;
iguana::from_json(comp, str_comp);
```

#### xml

At first, define the structure and reflect the meta data.

```c++
struct book_t {
  std::string author;
  float price;
};
YLT_REFL(book_t, author, price);
struct library_t {
  std::string name;
  int id;
  std::vector<book_t> book;
};
YLT_REFL(library_t, name, id, book);
```

And then, simply call the interface:

```c++
// serialization the structure to the string
library_t library = {"Pro Lib", 110, {{"tom", 1.8}, {"jack", 2.1}}};
std::string ss;
iguana::to_xml(library, ss);
std::cout << ss << "\n";

// deserialization the structure from the string
std::string str = R"(
<library>
  <name>Pro Lib</name>
  <id>110</id>
  <book><author>tom</author><price>1.8</price>
  </book>
  <book><author>jack</author><price>2.1</price>
  </book>
</library>
)";
library_t lib;
iguana::from_xml(lib, str);
```
#### yaml

As always what we do, define the structure and reflect the meta data.

```c++
enum class enum_status {
  start,
  stop,
};
struct plain_type_t {
  bool isok;
  enum_status status;
  char c;
  std::optional<bool> hasprice;
  std::optional<float> num;
  std::optional<int> price;
};
YLT_REFL(plain_type_t, isok, status, c, hasprice, num, price);
```

And then, simply call the interface:

```c++
// deserialization the structure from the string
std::string str = R"(
isok: false
status: 1
c: a
hasprice: true
num:
price: 20
)";
plain_type_t p;
iguana::from_yaml(p, str);
// serialization the structure to the string
std::string ss;
iguana::to_yaml(ss, p);
std::cout << ss << "\n";
```

### How to solve the problem of unicode path in a json file?

If there is an unicode string as a path in a json file, however iguana parse the file as utf-8, so maybe you can see some strange characters after parse.

It's ok, because you see the utf-8 strings. The problem is you can't use the string directly, such as use std::ifstream to open the file with the unicode string path.

We can slove the problem1 easily with c++17:

```c++
  //the p.path is a unicode string path
  std::ifstream in(std::filesystem::u8path(p.path)); //std::filesystem::u8path help us
  //now you can operate the file
```

### how to handle the enum type as strings?

By default, Iguana handle enum type as  number type during serialization and deserialization.
To handle the enum type as strings during serialization and deserialization with Iguana, we need to define a  full specialization template in the "iguana" namespace. This template is a struct that contains an array with the underlying numbers corresponding to the enum type.
For example, if we have the following enum type:

```c++
enum class Status { STOP = 10, START };
```

And we want to handle the enum type as strings when parsing JSON:

```c++
    std::string str = R"(
{
  "a": "START",
  "b": "STOP"
}
  )";
```

To do this, we define the full specialization template in the "iguana" namespace:

```c++
namespace iguana {
template <> struct enum_value<Status> {
  constexpr static std::array<int, 2> value = {10, 11};
};
} // namespace iguana
```

Once this is done, we can continue writing the rest of the code as usual.

```c++
struct enum_t {
    Status a;
    Status b;
};
YLT_REFL(enum_t, a, b);

// deserialization
enum_t e;
iguana::from_json(e, str);
// serialization
enum_t e1;
e1.a = Status::START;
e1.b = Status::STOP;
std::string ss;
iguana::to_json(e1, ss);
```

### Serialization of protobuf
similar with before:
```cpp
struct person {
  int id;
  std::string name;
  int age;
  bool operator==(person const& rhs) const {
    return id == rhs.id && name == rhs.name && age == rhs.age;
  }
};

#if __cplusplus < 202002L
YLT_REFL(person, id, name, age) //define meta data
#endif

void test() {
  person p{1, "tom", 20};
  std::string pb;
  iguana::to_pb(p, pb);
  person p1;
  iguana::from_pb(p1, pb);
  CHECK(p == p1);
}
```
[more detail](lang/struct_pb_intro.md)

### Full sources:


+ More examples about [json](https://github.com/qicosmos/iguana/blob/master/example/example.cpp)
+ More examples about [xml](https://github.com/qicosmos/iguana/blob/master/example/xml_example.cpp)
+ More examples about [yaml](https://github.com/qicosmos/iguana/blob/master/example/yaml_example.cpp)
+ More examples about [struct_pb](https://github.com/qicosmos/iguana/blob/master/test/test_pb.cpp)

### Scripts

Automatically generate `YLT_REFL` macros based by struct.

To get a list of basic options and switches use:

```bash
python automatic_macro_generator.py -h
```

basic example:

The content of the test_macro_generator.cpp is as follows:

```c++
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
```


execute script:

```
python automatic_macro_generator.py -i test_macro_generator.cpp
```

After processing by the automatic_macro_generator.py script,test_macro_generator.cpp change into:

```c++
struct person {
  std::string name;
  int age;
};
YLT_REFL(person, name, age);
char *iguana = NULL;

struct composit_t { int a; std::vector<std::string> b; int c; std::map<int, int> d; std::unordered_map<int, int> e; double f;};
YLT_REFL(composit_t, a, b, c, d, e, f);

struct composit_t2
{
  int a;
  std::vector<std::string> b;
  int iguana;
  std::map<int, int> example_test;
  std::unordered_map<int, int> random_name__;
  double __f__number__complex;
};
YLT_REFL(composit_t2, a, b, iguana, example_test, random_name__, __f__number__complex);
```

other example:

```bash
python automatic_macro_generator.py -i test_macro_generator.cpp -o have_macro.cpp
```

test_macro_generator.cpp will be unchanged, have_macro.cpp will be changed to source file with YLT_REFL macro.

scripts works out of the box with Python version 2.7 and 3.x on any platform.

Notes: In Python3,Will prompt `DeprecationWarning: 'U' mode is deprecated`.Ignore it.


### F.A.Q

- **Question**: Why is the library called *iguana*?

  - **Answer**: I think serialization is like an iguana, because the only difference is the displaying format, however the meta data is never changed. With changeless meta data and YLT_REFL, you can serialize an object to any format, which is like how an iguana does.

- **Question**: Does *iguana* support raw pointer?

  - **Answer**: No. *iguana* doesn't support raw pointer, but it will support smart pointer in the future.

- **Question**: Is iguana thread-safe?

  - **Answer**: Not yet, but it's not a problem, you can use `lock` before calling `from_json` or `to_json`.


- **Question**: Is *iguana* high performance?
  - **Answer**: Yes, it is, because *iguana* is based on compile-time reflection.

- **Question**: I found a bug, how could I report?
  - **Answer**: Create an issue on [GitHub](https://github.com/qicosmos/iguana) with a detailed description. 

### deps
frozen lib

### Update

1. Support C++20 and C++17
2. Refactor json reader, modification based on glaze  [json/read.hpp](https://github.com/stephenberry/glaze/blob/main/include/glaze/json/read.hpp)