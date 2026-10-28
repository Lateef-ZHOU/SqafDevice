# 第三方依赖

本仓库源码使用 MIT；所链接依赖不因此重新授权。

- Qt 6：按所用模块的 LGPL/GPL 或商业许可使用和分发；本模块使用 Qt Core/Gui/Quick/Qml 及 ShaderTools 构建工具，私有 GUI API 要求版本对齐。分发 Qt 时应保留许可、通知并满足对应授权条件。
- HandleCore、DeviceCore、TaskCore、UICore、ChartCore、Algorithm：MIT，各自的许可证与第三方声明随所固定提交保留。
- 串口依赖 libserialport：LGPL-3.0-or-later；仅串口官邸需要。动态/静态分发应按依赖许可履行义务。
- Boost.CircularBuffer：使用 Boost Software License 1.0，默认拉取 Boost 1.88.0 官方 CMake 归档并校验 SHA256。只使用头文件；第三方源码及许可证保留在依赖目录，分发源文件时保留相应版权与许可文本。见 [Boost 许可全文](https://www.boost.org/LICENSE_1_0.txt) 和 [容器说明](https://www.boost.org/doc/libs/1_88_0/doc/html/circular_buffer.html)。
- Linux libpcap：BSD 类许可，保留其版权和声明。
- Windows Npcap：SDK 与驱动许可各自适用；免费许可不等于可任意再分发驱动。未取得相应授权时，由使用者从官方安装，不随产品打包。

依赖的完整文本及版本边界以各库固定提交和实际安装版本为准；HandleCore 的 THIRD_PARTY_NOTICES.md 提供串口与抓包依赖链接。
