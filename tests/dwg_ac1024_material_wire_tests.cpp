// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_ac1027_material_wire.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbufferw.h"
#include <array>
#include <cstdint>

namespace {
void appendBits(dwgBufferW &out, const dwgBufferW &source) {
  for (std::uint64_t bit = 0; bit < source.bitCount(); ++bit)
    out.putBit((source.data()[bit / 8u] >> (7u - (bit & 7u))) & 1u);
}
void putHandle(dwgBufferW &out, std::uint8_t code, std::uint32_t ref) {
  dwgHandle h;
  h.code = code;
  h.ref = ref;
  h.ref64 = ref;
  out.putHandle(h);
}
struct Generated {
  DRW_UnsupportedObject raw;
  std::uint64_t stringStart{0u};
  std::array<std::uint64_t, 5> handles{};
};
Generated material(DRW::Version version, DRW_Material &typed) {
  DRW_TextCodec codec;
  codec.setVersion(version, false);
  dwgBufferW out(&codec);
  out.putObjType(version, 503u);
  putHandle(out, 0u, 33u);
  out.putBitShort(0u);
  out.putBitLong(1);
  out.putBit(1u);
  if (version > DRW::AC1024)
    out.putBit(0u);
  out.putBit(0u); // bounded body payload; this observer checks the common links
  const auto bodyBits = out.bitCount();
  Generated result;
  result.handles[0] = out.bitCount();
  putHandle(out, 4u, 14u);
  result.handles[1] = out.bitCount();
  putHandle(out, 4u, 14u);
  out.alignToByte();
  auto &raw = result.raw;
  raw.m_rawBytes = out.data();
  raw.m_bodyBitSize = static_cast<std::uint32_t>(out.bitCount() - bodyBits);
  raw.m_version = version;
  raw.m_handle = 33u;
  raw.m_parentHandle = 14u;
  raw.m_isCustomClass = true;
  raw.m_objectType = 503;
  raw.m_recordName = "MATERIAL";
  raw.m_className = "AcDbMaterial";
  raw.m_numReactors = 1;
  raw.m_reactorHandles = {14u};
  raw.m_xDictFlag = 1u;
  typed.handle = 33u;
  typed.parentHandle = 14u;
  typed.reactorHandles = {14u};
  typed.setDwgCommonObjectState(1, 1u, false);
  return result;
}
Generated dictionary(DRW::Version version, DRW_Dictionary &typed) {
  DRW_TextCodec codec;
  codec.setVersion(version, false);
  dwgBufferW out(&codec), strings(&codec);
  out.putObjType(version, 42u);
  putHandle(out, 0u, 14u);
  out.putBitShort(0u);
  out.putBitLong(version == DRW::AC1024 ? 0 : 1);
  out.putBit(1u);
  if (version > DRW::AC1024)
    out.putBit(0u);
  out.putBitLong(3);
  out.putBitShort(1u);
  out.putRawChar8(0u);
  typed.handle = 14u;
  typed.parentHandle = 10u;
  typed.cloning = 1;
  typed.hardOwner = 0;
  typed.m_entries = {{"ByBlock", 31u}, {"ByLayer", 32u}, {"Global", 33u}};
  for (const auto &entry : typed.m_entries) {
    strings.putBitShort(static_cast<std::uint16_t>(entry.m_name.size()));
    for (const unsigned char c : entry.m_name)
      strings.putRawShort16(c);
  }
  Generated result;
  result.stringStart = out.bitCount();
  appendBits(out, strings);
  out.putRawShort16(static_cast<std::uint16_t>(strings.bitCount()));
  out.putBit(1u);
  const auto bodyBits = out.bitCount();
  result.handles[0] = out.bitCount();
  putHandle(out, 4u, 10u);
  std::size_t at = 1u;
  if (version > DRW::AC1024) {
    result.handles[at++] = out.bitCount();
    putHandle(out, 4u, 10u);
  }
  for (const auto &entry : typed.m_entries) {
    result.handles[at++] = out.bitCount();
    putHandle(out, 2u, entry.m_handle);
  }
  out.alignToByte();
  auto &raw = result.raw;
  raw.m_rawBytes = out.data();
  raw.m_bodyBitSize = static_cast<std::uint32_t>(out.bitCount() - bodyBits);
  raw.m_version = version;
  raw.m_handle = 14u;
  raw.m_parentHandle = 10u;
  raw.m_objectType = 42;
  raw.m_xDictFlag = 1u;
  if (version > DRW::AC1024) {
    raw.m_numReactors = 1;
    raw.m_reactorHandles = {10u};
  }
  return result;
}
void flip(std::vector<std::uint8_t> &bytes, std::uint64_t bit) {
  bytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - (bit & 7u)));
}
} // namespace
int main() {
  for (const auto version : {DRW::AC1024, DRW::AC1027}) {
    DRW_Material typedMaterial;
    const auto m = material(version, typedMaterial);
    const auto matchMaterial = [&](const DRW_UnsupportedObject &raw) {
      return version == DRW::AC1024
                 ? dx_ac1024_material_wire::matches(raw, typedMaterial)
                 : dx_ac1027_material_wire::matches(raw, typedMaterial);
    };
    if (!matchMaterial(m.raw))
      return 1;
    for (unsigned i = 0; i < 2; ++i) {
      auto bad = m.raw;
      flip(bad.m_rawBytes, m.handles[i]);
      if (matchMaterial(bad))
        return 1;
    }
    auto truncated = m.raw;
    truncated.m_rawBytes.pop_back();
    if (matchMaterial(truncated))
      return 1;
    auto wrong = m.raw;
    wrong.m_version = version == DRW::AC1024 ? DRW::AC1027 : DRW::AC1024;
    if (matchMaterial(wrong))
      return 1;
    if (!matchMaterial(m.raw))
      return 1;
    DRW_Dictionary typedDictionary;
    typedDictionary.reset();
    const auto d = dictionary(version, typedDictionary);
    const auto matchDictionary = [&](const DRW_UnsupportedObject &raw) {
      return version == DRW::AC1024
                 ? dx_ac1024_material_wire::dictionary(raw, typedDictionary,
                                                       10u)
                 : dx_ac1027_material_wire::dictionary(raw, typedDictionary,
                                                       10u);
    };
    if (!matchDictionary(d.raw))
      return 1;
    for (std::size_t i = 0; i < (version == DRW::AC1024 ? 4u : 5u); ++i) {
      auto bad = d.raw;
      flip(bad.m_rawBytes, d.handles[i]);
      if (matchDictionary(bad))
        return 1;
    }
    auto badName = d.raw;
    flip(badName.m_rawBytes, d.stringStart + 12u);
    if (matchDictionary(badName))
      return 1;
    auto badCount = d.raw;
    badCount.m_numReactors = version == DRW::AC1024 ? 1 : 0;
    if (matchDictionary(badCount))
      return 1;
    auto shortFrame = d.raw;
    shortFrame.m_rawBytes.pop_back();
    if (matchDictionary(shortFrame))
      return 1;
    if (!matchDictionary(d.raw))
      return 1;
  }
}
