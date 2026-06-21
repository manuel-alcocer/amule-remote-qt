// EC tag: a name, a typed value, and optional children.
//
// Ported from rstamule-cli's codec.rs (TagValue / Tag). See ADR-0005.

#pragma once

#include <array>
#include <optional>
#include <variant>
#include <vector>

#include <QByteArray>
#include <QString>
#include <QtTypes>

namespace amule::ec {

using Hash16 = std::array<quint8, 16>;

// Empty capability marker (CUSTOM tag type, zero-length data).
struct Empty {
    friend bool operator==(const Empty&, const Empty&) = default;
};

// IPv4 address + port (EC_TAGTYPE_IPV4).
struct Ipv4 {
    std::array<quint8, 4> ip{};
    quint16 port = 0;
    friend bool operator==(const Ipv4&, const Ipv4&) = default;
};

// A decoded EC tag value. The variant alternative selects the wire type:
//   Empty      -> CUSTOM, zero length (capability marker)
//   QByteArray -> CUSTOM with opaque data
//   quint64    -> integer, serialized as the smallest fitting UINT8/16/32/64
//   QString    -> null-terminated UTF-8 string
//   Hash16     -> 16-byte hash (MD4/MD5-sized digest)
//   Ipv4       -> IPv4 address + port
//   double     -> double, stored on the wire as a decimal ASCII string
using TagValue =
    std::variant<Empty, QByteArray, quint64, QString, Hash16, Ipv4, double>;

class Tag {
public:
    quint16 name = 0;
    TagValue value{Empty{}};
    std::vector<Tag> children{};

    Tag() = default;
    Tag(quint16 tagName, TagValue tagValue)
        : name(tagName), value(std::move(tagValue)) {}

    // Factory helpers mirroring the source prototype.
    static Tag integer(quint16 name, quint64 v) { return Tag(name, v); }
    static Tag string(quint16 name, QString v) { return Tag(name, std::move(v)); }
    static Tag empty(quint16 name) { return Tag(name, Empty{}); }
    static Tag hash(quint16 name, Hash16 h) { return Tag(name, h); }
    static Tag custom(quint16 name, QByteArray b) { return Tag(name, std::move(b)); }

    Tag& withChildren(std::vector<Tag> kids) {
        children = std::move(kids);
        return *this;
    }

    // First direct child with the given name, or nullptr.
    [[nodiscard]] const Tag* child(quint16 childName) const {
        for (const Tag& t : children) {
            if (t.name == childName)
                return &t;
        }
        return nullptr;
    }

    [[nodiscard]] std::optional<quint64> asInt() const {
        if (const auto* v = std::get_if<quint64>(&value))
            return *v;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<QString> asString() const {
        if (const auto* v = std::get_if<QString>(&value))
            return *v;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<Hash16> asHash() const {
        if (const auto* v = std::get_if<Hash16>(&value))
            return *v;
        return std::nullopt;
    }

    [[nodiscard]] std::optional<quint64> childInt(quint16 childName) const {
        if (const Tag* c = child(childName))
            return c->asInt();
        return std::nullopt;
    }

    [[nodiscard]] std::optional<QString> childString(quint16 childName) const {
        if (const Tag* c = child(childName))
            return c->asString();
        return std::nullopt;
    }

    friend bool operator==(const Tag&, const Tag&) = default;
};

} // namespace amule::ec
