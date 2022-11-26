#pragma once
#include "iguana/reflection.hpp"
#include <vector>

struct MyClass1 {
  double member0;
  double member1;
  double member2;
  double member3;

  bool operator==(MyClass1 const &rhs) const {
    return member0 == rhs.member0 and member1 == rhs.member1 and
           member2 == rhs.member2 and member3 == rhs.member3;
  }
};
REFLECTION(MyClass1, member0, member1, member2, member3);

struct MyClass2 {
  unsigned member_unsigned0;
  unsigned member_unsigned1;
  signed member_signed;

  bool operator==(MyClass2 const &rhs) const {
    return member_unsigned0 == rhs.member_unsigned0 and
           member_unsigned1 == rhs.member_unsigned1 and
           member_signed == rhs.member_signed;
  }
};
REFLECTION(MyClass2, member_unsigned0, member_unsigned1, member_signed);

struct person {
  std::string name;
  int age;
};
REFLECTION(person, name, age);

// canada.json
struct Property {
  std::string name;
}; // Property
REFLECTION(Property, name);

struct Polygon {
  std::string type;
  std::vector<std::vector<std::array<double, 2>>> coordinates;
}; // Polygon
REFLECTION(Polygon, type, coordinates);

struct Feature {
  std::string type;
  Property properties;
  Polygon geometry;
}; // Feature
REFLECTION(Feature, type, properties, geometry);

struct FeatureCollection {
  std::string type;
  std::vector<Feature> features;
}; // FeatureCollection
REFLECTION(FeatureCollection, type, features);

// apache_builds.json
struct jobs_t {
  std::string name;
  std::string url;
  std::string color;
};
REFLECTION(jobs_t, name, url, color);

struct views_t {
  std::string name;
  std::string url;
};
REFLECTION(views_t, name, url);

struct empty_t {};
REFLECTION_EMPTY(empty_t);

struct apache_builds {
  std::vector<empty_t> assignedLabels;
  std::string mode;
  std::string nodeDescription;
  std::string nodeName;
  int64_t numExecutors;
  std::string description;
  std::vector<jobs_t> jobs;
  empty_t overallLoad;
  empty_t unlabeledLoad;
  views_t primaryView;
  bool quietingDown;
  int64_t slaveAgentPort;
  bool useCrumbs;
  bool useSecurity;
  std::vector<views_t> views;
};
REFLECTION(apache_builds, assignedLabels, mode, nodeDescription, nodeName,
           numExecutors, description, jobs, overallLoad, unlabeledLoad,
           primaryView, quietingDown, slaveAgentPort, useCrumbs, useSecurity,
           views);
