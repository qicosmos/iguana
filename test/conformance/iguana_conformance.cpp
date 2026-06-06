#include <array>
#include <cstdint>
#include <exception>
#include <iostream>
#include <map>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "iguana/pb_util.hpp"

namespace {

enum class WireFormat : uint32_t {
  unspecified = 0,
  protobuf = 1,
  json = 2,
  jspb = 3,
  text_format = 4,
};

enum class TestCategory : uint32_t {
  unspecified = 0,
  binary = 1,
  json = 2,
  json_ignore_unknown = 3,
  jspb = 4,
  text_format = 5,
};

enum class PayloadCase {
  none,
  protobuf,
  json,
  jspb,
  text,
};

struct ConformanceRequest {
  PayloadCase payload_case = PayloadCase::none;
  std::string payload;
  WireFormat requested_output_format = WireFormat::unspecified;
  std::string message_type;
  TestCategory test_category = TestCategory::unspecified;
  bool print_unknown_fields = false;
};

enum class ScalarKind {
  int32,
  int64,
  uint32,
  uint64,
  sint32,
  sint64,
  fixed32,
  fixed64,
  sfixed32,
  sfixed64,
  float32,
  float64,
  boolean,
  enumeration,
  null_value,
};

enum class MessageKind {
  test_all_types,
  nested_message,
  foreign_message,
  bool_wrapper,
  int32_wrapper,
  int64_wrapper,
  uint32_wrapper,
  uint64_wrapper,
  float_wrapper,
  double_wrapper,
  string_wrapper,
  bytes_wrapper,
  duration,
  timestamp,
  field_mask,
  any,
  struct_value,
  value,
  list_value,
  map_int32_int32,
  map_int64_int64,
  map_uint32_uint32,
  map_uint64_uint64,
  map_sint32_sint32,
  map_sint64_sint64,
  map_fixed32_fixed32,
  map_fixed64_fixed64,
  map_sfixed32_sfixed32,
  map_sfixed64_sfixed64,
  map_int32_float,
  map_int32_double,
  map_bool_bool,
  map_string_string,
  map_string_bytes,
  map_string_nested_message,
  map_string_foreign_message,
  map_string_nested_enum,
  map_string_foreign_enum,
  struct_fields_entry,
};

struct FieldSpec {
  uint32_t field_number;
  bool string_value = false;
  bool bytes_value = false;
  std::optional<ScalarKind> scalar;
  std::optional<MessageKind> message;
  std::optional<ScalarKind> packed_scalar;
  std::optional<ScalarKind> unpacked_scalar;
};

constexpr uint32_t to_wire(iguana::WireType wire_type) {
  return static_cast<uint32_t>(wire_type);
}

void append_varint(std::string& out, uint64_t value) {
  while (value >= 0x80) {
    out.push_back(static_cast<char>((value & 0x7fU) | 0x80U));
    value >>= 7U;
  }
  out.push_back(static_cast<char>(value));
}

void append_tag(std::string& out, uint32_t field_no,
                iguana::WireType wire_type) {
  append_varint(out, (static_cast<uint64_t>(field_no) << 3U) |
                         static_cast<uint32_t>(wire_type));
}

void append_fixed32(std::string& out, uint32_t value) {
  for (int i = 0; i < 4; ++i) {
    out.push_back(static_cast<char>((value >> (8 * i)) & 0xffU));
  }
}

void append_fixed64(std::string& out, uint64_t value) {
  for (int i = 0; i < 8; ++i) {
    out.push_back(static_cast<char>((value >> (8 * i)) & 0xffU));
  }
}

void append_value(std::string& out, uint32_t field_no,
                  iguana::WireType wire_type, std::string_view payload) {
  append_tag(out, field_no, wire_type);
  out.append(payload.data(), payload.size());
}

void append_length_delimited(std::string& out, uint32_t field_no,
                             std::string_view value) {
  append_tag(out, field_no, iguana::WireType::LengthDelimeted);
  append_varint(out, value.size());
  out.append(value.data(), value.size());
}

void append_string_field(std::string& out, uint32_t field_no,
                         std::string_view value) {
  append_length_delimited(out, field_no, value);
}

uint64_t read_varint(std::string_view& in) {
  size_t pos = 0;
  uint64_t value = iguana::detail::decode_varint(in, pos);
  in.remove_prefix(pos);
  return value;
}

std::string_view read_length_delimited(std::string_view& in,
                                       const char* error) {
  size_t size = iguana::detail::decode_length_delimited(in, error);
  std::string_view value = in.substr(0, size);
  in.remove_prefix(size);
  return value;
}

bool valid_utf8(std::string_view s) {
  const auto* p = reinterpret_cast<const unsigned char*>(s.data());
  size_t i = 0;
  while (i < s.size()) {
    uint32_t cp = 0;
    unsigned char c = p[i];
    size_t extra = 0;
    if (c <= 0x7f) {
      ++i;
      continue;
    }
    if (c >= 0xc2 && c <= 0xdf) {
      cp = c & 0x1fU;
      extra = 1;
    }
    else if (c >= 0xe0 && c <= 0xef) {
      cp = c & 0x0fU;
      extra = 2;
    }
    else if (c >= 0xf0 && c <= 0xf4) {
      cp = c & 0x07U;
      extra = 3;
    }
    else {
      return false;
    }
    if (i + extra >= s.size()) {
      return false;
    }
    for (size_t j = 1; j <= extra; ++j) {
      unsigned char cc = p[i + j];
      if ((cc & 0xc0U) != 0x80U) {
        return false;
      }
      cp = (cp << 6U) | (cc & 0x3fU);
    }
    if ((extra == 2 && cp < 0x800U) || (extra == 3 && cp < 0x10000U) ||
        (cp >= 0xd800U && cp <= 0xdfffU) || cp > 0x10ffffU) {
      return false;
    }
    i += extra + 1;
  }
  return true;
}

iguana::WireType scalar_wire_type(ScalarKind kind) {
  switch (kind) {
    case ScalarKind::fixed32:
    case ScalarKind::sfixed32:
    case ScalarKind::float32:
      return iguana::WireType::Fixed32;
    case ScalarKind::fixed64:
    case ScalarKind::sfixed64:
    case ScalarKind::float64:
      return iguana::WireType::Fixed64;
    default:
      return iguana::WireType::Varint;
  }
}

size_t fixed_size(ScalarKind kind) {
  switch (kind) {
    case ScalarKind::fixed32:
    case ScalarKind::sfixed32:
    case ScalarKind::float32:
      return 4;
    case ScalarKind::fixed64:
    case ScalarKind::sfixed64:
    case ScalarKind::float64:
      return 8;
    default:
      return 0;
  }
}

void consume_scalar(std::string_view& in, iguana::WireType wire_type,
                    ScalarKind kind) {
  iguana::WireType expected = scalar_wire_type(kind);
  if (wire_type != expected) {
    throw std::invalid_argument("wrong wire type for scalar field");
  }
  if (expected == iguana::WireType::Varint) {
    (void)read_varint(in);
    return;
  }
  size_t size = fixed_size(kind);
  if (in.size() < size) {
    throw std::invalid_argument("truncated fixed-width scalar");
  }
  in.remove_prefix(size);
}

void consume_packed(std::string_view payload, ScalarKind kind) {
  size_t size = fixed_size(kind);
  if (size != 0) {
    if (payload.size() % size != 0) {
      throw std::invalid_argument("bad packed fixed-width length");
    }
    return;
  }
  while (!payload.empty()) {
    (void)read_varint(payload);
  }
}

struct ScalarPayload {
  std::string bytes;
  bool is_default = false;
};

struct StoredField {
  FieldSpec spec;
  bool present = false;
  bool repeated = false;
  bool packed_output = false;
  bool oneof = false;
  std::string singular;
  std::vector<std::string> values;
};

struct CanonicalMessage {
  std::map<uint32_t, StoredField> fields;
  std::vector<std::string> unknown_fields;
};

uint32_t read_fixed32(std::string_view& in) {
  if (in.size() < 4) {
    throw std::invalid_argument("truncated fixed32");
  }
  uint32_t value = 0;
  for (int i = 0; i < 4; ++i) {
    value |= static_cast<uint32_t>(
                 static_cast<unsigned char>(in[static_cast<size_t>(i)]))
             << (8 * i);
  }
  in.remove_prefix(4);
  return value;
}

uint64_t read_fixed64(std::string_view& in) {
  if (in.size() < 8) {
    throw std::invalid_argument("truncated fixed64");
  }
  uint64_t value = 0;
  for (int i = 0; i < 8; ++i) {
    value |= static_cast<uint64_t>(
                 static_cast<unsigned char>(in[static_cast<size_t>(i)]))
             << (8 * i);
  }
  in.remove_prefix(8);
  return value;
}

void append_int32_payload(std::string& out, int32_t value) {
  if (value < 0) {
    append_varint(out, static_cast<uint64_t>(static_cast<int64_t>(value)));
  }
  else {
    append_varint(out, static_cast<uint32_t>(value));
  }
}

uint32_t zigzag32(int32_t value) {
  return (static_cast<uint32_t>(value) << 1U) ^
         static_cast<uint32_t>(value >> 31);
}

uint64_t zigzag64(int64_t value) {
  return (static_cast<uint64_t>(value) << 1U) ^
         static_cast<uint64_t>(value >> 63);
}

int32_t unzigzag32(uint32_t value) {
  return static_cast<int32_t>((value >> 1U) ^ (~(value & 1U) + 1U));
}

int64_t unzigzag64(uint64_t value) {
  return static_cast<int64_t>((value >> 1U) ^ (~(value & 1U) + 1U));
}

ScalarPayload read_scalar_payload(std::string_view& in, ScalarKind kind) {
  ScalarPayload result;
  switch (kind) {
    case ScalarKind::int32: {
      int32_t value =
          static_cast<int32_t>(static_cast<uint32_t>(read_varint(in)));
      append_int32_payload(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::int64: {
      int64_t value = static_cast<int64_t>(read_varint(in));
      append_varint(result.bytes, static_cast<uint64_t>(value));
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::uint32: {
      uint32_t value = static_cast<uint32_t>(read_varint(in));
      append_varint(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::uint64: {
      uint64_t value = read_varint(in);
      append_varint(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::sint32: {
      int32_t value = unzigzag32(static_cast<uint32_t>(read_varint(in)));
      append_varint(result.bytes, zigzag32(value));
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::sint64: {
      int64_t value = unzigzag64(read_varint(in));
      append_varint(result.bytes, zigzag64(value));
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::fixed32:
    case ScalarKind::sfixed32:
    case ScalarKind::float32: {
      uint32_t value = read_fixed32(in);
      append_fixed32(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::fixed64:
    case ScalarKind::sfixed64:
    case ScalarKind::float64: {
      uint64_t value = read_fixed64(in);
      append_fixed64(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
    case ScalarKind::boolean: {
      bool value = read_varint(in) != 0;
      append_varint(result.bytes, value ? 1U : 0U);
      result.is_default = !value;
      return result;
    }
    case ScalarKind::enumeration:
    case ScalarKind::null_value: {
      int32_t value =
          static_cast<int32_t>(static_cast<uint32_t>(read_varint(in)));
      append_int32_payload(result.bytes, value);
      result.is_default = value == 0;
      return result;
    }
  }
  throw std::invalid_argument("unknown scalar kind");
}

void append_stored_field(CanonicalMessage& msg, uint32_t field_number,
                         const FieldSpec& spec, std::string payload,
                         bool repeated, bool packed_output, bool oneof,
                         bool is_default) {
  auto& field = msg.fields[field_number];
  field.spec = spec;
  field.repeated = repeated;
  field.packed_output = packed_output;
  field.oneof = oneof;
  if (repeated) {
    field.present = true;
    field.values.emplace_back(std::move(payload));
  }
  else {
    field.present = oneof || !is_default;
    field.singular = std::move(payload);
    if (!field.present) {
      field.singular.clear();
    }
  }
}

void validate_message(std::string_view data, MessageKind kind,
                      uint32_t depth = 0);

void skip_group(std::string_view& in, uint32_t field_number, uint32_t depth) {
  if (depth > 100) {
    throw std::invalid_argument("protobuf recursion limit exceeded");
  }
  while (!in.empty()) {
    auto key = iguana::detail::decode_key(in, true);
    in.remove_prefix(key.tag_size);
    if (key.wire_type == iguana::WireType::EndGroup) {
      if (key.field_number != field_number) {
        throw std::invalid_argument("protobuf end group field mismatch");
      }
      return;
    }
    if (key.wire_type == iguana::WireType::StartGroup) {
      skip_group(in, key.field_number, depth + 1);
    }
    else if (key.wire_type == iguana::WireType::Varint) {
      (void)read_varint(in);
    }
    else if (key.wire_type == iguana::WireType::Fixed64) {
      if (in.size() < 8) {
        throw std::invalid_argument("truncated unknown fixed64");
      }
      in.remove_prefix(8);
    }
    else if (key.wire_type == iguana::WireType::LengthDelimeted) {
      (void)read_length_delimited(in, "truncated unknown length-delimited");
    }
    else if (key.wire_type == iguana::WireType::Fixed32) {
      if (in.size() < 4) {
        throw std::invalid_argument("truncated unknown fixed32");
      }
      in.remove_prefix(4);
    }
  }
  throw std::invalid_argument("unterminated protobuf group");
}

void skip_unknown(std::string_view& in, iguana::WireType wire_type,
                  uint32_t field_number, uint32_t depth) {
  switch (wire_type) {
    case iguana::WireType::Varint:
      (void)read_varint(in);
      return;
    case iguana::WireType::Fixed64:
      if (in.size() < 8) {
        throw std::invalid_argument("truncated unknown fixed64");
      }
      in.remove_prefix(8);
      return;
    case iguana::WireType::LengthDelimeted:
      (void)read_length_delimited(in, "truncated unknown length-delimited");
      return;
    case iguana::WireType::StartGroup:
      skip_group(in, field_number, depth + 1);
      return;
    case iguana::WireType::Fixed32:
      if (in.size() < 4) {
        throw std::invalid_argument("truncated unknown fixed32");
      }
      in.remove_prefix(4);
      return;
    case iguana::WireType::EndGroup:
    default:
      throw std::invalid_argument("unexpected end group");
  }
}

FieldSpec map_entry_spec(MessageKind kind, uint32_t field_number) {
  switch (kind) {
    case MessageKind::map_int32_int32:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::int32}
                               : FieldSpec{2, false, false, ScalarKind::int32};
    case MessageKind::map_int64_int64:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::int64}
                               : FieldSpec{2, false, false, ScalarKind::int64};
    case MessageKind::map_uint32_uint32:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::uint32}
                               : FieldSpec{2, false, false, ScalarKind::uint32};
    case MessageKind::map_uint64_uint64:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::uint64}
                               : FieldSpec{2, false, false, ScalarKind::uint64};
    case MessageKind::map_sint32_sint32:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::sint32}
                               : FieldSpec{2, false, false, ScalarKind::sint32};
    case MessageKind::map_sint64_sint64:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::sint64}
                               : FieldSpec{2, false, false, ScalarKind::sint64};
    case MessageKind::map_fixed32_fixed32:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::fixed32}
                 : FieldSpec{2, false, false, ScalarKind::fixed32};
    case MessageKind::map_fixed64_fixed64:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::fixed64}
                 : FieldSpec{2, false, false, ScalarKind::fixed64};
    case MessageKind::map_sfixed32_sfixed32:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::sfixed32}
                 : FieldSpec{2, false, false, ScalarKind::sfixed32};
    case MessageKind::map_sfixed64_sfixed64:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::sfixed64}
                 : FieldSpec{2, false, false, ScalarKind::sfixed64};
    case MessageKind::map_int32_float:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::int32}
                 : FieldSpec{2, false, false, ScalarKind::float32};
    case MessageKind::map_int32_double:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::int32}
                 : FieldSpec{2, false, false, ScalarKind::float64};
    case MessageKind::map_bool_bool:
      return field_number == 1
                 ? FieldSpec{1, false, false, ScalarKind::boolean}
                 : FieldSpec{2, false, false, ScalarKind::boolean};
    case MessageKind::map_string_string:
      return field_number == 1 ? FieldSpec{1, true} : FieldSpec{2, true};
    case MessageKind::map_string_bytes:
      return field_number == 1 ? FieldSpec{1, true} : FieldSpec{2, false, true};
    case MessageKind::map_string_nested_message:
      return field_number == 1 ? FieldSpec{1, true}
                               : FieldSpec{2, false, false, std::nullopt,
                                           MessageKind::nested_message};
    case MessageKind::map_string_foreign_message:
      return field_number == 1 ? FieldSpec{1, true}
                               : FieldSpec{2, false, false, std::nullopt,
                                           MessageKind::foreign_message};
    case MessageKind::map_string_nested_enum:
    case MessageKind::map_string_foreign_enum:
      return field_number == 1
                 ? FieldSpec{1, true}
                 : FieldSpec{2, false, false, ScalarKind::enumeration};
    case MessageKind::struct_fields_entry:
      return field_number == 1
                 ? FieldSpec{1, true}
                 : FieldSpec{2, false, false, std::nullopt, MessageKind::value};
    default:
      return FieldSpec{field_number};
  }
}

FieldSpec test_all_types_spec(uint32_t field_number) {
  switch (field_number) {
    case 1:
    case 31:
    case 89:
    case 401:
    case 402:
    case 403:
    case 404:
    case 405:
    case 406:
    case 407:
    case 408:
    case 409:
    case 410:
    case 411:
    case 412:
    case 413:
    case 414:
    case 415:
    case 416:
    case 417:
    case 418:
      return {field_number,      false,        false,
              ScalarKind::int32, std::nullopt, ScalarKind::int32,
              ScalarKind::int32};
    case 2:
    case 32:
    case 90:
      return {field_number,      false,        false,
              ScalarKind::int64, std::nullopt, ScalarKind::int64,
              ScalarKind::int64};
    case 3:
    case 33:
    case 75:
    case 77:
    case 91:
    case 111:
      return {field_number,       false,        false,
              ScalarKind::uint32, std::nullopt, ScalarKind::uint32,
              ScalarKind::uint32};
    case 4:
    case 34:
    case 76:
    case 78:
    case 92:
    case 116:
      return {field_number,       false,        false,
              ScalarKind::uint64, std::nullopt, ScalarKind::uint64,
              ScalarKind::uint64};
    case 5:
    case 35:
    case 79:
    case 93:
      return {field_number,       false,        false,
              ScalarKind::sint32, std::nullopt, ScalarKind::sint32,
              ScalarKind::sint32};
    case 6:
    case 36:
    case 80:
    case 94:
      return {field_number,       false,        false,
              ScalarKind::sint64, std::nullopt, ScalarKind::sint64,
              ScalarKind::sint64};
    case 7:
    case 37:
    case 81:
    case 95:
      return {field_number,        false,        false,
              ScalarKind::fixed32, std::nullopt, ScalarKind::fixed32,
              ScalarKind::fixed32};
    case 8:
    case 38:
    case 82:
    case 96:
      return {field_number,        false,        false,
              ScalarKind::fixed64, std::nullopt, ScalarKind::fixed64,
              ScalarKind::fixed64};
    case 9:
    case 39:
    case 83:
    case 97:
      return {field_number,         false,        false,
              ScalarKind::sfixed32, std::nullopt, ScalarKind::sfixed32,
              ScalarKind::sfixed32};
    case 10:
    case 40:
    case 84:
    case 98:
      return {field_number,         false,        false,
              ScalarKind::sfixed64, std::nullopt, ScalarKind::sfixed64,
              ScalarKind::sfixed64};
    case 11:
    case 41:
    case 85:
    case 99:
    case 117:
      return {field_number,        false,        false,
              ScalarKind::float32, std::nullopt, ScalarKind::float32,
              ScalarKind::float32};
    case 12:
    case 42:
    case 86:
    case 100:
    case 118:
      return {field_number,        false,        false,
              ScalarKind::float64, std::nullopt, ScalarKind::float64,
              ScalarKind::float64};
    case 13:
    case 43:
    case 87:
    case 101:
    case 115:
      return {field_number,        false,        false,
              ScalarKind::boolean, std::nullopt, ScalarKind::boolean,
              ScalarKind::boolean};
    case 14:
    case 24:
    case 25:
    case 44:
    case 54:
    case 55:
    case 113:
      return {field_number, true};
    case 15:
    case 45:
    case 114:
      return {field_number, false, true};
    case 18:
    case 48:
    case 112:
      return {field_number, false, false, std::nullopt,
              MessageKind::nested_message};
    case 19:
    case 49:
      return {field_number, false, false, std::nullopt,
              MessageKind::foreign_message};
    case 21:
    case 22:
    case 23:
    case 51:
    case 52:
    case 88:
    case 102:
    case 119:
      return {field_number,
              false,
              false,
              ScalarKind::enumeration,
              std::nullopt,
              ScalarKind::enumeration,
              ScalarKind::enumeration};
    case 27:
      return {field_number, false, false, std::nullopt,
              MessageKind::test_all_types};
    case 56:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_int32_int32};
    case 57:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_int64_int64};
    case 58:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_uint32_uint32};
    case 59:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_uint64_uint64};
    case 60:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_sint32_sint32};
    case 61:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_sint64_sint64};
    case 62:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_fixed32_fixed32};
    case 63:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_fixed64_fixed64};
    case 64:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_sfixed32_sfixed32};
    case 65:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_sfixed64_sfixed64};
    case 66:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_int32_float};
    case 67:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_int32_double};
    case 68:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_bool_bool};
    case 69:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_string};
    case 70:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_bytes};
    case 71:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_nested_message};
    case 72:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_foreign_message};
    case 73:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_nested_enum};
    case 74:
      return {field_number, false, false, std::nullopt,
              MessageKind::map_string_foreign_enum};
    case 120:
    case 307:
      return {field_number, false, false, ScalarKind::null_value};
    case 201:
    case 211:
      return {field_number, false, false, std::nullopt,
              MessageKind::bool_wrapper};
    case 202:
    case 212:
      return {field_number, false, false, std::nullopt,
              MessageKind::int32_wrapper};
    case 203:
    case 213:
      return {field_number, false, false, std::nullopt,
              MessageKind::int64_wrapper};
    case 204:
    case 214:
      return {field_number, false, false, std::nullopt,
              MessageKind::uint32_wrapper};
    case 205:
    case 215:
      return {field_number, false, false, std::nullopt,
              MessageKind::uint64_wrapper};
    case 206:
    case 216:
      return {field_number, false, false, std::nullopt,
              MessageKind::float_wrapper};
    case 207:
    case 217:
      return {field_number, false, false, std::nullopt,
              MessageKind::double_wrapper};
    case 208:
    case 218:
      return {field_number, false, false, std::nullopt,
              MessageKind::string_wrapper};
    case 209:
    case 219:
      return {field_number, false, false, std::nullopt,
              MessageKind::bytes_wrapper};
    case 301:
    case 311:
      return {field_number, false, false, std::nullopt, MessageKind::duration};
    case 302:
    case 312:
      return {field_number, false, false, std::nullopt, MessageKind::timestamp};
    case 303:
    case 313:
      return {field_number, false, false, std::nullopt,
              MessageKind::field_mask};
    case 304:
    case 324:
      return {field_number, false, false, std::nullopt,
              MessageKind::struct_value};
    case 305:
    case 315:
      return {field_number, false, false, std::nullopt, MessageKind::any};
    case 306:
    case 316:
      return {field_number, false, false, std::nullopt, MessageKind::value};
    case 317:
      return {field_number, false, false, std::nullopt,
              MessageKind::list_value};
    default:
      return {field_number};
  }
}

FieldSpec field_spec(MessageKind kind, uint32_t field_number) {
  switch (kind) {
    case MessageKind::test_all_types:
      return test_all_types_spec(field_number);
    case MessageKind::nested_message:
      if (field_number == 1) {
        return {1, false, false, ScalarKind::int32};
      }
      if (field_number == 2) {
        return {2, false, false, std::nullopt, MessageKind::test_all_types};
      }
      return {field_number};
    case MessageKind::foreign_message:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::int32}
                               : FieldSpec{field_number};
    case MessageKind::bool_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::boolean}
                               : FieldSpec{field_number};
    case MessageKind::int32_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::int32}
                               : FieldSpec{field_number};
    case MessageKind::int64_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::int64}
                               : FieldSpec{field_number};
    case MessageKind::uint32_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::uint32}
                               : FieldSpec{field_number};
    case MessageKind::uint64_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::uint64}
                               : FieldSpec{field_number};
    case MessageKind::float_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::float32}
                               : FieldSpec{field_number};
    case MessageKind::double_wrapper:
      return field_number == 1 ? FieldSpec{1, false, false, ScalarKind::float64}
                               : FieldSpec{field_number};
    case MessageKind::string_wrapper:
      return field_number == 1 ? FieldSpec{1, true} : FieldSpec{field_number};
    case MessageKind::bytes_wrapper:
      return field_number == 1 ? FieldSpec{1, false, true}
                               : FieldSpec{field_number};
    case MessageKind::duration:
    case MessageKind::timestamp:
      if (field_number == 1) {
        return {1, false, false, ScalarKind::int64};
      }
      if (field_number == 2) {
        return {2, false, false, ScalarKind::int32};
      }
      return {field_number};
    case MessageKind::field_mask:
      return field_number == 1 ? FieldSpec{1, true} : FieldSpec{field_number};
    case MessageKind::any:
      if (field_number == 1) {
        return {1, true};
      }
      if (field_number == 2) {
        return {2, false, true};
      }
      return {field_number};
    case MessageKind::struct_value:
      return field_number == 1 ? FieldSpec{1, false, false, std::nullopt,
                                           MessageKind::struct_fields_entry}
                               : FieldSpec{field_number};
    case MessageKind::value:
      if (field_number == 1) {
        return {1, false, false, ScalarKind::null_value};
      }
      if (field_number == 2) {
        return {2, false, false, ScalarKind::float64};
      }
      if (field_number == 3) {
        return {3, true};
      }
      if (field_number == 4) {
        return {4, false, false, ScalarKind::boolean};
      }
      if (field_number == 5) {
        return {5, false, false, std::nullopt, MessageKind::struct_value};
      }
      if (field_number == 6) {
        return {6, false, false, std::nullopt, MessageKind::list_value};
      }
      return {field_number};
    case MessageKind::list_value:
      return field_number == 1
                 ? FieldSpec{1, false, false, std::nullopt, MessageKind::value}
                 : FieldSpec{field_number};
    default:
      if (field_number == 1 || field_number == 2) {
        return map_entry_spec(kind, field_number);
      }
      return {field_number};
  }
}

bool field_is_known(const FieldSpec& spec) {
  return spec.string_value || spec.bytes_value || spec.scalar.has_value() ||
         spec.message.has_value() || spec.packed_scalar.has_value() ||
         spec.unpacked_scalar.has_value();
}

bool is_test_all_types_repeated(uint32_t field_number) {
  return (field_number >= 31 && field_number <= 55) ||
         (field_number >= 56 && field_number <= 74) ||
         (field_number >= 75 && field_number <= 102) ||
         (field_number >= 211 && field_number <= 219) ||
         (field_number >= 311 && field_number <= 317) || field_number == 324;
}

bool is_repeated_field(MessageKind kind, uint32_t field_number) {
  switch (kind) {
    case MessageKind::test_all_types:
      return is_test_all_types_repeated(field_number);
    case MessageKind::struct_value:
    case MessageKind::list_value:
      return field_number == 1;
    default:
      return false;
  }
}

bool is_oneof_field(MessageKind kind, uint32_t field_number) {
  return kind == MessageKind::test_all_types && field_number >= 111 &&
         field_number <= 120;
}

bool is_unpacked_output_field(MessageKind kind, uint32_t field_number) {
  return kind == MessageKind::test_all_types && field_number >= 89 &&
         field_number <= 102;
}

bool is_packed_output_field(MessageKind kind, uint32_t field_number,
                            const FieldSpec& spec) {
  return is_repeated_field(kind, field_number) &&
         !is_unpacked_output_field(kind, field_number) &&
         (spec.scalar.has_value() || spec.packed_scalar.has_value() ||
          spec.unpacked_scalar.has_value()) &&
         !spec.string_value && !spec.bytes_value && !spec.message.has_value();
}

ScalarKind repeated_scalar_kind(const FieldSpec& spec) {
  if (spec.packed_scalar.has_value()) {
    return *spec.packed_scalar;
  }
  if (spec.unpacked_scalar.has_value()) {
    return *spec.unpacked_scalar;
  }
  if (spec.scalar.has_value()) {
    return *spec.scalar;
  }
  throw std::invalid_argument("field is not repeated scalar");
}

std::string serialize_canonical(const CanonicalMessage& msg) {
  std::string out;
  for (const auto& [field_number, field] : msg.fields) {
    if (!field.present) {
      continue;
    }
    if (field.repeated) {
      if (field.values.empty()) {
        continue;
      }
      if (field.packed_output) {
        std::string payload;
        for (const auto& value : field.values) {
          payload.append(value);
        }
        append_length_delimited(out, field_number, payload);
      }
      else {
        iguana::WireType wire_type =
            field.spec.message.has_value() || field.spec.string_value ||
                    field.spec.bytes_value
                ? iguana::WireType::LengthDelimeted
                : scalar_wire_type(repeated_scalar_kind(field.spec));
        for (const auto& value : field.values) {
          if (wire_type == iguana::WireType::LengthDelimeted) {
            append_length_delimited(out, field_number, value);
          }
          else {
            append_value(out, field_number, wire_type, value);
          }
        }
      }
      continue;
    }

    iguana::WireType wire_type = field.spec.message.has_value() ||
                                         field.spec.string_value ||
                                         field.spec.bytes_value
                                     ? iguana::WireType::LengthDelimeted
                                     : scalar_wire_type(*field.spec.scalar);
    if (wire_type == iguana::WireType::LengthDelimeted) {
      append_length_delimited(out, field_number, field.singular);
    }
    else {
      append_value(out, field_number, wire_type, field.singular);
    }
  }
  for (const auto& unknown : msg.unknown_fields) {
    out.append(unknown);
  }
  return out;
}

std::string canonicalize_message(std::string_view data, MessageKind kind,
                                 uint32_t depth = 0);

void store_oneof(CanonicalMessage& msg, uint32_t field_number,
                 const FieldSpec& spec, std::string payload) {
  for (uint32_t current = 111; current <= 120; ++current) {
    if (current != field_number) {
      msg.fields.erase(current);
    }
  }

  auto& field = msg.fields[field_number];
  if (field.present && field.oneof && spec.message.has_value()) {
    std::string merged = field.singular;
    merged.append(payload);
    payload = canonicalize_message(merged, *spec.message);
  }
  field.spec = spec;
  field.present = true;
  field.repeated = false;
  field.packed_output = false;
  field.oneof = true;
  field.singular = std::move(payload);
}

void store_singular_message(CanonicalMessage& msg, uint32_t field_number,
                            const FieldSpec& spec, std::string payload) {
  auto& field = msg.fields[field_number];
  if (field.present) {
    std::string merged = field.singular;
    merged.append(payload);
    payload = canonicalize_message(merged, *spec.message);
  }
  field.spec = spec;
  field.present = true;
  field.repeated = false;
  field.packed_output = false;
  field.oneof = false;
  field.singular = std::move(payload);
}

void parse_known_for_canonical(CanonicalMessage& msg, std::string_view& data,
                               MessageKind kind, uint32_t field_number,
                               iguana::WireType wire_type,
                               const FieldSpec& spec, uint32_t depth) {
  bool repeated = is_repeated_field(kind, field_number);
  bool oneof = is_oneof_field(kind, field_number);
  bool packed_output = is_packed_output_field(kind, field_number, spec);

  if (spec.string_value || spec.bytes_value) {
    if (wire_type != iguana::WireType::LengthDelimeted) {
      throw std::invalid_argument(spec.string_value
                                      ? "wrong wire type for string field"
                                      : "wrong wire type for bytes field");
    }
    std::string_view value = read_length_delimited(
        data, spec.string_value ? "truncated string" : "truncated bytes");
    if (spec.string_value && !valid_utf8(value)) {
      throw std::invalid_argument("invalid UTF-8 string");
    }
    std::string payload(value);
    if (oneof) {
      store_oneof(msg, field_number, spec, std::move(payload));
    }
    else {
      append_stored_field(msg, field_number, spec, std::move(payload), repeated,
                          false, false, value.empty());
    }
    return;
  }

  if (spec.message.has_value()) {
    if (wire_type != iguana::WireType::LengthDelimeted) {
      throw std::invalid_argument("wrong wire type for message field");
    }
    std::string_view payload = read_length_delimited(data, "truncated message");
    std::string canonical =
        canonicalize_message(payload, *spec.message, depth + 1);
    if (oneof) {
      store_oneof(msg, field_number, spec, std::move(canonical));
    }
    else if (repeated) {
      append_stored_field(msg, field_number, spec, std::move(canonical), true,
                          false, false, false);
    }
    else {
      store_singular_message(msg, field_number, spec, std::move(canonical));
    }
    return;
  }

  if (repeated &&
      (spec.packed_scalar.has_value() || spec.unpacked_scalar.has_value() ||
       spec.scalar.has_value())) {
    ScalarKind scalar = repeated_scalar_kind(spec);
    if (wire_type == iguana::WireType::LengthDelimeted) {
      std::string_view packed =
          read_length_delimited(data, "truncated packed repeated field");
      while (!packed.empty()) {
        ScalarPayload value = read_scalar_payload(packed, scalar);
        append_stored_field(msg, field_number, spec, std::move(value.bytes),
                            true, packed_output, false, value.is_default);
      }
      return;
    }
    if (wire_type == scalar_wire_type(scalar)) {
      ScalarPayload value = read_scalar_payload(data, scalar);
      append_stored_field(msg, field_number, spec, std::move(value.bytes), true,
                          packed_output, false, value.is_default);
      return;
    }
    throw std::invalid_argument("wrong wire type for repeated scalar field");
  }

  if (spec.scalar.has_value()) {
    if (wire_type != scalar_wire_type(*spec.scalar)) {
      throw std::invalid_argument("wrong wire type for scalar field");
    }
    ScalarPayload value = read_scalar_payload(data, *spec.scalar);
    if (oneof) {
      store_oneof(msg, field_number, spec, std::move(value.bytes));
    }
    else {
      append_stored_field(msg, field_number, spec, std::move(value.bytes),
                          false, false, false, value.is_default);
    }
    return;
  }

  throw std::invalid_argument("unknown field spec");
}

std::string canonicalize_message(std::string_view data, MessageKind kind,
                                 uint32_t depth) {
  if (depth > 100) {
    throw std::invalid_argument("protobuf recursion limit exceeded");
  }

  CanonicalMessage msg;
  while (!data.empty()) {
    const char* field_start = data.data();
    auto key = iguana::detail::decode_key(data);
    data.remove_prefix(key.tag_size);
    FieldSpec spec = field_spec(kind, key.field_number);
    try {
      if (field_is_known(spec)) {
        parse_known_for_canonical(msg, data, kind, key.field_number,
                                  key.wire_type, spec, depth);
      }
      else {
        skip_unknown(data, key.wire_type, key.field_number, depth);
        msg.unknown_fields.emplace_back(
            field_start, static_cast<size_t>(data.data() - field_start));
      }
    } catch (const std::invalid_argument&) {
      throw;
    } catch (const std::exception& e) {
      throw std::invalid_argument(e.what());
    }
  }
  return serialize_canonical(msg);
}

void consume_known_field(std::string_view& in, const FieldSpec& spec,
                         iguana::WireType wire_type, uint32_t depth) {
  if (spec.string_value) {
    if (wire_type != iguana::WireType::LengthDelimeted) {
      throw std::invalid_argument("wrong wire type for string field");
    }
    std::string_view value = read_length_delimited(in, "truncated string");
    if (!valid_utf8(value)) {
      throw std::invalid_argument("invalid UTF-8 string");
    }
    return;
  }
  if (spec.bytes_value) {
    if (wire_type != iguana::WireType::LengthDelimeted) {
      throw std::invalid_argument("wrong wire type for bytes field");
    }
    (void)read_length_delimited(in, "truncated bytes");
    return;
  }
  if (spec.message.has_value()) {
    if (wire_type != iguana::WireType::LengthDelimeted) {
      throw std::invalid_argument("wrong wire type for message field");
    }
    std::string_view payload = read_length_delimited(in, "truncated message");
    validate_message(payload, *spec.message, depth + 1);
    return;
  }
  if (spec.packed_scalar.has_value() &&
      wire_type == iguana::WireType::LengthDelimeted) {
    std::string_view payload =
        read_length_delimited(in, "truncated packed repeated field");
    consume_packed(payload, *spec.packed_scalar);
    return;
  }
  if (spec.unpacked_scalar.has_value() &&
      wire_type == scalar_wire_type(*spec.unpacked_scalar)) {
    consume_scalar(in, wire_type, *spec.unpacked_scalar);
    return;
  }
  if (spec.scalar.has_value()) {
    consume_scalar(in, wire_type, *spec.scalar);
    return;
  }
  throw std::invalid_argument("unknown field spec");
}

void validate_message(std::string_view data, MessageKind kind, uint32_t depth) {
  if (depth > 100) {
    throw std::invalid_argument("protobuf recursion limit exceeded");
  }
  while (!data.empty()) {
    auto key = iguana::detail::decode_key(data);
    data.remove_prefix(key.tag_size);
    FieldSpec spec = field_spec(kind, key.field_number);
    try {
      if (spec.string_value || spec.bytes_value || spec.scalar.has_value() ||
          spec.message.has_value() || spec.packed_scalar.has_value() ||
          spec.unpacked_scalar.has_value()) {
        consume_known_field(data, spec, key.wire_type, depth);
      }
      else {
        skip_unknown(data, key.wire_type, key.field_number, depth);
      }
    } catch (const std::invalid_argument&) {
      throw;
    } catch (const std::exception& e) {
      throw std::invalid_argument(e.what());
    }
  }
}

ConformanceRequest parse_request(std::string_view bytes) {
  ConformanceRequest req;
  while (!bytes.empty()) {
    auto key = iguana::detail::decode_key(bytes);
    bytes.remove_prefix(key.tag_size);
    switch (key.field_number) {
      case 1:
        if (key.wire_type != iguana::WireType::LengthDelimeted) {
          throw std::invalid_argument("bad protobuf_payload wire type");
        }
        req.payload = std::string(
            read_length_delimited(bytes, "truncated protobuf_payload"));
        req.payload_case = PayloadCase::protobuf;
        break;
      case 2:
        if (key.wire_type != iguana::WireType::LengthDelimeted) {
          throw std::invalid_argument("bad json_payload wire type");
        }
        req.payload =
            std::string(read_length_delimited(bytes, "truncated json_payload"));
        req.payload_case = PayloadCase::json;
        break;
      case 3:
        if (key.wire_type != iguana::WireType::Varint) {
          throw std::invalid_argument("bad requested_output_format wire type");
        }
        req.requested_output_format =
            static_cast<WireFormat>(read_varint(bytes));
        break;
      case 4:
        if (key.wire_type != iguana::WireType::LengthDelimeted) {
          throw std::invalid_argument("bad message_type wire type");
        }
        req.message_type =
            std::string(read_length_delimited(bytes, "truncated message_type"));
        break;
      case 5:
        if (key.wire_type != iguana::WireType::Varint) {
          throw std::invalid_argument("bad test_category wire type");
        }
        req.test_category = static_cast<TestCategory>(read_varint(bytes));
        break;
      case 7:
        if (key.wire_type != iguana::WireType::LengthDelimeted) {
          throw std::invalid_argument("bad jspb_payload wire type");
        }
        req.payload =
            std::string(read_length_delimited(bytes, "truncated jspb_payload"));
        req.payload_case = PayloadCase::jspb;
        break;
      case 8:
        if (key.wire_type != iguana::WireType::LengthDelimeted) {
          throw std::invalid_argument("bad text_payload wire type");
        }
        req.payload =
            std::string(read_length_delimited(bytes, "truncated text_payload"));
        req.payload_case = PayloadCase::text;
        break;
      case 9:
        if (key.wire_type != iguana::WireType::Varint) {
          throw std::invalid_argument("bad print_unknown_fields wire type");
        }
        req.print_unknown_fields = read_varint(bytes) != 0;
        break;
      default:
        skip_unknown(bytes, key.wire_type, key.field_number, 0);
        break;
    }
  }
  return req;
}

std::string response_parse_error(std::string_view error) {
  std::string out;
  append_string_field(out, 1, error);
  return out;
}

std::string response_runtime_error(std::string_view error) {
  std::string out;
  append_string_field(out, 2, error);
  return out;
}

std::string response_protobuf(std::string_view payload) {
  std::string out;
  append_length_delimited(out, 3, payload);
  return out;
}

std::string response_skipped(std::string_view reason) {
  std::string out;
  append_string_field(out, 5, reason);
  return out;
}

std::string handle_request(const ConformanceRequest& req) {
  if (req.message_type == "conformance.FailureSet") {
    return response_protobuf("");
  }
  if (req.message_type != "protobuf_test_messages.proto3.TestAllTypesProto3") {
    return response_skipped("iguana conformance currently targets proto3");
  }
  if (req.test_category != TestCategory::binary) {
    return response_skipped("wire-only binary conformance only");
  }
  if (req.payload_case != PayloadCase::protobuf) {
    return response_skipped("protobuf payload only");
  }
  if (req.requested_output_format != WireFormat::protobuf) {
    return response_skipped("protobuf output only");
  }
  if (req.print_unknown_fields) {
    return response_skipped("unknown field text output is not supported");
  }

  try {
    std::string canonical =
        canonicalize_message(req.payload, MessageKind::test_all_types);
    return response_protobuf(canonical);
  } catch (const std::exception& e) {
    return response_parse_error(e.what());
  }
}

bool read_exact(std::istream& in, char* data, size_t size) {
  in.read(data, static_cast<std::streamsize>(size));
  return static_cast<size_t>(in.gcount()) == size;
}

bool read_frame(std::string& frame) {
  uint32_t size = 0;
  if (!read_exact(std::cin, reinterpret_cast<char*>(&size), sizeof(size))) {
    return false;
  }
  frame.assign(size, '\0');
  return read_exact(std::cin, frame.data(), frame.size());
}

void write_frame(std::string_view frame) {
  uint32_t size = static_cast<uint32_t>(frame.size());
  std::cout.write(reinterpret_cast<const char*>(&size), sizeof(size));
  std::cout.write(frame.data(), static_cast<std::streamsize>(frame.size()));
  std::cout.flush();
}

}  // namespace

int main() {
  std::ios::sync_with_stdio(false);

  std::string frame;
  while (read_frame(frame)) {
    try {
      ConformanceRequest req = parse_request(frame);
      write_frame(handle_request(req));
    } catch (const std::exception& e) {
      write_frame(response_runtime_error(e.what()));
    }
  }
  return 0;
}
