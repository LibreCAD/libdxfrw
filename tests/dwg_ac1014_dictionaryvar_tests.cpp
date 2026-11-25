// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_ac1014_dictionaryvar_wire.h"
#include "dx_legacy_dwg_wire.h"
#include "dx_legacy_mlinestyle_wire.h"
#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {
class Variable final : public DRW_DictionaryVar {
public:
    using DRW_DictionaryVar::parseDwg;
};
class Style final : public DRW_MLineStyle {
public:
    using DRW_MLineStyle::parseDwg;
};
class Dictionary final : public DRW_Dictionary {
public:
    using DRW_Dictionary::parseDwg;
};
class DictionaryWithDefault final : public DRW_DictionaryWithDefault {
public:
    using DRW_DictionaryWithDefault::parseDwg;
};

bool sharedLegacyFrameControl() {
    using namespace dx_legacy_dwg_wire;
    const auto bytesFor = [](DRW::Version version, std::uint32_t end,
                             unsigned ownerRole, unsigned reactorRole,
                             unsigned extensionRole, unsigned ownerLength = 1u,
                             std::uint32_t ownerValue = 0x20u,
                             unsigned reactorLength = 1u,
                             std::uint32_t reactorValue = 0x21u) {
        dwgBufferW w;
        w.putObjType(version, 73u);
        if (version == DRW::AC1015) w.putRawLong32(end);
        w.putFixedHandle(0u, 1u, 0x31u);
        w.putBitShort(0u);
        if (version != DRW::AC1015) w.putRawLong32(end);
        w.putBitLong(1u);
        w.putFixedHandle(ownerRole, ownerLength, ownerValue);
        w.putFixedHandle(reactorRole, reactorLength, reactorValue);
        w.putFixedHandle(extensionRole, 0u, 0u);
        w.put2Bits(0u);
        return w.data();
    };
    const auto check = [](const std::vector<std::uint8_t>& bytes, DRW::Version version,
                          std::uint32_t owner = 0x20u) {
        Bits bits{bytes.data(), 0u, static_cast<std::uint32_t>(bytes.size() * 8u)};
        std::uint32_t bodyEnd = 0u;
        const auto noEed = [](Bits& b) noexcept { return b.shortValue() == 0u && b.good; };
        return frameStart(bits, version, 73u, 0x31u, 1u, noEed, bodyEnd)
            && bits.pos == bodyEnd
            && commonTail(bits, 0x31u, owner, std::vector<std::uint32_t>{0x21u}, 0u)
            && padding(bits);
    };
    const auto rejectsRelativeReactor = [&](DRW::Version version, unsigned role,
                                            unsigned length, std::uint32_t offset,
                                            std::uint32_t target) {
        const auto bytes = bytesFor(version, 70u, 4u, role, 3u,
                                    1u, 0x20u, length, offset);
        Bits bits{bytes.data(), 0u, static_cast<std::uint32_t>(bytes.size() * 8u)};
        std::uint32_t bodyEnd = 0u;
        const auto noEed = [](Bits& b) noexcept { return b.shortValue() == 0u && b.good; };
        if (!frameStart(bits, version, 73u, 0x31u, 1u, noEed, bodyEnd)
            || bits.pos > bodyEnd || !bits.skip(bodyEnd - bits.pos)) return false;
        auto decoded = bits;
        const auto parent = handle(decoded, 4u, 0x31u);
        const auto reactor = handle(decoded, 4u, 0x31u);
        if (!decoded.good || parent.value != 0x20u || reactor.role != role
            || reactor.value != target) return false;
        return !commonTail(bits, 0x31u, 0x20u,
                           std::vector<std::uint32_t>{target}, 0u);
    };
    for (const auto version : {DRW::AC1012, DRW::AC1014, DRW::AC1015}) {
        const auto valid = bytesFor(version, 70u, 4u, 4u, 3u);
        if (!check(valid, version) || check(bytesFor(version, 69u, 4u, 4u, 3u), version)
            || check(bytesFor(version, 113u, 4u, 4u, 3u), version)
            || check(valid, version == DRW::AC1015 ? DRW::AC1014 : DRW::AC1015)
            || !check(valid, version)) return false;
        for (const auto& relative : {
                 std::array<std::uint32_t, 3>{{6u, 0u, 0x32u}},
                 std::array<std::uint32_t, 3>{{8u, 0u, 0x30u}},
                 std::array<std::uint32_t, 3>{{10u, 1u, 0x32u}},
                 std::array<std::uint32_t, 3>{{12u, 1u, 0x30u}}}) {
            const auto offset = relative[1] == 0u ? 0u : 1u;
            const auto relativeOwner = bytesFor(version, 70u, relative[0], 4u, 3u,
                                                relative[1], offset, 1u, 0x21u);
            if (!check(relativeOwner, version, relative[2])
                || !rejectsRelativeReactor(version, relative[0], relative[1],
                                           offset, relative[2])) return false;
        }
        // The common-tail helper is shared by the legacy DICTIONARY and
        // MLINESTYLE source observers. Check every handle-role nibble at each
        // position, including the typed-null extension-dictionary handle.
        const std::size_t roleBitStarts[] = {70u, 86u, 102u};
        const unsigned expectedRoles[] = {4u, 4u, 3u};
        for (std::size_t field = 0; field < 3u; ++field) {
            for (unsigned role = 0u; role < 16u; ++role) {
                auto changed = valid;
                for (unsigned bit = 0u; bit < 4u; ++bit) {
                    const auto position = roleBitStarts[field] + bit;
                    const auto mask = static_cast<std::uint8_t>(0x80u >> (position % 8u));
                    const auto roleMask = 1u << (3u - bit);
                    if ((role & roleMask) != 0u)
                        changed[position / 8u] |= mask;
                    else
                        changed[position / 8u] &= static_cast<std::uint8_t>(~mask);
                }
                if (check(changed, version) != (role == expectedRoles[field])
                    || !check(valid, version)) return false;
            }
            if (field < 2u) {
                auto wrongTarget = valid;
                const auto position = roleBitStarts[field] + 8u;
                wrongTarget[position / 8u] ^=
                    static_cast<std::uint8_t>(0x80u >> (position % 8u));
                if (check(wrongTarget, version) || !check(valid, version))
                    return false;
            }
        }
        for (std::size_t length = 0; length < valid.size(); ++length) {
            const std::vector<std::uint8_t> truncated(valid.begin(), valid.begin() + length);
            if (check(truncated, version) || !check(valid, version)) return false;
        }
    }
    return true;
}

bool eedLayerWriteOrderControl() {
    const auto little = DRW::DwgEedLayerByteOrder::LegacyLittleEndian;
    const auto big = DRW::DwgEedLayerByteOrder::BigEndian;
    dwgBufferW staged;
    staged.setEedLayerByteOrder(big);
    dwgBufferW copy = staged;
    copy.reset();
    dwgBufferW other;
    copy.swap(other);
    if (other.eedLayerByteOrder() != big || copy.eedLayerByteOrder() != little) return false;
    DRW_Variant app(1001, std::string("ACAD"));
    DRW_Variant layer(1003, std::string("0"));
    const std::vector<DRW_Variant*> data{&app, &layer};
    const std::vector<DRW_Entity::PendingHandleRef> apps{{0u, 0x14u}};
    for (const auto order : {little, big}) {
        for (const bool raw : {false, true}) {
            const std::uint64_t reference = raw ? UINT64_C(0x0102030405060708) : UINT64_C(0x12345678);
            const std::vector<DRW_Entity::PendingHandleRef> refs{
                {1u, static_cast<std::uint32_t>(reference), reference, raw}};
            dwgBufferW output;
            output.setEedLayerByteOrder(order);
            output.putRawChar8(0xa5u);
            const std::vector<DRW_Entity::PendingHandleRef> missing{{1u, 0u}};
            if (DRW_Entity::encodeDwgEed(DRW::AC1015, data, apps, missing, 30u, output)
                || output.data() != std::vector<std::uint8_t>{0xa5u}
                || output.eedLayerByteOrder() != order) return false;
            if (!DRW_Entity::encodeDwgEed(DRW::AC1015, data, apps, refs, 30u, output)) return false;
            output.putRawChar8(0x5au);
            dwgBuffer input(output.data().data(), output.data().size());
            if (input.getRawChar8() != 0xa5u || input.getBitShort() != 9u) return false;
            const auto appHandle = input.getHandle();
            if (appHandle.code != 5u || appHandle.ref != 0x14u || input.getRawChar8() != 3u) return false;
            for (unsigned index = 0u; index < 8u; ++index) {
                const bool useBig = !raw && order == big;
                const auto shift = 8u * (useBig ? 7u - index : index);
                if (input.getRawChar8() != static_cast<std::uint8_t>(reference >> shift)) return false;
            }
            if (input.getBitShort() != 0u || input.getRawChar8() != 0x5au || !input.isGood()) return false;
        }
    }
    return true;
}

bool legacyDictionaryWithDefaultControl() {
    // Physical object bodies from locally generated ODA R13/R14 files. Unlike
    // plain DICTIONARY, these observed class bodies carry cloning and owner.
    struct Control { DRW::Version version; std::vector<std::uint8_t> bytes; std::size_t defaultRoleBit; };
    const Control controls[] = {
        {DRW::AC1012, {0x3du,0x00u,0x40u,0x44u,0x91u,0x45u,0x12u,0xa0u,0x00u,0xd0u,0x02u,0xd5u,0x45u,0x24u,0x54u,0x15u,0x45u,0xf4u,0x15u,0x35u,0xf4u,0x84u,0x15u,0x24u,0x44u,0x60u,0x00u,0x09u,0x58u,0x04u,0x00u,0x02u,0x40u,0x50u,0x10u,0x04u,0x19u,0x39u,0x3du,0x49u,0x35u,0x05u,0x31u,0x04u,0x28u,0xc6u,0xdcu,0x84u,0x4du,0x44u,0x4cu}, 390u},
        {DRW::AC1014, {0x3du,0x00u,0x40u,0x44u,0xa9u,0xc0u,0x00u,0x00u,0x09u,0x01u,0x40u,0x40u,0x10u,0x64u,0xe4u,0xf5u,0x24u,0xd4u,0x14u,0xc4u,0x10u,0xa3u,0x1bu,0xd2u,0x11u,0x35u,0x11u,0x30u}, 204u}
    };
    for (const auto& control : controls) {
        DictionaryWithDefault parsed;
        const auto read = [&](const std::vector<std::uint8_t>& bytes) {
            dwgBuffer input(bytes.data(), bytes.size());
            return parsed.parseDwg(control.version, &input, 0u) && input.isGood();
        };
        const auto modelMatches = [&]() {
            return parsed.hasCompleteDwgEntries() && parsed.handle == 0x12u
                && !parsed.legacyDwgCloningAbsent()
                && parsed.parentHandle == 0xau && parsed.cloning == 1 && parsed.hardOwner == 0
                && parsed.m_defaultEntryHandle == 0x13u && parsed.m_entries.size() == 1u
                && parsed.m_entries[0].m_name == "NORMAL" && parsed.m_entries[0].m_handle == 0x13u;
        };
        if (!read(control.bytes) || !modelMatches()) return false;
        for (std::size_t offset : {48u, 32u, 16u, 0u}) {
            auto wrongRole = control.bytes;
            const std::size_t roleBit = control.defaultRoleBit - offset + 3u;
            wrongRole[roleBit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (roleBit % 8u));
            if (read(wrongRole) || parsed.hasCompleteDwgEntries() || !parsed.m_entries.empty()
                || parsed.m_defaultEntryHandle != 0u || !read(control.bytes) || !modelMatches()) return false;
        }
        auto suffix = control.bytes;
        suffix.push_back(0u);
        if (read(suffix) || parsed.hasCompleteDwgEntries() || !parsed.m_entries.empty()
            || parsed.m_defaultEntryHandle != 0u) return false;
        for (std::size_t length = 0; length < control.bytes.size(); ++length) {
            const std::vector<std::uint8_t> truncated(control.bytes.begin(), control.bytes.begin() + length);
            if (read(truncated) || parsed.hasCompleteDwgEntries() || !parsed.m_entries.empty()
                || parsed.m_defaultEntryHandle != 0u) return false;
        }
        if (!read(control.bytes) || !modelMatches()) return false;
    }
    return true;
}

void putText(dwgBufferW& w, const std::string& text) {
    w.putBitShort(static_cast<std::uint16_t>(text.size()));
    for (const unsigned char c : text) w.putRawChar8(c);
}

std::uint32_t writtenBits(const dwgBufferW& w) {
    return static_cast<std::uint32_t>(w.bitPos() == 0u
        ? w.size() * 8u : (w.size() - 1u) * 8u + w.bitPos());
}

void setNibble(std::vector<std::uint8_t>& bytes, std::size_t start,
               unsigned value) {
    for (unsigned bit = 0u; bit < 4u; ++bit) {
        const auto position = start + bit;
        const auto mask = static_cast<std::uint8_t>(0x80u >> (position % 8u));
        const auto valueMask = 1u << (3u - bit);
        if ((value & valueMask) != 0u) bytes[position / 8u] |= mask;
        else bytes[position / 8u] &= static_cast<std::uint8_t>(~mask);
    }
}

void setRawLong32Bits(std::vector<std::uint8_t>& bytes, std::size_t start,
                      std::uint32_t value) {
    for (unsigned byte = 0u; byte < 4u; ++byte) {
        const auto octet = static_cast<std::uint8_t>(value >> (8u * byte));
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            const auto position = start + byte * 8u + bit;
            const auto mask = static_cast<std::uint8_t>(0x80u >> (position % 8u));
            if ((octet & (0x80u >> bit)) != 0u) bytes[position / 8u] |= mask;
            else bytes[position / 8u] &= static_cast<std::uint8_t>(~mask);
        }
    }
}

bool generatedAc1012DictionaryFrameControl() {
    const auto reject = [](const char* stage) {
        std::cerr << "generated AC1012 DICTIONARY control failed: " << stage << '\n';
        return false;
    };
    constexpr std::uint32_t dictionaryHandle = 0x30u;
    constexpr std::uint32_t ownerHandle = 0x31u;
    constexpr std::uint32_t reactorHandle = 0x32u;
    const std::vector<std::pair<std::string, std::uint32_t>> entries{
        {"Standard", 0x34u}, {"SAME_KEY", 0x35u}};
    struct WireHandle {
        std::uint8_t role;
        std::uint8_t size;
        std::uint64_t value;
    };

    // Build the bounded R13 DICTIONARY layout from the ODA field order. The
    // relative soft-pointer parent is a valid offset handle; files are never
    // written to the repository or treated as producer/native evidence.
    const auto makeFrame = [&](const WireHandle& parent,
                               const WireHandle& reactor,
                               const WireHandle& xdictionary,
                               const std::vector<WireHandle>& itemHandles) {
        dwgBufferW prefix;
        prefix.putObjType(DRW::AC1012, DRW_Dictionary::kDwgFixedType);
        prefix.putFixedHandle(0u, 1u, dictionaryHandle);
        prefix.putBitShort(0u); // no EED

        dwgBufferW body;
        body.putBitLong(1u); // Numreactors
        body.putBitLong(static_cast<std::int32_t>(entries.size()));
        for (const auto& entry : entries) putText(body, entry.first);
        const auto bodyEnd = writtenBits(prefix) + 32u + writtenBits(body);

        dwgBufferW writer;
        writer.putObjType(DRW::AC1012, DRW_Dictionary::kDwgFixedType);
        writer.putFixedHandle(0u, 1u, dictionaryHandle);
        writer.putBitShort(0u);
        writer.putRawLong32(bodyEnd);
        writer.putBitLong(1u);
        writer.putBitLong(static_cast<std::int32_t>(entries.size()));
        for (const auto& entry : entries) putText(writer, entry.first);
        if (writtenBits(writer) != bodyEnd) return std::vector<std::uint8_t>{};
        writer.putFixedHandle(parent.role, parent.size, parent.value);
        writer.putFixedHandle(reactor.role, reactor.size, reactor.value);
        writer.putFixedHandle(xdictionary.role, xdictionary.size,
                              xdictionary.value);
        for (const auto& item : itemHandles)
            writer.putFixedHandle(item.role, item.size, item.value);
        while (writer.bitPos() != 0u) writer.putBit(0u);
        return writer.isGood() ? writer.data() : std::vector<std::uint8_t>{};
    };
    const WireHandle parent{6u, 0u, 0u}; // relative soft pointer: handle + 1
    const WireHandle reactor{4u, 1u, reactorHandle};
    const WireHandle xdictionary{3u, 0u, 0u}; // typed-null hard owner
    const std::vector<WireHandle> itemHandles{
        {2u, 1u, entries[0].second}, {2u, 1u, entries[1].second}};
    const auto bytes = makeFrame(parent, reactor, xdictionary, itemHandles);
    if (bytes.empty()) return reject("writer state");
    dwgBufferW boundaryProbe;
    boundaryProbe.putObjType(DRW::AC1012, DRW_Dictionary::kDwgFixedType);
    boundaryProbe.putFixedHandle(0u, 1u, dictionaryHandle);
    boundaryProbe.putBitShort(0u);
    const std::size_t bodyEndFieldBit = writtenBits(boundaryProbe);

    Dictionary model;
    const auto parse = [&model](const std::vector<std::uint8_t>& source) {
        dwgBuffer input(source.data(), source.size());
        return model.parseDwg(DRW::AC1012, &input, 0u) && input.isGood();
    };
    const auto matches = [&]() {
        if (model.handle != dictionaryHandle || model.parentHandle != ownerHandle
            || model.reactorHandles != std::vector<std::uint32_t>{reactorHandle}
            || model.xDictHandle != 0u || !model.legacyDwgCloningAbsent()
            || !model.hasCompleteDwgEntries() || model.m_entries.size() != entries.size())
            return false;
        for (std::size_t i = 0u; i < entries.size(); ++i)
            if (model.m_entries[i].m_name != entries[i].first
                || model.m_entries[i].m_handle != entries[i].second) return false;
        return true;
    };
    if (!parse(bytes)) return reject("typed parser");
    if (!matches()) {
        std::cerr << "AC1012 model: h=" << model.handle << " p=" << model.parentHandle
                  << " r=" << model.reactorCount() << " x=" << model.xDictHandle
                  << " entries=" << model.m_entries.size() << " complete="
                  << model.hasCompleteDwgEntries() << " absent="
                  << model.legacyDwgCloningAbsent() << '\n';
        for (const auto& entry : model.m_entries)
            std::cerr << "  " << entry.m_name << " -> " << entry.m_handle << '\n';
        return reject("decoded model");
    }

    auto missingBodyBoundary = bytes;
    setRawLong32Bits(missingBodyBoundary, bodyEndFieldBit, 0u);
    if (parse(missingBodyBoundary) || model.hasCompleteDwgEntries()
        || !model.m_entries.empty() || !parse(bytes) || !matches())
        return reject("missing legacy object-data boundary or corrected retry");

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1012;
    raw.m_handle = model.handle;
    raw.m_objectType = DRW_Dictionary::kDwgFixedType;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
    raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle;
    raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles;
    raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    if (!dx_legacy_dwg_wire::dictionary(raw, model, false))
        return reject("shared dictionary observer");

    using dx_legacy_dwg_wire::Bits;
    Bits bits{bytes.data(), 0u, static_cast<std::uint32_t>(bytes.size() * 8u)};
    std::uint32_t parsedBodyEnd = 0u;
    const auto noEed = [](Bits& source) noexcept {
        return source.shortValue() == 0u && source.good;
    };
    if (!dx_legacy_dwg_wire::frameStart(bits, DRW::AC1012,
            DRW_Dictionary::kDwgFixedType, dictionaryHandle, 1u,
            noEed, parsedBodyEnd)
        || bits.longValue() != entries.size()) return reject("independent frame start");
    for (const auto& entry : entries)
        if (!dx_legacy_dwg_wire::text(bits, entry.first)) return reject("independent dictionary name");
    if (bits.pos != parsedBodyEnd) return reject("independent body boundary");

    std::vector<std::pair<std::size_t, unsigned>> handles;
    handles.emplace_back(bits.pos, 2u); // parent must be a soft pointer
    if (!dx_legacy_dwg_wire::reference(bits, 4u, dictionaryHandle, ownerHandle))
        return reject("parent soft pointer");
    handles.emplace_back(bits.pos, 2u); // reactor must be a soft pointer
    if (!dx_legacy_dwg_wire::reference(bits, 4u, dictionaryHandle, reactorHandle))
        return reject("reactor soft pointer");
    handles.emplace_back(bits.pos, 4u); // extension dictionary is hard-owner typed null
    if (!dx_legacy_dwg_wire::reference(bits, 3u, dictionaryHandle, 0u))
        return reject("typed-null extension dictionary");
    for (const auto& entry : entries) {
        handles.emplace_back(bits.pos, 3u); // dictionary items are soft-owner handles
        if (!dx_legacy_dwg_wire::reference(bits, 2u, dictionaryHandle, entry.second))
            return reject("item soft owner");
    }
    if (!dx_legacy_dwg_wire::padding(bits)) return reject("frame padding");

    for (const auto& [start, invalidRole] : handles) {
        auto malformed = bytes;
        setNibble(malformed, start, invalidRole);
        auto malformedRaw = raw;
        malformedRaw.m_rawBytes = malformed;
        if (dx_legacy_dwg_wire::dictionary(malformedRaw, model, false)
            || parse(malformed) || model.hasCompleteDwgEntries()
            || !model.m_entries.empty() || !parse(bytes) || !matches())
            return reject("role refusal or corrected retry");
    }
    for (std::size_t length = 0u; length < bytes.size(); ++length) {
        const std::vector<std::uint8_t> truncated(bytes.begin(), bytes.begin() + length);
        if (parse(truncated) || model.hasCompleteDwgEntries()
            || !model.m_entries.empty() || !parse(bytes) || !matches())
            return reject("truncation refusal or corrected retry");
    }
    const std::uint64_t wideHandle = UINT64_C(0x100000001);
    const std::vector<std::vector<std::uint8_t>> wideHandleFrames{
        makeFrame({4u, 5u, wideHandle}, reactor, xdictionary, itemHandles),
        makeFrame(parent, {4u, 5u, wideHandle}, xdictionary, itemHandles),
        makeFrame(parent, reactor, {3u, 5u, wideHandle}, itemHandles),
        makeFrame(parent, reactor, xdictionary,
                  {{2u, 5u, wideHandle}, itemHandles[1]})};
    for (const auto& malformed : wideHandleFrames) {
        if (malformed.empty() || parse(malformed)
            || model.hasCompleteDwgEntries() || !model.m_entries.empty()
            || !parse(bytes) || !matches())
            return reject("unrepresentable handle refusal or corrected retry");
    }
    return true;
}

void setLittleEndianDoubleBits(std::vector<std::uint8_t>& bytes, std::size_t start,
                               std::uint64_t value) {
    for (unsigned byte = 0u; byte < 8u; ++byte) {
        const auto octet = static_cast<std::uint8_t>(value >> (8u * byte));
        for (unsigned bit = 0u; bit < 8u; ++bit) {
            const auto pos = start + byte * 8u + bit;
            const auto mask = static_cast<std::uint8_t>(0x80u >> (pos % 8u));
            if ((octet & (0x80u >> bit)) != 0u) bytes[pos / 8u] |= mask;
            else bytes[pos / 8u] &= static_cast<std::uint8_t>(~mask);
        }
    }
}

bool r13DictionaryEedControl(bool utf8 = false, bool longInts = false, bool reals = false,
                             bool groups = false, bool binaryEed = false, bool pointEed = false, bool handleEed = false) {
    // ODA-produced scratch source, re-encoded from its observed fields. The
    // caller must bind APPID and codepage; this is not an output proof.
    dwgBufferW w;
    w.putObjType(DRW::AC1012, 42u);
    w.putFixedHandle(0u, 1u, 0x30u);
    const std::string value = utf8 ? "caf\xC3\xA9" : "TREAT_AS_HARD";
    w.putBitShort(static_cast<std::uint16_t>(value.size() + 7u + (longInts ? 15u : 0u) + (reals ? 27u : 0u) + (groups ? 4u : 0u) + (binaryEed ? 130u : 0u) + (pointEed ? 25u : 0u) + (handleEed ? 9u : 0u)));
    w.putFixedHandle(5u, 1u, 0x2au);
    w.putRawChar8(0u);
    w.putRawChar8(static_cast<std::uint8_t>(value.size()));
    w.putRawChar8(0u);
    w.putRawChar8(45u);
    const std::size_t textBit = w.bitPos() == 0u ? w.size() * 8u
        : (w.size() - 1u) * 8u + w.bitPos();
    for (const unsigned char c : value) w.putRawChar8(c);
    w.putRawChar8(70u);
    w.putRawChar8(0u);
    w.putRawChar8(0u);
    if (longInts) {
        for (std::uint32_t number : {123456u, 0x80000000u, 0x7fffffffu}) {
            w.putRawChar8(71u);
            w.putRawLong32(number);
        }
    }
    std::size_t firstRealBit = 0u;
    if (reals) {
        for (const auto& item : {std::pair<std::uint8_t, double>{40u, 1.25},
                                {41u, -0.5}, {42u, 42.125}}) {
            w.putRawChar8(item.first);
            if (item.first == 40u)
                firstRealBit = w.bitPos() == 0u ? w.size() * 8u
                    : (w.size() - 1u) * 8u + w.bitPos();
            w.putRawDouble(item.second);
        }
    }
    std::size_t firstGroupValueBit = 0u;
    if (groups) {
        w.putRawChar8(2u);
        firstGroupValueBit = w.bitPos() == 0u ? w.size() * 8u
            : (w.size() - 1u) * 8u + w.bitPos();
        w.putRawChar8(0u);
        w.putRawChar8(2u);
        w.putRawChar8(1u);
    }
    if (binaryEed) {
        w.putRawChar8(4u);
        w.putRawChar8(128u);
        for (unsigned n = 0u; n < 128u; ++n) w.putRawChar8(static_cast<std::uint8_t>(n));
    }
    std::size_t firstPointBit = 0u;
    if (pointEed) {
        w.putRawChar8(10u);
        firstPointBit = w.bitPos() == 0u ? w.size() * 8u
            : (w.size() - 1u) * 8u + w.bitPos();
        w.putRawDouble(1.25);
        w.putRawDouble(-2.5);
        w.putRawDouble(3.75);
    }
    if (handleEed) {
        w.putRawChar8(5u);
        for (unsigned n = 0u; n < 7u; ++n) w.putRawChar8(0u);
        w.putRawChar8(0x2eu);
    }
    w.putBitShort(0u);
    w.putRawLong32((utf8 ? 186u : 250u) + (longInts ? 120u : 0u)
                   + (reals ? 216u : 0u) + (groups ? 32u : 0u) + (binaryEed ? 1040u : 0u) + (pointEed ? 200u : 0u) + (handleEed ? 72u : 0u));
    w.putBitLong(0);
    w.putBitLong(0);
    w.putFixedHandle(4u, 1u, 0xau);
    w.putFixedHandle(3u, 0u, 0u);
    for (int i = 0; i < 6; ++i) w.putBit(0u);
    const auto bytes = w.data();
    dwgBuffer input(bytes.data(), bytes.size());
    Dictionary model;
    if (bytes.size() != (utf8 ? 27u : 35u) + (longInts ? 15u : 0u) + (reals ? 27u : 0u) + (groups ? 4u : 0u) + (binaryEed ? 130u : 0u) + (pointEed ? 25u : 0u) + (handleEed ? 9u : 0u) || !model.parseDwg(DRW::AC1012, &input, 0u)
        || !input.isGood() || model.extData.size() != 3u + (longInts ? 3u : 0u) + (reals ? 3u : 0u) + (groups ? 2u : 0u) + (binaryEed ? 1u : 0u) + (pointEed ? 1u : 0u) + (handleEed ? 1u : 0u)
        || model.pendingAppIdResolutions.size() != 1u) return false;
    model.extData[0]->addString(1001, "ACAD");
    model.pendingAppIdResolutions.clear();
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1012;
    raw.m_handle = model.handle;
    raw.m_objectType = 42;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
    raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle;
    raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles;
    raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    const auto app = [](std::uint32_t h, const std::string& name) noexcept {
        return h == 0x2au && name == "ACAD";
    };
    const auto page = [](std::uint16_t be, std::uint16_t le) noexcept {
        return be == 45u && le == 0x2d00u;
    };
    const auto check = [&](const DRW_UnsupportedObject& source) {
        return dx_legacy_dwg_wire::observeDictionaryEed(source, model, false, app, page);
    };
    if (!check(raw) || dx_legacy_dwg_wire::dictionary(raw, model, false)
        || dx_legacy_dwg_wire::observeDictionaryEed(raw, model, false,
            [](std::uint32_t, const std::string&) noexcept { return false; }, page)
        || dx_legacy_dwg_wire::observeDictionaryEed(raw, model, false, app,
            [](std::uint16_t, std::uint16_t) noexcept { return false; })) return false;
    for (std::size_t bit = 0; bit < bytes.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (check(changed) != (bit >= bytes.size() * 8u - 6u)) return false;
    }
    for (std::size_t size = 0; size < bytes.size(); ++size) {
        auto truncated = raw;
        truncated.m_rawBytes.resize(size);
        truncated.m_objectSize = static_cast<std::uint32_t>(size);
        if (check(truncated)) return false;
    }
    if (utf8) {
        auto malformed = raw;
        // Change C3 to FF in both model and wire. Matching bytes alone must
        // not make an invalid UTF-8 scalar sequence acceptable.
        const std::size_t invalidByte = textBit + 3u * 8u;
        for (unsigned int bit = 0; bit < 8u; ++bit) {
            if ((0x3cu & (0x80u >> bit)) != 0u) {
                const std::size_t pos = invalidByte + bit;
                malformed.m_rawBytes[pos / 8u] ^= static_cast<std::uint8_t>(0x80u >> (pos % 8u));
            }
        }
        model.extData[1]->addString(1000, std::string("caf\xff\xa9", 5u));
        const bool invalidAccepted = check(malformed);
        model.extData[1]->addString(1000, value);
        if (invalidAccepted || !check(raw)) return false;
    }
    if (longInts) {
        if (model.extData[3]->i_val() != 123456
            || model.extData[4]->i_val() != (std::numeric_limits<std::int32_t>::min)()
            || model.extData[5]->i_val() != (std::numeric_limits<std::int32_t>::max)()) return false;
        model.extData[4]->addInt(1071, 0);
        const bool changedAccepted = check(raw);
        model.extData[4]->addInt(1071, (std::numeric_limits<std::int32_t>::min)());
        if (changedAccepted || !check(raw)) return false;
    }
    if (reals) {
        const std::size_t first = 3u + (longInts ? 3u : 0u);
        if (model.extData[first]->d_val() != 1.25 || model.extData[first + 1u]->d_val() != -0.5
            || model.extData[first + 2u]->d_val() != 42.125) return false;
        model.extData[first + 1u]->addDouble(1041, 0.5);
        const bool changedAccepted = check(raw);
        model.extData[first + 1u]->addDouble(1041, -0.5);
        if (changedAccepted || !check(raw)) return false;
        auto nonFinite = raw;
        const std::uint64_t nanBits = UINT64_C(0x7ff8000000000000);
        setLittleEndianDoubleBits(nonFinite.m_rawBytes, firstRealBit, nanBits);
        double nan = 0.0;
        std::memcpy(&nan, &nanBits, sizeof nan);
        model.extData[first]->addDouble(1040, nan);
        const bool nanAccepted = check(nonFinite);
        model.extData[first]->addDouble(1040, 1.25);
        if (nanAccepted || !check(raw)) return false;
    }
    if (groups) {
        const std::size_t first = 3u + (longInts ? 3u : 0u) + (reals ? 3u : 0u);
        if (!model.extData[first]->content.s || *model.extData[first]->content.s != "{"
            || !model.extData[first + 1u]->content.s || *model.extData[first + 1u]->content.s != "}") return false;
        auto reversed = raw;
        for (const auto bit : {firstGroupValueBit + 7u, firstGroupValueBit + 23u})
            reversed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        model.extData[first]->addString(1002, "}");
        model.extData[first + 1u]->addString(1002, "{");
        const bool reversedAccepted = check(reversed);
        model.extData[first]->addString(1002, "{");
        model.extData[first + 1u]->addString(1002, "}");
        if (reversedAccepted || !check(raw)) return false;
    }
    if (binaryEed) {
        const std::size_t index = 3u + (longInts ? 3u : 0u) + (reals ? 3u : 0u) + (groups ? 2u : 0u);
        if (!model.extData[index]->binary() || model.extData[index]->binary()->size() != 128u) return false;
        const auto original = *model.extData[index]->binary();
        auto changed = original;
        changed[127] ^= 1u;
        model.extData[index]->addBinary(1004, changed);
        const bool changedAccepted = check(raw);
        model.extData[index]->addBinary(1004, original);
        if (changedAccepted || !check(raw)) return false;
    }
    if (pointEed) {
        const std::size_t index = 3u + (longInts ? 3u : 0u) + (reals ? 3u : 0u)
            + (groups ? 2u : 0u) + (binaryEed ? 1u : 0u);
        const auto* point = model.extData[index]->coord();
        if (!point || point->x != 1.25 || point->y != -2.5 || point->z != 3.75) return false;
        const DRW_Coord original = *point;
        model.extData[index]->addCoord(1010, {1.25, -2.5, 4.75});
        const bool changedAccepted = check(raw);
        model.extData[index]->addCoord(1010, original);
        if (changedAccepted || !check(raw)) return false;
        auto nonFinite = raw;
        const std::uint64_t nanBits = UINT64_C(0x7ff8000000000000);
        setLittleEndianDoubleBits(nonFinite.m_rawBytes, firstPointBit, nanBits);
        double nan = 0.0;
        std::memcpy(&nan, &nanBits, sizeof nan);
        model.extData[index]->addCoord(1010, {nan, original.y, original.z});
        const bool nonFiniteAccepted = check(nonFinite);
        model.extData[index]->addCoord(1010, original);
        if (nonFiniteAccepted || !check(raw)) return false;
    }
    if (handleEed) {
        const std::size_t index = 3u + (longInts ? 3u : 0u) + (reals ? 3u : 0u)
            + (groups ? 2u : 0u) + (binaryEed ? 1u : 0u) + (pointEed ? 1u : 0u);
        if (model.extData[index]->type() != DRW_Variant::STRING
            || !model.extData[index]->content.s || *model.extData[index]->content.s != "2E") return false;
        for (const auto* invalid : {"0", "2F", "2Ex", "100000000", ""}) {
            model.extData[index]->addString(1005, invalid);
            if (check(raw)) return false;
        }
        model.extData[index]->addString(1005, "2E");
        if (!check(raw)) return false;
    }
    model.pendingAppIdResolutions.push_back({});
    const bool unresolvedAccepted = check(raw);
    model.pendingAppIdResolutions.clear();
    if (unresolvedAccepted || !check(raw)) return false;
    model.extData[2]->addInt(1070, 1);
    const bool wrongValueAccepted = check(raw);
    model.extData[2]->addInt(1070, 0);
    return !wrongValueAccepted && check(raw);
}

bool r13EedChunkGroupBoundary(bool crossed) {
    dwgBufferW w;
    w.putObjType(DRW::AC1012, 42u);
    w.putFixedHandle(0u, 1u, 0x30u);
    for (unsigned chunk = 0u; chunk < 2u; ++chunk) {
        w.putBitShort(crossed ? 2u : 4u);
        w.putFixedHandle(5u, 1u, chunk == 0u ? 0x2au : 0x2bu);
        if (!crossed || chunk == 0u) { w.putRawChar8(2u); w.putRawChar8(0u); }
        if (!crossed || chunk == 1u) { w.putRawChar8(2u); w.putRawChar8(1u); }
    }
    w.putBitShort(0u);
    const auto bit = w.bitPos() == 0u ? w.size() * 8u : (w.size() - 1u) * 8u + w.bitPos();
    w.putRawLong32(static_cast<std::uint32_t>(bit + 36u));
    w.putBitLong(0);
    w.putBitLong(0);
    w.putFixedHandle(4u, 1u, 0xau);
    w.putFixedHandle(3u, 0u, 0u);
    while (w.bitPos() != 0u) w.putBit(0u);
    const auto bytes = w.data();
    dwgBuffer input(bytes.data(), bytes.size());
    Dictionary model;
    if (!model.parseDwg(DRW::AC1012, &input, 0u) || !input.isGood()
        || model.extData.size() != (crossed ? 4u : 6u)
        || model.pendingAppIdResolutions.size() != 2u) return false;
    model.extData[0]->addString(1001, "ACAD");
    model.extData[crossed ? 2u : 3u]->addString(1001, "OTHER");
    model.pendingAppIdResolutions.clear();
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1012; raw.m_handle = model.handle; raw.m_objectType = 42;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size()); raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle; raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles; raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    const auto app = [](std::uint32_t handle, const std::string& name) noexcept {
        return (handle == 0x2au && name == "ACAD") || (handle == 0x2bu && name == "OTHER");
    };
    return dx_legacy_dwg_wire::observeDictionaryEed(raw, model, false, app,
        [](std::uint16_t, std::uint16_t) noexcept { return false; }) != crossed;
}

// Generated R14 producers may include a terminator in a TV field and encode
// an owner as an offset handle. Bind both forms to the same typed graph.
bool terminatedDictionaryControl(DRW::Version version) {
    dwgBufferW w;
    w.putObjType(version, 42u);
    w.putFixedHandle(0u, 1u, 0x17u);
    w.putBitShort(0u);
    w.putRawLong32(version == DRW::AC1014 ? 170u : 154u);
    w.putBitLong(1);
    w.putBitLong(1);
    if (version == DRW::AC1014) w.putRawChar8(0u);
    w.putBitShort(version == DRW::AC1014 ? 9u : 8u);
    for (unsigned char c : std::string("Standard")) w.putRawChar8(c);
    if (version == DRW::AC1014) w.putRawChar8(0u);
    w.putFixedHandle(12u, 1u, 11u);
    w.putFixedHandle(4u, 1u, 0xcu);
    w.putFixedHandle(3u, 0u, 0u);
    w.putFixedHandle(2u, 1u, 0x18u);
    for (int i = 0; i < 6; ++i) w.putBit(0u);
    const auto bytes = w.data();
    dwgBuffer input(bytes.data(), bytes.size());
    Dictionary model;
    if (!model.parseDwg(version, &input, 0u) || !input.isGood()
        || !model.legacyDwgCloningAbsent()) return false;
    DRW_Dictionary copied(model), assigned;
    assigned = model;
    if (!copied.legacyDwgCloningAbsent() || !assigned.legacyDwgCloningAbsent()
        || !copied.omitLegacyDxfCloning(DRW::AC1014)
        || copied.omitLegacyDxfCloning(DRW::AC1015)) return false;
    assigned.cloning = 1;
    if (assigned.omitLegacyDxfCloning(DRW::AC1014)) return false;
    assigned.reset();
    if (assigned.legacyDwgCloningAbsent() || assigned.omitLegacyDxfCloning(DRW::AC1014)) return false;
    dwgBuffer truncated(bytes.data(), bytes.size() - 1u);
    if (model.parseDwg(version, &truncated, 0u) || model.legacyDwgCloningAbsent()) return false;
    dwgBuffer retry(bytes.data(), bytes.size());
    if (!model.parseDwg(version, &retry, 0u) || !model.legacyDwgCloningAbsent()) return false;
    DRW_UnsupportedObject raw;
    raw.m_version = version;
    raw.m_handle = model.handle;
    raw.m_objectType = 42;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
    raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle;
    raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles;
    raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    if (!dx_legacy_dwg_wire::dictionary(raw, model, false, true)) return false;
    for (std::size_t bit = 0; bit < bytes.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (dx_legacy_dwg_wire::dictionary(changed, model, false, true)
            != (bit >= bytes.size() * 8u - 6u)) return false;
    }
    auto wrongVersion = raw;
    wrongVersion.m_version = version == DRW::AC1012 ? DRW::AC1014 : DRW::AC1012;
    return !dx_legacy_dwg_wire::dictionary(wrongVersion, model, false, true);
}

bool dictionaryWireControl(bool nod, bool mlineOwner) {
    const std::uint32_t id = nod ? 0xcu : (mlineOwner ? 0x1au : 0x22u);
    dwgBufferW w;
    w.putObjType(DRW::AC1014, 42u);
    w.putFixedHandle(0u, 1u, id);
    w.putBitShort(0u);
    w.putRawLong32(nod ? 486u : (mlineOwner ? 162u : 154u));
    w.putBitLong(nod ? 0 : 1);
    w.putBitLong(nod ? 3 : 1);
    w.putRawChar8(0u);
    if (nod) {
        putText(w, "ACAD_GROUP");
        putText(w, "ACAD_MLINESTYLE");
        putText(w, "ACDBVARIABLEDICTIONARY");
    } else putText(w, mlineOwner ? "STANDARD" : "DIMADEC");
    w.putFixedHandle(4u, nod ? 0u : 1u, nod ? 0u : 0xcu);
    if (!nod) w.putFixedHandle(4u, 1u, 0xcu);
    w.putFixedHandle(3u, 0u, 0u);
    if (nod) {
        w.putFixedHandle(2u, 1u, 0xdu);
        w.putFixedHandle(2u, 1u, 0x1au);
        w.putFixedHandle(2u, 1u, 0x22u);
        w.put2Bits(2u);
    } else {
        w.putFixedHandle(2u, 1u, mlineOwner ? 0x1bu : 0x23u);
        for (int bit = 5; bit >= 0; --bit)
            w.putBit(static_cast<std::uint8_t>(((mlineOwner ? 0u : 6u) >> bit) & 1u));
    }
    const auto bytes = w.data();
    dwgBuffer input(bytes.data(), bytes.size());
    Dictionary model;
    if (!model.parseDwg(DRW::AC1014, &input, 0u) || !input.isGood()) return false;
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1014;
    raw.m_handle = id;
    raw.m_objectType = 42;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
    raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle;
    raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles;
    raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    if (!dx_legacy_dwg_wire::dictionary(raw, model, nod, mlineOwner)) {
        std::cerr << "dictionary rejected: nod=" << nod << " mline=" << mlineOwner << '\n';
        return false;
    }
    for (std::size_t bit = 0; bit < bytes.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        const bool accepted = dx_legacy_dwg_wire::dictionary(changed, model, nod, mlineOwner);
        const std::size_t paddingBits = nod ? 2u : 6u;
        if (accepted != (bit >= bytes.size() * 8u - paddingBits)) {
            std::cerr << "dictionary mutation: nod=" << nod << " mline=" << mlineOwner << " bit=" << bit << " accepted=" << accepted << '\n';
            return false;
        }
    }
    return true;
}

bool populatedAc1012DictionaryFrame(const std::string& path,
                                    std::uint32_t expectedDictionary,
                                    std::uint32_t expectedParent,
                                    std::uint32_t expectedTarget) {
    std::ifstream input(path, std::ios::binary);
    const std::vector<std::uint8_t> bytes{
        std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    if (!input.is_open() || bytes.empty()) return false;

    Dictionary model;
    const auto read = [&](const std::vector<std::uint8_t>& source) {
        dwgBuffer buffer(source.data(), source.size());
        return model.parseDwg(DRW::AC1012, &buffer, 0u) && buffer.isGood();
    };
    const auto matches = [&]() {
        bool selectedEntry = false;
        for (const auto& entry : model.m_entries) {
            if (entry.m_name == "SAME_KEY"
                && entry.m_handle == expectedTarget) selectedEntry = true;
        }
        return model.handle == expectedDictionary
            && model.parentHandle == expectedParent
            && model.reactorHandles == std::vector<std::uint32_t>{expectedParent}
            && model.xDictHandle == 0u && model.legacyDwgCloningAbsent()
            && model.hasCompleteDwgEntries() && model.hasDwgCommonLinkTail()
            && !model.m_entries.empty() && selectedEntry;
    };
    if (!read(bytes)) {
        std::cerr << "AC1012 dictionary frame parser rejected physical frame\n";
        return false;
    }
    if (!matches()) {
        std::cerr << "AC1012 dictionary model mismatch: handle=" << model.handle
                  << " parent=" << model.parentHandle << " reactors="
                  << model.reactorHandles.size() << " entries="
                  << model.m_entries.size() << " complete="
                  << model.hasCompleteDwgEntries() << " common-tail="
                  << model.hasDwgCommonLinkTail() << '\n';
        for (const auto& entry : model.m_entries)
            std::cerr << "  entry " << entry.m_name << " -> " << entry.m_handle << '\n';
        return false;
    }

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1012;
    raw.m_handle = model.handle;
    raw.m_objectType = 42;
    raw.m_objectSize = static_cast<std::uint32_t>(bytes.size());
    raw.m_rawBytes = bytes;
    raw.m_parentHandle = model.parentHandle;
    raw.m_numReactors = model.reactorCount();
    raw.m_reactorHandles = model.reactorHandles;
    raw.m_xDictHandle = model.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    if (!dx_legacy_dwg_wire::dictionary(raw, model, false)) {
        std::cerr << "AC1012 dictionary shared source proof rejected physical frame\n";
        return false;
    }

    using dx_legacy_dwg_wire::Bits;
    Bits bits{bytes.data(), 0u, static_cast<std::uint32_t>(bytes.size() * 8u)};
    std::uint32_t bodyEnd = 0u;
    const auto noEed = [](Bits& source) noexcept {
        return source.shortValue() == 0u && source.good;
    };
    if (!dx_legacy_dwg_wire::frameStart(
            bits, DRW::AC1012, 42u, model.handle, model.reactorHandles.size(),
            noEed, bodyEnd)
        || bits.longValue() != model.m_entries.size())
        return false;
    bool foundSelectedEntry = false;
    for (const auto& entry : model.m_entries) {
        if (!dx_legacy_dwg_wire::text(bits, entry.m_name)) return false;
        if (entry.m_name == "SAME_KEY") {
            if (entry.m_handle != expectedTarget) return false;
            foundSelectedEntry = true;
        }
    }
    if (!foundSelectedEntry || bits.pos != bodyEnd) {
        std::cerr << "AC1012 dictionary independent frame walk failed\n";
        return false;
    }
    const auto commonTailBitStart = bits.pos;
    if (!dx_legacy_dwg_wire::commonTail(
            bits, model.handle, model.parentHandle, model.reactorHandles, 0u)) {
        std::cerr << "AC1012 dictionary common tail failed\n";
        return false;
    }
    std::vector<std::size_t> commonHandleBitStarts;
    Bits commonBits{bytes.data(), static_cast<std::uint32_t>(commonTailBitStart),
                    static_cast<std::uint32_t>(bytes.size() * 8u)};
    const auto captureCommonHandle = [&](unsigned role, std::uint32_t target) {
        commonHandleBitStarts.push_back(commonBits.pos);
        const auto parsed = dx_raster_wire::detail::handle(
            commonBits, role, model.handle);
        const bool directRole = parsed.role == role;
        const bool softPointerOffset = role == 4u
            && (parsed.role == 6u || parsed.role == 8u
                || parsed.role == 10u || parsed.role == 12u);
        return commonBits.good && (directRole || softPointerOffset)
            && parsed.value == target;
    };
    if (!captureCommonHandle(4u, model.parentHandle)) return false;
    for (const auto reactor : model.reactorHandles)
        if (!captureCommonHandle(4u, reactor)) return false;
    if (!captureCommonHandle(3u, 0u) || commonBits.pos != bits.pos) return false;

    std::vector<std::size_t> itemHandleBitStarts;
    itemHandleBitStarts.reserve(model.m_entries.size());
    for (const auto& entry : model.m_entries) {
        itemHandleBitStarts.push_back(bits.pos);
        if (!dx_legacy_dwg_wire::reference(
                bits, 2u, model.handle, entry.m_handle)) return false;
    }
    if (!dx_legacy_dwg_wire::padding(bits)) return false;

    for (const auto itemHandleBitStart : itemHandleBitStarts) {
        auto wrongRole = bytes;
        const auto hardOwnerRoleBit = itemHandleBitStart + 3u;
        wrongRole[hardOwnerRoleBit / 8u] ^=
            static_cast<std::uint8_t>(0x80u >> (hardOwnerRoleBit % 8u));
        auto mutatedRaw = raw;
        mutatedRaw.m_rawBytes = wrongRole;
        if (dx_legacy_dwg_wire::dictionary(mutatedRaw, model, false)
            || read(wrongRole) || model.hasCompleteDwgEntries()
            || !model.m_entries.empty() || !read(bytes) || !matches()) return false;
    }

    for (const auto commonHandleBitStart : commonHandleBitStarts) {
        auto wrongRole = bytes;
        const auto wrongRoleBit = commonHandleBitStart + 3u;
        wrongRole[wrongRoleBit / 8u] ^=
            static_cast<std::uint8_t>(0x80u >> (wrongRoleBit % 8u));
        auto mutatedRaw = raw;
        mutatedRaw.m_rawBytes = wrongRole;
        if (dx_legacy_dwg_wire::dictionary(mutatedRaw, model, false)
            || read(wrongRole) || model.hasCompleteDwgEntries()
            || !model.m_entries.empty() || !read(bytes) || !matches()) return false;
    }

    for (std::size_t length = 0; length < bytes.size(); ++length) {
        const std::vector<std::uint8_t> truncated(bytes.begin(),
                                                  bytes.begin() + length);
        if (read(truncated) || model.hasCompleteDwgEntries()
            || !model.m_entries.empty() || !read(bytes) || !matches()) return false;
    }
    return true;
}

std::vector<std::uint8_t> makeVariable(std::uint32_t dataBits = 104u,
                                       unsigned ownerRole = 8u,
                                       unsigned reactorRole = 4u,
                                       unsigned extensionRole = 3u) {
    // This control is encoded locally from fields; it is not a sample file.
    dwgBufferW w;
    w.putObjType(DRW::AC1014, 500u);
    w.putFixedHandle(0u, 1u, 0x23u);
    w.putBitShort(0u);
    w.putRawLong32(dataBits);
    w.putBitLong(1);
    w.putRawChar8(0u);
    w.putBitShort(1u);
    w.putRawChar8('0');
    w.putFixedHandle(ownerRole, 0u, 0u);
    w.putFixedHandle(reactorRole, 1u, 0x22u);
    w.putFixedHandle(extensionRole, 0u, 0u);
    return w.data();
}

bool parse(const std::vector<std::uint8_t>& data, Variable& variable) {
    dwgBuffer input(data.data(), data.size());
    return variable.parseDwg(DRW::AC1014, &input, 0u) && input.isGood();
}

bool expected(const Variable& v) {
    return v.handle == 0x23u && v.parentHandle == 0x22u
        && v.m_schema == 0 && v.m_value == "0"
        && v.reactorCount() == 1 && v.reactorHandles.size() == 1u
        && v.reactorHandles.front() == 0x22u && v.xDictHandle == 0u
        && v.hasDwgCommonLinkTail();
}

struct Ac1012VariableControl {
    std::vector<std::uint8_t> bytes;
    std::uint32_t bodyEnd {0u};
    std::size_t bodyEndFieldBit {0u};
};

Ac1012VariableControl makeAc1012Variable(bool appendUnknownBodyByte = false) {
    dwgBufferW prefix;
    prefix.putObjType(DRW::AC1012, 500u);
    prefix.putFixedHandle(0u, 1u, 0x23u);
    prefix.putBitShort(0u);

    dwgBufferW body;
    body.putBitLong(1u); // Numreactors
    body.putRawChar8(0u); // Intval / schema
    body.putBitShort(1u); // one-byte string
    body.putRawChar8('0');
    if (appendUnknownBodyByte)
        body.putRawChar8(0u);

    Ac1012VariableControl control;
    control.bodyEnd = writtenBits(prefix) + 32u + writtenBits(body);
    dwgBufferW writer;
    writer.putObjType(DRW::AC1012, 500u);
    writer.putFixedHandle(0u, 1u, 0x23u);
    writer.putBitShort(0u);
    control.bodyEndFieldBit = writtenBits(writer);
    writer.putRawLong32(control.bodyEnd);
    writer.putBitLong(1u);
    writer.putRawChar8(0u);
    writer.putBitShort(1u);
    writer.putRawChar8('0');
    if (appendUnknownBodyByte)
        writer.putRawChar8(0u);
    writer.putFixedHandle(8u, 0u, 0u); // relative soft pointer: self - 1
    writer.putFixedHandle(4u, 1u, 0x22u); // reactor soft pointer
    writer.putFixedHandle(3u, 0u, 0u); // typed-null extension dictionary
    while (writer.bitPos() != 0u)
        writer.putBit(0u);
    control.bytes = writer.data();
    return control;
}

bool parse(DRW::Version version, const std::vector<std::uint8_t>& data,
           Variable& variable) {
    dwgBuffer input(data.data(), data.size());
    return variable.parseDwg(version, &input, 0u) && input.isGood();
}

bool ac1012DictionaryVarControl() {
    const auto control = makeAc1012Variable();
    Variable variable;
    const auto parseSource = [&](const std::vector<std::uint8_t>& bytes) {
        return parse(DRW::AC1012, bytes, variable);
    };
    if (!parseSource(control.bytes) || !expected(variable))
        return false;

    auto missingBodyBoundary = control.bytes;
    setRawLong32Bits(missingBodyBoundary, control.bodyEndFieldBit, 0u);
    if (parseSource(missingBodyBoundary) || variable.hasDwgCommonLinkTail()
        || !variable.m_value.empty()
        || !parseSource(control.bytes) || !expected(variable))
        return false;

    const std::array<std::pair<std::size_t, unsigned>, 3> wrongRoles{{
        {control.bodyEnd, 2u},
        {static_cast<std::size_t>(control.bodyEnd) + 8u, 2u},
        {static_cast<std::size_t>(control.bodyEnd) + 24u, 4u}}};
    for (const auto& wrongRole : wrongRoles) {
        auto malformed = control.bytes;
        setNibble(malformed, wrongRole.first, wrongRole.second);
        if (parseSource(malformed) || variable.hasDwgCommonLinkTail()
            || !variable.m_value.empty()
            || !parseSource(control.bytes) || !expected(variable))
            return false;
    }

    const auto unknownBody = makeAc1012Variable(true);
    if (parseSource(unknownBody.bytes) || variable.hasDwgCommonLinkTail()
        || !variable.m_value.empty()
        || !parseSource(control.bytes) || !expected(variable))
        return false;

    for (std::size_t length = 0; length < control.bytes.size(); ++length) {
        const std::vector<std::uint8_t> truncated(control.bytes.begin(),
                                                  control.bytes.begin() + length);
        if (parseSource(truncated) || variable.hasDwgCommonLinkTail()
            || !variable.m_value.empty()
            || !parseSource(control.bytes) || !expected(variable))
            return false;
    }
    return true;
}

bool run() {
    if (!sharedLegacyFrameControl() || !eedLayerWriteOrderControl() || !legacyDictionaryWithDefaultControl() || !r13DictionaryEedControl() || !r13DictionaryEedControl(true) || !r13DictionaryEedControl(false, true) || !r13DictionaryEedControl(false, false, true) || !r13DictionaryEedControl(false, false, false, true) || !r13DictionaryEedControl(false, false, false, false, true) || !r13DictionaryEedControl(false, false, false, false, false, true) || !r13DictionaryEedControl(false, false, false, false, false, false, true)
        || !r13EedChunkGroupBoundary(false) || !r13EedChunkGroupBoundary(true) || !terminatedDictionaryControl(DRW::AC1012)
        || !terminatedDictionaryControl(DRW::AC1014) || !dictionaryWireControl(true, false)
        || !dictionaryWireControl(false, false)
        || !dictionaryWireControl(false, true)
        || !generatedAc1012DictionaryFrameControl()
        || !ac1012DictionaryVarControl()) {
        std::cerr << "generated dictionary source or mutation control failed\n";
        return false;
    }
    const auto source = makeVariable();
    Variable value;
    const bool sourceParsed = parse(source, value);
    if (source.size() != 17u || !sourceParsed || !expected(value)) {
        std::cerr << "generated source did not parse or bind, size=" << source.size()
                  << " parsed=" << sourceParsed << " handle=" << value.handle
                  << " owner=" << value.parentHandle << " value=" << value.m_value
                  << " reactors=" << value.reactorCount() << " tail="
                  << value.hasDwgCommonLinkTail() << '\n';
        return false;
    }

    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1014;
    raw.m_handle = value.handle;
    raw.m_objectType = 500;
    raw.m_objectSize = static_cast<std::uint32_t>(source.size());
    raw.m_rawBytes = source;
    raw.m_isCustomClass = true;
    raw.m_hasClassDefinition = true;
    raw.m_recordName = "DICTIONARYVAR";
    raw.m_className = "AcDbDictionaryVar";
    raw.m_parentHandle = value.parentHandle;
    raw.m_numReactors = value.reactorCount();
    raw.m_reactorHandles = value.reactorHandles;
    raw.m_xDictHandle = value.xDictHandle;
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    if (!dx_ac1014_dictionaryvar_wire::variable(raw, value)) {
        std::cerr << "wire observer rejected generated source\n";
        return false;
    }
    for (std::size_t bit = 0; bit < source.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (dx_ac1014_dictionaryvar_wire::variable(changed, value)) {
            std::cerr << "wire observer accepted bit mutation " << bit << '\n';
            return false;
        }
    }

    // A failed parse resets the same object; retrying the corrected source
    // must publish the complete value and validated common-link tail.
    for (std::size_t size = 0; size < source.size(); ++size) {
        auto truncated = source;
        truncated.resize(size);
        if (parse(truncated, value) || value.hasDwgCommonLinkTail()
            || !parse(source, value) || !expected(value)) {
            std::cerr << "truncation/refusal/retry failed at size " << size << '\n';
            return false;
        }
    }
    for (const auto& bad : {makeVariable(103u), makeVariable(105u),
                            makeVariable(104u, 2u), makeVariable(104u, 8u, 2u),
                            makeVariable(104u, 8u, 4u, 2u)}) {
        if (parse(bad, value) || value.hasDwgCommonLinkTail()
            || !parse(source, value) || !expected(value)) {
            std::cerr << "invalid boundary or role refusal/retry failed\n";
            return false;
        }
    }

    dwgBufferW w;
    w.putObjType(DRW::AC1014, 73u);
    w.putFixedHandle(0u, 1u, 0x1bu);
    w.putBitShort(0u);
    w.putRawLong32(462u);
    w.putBitLong(1);
    putText(w, "STANDARD");
    putText(w, "");
    w.putBitShort(0u);
    w.putBitShort(256u);
    w.putBitDouble(1.5707963267948966);
    w.putBitDouble(1.5707963267948966);
    w.putRawChar8(2u);
    for (const double offset : {0.5, -0.5}) {
        w.putBitDouble(offset);
        w.putBitShort(256u);
        w.putBitShort(32767u);
    }
    w.putFixedHandle(8u, 0u, 0u);
    w.putFixedHandle(4u, 1u, 0x1au);
    w.putFixedHandle(3u, 0u, 0u);
    w.put2Bits(3u);
    const auto styleBytes = w.data();
    dwgBuffer styleInput(styleBytes.data(), styleBytes.size());
    Style style;
    if (styleBytes.size() != 62u || !style.parseDwg(DRW::AC1014, &styleInput, 0u)
        || !styleInput.isGood()) {
        std::cerr << "generated MLINESTYLE did not parse\n";
        return false;
    }
    DRW_UnsupportedObject rawStyle;
    rawStyle.m_version = DRW::AC1014;
    rawStyle.m_objectType = 73;
    rawStyle.m_handle = 0x1bu;
    rawStyle.m_objectSize = static_cast<std::uint32_t>(styleBytes.size());
    rawStyle.m_rawBytes = styleBytes;
    if (!dx_legacy_mlinestyle_wire::style(rawStyle, style)) {
        std::cerr << "MLINESTYLE observer rejected generated source\n";
        return false;
    }
    dwgBuffer r13StyleInput(styleBytes.data(), styleBytes.size());
    Style r13Style;
    rawStyle.m_version = DRW::AC1012;
    if (!r13Style.parseDwg(DRW::AC1012, &r13StyleInput, 0u)
        || !r13StyleInput.isGood() || !dx_legacy_mlinestyle_wire::style(rawStyle, r13Style)) {
        std::cerr << "R13 MLINESTYLE shared framing failed\n";
        return false;
    }
    rawStyle.m_version = DRW::AC1014;
    for (std::size_t bit = 0; bit < styleBytes.size() * 8u; ++bit) {
        auto changed = rawStyle;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        const bool accepted = dx_legacy_mlinestyle_wire::style(changed, style);
        if (accepted != (bit >= styleBytes.size() * 8u - 2u)) {
            std::cerr << "MLINESTYLE observer mishandled bit mutation " << bit << '\n';
            return false;
        }
    }
    return true;
}
} // namespace

int main(int argc, char** argv) {
    if (argc == 6 && std::string(argv[1]) == "--source-dictionary-frame") {
        try {
            const auto dictionary =
                static_cast<std::uint32_t>(std::stoul(argv[3]));
            const auto parent =
                static_cast<std::uint32_t>(std::stoul(argv[4]));
            const auto target = static_cast<std::uint32_t>(std::stoul(argv[5]));
            if (populatedAc1012DictionaryFrame(
                    argv[2], dictionary, parent, target)) {
                std::cout << "physical AC1012 dictionary frame and role/refusal/retry: PASS\n";
                return 0;
            }
        } catch (...) {
            std::cerr << "invalid physical AC1012 dictionary test arguments\n";
            return 2;
        }
        std::cerr << "physical AC1012 dictionary frame check failed\n";
        return 1;
    }
    if (argc != 1) return 2;
    if (run()) return 0;
    std::cerr << "AC1014 DICTIONARYVAR body or refusal/retry check failed\n";
    return 1;
}
