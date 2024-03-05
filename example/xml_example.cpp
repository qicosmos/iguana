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
  <![CDATA[This is some <b>bold</b> text.]]>
	<description>Some description </description>
	<child>
		<key1>10</key1>
		<key2>20</key2>
	</child>
	<hasdescription>true</hasdescription>
  <![CDATA[This is some <b>bold</b> text.]]>
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
  std::cout << st.child->key1 << " " << st.child->key2 << "\n\n";
  std::cout << "========== serialize person_t =========\n";

  std::string ss;
  iguana::to_xml(st, ss);
  std::cout << "minify serialize:\n";
  std::cout << ss << "\n";

  std::string ss1;
  iguana::to_xml<true>(st, ss1);
  std::cout << "pretty serialize:\n";
  std::cout << ss1;
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
    std::cout << "minify serialize:\n";
    std::cout << ss << "\n";

    std::string ss1;
    iguana::to_xml<true>(lib, ss1);
    std::cout << "pretty serialize:\n";
    std::cout << ss1;
  }
  {
    std::cout << "========= serialize library with attr ========\n";
    iguana::xml_attr_t<library> lib;
    iguana::from_xml(lib, str);

    std::string ss;
    iguana::to_xml(lib, ss);
    std::cout << "minify serialize:\n";
    std::cout << ss << "\n";

    std::string ss1;
    iguana::to_xml<true>(lib, ss1);
    std::cout << "pretty serialize:\n";
    std::cout << ss1;
  }
}

struct package_t {
  iguana::xml_attr_t<std::optional<std::string_view>> version;
  iguana::xml_attr_t<std::string_view> changelog;
};
REFLECTION(package_t, version, changelog);
void package_example() {
  auto validator = [](iguana::xml_attr_t<package_t> package) {
    assert(package.attr()["name"] == "apr-util-ldap");
    assert(package.attr()["arch"] == "x86_64");
    auto& p = package.value();
    assert(p.version.attr()["epoch"] == "0");
    assert(p.version.attr()["ver"] == "1.6.1");
    assert(p.version.attr()["rel"] == "6.el8");
    assert(p.changelog.attr()["author"] == "Lubo");
    assert(p.changelog.attr()["date"] == "1508932800");
    assert(p.changelog.value() == "new version 1.6.1");
  };
  std::string str = R"(
    <package_t name="apr-util-ldap" arch="x86_64">
      <version epoch="0" ver="1.6.1" rel="6.el8"/>
      <changelog author="Lubo" date="1508932800">
      new version 1.6.1</changelog>
    </package_t>
  )";
  iguana::xml_attr_t<package_t> package;
  iguana::from_xml(package, str);
  validator(package);
  std::string ss;
  iguana::to_xml(package, ss);
  std::cout << "========= serialize package_t with attr ========\n";
  std::cout << "minify serialize:\n";
  std::cout << ss << "\n";

  std::string ss1;
  iguana::to_xml<true>(package, ss1);
  std::cout << "pretty serialize:\n";
  std::cout << ss1;
}

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

  std::string s =
      "<derived_t><id>1</id><name>tom</name><version>42</"
      "version><tag>tag</tag></derived_t>";
  assert(str == s);

  derived_t d1{};
  iguana::from_xml(d1, str);
  assert(d.tag == d1.tag);
}

struct description_t {
  iguana::xml_cdata_t<std::string> cdata;
};
REFLECTION(description_t, cdata);
struct node_t {
  std::string title;
  description_t description;
  iguana::xml_cdata_t<> cdata;
};
REFLECTION(node_t, title, description, cdata);
void cdata_example() {
  std::string str = R"(
    <node_t>
      <title>what's the cdata</title>
      <description>
        <![CDATA[<p>nest cdata node1 and </p>]]>
        <![CDATA[<p>nest cdata node</p>]]>
      </description>
      <![CDATA[<p>this is a  cdata node</p>]]>
    </node_t>
  )";
  node_t node;
  iguana::from_xml(node, str);
  std::cout << "========= deserialize cdata ========\n";
  std::cout << "title: " << node.title << "\n";
  std::cout << "description: " << node.description.cdata.value() << "\n";
  std::cout << "cdata" << node.cdata.value() << "\n\n";
  std::cout << "========= serialize cdata ========\n";
  std::string ss;
  iguana::to_xml(node, ss);
  std::cout << "minify serialize:\n";
  std::cout << ss << "\n";

  std::string ss1;
  iguana::to_xml<true>(node, ss1);
  std::cout << "pretty serialize:\n";
  std::cout << ss1;
}

struct city_t {
  iguana::xml_attr_t<long> area;
  iguana::xml_cdata_t<> cd;
  std::string name;
};
REFLECTION(city_t, area, cd, name);
struct cities_t {
  std::vector<city_t> city;
};
REFLECTION(cities_t, city);
struct province {
  iguana::xml_attr_t<long> area;
  iguana::xml_cdata_t<> cd;
  std::unique_ptr<cities_t> cities;
  std::string capital;
};
REFLECTION(province, area, cd, cities, capital);

void province_example() {
  std::string_view str = R"(
<province name="Sichuan Province">
  <capital>Chengdu</capital>
  <![CDATA[ sichuan <> ]]>
  <area unit="km^2">485000</area>
  <cities>
    <city>
      <name>Chengdu City</name>
      <![CDATA[ chengdu <> ]]>
      <area unit="km^2">14412</area>
    </city>
    <city>
      <name>Suining City</name>
      <![CDATA[ Suining <> ]]>
      <area unit="km^2">20788</area>
    </city>
    <!-- More cities -->
  </cities>
</province>
  )";
  province p;
  iguana::from_xml(p, str);
  std::cout << "========= serialize province ========\n";
  std::string ss;
  iguana::to_xml(p, ss);
  std::cout << "minify serialize:\n";
  std::cout << ss << "\n";

  std::string ss1;
  iguana::to_xml<true>(p, ss1);
  std::cout << "pretty serialize:\n";
  std::cout << ss1;
}

int main(void) {
  some_type_example();
  lib_example();
  package_example();
  derived_object();
  cdata_example();
  province_example();
  return 0;
}