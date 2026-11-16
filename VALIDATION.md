## 2026-11-17：串口参数只传端口名（用户定）

serialParameters 接收 std::string 只读引用，官邸从原枚举表取出 portName，测试直接构造新的 SerialParams。固定规格、枚举展示和参数比较语义保持；无收发、重连或关闭逻辑变化。

Windows/MSVC build-params Release 增量构建（含公开头）与 CTest 2/2 通过（1.31 秒），覆盖页面和 Pelco-D 串口；主程序串口专项 1/1（1.20 秒）通过。日志：build-params/serial-params-{build,test}.log，以及主程序 build/serial-params-pelco-{build,test}.log。Linux 桥接器复用本协议的验证见 BridgeCore/VALIDATION.md，未操作转台实物，不推送。

## 2026-11-15：三字节分块成帧验证

按七字节帧的一半设置三字节上传上限，环形容量为九字节；追加整块后滑动校验窗口，一次最多返回一帧。既有 8192 种分片、假帧头、连续帧、校验恢复、帧头块时间和重连验证改用真实三字节上限。

Windows Release、公开头与 CTest 2/2 通过，主程序串口协议 1/1、Release 构建和 8 秒冒烟通过。Sqaf 配置的 qmllint 目标返回 0，保留两条现有模板设备到 QObject 的静态识别告警。日志为相邻主程序 build/serial-chunk-SqafDevice-*.log、serial-chunk-pelco-*.log、serial-chunk-qmllint.log。未操作硬件、不推送。

## 2026-11-15：单字节 onRX 成帧验证

删除 PelcoDFramer，七字节环形窗口和帧头块时间移入 PelcoDHandle::onRX；结果借用成员连续数组，重新打开前清除旧半帧。保留 8192 种分片、连续帧、坏帧恢复、帧头时间及重连覆盖。

Windows Release、公开头编译和 CTest 2/2 通过；主程序独立串口协议验证 1/1、Release 构建及 8 秒离屏冒烟通过。主程序旧串口测试缓存仍指向迁移前目录，本轮使用新的 build/serial-byte-pelco 配置验证，未改旧缓存。证据：相邻主程序 build/serial-byte-SqafDevice-*.log、serial-byte-pelco-*.log。未操作硬件、不推送。

## 2026-11-14：串口成帧 span 验证

成帧器 feed/deliver 改用只读 span；底层缓冲仍自有，测试留存结果时复制字节。保留 8192 种分片、环形跨界、假帧头、校验失败恢复及帧头块时间验证。Windows Release、公开头及 CTest 2/2 通过。主程序构建和离屏冒烟通过，日志在主程序 build/framerspan-SqafDevice-*.log。未操作硬件、不推送。

## 2026-11-14：串口标准字符串验证

适配 SerialPortInfo 标准字符串；QML 签名不变，仅显示/输入边界进行本地编码转换。Windows 独立 Release、公开头与 CTest 2/2 通过，主程序增量构建和 8 秒离屏冒烟通过。日志：主程序 build/serialstring-SqafDevice-*.log。本轮未操作硬件、不推送。

## 2026-11-14：vector 引用发送验证

测试命令适配 const vector& 发送入口；Windows 独立构建、公开头及 CTest 2/2 通过。证据主程序 build/vectorsend-SqafDevice-{build,test}.log。未操作硬件、不推送。

## 2026-11-14：onData 只读 span 验证

上传/测试替身适配 span；只在现有 Qt 解析或断言边界借用适配。Windows 独立构建、公开头和 CTest 2/2 通过；日志在主程序 build/ondata-SqafDevice-{build,test}.log。仅本地，不推送，无真实硬件操作。

## 2026-11-14：发送裁决所有权与收发 span（用户定）

适配拥有型 TXRuling 与只读 span 收发口。Windows Release 独立构建及 CTest 2/2 通过；原输入寿命、排队所有权和协议字节回归通过。日志：相邻主程序 build/wire-SqafDevice-{build,test}.log。重连、关闭与业务时序保持，仅本地提交，不推送；不涉及实物测试。

# 验证记录

## 2026-11-03：删除串口连接规格说明（用户定）

删除页面的波特率、数据位、停止位、校验和流控说明及对应 C++ 字符串组装接口，只保留端口选择与连接/断开。型号固定构参、协议、发送和关闭行为不变。

通过主程序本地源码覆盖验证：Windows 增量 Release 构建、LidarSqafDevice_qmllint 和 RayshiningDevice_qmllint 均通过，无编译/QML 诊断；主程序八秒离屏存活，仅有既有字体提示。证据位于 LiDAR_DataACQ/build/build-serial-ui.log、smoke-serial-ui.json。主程序当前装配 UDP 激光器，串口页面本轮验证为编译与静态 QML 检查，未做真实串口或 Linux 验证。仅本地提交，未推送。

## 2026-10-28：Pelco-D 改用环形容器（用户定）

- 成帧器使用 `boost::circular_buffer<StampedByte>`，容量 7、初始长度 0。字节和读取时间绑定存放，删除两组数组、实际长度成员与坏帧时的逐元素搬移。
- 每追加一个字节，循环检查最前 7 字节；复用帧头/校验和联合判断，失败只移除一个字节，成功移除整帧并同步交付。首字节所属读取块的 RxTime 保持不变。
- 发送封装、固定地址、RX/TX 裁决、重连与同步关闭未改。使用与激光器相同的 Boost 1.88.0 依赖配置，未修改 HandleCore。
- 原应用的串口专项移入本仓独立测试入口，并补上环绕和重同步案例。Windows 与 Linux 各通过 16411 项断言，包括 8192 种分块、7 个物理起点、噪声、坏头、坏校验、坏候选中的有效帧、数据/校验字节中的 FF、时间戳、千帧连续接收、TX 及 I/O 失败重连后的半帧隔离。
- 两端 CTest 均为 2/2：生产页面创建/销毁与串口专项通过；所有公开头独立编译通过，`LidarSqafDevice_qmllint` 通过。测试使用串口 C API 替身，未打开实物设备，不等于完成硬件验证。

环境与记录：

- Windows：MSVC 19.51、Qt 6.11.2、Release；测试与逐头编译启用 /W4 /WX。初次新增测试因 FF 常量转 char 触发 C4310，改为无符号比较和字节串构造后重新构建、执行通过；最终无编译警告。证据：`build/build-final.log`、`build/test.log`、`build/qmllint.log`。
- Linux：ArchHFNL，GCC 16.2.1、系统 Qt 6.12.0、libserialport 0.1.2、Release；测试与逐头编译启用 -Wall -Wextra -Wpedantic -Werror，最终无编译警告。证据：`~/.cache/sqaf-ring-20261028/build-final.log`、`test-final.log`、`qmllint-final.log`。
- 本轮复用先前已下载并核验的 Boost 1.88.0 源码，其他依赖按固定提交拉取。配置阶段保留环境原有的 Qt 私有 API、可选 Vulkan 头缺失提示；Windows 另有未安装 QtTaskTree 引起的可选 QML 插件提示，未影响配置、构建和页面验证。

本次只在 SqafDevice 本地提交，未推送，未更新主程序固定依赖、simonly 或旧 sim-ui。

## 2026-11-03 回调类型声明整理

Pelco-D 成帧器和测试直接使用 std::function，删除对 DataCallback 的依赖；收发和成帧逻辑保持。Windows/MSVC/Qt 6.11.2 独立 Pelco-D 验证通过 16411 项检查，包含切分、回绕、坏帧同步、时间戳、千帧连续交付及重连；构建日志在主程序 build/virtual-protocol/build-callback-alias.log。Linux 桥接器直接使用本仓协议头，构建及 CTest 4/4 通过（virtual-upload-20261103/build-BridgeCore/callback-alias-{build,test}.log）；未打开真实串口。

## 2026-11-03 清理无效构造继承声明（用户定）

删除 PelcoDHandle 的 using SerialHandle::SerialHandle；基类没有需要继承的自定义构造函数。协议、发送线程、重连及关闭不变。通过主程序相邻源码覆盖完成 Windows/MSVC/Qt 6.11.2 Release 增量构建及八秒离屏冒烟；stderr 仅原有字体提示。HandleCore 本轮 CTest 6/6，包含串口测试。证据在 LiDAR_DataACQ/build/build-serial-cleanup.log、smoke-serial-cleanup.json；本轮未重跑独立 Pelco-D 专项、Linux 或实物验证，不推送。
