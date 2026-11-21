// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_ac1014_dictionaryvar_wire.h"
#include "dx_legacy_mlinestyle_wire.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

#include <cstdint>
#include <iostream>
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

void putText(dwgBufferW& w, const std::string& text) {
    w.putBitShort(static_cast<std::uint16_t>(text.size()));
    for (const unsigned char c : text) w.putRawChar8(c);
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
    if (!model.parseDwg(version, &input, 0u) || !input.isGood()) return false;
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
    if (!dx_ac1014_dictionaryvar_wire::dictionary(raw, model, false, true)) return false;
    for (std::size_t bit = 0; bit < bytes.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (dx_legacy_dwg_wire::dictionary(changed, model, false, true)
            != (bit >= bytes.size() * 8u - 6u)) return false;
    }
    auto wrongVersion = raw;
    wrongVersion.m_version = version == DRW::AC1012 ? DRW::AC1014 : DRW::AC1012;
    return !dx_ac1014_dictionaryvar_wire::dictionary(wrongVersion, model, false, true);
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
    if (!dx_ac1014_dictionaryvar_wire::dictionary(raw, model, nod, mlineOwner)) {
        std::cerr << "dictionary rejected: nod=" << nod << " mline=" << mlineOwner << '\n';
        return false;
    }
    for (std::size_t bit = 0; bit < bytes.size() * 8u; ++bit) {
        auto changed = raw;
        changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        const bool accepted = dx_ac1014_dictionaryvar_wire::dictionary(changed, model, nod, mlineOwner);
        const std::size_t paddingBits = nod ? 2u : 6u;
        if (accepted != (bit >= bytes.size() * 8u - paddingBits)) {
            std::cerr << "dictionary mutation: nod=" << nod << " mline=" << mlineOwner << " bit=" << bit << " accepted=" << accepted << '\n';
            return false;
        }
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

bool run() {
    if (!terminatedDictionaryControl(DRW::AC1012)
        || !terminatedDictionaryControl(DRW::AC1014) || !dictionaryWireControl(true, false)
        || !dictionaryWireControl(false, false)
        || !dictionaryWireControl(false, true)) {
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

int main() {
    if (run()) return 0;
    std::cerr << "AC1014 DICTIONARYVAR body or refusal/retry check failed\n";
    return 1;
}
