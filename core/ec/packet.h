// EC packet: an opcode plus a flat list of top-level tags, with on-the-wire
// (de)serialization in the simplest negotiated mode (no zlib, no FSS-UTF
// numbers, big-endian). See ADR-0005.

#pragma once

#include <expected>
#include <vector>

#include <QByteArray>
#include <QByteArrayView>
#include <QString>
#include <QtTypes>

#include "ec/tag.h"

namespace amule::ec {

class Packet {
public:
    quint8 opcode = 0;
    std::vector<Tag> tags{};

    Packet() = default;
    explicit Packet(quint8 op) : opcode(op) {}
    Packet(quint8 op, std::vector<Tag> packetTags)
        : opcode(op), tags(std::move(packetTags)) {}

    // First top-level tag with the given name, or nullptr.
    [[nodiscard]] const Tag* tag(quint16 name) const {
        for (const Tag& t : tags) {
            if (t.name == name)
                return &t;
        }
        return nullptr;
    }

    [[nodiscard]] std::optional<quint64> tagInt(quint16 name) const {
        if (const Tag* t = tag(name))
            return t->asInt();
        return std::nullopt;
    }

    [[nodiscard]] std::optional<QString> tagString(quint16 name) const {
        if (const Tag* t = tag(name))
            return t->asString();
        return std::nullopt;
    }

    // Serialize the full on-the-wire frame (8-byte header + payload).
    [[nodiscard]] QByteArray encode() const;

    // Decode a packet payload (already de-framed and decompressed). Returns an
    // error message if the buffer is truncated or malformed.
    [[nodiscard]] static std::expected<Packet, QString>
    decodePayload(QByteArrayView payload);

    friend bool operator==(const Packet&, const Packet&) = default;
};

} // namespace amule::ec
