# langTextExtra

从一组 DuiLib 的 `style*.xml` 皮肤语言文件中提取界面文案，按**稳定的节点 key** 对齐各语言，导出为 CSV / XLSX 翻译对照表，并输出结构化的缺失项报告。

v2 将原来写死在窗口类里的 433 行单体实现，拆分为可独立测试的纯 C++ `core`、跨平台命令行 `cli`、Windows DuiLib 界面 `gui` 三部分，并补齐了配置、样例、单元测试、CI 与文档。

> 本仓库原先没有 README / LICENSE / `.gitignore` / 测试 / CI / 输入样例，且把大量编译产物提交进了 Git。本次改造已全部补齐。

---

## 快速开始

```bash
# 1. 构建（跨平台；默认生成 CSV，不依赖 MSVC）
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 2. 跑测试
ctest --test-dir build --output-on-failure

# 3. 用仓库自带的样例数据试跑
./build/cli/langtext --input samples --output build/translations.csv \
                     --report build/report.json
```

`samples/` 自带三语言配置和一份故意缺失译文的数据，运行后应看到：

```
groups=2 files=5 entries=7 missing=4 orphan=0 duplicate=0 skipped=2 errors=0
```

---

## 修复了什么

v1 的核心问题是**按文本出现顺序（行号）对齐译文**。只要某个语言文件多一条或少一条文本，后面所有行都会整体错位，导出的表看起来"有数据"但全是错的。v2 改为按 XML 节点路径生成稳定 key 对齐。

| # | v1 的问题 | v2 的处理 |
| --- | --- | --- |
| 1 | 按行号/顺序对齐译文，增删一条即整体错位 | 由元素路径 + `name`/`id`/`key` 属性 + 同名兄弟序号生成稳定 key；以参考语言决定行序 |
| 2 | `currentRow += langText.size()` 使用"最后一个语言文件"的条数推进行号 | 行号统一由 `keys.size()` 推导，与任何单个文件无关 |
| 3 | 在 `recursive_directory_iterator` 循环内对每个子目录再次递归 `getFiles()`，导致重复扫描 | 单次 `recursive_directory_iterator` 遍历，文件夹内文件排序后一次性处理 |
| 4 | 语言文件名用子串匹配，`style_en.xml.bak` 会被当成英文文件 | 文件名**精确、大小写不敏感**匹配；备份/临时文件被忽略 |
| 5 | `.vcxproj` 写死 `C:\Users\dnake\Desktop\...`；只有 `Debug\|Win32` 有 include/lib 配置 | 全部改为 `$(ProjectDir)`/`$(SolutionDir)` 相对路径，四种配置一致；新增可移植的 CMake 构建 |
| 6 | `OnFinalMessage() { delete this; }` 与 `_tWinMain` 中的 `delete pFrame` 造成重复释放 | 窗口对象改为栈上生命周期，`OnFinalMessage` 只转发给基类 |
| 7 | `Attribute("Value")` 可能为 `nullptr` 却被塞进 `std::string` | 所有属性读取先判空 |
| 8 | `OPENFILENAME::nMaxFile = sizeof(szFile)` 把字节数当成宽字符数 | 改为 `_countof(file)` |
| 9 | `SHBrowseForFolder` 返回的 PIDL 未释放 | `CoTaskMemFree` 正确释放 |
| 10 | 用户取消保存对话框后仍继续 `workbook_new("")` | 取消即中止，输出路径与扩展名做校验 |
| 11 | `isSkip()` 把长度 ≥ 32 的文本直接丢弃，单字、中文、阿拉伯文也被误杀 | 改为可配置过滤规则：只跳过空白、纯数字/占位符/标点；长文本和所有语言字符一律保留 |
| 12 | TinyXML 以 `TIXML_ENCODING_LEGACY` 解析 UTF-8 XML，数字实体可能解码错误 | 统一 `TIXML_ENCODING_UTF8`，并覆盖十六进制与十进制实体 |
| 13 | GBK / UTF-8 / ANSI 混用，中文路径在非 UTF-8 环境下崩溃 | 内部全 UTF-8；Win32 边界统一 `CP_UTF8` 转换；Windows 下用 `CommandLineToArgvW` 读取原生 UTF-16 命令行 |
| 14 | 缺失译文静默填空、孤立文本无提示 | 显式记录 `missing` / `orphan` / `duplicate` / `skipped` / `parse_error` / `io_error` 六类问题 |
| 15 | 只能输出 XLSX，写失败无反馈 | CSV / XLSX / JSON 三种输出，全部有明确错误返回 |
| 16 | 语言列表、标签、过滤规则、分组标题全部硬编码 | 全部移到 `settings.xml` / `config.xml` |
| 17 | 无 README / LICENSE / `.gitignore` / 测试 / CI / 样例；288 个编译产物被提交 | 全部补齐，并清理 Git 跟踪的编译产物 |

---

## 新增功能

- **稳定 key 对齐**：`/root/Panel#main/Button#ok/text#0` 形式的节点路径，重复标签自动加下标，属性优先。
- **多输出格式**：带 BOM 的 CSV（Excel 直接打开不乱码）、XLSX（分组行合并、表头冻结、缺失项红色标记、独立 `issues` 工作表）、JSON 问题报告。
- **后台线程 GUI**：进度条、状态文本、可取消；UI 线程只通过 `PostMessageW` 接收消息，不再卡死。
- **CLI / CI 支持**：`--fail-on-missing` 在存在缺失译文时返回退出码 `2`，可直接作为流水线门禁。
- **非 ASCII 路径**：中文/阿拉伯文输入目录、输出文件名、XLSX 临时路径均可正常工作。
- **配置驱动**：增删语言只需在 `settings.xml` 加一行，无需重新编译。

---

## 目录结构

```
CMakeLists.txt                 根构建（选项见下）
core/                          纯 C++17 核心：模型、配置、XML、扫描、过滤、抽取、写出
  include/langtext/*.h         公开头文件（不依赖 Win32，可在 Linux 编译）
  src/*.cpp
cli/main.cpp                   命令行入口（Windows 下读取 UTF-16 命令行）
gui/                           Windows DuiLib 界面：MainWindow / WinUtils / 入口
tests/                         无第三方依赖的单元测试 + fixtures
samples/                       可直接运行的三语言输入样例（含缺失译文）
settings.xml                   默认语言/标签/过滤规则配置（16 种语言）
config.xml                     默认「目录路径 -> 分组标题」映射
out/skin/duilib.xml            新版界面皮肤
langTextExtra/                 第三方库源码 + 已冻结的 v1 Visual Studio 工程
.github/workflows/ci.yml       Linux / Windows 构建与冒烟测试
LICENSE / THIRD_PARTY_NOTICES.md
```

---

## 构建

| 选项 | 默认 | 说明 |
| --- | --- | --- |
| `LANGTEXT_WITH_XLSX` | `OFF` | 编译内置的 zlib + libxlsxwriter，启用 `.xlsx` 输出 |
| `LANGTEXT_BUILD_TESTS` | `ON` | 构建单元测试并注册到 CTest |
| `LANGTEXT_BUILD_GUI` | `ON` | 在 Windows 上构建 DuiLib 界面（仅 MSVC） |

```bash
# 完整构建（Windows + MSVC，含界面和 XLSX）
cmake -S . -B build -DLANGTEXT_WITH_XLSX=ON
cmake --build build --config Release --parallel

# 只要命令行 / 跑 CI（Linux、macOS、MinGW 均可，界面目标会被自动跳过）
cmake -S . -B build -DLANGTEXT_BUILD_GUI=OFF -DLANGTEXT_WITH_XLSX=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

界面目标 `langtext_gui` 会把 `settings.xml`、`config.xml` 和 `out/skin/` 自动复制到 exe 所在目录，因为 GUI 是按 exe 路径而不是工作目录定位资源的。

**未编译 XLSX 支持时**，输出 `.xlsx` 会返回明确错误：`XLSX support was not compiled in...`，不会被静默当成 CSV。

> DuiLib 是 GBK 编码且依赖 MSVC 扩展的第三方代码，CMake 把它单独放进 `duilib_vendor` 目标，不施加 `/utf-8`；本项目自己的 UTF-8 源码才使用 `/utf-8`。因此 GUI 目标需要 MSVC，而 `core` / `cli` / `tests` 是标准 C++17。

---

## 命令行用法

```
langtext --input DIR --output FILE [options]
```

| 参数 | 说明 |
| --- | --- |
| `--input DIR` | 包含 `style*.xml` 的目录树（必需） |
| `--output FILE` | 输出表；`.csv` 或 `.xlsx`（需 XLSX 支持），必需 |
| `--settings FILE` | 语言/规则配置，默认自动查找 `settings.xml` |
| `--config FILE` | 分组标题映射，默认取 `settings.xml` 中的 `<groupMap file="..."/>` |
| `--report FILE` | 额外输出机器可读的 JSON 问题报告 |
| `--no-key-column` | 隐藏首列 key |
| `--no-missing-marker` | 缺失单元格留空，而不是写 `[MISSING]` |
| `--fail-on-missing` | 存在缺失/孤立译文时返回退出码 `2`（CI 门禁） |
| `--quiet` | 不打印进度 |
| `--help` | 帮助 |

退出码：`0` 成功，`1` 参数/配置/写文件/解析错误，`2` 有缺失译文且使用了 `--fail-on-missing`。

配置文件查找顺序：显式参数 → 输入目录及其最多 3 级父目录 → 当前工作目录 → exe 所在目录。因此把 `settings.xml` 放在数据根目录即可，无需每次传参（`samples/settings.xml` 就是这么工作的）。

```bash
# 中文输出路径
langtext --input "D:\项目\皮肤" --output "D:\项目\翻译表.xlsx" --report report.json

# CI 门禁
langtext --input ./skins --output out.csv --fail-on-missing --quiet || exit 1
```

---

## 界面用法

运行 `langTextExtra.exe`（CMake 构建产物名）：

1. 点「导入」选择语言文件根目录；
2. 点「开始」，在保存对话框中选择 CSV 或 XLSX 输出路径；
3. 进度条与状态栏实时更新，处理中可点「取消」中止；
4. 完成后结果列表显示各分组、条目数与问题明细。

窗口尺寸、控件（状态标签、进度条、取消按钮）定义在 `out/skin/duilib.xml`。

---

## 配置

### `settings.xml`

```xml
<settings version="2">
  <languages>
    <language file="style.xml"     code="zh-CN" name="简体中文" reference="1"/>
    <language file="style_en.xml"  code="en"    name="English"/>
    <language file="style_ar.xml"  code="ar"    name="Arabic"/>
  </languages>
  <extract>
    <textTags><tag>text</tag></textTags>          <!-- 其内文本全部提取 -->
    <specialTags><tag>panel</tag></specialTags>  <!-- 自身直接文本也提取 -->
    <keyAttributes><attr>name</attr><attr>id</attr><attr>key</attr></keyAttributes>
    <filter skipBlank="1" skipPureNumeric="1">
      <skipExact>%s</skipExact><skipExact>{0}</skipExact>
    </filter>
  </extract>
  <groupMap file="config.xml"/>
</settings>
```

- `file` 与文件名精确匹配（大小写不敏感），子串不再匹配；`reference="1"` 的语言决定表格行序，必须且只能有一个。
- 找不到配置文件时使用内置默认值（16 种语言）。
- 自定义语言时，建议像 `samples/settings.xml` 一样把配置放进数据目录，避免默认 16 种语言导致大量 `missing`。

### `config.xml`

```xml
<root>
  <group path="main"        title="Main screen"/>
  <group path="misc/alert"  title="Alert dialogs"/>
</root>
```

路径以 `/` 分隔、相对于输入目录。旧版嵌套写法（`<misc><alert><login>…</login></alert></misc>`）仍然兼容。未配置的目录使用其相对路径作为标题。

---

## 对齐与问题报告

key 由 XML 元素路径拼接而成，例如：

```
/root/Panel#main/Button#ok/text#0
```

生成规则：元素标签名 + 第一个非空的 `name`/`id`/`key` 属性值；无属性且存在同名兄弟时补 `[序号]`；文本节点在容器路径后补 `#文本序号`。

以参考语言（`reference="1"`）的 key 序列作为表的行序，其余语言按 key 查表填入：

| 情况 | 表现 |
| --- | --- |
| 参考语言有、某语言没有 | 单元格写 `[MISSING]`，记 `missing_translation` |
| 某语言有、参考语言没有 | 记 `orphan_text`，不新增行（避免表结构抖动） |
| 同一文件内 key 重复 | 记 `duplicate_key` |
| XML 解析失败 / 读写失败 | 记 `parse_error` / `io_error`，退出码 1 |
| 命中过滤规则 | 记 `skipped` 并注明原文与行号 |

JSON 报告示例（统计 + 逐条问题，含来源文件与行号）：

```json
{
  "stats": { "groups": 2, "entries": 7, "missing": 4, "orphan": 0,
             "duplicate": 0, "skipped": 2, "errors": 0 },
  "languages": ["zh-CN", "en", "ar"],
  "issues": [
    {"kind": "skipped", "group": "main", "language": "zh-CN",
     "key": "/root/Panel#main/Label#version/text#0",
     "detail": "non-translatable text skipped: 1.2.3",
     "source": "main/style.xml", "line": 8}
  ]
}
```

---

## 测试与 CI

`tests/` 是一个不依赖第三方测试框架的可执行程序，覆盖：key 稳定性、增删条目不错位、备份文件不被误匹配、UTF-8 十六进制/十进制实体、过滤规则、缺失/孤立/重复报告、CSV/JSON 转义等（75 项断言）。

`.github/workflows/ci.yml` 包含：

- Linux / GCC，XLSX `OFF` 与 `ON` 两档，构建 + CTest + CSV/JSON/XLSX 内容断言 + `--fail-on-missing` 退出码断言；
- Windows / MSVC，构建 `core` + `cli`，CTest，以及中文输出路径冒烟测试（守护 UTF-16 命令行处理）。

---

## 样例数据

```
samples/settings.xml        三语言配置（zh-CN 参考 / en / ar）
samples/config.xml          分组标题
samples/main/style*.xml     中文 / 英文 / 阿拉伯文，含长句、XML 实体、可跳过文本
samples/misc/alert/         故意省略部分译文，用于演示 missing 报告
```

`samples/main/style.xml` 里特意放了一句超过 32 字的中文，用来回归验证 v1「长文本被丢弃」的缺陷；`style_en.xml.bak` 类的备份文件不会进入结果。

---

## Legacy v1

`langTextExtra/` 目录里保留了第三方库源码，以及**已冻结、不再维护**的原版 Visual Studio 工程（`langTextExtra.sln` / `langTextExtra.vcxproj` / `langTextExtra.cpp` 等）。这些文件只是为了保留历史与方便对照，其中的旧缺陷**没有**在 v1 里修补 —— 修复都体现在新的 `core/` / `cli/` / `gui/` 实现中。

请**不要**用旧 `.sln` 作为日常构建入口；统一使用根目录的 `CMakeLists.txt`。旧工程本次只做了可移植性修正（移除开发者本机绝对路径、补齐四种配置），不含行为变更。

---

## 第三方依赖与许可证

内置源码：`langTextExtra/DuiLib`、`langTextExtra/libxlsxwriter`、`langTextExtra/zlib-1.3.1`、`langTextExtra/tinyxml`。详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。

本项目自有代码采用 [MIT](LICENSE)。两处第三方 CMake 文件的本地修改仅涉及路径与 `cmake_minimum_required` 版本，未改动任何第三方头文件或实现。

---

## 已知限制

- `langtext_gui` 只能在 Windows + MSVC 下构建（DuiLib 为 GBK 且使用 MSVC 扩展）；`core` / `cli` / `tests` 是标准 C++17，可跨平台。
- 本次开发环境的工具链为 Zig C++ / CMake + Ninja，Windows GUI 目标只做了严格 C++17 的语法检查和桩编译，未在真实 MSVC + DuiLib 上完成链接验证；CI 中的 Windows 作业覆盖 `core` + `cli`，不覆盖 GUI。
- key 依赖 XML 结构：如果两个语言文件的同一控件标签层级不同，会被识别为不同 key 并报 `missing`/`orphan`，这正是需要人工检查的情况。
- 输出表按参考语言的行序生成；孤立文本只出现在问题报告中，不会插入表格行。
