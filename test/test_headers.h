#pragma once
#include "iguana/reflection.hpp"
#include <map>
#include <optional>
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
struct property_t {
  std::string name;
}; // Property
REFLECTION(property_t, name);

struct polygon_t {
  std::string type;
  std::vector<std::vector<std::array<double, 2>>> coordinates;
}; // Polygon
REFLECTION(polygon_t, type, coordinates);

struct feature_t {
  std::string type;
  property_t properties;
  polygon_t geometry;
}; // Feature
REFLECTION(feature_t, type, properties, geometry);

struct FeatureCollection {
  std::string type;
  std::vector<feature_t> features;
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

struct apache_empty_t {};
REFLECTION_EMPTY(apache_empty_t);

struct apache_builds {
  std::vector<apache_empty_t> assignedLabels;
  std::string mode;
  std::string nodeDescription;
  std::string nodeName;
  int64_t numExecutors;
  std::string description;
  std::vector<jobs_t> jobs;
  apache_empty_t overallLoad;
  apache_empty_t unlabeledLoad;
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

// citm_catalog.json
struct events_value_t {
  std::optional<std::string> description;
  std::int64_t id;
  std::optional<std::string> logo;
  std::string name;
  std::vector<std::int64_t> subTopicIds;
  std::optional<int64_t> subjectCode;
  std::optional<std::string> subtitle;
  std::vector<std::int64_t> topicIds;
}; // events_value_t
REFLECTION(events_value_t, description, id, logo, name, subTopicIds,
           subjectCode, subtitle, topicIds);

struct prices_element_t {
  std::int64_t amount;
  std::int64_t audienceSubCategoryId;
  std::int64_t seatCategoryId;
}; // prices_element_t
REFLECTION(prices_element_t, amount, audienceSubCategoryId, seatCategoryId);

struct areas_element_t {
  std::int64_t areaId;
  std::vector<int64_t> blockIds;
}; // areas_element_t
REFLECTION(areas_element_t, areaId, blockIds);

struct seatCategories_element_t {
  std::vector<areas_element_t> areas;
  std::int64_t seatCategoryId;
}; // seatCategories_element_t
REFLECTION(seatCategories_element_t, areas, seatCategoryId);

struct performances_element_t {
  std::int64_t eventId;
  std::int64_t id;
  std::optional<std::string> logo;
  std::optional<std::string> name;
  std::vector<prices_element_t> prices;
  std::vector<seatCategories_element_t> seatCategories;
  std::optional<std::string> seatMapImage;
  std::int64_t start;
  std::string venueCode;
}; // performances_element_t
REFLECTION(performances_element_t, eventId, id, logo, name, prices,
           seatCategories, seatMapImage, start, venueCode);

struct venueNames_t {
  std::string PLEYEL_PLEYEL;
}; // venueNames_t
REFLECTION(venueNames_t, PLEYEL_PLEYEL);

struct citm_object_t {
  std::unordered_map<std::int64_t, std::string> areaNames;
  std::unordered_map<std::int64_t, std::string> audienceSubCategoryNames;
  apache_empty_t blockNames;
  apache_empty_t subjectNames;
  std::unordered_map<std::int64_t, events_value_t> events;
  std::vector<performances_element_t> performances;
  std::unordered_map<std::string, std::string> seatCategoryNames;
  std::unordered_map<std::string, std::string> subTopicNames;
  std::unordered_map<std::string, std::string> topicNames;
  std::unordered_map<std::string, std::vector<std::int64_t>> topicSubTopics;
  std::optional<venueNames_t> venueNames;
}; // citm_object_t
REFLECTION(citm_object_t, areaNames, audienceSubCategoryNames, blockNames,
           subjectNames, events, performances, seatCategoryNames, subTopicNames,
           topicNames, topicSubTopics, venueNames);

// gsoc-2018.json
struct sponsor_t {
  std::string type; //@
  std::string name;
  std::string disambiguatingDescription;
  std::string description;
  std::string url;
  std::string logo;
};
REFLECTION(sponsor_t, type, name, disambiguatingDescription, description, url,
           logo);

struct author_t {
  std::string type; //@
  std::string name;
};
REFLECTION(author_t, type, name);

struct gsoc_element_t {
  std::string context; //@
  std::string type;    //@
  std::string name;
  std::string description;
  sponsor_t sponsor;
  author_t author;
};
REFLECTION(gsoc_element_t, context, type, name, description, sponsor, author);

using gsoc_object_t = std::map<int, gsoc_element_t>;