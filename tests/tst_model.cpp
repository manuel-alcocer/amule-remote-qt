// Tests for the domain model: human-readable formatting and parsing of
// synthetic EC packets through the query layer's logic. No networking — these
// run headless (ADR-0003).

#include <QByteArray>
#include <QObject>
#include <QtTest>

#include "ec/codes.h"
#include "ec/packet.h"
#include "ec/tag.h"
#include "model/model.h"

using namespace amule;
using namespace amule::ec;

class TestModel : public QObject {
    Q_OBJECT

private slots:
    void humanBytesFormatting();
    void humanRateFormatting();
    void humanCountFormatting();
    void humanSizePairFormatting();
    void statsAndHighIdDetection();
    void downloadProgressAndStatusLabel();
    void sourceClientIpString();
};

void TestModel::humanBytesFormatting() {
    QCOMPARE(humanBytes(0), QStringLiteral("0 B"));
    QCOMPARE(humanBytes(512), QStringLiteral("512 B"));
    QCOMPARE(humanBytes(1024), QStringLiteral("1.00 KiB"));
    QCOMPARE(humanBytes(1536), QStringLiteral("1.50 KiB"));
    QCOMPARE(humanBytes(1024ULL * 1024), QStringLiteral("1.00 MiB"));
    QCOMPARE(humanBytes(1024ULL * 1024 * 1024), QStringLiteral("1.00 GiB"));
}

void TestModel::humanRateFormatting() {
    QCOMPARE(humanRate(0), QStringLiteral("0 B/s"));
    QCOMPARE(humanRate(2048), QStringLiteral("2.00 KiB/s"));
}

void TestModel::humanCountFormatting() {
    QCOMPARE(humanCount(0), QStringLiteral("0"));
    QCOMPARE(humanCount(999), QStringLiteral("999"));
    QCOMPARE(humanCount(34406), QStringLiteral("34.4k"));
    QCOMPARE(humanCount(38678693), QStringLiteral("38.7M"));
    QCOMPARE(humanCount(2500000000ULL), QStringLiteral("2.5G"));
}

void TestModel::humanSizePairFormatting() {
    // Done and total share the unit of the total.
    QCOMPARE(humanSizePair(0, 0), QStringLiteral("0 / 0 B"));
    QCOMPARE(humanSizePair(512, 1024), QStringLiteral("0.5 / 1.0 KiB"));
    const quint64 mib = 1024ULL * 1024;
    QCOMPARE(humanSizePair(mib / 2, 60 * mib), QStringLiteral("0.5 / 60.0 MiB"));
}

void TestModel::statsAndHighIdDetection() {
    // Build a synthetic EC_OP_STATS reply and parse it like fetchStats does.
    Packet pkt(EC_OP_STATS,
               {Tag::integer(EC_TAG_STATS_DL_SPEED, 4096),
                Tag::integer(EC_TAG_STATS_UL_SPEED, 1024),
                Tag::integer(EC_TAG_STATS_ED2K_ID, 0x12345678)});
    const auto decoded =
        Packet::decodePayload(QByteArrayView(pkt.encode()).sliced(EC_HEADER_SIZE));
    QVERIFY(decoded.has_value());

    Stats s;
    for (const Tag& t : decoded->tags) {
        const quint64 v = t.asInt().value_or(0);
        if (t.name == EC_TAG_STATS_DL_SPEED)
            s.dlSpeed = v;
        else if (t.name == EC_TAG_STATS_UL_SPEED)
            s.ulSpeed = v;
        else if (t.name == EC_TAG_STATS_ED2K_ID)
            s.ed2kId = static_cast<quint32>(v);
    }
    QCOMPARE(s.dlSpeed, quint64{4096});
    QCOMPARE(s.ulSpeed, quint64{1024});
    QVERIFY(s.isHighId()); // 0x12345678 > 0x00FFFFFF

    Stats low;
    low.ed2kId = 0x00ABCDEF;
    QVERIFY(!low.isHighId());
}

void TestModel::downloadProgressAndStatusLabel() {
    Download d;
    d.sizeFull = 1000;
    d.sizeDone = 250;
    QCOMPARE(d.progress(), 0.25F);

    d.sizeFull = 0;
    QCOMPARE(d.progress(), 0.0F); // no division by zero

    d.status = status::PAUSED;
    QCOMPARE(d.statusLabel(), QStringLiteral("Paused"));
    d.status = status::COMPLETE;
    QCOMPARE(d.statusLabel(), QStringLiteral("Complete"));
    d.status = 99;
    QCOMPARE(d.statusLabel(), QStringLiteral("Unknown"));
}

void TestModel::sourceClientIpString() {
    SourceClient c;
    c.userIp = 0xC0000201; // 192.0.2.1 (RFC 5737 doc range), big-endian packed
    QCOMPARE(c.ipString(), QStringLiteral("192.0.2.1"));
}

QTEST_MAIN(TestModel)
#include "tst_model.moc"
