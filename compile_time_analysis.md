# Iguana 全量编译时间分析

日期：2026-05-31

本文档记录本地 MSVC 环境下的全量构建基线、热点定位、已验证的优化原型，以及下一步建议。这里的“全量构建”指当前 CMake 默认行为：同时构建 tests、examples 和 benchmarks，而不是只构建一个 header-only 库目标。

## 测试环境

| 项目 | 值 |
| --- | --- |
| 仓库目录 | `D:\code\iguana` |
| CMake 生成器 | Visual Studio 17 2022 |
| 构建配置 | Release |
| 并行度 | `--parallel 12` |
| CMake | 4.0.3 |
| Visual Studio | VS 2022 Community 17.14.16 |
| MSVC toolset | 14.44.35207 |
| 编译器 | MSVC 19.44.35217 |
| 默认 C++ 标准 | C++17 |

说明：当前实测数据来自 MSVC。本计划后续需要把 GCC 和 Clang 纳入同一套复测矩阵，因为三类编译器在模板实例化、优化器耗时、PCH 和缓存上的行为差异很大。

基线测试命令：

```powershell
cmake -S . -B out\perf_base_vs -G "Visual Studio 17 2022" -A x64
cmake --build out\perf_base_vs --config Release --parallel 12 -- /v:minimal
```

## 当前基线

| 步骤 | 耗时 |
| --- | ---: |
| CMake 配置 | 11.20 s |
| Release 全量构建 | 164.87 s |
| 配置 + 构建合计 | 176.07 s |

当前全量构建会生成 18 个可执行目标：

```text
iguana_conformance
json_benchmark
json_example
test_cpp20
test_json_files
test_nothrow
test_pb
test_reflection
test_some
test_ut
test_util
test_xml
test_xml_nothrow
test_yaml
xml_benchmark
xml_example
yaml_benchmark
yaml_example
```

目标：把 `164.87 s` 的 Release 全量构建时间至少降低 50%，也就是降到 `82.44 s` 或更低。

## 热点排序

下面的数据是用 MSVC Release 风格参数逐个编译源文件得到的。绝对时间包含每次启动 VS 开发环境的开销，所以更适合用于判断热点排序，而不是精确归因到最终构建墙钟时间。

| 排名 | 源文件 | 耗时 |
| ---: | --- | ---: |
| 1 | `test/test_json_files.cpp` | 42.02 s |
| 2 | `test/test_pb.cpp` | 38.59 s |
| 3 | `benchmark/json_benchmark.cpp` | 35.48 s |
| 4 | `test/test_yaml.cpp` | 29.83 s |
| 5 | `test/test_xml.cpp` | 28.28 s |
| 6 | `test/unit_test.cpp` | 24.56 s |
| 7 | `test/test_some.cpp` | 22.05 s |
| 8 | `example/yaml_example.cpp` | 12.34 s |
| 9 | `example/xml_example.cpp` | 12.32 s |
| 10 | `benchmark/xml_benchmark.cpp` | 8.70 s |
| 11 | `test/test_xml_nothrow.cpp` | 8.30 s |
| 12 | `benchmark/yaml_benchmark.cpp` | 8.12 s |
| 13 | `test/test_yaml_nothrow.cpp` | 7.36 s |
| 14 | `test/test_yaml_bech.cpp` | 6.28 s |
| 15 | `example/json_example.cpp` | 6.06 s |
| 16 | `test/test_reflection.cpp` | 5.07 s |
| 17 | `test/test_cpp20.cpp` | 4.22 s |
| 18 | `test/test_util.cpp` | 4.14 s |
| 19 | `test/conformance/iguana_conformance.cpp` | 3.28 s |

最慢的文件都不是库产物本身，而是测试或 benchmark 源文件。这些文件 include 了大量 header-only 模板代码，并在每个翻译单元里触发实例化。

## 关键验证：优化级别是主要瓶颈

对最慢的几个翻译单元分别用 `/O2` 和 `/Od` 编译，结果如下：

| 源文件 | `/O2` | `/Od` | 降幅 |
| --- | ---: | ---: | ---: |
| `test/test_json_files.cpp` | 36.82 s | 11.93 s | 67.6% |
| `test/test_pb.cpp` | 36.61 s | 12.99 s | 64.5% |
| `benchmark/json_benchmark.cpp` | 33.40 s | 11.67 s | 65.1% |
| `test/test_yaml.cpp` | 26.52 s | 6.01 s | 77.3% |
| `test/test_xml.cpp` | 25.14 s | 5.67 s | 77.4% |
| `test/unit_test.cpp` | 23.33 s | 9.20 s | 60.6% |
| `test/test_some.cpp` | 20.99 s | 6.24 s | 70.3% |

这个结果说明：全量 Release 构建慢的主要原因不是单纯的 include 数量，而是 **MSVC 在模板重代码上做 `/O2` 优化非常耗时**。测试程序并不需要优化后的机器码才能验证语义，因此它们是最适合降优化级别的对象。

## 全量 no-opt 原型验证

为了确认这个方向能影响全量墙钟时间，做了一个最小原型：保持当前 18 个目标都构建，只把 Release flags 改成 `/Od /Ob0 /DNDEBUG`。

验证命令：

```powershell
cmake -S . -B out\perf_od_vs -G "Visual Studio 17 2022" -A x64 `
  -DCMAKE_CXX_FLAGS_RELEASE="/Od /Ob0 /DNDEBUG"
cmake --build out\perf_od_vs --config Release --parallel 12 -- /v:minimal
```

结果：

| 场景 | 配置耗时 | 构建耗时 | 相对基线 |
| --- | ---: | ---: | ---: |
| 当前 Release 全量构建 | 11.20 s | 164.87 s | 基线 |
| 全部目标 `/Od` 原型 | 7.98 s | 32.24 s | 降低 80.4% |

这个原型不是最终方案，因为 benchmarks 如果要用于性能测试，应该继续用优化编译。但它证明了核心判断：**只要避免把测试和示例按 `/O2` 编译，全量构建时间就能大幅下降**。

## 推荐方案实测结果

已验证的推荐方案：

| 目标类别 | Release 编译策略 |
| --- | --- |
| tests | `/Od /Ob0` |
| examples | `/Od /Ob0` |
| conformance runner | `/Od /Ob0` |
| benchmarks | 保持 Release 优化 |

复测命令：

```powershell
cmake -S . -B out\perf_fast_validation_vs -G "Visual Studio 17 2022" -A x64
cmake --build out\perf_fast_validation_vs --config Release --parallel 12 -- /v:minimal
ctest --test-dir out\perf_fast_validation_vs -C Release --output-on-failure -j 1
```

复测结果：

| 场景 | 配置耗时 | 构建耗时 | 相对基线 |
| --- | ---: | ---: | ---: |
| 当前 Release 全量构建 | 11.20 s | 164.87 s | 基线 |
| 推荐方案 | 8.86 s | 68.09 s | 降低 58.7% |

也就是全量构建从 `164.87 s` 降到 `68.09 s`，快了约 `96.78 s`，构建速度约为原来的 `2.42x`。这已经超过“降低至少 50%”的目标。

测试结果：

| 测试 | 结果 |
| --- | --- |
| `ctest -C Release` | 10/10 通过 |
| 测试总耗时 | 5.76 s |

## Debug 构建影响

这个优化主要针对 Release 全量构建。Debug 配置下，MSVC 生成的 benchmark、test 和 example 目标原本就是低优化：

| 目标 | Debug 优化设置 |
| --- | --- |
| `json_benchmark` | `Optimization=Disabled` |
| `test_json_files` | `Optimization=Disabled` |
| `json_example` | `Optimization=Disabled` |

因此当前改动对 Debug 构建基本没有加速空间。

Debug 全量构建参考数据：

| 场景 | 配置耗时 | 构建耗时 | 说明 |
| --- | ---: | ---: | --- |
| Debug 全量构建 | 7.45 s | 31.58 s | 当前改动前后预期基本一致 |

结论：Debug 下提升约为 `0%`。Debug 本身已经接近“全部目标 `/Od` 原型”的 `32.24 s`，慢点主要来自模板解析和代码生成，而不是优化器。

## 其他方向验证

### 关闭警告

对几个慢文件测试了 `/w`。结果波动较大，没有稳定收益。警告输出会让日志变吵，但不是当前主要耗时来源。

### PCH

做了一个临时通用 PCH 探测。PCH 创建耗时约 `8.04 s`。在可成功编译的慢文件上，收益不明显：

| 源文件 | 普通 `/O2` | PCH `/O2` |
| --- | ---: | ---: |
| `test/test_json_files.cpp` | 36.82 s | 36.83 s |
| `test/test_yaml.cpp` | 26.52 s | 27.32 s |

此外，过宽的通用 PCH 会碰到 include 顺序和宏定义风险，例如 doctest 的 `DOCTEST_CONFIG_IMPLEMENT` 这类宏。当前 tests 大多是单源文件目标，CMake 默认的 per-target PCH 还会为每个目标单独生成 PCH，收益更不稳定。因此 PCH 暂不作为优先方案。

### `/MP`

当前 VS 工程没有看到 `/MP`，可以作为补充项加入 MSVC 编译选项。但当前多数目标只有一个 `.cpp`，而外层 `cmake --build --parallel 12` 已经在并行构建多个目标，所以 `/MP` 不是主要杠杆。

## 推荐优化方案

### 方案一：测试和示例 Release 下使用低优化级别

这是当前已验证、收益最大的优化。

保留全量目标，也就是 tests、examples、benchmarks 都仍然可以构建；但把非性能目标在 Release 下改成低优化级别：

| 目标类别 | Release 编译策略 | 原因 |
| --- | --- | --- |
| tests | `/Od /Ob0` 或 `-O0` | 测试只需要验证语义，不需要优化代码 |
| examples | `/Od /Ob0` 或 `-O0` | 示例只需要能编译运行，不承担性能数据 |
| conformance runner | `/Od /Ob0` 或 `-O0` | 主要用于兼容性验证 |
| benchmarks | 保持 `/O2` 或 `-O3` | benchmark 结果依赖优化后性能 |

建议在 CMake 里加一个 helper，例如：

```cmake
function(iguana_fast_validation_target target)
    if(MSVC)
        target_compile_options(${target} PRIVATE
            "$<$<CONFIG:Release>:/Od;/Ob0>")
    else()
        target_compile_options(${target} PRIVATE
            "$<$<CONFIG:Release>:-O0>")
    endif()
endfunction()
```

然后应用到 `test_*`、`iguana_conformance` 和 `*_example` 目标。benchmark 目标先不应用，保证 benchmark 仍然有意义。

实测结果：Release 全量构建从 `164.87 s` 降到 `68.09 s`，降低 `58.7%`。不会达到“全部目标 `/Od` 原型”的 `32.24 s` 那么低，因为 `json_benchmark` 等 benchmark 仍然保留优化编译；但它已经明显低于 `82.44 s` 的目标。

### 方案二：增加快速 benchmark 编译开关

如果目标是“CI 或本地全量只验证能否编译”，而不是运行 benchmark 得到性能数据，可以增加一个显式选项：

```cmake
option(IGUANA_FAST_BENCHMARK_COMPILE
       "Compile benchmark targets with low optimization for faster full builds"
       OFF)
```

开启后 benchmark 也使用 `/Od` 或 `-O0`。这会接近已验证的 `32.24 s` 原型，但该模式下生成的 benchmark 可执行文件不应用来比较运行性能。

### 方案三：默认目标拆分仍然有用，但不是本次主解

把默认构建改成只构建 header-only interface target，可以改善普通用户的默认构建体验。但它只是减少默认构建目标数量，不能解决“我要构建完整 tests/examples/benchmarks 时仍然慢”的问题。

因此默认目标拆分可以保留为次要优化，但本次全量构建的主优化应优先做“测试和示例低优化编译”。

## 建议复测矩阵

应用 CMake 改动后，建议记录以下场景：

| 场景 | 配置耗时 | 构建耗时 | 说明 |
| --- | ---: | ---: | --- |
| 当前 Release 全量构建 | 11.20 s | 164.87 s | 当前基线 |
| 全部目标 `/Od` 原型 | 7.98 s | 32.24 s | 已验证，用于证明方向 |
| tests/examples `/Od`，benchmarks 保持优化 | 8.86 s | 68.09 s | 已验证，推荐最终方案 |
| tests/examples/benchmarks 全部 `/Od` | 8.6 s | 23.56 s | `IGUANA_FAST_BENCHMARK_COMPILE=ON` 快速编译模式，benchmark 不用于性能比较 |
| 默认只构建 interface target | 待测 | 待测 | 用户体验优化，不代表全量构建 |

推荐复测命令：

```powershell
cmake -S . -B out\perf_fast_validation_vs -G "Visual Studio 17 2022" -A x64
cmake --build out\perf_fast_validation_vs --config Release --parallel 12 -- /v:minimal
ctest --test-dir out\perf_fast_validation_vs -C Release --output-on-failure
```

验收标准：

1. Release 全量构建时间低于 `82.44 s`。
2. `ctest` 通过。
3. benchmark 目标默认仍使用优化编译；如果启用快速 benchmark 编译，需要在文档中明确该模式不能用于性能对比。

## 后续专项优化计划

上面的推荐方案属于“构建策略优化”：它避免 tests/examples/conformance 在 Release 下浪费大量优化器时间，但还没有减少 iguana 模板本身的实例化成本。后续如果要继续做“治本”的编译性能优化，建议按下面顺序推进。

### 阶段 0：保留已验证的快速收益

目标：先保留已经证明有效、风险较低的方案。

计划：

1. 保持 tests、examples、conformance 在 Release 下使用低优化级别。
2. benchmark 默认保持 Release 优化，确保性能测试结果不被污染。
3. 可选增加一个显式开关，例如 `IGUANA_FAST_BENCHMARK_COMPILE=ON`，只在需要快速验证“能否编译”时让 benchmark 也低优化。

验收：

| 指标 | 目标 |
| --- | --- |
| Release 全量构建 | 低于 `82.44 s` |
| 当前实测 | `68.09 s` |
| `ctest -C Release` | 通过 |

### 阶段 1：做精确热点归因

目标：找出真正消耗编译时间的模板路径，而不是只看哪个 `.cpp` 慢。

优先分析对象：

| 文件 | 原因 |
| --- | --- |
| `benchmark/json_benchmark.cpp` | benchmark 必须保持优化编译，是 Release 全量里剩下的核心成本 |
| `test/test_pb.cpp` | PB schema、variant、reflection 路径复杂 |
| `test/test_json_files.cpp` | JSON DOM、reader/writer、variant 路径复杂 |
| `test/test_yaml.cpp` | YAML reader/writer 模板实例化较重 |
| `test/test_xml.cpp` | XML reader/writer 模板实例化较重 |

建议验证方法：

1. MSVC：用 `/d1reportTime` 或 build log 观察前端、后端、模板实例化耗时。
2. Clang：用 `-ftime-trace` 生成 JSON trace，定位最重的模板和头文件。
3. GCC/Clang：用 `-ftime-report` 看 parser、template instantiation、optimization 的占比。
4. 对每个候选改动都用单文件编译和全量构建双重复测，避免局部优化在全量里没有收益。

跨编译器诊断矩阵：

| 编译器 | 主要诊断参数 | 重点观察 |
| --- | --- | --- |
| MSVC | `/d1reportTime`、`/Bt+` | 前端解析、后端优化、单文件耗时 |
| Clang | `-ftime-trace`、`-ftime-report` | 最重模板实例化、头文件解析、优化 pass |
| GCC | `-ftime-report` | parsing、template instantiation、optimization 占比 |

建议先用 Clang 的 `-ftime-trace` 做模板热点定位，因为它输出的 JSON 更容易反查具体模板和头文件；再用 MSVC/GCC 复测这些热点是否也成立。

输出：

| 输出物 | 说明 |
| --- | --- |
| 热点模板列表 | 例如 `from_json_impl`、`from_pb_impl`、`std::variant` visitor、reflection member traversal |
| 热点头文件列表 | 例如 `common.hpp`、`pb_util.hpp`、`json_reader.hpp`、`value.hpp` |
| 前端/后端占比 | 判断该优先减少模板实例化还是减少优化器负担 |

### 阶段 2：治理 include 依赖

目标：减少不必要的头文件传播，降低业务项目和测试目标的重复解析成本。

判断：这个方向靠谱，尤其对增量构建和大型业务项目有效。

计划：

1. 梳理 `iguana/iguana.hpp`、`common.hpp`、`pb_util.hpp`、`json_reader.hpp`、`json_writer.hpp` 的 include 图。
2. 把只在实现细节里需要的重头文件尽量下沉到具体 reader/writer 头中。
3. 对业务侧使用文档给出建议：模型声明头不 include iguana，序列化逻辑放在单独 `.hpp/.cpp` 或较上层文件里。
4. 避免推荐“在函数体里 include iguana”这种写法；更推荐分离模型声明和序列化入口。

验收：

| 指标 | 目标 |
| --- | --- |
| include 图 | 明确重头文件入口 |
| 单文件预处理体积 | 重点文件下降 |
| 增量构建 | 修改普通模型头时受影响 TU 减少 |

### 阶段 3：减少重复模板实例化

目标：降低多个翻译单元重复实例化相同序列化路径的成本。

判断：方向有价值，但不能简单套 `extern template`。

原因：

1. iguana 的核心函数大量使用 `IGUANA_INLINE`，MSVC 下是 `__forceinline`。
2. `extern template` 不能可靠抑制 inline function 为了内联而发生的实例化。
3. `to_json/from_json` 的实际模板参数包含引用类别、stream 类型、iterator 类型，签名容易写错。
4. 当前测试里很多类型只在单个 `.cpp` 内使用，显式实例化收益有限。

更稳妥的实验方向：

1. 先针对少数稳定、高频、跨多个 TU 使用的业务类型设计非 inline wrapper。
2. wrapper 在 `.cpp` 中显式实例化或显式定义，其他 TU 只调用普通函数。
3. 对 benchmark 中固定类型，例如 `obj_t`，实验是否能把部分 reader/writer 路径外置。

示意：

```cpp
// user_json.hpp
struct User;
std::string user_to_json(const User& user);
void user_from_json(User& user, std::string_view json);

// user_json.cpp
#include <iguana/json_reader.hpp>
#include <iguana/json_writer.hpp>
#include "user.hpp"

std::string user_to_json(const User& user) {
    std::string out;
    iguana::to_json(user, out);
    return out;
}

void user_from_json(User& user, std::string_view json) {
    iguana::from_json(user, json);
}
```

验收：

| 指标 | 目标 |
| --- | --- |
| 跨 TU 重复实例化 | 减少 |
| 增量构建 | 调用方不再因 include iguana 重编重模板 |
| 运行性能 | wrapper 不引入不可接受开销 |

### 阶段 4：PCH 小范围验证

目标：确认 PCH 是否适合具体业务 target，而不是默认认为它是银弹。

当前最小验证结果：

| 源文件 | 普通 `/O2` | PCH `/O2` |
| --- | ---: | ---: |
| `test/test_json_files.cpp` | 36.82 s | 36.83 s |
| `test/test_yaml.cpp` | 26.52 s | 27.32 s |

结论：对当前仓库的单源文件测试目标，通用 PCH 收益不明显。PCH 更适合一个业务 target 有很多 `.cpp`，并且这些 `.cpp` 都稳定 include 同一批重头文件的场景。

计划：

1. 不在当前仓库默认强制开启 PCH。
2. 如果未来有多 `.cpp` 的业务 target，再对该 target 单独试 `target_precompile_headers`。
3. PCH 内容避免包含带宏开关行为的测试框架入口，例如 doctest 的 `DOCTEST_CONFIG_IMPLEMENT`。

跨编译器注意事项：

| 编译器 | PCH 注意点 |
| --- | --- |
| MSVC | `.pch` 通常按 target/config 生成；单源文件 target 收益有限 |
| Clang | PCH/modules 机制更灵活，但宏和编译选项变化会导致失效 |
| GCC | `.gch` 对头文件路径、宏、编译选项敏感，工程化维护成本较高 |

PCH 不建议作为全局默认优化；更适合在具体业务 target 中按需启用，并分别对 MSVC/GCC/Clang 复测。

验收：

| 指标 | 目标 |
| --- | --- |
| PCH 创建时间 | 小于节省时间 |
| 全量构建 | 有稳定收益 |
| 增量构建 | 修改普通 `.cpp` 时收益明显 |

### 阶段 5：构建工具链优化

目标：利用构建系统和缓存改善开发迭代体验。

计划：

1. MSVC 下评估 `/MP`。当前很多 target 是单 `.cpp`，且已经使用 `cmake --build --parallel 12`，所以预期收益有限；如果后续拆分大测试文件，`/MP` 的价值会上升。
2. Windows 可评估 `sccache`，Linux/macOS 可评估 `ccache`。
3. CI 使用缓存时，重点缓存 compiler launcher 结果，而不是只缓存 build 目录。
4. `/Zc:preprocessor` 单独小测，不预设一定更快。

跨平台建议：

| 平台/编译器 | 优先工具 | CMake 方式 | 说明 |
| --- | --- | --- | --- |
| MSVC | `/MP`、`sccache` | `target_compile_options(... /MP)`、`CMAKE_CXX_COMPILER_LAUNCHER=sccache` | `/MP` 适合单 target 多 `.cpp`；sccache 适合重复构建 |
| clang-cl | `/MP`、`sccache` | 同 MSVC 风格参数 | 需要单独确认 clang-cl 对现有 flags 的兼容性 |
| GCC | `ccache`、`-ftime-report` | `CMAKE_CXX_COMPILER_LAUNCHER=ccache` | 首次全量收益小，二次构建收益大 |
| Clang | `ccache/sccache`、`-ftime-trace` | `CMAKE_CXX_COMPILER_LAUNCHER=ccache` 或 `sccache` | 最适合做模板热点 trace |

注意：ccache/sccache 不是优化“首次干净全量构建”的主要手段。它们主要优化未改动源码的重复构建、CI 缓存和小改动增量构建。

验收：

| 场景 | 预期收益 |
| --- | --- |
| 首次干净全量构建 | cache 基本无收益 |
| 未改源码的重复构建 | cache 应接近秒级 |
| 只改少量 `.cpp` | cache 应明显减少重编时间 |
| 修改 iguana 核心头 | cache 命中会显著下降 |

### 阶段 6：编译器版本对比

目标：确认升级编译器是否能自然降低模板编译成本。

计划：

1. MSVC：对比当前 `19.44.35217` 和更新 VS 2022 patch 的 Release 全量构建。
2. GCC：对比 GCC 11、13、16。
3. Clang：对比 Clang 14、16、18+。
4. 对 C++20/C++26 反射路径单独建表，不和默认 C++17 路径混在一起。

建议复测配置：

| 编译器 | C++ 标准 | 构建类型 | 是否启用 C++26 reflection |
| --- | --- | --- | --- |
| MSVC 19.44+ | C++17、C++20 | Debug、Release | 否 |
| GCC 11/13 | C++17、C++20 | Debug、Release | 否 |
| GCC 16 | C++17、C++20、C++26 | Debug、Release | C++26 单独测 |
| Clang 16/18+ | C++17、C++20 | Debug、Release | 否 |

C++26 reflection 路径必须单独看，因为它引入新的编译器前端能力和 `<meta>` 支持，不能和当前宏反射/C++17 路径直接比较。

验收：

| 指标 | 目标 |
| --- | --- |
| 同一代码同一 CMake 配置 | 只替换编译器版本 |
| Release 全量构建 | 记录变化 |
| Debug 全量构建 | 记录变化 |
| 单文件热点 | 记录变化 |

## 方法优先级总结

| 优先级 | 方法 | 判断 |
| ---: | --- | --- |
| 1 | tests/examples/conformance Release 低优化 | 已验证，收益最大，风险低 |
| 2 | 精确热点归因 | 必须做，否则容易盲改 |
| 3 | include 依赖治理 | 对增量构建和业务接入价值高 |
| 4 | 非 inline wrapper / 显式实例化实验 | 有潜力，但只适合稳定高频类型 |
| 5 | ccache/sccache | 对重复构建和 CI 有价值 |
| 6 | `/MP` | 当前单 `.cpp` 目标收益有限，拆分后再看 |
| 7 | PCH | 当前仓库小测收益不明显，只建议按 target 验证 |
| 8 | 直接套 `extern template` | 不建议优先做，容易复杂且收益不稳 |

## 跨编译器复测计划

为了避免只优化 MSVC，后续每个候选方案都应该至少跑下面三类编译器：

| 编译器族 | 推荐生成器 | 重点问题 |
| --- | --- | --- |
| MSVC | Visual Studio 或 Ninja | Release `/O2` 后端优化是否仍是主瓶颈 |
| GCC | Ninja 或 Unix Makefiles | `-O3` 下 template instantiation 和 optimization 占比 |
| Clang | Ninja 或 Unix Makefiles | `-ftime-trace` 定位出的热点是否与 MSVC/GCC 一致 |

基础复测命令模板：

```powershell
# MSVC
cmake -S . -B out\msvc_release -G "Visual Studio 17 2022" -A x64
cmake --build out\msvc_release --config Release --parallel 12 -- /v:minimal
```

```bash
# GCC
CXX=g++ CC=gcc cmake -S . -B out/gcc_release -DCMAKE_BUILD_TYPE=Release
cmake --build out/gcc_release -j"$(nproc)"
```

```bash
# Clang
CXX=clang++ CC=clang cmake -S . -B out/clang_release -DCMAKE_BUILD_TYPE=Release
cmake --build out/clang_release -j"$(nproc)"
```

候选优化的记录表：

| 方案 | MSVC Release | GCC Release | Clang Release | Debug 影响 | 结论 |
| --- | ---: | ---: | ---: | ---: | --- |
| 当前基线 | `164.87 s` | 待测 | 待测 | `31.58 s` on MSVC | MSVC 已测 |
| tests/examples 低优化 | `68.09 s` | 待测 | 待测 | 约 `0%` | 需确认 GCC/Clang |
| PCH | 无稳定收益 | 待测 | 待测 | 待测 | 按 target 验证 |
| cache launcher | 首次全量收益小 | 待测 | 待测 | 增量更重要 | 看 CI/二次构建 |
| include 治理 | 待测 | 待测 | 待测 | 增量更重要 | 需要热点驱动 |

只有在 MSVC/GCC/Clang 至少两类编译器上都有稳定收益的改动，才建议作为库级默认优化；只对单个编译器有效的优化，应使用编译器条件或 CMake option 包起来。

## Clang 主导执行计划

后续工作以 Clang 为主要分析工具，MSVC/GCC 作为结果验证工具。原因是 Clang 的 `-ftime-trace` 能直接输出模板实例化、头文件解析和优化 pass 的耗时，适合定位“为什么慢”。

### 执行原则

1. 每一项实验都记录命令、耗时、结果和结论。
2. 每次只改变一个变量，例如编译器、优化级别、PCH、cache 或 include 结构。
3. 先测单文件，再测全量构建，避免局部收益无法转化成全量收益。
4. benchmark 默认保持优化编译；任何降低 benchmark 优化级别的实验都必须标记为“只验证编译，不用于性能对比”。
5. MSVC/GCC/Clang 至少两类编译器上稳定有效的方案，才考虑作为默认优化。

### 任务清单

| 序号 | 任务 | 状态 | 输出 |
| ---: | --- | --- | --- |
| 1 | 确认 Windows 上 clang-cl/clang++ 可用 | 待执行 | 工具版本、路径 |
| 2 | 建立 clang-cl Release/Debug 全量构建基线 | 待执行 | 配置耗时、构建耗时、测试结果 |
| 3 | 对慢文件生成 `-ftime-trace` | 待执行 | trace 文件、热点列表 |
| 4 | 归纳 Clang 热点类型 | 待执行 | 模板实例化、头文件解析、优化器占比 |
| 5 | 选 1-2 个小优化实验 | 待执行 | 单文件耗时和全量耗时对比 |
| 6 | 用 MSVC/GCC 复测有效方案 | 待执行 | 跨编译器结果 |
| 7 | 整理默认启用/可选启用/不建议启用方案 | 待执行 | 最终建议 |

### Clang 基线命令

Windows 上优先使用 `clang-cl`，因为它兼容 MSVC ABI 和 MSVC STL，更接近 Windows 用户的实际使用环境：

```powershell
cmake -S . -B out\clangcl_release -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build out\clangcl_release --config Release --parallel 12 -- /v:minimal
ctest --test-dir out\clangcl_release -C Release --output-on-failure -j 1
```

```powershell
cmake -S . -B out\clangcl_debug -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build out\clangcl_debug --config Debug --parallel 12 -- /v:minimal
ctest --test-dir out\clangcl_debug -C Debug --output-on-failure -j 1
```

### Clang Trace 目标文件

优先 trace 以下文件：

| 文件 | 原因 |
| --- | --- |
| `benchmark/json_benchmark.cpp` | benchmark 保持优化编译，是 Release 全量剩余大头 |
| `test/test_pb.cpp` | PB schema、variant、reflection 路径复杂 |
| `test/test_json_files.cpp` | JSON DOM、reader/writer、variant 路径复杂 |
| `test/test_yaml.cpp` | YAML reader/writer 模板实例化较重 |
| `test/test_xml.cpp` | XML reader/writer 模板实例化较重 |

### 执行记录

| 时间 | 项目 | 命令/配置 | 结果 | 结论 |
| --- | --- | --- | --- | --- |
| 2026-05-31 | MSVC Release 基线 | VS 2022, Release, parallel 12 | `164.87 s` | Release 慢主要来自优化器处理重模板目标 |
| 2026-05-31 | MSVC 推荐方案 | tests/examples/conformance `/Od /Ob0`，benchmarks 保持优化 | `68.09 s`，`ctest 10/10` | 构建策略优化有效，降低 `58.7%` |
| 2026-05-31 | MSVC Debug | VS 2022, Debug, parallel 12 | `31.58 s` | Debug 本来已低优化，当前方案基本无收益 |
| 2026-05-31 | Clang 工具链检查 | `clang-cl --version`、`clang++ --version` | Clang `18.1.8`，路径 `D:\Program Files\LLVM\bin` | `clang-cl` 和 `clang++` 可用；`ninja` 不在 PATH，优先用 VS 生成器 `-T ClangCL` |
| 2026-05-31 | Clang 构建执行门禁 | 用户要求正式编译前先确认 | 待用户明确说“开始” | 暂不启动 Release/Debug 全量构建和 `-ftime-trace` 编译 |
| 2026-05-31 | clang-cl + VS 生成器 | `-G "Visual Studio 17 2022" -T ClangCL` | 配置失败 | VS 未安装 `ClangCL` 平台工具集 |
| 2026-05-31 | clang-cl + Ninja fallback | VS 自带 Ninja + standalone `clang-cl 18.1.8` | 配置成功，编译失败 | MSVC STL `14.44.35207` 要求 Clang `19.0.0+`，本机 Clang `18.1.8` 触发 `STL1000` |
| 2026-05-31 | Clang trace override | `clang-cl 18.1.8` + `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` + `-ftime-trace` | 5 个慢文件 trace 成功 | 仅用于热点探索，不作为正式 Clang 支持基线 |

### 待确认执行命令

以下命令只作为准备记录，尚未执行。并行度保持 `12`，用于和 MSVC 基线对齐。

```powershell
cmake -S . -B out\clangcl_release -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build out\clangcl_release --config Release --parallel 12 -- /v:minimal
ctest --test-dir out\clangcl_release -C Release --output-on-failure -j 1
```

```powershell
cmake -S . -B out\clangcl_debug -G "Visual Studio 17 2022" -A x64 -T ClangCL
cmake --build out\clangcl_debug --config Debug --parallel 12 -- /v:minimal
ctest --test-dir out\clangcl_debug -C Debug --output-on-failure -j 1
```

`-ftime-trace` 单文件分析将在全量基线之后执行，且每个慢文件单独编译，避免并行任务干扰 trace 和耗时记录。

### Clang 阻塞结论

当前 Windows Clang 基线暂时不能作为正式数据继续跑，原因不是 iguana 代码，而是工具链版本不匹配：

| 组件 | 当前版本/状态 |
| --- | --- |
| LLVM standalone `clang-cl` | `18.1.8` |
| MSVC STL | `14.44.35207`，`_MSVC_STL_UPDATE 202503L` |
| STL 要求 | Clang `19.0.0` 或更新 |
| VS ClangCL 平台工具集 | 未安装 |
| VS 自带 Ninja | 可用 |

可选处理方式：

1. 安装 LLVM/Clang `19+`，然后继续用 `Ninja + clang-cl` 跑正式基线。
2. 安装 Visual Studio 的 `ClangCL` 平台工具集，然后继续用 VS 生成器 `-T ClangCL`。
3. 仅用于探索性 trace 时，可以定义 `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` 绕过 STL 版本检查；该结果不能作为正式支持基线，必须在记录中标注“unsupported toolchain override”。

建议：正式基线不要使用 `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH`。如果只是为了先看模板热点，可以在用户确认后用该宏做临时 `-ftime-trace` 实验。

### Clang Trace 结果

以下数据使用 standalone `clang-cl 18.1.8`，并定义 `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH` 绕过当前 MSVC STL 对 Clang 19+ 的版本要求。该结果只用于探索编译热点，不作为正式支持基线。

命令形态：

```powershell
clang-cl /std:c++17 /I. /D_CRT_SECURE_NO_WARNINGS /DTHROW_UNKNOWN_KEY `
  /D_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH /EHsc /bigobj /Zc:__cplusplus `
  /utf-8 /O2 /DNDEBUG /clang:-ftime-trace /c <source.cpp>
```

单文件耗时：

| 文件 | 编译耗时 | Trace 大小 |
| --- | ---: | ---: |
| `benchmark/json_benchmark.cpp` | 26.78 s | 35.7 MB |
| `test/test_pb.cpp` | 27.23 s | 6.4 MB |
| `test/test_json_files.cpp` | 30.09 s | 32.6 MB |
| `test/test_yaml.cpp` | 22.85 s | 6.0 MB |
| `test/test_xml.cpp` | 24.86 s | 6.1 MB |

Trace 汇总：

| 文件 | ExecuteCompiler | Frontend | Backend | Optimizer | InstantiateFunction | InstantiateClass |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `json_benchmark.cpp` | 24.42 s | 7.84 s | 16.49 s | 10.41 s | 6.33 s | 3.31 s |
| `test_pb.cpp` | 25.25 s | 7.30 s | 17.86 s | 11.75 s | 5.47 s | 2.42 s |
| `test_json_files.cpp` | 27.96 s | 7.76 s | 20.11 s | 12.31 s | 5.98 s | 3.17 s |
| `test_yaml.cpp` | 20.79 s | 2.86 s | 17.89 s | 10.80 s | 1.50 s | 0.65 s |
| `test_xml.cpp` | 22.82 s | 2.79 s | 19.99 s | 11.66 s | 1.50 s | 0.56 s |

初步结论：

1. Clang 下这几个慢文件也明显受后端优化器影响。`Backend` 和 `Optimizer` 是总耗时中的最大块。
2. JSON 相关路径额外有较重的前端模板实例化成本，尤其是 `githubEvents::event_t` 和 `payload_t`。
3. YAML/XML 的前端模板实例化相对较轻，主要慢在后端优化和代码生成。
4. PB 的前端和后端都重，热点集中在 `to_proto`、`from_pb`、`build_pb_fields`、`tuple_cat`、`pb_field_t` 等路径。

最重函数模板实例化：

| 文件 | 主要热点 |
| --- | --- |
| `json_benchmark.cpp` | `iguana::from_json<std::vector<githubEvents::event_t>>`、`from_json_impl<githubEvents::event_t>`、`from_json_impl<githubEvents::payload_t>` |
| `test_json_files.cpp` | 同样集中在 `githubEvents::event_t` / `payload_t` 的 JSON 解析 |
| `test_pb.cpp` | `iguana::to_proto<vector_t>`、`proto_needs_timestamp_import<vector_t>`、`get_pb_members_tuple<vector_t&>`、`build_pb_fields`、`from_pb<test_pb_merge_outer>` |
| `test_yaml.cpp` | `from_yaml<person_t>`、`from_yaml<store_example_t>`、`from_yaml<test_enum_t>`、`from_yaml<some_type_t>` |
| `test_xml.cpp` | `from_xml<province>`、`from_xml<some_type_t>`、`xml_parse_item<province>`、`xml_parse_item<some_type_t>` |

最重源码入口：

| 文件 | 主要 Source 热点 |
| --- | --- |
| `json_benchmark.cpp` | `benchmark/json_benchmark.h`、`iguana/json_reader.hpp`、`iguana/json_util.hpp`、`iguana/common.hpp` |
| `test_json_files.cpp` | `test/test_headers.h`、`filesystem`、`doctest.h`、`iguana/json_reader.hpp` |
| `test_pb.cpp` | `doctest.h`、`iguana/dynamic.hpp`、`iguana/common.hpp`、`iguana/util.hpp`、`windows.h` |
| `test_yaml.cpp` | `iguana/yaml_reader.hpp`、`doctest.h`、`iguana/yaml_util.hpp`、`iguana/common.hpp` |
| `test_xml.cpp` | `iguana/xml_reader.hpp`、`doctest.h`、`iguana/xml_util.hpp`、`iguana/common.hpp` |

下一步实验：

1. 用 Clang 对这 5 个文件做 `/O2` vs `/Od` 对比，确认后端优化器成本占比。
2. 对 `json_benchmark.cpp` 和 `test_json_files.cpp` 单独研究 `githubEvents` 类型，判断是否可以减少大 tuple / variant / reflection traversal 的实例化成本。
3. 对 `test_pb.cpp` 单独研究 `build_pb_fields` 和 `tuple_cat`，判断是否可以缓存或简化 PB schema 构建路径。

### Clang `/O2` vs `/Od` 对比

同样使用 `_ALLOW_COMPILER_AND_STL_VERSION_MISMATCH`，只作为探索性数据。

| 文件 | `/O2` | `/Od /Ob0` | 降幅 |
| --- | ---: | ---: | ---: |
| `benchmark/json_benchmark.cpp` | 34.93 s | 15.37 s | 56.0% |
| `test/test_pb.cpp` | 38.62 s | 14.68 s | 62.0% |
| `test/test_json_files.cpp` | 37.06 s | 13.76 s | 62.9% |
| `test/test_yaml.cpp` | 29.62 s | 7.81 s | 73.6% |
| `test/test_xml.cpp` | 31.99 s | 8.44 s | 73.6% |

结论：

1. Clang 下也能复现 MSVC 的核心规律：优化器成本是 Release 慢的主要来源。
2. YAML/XML 的前端模板实例化较轻，低优化收益最大。
3. JSON/PB 低优化后仍有 13-15 秒，说明除了优化器，前端模板实例化和大型类型 schema 仍然值得专项优化。

### JSON Schema 结构观察

`json_benchmark.cpp` 和 `test_json_files.cpp` 的热点都集中在 `githubEvents` 类型。代码结构上有一个明显问题：`benchmark/json_benchmark.h` 和 `test/test_headers.h` 各自复制了一整套大型 JSON schema 类型。

| 文件 | 行数 | 说明 |
| --- | ---: | --- |
| `benchmark/json_benchmark.h` | 688 | benchmark 使用，字段多为 `std::string_view` / `iguana::numeric_str` |
| `test/test_headers.h` | 672 | test 使用，字段多为 `std::string` / 数值类型 |

两个文件都包含 `githubEvents::event_t`、`payload_t`、`forkee_t` 等大型反射类型。Trace 中最重的 JSON 实例化集中在：

```text
iguana::from_json<std::vector<githubEvents::event_t>>
iguana::detail::from_json_impl<githubEvents::event_t>
iguana::detail::from_json_impl<githubEvents::payload_t>
iguana::detail::from_json_impl<std::optional<githubEvents::forkee_t>>
```

后续可以考虑的方向：

1. 把测试和 benchmark 的大型 schema 拆成更小的专用头，避免无关测试 include 全量 schema。
2. 对 `githubEvents` 单独建立专用 benchmark/test 文件，避免它拖慢普通 JSON 测试。
3. 分析 `payload_t` 和 `forkee_t` 的字段数、optional、vector、嵌套对象是否触发过多 tuple/variant 组合实例化。
4. 保持 test 和 benchmark 的类型语义差异，不要盲目合并 `test_headers.h` 和 `json_benchmark.h`；它们一个偏 owning type，一个偏 view/numeric_str benchmark type。
