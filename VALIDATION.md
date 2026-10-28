# 验证记录

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
