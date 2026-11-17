#pragma once
#include <vector>
#include <string>
#include <format>
#include <array>
#include <span>
#include <memory>
#include <boost/circular_buffer.hpp>
#include <HandleBase.h>
#include <SerialHandle.h>
#include "PelcoDProtocol.h"

// Pelco-D 适配；串口机械收发来自独立库，设备命令与角度解释留在 Sqaf。
class PelcoDHandle : public SerialHandle<PelcoFrame::Size / 2>
{
    Q_OBJECT
public:
    using SerialHandle::SerialHandle;
    ~PelcoDHandle() override = default;

Q_SIGNALS:
    void connectionChanged(HandleCore::ResourcePhase connected) override;
    void logMessage(const std::string &text) const final;

protected:
    // 每块最多三字节；滑动七字节校验窗口，保留帧头所在读取块的时刻。
    [[nodiscard]] RXRuling onRX(std::span<const unsigned char> view, const RxTime at,
                               std::shared_ptr<SerialConn>) override
    {
        for (const auto byte : view)
            bytes_.push_back({static_cast<char>(byte), at});
        while (bytes_.size() >= PelcoFrame::Size) {
            for (int i = 0; i < PelcoFrame::Size; ++i)
                frame_[i] = bytes_[i].value;
            if (!PelcoFrame::kindOf(QByteArray::fromRawData(frame_.data(), PelcoFrame::Size))) {
                bytes_.pop_front();
                continue;
            }
            const auto frameAt = bytes_.front().at;
            bytes_.erase_begin(PelcoFrame::Size);
            return {{reinterpret_cast<const unsigned char *>(frame_.data()), frame_.size()}, RXRuling::Decode, frameAt};
        }
        return {};
    }

private:
    std::shared_ptr<SerialConn> connOpen(const SerialParams &params) final
    {
        bytes_.clear();
        return SerialHandle::connOpen(params);
    }
    struct StampedByte { char value; RxTime at; };
    // frame_ 为同步上传提供连续存储；下一次 onRX 前不会改变。
    // 六字节尾部加最多三字节输入，解析前不得覆盖旧字节。
    boost::circular_buffer<StampedByte> bytes_{PelcoFrame::Size + PelcoFrame::Size / 2 - 1};
    std::array<char, PelcoFrame::Size> frame_{};

    [[nodiscard]] TXRuling onTX(const std::vector<unsigned char> &payload, std::shared_ptr<SerialConn>) const override
    {
        if (payload.size() != PelcoFrame::CommandSize)
        {
            Q_EMIT logMessage(std::format("串口指令长度错误（{} 字节），未发送", payload.size()));
            return {};
        }
        const PelcoFrame command(payload[0], payload[1], payload[2], payload[3]);
        return {{reinterpret_cast<const unsigned char *>(command.constData()),
                 reinterpret_cast<const unsigned char *>(command.constData()) + command.size()}, TXRuling::Encode};
    }
};
