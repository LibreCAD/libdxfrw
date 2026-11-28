// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dwg_default_material.h"
#include "intern/dwgbuffer.h"
#include "material_dwg_test_helpers.h"
#include <array>
#include <cstdio>
#include <string>
#include <vector>

namespace {
using drwDwgDefaultMaterial::Kind;
using material_dwg_test::appendBits;

bool frame(DRW::Version version, Kind kind, std::uint32_t handle,
           material_dwg_test::Frame& result) {
    dwgBufferW body, strings, object, handles;
    if (!drwDwgDefaultMaterial::build(version, kind, body, strings)) return false;
    object.putObjType(version, 503); // file-local class ordinal in witness
    dwgHandle own; own.ref = handle; object.putHandle(own);
    object.putBitShort(0); object.putBitLong(1); object.putBit(1);
    appendBits(object, body);
    appendBits(object, strings);
    object.putRawShort16(static_cast<std::uint16_t>(strings.bitCount()));
    object.putBit(1);
    dwgHandle ref; ref.code = DRW::DwgSoftPointer; ref.ref = 0xEu;
    handles.putHandle(ref); handles.putHandle(ref);
    result.handleBits = static_cast<std::uint32_t>(handles.bitCount() + 2u);
    appendBits(object, handles);
    object.putBit(0); object.putBit(0);
    result.bytes = object.data();
    return object.isGood() && handles.isGood();
}

bool run() {
    const std::array<Kind, 3> kinds{Kind::ByBlock, Kind::ByLayer, Kind::Global};
    const std::array<const char*, 3> names{"ByBlock", "ByLayer", "Global"};
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        dwgBufferW body, strings;
        if (!drwDwgDefaultMaterial::build(DRW::AC1024, kinds[i], body, strings)
            || body.bitCount() != 550u || strings.bitCount() != (i == 2u ? 120u : 136u))
            return false;
        material_dwg_test::Frame built;
        material_dwg_test::CodecProbe parsed;
        if (!frame(DRW::AC1024, kinds[i], static_cast<std::uint32_t>(0x1Fu + i), built)
            || !material_dwg_test::parse(built, parsed)
            || !parsed.hasCompleteDwgSemanticFields()
            || parsed.m_name != names[i] || !parsed.m_description.empty()
            || parsed.handle != 0x1Fu + i || parsed.parentHandle != 0xEu
            || parsed.reactorHandles != std::vector<std::uint32_t>{0xEu}
            || parsed.m_diffuseColorMethod != 1u
            || parsed.m_diffuseColorValue != 0xc2ffffffu
            || parsed.m_specularGlossFactor != 0.5
            || parsed.m_channelFlags != 63
            || !parsed.m_diffuseMapFileName.empty()
            || !parsed.m_refractionMapFileName.empty())
            return false;

        dwgBufferW body27, strings27;
        if (!drwDwgDefaultMaterial::build(DRW::AC1027, kinds[i], body27, strings27)
            || body27.bitCount() != 516u
            || strings27.bitCount() != (i == 2u ? 118u : 134u)
            || body27.data() == body.data()
            || strings27.data() == strings.data())
        { std::fprintf(stderr, "AC1027 builder mismatch kind=%u body=%llu strings=%llu\n",
              static_cast<unsigned>(i),
              static_cast<unsigned long long>(body27.bitCount()),
              static_cast<unsigned long long>(strings27.bitCount())); return false; }
    }
    dwgBufferW body, strings;
    body.putRawChar8(0xA5); strings.putRawChar8(0x5A);
    if (drwDwgDefaultMaterial::build(DRW::AC1032, Kind::Global, body, strings)
        || drwDwgDefaultMaterial::build(DRW::AC1024, static_cast<Kind>(99), body, strings)
        || drwDwgDefaultMaterial::build(DRW::AC1024, Kind::Global, body, body)
        || body.data() != std::vector<std::uint8_t>{0xA5}
        || strings.data() != std::vector<std::uint8_t>{0x5A}) return false;
    return drwDwgDefaultMaterial::build(DRW::AC1024, Kind::Global, body, strings)
        && body.bitCount() == 550u && strings.bitCount() == 120u;
}
}
int main() {
    if (!run()) { std::fprintf(stderr, "AC1024 default material control failed\n"); return 1; }
    std::puts("AC1024 default material control passed"); return 0;
}
