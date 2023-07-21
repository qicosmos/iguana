#pragma once
#include "iguana/json_reader.hpp"
#include "iguana/json_writer.hpp"
#include "iguana/value.hpp"
#include <chrono>
#include <iostream>
#include <map>
#include <optional>
#include <tuple>
#include <vector>
#ifdef HAS_RAPIDJSON
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#endif

// canada.json
struct property_t {
  std::string_view name;
}; // Property
REFLECTION(property_t, name);

struct polygon_t {
  std::string_view type;
  std::vector<std::vector<std::array<iguana::numeric_str, 2>>> coordinates;
}; // Polygon
REFLECTION(polygon_t, type, coordinates);

struct feature_t {
  std::string_view type;
  property_t properties;
  polygon_t geometry;
}; // Feature
REFLECTION(feature_t, type, properties, geometry);

struct FeatureCollection {
  std::string_view type;
  std::vector<feature_t> features;
}; // FeatureCollection
REFLECTION(FeatureCollection, type, features);

// apache_builds.json
struct jobs_t {
  std::string_view name;
  std::string_view url;
  std::string_view color;
};
REFLECTION(jobs_t, name, url, color);

struct views_t {
  std::string_view name;
  std::string_view url;
};
REFLECTION(views_t, name, url);

struct apache_empty_t {};
REFLECTION_EMPTY(apache_empty_t);

struct apache_builds {
  std::vector<apache_empty_t> assignedLabels;
  std::string_view mode;
  std::string_view nodeDescription;
  std::string_view nodeName;
  iguana::numeric_str numExecutors;
  std::string_view description;
  std::vector<jobs_t> jobs;
  apache_empty_t overallLoad;
  views_t primaryView;
  bool quietingDown;
  iguana::numeric_str slaveAgentPort;
  apache_empty_t unlabeledLoad;
  bool useCrumbs;
  bool useSecurity;
  std::vector<views_t> views;
};
REFLECTION(apache_builds, assignedLabels, mode, nodeDescription, nodeName,
           numExecutors, description, jobs, overallLoad, primaryView,
           quietingDown, slaveAgentPort, unlabeledLoad, useCrumbs, useSecurity,
           views);

// citm_catalog.json
struct events_value_t {
  std::optional<std::string_view> description;
  iguana::numeric_str id;
  std::optional<std::string_view> logo;
  std::string_view name;
  std::vector<iguana::numeric_str> subTopicIds;
  std::optional<iguana::numeric_str> subjectCode;
  std::optional<std::string_view> subtitle;
  std::vector<iguana::numeric_str> topicIds;
}; // events_value_t
REFLECTION(events_value_t, description, id, logo, name, subTopicIds,
           subjectCode, subtitle, topicIds);

struct prices_element_t {
  iguana::numeric_str amount;
  iguana::numeric_str audienceSubCategoryId;
  iguana::numeric_str seatCategoryId;
}; // prices_element_t
REFLECTION(prices_element_t, amount, audienceSubCategoryId, seatCategoryId);

struct areas_element_t {
  iguana::numeric_str areaId;
  std::vector<iguana::numeric_str> blockIds;
}; // areas_element_t
REFLECTION(areas_element_t, areaId, blockIds);

struct seatCategories_element_t {
  std::vector<areas_element_t> areas;
  iguana::numeric_str seatCategoryId;
}; // seatCategories_element_t
REFLECTION(seatCategories_element_t, areas, seatCategoryId);

struct performances_element_t {
  iguana::numeric_str eventId;
  iguana::numeric_str id;
  std::optional<std::string_view> logo;
  std::optional<std::string_view> name;
  std::vector<prices_element_t> prices;
  std::vector<seatCategories_element_t> seatCategories;
  std::optional<std::string_view> seatMapImage;
  iguana::numeric_str start;
  std::string_view venueCode;
}; // performances_element_t
REFLECTION(performances_element_t, eventId, id, logo, name, prices,
           seatCategories, seatMapImage, start, venueCode);

struct venueNames_t {
  std::string_view PLEYEL_PLEYEL;
}; // venueNames_t
REFLECTION(venueNames_t, PLEYEL_PLEYEL);

struct citm_object_t {
  std::unordered_map<std::string_view, std::string_view> areaNames;
  std::unordered_map<std::string_view, std::string_view>
      audienceSubCategoryNames;
  apache_empty_t blockNames;
  std::unordered_map<std::string_view, events_value_t> events;
  std::vector<performances_element_t> performances;
  std::unordered_map<std::string_view, std::string_view> seatCategoryNames;
  std::unordered_map<std::string_view, std::string_view> subTopicNames;
  apache_empty_t subjectNames;
  std::unordered_map<std::string_view, std::string_view> topicNames;
  std::unordered_map<std::string_view, std::vector<iguana::numeric_str>>
      topicSubTopics;
  std::optional<venueNames_t> venueNames;
}; // citm_object_t
REFLECTION(citm_object_t, areaNames, audienceSubCategoryNames, blockNames,
           events, performances, seatCategoryNames, subTopicNames, subjectNames,
           topicNames, topicSubTopics, venueNames);

// gsoc-2018.json
struct sponsor_t {
  std::string_view type; //@
  std::string_view name;
  std::string_view disambiguatingDescription;
  std::string_view description;
  std::string_view url;
  std::string_view logo;
};
REFLECTION(sponsor_t, type, name, disambiguatingDescription, description, url,
           logo);

struct author_t {
  std::string_view type; //@
  std::string_view name;
};
REFLECTION(author_t, type, name);

struct gsoc_element_t {
  std::string_view context; //@
  std::string_view type;    //@
  std::string_view name;
  std::string_view description;
  sponsor_t sponsor;
  author_t author;
};
REFLECTION(gsoc_element_t, context, type, name, description, sponsor, author);

using gsoc_object_t = std::map<std::string_view, gsoc_element_t>;

// mesh.pretty.json
struct mesh_element_t {
  std::vector<iguana::numeric_str> indexRange;
  std::vector<iguana::numeric_str> usedBones;
  std::vector<iguana::numeric_str> vertexRange;
};
REFLECTION(mesh_element_t, indexRange, usedBones, vertexRange);

struct mesh_t {
  std::vector<mesh_element_t> batches;
  std::vector<iguana::numeric_str> colors;
  std::vector<iguana::numeric_str> indices;
  std::vector<std::vector<iguana::numeric_str>> influences;
  apache_empty_t morphTargets;
  std::vector<iguana::numeric_str> normals;
  std::vector<iguana::numeric_str> positions;
  std::vector<iguana::numeric_str> tex0;
};
REFLECTION(mesh_t, batches, colors, indices, influences, morphTargets, normals,
           positions, tex0);

// random.json
struct friend_t {
  iguana::numeric_str id;
  std::string_view name;
  std::string_view phone;
};
REFLECTION(friend_t, id, name, phone);

struct random_element_t {
  iguana::numeric_str id;
  std::string_view avatar;
  iguana::numeric_str age;
  bool admin;
  std::string_view name;
  std::string_view company;
  std::string_view phone;
  std::string_view email;
  std::string_view birthDate;
  std::vector<friend_t> friends;
  std::string_view field;
};
REFLECTION(random_element_t, id, avatar, age, admin, name, company, phone,
           email, birthDate, friends, field);

struct random_t {
  iguana::numeric_str id;
  std::string_view jsonrpc;
  iguana::numeric_str total;
  std::vector<random_element_t> result;
};
REFLECTION(random_t, id, jsonrpc, total, result);

// github_events.json
namespace githubEvents {
struct user_t {
  std::string_view gists_url;
  std::string_view gravatar_id;
  std::string_view url;
  std::string_view type;
  std::string_view avatar_url;
  std::string_view subscriptions_url;
  std::string_view organizations_url;
  std::string_view received_events_url;
  std::string_view repos_url;
  std::string_view login;
  std::string_view starred_url;
  iguana::numeric_str id;
  std::string_view events_url;
  std::string_view followers_url;
  std::string_view following_url;
};
REFLECTION(user_t, gists_url, gravatar_id, url, type, avatar_url,
           subscriptions_url, organizations_url, received_events_url, repos_url,
           login, starred_url, id, events_url, followers_url, following_url);

struct page_t {
  std::string_view page_name;
  std::string_view html_url;
  std::string_view title;
  std::string_view sha;
  std::optional<std::string_view> summary;
  std::string_view action;
};
REFLECTION(page_t, page_name, html_url, title, sha, summary, action);

struct pull_request_t {
  std::optional<std::string_view> html_url;
  std::optional<std::string_view> patch_url;
  std::optional<std::string_view> diff_url;
};
REFLECTION(pull_request_t, html_url, patch_url, diff_url);

struct forkee_t {
  std::string_view full_name;
  std::string_view stargazers_url;
  std::string_view clone_url;
  bool fork;
  std::string_view url;
  std::string_view tags_url;
  std::string_view description;
  std::string_view merges_url;
  iguana::numeric_str forks;
  std::string_view language;
  bool ___private;
  std::string_view archive_url;
  std::string_view collaborators_url;
  std::string_view languages_url;
  user_t owner;
  std::string_view git_refs_url;
  std::string_view labels_url;
  std::string_view pushed_at;
  std::string_view html_url;
  std::string_view trees_url;
  std::string_view forks_url;
  std::string_view commits_url;
  std::string_view branches_url;
  std::string_view notifications_url;
  std::string_view created_at;
  bool has_issues;
  std::string_view blobs_url;
  std::string_view issues_url;
  std::string_view compare_url;
  iguana::numeric_str open_issues;
  std::string_view contents_url;
  std::string_view name;
  std::string_view statuses_url;
  std::string_view assignees_url;
  iguana::numeric_str forks_count;
  std::string_view updated_at;
  std::string_view issue_events_url;
  std::string_view ssh_url;
  std::string_view subscribers_url;
  std::optional<std::string_view> mirror_url;
  bool ___public;
  bool has_wiki;
  std::string_view git_commits_url;
  std::string_view downloads_url;
  iguana::numeric_str id;
  std::string_view pulls_url;
  bool has_downloads;
  std::string_view issue_comment_url;
  iguana::numeric_str watchers_count;
  std::optional<std::string_view> homepage;
  std::string_view hooks_url;
  std::string_view subscription_url;
  std::string_view milestones_url;
  std::string_view events_url;
  std::string_view svn_url;
  std::string_view git_tags_url;
  std::string_view teams_url;
  std::string_view comments_url;
  iguana::numeric_str open_issues_count;
  std::string_view keys_url;
  std::string_view contributors_url;
  iguana::numeric_str size;
  iguana::numeric_str watchers;
  std::string_view git_url;
};
REFLECTION(forkee_t, full_name, stargazers_url, clone_url, fork, url, tags_url,
           description, merges_url, forks, language, ___private, archive_url,
           collaborators_url, languages_url, owner, git_refs_url, labels_url,
           pushed_at, html_url, trees_url, forks_url, commits_url, branches_url,
           notifications_url, created_at, has_issues, blobs_url, issues_url,
           open_issues, contents_url, name, statuses_url, assignees_url,
           forks_count, updated_at, issue_events_url, ssh_url, subscribers_url,
           mirror_url, ___public, has_wiki, git_commits_url, downloads_url, id,
           pulls_url, has_downloads, issue_comment_url, watchers_count,
           homepage, hooks_url, subscription_url, milestones_url, events_url,
           svn_url, git_tags_url, teams_url, comments_url, open_issues_count,
           keys_url, contributors_url, size, watchers, git_url, compare_url);

struct issue_t {
  user_t user;
  std::string_view url;
  std::vector<std::string_view> labels;
  std::string_view html_url;
  std::string_view labels_url;
  pull_request_t pull_request;
  std::string_view created_at;
  std::optional<std::string_view> closed_at;
  std::optional<std::string_view> milestone;
  std::string_view title;
  std::string_view body;
  std::string_view updated_at;
  iguana::numeric_str number;
  std::string_view state;
  std::optional<user_t> assignee;
  iguana::numeric_str id;
  iguana::numeric_str comments;
  std::string_view events_url;
  std::string_view comments_url;
};
REFLECTION(issue_t, user, url, labels, html_url, labels_url, pull_request,
           created_at, closed_at, milestone, title, body, updated_at, number,
           state, assignee, id, comments, events_url, comments_url);

struct comment_t {
  user_t user;
  std::string_view url;
  std::string_view issue_url;
  std::string_view created_at;
  std::string_view body;
  std::string_view updated_at;
  iguana::numeric_str id;
};
REFLECTION(comment_t, user, url, issue_url, created_at, body, updated_at, id);

struct actor_org_t {
  std::string_view gravatar_id;
  std::string_view login;
  std::string_view avatar_url;
  std::string_view url;
  iguana::numeric_str id;
};
REFLECTION(actor_org_t, gravatar_id, login, avatar_url, url, id);

struct repo_t {
  std::string_view url;
  iguana::numeric_str id;
  std::string_view name;
};
REFLECTION(repo_t, url, id, name);

struct author_t {
  std::string_view email;
  std::string_view name;
};
REFLECTION(author_t, email, name);

struct commit_t {
  std::string_view url;
  std::string_view message;
  bool distinct;
  std::string_view sha;
  author_t author;
};
REFLECTION(commit_t, url, message, distinct, sha, author);

struct payload_t {
  std::optional<std::vector<commit_t>> commits;
  std::optional<iguana::numeric_str> distinct_size;
  std::optional<std::string_view> ref;
  std::optional<iguana::numeric_str> push_id;
  std::optional<std::string_view> head;
  std::optional<std::string_view> before;
  std::optional<iguana::numeric_str> size;
  std::optional<forkee_t> forkee;
  std::optional<std::vector<page_t>> pages;
  std::optional<std::string_view> action;
  std::optional<comment_t> comment;
  std::optional<issue_t> issue;
  std::optional<std::string_view> description;
  std::optional<std::string_view> master_branch;

  std::optional<std::string_view> ref_type;
};
REFLECTION(payload_t, commits, distinct_size, ref, push_id, head, before, size,
           forkee, pages, action, comment, issue, description, master_branch,
           ref_type);

struct event_t {
  std::string_view type;
  std::string_view created_at;
  std::optional<actor_org_t> actor;
  repo_t repo;
  bool ___public;
  std::optional<actor_org_t> org;
  payload_t payload;
  std::string_view id;
};
REFLECTION(event_t, type, created_at, actor, repo, ___public, org, payload, id);

using events_t = std::vector<event_t>;
} // namespace githubEvents

namespace marine_ik {
struct image_element_t {
  std::string_view url;
  std::string_view uuid;
  std::string_view name;
};
REFLECTION(image_element_t, url, uuid, name);

struct item_t {
  std::string_view name;
  std::string_view type;
  std::string_view uuid;
};
REFLECTION(item_t, name, type, uuid);

struct key_element_t {
  std::array<iguana::numeric_str, 4> rot;
  iguana::numeric_str time{};
  std::array<iguana::numeric_str, 3> scl;
  std::array<iguana::numeric_str, 3> pos;
};
REFLECTION(key_element_t, rot, time, scl, pos);

struct hierarchy_element_t {
  iguana::numeric_str parent;
  std::vector<key_element_t> keys;
};
REFLECTION(hierarchy_element_t, parent, keys);

struct geo_anim_element_t {
  std::vector<hierarchy_element_t> hierarchy;
  iguana::numeric_str length{};
  iguana::numeric_str fps{};
  std::string_view name;
};
REFLECTION(geo_anim_element_t, hierarchy, length, fps, name);

struct bone_element_t {
  iguana::numeric_str parent;
  std::array<iguana::numeric_str, 3> pos;
  std::array<iguana::numeric_str, 4> rotq;
  std::array<iguana::numeric_str, 3> scl;
  std::string_view name;
};
REFLECTION(bone_element_t, parent, pos, rotq, scl, name);

struct geo_meta_data_t {
  iguana::numeric_str uvs;
  iguana::numeric_str version;
  iguana::numeric_str faces;
  std::string_view generator;
  iguana::numeric_str normals;
  iguana::numeric_str bones;
  iguana::numeric_str vertices;
};
REFLECTION(geo_meta_data_t, uvs, version, faces, generator, normals, bones,
           vertices);

struct geo_data_t {
  std::vector<std::vector<iguana::numeric_str>> uvs;
  std::vector<geo_anim_element_t> animations;
  std::vector<iguana::numeric_str> vertices;
  geo_meta_data_t metadata;
  std::string_view name;
  std::vector<iguana::numeric_str> skinWeights;
  std::vector<iguana::numeric_str> skinIndices;
  iguana::numeric_str influencesPerVertex{};
  std::vector<iguana::numeric_str> normals;
  std::vector<bone_element_t> bones;
  std::vector<iguana::numeric_str> faces;
};
REFLECTION(geo_data_t, uvs, animations, vertices, metadata, name, skinWeights,
           skinIndices, influencesPerVertex, normals, bones, faces);

struct geometry_element_t {
  std::string_view type;
  std::string_view uuid;
  geo_data_t data;
};
REFLECTION(geometry_element_t, type, uuid, data);

struct texture_element_t {
  std::array<iguana::numeric_str, 2> repeat;
  std::array<iguana::numeric_str, 2> wrap;
  iguana::numeric_str anisotropy{};
  std::string_view image;
  std::string_view name;
  iguana::numeric_str mapping{};
  iguana::numeric_str minFilter{};
  std::string_view uuid;
  iguana::numeric_str magFilter{};
};
REFLECTION(texture_element_t, repeat, wrap, anisotropy, image, name, mapping,
           minFilter, uuid, magFilter);

struct meta_data_t {
  std::string_view sourceFile;
  std::string_view generator;
  std::string_view type;
  iguana::numeric_str version{};
};
REFLECTION(meta_data_t, sourceFile, generator, type, version);

struct material_element_t : item_t {
  iguana::numeric_str vertexColors{};
  std::string_view blending;
  std::string_view map;
  bool transparent{};
  bool depthTest{};
  iguana::numeric_str color;
  iguana::numeric_str shininess;
  iguana::numeric_str emissive;
  bool depthWrite{};
  iguana::numeric_str specular{};
};
REFLECTION(material_element_t, vertexColors, name, type, uuid, blending, map,
           transparent, depthTest, color, shininess, emissive, depthWrite,
           specular);

struct obj_child_t : item_t {
  std::array<iguana::numeric_str, 16> matrix;
  bool visible{};
  std::string_view material;
  bool castShadow{};
  bool receiveShadow{};
  std::string_view geometry;
};
REFLECTION(obj_child_t, name, uuid, matrix, visible, type, material, castShadow,
           receiveShadow, geometry);

struct object_t {
  std::vector<obj_child_t> children;
  std::string_view type;
  std::array<iguana::numeric_str, 16> matrix;
  std::string_view uuid;
};
REFLECTION(object_t, children, type, matrix, uuid);

struct animation_element_t {
  std::vector<iguana::numeric_str> tracks;
  iguana::numeric_str fps;
  std::string_view name;
};
REFLECTION(animation_element_t, tracks, fps, name);

struct marine_ik_t {
  std::vector<image_element_t> images;
  std::vector<geometry_element_t> geometries;
  std::vector<texture_element_t> textures;
  meta_data_t metadata;
  std::vector<material_element_t> materials;
  object_t object;
  std::vector<animation_element_t> animations;
};
REFLECTION(marine_ik_t, images, geometries, textures, metadata, materials,
           object, animations);
} // namespace marine_ik

// instruments.json
struct sample_element {
  iguana::numeric_str c5_samplerate;
  iguana::numeric_str global_volume;
  std::string_view legacy_filename;
  iguana::numeric_str length;
  iguana::numeric_str loop_end;
  iguana::numeric_str loop_start;
  std::string_view name;
  iguana::numeric_str pan;
  iguana::numeric_str sustain_end;
  iguana::numeric_str sustain_start;
  iguana::numeric_str vibrato_depth;
  iguana::numeric_str vibrato_rate;
  iguana::numeric_str vibrato_sweep;
  iguana::numeric_str vibrato_type;
  iguana::numeric_str volume;
};
REFLECTION(sample_element, c5_samplerate, global_volume, legacy_filename,
           length, loop_end, loop_start, name, pan, sustain_end, sustain_start,
           vibrato_depth, vibrato_rate, vibrato_sweep, vibrato_type, volume);

struct data_t {
  iguana::numeric_str channel;
  iguana::numeric_str fxcmd;
  iguana::numeric_str fxparam;
  iguana::numeric_str instr;
  iguana::numeric_str note;
  iguana::numeric_str row;
  iguana::numeric_str volcmd;
  iguana::numeric_str volval;
};
REFLECTION(data_t, channel, fxcmd, fxparam, instr, note, row, volcmd, volval);

struct pattern_element {
  std::optional<std::vector<data_t>> data;
  std::string_view name;
  iguana::numeric_str rows;
  iguana::numeric_str rows_per_beat;
  iguana::numeric_str rows_per_measure;
};
REFLECTION(pattern_element, data, name, rows, rows_per_beat, rows_per_measure);

struct node_t {
  iguana::numeric_str tick;
  iguana::numeric_str value;
};
REFLECTION(node_t, tick, value);

struct panning_envelope_t {
  iguana::numeric_str loop_end;
  iguana::numeric_str loop_start;
  std::vector<node_t> nodes;
  iguana::numeric_str release_node;
  iguana::numeric_str sustain_end;
  iguana::numeric_str sustain_start;
};
REFLECTION(panning_envelope_t, loop_end, loop_start, nodes, release_node,
           sustain_end, sustain_start);

struct instrument_element {
  iguana::numeric_str default_filter_cutoff;
  bool default_filter_cutoff_enabled;
  iguana::numeric_str default_filter_mode;
  iguana::numeric_str default_filter_resonance;
  bool default_filter_resonance_enabled;
  iguana::numeric_str default_pan;
  iguana::numeric_str duplicate_check_type;
  iguana::numeric_str duplicate_note_action;
  iguana::numeric_str fadeout;
  iguana::numeric_str global_volume;
  iguana::numeric_str graph_insert;
  std::string_view legacy_filename;
  iguana::numeric_str midi_bank;
  iguana::numeric_str midi_channel;
  iguana::numeric_str midi_drum_set;
  iguana::numeric_str midi_program;
  std::string_view name;
  iguana::numeric_str new_note_action;
  std::optional<iguana::numeric_str> note_map;
  panning_envelope_t panning_envelope;
  panning_envelope_t pitch_envelope;

  iguana::numeric_str pitch_pan_center;
  iguana::numeric_str pitch_pan_separation;
  iguana::numeric_str pitch_to_tempo_lock;
  iguana::numeric_str random_cutoff_weight;
  iguana::numeric_str random_pan_weight;
  iguana::numeric_str random_resonance_weight;
  iguana::numeric_str random_volume_weight;
  std::optional<iguana::numeric_str> sample_map;
  std::optional<iguana::numeric_str> tuning;

  panning_envelope_t volume_envelope;
  iguana::numeric_str volume_ramp_down;
  iguana::numeric_str volume_ramp_up;
};
REFLECTION(instrument_element, default_filter_cutoff,
           default_filter_cutoff_enabled, default_filter_mode,
           default_filter_resonance, default_filter_resonance_enabled,
           default_pan, duplicate_check_type, duplicate_note_action, fadeout,
           global_volume, graph_insert, legacy_filename, midi_bank,
           midi_channel, midi_drum_set, midi_program, name, new_note_action,
           note_map, panning_envelope, pitch_envelope, pitch_pan_center,
           pitch_pan_separation, pitch_to_tempo_lock, random_cutoff_weight,
           random_pan_weight, random_resonance_weight, random_volume_weight,
           sample_map, tuning, volume_envelope, volume_ramp_down,
           volume_ramp_up);

struct instruments_t {
  std::optional<iguana::numeric_str> graphstate;
  std::vector<instrument_element> instruments;
  std::optional<std::string_view> message;
  std::string_view name;
  std::optional<std::string_view> orderlist;
  std::vector<pattern_element> patterns;
  std::optional<iguana::numeric_str> pluginstate;
  std::vector<sample_element> samples;
  iguana::numeric_str version;
};
REFLECTION(instruments_t, graphstate, instruments, message, name, orderlist,
           patterns, pluginstate, samples, version);
