// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_ac1027_plotstyle_wire.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include <array>
#include <cstdint>

namespace {
void appendBits(dwgBufferW& out, const dwgBufferW& source) {
    for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit)
        out.putBit((source.data()[bit / 8u] >> (7u - (bit & 7u))) & 1u);
}
void putHandle(dwgBufferW& out, std::uint8_t code, std::uint32_t ref) {
    dwgHandle handle;
    handle.code = code; handle.ref = ref; handle.ref64 = ref;
    out.putHandle(handle);
}
struct Generated {
    DRW_UnsupportedObject raw;
    std::uint64_t stringStart{0u};
    std::array<std::uint64_t, 3> handleStarts{};
};
Generated build(bool dictionary, DRW::Version version, bool writerPadding,
                bool oneReactor = false) {
    DRW_TextCodec codec;
    codec.setVersion(version, false);
    dwgBufferW out(&codec), strings(&codec);
    const std::uint32_t self = dictionary ? 18u : 19u;
    out.putObjType(version, dictionary ? 500u : 80u);
    putHandle(out, 0u, self);
    out.putBitShort(0u); // no EED
    out.putBitLong(oneReactor ? 1 : 0);
    out.putBit(1u); // no extension dictionary
    if (version > DRW::AC1024) out.putBit(0u); // no data storage
    Generated result;
    if (dictionary) {
        out.putBitLong(1); // one dictionary entry
        out.putBitShort(1u); // cloning
        out.putRawChar8(0u); // soft-owner dictionary
        if (oneReactor) {
            strings.putBitShort(6u);
            for (const char c : {'N', 'o', 'r', 'm', 'a', 'l'})
                strings.putRawShort16(static_cast<std::uint16_t>(c));
        } else {
            strings.putVariableText(version, "Normal");
        }
        result.stringStart = out.bitCount();
        appendBits(out, strings);
        // Exercise the two- and nine-bit zero-padded seven-code-unit streams.
        const int padding = oneReactor ? 0 : writerPadding ? 2 : 9;
        for (int i = 0; i < padding; ++i) out.putBit(0u);
        out.putRawShort16(static_cast<std::uint16_t>(strings.bitCount() + padding));
        out.putBit(1u);
    } else {
        out.putBit(0u); // no string stream
        if (writerPadding)
            for (int i = 0; i < 24; ++i) out.putBit(0u);
    }
    const auto bodyBits = out.bitCount();
    result.handleStarts[0] = out.bitCount();
    if (oneReactor) {
        putHandle(out, dictionary ? 12u : 8u, dictionary ? 8u : 0u);
        putHandle(out, 4u, dictionary ? 10u : 18u);
    } else {
        putHandle(out, 4u, dictionary ? 10u : 18u);
    }
    if (dictionary) {
        result.handleStarts[1] = out.bitCount();
        putHandle(out, 2u, 19u);
        result.handleStarts[2] = out.bitCount();
        putHandle(out, 5u, 19u);
    }
    if (oneReactor && dictionary) {
        out.putBit(1u);
        out.putBit(1u);
    }
    out.alignToByte();
    result.raw.m_rawBytes = out.data();
    result.raw.m_bodyBitSize = static_cast<std::uint32_t>(out.bitCount() - bodyBits);
    result.raw.m_version = version;
    result.raw.m_handle = self;
    result.raw.m_parentHandle = dictionary ? 10u : 18u;
    result.raw.m_isCustomClass = dictionary;
    result.raw.m_objectType = dictionary ? 500 : 80;
    result.raw.m_xDictFlag = 1u;
    if (oneReactor) {
        result.raw.m_objectSize = static_cast<std::uint32_t>(out.data().size());
        result.raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
        result.raw.m_numReactors = 1;
        result.raw.m_reactorHandles = {result.raw.m_parentHandle};
    }
    return result;
}
void flip(std::vector<std::uint8_t>& bytes, std::uint64_t bit) {
    bytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - (bit & 7u)));
}
} // namespace

int main() {
    for (const bool dictionary : {false, true}) {
        const auto generated = build(dictionary, DRW::AC1024, false, true);
        const auto matches = [dictionary](const DRW_UnsupportedObject& raw) {
            return dx_ac1024_plotstyle_wire::matchesOneReactor(
                raw, dictionary, dictionary ? 10u : 18u, 19u);
        };
        if (!matches(generated.raw)
            || dx_ac1024_plotstyle_wire::matches(
                generated.raw, dictionary, dictionary ? 10u : 18u, 19u))
            return 1;
        for (std::uint64_t bit = 0; bit < generated.raw.m_rawBytes.size() * 8u; ++bit) {
            auto changed = generated.raw;
            flip(changed.m_rawBytes, bit);
            if (matches(changed)) return 1;
        }
        auto changed = generated.raw;
        changed.m_rawBytes.pop_back();
        if (matches(changed)) return 1;
        changed = generated.raw;
        changed.m_reactorHandles.front() = 1u;
        if (matches(changed)) return 1;
        changed = generated.raw;
        changed.m_version = DRW::AC1027;
        if (matches(changed) || !matches(generated.raw)) return 1;
    }
    for (const auto version : {DRW::AC1024, DRW::AC1027}) {
     for (const bool writerPadding : {false, true}) {
      if (writerPadding && version != DRW::AC1024) continue;
      for (const bool dictionary : {false, true}) {
        const auto generated = build(dictionary, version, writerPadding);
        const auto matches = [dictionary, version](const DRW_UnsupportedObject& raw) {
            return version == DRW::AC1024
                ? dx_ac1024_plotstyle_wire::matches(raw, dictionary,
                                                    dictionary ? 10u : 18u, 19u)
                : dx_ac1027_plotstyle_wire::matches(raw, dictionary,
                                                    dictionary ? 10u : 18u, 19u);
        };
        if (!matches(generated.raw))
            return 1;
        auto wrongVersion = generated.raw;
        wrongVersion.m_version = version == DRW::AC1024 ? DRW::AC1027 : DRW::AC1024;
        if (matches(wrongVersion)) return 1;
        for (std::size_t index = 0; index < (dictionary ? 3u : 1u); ++index) {
            auto mutated = generated.raw;
            flip(mutated.m_rawBytes, generated.handleStarts[index]);
            if (matches(mutated)) return 1;
        }
        auto truncated = generated.raw;
        truncated.m_rawBytes.pop_back();
        if (matches(truncated)) return 1;
        auto badReactors = generated.raw;
        badReactors.m_numReactors = 1;
        if (matches(badReactors)) return 1;
        if (dictionary) {
            auto badName = generated.raw;
            flip(badName.m_rawBytes, generated.stringStart + 12u);
            if (matches(badName)) return 1;
        }
        if (!matches(generated.raw)) return 1;
      }
     }
    }
    return 0;
}
