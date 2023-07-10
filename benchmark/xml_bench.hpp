#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "iguana/reflection.hpp"
#include "iguana/xml_reader.hpp"
#include "iguana/xml_writer.hpp"

// ************ struct for filelists.xml ****************

struct package_t {
  std::optional<std::string_view> version;
  std::vector<std::string_view> file;
};
REFLECTION(package_t, version, file);

struct filelists_t {
  std::vector<package_t> package;
};
REFLECTION(filelists_t, package);
// ************ struct for sample_rss.xml ****************

struct skip_t {
  std::string_view skip;
};
REFLECTION(skip_t, skip);
struct item_t {
  std::string_view title;
  std::string_view link;
  skip_t description;
  std::string_view pubDate;
  std::optional<std::string_view> enclosure;
  std::string_view guid;
  std::string_view itunes_author;
  std::string_view itunes_subtitle;
  skip_t itunes_summary;
  std::string_view itunes_explicit;
  std::string_view itunes_duration;
  std::string_view dc_creator;
  std::string_view author;
  std::optional<std::string_view> media_thumbnail;
  std::optional<std::string_view> media_content;
  std::string_view itunes_keywords;
};
REFLECTION(item_t, title, link, description, pubDate, enclosure, guid,
           itunes_author, itunes_subtitle, itunes_summary, itunes_explicit,
           itunes_duration, dc_creator, author, media_thumbnail, media_content,
           itunes_keywords);

struct image_t {
  std::string_view link;
  std::string_view url;
  std::string_view title;
};
REFLECTION(image_t, link, url, title);

struct channel_t {
  std::string_view title;
  std::string_view link;
  std::string_view description;
  std::string_view generator;
  std::string_view docs;
  std::string_view language;
  std::string_view pubDate;
  std::string_view lastBuildDate;
  image_t image;
  std::string_view itunes_author;
  std::string_view itunes_subtitle;
  std::string_view itunes_summary;
  std::string_view itunes_keywords;
  std::optional<std::string_view> itunes_image;
  std::string_view itunes_explicit;
  std::string_view itunes_block;
  std::vector<item_t> item;
  std::string_view media_credit;
  std::string_view media_rating;
  std::string_view media_description;
};
REFLECTION(channel_t, title, link, description, generator, docs, language,
           pubDate, lastBuildDate, image, itunes_author, itunes_subtitle,
           itunes_summary, itunes_keywords, itunes_image, itunes_explicit,
           itunes_block, item, media_credit, media_rating, media_description);

struct rss_t {
  channel_t channel;
};
REFLECTION(rss_t, channel);

// ************ struct for bench_num.xml ****************

struct goods_t {
  std::string_view id;
  std::string_view sales;
  std::string_view inventory;
  std::string_view weight;
  std::string_view price;
  std::string_view rating;
  std::string_view discount;
};
REFLECTION(goods_t, id, sales, inventory, weight, price, rating, discount);
struct storeowner_t {
  std::string_view name;
  std::string_view telephone;
};
REFLECTION(storeowner_t, name, telephone);
struct store_t {
  std::string_view name;
  std::string_view address;
  storeowner_t owner;
  std::vector<goods_t> goods;
};
REFLECTION(store_t, name, address, owner, goods);
