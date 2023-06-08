#include <deque>
#include <iterator>
#include <list>
#include <vector>
#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <iostream>
#include <optional>

namespace {

struct address_t {
  std::string_view street;
  std::string_view city;
  std::string_view state;
  std::string_view country;
};
REFLECTION(address_t, street, city, state, country);
struct contact_t {
  std::string_view type;
  std::string_view value;
};
REFLECTION(contact_t, type, value);
struct person_t {
  std::string_view name;
  // float age;
  address_t address;
  std::vector<contact_t> contacts;
};
std::ostream& operator<<(std::ostream& os, person_t p) {
  os << "name: " << p.name << "age: " << std::endl;
  os << p.address.street << "\n";
  os << p.address.city << "\n";
  os << p.address.state << "\n";
  os << p.address.country << "\n";
  os << p.contacts[0].type << " : " << p.contacts[0].value << "\n";
  os << p.contacts[1].type << " : " << p.contacts[1].value << "\n";
  return os;
}
REFLECTION(person_t, name, address, contacts);

void person_example() {
  std::string str = R"(
name: John Doe
address:
  street: 123 Main St
  city: Anytown
  state: Example State
  country: Example Country
contacts:
  - type: email
    value: john@example.com
  - type: phone
    value: 123456789
  - type: social
    value: '@johndoe'
  )";
  person_t p;
  iguana::from_yaml(p, str);
  std::cout << p;
}



}


int main() {
  person_example();
}