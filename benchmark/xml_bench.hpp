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
  std::string_view version;
  std::vector<std::string_view> file;
};
REFLECTION(package_t, version, file);

struct filelists_t {
  std::vector<package_t> package;
};
REFLECTION(filelists_t, package);
// ************ struct for sample_rss.xml ****************

struct item_t {
  std::string_view title;
  std::string_view description; // CDATA
  std::string_view pubDate;
  std::string_view enclosure;
  std::string_view guid;
  std::string_view itunes_explicit;
  std::string_view dc_creator;
  std::string_view media_thumbnail;
  std::string_view media_content;
  std::string_view itunes_subtitle;
  std::string_view itunes_summary;
  std::string_view itunes_keywords;
};
REFLECTION(item_t, title, description, pubDate, enclosure, guid,
           itunes_explicit, dc_creator, media_thumbnail, media_content,
           itunes_subtitle, itunes_summary, itunes_keywords)

struct itunes_owner_t {
  std::string_view itunes_email;
};
REFLECTION(itunes_owner_t, itunes_email);

struct channel_t {
  std::string_view title;
  std::string_view link;
  std::string_view description;
  std::string_view generator;
  std::string_view docs;
  std::string_view language;
  std::string_view pubDate;
  std::string_view lastBuildDate;
  std::string_view itunes_author;
  std::string_view itunes_subtitle;
  std::string_view itunes_summary;
  std::string_view itunes_keywords;
  std::string_view itunes_image;
  std::string_view itunes_explicit;
  std::string_view itunes_block;
  std::string_view xmlns_feedburner;
  std::string_view media_thumbnail;
  std::string_view media_keywords;
  std::string_view media_category;
  itunes_owner_t itunes_owner;
  std::vector<item_t> item;
};
REFLECTION(channel_t, title, link, description, generator, docs, language,
           pubDate, lastBuildDate, itunes_author, itunes_subtitle,
           itunes_summary, itunes_keywords, itunes_image, itunes_explicit,
           itunes_block, xmlns_feedburner, media_thumbnail, media_keywords,
           media_category, itunes_owner, item);

struct rss_t {
  channel_t channel;
};
REFLECTION(rss_t, channel);
