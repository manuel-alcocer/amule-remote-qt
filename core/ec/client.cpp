#include "ec/client.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include <QCryptographicHash>
#include <QtEndian>

#include "ec/codes.h"

namespace amule::ec {
namespace {

// Blocking timeout for connect/read/write, matching the prototype's 20 s.
constexpr int kTimeoutMs = 20'000;

// Client identification advertised in the auth request.
const QString kClientName = QStringLiteral("amule-remote-qt");
const QString kClientVersion = QStringLiteral("0.1.0");

std::unexpected<EcError> ioError(QString message) {
    return std::unexpected(EcError{EcError::Kind::Io, std::move(message)});
}

} // namespace

// --- Password hashing helpers ----------------------------------------------

QByteArray md5Raw(QByteArrayView data) {
    return QCryptographicHash::hash(data, QCryptographicHash::Md5);
}

QString md5Hex(QByteArrayView data) {
    return QString::fromLatin1(md5Raw(data).toHex()); // toHex() is lowercase
}

QString normalizePasswordMd5(const QString& input) {
    const bool isMd5Hex =
        input.size() == 32 &&
        std::all_of(input.cbegin(), input.cend(), [](QChar c) {
            const char l = c.toLatin1();
            return l != 0 && std::isxdigit(static_cast<unsigned char>(l));
        });
    if (isMd5Hex)
        return input.toLower();
    return md5Hex(input.toUtf8());
}

Hash16 saltedPasswordHash(const QString& passMd5Hex, quint64 salt) {
    // Uppercase hex of the salt with no leading zeros (matches Rust "{:X}").
    const QString saltStr = QString::number(salt, 16).toUpper();
    const QString saltHash = md5Hex(saltStr.toLatin1());

    QByteArray combined = passMd5Hex.toLower().toLatin1();
    combined += saltHash.toLatin1();

    const QByteArray raw = md5Raw(combined);
    Hash16 h{};
    std::memcpy(h.data(), raw.constData(), 16);
    return h;
}

// --- EcClient --------------------------------------------------------------

EcClient::EcClient() = default;
EcClient::~EcClient() = default;

std::expected<std::unique_ptr<EcClient>, EcError>
EcClient::connect(const QString& host, quint16 port, const QString& password) {
    return connectWithMd5(host, port, md5Hex(password.toUtf8()));
}

std::expected<std::unique_ptr<EcClient>, EcError>
EcClient::connectWithMd5(const QString& host, quint16 port,
                         const QString& passMd5Hex) {
    std::unique_ptr<EcClient> client(new EcClient());

    client->socket_.connectToHost(host, port);
    if (!client->socket_.waitForConnected(kTimeoutMs))
        return ioError(client->socket_.errorString());
    client->socket_.setSocketOption(QAbstractSocket::LowDelayOption, 1);

    if (auto ok = client->authenticate(passMd5Hex.toLower()); !ok)
        return std::unexpected(ok.error());

    return client;
}

std::expected<void, EcError> EcClient::authenticate(const QString& passMd5Hex) {
    // Step 1: AUTH_REQ. We advertise neither CAN_ZLIB nor CAN_UTF8_NUMBERS so
    // every reply uses plain big-endian numbers and no compression.
    Packet req(EC_OP_AUTH_REQ,
               {Tag::string(EC_TAG_CLIENT_NAME, kClientName),
                Tag::string(EC_TAG_CLIENT_VERSION, kClientVersion),
                Tag::integer(EC_TAG_PROTOCOL_VERSION, EC_CURRENT_PROTOCOL_VERSION),
                Tag::empty(EC_TAG_CAN_LARGE_TAG_COUNT),
                Tag::empty(EC_TAG_CAN_PARTIAL_UPDATE)});

    auto resp = exchange(req);
    if (!resp)
        return std::unexpected(resp.error());

    // Step 2/3: if the server sent a salt, reply with the salted hash.
    if (resp->opcode == EC_OP_AUTH_SALT) {
        const auto salt = resp->tagInt(EC_TAG_PASSWD_SALT);
        if (!salt)
            return std::unexpected(
                EcError{EcError::Kind::MissingSalt,
                        QStringLiteral("the daemon did not send a password salt")});

        Packet auth(EC_OP_AUTH_PASSWD,
                    {Tag::hash(EC_TAG_PASSWD_HASH,
                               saltedPasswordHash(passMd5Hex, *salt))});
        resp = exchange(auth);
        if (!resp)
            return std::unexpected(resp.error());
    }

    switch (resp->opcode) {
    case EC_OP_AUTH_OK:
        serverVersion_ =
            resp->tagString(EC_TAG_SERVER_VERSION).value_or(QStringLiteral("unknown"));
        return {};
    case EC_OP_AUTH_FAIL:
        return std::unexpected(EcError{
            EcError::Kind::AuthFailed,
            resp->tagString(EC_TAG_STRING)
                .value_or(QStringLiteral("daemon rejected the connection"))});
    default:
        return std::unexpected(EcError{
            EcError::Kind::Unexpected,
            QStringLiteral("unexpected response: opcode 0x%1")
                .arg(resp->opcode, 2, 16, QLatin1Char('0'))});
    }
}

std::expected<Packet, EcError> EcClient::exchange(const Packet& request) {
    const QByteArray frame = request.encode();
    if (socket_.write(frame) != frame.size())
        return ioError(socket_.errorString());
    while (socket_.bytesToWrite() > 0) {
        if (!socket_.waitForBytesWritten(kTimeoutMs))
            return ioError(socket_.errorString());
    }
    return readPacket();
}

std::expected<Packet, EcError> EcClient::request(quint8 opcode) {
    return exchange(Packet(opcode));
}

std::expected<Packet, EcError> EcClient::readPacket() {
    auto header = readExactly(EC_HEADER_SIZE);
    if (!header)
        return std::unexpected(header.error());

    const quint32 flags = qFromBigEndian<quint32>(header->constData());
    const quint32 length = qFromBigEndian<quint32>(header->constData() + 4);

    auto payload = readExactly(length);
    if (!payload)
        return std::unexpected(payload.error());

    if (flags & EC_FLAG_ZLIB) {
        // We never advertise CAN_ZLIB, so the daemon must not compress. If it
        // does anyway, surface it rather than mis-decode (zlib inflate is a
        // documented follow-up; see ADR-0005).
        return std::unexpected(EcError{
            EcError::Kind::Remote,
            QStringLiteral("received a zlib-compressed reply (unsupported)")});
    }

    auto packet = Packet::decodePayload(*payload);
    if (!packet)
        return std::unexpected(EcError{EcError::Kind::Remote, packet.error()});
    return *packet;
}

std::expected<QByteArray, EcError> EcClient::readExactly(qsizetype n) {
    QByteArray buf;
    buf.reserve(n);
    while (buf.size() < n) {
        if (socket_.bytesAvailable() == 0 &&
            !socket_.waitForReadyRead(kTimeoutMs)) {
            return ioError(socket_.state() == QAbstractSocket::ConnectedState
                               ? QStringLiteral("timed out waiting for the daemon")
                               : socket_.errorString());
        }
        const QByteArray chunk = socket_.read(n - buf.size());
        if (chunk.isEmpty() &&
            socket_.state() != QAbstractSocket::ConnectedState) {
            return ioError(QStringLiteral("connection closed by the daemon"));
        }
        buf += chunk;
    }
    return buf;
}

} // namespace amule::ec
