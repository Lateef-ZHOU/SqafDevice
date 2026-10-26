#pragma once
#include <chrono>

#include <SerialPort.h> // 串口档位三枚举 + sp_parity

// ============================================================
// SqafConfig — sqaf 云台常量店（编译期常量；型号夹宪法，对齐 HfnlConfig）：夹内皆可读、
// 夹外不读（界面要的量由官邸按创建参数转交）。店持数值事实，词典（SqafVocab / PelcoDProtocol）
// 只留词汇与成帧内核；协议定死的字节事实就地注明出处。
// ============================================================
namespace SqafConfig {
    // ── 串口默认档位（QML 官邸初始选中的下拉项；强类型词汇：三枚举 + sp_parity）──
    static constexpr BaudRate baudRate = BaudRate::B9600;
    static constexpr DataBits dataBits = DataBits::D8;
    static constexpr StopBits stopBits = StopBits::One; // libserialport 只收 1/2（无 1.5）
    static constexpr sp_parity parity = SP_PARITY_NONE;

    // ── 预置位 / 速度（范围是 Pelco-D 数据字节的协议事实）──────
    static constexpr int presetDefault = 1; // 预置位编号默认
    static constexpr int presetMin = 1;
    static constexpr int presetMax = 255;   // data 字节上限 0xFF（协议定死）
    static constexpr int speedDefault = 63; // 速度默认 = 协议上限（全速）
    static constexpr int speedMin = 1;
    static constexpr int speedMax = 63;     // Pelco-D 速度字节上限 0x3F（协议定死）

    // ── 轴作动范围（°）：限位真身在电机，此处是"这台云台"的数值事实──
    // 消费者：官邸交入 QML 模板的输入范围，以及 SqafVocab::withinTravel 的行程检查（超出即不造斗 / 不发）。
    //  PanTiltModel 的范围是双轴云台的全部位形（水平一周 × 俯仰 ±180°），不冒充设备行程——两者本就不是一回事。）
    // 俯仰 0°~90° **有实测依据**（用户用方向键连动试过）：转到 0° 与 90° 都会自己停下，像是硬限位。
    // 协议文档在仓库 docs/sqaf/（《PelcoD云台指令》《增强版3081说明书》）。编码能表示整圈（±180°，0.01°），行程 0~90
    // 以实测为准：说明书的 ±45° 是同一段 90° 行程、原点摆在中间（原点由 107 号预置位定）。角按本机定义：0 = 平视、抬头为正。
    static constexpr double panMin = 0.0;   // 本型号行程：命令校验与 QML 创建参数共用
    static constexpr double panMax = 360.0;
    static constexpr double tiltMin = 0.0;  // 俯仰轴
    static constexpr double tiltMax = 90.0;

    // ── 任务期常量（官邸造斗时填）──────────────────────────
    // 角度判差阈值（°）：造斗时交给族级收集器，"多近算进了容差圈"。TODO: 待调（接机）
    static constexpr double angleTol = 0.05;
    // 单次动作的看门狗时限；到位驻留时长由任务传入 GMBCollector，不在型号中另设。
    // TODO[接机]: 核对实际行程时间与此时限。
    static constexpr int expectedDurationMs = 5000; // 超时时限：单次云台动作预期时长上限

    // 上行回显间隔：**协议事实**。本台约每秒两次（用户记）。角度图的环长由它与下面的窗宽定。
    // 物理上限也支持这个量级：9600 baud 8N1 = 960 B/s，Pelco-D 帧定长 7 B ⇒ 一帧最快 7.3 ms，
    // 本台远没跑到那儿——500 ms 是设备自己的回传节奏，不是链路限制。
    static constexpr auto uplinkInterval = std::chrono::milliseconds{500};
    // 角度图看多长：显示决定（用户定）。环长 = 窗 ÷ 回显间隔，在图里算，这儿不抄。
    static constexpr auto angleChartWindow = std::chrono::minutes{2};
    static constexpr auto angleHistoryWindow = std::chrono::hours{1}; // 覆盖虚拟卡最长累积窗，显示窗不变。
} // namespace SqafConfig
