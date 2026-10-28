#pragma once
#include <memory>
#include <boost/circular_buffer.hpp>
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
            bytes_.push_back({byte, at});
            while (bytes_.size() >= PelcoFrame::Size)
            {
                QByteArray frame(PelcoFrame::Size, Qt::Uninitialized);
                for (int i = 0; i < PelcoFrame::Size; ++i)
                    frame[i] = bytes_[i].value;
                if (!PelcoFrame::kindOf(frame))
                {
                    bytes_.pop_front(); // 帧头或校验和失败，只丢一个字节重新找帧。
                    continue;
                }
                const auto frameAt = bytes_.front().at;
                bytes_.erase_begin(PelcoFrame::Size);
                deliver(frame, frameAt);
            }
        }
    }

private:
    struct StampedByte
    {
        char value;
        RxTime at;
    };
    // 每追加一个字节就处理已有完整帧；剩余不足七字节，不会覆盖尚未判定的帧。
    boost::circular_buffer<StampedByte> bytes_{PelcoFrame::Size};
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
