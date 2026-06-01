#pragma once

#include <optional>
#include <string>
#include <vector>

#include "iguana/ylt/reflection/user_reflect_macro.hpp"

namespace githubEvents {
struct user_t {
  std::string gists_url;
  std::string gravatar_id;
  std::string url;
  std::string type;
  std::string avatar_url;
  std::string subscriptions_url;
  std::string organizations_url;
  std::string received_events_url;
  std::string repos_url;
  std::string login;
  std::string starred_url;
  int id;
  std::string events_url;
  std::string followers_url;
  std::string following_url;
};
YLT_REFL(user_t, gists_url, gravatar_id, url, type, avatar_url,
         subscriptions_url, organizations_url, received_events_url, repos_url,
         login, starred_url, id, events_url, followers_url, following_url);

struct page_t {
  std::string page_name;
  std::string html_url;
  std::string title;
  std::string sha;
  std::optional<std::string> summary;
  std::string action;
};
YLT_REFL(page_t, page_name, html_url, title, sha, summary, action);

struct pull_request_t {
  std::optional<std::string> html_url;
  std::optional<std::string> patch_url;
  std::optional<std::string> diff_url;
};
YLT_REFL(pull_request_t, html_url, patch_url, diff_url);

struct forkee_t {
  std::string full_name;
  std::string stargazers_url;
  std::string clone_url;
  bool fork;
  std::string url;
  std::string tags_url;
  std::string description;
  std::string merges_url;
  int forks;
  std::string language;
  bool ___private;
  std::string archive_url;
  std::string collaborators_url;
  std::string languages_url;
  user_t owner;
  std::string git_refs_url;
  std::string labels_url;
  std::string pushed_at;
  std::string html_url;
  std::string trees_url;
  std::string forks_url;
  std::string commits_url;
  std::string branches_url;
  std::string notifications_url;
  std::string created_at;
  bool has_issues;
  std::string blobs_url;
  std::string issues_url;
  std::string compare_url;
  int open_issues;
  std::string contents_url;
  std::string name;
  std::string statuses_url;
  std::string assignees_url;
  int forks_count;
  std::string updated_at;
  std::string issue_events_url;
  std::string ssh_url;
  std::string subscribers_url;
  std::optional<std::string> mirror_url;
  bool ___public;
  bool has_wiki;
  std::string git_commits_url;
  std::string downloads_url;
  int id;
  std::string pulls_url;
  bool has_downloads;
  std::string issue_comment_url;
  int watchers_count;
  std::optional<std::string> homepage;
  std::string hooks_url;
  std::string subscription_url;
  std::string milestones_url;
  std::string events_url;
  std::string svn_url;
  std::string git_tags_url;
  std::string teams_url;
  std::string comments_url;
  int open_issues_count;
  std::string keys_url;
  std::string contributors_url;
  int size;
  int watchers;
  std::string git_url;
};
YLT_REFL(forkee_t, full_name, stargazers_url, clone_url, fork, url, tags_url,
         description, merges_url, forks, language, ___private, archive_url,
         collaborators_url, languages_url, owner, git_refs_url, labels_url,
         pushed_at, html_url, trees_url, forks_url, commits_url, branches_url,
         notifications_url, created_at, has_issues, blobs_url, issues_url,
         open_issues, contents_url, name, statuses_url, assignees_url,
         forks_count, updated_at, issue_events_url, ssh_url, subscribers_url,
         mirror_url, ___public, has_wiki, git_commits_url, downloads_url, id,
         pulls_url, has_downloads, issue_comment_url, watchers_count, homepage,
         hooks_url, subscription_url, milestones_url, events_url, svn_url,
         git_tags_url, teams_url, comments_url, open_issues_count, keys_url,
         contributors_url, size, watchers, git_url, compare_url);

struct issue_t {
  user_t user;
  std::string url;
  std::vector<std::string> labels;
  std::string html_url;
  std::string labels_url;
  pull_request_t pull_request;
  std::string created_at;
  std::optional<std::string> closed_at;
  std::optional<std::string> milestone;
  std::string title;
  std::string body;
  std::string updated_at;
  int number;
  std::string state;
  std::optional<user_t> assignee;
  int id;
  int comments;
  std::string events_url;
  std::string comments_url;
};
YLT_REFL(issue_t, user, url, labels, html_url, labels_url, pull_request,
         created_at, closed_at, milestone, title, body, updated_at, number,
         state, assignee, id, comments, events_url, comments_url);

struct comment_t {
  user_t user;
  std::string url;
  std::string issue_url;
  std::string created_at;
  std::string body;
  std::string updated_at;
  int id;
};
YLT_REFL(comment_t, user, url, issue_url, created_at, body, updated_at, id);

struct actor_org_t {
  std::string gravatar_id;
  std::string login;
  std::string avatar_url;
  std::string url;
  int id;
};
YLT_REFL(actor_org_t, gravatar_id, login, avatar_url, url, id);

struct repo_t {
  std::string url;
  int id;
  std::string name;
};
YLT_REFL(repo_t, url, id, name);

struct author_t {
  std::string email;
  std::string name;
};
YLT_REFL(author_t, email, name);

struct commit_t {
  std::string url;
  std::string message;
  bool distinct;
  std::string sha;
  author_t author;
};
YLT_REFL(commit_t, url, message, distinct, sha, author);

struct payload_t {
  std::optional<std::vector<commit_t>> commits;
  std::optional<int> distinct_size;
  std::optional<std::string> ref;
  std::optional<int> push_id;
  std::optional<std::string> head;
  std::optional<std::string> before;
  std::optional<int> size;
  std::optional<forkee_t> forkee;
  std::optional<std::vector<page_t>> pages;
  std::optional<std::string> action;
  std::optional<comment_t> comment;
  std::optional<issue_t> issue;
  std::optional<std::string> description;
  std::optional<std::string> master_branch;

  std::optional<std::string> ref_type;
};
YLT_REFL(payload_t, commits, distinct_size, ref, push_id, head, before, size,
         forkee, pages, action, comment, issue, description, master_branch,
         ref_type);

struct event_t {
  std::string type;
  std::string created_at;
  std::optional<actor_org_t> actor;
  repo_t repo;
  bool ___public;
  std::optional<actor_org_t> org;
  payload_t payload;
  std::string id;
};
YLT_REFL(event_t, type, created_at, actor, repo, ___public, org, payload, id);

using events_t = std::vector<event_t>;
}  // namespace githubEvents
