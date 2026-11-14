#pragma once
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
class PelcoDHandle : public SerialHandle
{
    Q_OBJECT
public:
    ~PelcoDHandle() override = default;

Q_SIGNALS:
    void connectionChanged(HandleCore::ResourcePhase connected) override;
    void logMessage(const std::string &text) const final;

protected:
    // 单字节输入；固定七字节窗口每次至多形成一帧，保留帧头所在读取块的时刻。
    [[nodiscard]] RXRuling onRX(std::span<const unsigned char> view, const RxTime at,
                               std::shared_ptr<SerialConn>) override
    {
        bytes_.push_back({static_cast<char>(view.front()), at});
        if (bytes_.size() < PelcoFrame::Size)
            return {};
        for (int i = 0; i < PelcoFrame::Size; ++i)
            frame_[i] = bytes_[i].value;
        if (!PelcoFrame::kindOf(QByteArray::fromRawData(frame_.data(), PelcoFrame::Size))) {
            bytes_.pop_front();
            return {};
        }
        const auto frameAt = bytes_.front().at;
        bytes_.erase_begin(PelcoFrame::Size);
        return {{reinterpret_cast<const unsigned char *>(frame_.data()), frame_.size()}, RXRuling::Decode, frameAt};
    }

private:
    std::shared_ptr<SerialConn> connOpen(const SerialParams &params) final
    {
        bytes_.clear();
        return SerialHandle::connOpen(params);
    }
    struct StampedByte { char value; RxTime at; };
    // frame_ 为同步上传提供连续存储；下一次 onRX 前不会改变。
    boost::circular_buffer<StampedByte> bytes_{PelcoFrame::Size};
    std::array<char, PelcoFrame::Size> frame_{};

    [[nodiscard]] TXRuling onTX(std::span<const unsigned char> view, std::shared_ptr<SerialConn>) const override
    {
        if (view.size() != PelcoFrame::CommandSize)
        {
            Q_EMIT logMessage(std::format("串口指令长度错误（{} 字节），未发送", view.size()));
            return {};
        }
        const PelcoFrame command(view[0], view[1], view[2], view[3]);
        return {{reinterpret_cast<const unsigned char *>(command.constData()),
                 reinterpret_cast<const unsigned char *>(command.constData()) + command.size()}, TXRuling::Encode};
    }
};
