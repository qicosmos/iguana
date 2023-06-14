#include "iguana/yaml_reader.hpp"
#include "iguana/yaml_writer.hpp"
#include <cassert>
#include <deque>
#include <iostream>
#include <iterator>
#include <list>
#include <optional>
#include <vector>

enum class enum_status {
  start,
  stop,
};
struct some_type_t {
  std::vector<float> price;
  std::string description;
  std::map<std::string, int> child;
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
price: [1.23, 3.25, 9.57]
description: >-
    Some 
    description
child:
  key1: 10
  key2: 20
hasdescription: true
c: X
d_v: 3.14159
name: John Doe
addr: '123 Main St'
status : 1
)";
  some_type_t s;
  iguana::from_yaml(s, str);
  std::cout << "========= deserialize some_type_t ========\n";
  std::cout << "price: ";
  for (auto p : s.price) {
    std::cout << p << " ";
  }
  std::cout << "\n description : " << s.description << "\n";
  std::cout << s.child["key1"] << " " << s.child["key2"];
  std::cout << "========== serialize person_t =========\n";
  std::string ss;
  iguana::to_yaml(s, ss);
  std::cout << ss;
  some_type_t s1;
  iguana::from_yaml(s1, ss);
}

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

std::ostream &operator<<(std::ostream &os, person_t p) {
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
  std::cout << "========= deserialize person_t ========\n";
  std::cout << p;
  std::string ss;
  iguana::to_yaml(p, ss);
  std::cout << "========== serialize person_t =========\n";
  std::cout << ss;
  person_t p2;
  iguana::from_yaml(p2, ss);
}

struct map_person_t {
  using map_type = std::unordered_map<std::string_view, std::string_view>;
  std::string_view name;
  int age;
  map_type address;
  std::vector<map_type> contacts;
};
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
  std::cout << "\ncontacts :\n";
  for (auto &map : p.contacts) {
    for (auto [k, v] : map) {
      std::cout << k << ":" << v << "\t";
    }
  }
  std::cout << "\n";
};

struct product_t {
  std::string_view name;
  float price;
  std::optional<std::string> description;
};
REFLECTION(product_t, name, price, description);
struct store_t {
  std::string name;
  std::string_view location;
  std::vector<product_t> products;
};
REFLECTION(store_t, name, location, products);
struct store_example_t {
  store_t store;
};
REFLECTION(store_example_t, store);
void store_example() {
  std::string str = R"(
store:
  name: "\u6c38\u8f89\u8d85\u5e02\t"
  location: Chengdu
  products:
    - name: iPad
      price: 
        899.4
      description: >
        nice
        ipad
    - name: watch
      price: 488.8
      description: |
        cheap watch
    - name: iPhone
      price: 999.99
      description: >-   
        expensive
        iphone
  )";
  store_example_t store_1;
  iguana::from_yaml(store_1, str);
  auto store = store_1.store;
  std::cout << "========= deserialize store_example_t ========\n";
  std::cout << "name :" << store.name << "location : " << store.location
            << "\n";
  std::cout << "products total 3\n";
  for (auto &p : store.products) {
    std::cout << "name: " << p.name << "\tprice:" << p.price
              << "\tdesc:" << *p.description;
  }
  std::cout << "\n";
}

struct book_t {
  std::optional<std::string_view> title;
  std::vector<std::string_view> categories;
};
REFLECTION(book_t, title, categories);
struct library_t {
  std::unique_ptr<std::string_view> name;
  std::string location;
  std::vector<std::unique_ptr<book_t>> books;
};
REFLECTION(library_t, name, location, books);
struct library_example_t {
  std::vector<library_t> libraries;
};
REFLECTION(library_example_t, libraries);

void library_example() {
  std::string str = R"(
libraries:
  - name: 
      Central Library
    location: "Main\tStreet"
    books:
      - title:
        categories: 
          - computer science
          - programming
      - title: The Great Gatsby
        categories:
          - classic literature
          - fiction
  - name: North Library
    location: "Elm Avenue"
    books:
      - title: 
        categories:
          - computer science
          - algorithms
      - title: 
        categories:
          - classic literature
          - romance
  )";
  library_example_t libs;
  iguana::from_yaml(libs, str);
  std::cout << "========= serialize library_example_t ==========\n";
  std::string ss;
  iguana::to_yaml(libs, ss);
  std::cout << ss;
}

void test_books_example() {
  std::vector<book_t> books;
  std::string str = R"(
    - title:
      categories: 
        - computer science
        - programming
    - title: The Great Gatsby
      categories:
        - classic literature
        - fiction
  )";
  iguana::from_yaml(books, str);
  assert(!books[0].title);
  assert(books[0].categories[0] == "computer science");
  assert(books[0].categories[1] == "programming");
  assert(*books[1].title == "The Great Gatsby");
  assert(books[1].categories[0] == "classic literature");
  assert(books[1].categories[1] == "fiction");
}

int main() {
  some_type_example();
  person_example();
  map_person_example();
  store_example();
  library_example();
  test_books_example();
}