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
struct some_type_root_t {
  some_type_t root;
};
REFLECTION(some_type_root_t, root);
void some_type_example() {
  auto validator_some_type = [](const some_type_t& s) {
    assert(s.price[0] == 1.23f);
    assert(s.price[1] == 3.25f);
    assert(s.price[2] == 9.57f);
    assert(*s.description == "Some description");
    assert(s.child->key1 == 10);
    assert(s.child->key2 == 20);
    assert(s.hasdescription == true);
    assert(s.c == 'X');
    assert(*s.d_v == 3.14159);
    assert(s.name == "John Doe");
    assert(s.addr == "123 Main St");
    assert(s.status == enum_status::stop);
  };
  std::string str = R"(
<?xml version="1.0" encoding="UTF-8"?>
<root a="b" b="c">
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
</root>
)";
  some_type_root_t t;
  iguana::from_xml(t, str);
  auto &s = t.root;
  validator_some_type(s);
  std::cout << "========= deserialize some_type_t ========\n";
  std::cout << "price: ";
  for (auto p : s.price) {
    std::cout << p << " ";
  }
  std::cout << "\n description : " << *s.description << "\n";
  std::cout << s.child->key1 << " " << s.child->key2 << std::endl;
  std::cout << "========== serialize person_t =========\n";
  std::string ss;
  iguana::to_xml(t, ss);
  std::cout << ss << std::endl;

  some_type_t some_type;
  iguana::from_xml<true>(some_type, str);
  validator_some_type(some_type);
}

int main(void) {
  some_type_example();
  return 0;
}