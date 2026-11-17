## 2026-11-17：构造链接收 QObject 父对象（用户定）

PelcoDHandle 继承 SerialHandle 构造函数，接收 Device 原样传入的 parent，最终由 QThread/QObject 建立父子关系。默认独立构造、成帧、发送与关闭链保持，不添加转发函数或成员。验证见主程序维护操作，暂不推送。

## 2026-11-17：命令集合使用 vector（用户定）

startOf/stopOf 与 Collector 标签统一返回 std::vector<std::vector<unsigned char>>，官邸直接交 Device::send，删除原批次转换。原单条编码器保持；定位仍先水平后俯仰两包，点动和停机仍单包，字节、顺序与取消行为不变。

不新增命令缓存或兼容包装。验证见主程序 docs/maintenance/validation.md；使用相邻源码覆盖，暂不推送。

## 2026-11-17：固定波特率使用整数（用户定）

SqafConfig::baudRate 改为 constexpr int，值仍为 9600；serialParameters 直接使用该常量，删除 BaudRate 枚举依赖及转 int 的中转。数据位、停止位和校验配置不变，仍为 9600/8N1；文字输出、发送、重连和关闭保持。验证见 VALIDATION.md，暂不推送。

## 2026-11-17：串口构参只接收端口名（用户定）

SqafConfig::serialParameters 接收 const std::string& portName，直接构造只含实际连接字段的 SerialParams。官邸仍按原选项表验证下标，再提取端口名；SerialPortInfo 的枚举、描述与身份比较不变。固定 9600/8N1、无流控及发送、重连、关闭保持。验证见 VALIDATION.md，未推送。

## 2026-11-17：onTX 接入 vector 只读引用（用户定）

onTX 输入统一为 const std::vector<unsigned char>&，同步传参不复制调用方数组；RX 仍用只读 span。Pelco-D 的命令尺寸检查与编码保持。 不新增成员状态；使用相邻本地 HandleCore 源码覆盖，远端锁定版本暂不变，不推送。Windows build-params Release 与 CTest 2/2（页面、串口）通过。 证据见主程序 docs/maintenance/validation.md 本轮记录。

## 2026-11-15：按半个最小帧长分块上传（用户定）

SerialHandle<MaxRxBytes> 的编译期参数只表示上传上限，不恢复 Framer 类型或回调对象。各协议按最小帧长的一半向下取整：激光器 13 → 6 字节，Pelco-D 7 → 3 字节；温控成帧层接受非空主体加 CR，最短 2 → 1 字节，业务报文合法性仍由上层检查。SerialConn 仍最多一次读取 256 字节，然后顺序切为不超过上限的非空 span 同步上传；短块立即交付，不等凑满，同一读取块共用原 RxTime。

每次 onRX 至多返回一帧。具体 Handle 持有接收状态和连续结果，结果只在同步派发期间借用，异步使用须取得所有权；每次 connOpen 清空旧半帧。激光器和 Pelco-D 先追加整块，再查找第一份有效帧；环形容量为最大帧长加上传上限减一，避免追加末块时覆盖尚未校验的头部。激光/温控使用触发交付块的时刻，Pelco-D 保留帧头块时刻。TX、温控 75 ms 节奏、重连和关闭链不变。

设计权衡：分块减少逐字节的回调和虚函数调用次数，并不是“永远只有一帧”的证明。变长协议的错误长度可能阻塞一个长候选，候选失败后重新找头会一次发现多个既有完整帧；单字节同样可能积压，只是后续同量输入提供更多交付机会。这里接受分块恢复更慢：剩余完整帧严格等待后续非空输入，没有新输入就保持到重连清空，不加空输入排空或定时补发。一次片段缺失或长度字段损坏也可能触发，不能据此假设整体丢包率很高。详细示例及验证见 HandleCore/docs/reference.md 和各库验证记录。下方带日期的旧方案仅供历史追溯。

## 2026-11-14：成帧器同步借用字节

feed 收只读 span，完整帧经 deliver 只读 span 同步交出；半帧和完整帧底层缓冲仍由成帧器拥有。校验、噪声重同步、帧时间及重连清理行为保持；异步使用必须取得所有权，不保存 span。本轮不推送。

## 2026-11-14：串口标准容器验证

Windows Release、公开头与 CTest 2/2 通过；端口缓存改用 vector，判空及负数/越界下标处理保持，移除本头无用 QList 包含。主程序增量构建及 8 秒离屏冒烟通过，证据主程序 build/serialvector-SqafDevice-*.log。未操作硬件、不推送。

## 2026-11-14：串口缓存使用标准容器

端口缓存改用 const std::vector<SerialPortInfo>，保持首次枚举一次、同序展示和按下标连接。使用 empty() 和有符号下标校验，删除本头已无用户的 QList 包含。Qt 文本仅在既有界面边界转换；连接行为不变、不推送。

## 2026-11-14：串口字符串边界

SerialPortInfo 的名称与描述改用 std::string，保留 libserialport 原始编码。仅在 QML/命令行边界转换 Qt 字符串，线路规格文本转换返回标准字符串。协议、连接参数及关闭行为不变。本轮不推送。

## 2026-11-14：Handle 收发边界

Pelco-D 的 onRX/onTX 改收只读 span，发送封装后将完整七字节报文交给拥有存储的 TXRuling。成帧器和协议字节保持，输入只同步借用。 发送顺序、重连和关闭不变；本轮不部署硬件、不推送。统一契约见 HandleCore/docs/reference.md。

## 2026-11-14：适配只读发送视图

串口协议测试适配 Handle 的只读 span 单条发送入口，临时 QByteArray 只在完整发送表达式内借用；生产协议、命令表发送和关闭行为不变。

Windows build-params 构建及页面/串口测试 2/2 通过；日志 sendspan-{build,test}.log。 本轮不推送。

## 2026-11-14：UTF-8 日志接口（用户定）

串口 Handle 的 logMessage 改为 UTF-8 std::string；长度、行程和预置位动态日志使用 std::format，保留角度两位小数。QML 使用 UICore 的公共转换，协议、发送、重连和关闭不变。

需要支持 C++20 std::format 的标准库。本轮仅本地修改与验证，不推送。

验证：SqafDevice_serial 1/1 通过（build-params/logstring-*.log）；主程序集成构建和 8 秒离屏冒烟通过。

## 历史：2026-11-14 TXRuling 只读 span（已被上方拥有型裁决取代）

Pelco-D 在 send 的局部缓冲中保存封装报文，再返回只读 span；调用方命令保持。 重连和关闭保持，与本地 HandleCore 同批使用，暂不推送。

Windows build-params 的 SqafDevice_serial 1/1 通过；Linux Bridge 转发通过。 Windows 日志为上述构建目录的 txspan-{build,test}.log。

## 2026-11-14：只读 span 接收裁决（用户定）

2026-11-14 接收裁决适配：onRX 返回 `std::span<const unsigned char>` 借用接收数据，onData 同步借用只读 span，现有 Qt 解析器在消费处零拷贝适配。留存或异步投递前必须复制取得字节所有权，不能仅浅拷贝视图。协议、重连和关闭逻辑不变；与本地 HandleCore 同批使用，尚未推送。

Windows/MSVC：build-params 的 SqafDevice_serial 增量构建及专项 1/1 通过。验证日志为相应构建目录下 rxspan-build.log / rxspan-test.log（Linux 使用上述命名）。

## 2026-11-14：接入 FamilyCore（用户定）

依赖声明与开关改用 FamilyCore / FAMILYCORE_WITH_*，协程头由 DeviceCore 提供。当前仅本地整理，GitHub 仍为 TaskCore，旧固定提交暂不更新；联调使用 `FETCHCONTENT_SOURCE_DIR_LIDAR_FAMILYCORE` 指向相邻 FamilyCore，并覆盖配套 DeviceCore/Algorithm 等源码。Windows 主程序或 simonly 的实际消费目标构建通过，业务与数据格式不变；未推送、未更新交付包。

# SqafDevice

2026-11-09 用户定：官邸提供 `paramsFor(target, hold, speeds)`（型号校验与量化，任务预验、造斗与直发共用），makeMoveCollector 与手动 issueGoTo 都收 `GMBMoveParameters` 并复用该判据；删除 collectorFor 与只读原型，Collector 由 begin 直接造出完整对象。

2026-11-09 用户定：设备与云台族合为一个对象：末端官邸直接继承 `GMBDevice<HandleT, CollectorT>`（它同时继承 `Device<HandleT, CollectorT>` 与纯 C++ 的 `GMBDeviceBase`），删除中间 Core 类与 `device_` 指针；唯一 QObject 来自 Handle。末端声明 displayName/subtitle/isOpen/holder/mounting/slipRing 属性及 lockChanged/mountingChanged/anglesReady 信号，析构首句预析构只走 Device 链。页面 `family` 改为 `QtObject`，`deviceState`/日志直接指向设备本身，删除 `device.device` 自引用。页面测试新增 family 即后端本身、可交叉转换到族基的断言。

2026-11-03 上传接口：官邸仍拥有唯一具体 Core；Core 继承 `Device<HandleT, CollectorT>`，Device 直接继承 Handle 并重写 onData。Core 只提供 Qt 属性、锁信号与末端预析构；不增设上传回调、状态缓存或逐包信号转发。协议、Collector 与 UI 逻辑保持。

2026-11-03 连接显示整理（用户定）：删除串口线路规格说明及后端字符串组装，连接区只保留端口选择和连接/断开。固定线路参数仍由 SqafConfig::serialParameters 构参，不因显示删减而改变。

2026-10-28 用户定：连接区只选择串口；`open(serialPortIndex)` 从 SqafConfig 取得固定的 9600 baud、8 数据位、1 停止位、无校验、无流控参数。删除 QML 的线路参数选中项、下拉框及后端对应选项表，端口枚举与下标校验保持；点动速度和预置位等业务设置不变。

本轮 Windows Qt 6.11.2 Release、qmllint 及既有页面冒烟 1/1 通过，无编译/QML 警告；证据为 `build/build-fixed-connection.log`、`build/test-fixed-connection.log`。未连接实物，不重复 Linux/协议全套，仅本地提交。

LiDAR 的独立官邸：具体 C++ 后端、Collector/协议/处理逻辑及其 QML 页面在同一仓库维护。初始代码来自 LiDAR_DataACQ master `5cc6d65`，后续维护记录见 [VALIDATION.md](VALIDATION.md)。

## 边界与接入

- QML 模块：`Lidar.Sqaf`；页面：`SqafModule`。
- CMake 目标：`Lidar::SqafDevice`；静态插件：`LidarSqafDeviceplugin`，最终可执行文件显式 `Q_IMPORT_QML_PLUGIN(LidarSqafDevicePlugin)`，并导入 TaskUI/UI/Charts 的静态插件。
- 页面以 attached 对象拥有唯一具体后端，后端不能在 QML 直接创建。设备通过只读 `family` 借出族引用；任务页面只接收族列表，不依赖设备官邸。
- 依赖由 `cmake/Dependencies.cmake` 固定到提交；可用标准 FetchContent 源码覆盖做联调。多个官邸并用时，先开启所有需要的 HandleCore/FamilyCore 可选项，再配置共享库，避免已创建目标后补开模块。
- 需要 libserialport >= 0.1.1；串口权限由运行环境提供。
- Pelco-D 成帧使用 Boost.CircularBuffer。优先复用 `Boost::circular_buffer` 或已安装的 Boost >= 1.88 CMake 包；否则自动拉取 SHA256 固定的 Boost 1.88.0 官方 CMake 归档，只启用 CircularBuffer 及其头文件依赖。没有新增运行时动态库；离线可设置 `FETCHCONTENT_SOURCE_DIR_BOOST` 为同版 CMake 归档解压目录。

## Pelco-D 成帧

PelcoDHandle 独持容量为 9、初始长度为 0 的 `boost::circular_buffer<StampedByte>`，元素同时保存字节和其读取块的 RxTime，避免维护两份平行环形状态。

1. 每块追加最多 3 字节；9 字节容量容纳旧的 6 字节尾部及新块，缓冲不足 7 字节则等后续输入。
2. 只要长度不少于 7，就检查最前面的 7 字节。复用 `PelcoFrame::kindOf` 一次判断帧头和校验和；任一失败，只移除头部一字节继续检查，直到有效帧或不足 7 字节。
3. 有效帧复制到成员中的连续七字节数组，移除环中完整 7 字节后返回 Decode，时间取该帧头所属读取块；不得重入 onRX。

本次处理结束最多保留 6 字节，下次追加至多 3 字节，因此容量 9 足够，不会覆盖未判定的候选帧。容器本身遵守“容量内追加不覆盖，满后才覆盖最旧元素”；移除头部不搬移剩余元素。协议没有固定帧尾标记，第 7 字节是第 2 至 6 字节之和的低 8 位。发送封装、地址 0x01、收包裁决及关闭/重连保持现有行为。

RxTime 是帧头所在串口读取块的软件观测时间，不代表硬件首字节到达时刻，也不改为完成帧所在块的时间。每次 connOpen 在打开前清空旧半帧。

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

## 资源阶段通知（2026-11-02）

连接采用 HandleCore 的 `connectionChanged(ResourcePhase)`；主动关闭前在所属线程同步发 `AboutToRelease`，完成事件用于回读现状。页面借出已有 `deviceState` 机制对象，不再保存 isOpen/isLocked/holder 的 QML 转交属性；标签通过连接/锁完成阶段回读，不恢复通用 statusChanged。固定依赖已同步；本轮提交尚未推送，验证使用显式本地源码覆盖。


2026-11-03 验证：Windows Qt 6.11.2 主程序 Release 编译、15 组 QML 检查、跨官邸生命周期和三任务回归 7/7 通过。接口联调使用相邻源码覆盖，新依赖固定为本地提交，尚未推送；未连接实物设备或更新板卡。

## 源码目录（2026-11-03）

公开文件直接放在 `include/`，引用时使用文件名，不再携带原单仓库的业务目录前缀。CMake 目标名称、C++ 类型和 QML 模块名保持不变；使用方须同步更新头文件路径及固定依赖版本。

目录整理验证（2026-11-03）：Windows：公开头独立编译、页面与 Pelco-D 测试 2/2 通过；Linux：BridgeCore 引用新路径构建、CTest 4/4 通过。本轮通过本地源码覆盖验证，固定依赖同步更新到本地提交；尚未推送，不代表已验证远端拉取。

## 2026-11-04：设备日志直接接线（用户定）

删除具体设备的 logSource 自引用属性，QML 直接监听现有设备对象的 logMessage/connectionChanged；设备失败日志直接调用继承自 Handle 的信号。协议、收发、重连和关闭顺序不变。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

## 2026-11-04：设备身份与占用查询（用户定）

设备名称/副标题由构造传入 DeviceBase 的公开 const 成员，Qt 通过 MEMBER CONSTANT 直接暴露；占用查询只保留 holder，Qt 用无效 QVariant 表示空闲、字符串表示占用，删除 isLocked 属性。既有连接、重连、收发和取消逻辑保持。

本轮仅本地整理；集成验证记录见主程序 docs/maintenance/validation.md，不推送。


## 2026-11-04：Handle 连接入口统一（用户定）

串口协议验证同步使用 Handle.open/close 连接入口；Pelco-D 成帧、发送和关闭行为不变。

Windows 主程序增量构建和八秒离屏冒烟通过；本仓测试调用已同步，本轮未单独重跑本仓全套测试，不含硬件或 Linux/ARM 验证。 依赖固定到相邻本地提交，尚未推送，联调使用源码覆盖。


2026-11-05 设备身份纠正（用户定）：该设备为亚安（YAAN）YS3081；界面主标题为“转台·亚安”、副标题为“YS3081”。Sqaf 的仓库名、类型名及原协议保持；旧记录中的“视强安防”不再作为厂家归属。用户可见的云台称呼统一为转台。

2026-11-05 表单统一（用户定）：预置位、自检和角度操作接入 DoubleButtonRow，九宫格方向键的尺寸和按下/松开行为保持。 公共视觉规格由 UICore 提供，不增加业务状态。Windows 主程序增量构建、八秒离屏冒烟通过；固定依赖在本地更新，暂不推送。


### 2026-11-05 公共视觉与连接布局

串口连接区使用 UICore ConnectionSection，小标题使用 Subheading；保留方向键、原有组间距与型号连接动作。 本轮仅本地整理，未推送；Windows 集成验证记录见主程序维护操作。

2026-11-05 输入宽度整理：预置位输入删除局部 preferredWidth=80，由 FieldRow 分配空间。方向键、特殊布局和既定组间距、命令与校验保持。同步 FamilyCore 本地依赖；主程序 Windows 增量构建及八秒冒烟通过，未推送。


### 2026-11-05 连接间距、累积默认值与径向风页面整理

串口连接区删除 actionTopMargin=4，选择行与按钮行使用 ConnectionSection 的统一间距。其他布局和方向键保持。

主程序 Windows Release 增量构建、八秒桌面冒烟及四窗口装配检查通过；运行 stderr 为空，保留既有 CDL5/Qt 生成代码 C4702 编译警告。仅本地验证，未连接硬件、未推送。


### 2026-11-05 按钮气泡与布局复用

预置位、自检、回传模式与角度查询的四排 DoubleButtonRow 直接交给 Section 排列，删除多余 ColumnLayout。方向键、按钮动作及既定组间距保持。

主程序Windows Release增量构建、八秒桌面冒烟与四窗口装配检查通过，运行stderr为空。未连接硬件，仅本地提交、未推送。
