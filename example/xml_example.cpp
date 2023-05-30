#include <iguana/xml_reader.hpp>
#include <iguana/xml_writer.hpp>
#include <rapidxml_print.hpp>

namespace client {
struct madoka {
  std::string onegayi;
  double power;
};

REFLECTION(madoka, onegayi, power);
} // namespace client

struct Owner_t {
  std::string ID;
  std::string DisplayName;
  auto operator==(const Owner_t &rhs) const {
    return ID == rhs.ID && DisplayName == rhs.DisplayName;
  }
};
REFLECTION(Owner_t, ID, DisplayName);

struct Contents {
  std::string Key;
  std::string LastModified;
  std::string ETag;
  std::string Type;
  uint32_t Size;
  std::string StorageClass;
  Owner_t Owner;

  auto operator==(const Contents &rhs) const {
    return Key == rhs.Key && LastModified == rhs.LastModified &&
           ETag == rhs.ETag && Type == rhs.Type && Size == rhs.Size &&
           StorageClass == rhs.StorageClass && Owner == rhs.Owner;
  }
};
REFLECTION(Contents, Key, LastModified, ETag, Type, Size, StorageClass, Owner);

void test_to_xml() {
  Contents contents{"key", "ddd", "ccc", "aaa", 123, "aaa", {"bbb", "sss"}};

  // pretty xml
  std::string ss;
  iguana::to_xml_pretty(contents, ss);
  std::cout << ss << "\n";

  // non pretty xml
  std::string s;
  iguana::to_xml(contents, s);
  std::cout << s << "\n";
}

void test_from_xml() {
  Contents contents{"key", "ddd", "ccc", "aaa", 123, "aaa", {"bbb", "sss"}};

  // pretty xml
  std::string ss;
  iguana::to_xml_pretty(contents, ss);
  std::cout << ss << "\n";

  Contents contents2{"test"};
  iguana::from_xml(contents2, ss.data());
  std::cout << contents2.Size << "\n";
  std::cout << contents2.Owner.DisplayName << "\n";
  assert(contents == contents2);
}

struct status_t {
  uint32_t fileId;
  uint32_t length;
  std::string path;
  int permission;
  std::string_view owner;
  std::string ownerGroup;
  uint64_t mtime;
  uint64_t atime;
  std::string symlink; // optional
  bool isDir;
  int storagePolicy;
  int childNum; // optional
};
REFLECTION(status_t, fileId, length, path, permission, owner, ownerGroup, mtime,
           atime, symlink, isDir, storagePolicy, childNum);

struct response {
  status_t status;
};
REFLECTION(response, status);

void test_parse_status() {
  std::string str = R"(
    <status>
        <fileId>123456789</fileId>
        <length>1025</length>
        <path>/tmp/sample.txt</path>
        <permission>777</permission>
        <owner>hadoop</owner>
        <ownerGroup>hadoop</ownerGroup>
        <mtime>123476424567890</mtime>
        <atime>123476424567890</atime>
        <symlink></symlink>
        <isDir>false</isDir>
        <storagePolicy>1</storagePolicy>
        <childNum>0</childNum>    //only exist when it is a dir
	</status>
)";

  status_t t{};
  iguana::from_xml(t, str.data());
  std::cout << t.owner << "\n";
  std::cout << t.mtime << ", " << t.atime << "\n";
  std::cout << t.storagePolicy << "\n";

  std::string ss;
  iguana::to_xml(t, ss);
}

void test_parse_response() {
  std::string str = R"(
    <response>
        <status>
            <fileId>123456789</fileId>
            <length>1025</length>
            <path>/tmp/sample.txt</path>
            <permission>777</permission>
            <owner>hadoop</owner>
            <ownerGroup>hadoop</ownerGroup>
            <mtime>123476424567890</mtime>
            <atime>123476424567890</atime>
            <symlink></symlink>
            <isDir>false</isDir>
            <storagePolicy>1</storagePolicy>
            <childNum>0</childNum>    //only exist when it is a dir
        </status>
    </response>
)";

  response t{};
  iguana::from_xml(t, str.data());
  std::cout << t.status.owner << "\n";
}

struct optional_t {
  int a;
  std::optional<int> b;
  std::optional<std::string> c;
  bool d;
  char e;
};
REFLECTION(optional_t, a, b, c, d, e);

void test_optional() {
  optional_t op{1, 2};
  op.d = true;
  op.e = 'o';
  std::string ss;
  iguana::to_xml(op, ss);
  std::cout << ss << "\n";

  optional_t op1;
  iguana::from_xml(op1, ss.data());
  if (op1.b) {
    std::cout << *op1.b << "\n";
  }
  if (op1.c) {
    std::cout << *op1.c << "\n";
  }

  std::cout << op1.d << "\n";
  std::cout << op1.e << "\n";
}

struct list_t {
  std::vector<optional_t> list;
  int id;
};
REFLECTION(list_t, list, id);
void test_list() {
  list_t l;
  l.list.push_back(optional_t{1, 2, {}, 0, 'o'});
  l.list.push_back(optional_t{3, 4, {}, 0, 'k'});
  l.list.push_back(optional_t{5, 6, {}, 0, 'l'});

  std::string ss;
  iguana::to_xml(l, ss);
  std::cout << ss << "\n";

  list_t l1;
  iguana::from_xml(l1, ss.data());
  std::cout << l1.list.size() << "\n";

  std::string s;
  iguana::to_xml_pretty(l, s);
  std::cout << s << '\n';
}

struct book_t {
  std::string title;
  std::string author;
  std::unordered_map<std::string, std::string> __attr;
};
std::ostream &operator<<(std::ostream &os, const book_t &b) {
  os << "book attribute : " << std::endl;
  for (auto &[k, v] : b.__attr) {
    os << "[ " << k << " : " << v << "]"
       << " ";
  }
  os << std::endl;
  os << "author : " << b.author << std::endl;
  os << "title : " << b.title << std::endl;
  return os;
}
REFLECTION(book_t, title, author, __attr);

void test_attribute() {
  std::cout << "********** test_attribute ************" << std::endl;
  std::string str = R"(
  <book id="1234" language="en" edition="1">
    <title>Harry Potter and the Philosopher's Stone</title>
    <author>J.K. Rowling</author>
  </book>
)";

  book_t book{};
  iguana::from_xml(book, str.data());
  std::cout << book;
  std::string ss;
  iguana::to_xml(book, ss);
  std::cout << "attr to_xml: " << ss << std::endl;
}

struct library_t {
  book_t book;
  std::unordered_map<std::string, std::string> __attr;
};
REFLECTION(library_t, book, __attr);

void test_nested_attribute() {
  std::cout << "********** test_nested_attribute ************" << std::endl;
  std::string str = R"(
  <library name="UESTC library">
    <book id="1234" language="en" edition="1">
      <title>Harry Potter and the Philosopher's Stone</title>
      <author>J.K. Rowling</author>
      </book>
  </library>
)";
  library_t library;
  iguana::from_xml(library, str.data());
  std::cout << "library attribute" << std::endl;
  for (auto &[k, v] : library.__attr) {
    std::cout << "[ " << k << " : " << v << "]"
              << " ";
  }
  std::cout << std::endl;
  std::cout << "\nbook\n" << library.book;

  std::string ss;
  iguana::to_xml(library, ss);
  std::cout << "library to_xml: " << ss << std::endl;
}
struct movie_t {
  std::string title;
  std::string director;
  std::unordered_map<std::string, iguana::any_t> __attr;
};
REFLECTION(movie_t, title, director, __attr);
void test_any_attribute() {
  std::cout << "********** test attribute with any ************" << std::endl;
  std::string str = R"(
  <movie id="1" time="2.3" price="32.8" language="en">
    <title>Harry Potter and the Philosopher's Stone</title>
    <director>Chris Columbus</director>
  </movie>
)";
  movie_t movie;
  iguana::from_xml(movie, str.data());
  std::cout << "movie attribute :" << std::endl;
  auto &attr = movie.__attr;
  {
    auto [isok, value] = attr["id"].get<int>();
    assert(isok == true);
    std::cout << "[ "
              << "id"
              << " : " << value << "]";
  }
  {
    auto [isok, value] = attr["price"].get<float>();
    assert(isok == true);
    std::cout << "[ "
              << "id"
              << " : " << value << "]";
  }
  {
    auto [isok, value] = attr["language"].get<std::string>();
    assert(isok == true);
    std::cout << "[ "
              << "language"
              << " : " << value << "]";
  }
  std::cout << std::endl;
}

struct person_t {
  std::vector<std::string> name;
};
REFLECTION(person_t, name);
void test_vector() {
  std::cout << "********** test vector toxml ************" << std::endl;
  person_t p;
  p.name.push_back("David");
  p.name.push_back("Bob");
  p.name.push_back("bbg");
  std::string ss;
  iguana::to_xml(p, ss);
  std::cout << ss << std::endl;
}

struct item_itunes_t {
  iguana::namespace_t<std::string_view> itunes_author;
  iguana::namespace_t<std::string_view> itunes_subtitle;
  iguana::namespace_t<int> itunes_user;
};
REFLECTION(item_itunes_t, itunes_author, itunes_subtitle, itunes_user);
struct item_t {
  iguana::namespace_t<item_itunes_t> item_itunes;
};
REFLECTION(item_t, item_itunes);
void test_namespace() {
  std::cout << "********** test namespace ************" << std::endl;
  std::string str = R"(
    <item>
      <item:itunes>
        <itunes:author>Jupiter Broadcasting</itunes:author>
        <itunes:subtitle>Linux enthusiasts talk top news stories, subtitle</itunes:subtitle>
        <itunes:user>10086</itunes:user>       
      </item:itunes>
    </item>
  )";
  item_t it;
  iguana::from_xml(it, str.data());
  auto itunes = it.item_itunes.get();
  std::cout << "author : " << itunes.itunes_author.get() << "\n";
  std::cout << "subtitle : " << itunes.itunes_subtitle.get() << "\n";
  std::cout << "user : " << itunes.itunes_user.get() << "\n";
  std::string ss;
  iguana::to_xml(it, ss);
  std::cout << "to_xml" << std::endl << ss << "\n";
}

struct package_t {
  std::pair<std::string, std::unordered_map<std::string, std::string>> version;
  std::pair<std::string, std::unordered_map<std::string, std::string>>
      changelog;
  std::unordered_map<std::string, std::string> __attr;
};
REFLECTION(package_t, version, changelog, __attr);
void test_leafnode_attribute() {
  std::string str = R"(
    <package name="apr-util-ldap" arch="x86_64">
      <version epoch="0" ver="1.6.1" rel="6.el8"/>
      <changelog author="Lubo" date="1508932800">new version 1.6.1</changelog>
    </package>
  )";
  package_t package;
  iguana::from_xml(package, str.data());
  std::cout << "package attr : \n";
  for (auto &[k, v] : package.__attr) {
    std::cout << "[ " << k << " : " << v << "]  ";
  }
  std::cout << "\nchangelog attr : \n";
  for (auto &[k, v] : package.changelog.second) {
    std::cout << "[ " << k << " : " << v << "]  ";
  }
  std::cout << "\nchangelog value : \n" << package.changelog.first << "\n";
  std::string ss;
  iguana::to_xml(package, ss);
  std::cout << "to_xml : \n" << ss << "\n";
}

void parse_error() {
  std::string str = "error xml";
  person_t p;
  bool r = iguana::from_xml(p, str.data());
  if (!r) {
    std::cout << iguana::get_last_read_err() << "\n";
  }
}
struct description_t {
  iguana::cdata_t cdata;
};
REFLECTION(description_t, cdata);
struct node_t {
  std::string title;
  description_t description;
  iguana::cdata_t cdata;
};
REFLECTION(node_t, title, description, cdata);
void test_cdata() {
  std::string str = R"(
    <node_t>
      <title>what's the cdata</title>
      <description>
        <![CDATA[<p>nest cdata node</p>]]>
      </description>
      <![CDATA[<p>this is a  cdata node</p>]]>
    </node_t>
  )";
  node_t node;
  iguana::from_xml(node, str.data());
  std::cout << "title: " << node.title << "\n";
  std::cout << "description: " << node.description.cdata.get() << "\n";
  std::cout << "cdata" << node.cdata.get() << "\n";
  std::string ss;
  iguana::to_xml(node, ss);
  std::cout << "to_xml:\n" << ss << "\n";
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
  std::cout << str << "\n";

  std::string s = "<derived_t><id>1</id><name>tom</name><version>42</"
                  "version><tag>tag</tag></derived_t>";
  assert(str == s);

  derived_t d1{};
  iguana::from_xml(d1, str.data());
  assert(d.tag == d1.tag);
}

int main(void) {
  derived_object();
  parse_error();
  test_parse_response();
  test_parse_status();
  test_list();
  test_to_xml();
  test_from_xml();
  test_optional();
  test_attribute();
  test_nested_attribute();
  test_any_attribute();
  test_vector();
  test_namespace();
  test_leafnode_attribute();
  test_cdata();
  return 0;
}