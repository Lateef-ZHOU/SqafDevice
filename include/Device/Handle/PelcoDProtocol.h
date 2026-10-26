#pragma once
#include <numeric>
#include <QByteArray>
#include <QtGlobal>

// Pelco-D 的七字节信封；设备命令含义由型号解释，当前发送地址保持 0x01。
class PelcoFrame final : public QByteArray
{
public:
    static constexpr int Size = 7;        // 帧定长唯一声明点（RX 切帧步长）
    static constexpr int CommandSize = 4; // TX 词汇 payload 定长 {cmd1, cmd2, d1, d2}
    static constexpr quint8 Sync = 0xFF;  // 帧头（TX 构造写 / RX 扫描判——双栖同住）

    // 构造即完整报文：FF 01 cmd1 cmd2 d1 d2 sum（地址固定 0x01；sum = (01+cmd1+cmd2+d1+d2) & 0xFF）。
    explicit PelcoFrame(const quint8 cmd1, const quint8 cmd2, const quint8 d1 = 0, const quint8 d2 = 0)
        : QByteArray(fill(cmd1, cmd2, d1, d2))
    {
    }

    // 检查首个七字节帧的帧头与校验和；不足一帧时拒绝，成帧器逐窗调用。
    [[nodiscard]] static bool kindOf(const QByteArray &frame)
    {
        if (frame.size() < Size)
            return false;
        const auto *p = reinterpret_cast<const quint8 *>(frame.constData());
        return p[0] == Sync && p[6] == (std::accumulate(p + 1, p + 6, 0u) & 0xFF);
    }

private:
    // 组包内核：栈上定长成帧、一次拷入。
    [[nodiscard]] static QByteArray fill(const quint8 cmd1, const quint8 cmd2, const quint8 d1, const quint8 d2)
    {
        quint8 b[Size] = {Sync, 0x01, cmd1, cmd2, d1, d2, 0};
        b[6] = std::accumulate(b + 1, b + 6, 0u) & 0xFF;
        return {reinterpret_cast<const char *>(b), Size};
    }
};
