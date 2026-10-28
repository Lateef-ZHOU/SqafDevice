#pragma once
#include <QtQml/qqmlregistration.h>
#include <algorithm>
#include <memory>
#include <optional>
#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>
#include <QVariantMap>
#include <QtEndian> // qFromBigEndian（常态灶解应答帧码字段）
#include <QtGlobal>

#include "Algorithm/Geometry/GimbalModel.h"                  // PanTiltModel（造斗收的目标位形）
#include "Device/Handle/PelcoDHandle.h" // 连接机制 PelcoDHandle
#include "Device/GMB/GMBDevice.h"               // 云台族两层（族基 GMBDeviceBase + 族层 GMBDevice）
#include "Device/GMB/Sqaf/SqafConfig.h"      // 型号夹宪法
#include "Device/GMB/Sqaf/SqafRunSide.h"     // 运行侧：词汇典 + 运动意图 + 意图 → 指令

// ============================================================
// SqafDevice.h — sqaf 云台型号夹的**官邸**（词汇典 / 运动意图 / 意图 → 指令住 SqafRunSide.h）。
// 信封（FF / 地址 / 校验和）归 PelcoDHandle，中间 4 字节的高层协议整本归本夹。
// 换异构云台 = 平行新增型号夹（编排层不动）；换同构云台 = 只改本夹宪法里的 pan* / tilt*。
// ============================================================

// ============================================================
// SqafDevice — sqaf 云台设备子类：子夹最高长官（职责对齐 HfnlDevice 之于 Hfnl）。四件事：
//   ① **造斗只走族口**：makeMoveCollector 照位形造族级收集器（方向键连动只直发、不造斗）；
//      实时回显归族层常态灶（本官邸只填解析口 decodeStream）；
//   ② 设备页云台指令直发（预置位 / 自检 / 角度回传开关 / 查询）——字节事实全在 SqafVocab 的词里，
//      本类只说意图；直发验锁统一在 Device::send（canEngage）；
//   ③ **是一台 pan-tilt 云台**：族层写死用 `PanTiltModel` 反解与表达位形；本类不继承、不持有模型。
//      本台行程取 SqafConfig::pan* / tilt*，预览与发令共用型号判据，输入框随模板接收范围；
//   ④ 型号事实按值交给自己的 QML 官邸，页面内容和分区顺序由 QML 决定。
// ============================================================
class SqafDevice final
#ifdef Q_MOC_RUN
    : public GMBDeviceBase // 模板层继承的族级元对象。
#else
    : public GMBDevice<PelcoDHandle, SqafCollector>
#endif
{
    Q_OBJECT
    QML_ELEMENT
    QML_ATTACHED(SqafDevice)
    QML_UNCREATABLE("由对应 QML 官邸创建和拥有")
    // 表单输入仅由 QML 官邸持有；连接和命令入口接收本次参数。
    // 标定整值从族层读取；固定行程随 QML 模板按值交入，不另存设备属性。

public:
    static SqafDevice *qmlAttachedProperties(QObject *owner) { return new SqafDevice(owner); }
    Q_INVOKABLE QVariantMap positioningParameters() const
    {
        return {{"panMin", SqafConfig::panMin}, {"panMax", SqafConfig::panMax},
                 {"tiltMin", SqafConfig::tiltMin}, {"tiltMax", SqafConfig::tiltMax}};
    }

    Q_INVOKABLE [[nodiscard]] QVariantMap chartParameters() const
    {
        return {{"configuration", QVariantMap{
                     {"windowSeconds", std::chrono::duration<double>(SqafConfig::angleChartWindow).count()},
                     {"historySeconds", std::chrono::duration<double>(SqafConfig::angleHistoryWindow).count()},
                     {"intervalSeconds", std::chrono::duration<double>(SqafConfig::uplinkInterval).count()}}}};
    }

    ~SqafDevice() override { PreDestruction(); }

    explicit SqafDevice(QObject *parent) : GMBDevice(QStringLiteral("视强云台"), QStringLiteral("串口 · Pelco-D"), parent)
    {
        setObjectName(QStringLiteral("Sqaf")); // ASCII 标识：落盘会话目录名用
    }

    // 族口：**本台没有滑环**——越过零点会绞线，轮末只能沿原路退回（任务页的轮末那一笔据此画）。
    [[nodiscard]] bool hasSlipRing() const noexcept final { return false; }

    // 预览只查询型号判据，不造 Collector、不发令、不记录操作日志。
    Q_INVOKABLE bool withinTravel(const double pan, const double tilt) const override
    {
        return SqafVocab::withinTravel(SqafMoveMeta{.pan = pan, .tilt = tilt});
    }

    // 枚举端口交给 QML；线路规格由 SqafConfig 固定。
    Q_INVOKABLE QVariantMap connectionParameters() const
    {
        QStringList names, tips;
        for (const auto &port : serialPorts())
        {
            names << port.portName;
            tips << port.description;
        }
        return {{"serialPortOptions", names}, {"serialPortTips", tips}, {"serialPortDefault", 0}};
    }

    Q_INVOKABLE QVariantMap controlParameters() const
    {
        return {{"speedMin", SqafConfig::speedMin}, {"speedMax", SqafConfig::speedMax},
                {"speedDefault", SqafConfig::speedDefault},
                {"presetMin", SqafConfig::presetMin}, {"presetMax", SqafConfig::presetMax},
                {"presetDefault", SqafConfig::presetDefault}};
    }

private:
    void PreDestruction() noexcept final
    {
        static_assert(FinalDevice<SqafDevice, decltype(&SqafDevice::PreDestruction)>);
        GMBDevice::PreDestruction();
    }

    // 首用时枚举一次；选项次序与 QML 收到的表一致，插拔仍需重启程序。
    [[nodiscard]] static const QList<SerialPortInfo> &serialPorts()
    {
        static const QList<SerialPortInfo> ports = availablePorts();
        return ports;
    }

    // 云台族可变点之一（NVI）：照目标位形造一只**族级收集器**。本台的角度定位不带速度字段，故除位形
    // 外无可填——族口 collectorFor 在此正好一一对应。位形拆成意图两数，过行程检查
    // （SqafVocab::withinTravel，与设备页直发同一道）后，交出启动 / 停机指令、容差、时限；驻留时长是调用方交的。
    [[nodiscard]] std::shared_ptr<const SqafCollector>
    makeMoveCollector(const PanTiltModel &target, const std::chrono::milliseconds hold,
                      const GMBMoveParameters &) const override
    {
        const SqafMoveMeta meta{.pan = target.pan(), .tilt = target.tilt()};
        if (!SqafVocab::withinTravel(meta))
            return nullptr;
        return SqafCollector::createPrototype(target, SqafConfig::angleTol, hold,
                                              SqafVocab::startOf(meta), SqafVocab::stopOf(),
                                              SqafConfig::expectedDurationMs);
    }

    // [worker] 族层常态灶的解析口：一帧 Pelco 应答 → 实测位形（回显与喂斗共用，**每一口都交**）。
    // **本台一帧只报一根轴**（经、纬分两种应答帧），故官邸暂存两轴各自最近一次的值，每来一帧更新其中
    // 一根；两根都收到过才摆位形交出（另一根拿 0 补位就是占位冒充读数）。族层链路遂与一帧同报两轴的
    // 型号一字不差。
    // 上抛的是**完整 7 字节 Pelco 帧**（PelcoDFramer 做帧头 / 长度 / 校验和三判，不剥
    // 信封）：p[3] 取应答帧型、p[4..5] 取大端码值——解码走 SqafVocab。
    [[nodiscard]] std::optional<PanTiltModel> decodeStream(const QByteArray &raw) override
    {
        if (raw.size() < PelcoFrame::Size)
            return std::nullopt; // 短于一帧：不认（正常路径上抛恒是整帧，此判为防御）
        const auto *p = reinterpret_cast<const quint8 *>(raw.constData());
        const quint16 code = qFromBigEndian<quint16>(p + 4);
        switch (static_cast<SqafVocab::RxFrame>(p[3]))
        {
        case SqafVocab::RxFrame::PanAngle:
            echoPan_ = SqafVocab::Code2PanAngle(code);
            break;
        case SqafVocab::RxFrame::TiltAngle:
            echoTilt_ = SqafVocab::Code2TiltAngle(code);
            break;
        default:
            return std::nullopt; // 非角度应答：不回显、不喂斗
        }
        if (!echoPan_ || !echoTilt_)
            return std::nullopt; // 两轴还没凑齐一对：不交半对
        return PanTiltModel(*echoPan_, *echoTilt_);
    }

    // [仅 worker] 两轴各自最近一次的应答值，各自可空——"没收到过"不得冒充"收到过 0"。
    std::optional<double> echoPan_;
    std::optional<double> echoTilt_;

    // ════════════════════════════════════════════════════════════════════════════
    // 设备页：族层规定分区，本官邸交组件地址和完整创建参数。
    //   · 设备连接 —— 串口参数 + 连接 / 断开；
    //   · 设备控制 —— 两个速度、九宫格点动、云台指令（预置位 / 自检 / 回传 / 查询）。
    //     （前往云台角 / 前往世界角 / 停止 / 安装标定是族级的，在族层那块"状态与定位"分区里。）
    // 按钮接本类的槽；设置由 QML 官邸保管，点击时显式传入。
    // **动作一律直发**（用户定）：这里是测试用的控制，不起 run、不拿锁、不等到位——随时能被下一条
    // 命令（包括停止、松开方向键）打断，测试阶段要的正是这个。任务里的自动控制走协程与锁。
    // **但验锁**：验锁统一在 `Device::send`（canEngage）——设备页传空令牌 = "我不持锁"，设备空闲且
    // 已连接才放行，被任务占着一律拒并记日志（连停止也拒：任务的停止走任务页）。
    // **动作对 QML 公开**：那几份 .qml 直接按名调这些槽（SqafConnection.qml / SqafControl.qml
    // 与族层的 GimbalSection.qml；按名绑定的耦合关在型号夹内部——QML 与官邸同作者、一起改）。
    // ════════════════════════════════════════════════════════════════════════════
public:
    using GMBDevice::setMounting;
public Q_SLOTS:
    void open(const int serialPortIndex)
    {
        const auto &ports = serialPorts();
        if (ports.isEmpty())
        {
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("没有可用串口，未连接"));
            return;
        }
        if (serialPortIndex < 0 || serialPortIndex >= ports.size())
        {
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("串口选项下标无效，未连接"));
            return;
        }
        this->device().open(SerialParams{ports.at(serialPortIndex), qToUnderlying(SqafConfig::baudRate),
                                        SqafConfig::dataBits, SqafConfig::stopBits, SqafConfig::parity});
    }
    void close() { this->device().close(); }

    // 云台指令直发：Device::send 验锁，成功文案由 QML 记录。
    bool storePreset(const int preset)
    {
        if (!validPreset(preset)) return false;
        return this->device().send({SqafVocab::buildSetPreset(static_cast<quint8>(preset))}, {});
    }
    bool recallPreset(const int preset)
    {
        if (!validPreset(preset)) return false;
        return this->device().send({SqafVocab::buildCallPreset(static_cast<quint8>(preset))}, {});
    }
    bool enableSelfCheck() { return this->device().send({SqafVocab::buildSelfCheckOn()}, {}); }
    bool disableSelfCheck() { return this->device().send({SqafVocab::buildSelfCheckOff()}, {}); }
    bool enableAngleRealtime() { return this->device().send({SqafVocab::buildAngleRealtimeOn()}, {}); }
    bool setAngleQueryMode() { return this->device().send({SqafVocab::buildAngleQueryMode()}, {}); }
    bool queryPan() { return this->device().send({SqafVocab::buildQueryPan()}, {}); }
    bool queryTilt() { return this->device().send({SqafVocab::buildQueryTilt()}, {}); }

    // Qt 入口接收页面参数；成功只返回结果，文案由 QML 生成。标定三个角一次提交。
    bool issueGoTo(const double pan, const double tilt, const QVariantMap &parameters)
    {
        return GMBDevice::issueGoTo(pan, tilt, parameters);
    }
    bool setMounting(const double azimuth, const double elevation, const double roll)
    {
        return GMBDeviceBase::setMounting(WorldModel(azimuth, elevation, roll), {});
    }
    bool issueStop() override { return this->device().send(SqafVocab::stopOf(), {}); }
    bool moveLeftUp(const int hSpeed, const int vSpeed) { return issueMove(SqafVocab::LeftUp, speedCode(hSpeed), speedCode(vSpeed)); }
    bool moveUp(const int vSpeed) { return issueMove(SqafVocab::Up, 0, speedCode(vSpeed)); }
    bool moveRightUp(const int hSpeed, const int vSpeed) { return issueMove(SqafVocab::RightUp, speedCode(hSpeed), speedCode(vSpeed)); }
    bool moveLeft(const int hSpeed) { return issueMove(SqafVocab::Left, speedCode(hSpeed), 0); }
    bool moveRight(const int hSpeed) { return issueMove(SqafVocab::Right, speedCode(hSpeed), 0); }
    bool moveLeftDown(const int hSpeed, const int vSpeed) { return issueMove(SqafVocab::LeftDown, speedCode(hSpeed), speedCode(vSpeed)); }
    bool moveDown(const int vSpeed) { return issueMove(SqafVocab::Down, 0, speedCode(vSpeed)); }
    bool moveRightDown(const int hSpeed, const int vSpeed) { return issueMove(SqafVocab::RightDown, speedCode(hSpeed), speedCode(vSpeed)); }

private:

    // 本台的角度定位没有速度字段（Pelco-D 扩展命令只带角度），故 meta 只填两轴角度。
    bool issueGoTo(const PanTiltModel &target, const GMBMoveParameters &) override
    {
        return issue(SqafMoveMeta{.pan = target.pan(), .tilt = target.tilt()});
    }

    // 实际发令时钳制使用轴的速度，再缩窄为协议字节；未使用的轴由调用处传 0。
    [[nodiscard]] static quint8 speedCode(const int speed)
    {
        return static_cast<quint8>(std::clamp(speed, SqafConfig::speedMin, SqafConfig::speedMax));
    }

    // 直发一份意图：组包前查行程，与造斗同一道 SqafVocab::withinTravel（越界不发、记一条）；成帧走
    // SqafVocab::startOf，与 Collector 共用；Device::send 验锁并直接返回发送结果。
    bool issue(const SqafMoveMeta &meta)
    {
        if (!SqafVocab::withinTravel(meta))
        {
            Q_EMIT this->device().logSource().logMessage(QStringLiteral("云台运动参数超出本云台行程（水平 %1~%2°、俯仰 %3~%4°、速度 ≤ %5），未下发")
                                .arg(SqafConfig::panMin).arg(SqafConfig::panMax).arg(SqafConfig::tiltMin)
                                .arg(SqafConfig::tiltMax).arg(SqafConfig::speedMax));
            return false;
        }
        return this->device().send(SqafVocab::startOf(meta), {});
    }

    // 编号检查先于缩窄转换，非法输入不成帧。
    [[nodiscard]] bool validPreset(const int preset)
    {
        if (preset >= SqafConfig::presetMin && preset <= SqafConfig::presetMax)
            return true;
        Q_EMIT this->device().logSource().logMessage(QStringLiteral("预置位编号超出范围（%1~%2），未下发")
                                         .arg(SqafConfig::presetMin).arg(SqafConfig::presetMax));
        return false;
    }

    bool issueMove(const SqafVocab::Direction dir, const quint8 hSpeed, const quint8 vSpeed)
    {
        return issue(SqafMoveMeta{.mode = SqafMoveMeta::Mode::Direc, .dir = dir, .hSpeed = hSpeed, .vSpeed = vSpeed});
    }

};
