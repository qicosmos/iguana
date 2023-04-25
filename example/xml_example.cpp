#include "iguana/xml_reader.hpp"
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
  iguana::xml::to_xml_pretty(ss, contents);
  std::cout << ss << "\n";

  // non pretty xml
  std::string s;
  iguana::xml::to_xml(s, contents);
  std::cout << s << "\n";
}

void test_from_xml() {
  Contents contents{"key", "ddd", "ccc", "aaa", 123, "aaa", {"bbb", "sss"}};

  // pretty xml
  std::string ss;
  iguana::xml::to_xml_pretty(ss, contents);
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
  std::string owner;
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
  iguana::xml::to_xml(ss, op);
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

  std::string ss;
  iguana::xml::to_xml(ss, l);
  std::cout << ss << "\n";

  iguana::xml::to_xml_pretty(ss, l);
  std::cout << ss << '\n';
}

int main(void) {
  test_list();
  test_parse_status();
  test_parse_response();
  test_to_xml();
  test_from_xml();
  test_optional();

  return 0;
}