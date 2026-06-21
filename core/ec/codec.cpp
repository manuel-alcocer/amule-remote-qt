// EC packet/tag (de)serialization.
//
// Deliberately operates in the simplest negotiated mode: every framing number
// is a plain fixed-width big-endian integer and payloads are never compressed.
// Ported from rstamule-cli's codec.rs (see ADR-0005).
//
// Wire layout:
//   header : u32 flags (BE) | u32 length (BE)            ; length excludes header
//   payload: u8 opcode | u16 tag_count (BE) | tags...
//   tag    : u16 name_wire (BE) = (name<<1)|has_children
//            u8  type
//            u32 length (BE) = data_len + serialized children
//            [ if has_children: u16 subtag_count (BE) | children... ]
//            data (data_len bytes)

#include "ec/packet.h"

#include <algorithm>
#include <cstring>
#include <utility>

#include <QtEndian>

#include "ec/codes.h"

namespace amule::ec {
namespace {

// --- Encoding --------------------------------------------------------------

template <typename T>
void appendBE(QByteArray& out, T v) {
    const T be = qToBigEndian(v);
    out.append(reinterpret_cast<const char*>(&be), sizeof(T));
}

// Encode an integer to its smallest fitting EC type and big-endian bytes.
std::pair<quint8, QByteArray> encodeInt(quint64 v) {
    QByteArray data;
    if (v <= 0xFFu) {
        data.append(static_cast<char>(v));
        return {EC_TAGTYPE_UINT8, data};
    }
    if (v <= 0xFFFFu) {
        appendBE<quint16>(data, static_cast<quint16>(v));
        return {EC_TAGTYPE_UINT16, data};
    }
    if (v <= 0xFFFF'FFFFull) {
        appendBE<quint32>(data, static_cast<quint32>(v));
        return {EC_TAGTYPE_UINT32, data};
    }
    appendBE<quint64>(data, v);
    return {EC_TAGTYPE_UINT64, data};
}

// (type byte, data bytes) for a tag value, excluding children.
std::pair<quint8, QByteArray> encodeValue(const TagValue& value) {
    if (std::holds_alternative<Empty>(value))
        return {EC_TAGTYPE_CUSTOM, QByteArray{}};
    if (const auto* b = std::get_if<QByteArray>(&value))
        return {EC_TAGTYPE_CUSTOM, *b};
    if (const auto* i = std::get_if<quint64>(&value))
        return encodeInt(*i);
    if (const auto* s = std::get_if<QString>(&value)) {
        QByteArray data = s->toUtf8();
        data.append('\0'); // null terminator is part of the stored data
        return {EC_TAGTYPE_STRING, data};
    }
    if (const auto* h = std::get_if<Hash16>(&value)) {
        return {EC_TAGTYPE_HASH16,
                QByteArray(reinterpret_cast<const char*>(h->data()), 16)};
    }
    if (const auto* a = std::get_if<Ipv4>(&value)) {
        QByteArray data(reinterpret_cast<const char*>(a->ip.data()), 4);
        appendBE<quint16>(data, a->port);
        return {EC_TAGTYPE_IPV4, data};
    }
    if (const auto* d = std::get_if<double>(&value)) {
        QByteArray data = QByteArray::number(*d);
        data.append('\0');
        return {EC_TAGTYPE_DOUBLE, data};
    }
    return {EC_TAGTYPE_CUSTOM, QByteArray{}};
}

// Value of the `length` field for this tag (data + serialized children),
// matching aMule's CECTag::GetTagLen. A child contributes its 7-byte header, an
// optional 2-byte subtag-count (counted in the parent), and its own tag length.
quint32 tagLen(const Tag& tag) {
    auto [type, data] = encodeValue(tag.value);
    Q_UNUSED(type);
    auto n = static_cast<quint32>(data.size());
    for (const Tag& c : tag.children) {
        n += 7; // child name(2) + type(1) + length(4)
        if (!c.children.empty())
            n += 2; // child's own subtag-count field
        n += tagLen(c);
    }
    return n;
}

void writeTag(QByteArray& out, const Tag& tag) {
    const bool hasChildren = !tag.children.empty();
    const auto nameWire =
        static_cast<quint16>((tag.name << 1) | (hasChildren ? 1u : 0u));
    auto [typeByte, data] = encodeValue(tag.value);

    appendBE<quint16>(out, nameWire);
    out.append(static_cast<char>(typeByte));
    appendBE<quint32>(out, tagLen(tag));

    if (hasChildren) {
        appendBE<quint16>(out, static_cast<quint16>(tag.children.size()));
        for (const Tag& c : tag.children)
            writeTag(out, c);
    }
    out.append(data);
}

// --- Decoding --------------------------------------------------------------

struct DecodeError {
    QString message;
};

class Cursor {
public:
    explicit Cursor(QByteArrayView b) : buf_(b) {}

    [[nodiscard]] qsizetype position() const { return pos_; }
    [[nodiscard]] qsizetype remaining() const { return buf_.size() - pos_; }

    QByteArrayView take(qsizetype n) {
        if (remaining() < n)
            throw DecodeError{QStringLiteral("EC payload truncated")};
        QByteArrayView s = buf_.sliced(pos_, n);
        pos_ += n;
        return s;
    }

    quint8 u8() { return static_cast<quint8>(take(1).front()); }
    quint16 u16() { return qFromBigEndian<quint16>(take(2).data()); }
    quint32 u32() { return qFromBigEndian<quint32>(take(4).data()); }

private:
    QByteArrayView buf_;
    qsizetype pos_ = 0;
};

TagValue decodeValue(quint8 type, QByteArrayView data) {
    const auto size = data.size();
    switch (type) {
    case EC_TAGTYPE_CUSTOM:
        if (data.isEmpty())
            return Empty{};
        return QByteArray(data.data(), size);
    case EC_TAGTYPE_UINT8:
        return static_cast<quint64>(size > 0 ? static_cast<quint8>(data[0]) : 0);
    case EC_TAGTYPE_UINT16: {
        quint8 b[2] = {0, 0};
        std::memcpy(b, data.data(), std::min<qsizetype>(size, 2));
        return static_cast<quint64>(qFromBigEndian<quint16>(b));
    }
    case EC_TAGTYPE_UINT32: {
        quint8 b[4] = {0, 0, 0, 0};
        std::memcpy(b, data.data(), std::min<qsizetype>(size, 4));
        return static_cast<quint64>(qFromBigEndian<quint32>(b));
    }
    case EC_TAGTYPE_UINT64: {
        quint8 b[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        std::memcpy(b, data.data(), std::min<qsizetype>(size, 8));
        return qFromBigEndian<quint64>(b);
    }
    case EC_TAGTYPE_STRING: {
        // Trailing null terminator is part of the data; drop it.
        qsizetype end = data.indexOf('\0');
        if (end < 0)
            end = size;
        return QString::fromUtf8(data.data(), end);
    }
    case EC_TAGTYPE_HASH16: {
        Hash16 h{};
        std::memcpy(h.data(), data.data(), std::min<qsizetype>(size, 16));
        return h;
    }
    case EC_TAGTYPE_IPV4: {
        Ipv4 addr{};
        std::memcpy(addr.ip.data(), data.data(), std::min<qsizetype>(size, 4));
        if (size >= 6)
            addr.port = qFromBigEndian<quint16>(data.sliced(4, 2).data());
        return addr;
    }
    case EC_TAGTYPE_DOUBLE: {
        qsizetype end = data.indexOf('\0');
        if (end < 0)
            end = size;
        return QByteArray(data.data(), end).trimmed().toDouble();
    }
    default:
        return QByteArray(data.data(), size);
    }
}

Tag readTag(Cursor& cur) {
    const quint16 nameWire = cur.u16();
    const auto name = static_cast<quint16>(nameWire >> 1);
    const bool hasChildren = (nameWire & 1) != 0;
    const quint8 type = cur.u8();
    const auto len = static_cast<qsizetype>(cur.u32());

    // The subtag-count u16 is charged to the parent's length, not ours, so we
    // start counting our children only after consuming it.
    std::vector<Tag> children;
    qsizetype bodyStart;
    if (hasChildren) {
        const quint16 count = cur.u16();
        bodyStart = cur.position();
        children.reserve(count);
        for (quint16 i = 0; i < count; ++i)
            children.push_back(readTag(cur));
    } else {
        bodyStart = cur.position();
    }

    const qsizetype childrenBytes = cur.position() - bodyStart;
    if (len < childrenBytes)
        throw DecodeError{QStringLiteral("EC tag length underflow")};
    const qsizetype dataLen = len - childrenBytes;
    const QByteArrayView data = cur.take(dataLen);

    Tag tag;
    tag.name = name;
    tag.value = decodeValue(type, data);
    tag.children = std::move(children);
    return tag;
}

} // namespace

QByteArray Packet::encode() const {
    QByteArray payload;
    payload.append(static_cast<char>(opcode));
    appendBE<quint16>(payload, static_cast<quint16>(tags.size()));
    for (const Tag& t : tags)
        writeTag(payload, t);

    QByteArray frame;
    frame.reserve(EC_HEADER_SIZE + payload.size());
    appendBE<quint32>(frame, EC_FLAG_BASE);
    appendBE<quint32>(frame, static_cast<quint32>(payload.size()));
    frame.append(payload);
    return frame;
}

std::expected<Packet, QString> Packet::decodePayload(QByteArrayView payload) {
    try {
        Cursor cur(payload);
        const quint8 opcode = cur.u8();
        const quint16 count = cur.u16();
        std::vector<Tag> tags;
        tags.reserve(count);
        for (quint16 i = 0; i < count; ++i)
            tags.push_back(readTag(cur));
        return Packet(opcode, std::move(tags));
    } catch (const DecodeError& e) {
        return std::unexpected(e.message);
    }
}

} // namespace amule::ec
