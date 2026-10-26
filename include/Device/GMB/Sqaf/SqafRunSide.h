#pragma once
#include "Device/GMB/GMBCollector.h"
#include <cmath> // lround（角→码）
#include <QByteArray>
#include <QList>
#include <QtGlobal>

#include "Device/Handle/PelcoDProtocol.h" // PelcoFrame（帧定长 / 命令词长契约）
#include "Device/GMB/Sqaf/SqafConfig.h"        // 型号夹宪法（作动范围 / 速度上限直读）

// ============================================================
// SqafRunSide.h — sqaf 云台型号夹的**运行侧**：收发词汇典 + 运动意图 + 意图 → 指令。
// 官邸单独住 SqafDevice.h——分家理由同 Hfnl / LidarQ：官邸那侧要拖设备栈与图表的重依赖，
// 运行侧这一簇不必背它，本文件对下只依赖帧契约（PelcoDProtocol）与宪法。
// 文件名不叫 Config：SqafConfig.h 才是纯常量店（角度容差 / 作动范围 / 时限），本文件装的是
// 会动的东西——词汇典与意图。
//
// 分工：
//   · SqafVocab     = 收发词汇典：角↔码 + 4B payload 成词（TX 说与 RX 听在此对望）；
//   · SqafMoveMeta  = 一次运动的意图（UI 只说意图，命令由 SqafVocab::startOf 现场成词）；
//   · SqafVocab::withinTravel / startOf / stopOf = 意图 → 指令（文件末尾重开命名空间）。
// ============================================================

// ── SqafVocab：本卡收发词汇典 ────────────────────────────────────────────────────
// 提成命名空间而非塞进某一个类：官邸要用它成直发命令（预置位 / 自检 / 查询）、意图 → 指令要用它成运动命令、
// 官邸常态灶要用它解应答码——三家共用。塞进任一家都会逼另外两家绕道，也会拆散角↔码这一对
// （TX 说 lon→code、RX 听 code→lon，本就该对望）。
namespace SqafVocab
{
// 应答帧型词汇（帧内第 3 字节；与下方 TX 词汇同典团圆——那边说，这边听）。
enum class RxFrame : quint8
{
    PanAngle = 0x59,
    TiltAngle = 0x5b,
};

enum Direction : quint8
{
    Up = 0x08,
    RightUp = 0x0a,
    Right = 0x02,
    RightDown = 0x12,
    Down = 0x10,
    LeftDown = 0x14,
    Left = 0x04,
    LeftUp = 0x0c,
    Stop = 0x00 // 无方向：SqafMoveMeta 在 Angle 档的占位值（停机走 buildStop，不经本枚举）
};

// ── 角↔码（TX 说 / RX 听在此对望）──────────────
// 角一律本机定义：方位 [0, 360)；俯仰 0 = 平视、抬头为正（本台报文的零点与方向与此相同，故只有码域换算）。
[[nodiscard]] inline quint16 PanAngle2Code(const double deg)
{
    return static_cast<quint16>(std::lround(deg * 100.0)) % 36000;
}
// 俯仰码 = (36000 − 角×100) % 36000，**取模在减法之后**：0° 编成 0，不是 36000（解码两边本就都折回 0，这里让发出去的码也落在 0~35999）。
[[nodiscard]] inline quint16 TiltAngle2Code(const double deg)
{
    const long code = (36000 - std::lround(deg * 100.0)) % 36000;
    return static_cast<quint16>(code < 0 ? code + 36000 : code);
}
[[nodiscard]] inline double Code2PanAngle(const quint16 code) { return code % 36000 / 100.0; }
[[nodiscard]] inline double Code2TiltAngle(const quint16 code)
{
    int latitude = 36000 - static_cast<int>(code);
    if (latitude < -18000)
        latitude += 36000;
    else if (latitude >= 18000)
        latitude -= 36000;
    return latitude / 100.0;
}

// 4B 命令 payload 成词内核（长度契约取协议词汇 PelcoFrame::CommandSize——只借长度，不借含义）。
// 词不是帧：信封归 Handle TX 裁决。
[[nodiscard]] inline QByteArray makeCommand(const quint8 cmd1, const quint8 cmd2,
                                            const quint8 d1 = 0, const quint8 d2 = 0)
{
    const char b[PelcoFrame::CommandSize] = {static_cast<char>(cmd1), static_cast<char>(cmd2),
                                             static_cast<char>(d1), static_cast<char>(d2)};
    return {b, PelcoFrame::CommandSize};
}

[[nodiscard]] inline QByteArray buildStop() { return makeCommand(0x00, 0x00); }

// 云台到位：水平 / 俯仰绝对角（本机定义）→ 码 → 命令 payload
[[nodiscard]] inline QByteArray buildPanAngle(const double deg)
{
    const quint16 c = PanAngle2Code(deg);
    return makeCommand(0x00, 0x4b, static_cast<quint8>(c >> 8), static_cast<quint8>(c));
}
[[nodiscard]] inline QByteArray buildTiltAngle(const double deg)
{
    const quint16 c = TiltAngle2Code(deg);
    return makeCommand(0x00, 0x4d, static_cast<quint8>(c >> 8), static_cast<quint8>(c));
}

// 方向连续运动：方向位 + 两轴速度（Pelco-D 速度字节上限 0x3F，见 SqafConfig::speedMax）。
// **直收枚举，不由带符号速度反推方向**：两头本来都是枚举形态——界面是八个离散方向键、协议是
// 四个方向位，中间若拿 (vh, vv) 两个带符号数"统一表达"，就是多过一道"符号编码 → lround → 解回
// 方向位"的转换，没换来简洁。用不到的那一轴速度由调用方给 0（各方向键就地写明，见官邸的方向键槽）。
[[nodiscard]] inline QByteArray buildMove(const Direction dir, const quint8 hSpeed, const quint8 vSpeed)
{
    return makeCommand(0x00, dir, hSpeed, vSpeed);
}

// 预置位三个标准词（厂家文档《PelcoD云台指令》2.1～2.3）：0x03 设置、0x07 调用、0x05 删除（本机不用删除）。
[[nodiscard]] inline QByteArray buildSetPreset(const quint8 n) { return makeCommand(0x00, 0x03, 0x00, n); }
[[nodiscard]] inline QByteArray buildCallPreset(const quint8 n) { return makeCommand(0x00, 0x07, 0x00, n); }
// 本型号的特殊功能走预置位号（《增强版3081说明书》附录 2）：
//   200 号：设置 = 打开角度实时回传，调用 = 改为查询回传；211 号：设置 = 关闭自检，调用 = 开启自检。
[[nodiscard]] inline QByteArray buildAngleRealtimeOn() { return buildSetPreset(200); }
[[nodiscard]] inline QByteArray buildAngleQueryMode() { return buildCallPreset(200); }
[[nodiscard]] inline QByteArray buildSelfCheckOff() { return buildSetPreset(211); }
[[nodiscard]] inline QByteArray buildSelfCheckOn() { return buildCallPreset(211); }
// 查询一根轴的当前角（Pelco-D 标准查询词）：应答回水平 / 俯仰角帧，由官邸的 decodeStream 解。
[[nodiscard]] inline QByteArray buildQueryPan() { return makeCommand(0x00, 0x51); }
[[nodiscard]] inline QByteArray buildQueryTilt() { return makeCommand(0x00, 0x53); }
} // namespace SqafVocab

// ============================================================
// SqafMoveMeta — 一次云台运动的运行参数（唯一事实源）：只表达意图（哪种运动、什么量值），
// 命令由 SqafVocab::startOf 就地衍生。两种互不相交形态用 mode 标注。
// 住型号夹而不住连接层：「传什么参数」归型号自己定，而用它的只有 sqaf 这一台。
// ============================================================
struct SqafMoveMeta
{
    enum class Mode
    {
        Angle, // 按角度定位：用 pan / tilt
        Direc, // 按方向连续运动：用 dir + hSpeed/vSpeed
    };

    Mode mode = Mode::Angle; // 标签在前：Angle 那一支可只写 `{.pan = h, .tilt = v}`

    // ── Mode::Angle ──
    // 目标两轴角（云台系，°），两个具名 double，不用 `PanTiltModel`（用户定：位形模型是给任务编排的
    // 通用接口，官邸内部不必用它——只为发两个数不必先摆一台位形再判空）。位形只在族口（makeMoveCollector /
    // issueGoTo / decodeStream）处出现，meta 内部不用。参考系靠字段名说话：本 meta 只在云台系里用，世界角进不来。
    double pan = 0.0;
    double tilt = 0.0;

    // ── Mode::Direc ──
    // **方向是枚举、速度是两个无符号数**——协议本就是这三样（见 SqafVocab::buildMove），
    // 界面那头也本就是八个离散方向键。
    SqafVocab::Direction dir = SqafVocab::Stop;
    quint8 hSpeed = 0; // 水平轴速度（该方向用不到此轴时给 0）
    quint8 vSpeed = 0; // 俯仰轴速度（同上）
};

// ── 意图 → 指令：SqafVocab 的运行侧延伸（自由函数）──────────
// 在 SqafMoveMeta 之后重开同一命名空间：词汇在上、意图在中、意图 → 词在下，一个文件读下来顺序即依赖序。
// 两处共用：设备页直发、官邸造斗时交给族级收集器。
namespace SqafVocab
{
// 意图在不在本云台行程内：定位档目标在宪法作动范围内（SqafConfig 的 pan / tilt Min / Max），方向连动两轴速度不超
// Pelco-D 上限（SqafConfig::speedMax）。组包程序有义务检查数据是否超限。
[[nodiscard]] inline bool withinTravel(const SqafMoveMeta &m)
{
    if (m.mode == SqafMoveMeta::Mode::Angle)
        return m.pan >= SqafConfig::panMin && m.pan <= SqafConfig::panMax && m.tilt >= SqafConfig::tiltMin &&
               m.tilt <= SqafConfig::tiltMax;
    return m.hSpeed <= SqafConfig::speedMax && m.vSpeed <= SqafConfig::speedMax;
}

// 意图 → 启动指令：按 mode 现场成词（定位双包 / 移动单包）。两支都是**直取 meta 字段下发**，零转换。
[[nodiscard]] inline QList<QByteArray> startOf(const SqafMoveMeta &m)
{
    if (m.mode == SqafMoveMeta::Mode::Angle)
        return {buildPanAngle(m.pan), buildTiltAngle(m.tilt)};
    return {buildMove(m.dir, m.hSpeed, m.vSpeed)};
}

// 停机指令：让云台真正停转。
[[nodiscard]] inline QList<QByteArray> stopOf() { return {buildStop()}; }
} // namespace SqafVocab

struct SqafCollectorTag;
using SqafCollector = TypedGMBCollector<SqafCollectorTag>;
