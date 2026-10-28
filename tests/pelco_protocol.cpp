#include "Device/Handle/PelcoDHandle.h"
#include "FakeSerialPort.h"
#include <array>
#include <atomic>
#include <cstdio>
#include <mutex>
#include <vector>
#include <QCoreApplication>

namespace
{
int checks = 0;
void require(bool ok, const char *what) { ++checks; if (!ok) qFatal("%s", what); }
template <typename Predicate> bool until(Predicate ready, int ms = 3000)
{
    const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
    while (!ready() && std::chrono::steady_clock::now() < end) { QCoreApplication::processEvents(); QThread::msleep(1); }
    return ready();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    const QByteArray a = PelcoFrame(0, 0x59, 0, 100);
    const QByteArray b = PelcoFrame(0, 0x5b, 0, 50);
    require(a == QByteArray::fromHex("ff0100590064be") && b == QByteArray::fromHex("ff01005b00328e"), "Pelco envelope changed");
    const QByteArray both = a + b;
    const RxTime origin{std::chrono::seconds(1234)};

    // 两帧之间的 13 个边界任意拆分：8192 种分片，验证帧头所在读取块的时间。
    for (unsigned mask = 0; mask < (1u << 13); ++mask)
    {
        PelcoDFramer framer;
        std::vector<QByteArray> frames;
        std::vector<RxTime> times;
        std::array<RxTime, 2> expected{};
        const DataCallback collect = [&](const QByteArray &frame, RxTime at) { frames.push_back(frame); times.push_back(at); };
        int start = 0;
        for (int end = 1; end <= both.size(); ++end)
        {
            if (end != both.size() && !(mask & (1u << (end - 1)))) continue;
            const auto at = origin + std::chrono::milliseconds(end);
            for (int index = 0; index < 2; ++index)
                if (start <= index * 7 && index * 7 < end) expected[index] = at;
            framer.feed(both.mid(start, end - start), at, collect);
            start = end;
        }
        require(frames == std::vector<QByteArray>{a, b}, "partition lost or combined frames");
        require(times == std::vector<RxTime>{expected[0], expected[1]}, "frame timestamp did not follow its header block");
    }
    {
        PelcoDFramer framer;
        std::vector<QByteArray> frames;
        const DataCallback collect = [&](const QByteArray &frame, RxTime) { frames.push_back(frame); };
        QByteArray bad = a; bad[6] = 0;
        framer.feed(QByteArray(4096, 'x') + bad + a + b.first(3), origin, collect);
        require(frames == std::vector<QByteArray>{a}, "noise/checksum/tail handling failed");
        framer.feed(b.mid(3) + a + b, origin, collect);
        require(frames == std::vector<QByteArray>{a, b, a, b}, "tail plus burst delivery failed");
    }

    // 有效帧头落在坏候选内部，须逐字节重找，且使用该头所属读取块的时刻。
    {
        PelcoDFramer framer;
        std::vector<QByteArray> frames;
        std::vector<RxTime> times;
        const DataCallback collect = [&](const QByteArray &frame, RxTime at) {
            frames.push_back(frame); times.push_back(at);
        };
        framer.feed(QByteArray::fromHex("ff0000"), origin, collect);
        framer.feed(a.first(2), origin + std::chrono::seconds(1), collect);
        framer.feed(a.sliced(2), origin + std::chrono::seconds(2), collect);
        require(frames == std::vector<QByteArray>{a}, "bad candidate swallowed inner frame header");
        require(times == std::vector<RxTime>{origin + std::chrono::seconds(1)},
                "resync kept noise timestamp instead of new frame header timestamp");
        framer.feed({}, origin, collect);
        require(frames.size() == 1, "empty read duplicated delivery");
    }

    // 七个物理起点都要能交出跨环尾的整帧；内容和校验字节可以包含 FF。
    const QByteArray embeddedSync = PelcoFrame(0xff, 0x59, 0xff, 100);
    const QByteArray checksumSync = PelcoFrame(0, 0, 0, 0xfe);
    require(static_cast<quint8>(checksumSync.back()) == 0xff, "checksum FF fixture");
    for (int offset = 0; offset < PelcoFrame::Size; ++offset)
    {
        PelcoDFramer framer;
        std::vector<QByteArray> frames;
        const DataCallback collect = [&](const QByteArray &frame, RxTime) { frames.push_back(frame); };
        framer.feed(QByteArray(offset, 'x') + embeddedSync + checksumSync + a + b, origin, collect);
        require(frames == std::vector<QByteArray>{embeddedSync, checksumSync, a, b},
                "physical wrap or payload/checksum FF altered frame boundaries");
    }

    // 重复假头不得阻塞后续有效帧；校验正确但头错误同样拒绝。
    {
        PelcoDFramer framer;
        std::vector<QByteArray> frames;
        const DataCallback collect = [&](const QByteArray &frame, RxTime) { frames.push_back(frame); };
        QByteArray wrongHead = a; wrongHead[0] = 0;
        framer.feed(wrongHead, origin, collect);
        require(frames.empty(), "checksum-only candidate with bad header accepted");
        framer.feed(QByteArray::fromHex("ff").repeated(4096) + a, origin, collect);
        require(frames == std::vector<QByteArray>{a}, "repeated bad headers blocked recovery");
    }

    using namespace FakeSerialPort;
    reset();
    std::mutex gate;
    std::vector<QByteArray> received;
    std::atomic<bool> worker{true}, typedTime{true};
    {
        PelcoDHandle handle([&](const QByteArray &frame, RxTime at) {
            worker = worker && QThread::currentThread() != app.thread();
            typedTime = typedTime && at != RxTime{};
            const std::lock_guard lock(gate); received.push_back(frame);
        });
        const auto count = [&] { const std::lock_guard lock(gate); return received.size(); };
        QStringList logs;
        QObject::connect(&handle, &PelcoDHandle::logMessage, &app, [&](const QString &text) { logs.push_back(text); });
        require(handle.start(SerialParams{{"controlled", {}}}) && until([&] { return handle.isOpen(); }), "production Pelco open failed");
        enqueue(both);
        require(until([&] { return count() == 2; }), "production burst needs another input");
        QByteArray burst;
        for (int i = 0; i < 1000; ++i) burst += a;
        enqueue(burst);
        require(until([&] { return count() == 1002; }), "large burst lost frames at serial read boundaries");
        require(handle.send(a.mid(2, 4)) && writes() == std::vector<QByteArray>{a}, "production TX envelope changed");
        require(!handle.send("bad"), "invalid command accepted");
        require(logs.contains("串口指令长度错误（3 字节），未发送"), "invalid TX diagnostic missing");

        const int readBefore = reads();
        enqueue(a.first(3));
        require(until([&] { return reads() > readBefore; }), "partial frame not read");
        failRead();
        require(until([&] { return opens() == 2 && handle.isOpen(); }), "production reconnect failed");
        enqueue(a.mid(3) + b);
        require(until([&] { return count() == 1003; }), "partial frame leaked across reconnect");
        handle.stop();
        require(received.back() == b && worker && typedTime, "production receive contract changed");
    }
    require(livePorts() == 0 && closes() == 2, "production destructor did not release connections");
    std::printf("PASS %d checks: 8192 partitions, ring wrap, resync, timestamp, 1000-frame burst, Pelco RX/TX and reconnect\n", checks);
}
