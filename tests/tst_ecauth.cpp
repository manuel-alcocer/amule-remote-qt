// Tests for the EC authentication helpers (MD5 digest, salted password hash,
// password normalization). No networking — these mirror rstamule-cli's
// client.rs unit tests and run headless (ADR-0003).

#include <QByteArray>
#include <QObject>
#include <QtTest>

#include "ec/client.h"

using namespace amule::ec;

namespace {

QByteArray toBytes(const Hash16& h) {
    return QByteArray(reinterpret_cast<const char*>(h.data()), 16);
}

} // namespace

class TestEcAuth : public QObject {
    Q_OBJECT

private slots:
    void md5HexKnownVectors();
    void saltStrFormattingDropsLeadingZeros();
    void saltedHashIsDeterministic();
    void normalizePasswordDetectsMd5Digest();
};

void TestEcAuth::md5HexKnownVectors() {
    QCOMPARE(md5Hex(QByteArray("")),
             QStringLiteral("d41d8cd98f00b204e9800998ecf8427e"));
    QCOMPARE(md5Hex(QByteArray("abc")),
             QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));
}

void TestEcAuth::saltStrFormattingDropsLeadingZeros() {
    // The salt is formatted as uppercase hex with no leading zeros (Rust "{:X}").
    QCOMPARE(QString::number(quint64{0x00AB12CD}, 16).toUpper(),
             QStringLiteral("AB12CD"));
}

void TestEcAuth::saltedHashIsDeterministic() {
    const QString passMd5 = md5Hex(QByteArray("secret"));
    const Hash16 a = saltedPasswordHash(passMd5, 0x00AB12CD);
    const Hash16 b = saltedPasswordHash(passMd5, 0x00AB12CD);
    QCOMPARE(toBytes(a), toBytes(b));

    // A different salt must yield a different digest.
    const Hash16 c = saltedPasswordHash(passMd5, 0x00AB12CE);
    QVERIFY(toBytes(a) != toBytes(c));

    // Cross-check the documented algorithm: md5(pass_md5 + md5_hex("AB12CD")).
    const QString saltHash = md5Hex(QByteArray("AB12CD"));
    const QByteArray expected =
        md5Raw((passMd5 + saltHash).toLatin1());
    QCOMPARE(toBytes(a), expected);
}

void TestEcAuth::normalizePasswordDetectsMd5Digest() {
    // A plaintext password is hashed.
    QCOMPARE(normalizePasswordMd5(QStringLiteral("abc")),
             QStringLiteral("900150983cd24fb0d6963f7d28e17f72"));

    // An already-MD5 32-char hex digest is kept (lowercased), not re-hashed.
    const QString digest = QStringLiteral("900150983CD24FB0D6963F7D28E17F72");
    QCOMPARE(normalizePasswordMd5(digest), digest.toLower());

    // A 32-char string that is not all hex is treated as a plaintext password.
    const QString notHex = QStringLiteral("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz");
    QCOMPARE(normalizePasswordMd5(notHex), md5Hex(notHex.toUtf8()));
}

QTEST_MAIN(TestEcAuth)
#include "tst_ecauth.moc"
