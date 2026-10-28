# SqafDevice

2026-10-28 连接显示整理（用户定）：连接区显示波特率、数据位、停止位、校验与流控。显示和连接共用 SqafConfig::serialParameters 构参，不在 QML 硬编码第二份规格；仍仅允许选择端口。

2026-10-28 用户定：连接区只选择串口；`open(serialPortIndex)` 从 SqafConfig 取得固定的 9600 baud、8 数据位、1 停止位、无校验、无流控参数。删除 QML 的线路参数选中项、下拉框及后端对应选项表，端口枚举与下标校验保持；点动速度和预置位等业务设置不变。

本轮 Windows Qt 6.11.2 Release、qmllint 及既有页面冒烟 1/1 通过，无编译/QML 警告；证据为 `build/build-fixed-connection.log`、`build/test-fixed-connection.log`。未连接实物，不重复 Linux/协议全套，仅本地提交。

LiDAR 的独立官邸：具体 C++ 后端、Collector/协议/处理逻辑及其 QML 页面在同一仓库维护。初始代码来自 LiDAR_DataACQ master `5cc6d65`，后续维护记录见 [VALIDATION.md](VALIDATION.md)。

## 边界与接入

- QML 模块：`Lidar.Sqaf`；页面：`SqafModule`。
- CMake 目标：`Lidar::SqafDevice`；静态插件：`LidarSqafDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarSqafDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/TaskCore 可选项，再配置共享库，避免已创建目标后补开模块。
- 需要 libserialport >= 0.1.1；串口权限由运行环境提供。
- Pelco-D 成帧使用 Boost.CircularBuffer。优先复用 `Boost::circular_buffer` 或已安装的 Boost >= 1.88 CMake 包；否则自动拉取 SHA256 固定的 Boost 1.88.0 官方 CMake 归档，只启用 CircularBuffer 及其头文件依赖。没有新增运行时动态库；离线可设置 `FETCHCONTENT_SOURCE_DIR_BOOST` 为同版 CMake 归档解压目录。

## Pelco-D 成帧

每个串口连接独持容量为 7、初始长度为 0 的 `boost::circular_buffer<StampedByte>`，元素同时保存字节和其读取块的 RxTime，避免维护两份平行环形状态。

1. 逐字节追加；缓冲不足 7 字节则等后续输入。
2. 只要长度不少于 7，就检查最前面的 7 字节。复用 `PelcoFrame::kindOf` 一次判断帧头和校验和；任一失败，只移除头部一字节并继续循环。
3. 有效帧移除完整 7 字节后同步交付，时间取该帧头所属读取块。回调不得重入同一成帧器。

循环结束最多保留 6 字节，因此容量 7 足够，不会覆盖未判定的候选帧。容器本身遵守“容量内追加不覆盖，满后才覆盖最旧元素”；移除头部不搬移剩余元素。协议没有固定帧尾标记，第 7 字节是第 2 至 6 字节之和的低 8 位。发送封装、地址 0x01、收包裁决及关闭/重连保持现有行为。

RxTime 是帧头所在串口读取块的软件观测时间，不代表硬件首字节到达时刻，也不改为完成帧所在块的时间。连接重建时自然丢弃旧半帧。

## 构建与验证

C++20、CMake >= 3.28、Qt 6.11.2（Quick/Qml/GuiPrivate/ShaderTools）。Windows 使用 MSVC x64；Linux 使用 GCC，Qt 与原生依赖由环境提供。

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="<Qt前缀;需要的原生依赖前缀>"
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
cmake --build build --config Release --target LidarSqafDevice_qmllint
```

独立测试逐头编译、创建真实页面并销毁后端，并用 HandleCore 自带的串口替身验证生产 Handle 的收发和重连。协议覆盖两帧所有 8192 种分块、7 个环绕起点、垃圾前缀、坏头/坏校验、坏候选内部的有效帧、时间戳、千帧连续交付及重连清除半帧。不连接实物设备；硬件收发仍需对应设备实测。主程序负责跨官邸仿真、任务及取消链回归。

## 许可

MIT，Copyright (c) 2026 周涛（ZHOU TAO）。Qt、libserialport、libpcap/Npcap 等依赖各自授权，MIT 不替代这些许可；详见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。本仓库不包含第三方 SDK、运行库或驱动。

CI：托管 Linux 自动验证；Windows 原生依赖使用预装 SDK/运行时的自托管入口，默认跳过不代表通过。本机 Windows 验证记录与主程序分仓记录一起保存。

本轮本机验证（2026-10-28）：Windows Qt 6.11.2 Release 构建、CMake qmllint 和既有页面冒烟 1/1 通过；未连接实物。日志：build/build-connection-summary.log、build/test-connection-summary.log。
