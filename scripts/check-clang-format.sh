#!/usr/bin/env bash
set -euo pipefail

base_ref="${1:-HEAD}"
clang_format="${CLANG_FORMAT:-clang-format-18}"
git_clang_format="${GIT_CLANG_FORMAT:-git-clang-format-18}"

if ! command -v "$clang_format" >/dev/null 2>&1; then
  echo "error: $clang_format not found; install clang-format-18 or set CLANG_FORMAT." >&2
  exit 127
fi

if ! command -v "$git_clang_format" >/dev/null 2>&1; then
  echo "error: $git_clang_format not found; install clang-format-18 or set GIT_CLANG_FORMAT." >&2
  exit 127
fi

set +e
diff="$("$git_clang_format" --binary "$clang_format" --diff "$base_ref")"
status=$?
set -e

if [[ $status -ne 0 && -z "$diff" ]]; then
  exit "$status"
fi

if ! [[ "$diff" = "no modified files to format" ||
  "$diff" = "clang-format did not modify any files" ]]; then
  echo "The diff you sent is not formatted correctly."
  echo "the suggested format is"
  echo "$diff"
  exit 1
fi
