#include <deque>
#include <iterator>
#include <list>
#include <vector>
#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <iostream>
#include <optional>
#include <cassert>

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
  int age;
  address_t address;
  std::vector<contact_t> contacts;
};
REFLECTION(person_t, name, age, address, contacts);

std::ostream& operator<<(std::ostream& os, person_t p) {
  os << "name: " << p.name << "\tage: " << p.age << std::endl;
  os << p.address.street << "\n";
  os << p.address.city << "\n";
  os << p.address.state << "\n";
  os << p.address.country << "\n";
  os << p.contacts[0].type << " : " << p.contacts[0].value << "\n";
  os << p.contacts[1].type << " : " << p.contacts[1].value << "\n";
  os << p.contacts[2].type << " : " << p.contacts[2].value << "\n";
  return os;
}
auto validator_person = [](const person_t& p) {
  assert(p.name == "John Doe");
  assert(p.age == 30);
  assert(p.address.street == "123 Main St");
  assert(p.address.city == "Anytown");
  assert(p.address.state == "Example State");
  assert(p.address.country == "Example Country");
  assert(p.contacts[0].type == "email");
  assert(p.contacts[0].value == "john@example.com");
  assert(p.contacts[1].type == "phone");
  assert(p.contacts[1].value == "123456789");
  assert(p.contacts[2].type == "social");
  assert(p.contacts[2].value == "johndoe");
};
void person_example() {
  std::string str = R"(
name: John Doe
age: 30
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
    value: "johndoe"
  )";
  person_t p;
  iguana::from_yaml(p, str);
  validator_person(p);
  std::cout << "========= deserialize person_t ========\n";
  std::cout << p;
  std::string ss;
  iguana::to_yaml(p, ss);
  std::cout << "========== serialize person_t =========\n";
  std::cout << ss;
  person_t p2;
  iguana::from_yaml(p2, ss);
  validator_person(p2);
}


struct map_person_t {
  using map_type =  std::unordered_map<std::string_view, std::string_view>;
  std::string_view name;
  int age;
  map_type address;
  std::vector<map_type> contacts;
};
// Do not support } or ,  at single line
REFLECTION(map_person_t, name, age, address, contacts);
void map_person_example() {
  std::string str = R"(
name: John Doe
age: 30
address: {  
  street: 123 Main St, city: Anytown,
  state: Example State,
  country: Example Country }
contacts:
  - {type: email, value: john@example.com}
  - {type: phone, value: 123456789}
  - {type: social, value: "johndoe"}
  )";
  map_person_t p;
  iguana::from_yaml(p, str);

  std::cout << "========= deserialize map_person_t ========\n";
  for (auto [k, v] : p.address) {
    std::cout << k << ":" << v << "\t";
  }
  std::cout  << "\ncontacts :\n";
  for (auto& map : p.contacts) {
    for (auto [k, v] : map) {
      std::cout << k << ":" << v << "\t";
    }
  }
  std::cout << "\n";
};

int main() {
  person_example();
  map_person_example();
}