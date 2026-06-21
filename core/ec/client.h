// Blocking EC client: TCP connection, salt+MD5 authentication and request
// helpers. Designed to be owned by a single worker thread (ADR-0004) and to
// perform blocking request/response exchanges. Ported from rstamule-cli's
// client.rs (see ADR-0005).

#pragma once

#include <expected>
#include <memory>

#include <QByteArrayView>
#include <QString>
#include <QTcpSocket>
#include <QtTypes>

#include "ec/packet.h"
#include "ec/tag.h"

namespace amule::ec {

// Error from an EC exchange. `kind` allows callers to branch (e.g. auth failure
// vs. transport loss); `message` is a human-readable detail.
struct EcError {
    enum class Kind {
        Io,          // transport error (connect/read/write/timeout)
        AuthFailed,  // daemon rejected the credentials
        Unexpected,  // unexpected opcode in the handshake
        Remote,      // malformed/decoding error from the daemon
        MissingSalt, // daemon did not send a password salt
    };

    Kind kind = Kind::Io;
    QString message;
};

// An authenticated EC connection to a running amuled. Non-copyable; created via
// the static factories, which connect and authenticate before returning.
class EcClient {
public:
    EcClient(const EcClient&) = delete;
    EcClient& operator=(const EcClient&) = delete;
    ~EcClient();

    // Connect to host:port and authenticate with a plaintext password (it is
    // MD5-hashed and salt-combined exactly as amulegui does).
    static std::expected<std::unique_ptr<EcClient>, EcError>
    connect(const QString& host, quint16 port, const QString& password);

    // Connect using a password that is already the lowercase MD5 hex digest of
    // the real password — the form aMule stores in remote.conf's Password=.
    static std::expected<std::unique_ptr<EcClient>, EcError>
    connectWithMd5(const QString& host, quint16 port, const QString& passMd5Hex);

    [[nodiscard]] QString serverVersion() const { return serverVersion_; }

    // Send a request and read exactly one response packet.
    std::expected<Packet, EcError> exchange(const Packet& request);

    // Convenience: send a tag-less request with the given opcode.
    std::expected<Packet, EcError> request(quint8 opcode);

private:
    EcClient();

    std::expected<void, EcError> authenticate(const QString& passMd5Hex);
    std::expected<Packet, EcError> readPacket();
    std::expected<QByteArray, EcError> readExactly(qsizetype n);

    QTcpSocket socket_;
    QString serverVersion_;
};

// --- Password hashing helpers (exposed for testing) ------------------------

// Raw 16-byte MD5 digest.
QByteArray md5Raw(QByteArrayView data);

// Lowercase 32-char MD5 hex digest.
QString md5Hex(QByteArrayView data);

// Normalize a user-entered password to the lowercase 32-char MD5 hex digest the
// daemon stores: an input that already looks like an MD5 hex digest is
// lowercased as-is, otherwise it is MD5-hashed.
QString normalizePasswordMd5(const QString& input);

// Compute EC_TAG_PASSWD_HASH from the password's MD5 hex digest and the salt:
//   salt_str  = uppercase hex of salt, no leading zeros
//   salt_hash = lowercase MD5 hex of salt_str
//   result    = raw 16-byte MD5(pass_md5_hex + salt_hash)
Hash16 saltedPasswordHash(const QString& passMd5Hex, quint64 salt);

} // namespace amule::ec
