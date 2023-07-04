#include <iguana/xml_reader.hpp>
#include <iguana/xml_writer.hpp>

enum class enum_status {
  start,
  stop,
};
struct child_t {
  int key1;
  int key2;
};
REFLECTION(child_t, key1, key2);
struct some_type_t {
  std::vector<float> price;
  std::optional<std::string> description;
  std::unique_ptr<child_t> child;
  bool hasdescription;
  char c;
  std::optional<double> d_v;
  std::string name;
  std::string_view addr;
  enum_status status;
};
REFLECTION(some_type_t, price, description, child, hasdescription, c, d_v, name,
           addr, status);
void some_type_example() {
  std::string str = R"(
<?xml version="1.0" encoding="UTF-8"?>
<some_type_t a="b" b="c">
	<price>1.23</price>
	<price>3.25</price>
	<price>9.57</price>
	<description>Some description </description>
	<child>
		<key1>10</key1>
		<key2>20</key2>
	</child>
	<hasdescription>true</hasdescription>
	<c>X</c>
	<d_v>3.14159</d_v>
	<name>John Doe</name>
	<addr>123 Main St</addr>
	<status>1</status>
</some_type_t>
)";
  some_type_t st;
  iguana::from_xml(st, str);
  std::cout << "========= deserialize some_type_t ========\n";
  std::cout << "price: ";
  for (auto p : st.price) {
    std::cout << p << " ";
  }
  std::cout << "\n description : " << *st.description << "\n";
  std::cout << st.child->key1 << " " << st.child->key2 << std::endl;
  std::cout << "========== serialize person_t =========\n";
  std::string ss;
  iguana::to_xml(st, ss);
  std::cout << ss << std::endl;

  some_type_t st1;
  iguana::from_xml(st1, ss);
}

struct book_t {
  std::string title;
  std::string author;
};
REFLECTION(book_t, title, author);
struct library {
  iguana::xml_attr_t<book_t> book;
};
REFLECTION(library, book);

void lib_example() {
  std::string str = R"(
  <library name="UESTC library">
    <book id="1234" language="en" edition="1">
      <title>Harry Potter and the Philosopher's Stone</title>
      <author>J.K. Rowling</author>
      </book>
  </library>
)";
  {
    std::cout << "========= serialize book_t with attr ========\n";
    library lib;
    iguana::from_xml(lib, str);
    std::string ss;
    iguana::to_xml(lib, ss);
    std::cout << ss << '\n';
  }
  {
    std::cout << "========= serialize library with attr ========\n";
    iguana::xml_attr_t<library> lib;
    iguana::from_xml(lib, str);

    std::string ss;
    iguana::to_xml(lib, ss);
    std::cout << ss << '\n';
  }
}

// struct package_t {
//   std::pair<std::string, std::unordered_map<std::string, std::string>>
//   version; std::pair<std::string, std::unordered_map<std::string,
//   std::string>>
//       changelog;
//   std::unordered_map<std::string, std::string> __attr;
// };
// REFLECTION(package_t, version, changelog, __attr);
// void test_leafnode_attribute() {
//   std::string str = R"(
//     <package name="apr-util-ldap" arch="x86_64">
//       <version epoch="0" ver="1.6.1" rel="6.el8"/>
//       <changelog author="Lubo" date="1508932800">new
//       version 1.6.1</changelog>
//     </package>
//   )";
//   package_t package;
//   iguana::from_xml(package, str.data());
//   std::cout << "package attr : \n";
//   for (auto &[k, v] : package.__attr) {
//     std::cout << "[ " << k << " : " << v << "]  ";
//   }
//   std::cout << "\nchangelog attr : \n";
//   for (auto &[k, v] : package.changelog.second) {
//     std::cout << "[ " << k << " : " << v << "]  ";
//   }
//   std::cout << "\nchangelog value : \n" << package.changelog.first << "\n";
//   std::string ss;
//   iguana::to_xml(package, ss);
//   std::cout << "to_xml : \n" << ss << "\n";
// }

struct base_t {
  int id;
  std::string name;
};

struct derived_t : public base_t {
  int version;
  std::string tag;
};
REFLECTION(derived_t, id, name, version, tag);

void derived_object() {
  derived_t d{};
  d.id = 1;
  d.name = "tom";
  d.version = 42;
  d.tag = "tag";

  std::string str;
  iguana::to_xml(d, str);
  std::cout << "========= serialize the derived object ========\n";
  std::cout << str << "\n";

  std::string s = "<derived_t><id>1</id><name>tom</name><version>42</"
                  "version><tag>tag</tag></derived_t>";
  assert(str == s);

  derived_t d1{};
  iguana::from_xml(d1, str);
  assert(d.tag == d1.tag);
}

int main(void) {
  some_type_example();
  lib_example();
  derived_object();
  return 0;
}