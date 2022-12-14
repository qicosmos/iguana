struct person {
  std::string name;
  int age;
};

struct two {
  std::string name;
  one_t one;
  int age;
};

struct composit_t {
  int a;
  std::vector<std::string> b;
  int c;
  std::map<int, int> d;
  std::unordered_map<int, int> e;
  double f;
};

struct composit_t2 {
  int a;
  std::vector<std::string> b;
  int iguana;
  std::map<int, int> example_test;
  std::unordered_map<int, int> random_name__;
  double __f__number__complex;
};
