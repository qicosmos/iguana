# iguana C++26 反射重构

对 iguana 使用 C++26 静态反射做了重构，简化了实现：

- 采用的核心思路是：结构体本身作为 schema，字段遍历使用 `std::meta::nonstatic_data_members_of` 和 `template for`，额外语义通过 `[[= ...]]` 注解挂到字段或类型上。

## 总体目标

这次改造把 C++26 分支从“继续复用 C++17/C++20 的宏注册、结构化绑定、偏移表分发”改成真正使用静态反射：

- 字段数量、字段名、字段访问都来自 `std::meta`。
- JSON/XML/YAML 反序列化不再通过 offset 转回字段指针，而是 splice 到真实成员。
- protobuf 自定义字段号支持 C++26 值注解；C++26 注解路径不再依赖 `YLT_REFL_PB`，但保留该宏作为旧代码和 custom reflection 入口。
- 非 C++26 路径补齐 `pb_members` helper，能表达 C++26 注解路径已经具备的 proto3 wire-only schema metadata。
- 公开入口尽量保留，但 C++26 下 `object_to_tuple`、`struct_to_tuple`、`visit_members` 的行为已经转为反射 metadata 和逐字段访问，不再保证旧的 tuple/参数包调用契约。

## 为什么 C++26 反射更适合这个方向

旧 C++17/C++20 反射路径本质上是在语言没有 class reflection 的前提下做模拟，能工作，但约束和实现复杂度都很高：

- 无宏路径依赖 aggregate。`members_count()` 只有在 `std::is_aggregate_v<T>` 为 true 时才会走结构化绑定字段数探测；非 aggregate 类型需要用户写 `YLT_REFL` 或自定义反射入口。
- 字段展开依赖结构化绑定和生成宏。旧路径通过 `auto& [field0, field1, ...] = obj` 再 `std::tie(...)` 生成字段 tuple，并依赖 `internal/generate/member_macro.hpp` 展开固定数量字段；这带来 aggregate 约束、生成代码复杂度和字段数上限。
- 字段名提取绕得很远。旧路径需要先用假对象和结构化绑定拿字段指针 tuple，再通过 `get_member_name<wrap(ptr)>()` 推导名字；C++17/C++20 下还要分开处理不同 constexpr 能力，代码路径比较复杂。
- 私有字段不能被普通结构化绑定枚举。旧路径需要 `YLT_REFL_PRIVATE` 通过 friend 注入和成员指针列表显式暴露私有字段；没有宏就不能自动反射私有 data member。
- 运行时字段分发依赖 offset 兼容层。旧读取路径会先用 offset/name map 定位字段，再转回对象成员，既复杂也容易受布局和 wrapper 类型影响。

C++26 静态反射把这些问题变成直接的语言能力：

- 字段集合直接来自 `std::meta::nonstatic_data_members_of`，不再要求类型是 aggregate，也不需要用结构化绑定猜字段数。
- 字段数量、字段名和字段类型都从同一组 `std::meta::info` 派生，`members_count()`、`member_names_array()`、`for_each_data_member()` 共享同一套 metadata。
- 字段访问通过 `obj.[:member:]` splice 到真实成员，不需要先构造引用 tuple，也不需要宏生成 `field0...fieldN` 结构化绑定模板。
- 当前实现使用 `std::meta::access_context::unchecked()`，因此可以枚举 private data member；私有字段不再需要 `YLT_REFL_PRIVATE` 这类宏才能进入反射字段列表。
- 额外 schema 语义通过 `[[= ...]]` 注解直接挂到类型、字段或继承边上，例如 `field_name`、`skip_field`、`skip_base`、`xml_required`、`pb_field`，不再把宏注册当成扩展点。

因此这次不是简单把旧 helper 换成 C++26 写法，而是把 iguana 的 C++26 分支从“宏反射/结构化绑定反射的兼容实现”切到真正基于语言静态反射的实现。旧宏仍保留为旧代码兼容入口，但 C++26 主路径已经不靠宏枚举成员。

## 变更追溯总表

下表按改动范围列出落地内容、改动原因和重点阅读区域。

| 范围 | 改了什么 | 为什么改 | 关注点 |
| --- | --- | --- | --- |
| `iguana/ylt/reflection/reflect26_compat.hpp` | 新增 C++26 反射特性开关，兼容 `__cpp_impl_reflection` 和后续 `__cpp_reflection`。 | 让 C++26 分支可以独立接入，同时旧编译器不受影响。 | 宏检测是否只在支持 `std::meta` 的编译器上打开。 |
| `iguana/ylt/reflection/reflect26_core.hpp` | 新增字段收集、字段名、注解扫描、逐字段访问、offset 兼容工具。 | 替代宏注册和结构化绑定字段枚举，统一支撑 JSON/XML/YAML/protobuf。 | 基类递归、`skip_base`/`skip_field`、`access_context::unchecked` 的可见性边界。 |
| `iguana/ylt/reflection/reflect26_dispatch.hpp` | 新增运行时 key 到 C++26 成员访问的分发表。 | JSON/XML/YAML 输入字段名是运行时字符串，不能只靠纯编译期遍历。 | alias 名称、查找失败、性能和旧 offset 分发行为是否一致。 |
| `member_count.hpp`、`member_names.hpp`、`member_ptr.hpp`、`member_value.hpp`、`user_reflect_macro.hpp` | C++26 下接入 `std::meta` 字段来源，`YLT_REFL`/`YLT_REFL_PRIVATE` 保留为兼容入口；tuple/参数包式 helper 在 C++26 下改为 metadata/逐字段访问。 | 让旧宏代码在 C++26 下可编译，同时让 C++26 路径不再依赖宏生成字段和结构化绑定。 | 旧路径是否完全保留；C++26 下 `visit_members`/`object_to_tuple` 行为变化是否可接受；no-op 宏是否改变私有反射预期。 |
| `iguana/util.hpp`、`iguana/dynamic.hpp`、`iguana/json_util.hpp`、`iguana/xml_util.hpp` | 调整 C++26 可反射类型过滤，给动态基类和 XML/JSON wrapper 增加排除标记；新增 `xml_required` 注解。 | 防止把容器、字符串、wrapper、框架基类内部状态当成业务 message 字段递归反射。 | 排除列表是否覆盖所有 wrapper。 |
| `iguana/json_reader.hpp`、`iguana/xml_reader.hpp`、`iguana/yaml_reader.hpp` | C++26 读取分支改用 `dispatch_by_name` 后直接解析真实成员。 | 删除 C++26 路径上的 offset 指针算术，避免假对象地址和私有布局依赖。 | 未知字段处理、alias、required 字段和旧路径语义是否一致。 |
| `iguana/common.hpp` | 新增 protobuf 注解、老路径 helper、字段号校验、oneof 展开、wire type selector、Timestamp/Duration/unknown fields 公共描述。 | C++26 注解路径和非 C++26 helper 路径需要共享同一套 proto3 schema metadata。 | 字段号合法性是否影响旧 `YLT_REFL_PB` 用户；helper 和注解是否语义对齐。 |
| `iguana/pb_util.hpp` | 统一 varint/tag 解码、字段号校验、递归深度 guard、wire schema 辅助。 | proto3 wire-only 需要对非法 field number、非法 wire type、截断 varint 做一致边界检查。 | 递归深度限制是否和 protobuf 兼容。 |
| `iguana/pb_reader.hpp` | 补 packed/unpacked repeated、map entry 任意顺序、重复字段 merge/last-wins、unknown fields 保留、well-known type 转换和非法 wire 检查。 | 使 public protobuf reader 达到 proto3 wire-only 互通语义，而不是只处理 iguana 自己写出的 bytes。 | 截断 payload、map 重复 key、oneof message merge、unknown group 深度。 |
| `iguana/pb_writer.hpp` | 写入端接入 schema-aware wire 类型、proto3 optional presence、oneof monostate 跳过、unknown fields 原样写回。 | 让用户字段类型可以和 protobuf wire 类型分离，并保持未知字段 roundtrip。 | 默认值省略、optional present default、oneof 输出和 unknown bytes 拼接顺序。 |
| `CMakeLists.txt` | 新增 `ENABLE_CXX26_REFLECTION`、`*_cpp26` 测试目标、`iguana_conformance` target 和可选 runner CTest。 | 同一构建目录内同时验证旧路径和 C++26 路径，并能接官方 conformance runner。 | CTest 是否覆盖必要目标；conformance target 的范围是否标注清楚。 |
| `.github/workflows/linux-gcc-cxx26.yml` | 新增 GCC 16.1.0 C++26 反射 CI。 | 提交后在 GitHub Actions 中真实编译 `-std=gnu++26 -freflection` 路径并运行 `_cpp26` 测试。 | 官方 gcc 容器 tag 和 GCC 反射实现仍可能随上游演进，需要定期确认。 |
| `benchmark/pb_benchmark.cpp` | 支持命令行迭代次数，输出 iguana/protobuf 耗时比，修正 map benchmark 自比较断言。 | 便于复现性能结果，并避免 benchmark 断言掩盖 map 场景问题。 | benchmark 结果只作为性能观察，不作为语义正确性依据。 |
| `test/test_pb.cpp`、`test/test_proto3.cpp`、`test/conformance/iguana_conformance.cpp`、`test/proto/*.proto` | 扩展 proto3 wire-only 单测、protoc runtime 对照、官方 conformance testee 和 fixture proto。 | 覆盖新增 schema helper、C++26 注解、旧路径 helper、wire 互操作和官方 proto3 binary conformance。 | `iguana_conformance` 是 binary wire canonicalizer；JSON/text/JSPB 是官方 runner 的非目标表示形式。 |
| `iguana_reflect26_changes.md`、`README.md`、`lang/struct_pb_intro.md` | 记录实际变更、验证命令、能力边界和用户用法。 | 排查时可以从文档追到代码、测试和验收结果，避免多个草稿文档分散事实来源。 | 文档保持已完成能力、pb3 binary wire 支持、官方 runner 非目标格式和待修复问题一致。 |

## 测试情况

这次验证分两条编译路径进行，目标是确认 C++26 静态反射不会破坏旧编译器和旧用户代码：

- 老版本编译器或未开启 C++26 reflection 特性的编译器不会定义 `YLT_USE_CXX26_REFLECTION`，`<meta>`、`reflect26_core.hpp`、`reflect26_dispatch.hpp` 和 C++26 分支代码都被预处理条件隔离，继续走原来的 C++17/C++20 宏反射、结构化绑定和 offset 兼容路径。
- 新版本编译器在支持 `__cpp_impl_reflection >= 202406L` 或 `__cpp_reflection >= 202406L` 时才进入 C++26 分支；CI 中用 GCC 16.1、`-std=gnu++26 -freflection` 和 `-DENABLE_CXX26_REFLECTION=ON` 单独生成 `_cpp26` 测试目标。
- CMake 中普通目标和 `_cpp26` 目标同时存在。普通目标继续验证旧路径，`_cpp26` 目标验证新路径，同一套测试源码分别编译运行，避免新旧代码互相覆盖或互相替代。
- `YLT_REFL`、`YLT_REFL_PRIVATE`、`YLT_REFL_PB` 仍作为旧代码兼容入口保留；C++26 主路径通过 `std::meta` 和注解拿字段信息，不依赖这些宏枚举成员。

验证结果：

- 普通目标和 `_cpp26` 目标均完成构建，`ctest --test-dir build_cpp26 --output-on-failure` 在配置 `ENABLE_CXX26_REFLECTION=ON` 后覆盖 21 个测试并全部通过。
- `test_pb`、`test_pb_cpp26`、`test_proto`、`test_xml`、`test_xml_cpp26` 均有单独执行记录，覆盖旧路径、C++26 注解路径、protobuf helper 路径、protoc runtime 对照和 XML 注解路径。
- `test_proto3.cpp` 使用 `protoc 3.21.12` 生成的 C++ runtime 做互操作对照，覆盖 iguana/protoc bytes 一致性，以及 packed/unpacked repeated、singular message merge、oneof last-wins、map entry key/value 任意顺序、unknown field、重复 map key last-wins 等 proto3 wire 行为。

proto3 binary wire 协议验证：

- public `to_pb` / `from_pb` 路径已经通过项目内单测和 protoc runtime 对照，覆盖本轮实现的 proto3 wire-only 行为，包括字段号合法性、非法 wire type、截断 varint/length/fixed、packed fixed 坏长度、递归深度、unknown fields 保留、proto3 optional presence、oneof、map、well-known Timestamp/Duration 等。
- 官方 `conformance_test_runner --enforce_recommended build_cpp26/iguana_conformance` 已通过，结果为 Binary/JSON suite `651 successes`、`1366 skipped`、`0 unexpected failures`，Text-format suite `0 successes`、`120 skipped`、`0 unexpected failures`。
- `iguana_conformance` 当前是面向官方 runner 的 proto3 binary canonicalizer，验证 `protobuf_test_messages.proto3.TestAllTypesProto3` 的 binary protobuf input 和 protobuf output。对 iguana 的 pb3 binary wire 格式支持来说，这就是目标协议格式；官方 runner 里的 JSON/text/JSPB 是其他表示形式，不属于 struct_pb 的 pb3 binary wire 格式。

格式范围：

- 官方 runner 中 JSON/text/JSPB 和非 protobuf output 会 skipped；这些不是 pb3 binary wire 格式本身，skipped 属于预期结果。
- proto2 required、extension、自定义 option 等属于 proto2 或更大 schema 语义，不属于 proto3 binary wire 支持范围；这些在本文后面仍列为后续独立方向。
- `iguana_conformance` 不直接调用 public `iguana::from_pb` / `iguana::to_pb`，所以它和 public API 单测是互补关系：前者验证官方 runner 的 proto3 binary wire 语义，后者验证实际 iguana API 行为。
- proto3 binary protobuf input/output 没有记录未通过项；skipped 项来自非目标格式或非 proto3 范围。

## 新增反射核心

新增文件：

- `iguana/ylt/reflection/reflect26_compat.hpp`
- `iguana/ylt/reflection/reflect26_core.hpp`
- `iguana/ylt/reflection/reflect26_dispatch.hpp`

`reflect26_compat.hpp` 只做特性开关。它检测 `__cpp_impl_reflection` 或未来的 `__cpp_reflection`，定义 `YLT_USE_CXX26_REFLECTION`。

`reflect26_core.hpp` 是纯 C++26 反射基础层：

- `data_members<T>()` 递归收集基类和本类非静态数据成员，字段来源是 `std::meta::nonstatic_data_members_of`，不是 aggregate 结构化绑定。
- `skip_field` 注解可以把本地状态字段从反射字段列表中排除。
- `has_annotation<Member, Predicate>()` 统一封装 `annotations_of + template for` 注解判定。
- `members_count<T>()` 替代结构化绑定计数字段。
- `member_names_array<T>()` 使用 `std::meta::identifier_of` 或字段注解拿字段名，不再通过假对象、字段指针 tuple 和 `get_member_name` 推导。
- `for_each_data_member(t, visitor)` 使用 `template for` 和 `t.[:member:]` 直接访问字段，并把字段名和 index 一起传给 visitor；在当前 unchecked access context 下 private data member 也能被枚举和访问。
- `member_offsets<T>()` 只保留给仍需要 offset 数组的兼容路径。

`reflect26_dispatch.hpp` 是运行时 key 到静态字段访问的桥：

- 使用 `template for` 线性遍历 C++26 字段列表，并用字段名或规范化字段名匹配运行时 key。
- 命中后直接 splice 到 `obj.[:member:]` 调用用户回调，回调可接收 `(field)`、`(field, name)` 或 `(field, name, index)`。
- 这样避免了旧实现的 offset 指针算术；当前没有使用 `frozen::unordered_map` 做 C++26 路径的运行时 key 查找，性能重点是线性比较的字段数成本。

这个文件仍然需要。C++26 反射解决的是“编译期知道有哪些成员”，但 JSON/XML/YAML 输入 key 是运行时字符串，需要一层运行时查找和类型化调用桥接。

## 注解能力

当前实现了九类注解能力。

字段别名：

```cpp
struct T {
  [[= ylt::reflection::field_name<"id">{}]] int user_id;
};
```

类型名别名：

```cpp
struct [[= ylt::reflection::struct_name<"user">{}]] user_t {
  int id;
};
```

protobuf 字段号：

```cpp
struct Person {
  [[= iguana::pb_field(10)]] std::string name;
  [[= iguana::pb_field(20)]] int32_t age;
};
```

不写 `pb_field` 时仍按字段顺序默认分配 protobuf field number，从 1 开始递增；`pb_field(N)` 只是在需要和既有 `.proto` 互通或需要稳定非连续编号时覆盖默认值。

base 跳过：

```cpp
struct D : [[= ylt::reflection::skip_base{}]] Base {
  int x;
};
```

字段跳过：

```cpp
struct T {
  int id;
  [[= ylt::reflection::skip_field{}]] int local_cache;
};
```

XML required 字段：

```cpp
struct T {
  [[= iguana::xml_required{}]] int id;
};
```

protobuf wire 语义：

```cpp
struct T {
  [[= iguana::pb_zigzag]] int32_t delta;
  [[= iguana::pb_fixed]] uint32_t checksum;
  [[= iguana::pb_bytes]] std::string payload;
  [[= iguana::pb_oneof<5, 8>]]
  std::variant<std::monostate, int32_t, std::string> result;
  [[= iguana::as_timestamp]]
  std::chrono::system_clock::time_point created_at;
  [[= iguana::as_duration]]
  std::chrono::nanoseconds timeout;
};
```

protobuf 未知字段保留：

```cpp
struct OldMessage {
  [[= iguana::pb_field(2)]] int32_t id;
  [[= iguana::pb_unknown_fields]] std::string unknown;
};
```

protobuf 字段号使用值注解 `[[= iguana::pb_field(N)]]`，不是 `pb_field<N>{}`。这是参考文章里 `[[= proto3::field(5)]]` 的写法，字段号作为 constexpr 值提取，API 更短，语义也更接近 protobuf schema。

## 反射层改动

`member_count.hpp`：

- C++26 分支直接调用 `reflect26::members_count<T>()`。
- 不再依赖聚合结构化绑定探测，也没有 256 字段限制。

`member_names.hpp`：

- C++26 分支直接返回 `reflect26::member_names_array<T>()`。
- `get_struct_name<T>()` 优先读取 `struct_name` 注解，再回退到旧 alias API 和 `type_string<T>()`。
- 字段名遍历改为 `template for`。
- C++26 offset 数组改用 `std::meta::offset_of` 生成，避免假对象地址计算。

`member_ptr.hpp`：

- C++26 分支不再包含宏生成文件 `internal/generate/member_macro.hpp`。
- `object_to_tuple(t)` 和 `struct_to_tuple<T>()` 当前返回 `reflect26::data_members<T>()`，即字段 `std::meta::info` 列表，而不是旧路径的引用 tuple 或字段指针 tuple。
- `visit_members(t, visitor)` 在 C++26 分支委托给 `for_each_data_member`，按字段逐次调用 visitor，传入 `(field, name, index)`。
- 因此 C++26 下不再兼容依赖 `visitor(field0, field1, ...)` 一次性参数包的旧用法；需要参数包契约的用户应继续走非 C++26 路径或单独补兼容层。

这里没有强行把所有 `index_sequence` 改成 `template for`。原因是非 C++26 旧路径的 tuple 构造、旧路径 frozen map 初始化，以及 protobuf variant/oneof 字段展开仍需要参数包展开；`template for` 更适合 C++26 的逐字段执行逻辑。

`member_value.hpp`：

- `for_each(t, visitor)` 在 C++26 分支用 `for_each_data_member` 直接访问字段。
- `for_each` 的字段名仍通过 `get_member_names<T>()` 取得，以同时保留旧 `get_alias_field_names` API 和新 `field_name` 注解。
- `index_of(t, field)` 在 C++26 分支用地址比较确定字段 index，不再排序 offset 数组再二分。
- 保留旧分支，非 C++26 编译器行为不变。

`user_reflect_macro.hpp`：

- C++26 分支下 `YLT_REFL`/`YLT_REFL_PRIVATE` 变成 no-op。
- 目的是让旧测试和旧用户代码在 C++26 下可以继续编译，但成员枚举实际来自语言反射。

## JSON/XML/YAML 读取路径

`json_reader.hpp`、`xml_reader.hpp`、`yaml_reader.hpp` 都增加了 C++26 分支：

- 运行时 key 使用 `reflect26::dispatch_by_name` 找到字段。
- 找到字段后直接调用原有的 `from_json_impl`、`xml_parse_item`、`yaml_parse_item`。
- 未知字段处理逻辑保持原样，继续受 `THROW_UNKNOWN_KEY` 控制。

旧路径还保留 `get_variant_map` + offset 的实现，用于 C++17/C++20。

## protobuf 改动

`common.hpp` 新增：

- `iguana::pb_field_annotation`
- `iguana::pb_field(size_t)`
- `iguana::pb_zigzag`
- `iguana::pb_fixed`
- `iguana::pb_bytes`
- `iguana::pb_optional`
- `iguana::pb_unknown_fields`
- C++26 注解提取函数 `pb_annotation_field_no<Member>()`
- 字段号合法性检查和重复字段号检查

字段号规则现在统一检查：

- 没有显式字段号时，普通字段按反射字段顺序从 1 开始递增。
- `pb_unknown_fields` 字段不参与普通字段号分配。
- `oneof/variant` 一个 C++ 字段可能展开为多个 wire field，默认编号会按展开后的 wire field 数量递增。
- 显式 `pb_field(N)` 使用用户指定的字段号，并覆盖默认顺序号。
- 必须大于 0。
- 必须小于等于 `2^29 - 1`。
- 不能落入 protobuf 保留区间 `[19000, 19999]`。
- 同一个 message 内不能重复。

`pb_util.hpp` 删除了重复的 `pb_field_no` 和重复字段号检查实现，改为复用 `common.hpp` 里的统一校验。

这让两条路径都能受益：

- 旧的 `YLT_REFL_PB(Struct, (field, no), ...)`
- 新的 `[[= iguana::pb_field(no)]]`
- 新的 `[[= iguana::pb_zigzag]]` 和 `[[= iguana::pb_fixed]]`
- 新的 `[[= iguana::pb_bytes]]`
- 新的 `[[= iguana::pb_oneof<N...>]]`
- 新的 `[[= iguana::pb_optional]]`
- 新的 `[[= iguana::as_timestamp]]` 和 `[[= iguana::as_duration]]`
- 新的 `[[= iguana::pb_unknown_fields]]`

`pb_zigzag` 把 `int32_t/int64_t` 映射到 `sint32/sint64` wire 语义；`pb_fixed` 把 `uint32_t/uint64_t/int32_t/int64_t` 映射到 `fixed32/fixed64/sfixed32/sfixed64`。二者同时支持标在 `std::optional<T>` 和 `std::vector<T>` 上，用户结构体仍保存原始整数类型，wire schema 单独保存 wrapper 类型。

`pb_bytes` 标记 `std::string` 或 `std::string_view` 字段在 `to_proto` schema 中生成为 `bytes`。它不改变 protobuf 二进制 wire 类型，因为 `string` 和 `bytes` 都是 LengthDelimited。

`pb_oneof<N...>` 标记 `std::variant<std::monostate, ...>` 字段为 protobuf oneof，并用注解里的字段号逐一对应非 `monostate` 备选项。`monostate` 只表示未选择状态，不写入 wire，也不会出现在 schema 里。

`pb_optional` 标记 `std::optional<T>` 字段为 proto3 explicit presence 字段。字段有值时即使是默认值 `0/false/""` 也会写入 wire，`to_proto` 会生成 `optional` label；未设置时不写入。

`as_timestamp` 标记 `std::chrono::system_clock::time_point` 字段按 `google.protobuf.Timestamp` message 编码；`as_duration` 标记 `std::chrono::nanoseconds` 字段按 `google.protobuf.Duration` message 编码。二者也支持对应的 `std::optional<T>` 和 `std::vector<T>`，用户结构体里仍保持 chrono 类型。

`pb_unknown_fields` 标记一个 `std::string` 字段作为未知 protobuf 字段的原始字节缓存。C++26 读取路径遇到未知 field 时会把 tag 和 payload 一起追加进去，写回时再原样输出；该字段不会参与普通 protobuf 字段号分配，也不会出现在 `to_proto` 生成的 schema 里。当前限制是每个 message 最多一个该字段。未知字段跳过覆盖 Varint、Fixed32、Fixed64、LengthDelimited，以及已废弃但仍合法的 group wire type。

protobuf 读取端现在同时接受 packed 和 unpacked 的 repeated primitive 编码。写入端仍使用 proto3 默认 packed 编码，但解析时允许同一 repeated 字段混合出现 packed chunk 和 unpacked value。

protobuf map entry 读取端现在按 entry 内部 field number 解析，不再依赖 key/value 顺序；entry 内 unknown field 会按 wire type 跳过，重复 key 按 protobuf map 语义采用 last-wins。

wire-only proto3 解析继续补齐了重复字段语义和非法输入边界：

- repeated primitive 的多个 packed chunk 会追加合并，不会覆盖前一个 chunk。
- singular message 重复出现时按 protobuf 语义 merge；`std::optional<message>` 也会在已有值上 merge。
- oneof 重复出现时保持 last-wins，同一 message 备选项重复出现也不做跨 oneof case merge。
- tag 解码统一校验 field number 不能为 0、wire type 不能为 6/7、顶层不能出现 EndGroup。
- length-delimited payload 会限制在自身 slice 内解析，截断 string/message/packed payload、过长 varint 和 fixed packed 坏长度都会抛异常。

字段注解的实现方式是把“用户字段类型”和“protobuf wire 类型”分开：

- `pb_field_t` 保留 `value_type` 作为结构体真实字段类型。
- `wire_value_type`/`wire_sub_type` 只用于 size 计算、写入、读取和 `to_proto` schema。
- 写入时通过 `make_pb_wire_scalar` 临时构造 `sint32_t/fixed32_t/...` wrapper，不改用户对象。
- 读取时通过 `assign_pb_wire_value` 转回原始字段；对 packed repeated wrapper 直接按 payload 解每个 item，避免把 `std::vector<fixed32_t>` 之类 wrapper vector 当成普通 `vector<char>` resize。

这样 `[[= iguana::pb_zigzag]] std::vector<int64_t>` 可以生成 `repeated sint64`，二进制和显式 wrapper 结构一致，但业务代码不需要暴露 wrapper 类型。

## XML required 注解

`xml_util.hpp` 新增 `iguana::xml_required`。C++26 分支会通过 `std::meta::annotations_of` 收集带该注解的字段名，`xml_reader.hpp` 的 required 检查同时支持旧 `REQUIRED(type, fields...)` 宏和新字段注解。

字段名仍然走统一的 `member_name`，因此 `xml_required` 可以和 `field_name` 注解组合使用：

```cpp
struct T {
  [[= ylt::reflection::field_name<"identifier">{}]]
  [[= iguana::xml_required{}]] int id;
};
```

required 检查现在记录已解析字段名列表并做精确匹配，不再用拼接字符串做 substring 查找。这样 `id` 和 `identifier` 这类重叠字段名不会误判 required 字段已出现。

## 可反射类型过滤

`util.hpp` 的 `ylt_refletable_v` 在 C++26 分支下更严格：

- 排除 string/container/fixed array/tuple/optional/variant/smart pointer/pb wrapper。
- 排除显式标记的反射包装类型。
- 避免把 `std::string`、`std::vector`、`std::optional` 或 XML wrapper 当成 message 继续反射内部实现。

`json_util.hpp` 和 `xml_util.hpp` 标记了需要排除的 wrapper：

- `numeric_str`
- `xml_attr_t`
- `xml_cdata_t`

`dynamic.hpp` 给 iguana 动态基类注册了 `skip_base_v`：

- `iguana::detail::base`
- `iguana::base_impl<T, ENABLE_FLAG>`

这避免 C++26 递归收集继承成员时把框架基类的虚函数状态或内部字段混入业务字段。

## 测试覆盖

新增和调整的测试点：

- `test/test_some.cpp`
  - 无宏 alias 仍可工作。
  - C++26 `field_name` 注解可驱动 JSON 写入和读取。
  - C++26 `skip_field` 注解可排除本地字段。

- `test/test_xml.cpp`
  - C++26 `struct_name` 和 `field_name` 注解可驱动 XML 根节点和字段名。
  - C++26 `xml_required` 注解可替代 `REQUIRED` 宏，并支持和 `field_name` 组合。
  - required 字段检查覆盖 `id` / `identifier` 这类字段名重叠场景，确认使用精确匹配。

- `test/test_pb.cpp`
  - `[[= iguana::pb_field(N)]]` 能生成正确 proto schema。
  - `[[= iguana::pb_zigzag]]` 和 `[[= iguana::pb_fixed]]` 能生成正确 wire 类型，并与 wrapper 类型二进制一致。
  - `[[= iguana::pb_zigzag]]` 和 `[[= iguana::pb_fixed]]` 覆盖 scalar、`std::optional<T>` 和 `std::vector<T>`。
  - `[[= iguana::pb_bytes]]` 能把字符串字段生成为 proto `bytes`，同时保持二进制编码与普通字符串一致。
  - `[[= iguana::pb_oneof<N...>]]` 能给 oneof 备选项指定非连续字段号，并跳过 `std::monostate`。
  - `[[= iguana::pb_optional]]` 能保持 proto3 optional presence，present default value 会写入并 roundtrip。
  - `[[= iguana::as_timestamp]]` 和 `[[= iguana::as_duration]]` 能按 protobuf well-known message 编码、解码和生成 schema import。
  - `[[= iguana::pb_unknown_fields]]` 能保留未知字段的原始 protobuf bytes，并在再序列化后恢复给新版本结构体，覆盖 group unknown field。
  - repeated primitive 读取兼容 packed、unpacked 和混合编码。
  - repeated primitive 读取兼容多个 packed chunk 追加合并。
  - map entry 读取兼容 key/value 任意顺序、entry 内 unknown field 和重复 key last-wins。
  - singular message/optional message 重复字段按 merge 解析，oneof 重复字段按 last-wins 解析。
  - malformed wire 覆盖 field number 0、非法 wire type、截断 length-delimited、截断 packed varint、过长 varint 和 fixed packed 坏长度。
  - unknown group 跳过受 protobuf 递归深度限制约束，深层 group 会抛出递归限制异常。
  - 注解字段号的二进制结果与 `YLT_REFL_PB` 旧宏一致。

- `test/test_proto3.cpp`
  - 启用本机 `protoc 3.21.12` 生成的 C++ runtime 对照。
  - 继续覆盖正常路径的 iguana/protoc serialized bytes 一致性。
  - 新增手写 wire 输入互操作：多 packed chunk + unpacked mixed repeated、singular message merge、oneof last-wins、map entry key/value 任意顺序、entry 内 unknown field、重复 map key last-wins。

- `test/conformance/iguana_conformance.cpp`
  - 新增官方 protobuf conformance runner 的 iguana testee。
  - 实现 runner pipe 协议，解析 `ConformanceRequest` 并返回 `ConformanceResponse`。
  - 当前处理 `protobuf_test_messages.proto3.TestAllTypesProto3` 的 binary protobuf payload 和 protobuf output；JSON/text/JSPB 是官方 runner 的其他表示形式，不属于 pb3 binary wire 格式，proto2/非 protobuf 输出明确 skipped。
  - 对 proto3 binary 输入做 wire 结构校验、递归深度限制、length/fixed/varint 截断检测、packed fixed 长度检测和 proto3 string UTF-8 校验；合法 payload 会按 `TestAllTypesProto3` schema 做 canonical binary re-encode 后返回。
  - canonical re-encode 覆盖 singular 默认值省略、repeated numeric packed/unpacked 输出规范、singular message merge、oneof last-wins 和 oneof message merge；该 testee 当前用于 proto3 binary protobuf-output conformance，不扩展到 JSON/text/proto2。

- `test/test_reflection.cpp`
  - C++26 下跳过依赖旧宏私有反射的断言。
  - C++26 下覆盖 `visit_members` 的逐字段 visitor 行为；旧路径继续覆盖参数包 visitor 行为。

`CMakeLists.txt` 新增 `ENABLE_CXX26_REFLECTION`：

- 打开后为现有测试生成 `_cpp26` 目标。
- 编译参数为 `-std=gnu++26 -freflection`。
- `_cpp26` 目标会从当前 C++ 编译器查询 `libstdc++.so` 位置并设置 build rpath，避免 GCC 16 构建产物在系统默认 libstdc++ 较旧时运行失败。
- CTest 同时覆盖普通目标和 C++26 目标。
- 新增 `iguana_conformance` target；如设置 `PROTOBUF_CONFORMANCE_RUNNER` cache 变量，可把官方 runner 作为 CTest 项接入。

`benchmark/pb_benchmark.cpp` 调整：

- 支持通过命令行传入迭代次数。
- 输出 iguana/protobuf 耗时比值，便于直接判断哪边更快。
- 修正 map serialize benchmark 的自比较断言。

## visit_members 当前行为

`visit_members` 入口仍然保留，但 C++26 和旧路径的调用契约不同。

旧路径仍支持一次性参数包 visitor：

```cpp
visit_members(obj, [](auto&... fields) {
  // 用户拿到字段参数包
});
```

C++26 路径当前直接复用 `for_each_data_member`，逐字段调用 visitor：

```cpp
visit_members(obj, [](auto& field, std::string_view name, std::size_t index) {
  // 每次处理一个字段
});
```

这和旧路径的 `visitor(field0, field1, ...)` 不等价。可变参数 lambda 在 C++26 下可能仍能编译，但它接收到的是单个字段调用的参数组，而不是全部字段组成的参数包。

## template for 和 index_sequence

逐字段访问已经优先使用 `template for`，例如字段名、注解扫描、JSON/XML/YAML 读取分发、protobuf C++26 字段收集都直接遍历 `std::meta::info` 数组。之前这种形式：

```cpp
[&]<std::size_t... Is>(std::index_sequence<Is...>) {
  (visitor(t.[:members[Is]:], std::string_view{},
           std::integral_constant<std::size_t, Is>{}),
   ...);
}(std::make_index_sequence<members.size()>{});
```

在逐字段执行场景下已经改成：

```cpp
template for (constexpr auto member : members) {
  visitor(t.[:member:], member_name<member>(), index++);
}
```

仍保留 `index_sequence` 或 pack expansion 的主要场景：

- 非 C++26 路径的 tuple 构造：`std::tie(field...)`、字段类型 tuple、`std::tuple_cat`。
- 非 C++26 路径的 frozen map 初始化：需要 `{name, index}...` 这种参数包。
- variant/pb 字段展开：protobuf oneof/variant 会把一个 C++ 字段展开为多个 wire field。
- enum/string 转换表：这些和 C++26 class reflection 无关。

这些位置使用 pack expansion 更直接，也更符合现有类型接口。把它们改成 `template for` 反而会引入中间容器或更复杂的状态代码。

## 后续注解方向

可以继续用注解简化的方向：

- XML attr/cdata：当前 wrapper 还承载实际存储结构，不能只靠注解等价替换；后续可以为普通字段提供注解式语法糖。
- protobuf proto2 required、extension、自定义 option 等更大 schema 语义。

当前没有把所有 wrapper 都改成注解，是因为 XML attr/cdata 还承载存储结构，proto2/extension 会改变兼容性和 schema 语义，需要单独测试矩阵。

## 关注点

需要确认的问题：

- `test/conformance/iguana_conformance.cpp` 当前是面向官方 runner 的 proto3 binary canonicalizer，没有直接调用 public `iguana::from_pb` / `iguana::to_pb`。public API 正确性由项目内单测和 protoc runtime 对照覆盖；官方 conformance 用于确认 proto3 binary wire 语义。
- C++26 路径使用 `access_context::unchecked`，可以反射 private data member。需要产品层明确这是预期能力，还是应该默认只反射公开成员/显式注册成员。

重点阅读区域：

- `reflect26_core.hpp` 的基类递归和 `skip_base` 判断是否符合业务继承模型。
- `reflect26_dispatch.hpp` 的线性 key 查找 + 类型化分发是否满足性能预期。
- `common.hpp` 中 protobuf 字段号校验是否会影响旧 `YLT_REFL_PB` 用户。
- `util.hpp` 的 C++26 `ylt_refletable_v` 排除条件是否覆盖所有 wrapper 类型。
- JSON/XML/YAML C++26 分支和旧分支在未知字段处理上的行为是否一致。

已处理问题：

- `pb_reader.hpp` unknown group 跳过路径已加 `pb_recursion_guard`，并新增深层 unknown group 回归测试。
- `xml_reader.hpp` required 字段检查已改为精确字段名匹配，并新增 `id` / `identifier` 重叠字段名回归测试。
- `NestedMsg.proto`、`test_vector.proto` 这类测试生成文件不再写到仓库根目录，根目录历史临时文件已删除并加入 `.gitignore` 兜底。
- C++26 GitHub Actions 容器内构建路径改用运行时 `$GITHUB_WORKSPACE`，避免 `${{ github.workspace }}` 展开到宿主 `/home/runner/work/...` 后让 `test_json_files_cpp26` 找不到 `data/`；同时为 JSON 文件测试固定 CTest 工作目录。

## 验证命令

核心验证命令：

```bash
cmake --build build_cpp26 --target test_some_cpp26 test_ut_cpp26 test_json_files_cpp26 test_xml_cpp26 test_yaml_cpp26 test_pb_cpp26 test_reflection_cpp26 -j2
cmake --build build_cpp26 -j2
cmake --build build_cpp26 --target test_proto -j2
ctest --test-dir build_cpp26 --output-on-failure
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_proto
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_pb
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_xml
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_pb_cpp26
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_xml_cpp26
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ctest --test-dir build_cpp26 --output-on-failure
cmake --build build_cpp26 --target iguana_conformance pb_benchmark -j2
LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/pb_benchmark 100000
git diff --check
```

官方 protobuf conformance reference 验证使用 `v3.21.12` 源码临时构建：

```bash
cmake -S .cache/protobuf-v3.21.12 -B .cache/protobuf-v3.21.12-build -Dprotobuf_BUILD_CONFORMANCE=ON -Dprotobuf_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build .cache/protobuf-v3.21.12-build --target conformance_test_runner conformance_cpp -j2
.cache/protobuf-v3.21.12-build/conformance_test_runner --text_format_failure_list .cache/protobuf-v3.21.12/conformance/text_format_failure_list_cpp.txt .cache/protobuf-v3.21.12-build/conformance_cpp
.cache/protobuf-v3.21.12-build/conformance_test_runner build_cpp26/iguana_conformance
.cache/protobuf-v3.21.12-build/conformance_test_runner --enforce_recommended build_cpp26/iguana_conformance
```

最终验证结果：

- `cmake --build build_cpp26 -j2`：通过，普通目标和 `_cpp26` 目标均完成构建。
- `cmake --build build_cpp26 --target test_proto -j2`：通过，使用 `protoc 3.21.12` 生成的 C++ fixture。
- `ctest --test-dir build_cpp26 --output-on-failure`：重新配置后通过，`_cpp26` 目标通过 build rpath 找到 GCC 16 libstdc++，`test_json_files_cpp26` 通过固定 CTest 工作目录读取源码树 `data/`。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_pb`：11/11 test cases、268/268 assertions 通过。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_xml`：20/20 test cases、227/227 assertions 通过。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_proto`：11/11 test cases、54/54 assertions 通过。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_pb_cpp26`：12/12 test cases、369/369 assertions 通过。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/test_xml_cpp26`：22/22 test cases、236/236 assertions 通过。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ctest --test-dir build_cpp26 --output-on-failure`：21/21 通过。
- 官方 `conformance_test_runner + conformance_cpp` reference：带 `text_format_failure_list_cpp.txt` 后通过。Binary/JSON suite 为 1989 successes、0 unexpected failures；Text-format suite 为 100 successes、20 expected failures、0 unexpected failures。临时 `.cache/protobuf-v3.21.12*` 源码和构建目录位于 `.cache/`，已被 `.gitignore` 忽略。
- 官方 `conformance_test_runner + build_cpp26/iguana_conformance --enforce_recommended`：通过。Binary/JSON suite 为 651 successes、1366 skipped、0 expected failures、0 unexpected failures；Text-format suite 为 0 successes、120 skipped、0 expected failures、0 unexpected failures。
- `LD_LIBRARY_PATH=/opt/gcc-16.1.0/lib64 ./build_cpp26/pb_benchmark 100000`：当前机器结果如下：
  - many serialize：iguana/protobuf `0.568x`
  - many deserialize：`0.810x`
  - simple serialize：`0.343x`
  - simple deserialize：`0.395x`
  - monster serialize：`0.535x`
  - monster deserialize：`0.368x`
  - int32 deserialize：`0.522x`
  - 比值小于 1 表示 iguana 更快；本次样本中 iguana 全部快于官方 C++ protobuf runtime。
- `git diff --check`：通过，无 whitespace/error 输出。

conformance 说明：`iguana_conformance` 目前是 proto3 binary protobuf-output 验收 testee。JSON/text/JSPB 属于官方 runner 的其他表示形式，不是 pb3 binary wire 格式；proto3 binary protobuf input/output 已在 `--enforce_recommended` 下通过，未记录未通过项。

构建过程中仍能看到既有 warning，主要来自 `frozen/string.h` 的 C++23 literal suffix 提示、JSON optional/variant 路径的 `maybe-uninitialized` 提示，以及 dynamic 测试里的 `stringop-overflow` 提示。这些历史 warning 不属于本次 C++26 反射改造内容。
