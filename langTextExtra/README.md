# 第三方源码与已冻结的 v1 工程

这个目录主要存放**内置第三方库源码**：

- `DuiLib/`、`libxlsxwriter/`、`zlib-1.3.1/`、`tinyxml/`

此外还保留了**原版（v1）单体实现**及其 Visual Studio 工程：

- `langTextExtra.sln`、`langTextExtra.vcxproj`、`langTextExtra.vcxproj.filters`
- `langTextExtra.cpp`、`Utils.*`、`dxml.*`、`stringConvert.*`、`UIlib.*`、`StdAfx.*`

> **v1 已冻结，不再维护，也不再修复其中的缺陷。**
> 所有已知问题的修复都在仓库根目录下的 `core/`、`cli/`、`gui/` 中实现，
> 请使用根目录的 `CMakeLists.txt` 构建。打开旧 `.sln` 只会得到旧行为。

这些文件保留下来只是为了历史对照和第三方依赖。v1 工程本次仅做了可移植性
修正（移除开发者本机绝对路径、四种配置统一），没有行为变更。

第三方库的许可证说明见仓库根目录的 `THIRD_PARTY_NOTICES.md`。
