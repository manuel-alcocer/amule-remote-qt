// Round-trip and edge-case tests for the EC codec (ADR-0003: ec-core is
// unit-tested headless, no GUI). Mirrors rstamule-cli's codec.rs tests.

#include <QByteArray>
#include <QObject>
#include <QtTest>

#include "ec/codes.h"
#include "ec/packet.h"
#include "ec/tag.h"

using namespace amule::ec;

namespace {

// Decode the payload of an encoded frame, skipping the 8-byte header.
std::expected<Packet, QString> decodeFrame(const QByteArray& frame) {
    return Packet::decodePayload(QByteArrayView(frame).sliced(EC_HEADER_SIZE));
}

QByteArray toBytes(const Hash16& h) {
    return QByteArray(reinterpret_cast<const char*>(h.data()), 16);
}

} // namespace

class TestEcCodec : public QObject {
    Q_OBJECT

private slots:
    void roundtripFlatPacket();
    void roundtripNestedTag();
    void smallestFitIntegerEncoding();
    void frameHeaderIsBaseFlagAndLength();
    void truncatedPayloadReportsError();
};

void TestEcCodec::roundtripFlatPacket() {
    Packet pkt(EC_OP_AUTH_REQ,
               {Tag::string(EC_TAG_CLIENT_NAME, QStringLiteral("amule-remote-qt")),
                Tag::integer(EC_TAG_PROTOCOL_VERSION, EC_CURRENT_PROTOCOL_VERSION),
                Tag::empty(EC_TAG_CAN_LARGE_TAG_COUNT)});

    const auto decoded = decodeFrame(pkt.encode());
    QVERIFY2(decoded.has_value(), qPrintable(decoded.error_or(QString())));

    QCOMPARE(decoded->opcode, EC_OP_AUTH_REQ);
    QCOMPARE(decoded->tagString(EC_TAG_CLIENT_NAME).value_or(QString()),
             QStringLiteral("amule-remote-qt"));
    QCOMPARE(decoded->tagInt(EC_TAG_PROTOCOL_VERSION).value_or(0),
             quint64{EC_CURRENT_PROTOCOL_VERSION});

    const Tag* canTag = decoded->tag(EC_TAG_CAN_LARGE_TAG_COUNT);
    QVERIFY(canTag != nullptr);
    QVERIFY(std::holds_alternative<Empty>(canTag->value));
}

void TestEcCodec::roundtripNestedTag() {
    const Hash16 hash = {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7};
    Tag partfile = Tag::hash(EC_TAG_PARTFILE, hash);
    partfile.withChildren(
        {Tag::string(EC_TAG_PARTFILE_NAME, QStringLiteral("movie.iso")),
         Tag::integer(EC_TAG_PARTFILE_SIZE_FULL, 1'500'000'000),
         Tag::integer(EC_TAG_PARTFILE_SIZE_DONE, 250)});

    Packet pkt(EC_OP_DLOAD_QUEUE, {partfile});

    const auto decoded = decodeFrame(pkt.encode());
    QVERIFY2(decoded.has_value(), qPrintable(decoded.error_or(QString())));

    const Tag* pf = decoded->tag(EC_TAG_PARTFILE);
    QVERIFY(pf != nullptr);
    QCOMPARE(toBytes(pf->asHash().value()), toBytes(hash));
    QCOMPARE(pf->childString(EC_TAG_PARTFILE_NAME).value_or(QString()),
             QStringLiteral("movie.iso"));
    QCOMPARE(pf->childInt(EC_TAG_PARTFILE_SIZE_FULL).value_or(0),
             quint64{1'500'000'000});
    QCOMPARE(pf->childInt(EC_TAG_PARTFILE_SIZE_DONE).value_or(0), quint64{250});
}

void TestEcCodec::smallestFitIntegerEncoding() {
    // Each integer must round-trip and pick the smallest fitting wire type.
    const struct {
        quint64 value;
        quint8 type;
    } cases[] = {
        {0x00, EC_TAGTYPE_UINT8},
        {0xFF, EC_TAGTYPE_UINT8},
        {0x100, EC_TAGTYPE_UINT16},
        {0xFFFF, EC_TAGTYPE_UINT16},
        {0x1'0000, EC_TAGTYPE_UINT32},
        {0xFFFF'FFFF, EC_TAGTYPE_UINT32},
        {0x1'0000'0000, EC_TAGTYPE_UINT64},
    };

    for (const auto& c : cases) {
        Packet pkt(EC_OP_NOOP, {Tag::integer(EC_TAG_STRING, c.value)});
        const auto decoded = decodeFrame(pkt.encode());
        QVERIFY2(decoded.has_value(), qPrintable(decoded.error_or(QString())));
        QCOMPARE(decoded->tagInt(EC_TAG_STRING).value_or(0), c.value);

        // Inspect the wire type byte: header(8) + opcode(1) + tagcount(2) +
        // name(2) then the type byte.
        const QByteArray frame = pkt.encode();
        const quint8 wireType =
            static_cast<quint8>(frame.at(EC_HEADER_SIZE + 1 + 2 + 2));
        QCOMPARE(wireType, c.type);
    }
}

void TestEcCodec::frameHeaderIsBaseFlagAndLength() {
    Packet pkt(EC_OP_STAT_REQ, {Tag::integer(EC_TAG_DETAIL_LEVEL, EC_DETAIL_CMD)});
    const QByteArray frame = pkt.encode();

    QVERIFY(frame.size() >= EC_HEADER_SIZE);
    const quint32 flags = qFromBigEndian<quint32>(frame.data());
    const quint32 length = qFromBigEndian<quint32>(frame.data() + 4);
    QCOMPARE(flags, EC_FLAG_BASE);
    QCOMPARE(length, static_cast<quint32>(frame.size() - EC_HEADER_SIZE));
}

void TestEcCodec::truncatedPayloadReportsError() {
    Packet pkt(EC_OP_DLOAD_QUEUE,
               {Tag::string(EC_TAG_PARTFILE_NAME, QStringLiteral("x"))});
    QByteArray frame = pkt.encode();
    frame.chop(2); // lose the last two payload bytes

    const auto decoded = decodeFrame(frame);
    QVERIFY(!decoded.has_value());
}

QTEST_MAIN(TestEcCodec)
#include "tst_eccodec.moc"
