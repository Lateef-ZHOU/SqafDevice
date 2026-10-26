#pragma once
#include <array>
#include <memory>
#include <HandleBase.h>
#include <SerialHandle.h>
#include "Device/Handle/PelcoDProtocol.h"

// 只保留不足一帧的尾部。每个字节携其读取块的观测时刻，重同步后仍能找到帧头所在块。
// RxTime 是软件读取观测值，不声称是硬件首字节到达时刻。
class PelcoDFramer
{
public:
    void feed(const QByteArray &chunk, const RxTime at, const DataCallback &deliver)
    {
        for (const char byte : chunk)
        {
            bytes_[used_] = byte;
            times_[used_] = at;
            ++used_;
            if (used_ < PelcoFrame::Size)
                continue;
            const QByteArray frame(bytes_.data(), PelcoFrame::Size);
            if (PelcoFrame::kindOf(frame))
            {
                const auto frameAt = times_[0];
                used_ = 0;
                deliver(frame, frameAt);
            }
            else
            {
                // 校验失败即滑动，不依赖下一次串口读取来继续检查已有字节。
                for (int i = 1; i < PelcoFrame::Size; ++i)
                {
                    bytes_[i - 1] = bytes_[i];
                    times_[i - 1] = times_[i];
                }
                --used_;
            }
        }
    }

private:
    std::array<char, PelcoFrame::Size> bytes_{};
    std::array<RxTime, PelcoFrame::Size> times_{};
    int used_ = 0;
};

// Pelco-D 适配；串口机械收发来自独立库，设备命令与角度解释留在 Sqaf。
class PelcoDHandle final : public SerialHandle<PelcoDFramer>
{
    Q_OBJECT
public:
    using SerialHandle::SerialHandle;
    ~PelcoDHandle() override { stop(); }

Q_SIGNALS:
    void connectionChanged(bool connected) override;
    void logMessage(const QString &text) const final;

private:
    [[nodiscard]] RXRuling onRX(const QByteArray &frame, const RxTime at,
                               std::shared_ptr<ConnType>) override
    {
        return {RXRuling::Decode, frame, at}; // 成帧器已检查帧头、长度和校验和。
    }

    [[nodiscard]] TXRuling onTX(const QByteArray &command, std::shared_ptr<ConnType>) const override
    {
        if (command.size() != PelcoFrame::CommandSize)
        {
            Q_EMIT logMessage(QStringLiteral("串口指令长度错误（%1 字节），未发送").arg(command.size()));
            return {};
        }
        const auto *p = reinterpret_cast<const quint8 *>(command.constData());
        return {TXRuling::Encode, PelcoFrame(p[0], p[1], p[2], p[3])};
    }
};
