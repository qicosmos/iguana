# struct_pb 简介

struct_pb 是基于C++17 开发的高性能、易用、header only的protobuf格式序列化/反序列化库。

## 动机
不再依赖proto文件去定义dsl message，而是通过C++ 结构体去定义需要序列化/反序列化的对象；因为没有protoc文件所以也不再依赖protoc去生成代码。通过C++17去实现可以做很多性能优化，从而获得更好的性能，比如可以支持反序列化时对字符串的零拷贝、尽可能内联和编译期计算以及字符串非memset的resize等。

## 例子

### 定义结构体
```cpp
#include <ylt/struct_pb.hpp>

struct my_struct {
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
YLT_REFL(my_struct, x, y, z);

struct nest {
  std::string name;
  my_struct value;
  int var;
};
YLT_REFL(nest, name, value, var);
```

### 序列化
```cpp
int main() {
  nest v{"Hi", {1, false, {3}}, 5}, v2{};
  std::string s;
  iguana::to_pb(v, s);
  iguana::from_pb(v2, s);
  assert(v.var == v2.var);
  assert(v.value.y == v2.value.y);
  assert(v.value.z == v2.value.z);
}
```
上面的这个结构体如果对应到protobuf的proto文件则是:
```
message my_struct {
  int32 optional_int32 = 1;
  bool optional_int64 = 2;
  sfixed64 z = 3;
}

message nest {
  string name = 1;
  my_struct value = 2;
  int32 var = 3;
}
```

### 自定义字段号

默认情况下，字段号按 C++ 成员声明顺序从 1 开始分配。需要和已有 `.proto` 互通时，应显式指定字段号：

```cpp
struct account {
  std::string name;
  int32_t age;
  std::vector<std::string> emails;
};

YLT_REFL_PB(account, (name, 10), (age, 20), (emails, 9));
```

也可以使用 helper 形式描述 proto3 wire schema。helper 形式适合需要 `bytes`、zigzag、fixed、optional、oneof、well-known type 或 unknown fields 的场景：

```cpp
struct event_msg {
  int32_t id{};
  std::string payload;
  int32_t delta{};
  uint32_t checksum{};
  std::chrono::system_clock::time_point created_at{};
  std::chrono::nanoseconds timeout{};
  std::optional<int32_t> retry_count;
  std::variant<std::monostate, int32_t, std::string> result;
  std::string unknown;
};

inline auto get_members_impl(event_msg*) {
  return iguana::pb_members(
      iguana::pb_field<&event_msg::id, 1>("id"),
      iguana::pb_bytes_field<&event_msg::payload, 3>("payload"),
      iguana::pb_zigzag_field<&event_msg::delta, 5>("delta"),
      iguana::pb_optional_field<&event_msg::retry_count, 6>("retry_count"),
      iguana::pb_fixed_field<&event_msg::checksum, 7>("checksum"),
      iguana::as_timestamp_field<&event_msg::created_at, 8>("created_at"),
      iguana::as_duration_field<&event_msg::timeout, 9>("timeout"),
      iguana::pb_oneof_field<&event_msg::result, 10, 12>("result"),
      iguana::pb_unknown_fields_field<&event_msg::unknown>("unknown"));
}
```

helper 接口说明：

| helper | 用途 |
| --- | --- |
| `pb_members(...)` | 在 `get_members_impl(T*)` 中返回 protobuf descriptor tuple。 |
| `pb_field<&T::field, N>("name")` | 指定 protobuf 字段号和 schema 名称。 |
| `pb_bytes_field` | 生成 `bytes`；C++ 字段是 `std::string` 或 `std::string_view`，也支持 optional/vector 外层。 |
| `pb_zigzag_field` | 生成 `sint32` 或 `sint64`；C++ 字段仍是 `int32_t` 或 `int64_t`，也支持 optional/vector 外层。 |
| `pb_fixed_field` | 为 32/64-bit 整数字段生成 `fixed32`、`fixed64`、`sfixed32` 或 `sfixed64`，也支持 optional/vector 外层。 |
| `pb_optional_field` | 生成 proto3 `optional`；C++ 字段必须是 `std::optional<T>`。 |
| `pb_timestamp_field` / `as_timestamp_field` | 把 `std::chrono::system_clock::time_point` 按 `google.protobuf.Timestamp` 编码，也支持 optional/vector 外层。 |
| `pb_duration_field` / `as_duration_field` | 把 `std::chrono::nanoseconds` 按 `google.protobuf.Duration` 编码，也支持 optional/vector 外层。 |
| `pb_oneof_field<&T::field, Ns...>("name")` | 把 `std::variant<std::monostate, ...>` 的非 `monostate` 备选项映射到 oneof 字段号。 |
| `pb_unknown_fields_field<&T::field>()` | 用单个 `std::string` 字段保留未知 protobuf wire bytes，并在再次序列化时原样写回。 |

显式 wrapper 类型 `iguana::pb_timestamp` 和 `iguana::pb_duration` 仍可用于直接保存 wire-shaped well-known type；业务代码通常优先使用 chrono 字段加 `pb_timestamp_field/as_timestamp_field` 或 `pb_duration_field/as_duration_field`。

需要组合多个 protobuf option 时使用 `pb_field_ex`：

```cpp
iguana::pb_field_ex<&event_msg::retry_count, 6>(
    "retry_count", iguana::pb_optional, iguana::pb_zigzag);
```

`pb_field_ex` 支持组合的 option 包括 `pb_bytes`、`pb_zigzag`、`pb_fixed`、`pb_optional`、`pb_as_timestamp/as_timestamp` 和 `pb_as_duration/as_duration`。`pb_zigzag` 不能和 `pb_fixed` 同时使用，`pb_as_timestamp` 不能和 `pb_as_duration` 同时使用。

### C++26 注解写法

启用 C++26 静态反射后，可以直接把 protobuf metadata 写成字段注解。本轮 C++26 测试构建使用 GCC 16.1 和 `-std=gnu++26 -freflection`。

```cpp
struct event_msg26 {
  [[= iguana::pb_field(1)]] int32_t id{};

  [[= iguana::pb_field(3)]]
  [[= iguana::pb_bytes]]
  std::string payload;

  [[= iguana::pb_field(5)]]
  [[= iguana::pb_zigzag]]
  int32_t delta{};

  [[= iguana::pb_field(6)]]
  [[= iguana::pb_optional]]
  std::optional<int32_t> retry_count;

  [[= iguana::pb_field(7)]]
  [[= iguana::pb_fixed]]
  uint32_t checksum{};

  [[= iguana::pb_field(8)]]
  [[= iguana::as_timestamp]]
  std::chrono::system_clock::time_point created_at{};

  [[= iguana::pb_field(9)]]
  [[= iguana::as_duration]]
  std::chrono::nanoseconds timeout{};

  [[= iguana::pb_oneof<10, 12>]]
  std::variant<std::monostate, int32_t, std::string> result;

  [[= iguana::pb_unknown_fields]]
  std::string unknown;
};
```

当前支持的 proto3 wire metadata 包括：自定义字段号、`bytes`、`sint32/sint64` zigzag、fixed/sfixed、explicit optional presence、oneof、`google.protobuf.Timestamp`、`google.protobuf.Duration`、unknown fields 保留。repeated primitive 读取端同时接受 packed、unpacked、多 packed chunk 和混合编码；写入端按 proto3 默认规则输出 packed。

C++26 注解和 helper 的含义一一对应：

| C++26 注解 | 对应 helper / 说明 |
| --- | --- |
| `[[= iguana::pb_field(N)]]` | 对应 `pb_field<&T::field, N>`，指定字段号；字段号必须在 `[1, 2^29 - 1]`，且不能落入 `[19000, 19999]`。 |
| `[[= iguana::pb_bytes]]` | 对应 `pb_bytes_field`，字段类型限制同 helper。 |
| `[[= iguana::pb_zigzag]]` | 对应 `pb_zigzag_field`，支持 `int32_t/int64_t` 及 optional/vector 外层。 |
| `[[= iguana::pb_fixed]]` | 对应 `pb_fixed_field`，支持 32/64-bit 整数及 optional/vector 外层。 |
| `[[= iguana::pb_optional]]` | 对应 `pb_optional_field`，字段必须是 `std::optional<T>`。 |
| `[[= iguana::pb_oneof<N...>]]` / `[[= iguana::oneof<N...>]]` | 对应 `pb_oneof_field`，字段必须是 `std::variant<std::monostate, ...>`。 |
| `[[= iguana::as_timestamp]]` / `[[= iguana::pb_as_timestamp]]` | 对应 `as_timestamp_field/pb_timestamp_field`。 |
| `[[= iguana::as_duration]]` / `[[= iguana::pb_as_duration]]` | 对应 `as_duration_field/pb_duration_field`。 |
| `[[= iguana::pb_unknown_fields]]` | 对应 `pb_unknown_fields_field`；每个 message 最多一个，字段必须是 `std::string`。 |

### 生成 proto schema

```cpp
std::string schema;
iguana::to_proto<event_msg>(schema, "demo");
```

`to_proto` 用于生成结构体对应的 proto3 schema 视图，便于 review 字段号和 wire 类型；它不是完整的 `.proto` 编译器。

## 动态反射
特性：
- 根据对象名称创建实例；
- 获取对象的所有字段名；
- 根据对象实例和字段名获取或设置字段的值

### 根据名称创建对象
```cpp
struct my_struct {
  int x;
  bool y;
  iguana::fixed64_t z;
};
YLT_REFL(my_struct, x, y, z);

struct nest1 : public iguana::base_impl<nest1> {
  nest1() = default;
  nest1(std::string s, my_struct t, int d)
      : name(std::move(s)), value(t), var(d) {}
  std::string name;
  my_struct value;
  int var;
};
YLT_REFL(nest1, name, value, var);
```

```cpp
std::shared_ptr<base> t = iguana::create_instance("nest1");
```
根据对象nest1创建了实例，返回的是基类指针。

“根据对象名称创建实例” 要求对象必须从iguana::base_impl 派生，如果没有派生则创建实例会抛异常。

### 根据名称设置字段的值
```cpp
  auto t = iguana::create_instance("nest1");

  std::vector<std::string_view> fields_name = t->get_fields_name();
  CHECK(fields_name == std::vector<std::string_view>{"name", "value", "var"});

  my_struct mt{2, true, {42}};
  t->set_field_value("value", mt);
  t->set_field_value("name", std::string("test"));
  t->set_field_value("var", 41);
  nest1 *st = dynamic_cast<nest1 *>(t.get());
  auto p = *st;
  std::cout << p.name << "\n";
  auto &r0 = t->get_field_value<std::string>("name");
  CHECK(r0 == "test");
  auto &r = t->get_field_value<int>("var");
  CHECK(r == 41);
  auto &r1 = t->get_field_value<my_struct>("value");
  CHECK(r1.x == 2);
```
“根据对象实例和字段名获取或设置字段的值” 如果字段名不存在则会抛异常；如果设置的值类型和结构体字段类型不相同则会抛异常；需要类型完全一样，不允许隐式转换。比如字段类型是double，但是设置字段的值类型是int也会抛异常，必须显式传double；如果字段类型是std::string, 设置值类型是const char * 同样会报错；如果字段类型是int32_t, 设置值类型是uint_8也会抛异常，因为类型不相同。

设置字段值时也可以显式指定字段类型：
```cpp
t->set_field_value<std::string>("name", "test");
```
这种方式则不要求设置值的类型和字段类型完全一样，只要能赋值成功即可。如果显式指定的字段类型不是实际的字段类型时也会抛异常。

## benchmark 
在benchmark monster场景下，struct_pb 性能比protobuf 更好，序列化速度是protobuf的2.4倍，反序列化是protobuf的3.4倍。详情可以自行运行struct_pack 中的benchmark复现结果。

## struct_pb 和 protobuf 类型映射
Scalar Value Types with no modifier (a.k.a **singular**) -> T

Scalar Value Types with **optional** -> `std::optional <T>`

any type with **repeat** -> `std::vector<T>`

types with **map** -> `std::map<K, V>`

any message type -> `std::optional <T>`

enum -> enum class

oneof -> `std::variant <...>`

### 映射表
| .proto Type | struct_pb Type                    | pb native C++ type | Notes                              |
|-------------|-----------------------------------|--------------------|------------------------------------|
| double      | double                            | double             | 8 bytes                            |
| float       | float                             | float              | 4 bytes                            |
| int32       | int32                             | int32              | Uses variable-length encoding.     |
| int64       | int64                             | int64              |                                    |
| uint32      | uint32                            | uint32             |                                    |
| uint64      | uint64                            | uint64             |                                    |
| sint32      | sint32_t                             | int32              | ZigZag + variable-length encoding. |
| sint64      | sint6_t                             | int64              |                                    |
| fixed32     | fixed32_t                            | uint32             | 4 bytes                            |
| fixed64     | fixed64_t                            | uint64             | 8 bytes                            |
| sfixed32    | sfixed32_t                             | int32              | 4 bytes,$2^{28}$                   |
| sfixed64    | sfixed64_t                             | int64              | 8 bytes,$2^{56}$                   |
| bool        | bool                              | bool               |                                    |
| string      | std::string                       | string             | $len < 2^{32}$                     |
| bytes       | std::string                       | string             |                                    |
| enum        | enum class: int {}/enum                | enum: int {}       |                                    |
| oneof       | std::variant<...> |                    |                                    |

## 约束
- 当前 protobuf 能力定位为 proto3 binary wire-only，不支持 proto2 required、extension、自定义 option、service，也不覆盖 proto3 JSON mapping 或 text format；
- 普通 `to_pb` / `from_pb` 不要求结构体派生 `base_impl`；只有“根据对象名称创建实例”的动态反射功能需要派生 `iguana::base_impl`；
- unknown fields 需要显式声明 `pb_unknown_fields_field` 或 C++26 `[[= iguana::pb_unknown_fields]]` 字段后才会保留；
- official conformance 当前接入的是 proto3 binary/protobuf-output 子集，JSON/text/proto2 和非 protobuf output 会跳过。

## roadmap
- 支持proto2；
- 扩展 proto3 JSON mapping 和 text format；
- 扩展 descriptor/custom option/service 等更完整 `.proto` 语义；
