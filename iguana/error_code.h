#pragma once

#include <system_error>

namespace iguana {
enum class errc {
  ok = 0,
  failed_parse_number,
  number_is_too_long,
  lack_of_parenthesis, // lack of )
  lack_of_bracket,     // lack of ]
  lack_of_quote,       // lack of "
  lack_of_backslash,   // lack of /
  not_a_bool,
  not_a_digit,
  unexpected_end,
  unknown_key,
  empty_file,
  file_size_error,
  not_match_specific_chars,
};

class iguana_category : public std::error_category {
public:
  virtual const char *name() const noexcept override {
    return "iguna::category";
  }
  virtual std::string message(int err_val) const override {
    switch (static_cast<errc>(err_val)) {
    case errc::ok:
      return "ok";
    case errc::failed_parse_number:
      return "failed parse a number";
    case errc::number_is_too_long:
      return "number is too long";
    case errc::lack_of_parenthesis:
      return "lack of parenthesis";
    case errc::lack_of_bracket:
      return "lack of bracket";
    case errc::lack_of_quote:
      return "lack of quote";
    case errc::lack_of_backslash:
      return "lack of backslash";
    case errc::not_a_bool:
      return "not a bool";
    case errc::not_a_digit:
      return "not a digit";
    case errc::unexpected_end:
      return "unexpected end";
    case errc::unknown_key:
      return "unknown key";
    case errc::empty_file:
      return "empty file";
    case errc::file_size_error:
      return "file size error";
    case errc::not_match_specific_chars:
      return "not match some specific chars";

    default:
      return "(unrecognized error)";
    }
  }
};

const std::error_category &category() {
  static iguana::iguana_category instance;
  return instance;
}

inline std::error_code make_error_code(iguana::errc err) {
  return std::error_code((std::underlying_type_t<errc> &)err,
                         iguana::category());
}
} // namespace iguana
