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
  validator_some_type(st);
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
}

int main(void) {
  some_type_example();
  return 0;
}