#include "dx_ac1027_material_wire.h"
#include "dx_ac1024_mlinestyle_wire.h"
#include "dx_layerstates_dwg_wire.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
class BitWriter {
public:
  void bits(std::uint64_t value, unsigned count) {
    for (unsigned index = 0; index < count; ++index) {
      const unsigned shift = count - index - 1u;
      if ((m_size & 7u) == 0u)
        m_bytes.push_back(0u);
      if (((value >> shift) & 1u) != 0u)
        m_bytes.back() |= static_cast<std::uint8_t>(1u << (7u - (m_size & 7u)));
      ++m_size;
    }
  }

  void little16(std::uint16_t value) {
    bits(value & 0xffu, 8u);
    bits(value >> 8u, 8u);
  }

  void shortValue(std::uint8_t value) {
    bits(1u, 2u);
    bits(value, 8u);
  }

  void longValue(std::uint8_t value) {
    bits(1u, 2u);
    bits(value, 8u);
  }

  void handle(std::uint8_t role, std::uint8_t value) {
    bits(static_cast<std::uint8_t>((role << 4u) | 1u), 8u);
    bits(value, 8u);
  }

  std::size_t size() const noexcept { return m_size; }
  std::vector<std::uint8_t> take() && { return std::move(m_bytes); }

private:
  std::vector<std::uint8_t> m_bytes;
  std::size_t m_size{0u};
};

void overwriteBits(std::vector<std::uint8_t> &bytes, std::size_t begin,
                   std::uint64_t value, unsigned count) {
  for (unsigned index = 0; index < count; ++index) {
    const std::size_t bit = begin + index;
    const std::uint8_t mask =
        static_cast<std::uint8_t>(1u << (7u - (bit & 7u)));
    const unsigned shift = count - index - 1u;
    if (((value >> shift) & 1u) != 0u)
      bytes[bit / 8u] |= mask;
    else
      bytes[bit / 8u] &= static_cast<std::uint8_t>(~mask);
  }
}

DRW_UnsupportedObject makeDictionaryFrame(
    const std::vector<std::pair<std::string, std::uint8_t>> &entries) {
  constexpr std::uint8_t dictionaryHandle = 66u;
  constexpr std::uint8_t ownerHandle = 67u;
  BitWriter writer;

  // Object type, self handle, empty EED, one reactor, R2004+ no-xdict
  // flag, no DataStorage, three entries, cloning=1, hardOwner=0.
  writer.bits(0u, 2u);
  writer.bits(42u, 8u);
  writer.handle(0u, dictionaryHandle);
  writer.bits(2u, 2u);  // empty EED size
  writer.longValue(1u); // Numreactors
  writer.bits(1u, 1u);  // XDic Missing Flag
  writer.bits(0u, 1u);  // no DataStorage
  writer.longValue(3u);
  writer.shortValue(1u);
  writer.bits(0u, 8u); // hardOwner

  for (const auto &entry : entries) {
    writer.shortValue(static_cast<std::uint8_t>(entry.first.size()));
    for (const unsigned char character : entry.first)
      writer.little16(character);
  }
  writer.little16(350u); // exact bit span of 3 length words + UTF-16 names
  writer.bits(1u, 1u);   // bounded string-stream marker

  // Common handle stream: parent, reactor, then ordered soft-owner members.
  writer.handle(4u, ownerHandle);
  writer.handle(4u, ownerHandle);
  for (const auto &entry : entries)
    writer.handle(2u, entry.second);
  writer.bits(0u, 5u); // terminal alignment included in the reported span

  if (writer.size() != 520u)
    std::cerr << "generated AC1027 dictionary frame has " << writer.size()
              << " bits, expected 520\n";

  DRW_UnsupportedObject raw;
  raw.m_version = DRW::AC1027;
  raw.m_objectType = 42;
  raw.m_handle = dictionaryHandle;
  raw.m_parentHandle = ownerHandle;
  raw.m_numReactors = 1;
  raw.m_reactorHandles = {ownerHandle};
  raw.m_xDictFlag = 1u;
  raw.m_bodyBitSize = 85u;
  raw.m_rawBytes = std::move(writer).take();
  return raw;
}

DRW_Dictionary makeTypedDictionary(
    const std::vector<std::pair<std::string, std::uint8_t>> &entries) {
  DRW_Dictionary typed;
  typed.handle = 66u;
  typed.parentHandle = 67u;
  typed.reactorHandles = {67u};
  for (const auto &entry : entries)
    typed.m_entries.push_back({entry.first, entry.second});
  return typed;
}

bool matches(const std::vector<std::pair<std::string, std::uint8_t>> &entries,
             bool expected) {
  const DRW_UnsupportedObject raw = makeDictionaryFrame(entries);
  const DRW_Dictionary typed = makeTypedDictionary(entries);
  const char *reason = nullptr;
  const bool actual =
      dx_ac1027_material_wire::dictionary(raw, typed, 67u, &reason);
  if (actual != expected) {
    std::cerr << "AC1027 dictionary wire expected " << expected << " got "
              << actual << " reason=" << (reason == nullptr ? "<none>" : reason)
              << '\n';
    return false;
  }
  return true;
}

// Generated independently from the selected AC1024 empty-dictionary wire
// profile. No drawing or external fixture is required for this control.
bool checkAc1024EmptyNodDictionary(std::uint8_t handle) {
  BitWriter writer;
  writer.bits(0u, 2u); writer.bits(42u, 8u);
  writer.handle(0u, handle);
  writer.bits(2u, 2u); // no EED
  writer.longValue(1u); // one reactor
  writer.bits(1u, 1u); // no extension dictionary
  writer.bits(2u, 2u); // zero entries
  writer.shortValue(1u); // cloning
  writer.bits(0u, 8u); // not hard-owned
  writer.bits(0u, 1u); // no detached string stream
  writer.handle(4u, 0x0cu); // owner
  writer.handle(4u, 0x0cu); // reactor
  writer.bits(15u, 4u); // exact observed terminal bits, not a general rule
  if (writer.size() != 96u)
    return false;

  DRW_Dictionary source;
  source.handle = handle;
  source.parentHandle = 0x0cu;
  source.cloning = 1;
  source.hardOwner = 0;
  source.reactorHandles = {0x0cu};
  source.setDwgCommonObjectState(1, 1u, false);
  source.setDwgCommonLinkTailValidated(true);

  DRW_UnsupportedObject raw;
  raw.m_version = DRW::AC1024;
  raw.m_objectType = 42;
  raw.m_handle = handle;
  raw.m_parentHandle = 0x0cu;
  raw.m_numReactors = 1;
  raw.m_reactorHandles = {0x0cu};
  raw.m_xDictFlag = 1u;
  raw.m_commonHandleDataValidated = true;
  raw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Validated;
  raw.m_objectSize = 12u;
  raw.m_bodyBitSize = 36u;
  raw.m_rawBytes = std::move(writer).take();
  const auto check = [&](const DRW_UnsupportedObject& frame,
                         const DRW_Dictionary& dictionary) {
    return dx_layerstates_dwg_wire::observeAc1024EmptyNodDictionary(
        frame, dictionary, 0x0cu);
  };
  if (!check(raw, source))
    return false;
  // Every physical bit, including each terminal bit, is bound in this profile.
  for (std::size_t bit = 0; bit < raw.m_rawBytes.size() * 8u; ++bit) {
    auto corrupt = raw;
    corrupt.m_rawBytes[bit / 8u] ^=
        static_cast<std::uint8_t>(1u << (7u - bit % 8u));
    if (check(corrupt, source) || !check(raw, source))
      return false;
  }
  auto truncated = raw;
  truncated.m_rawBytes.pop_back();
  if (check(truncated, source))
    return false;
  auto extended = raw;
  extended.m_rawBytes.push_back(0xffu);
  ++extended.m_objectSize;
  extended.m_bodyBitSize += 8u;
  if (check(extended, source))
    return false;
  auto wrongVersion = raw;
  wrongVersion.m_version = DRW::AC1021;
  if (check(wrongVersion, source))
    return false;
  auto wrongProof = raw;
  wrongProof.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Unknown;
  if (check(wrongProof, source))
    return false;
  wrongProof = raw;
  wrongProof.m_commonHandleDataValidated = false;
  if (check(wrongProof, source))
    return false;
  wrongProof = raw;
  wrongProof.m_xDictFlag = 0u;
  if (check(wrongProof, source))
    return false;
  auto changed = source;
  changed.reactorHandles.front() = 0x0du;
  if (check(raw, changed))
    return false;
  changed = source;
  changed.m_entries.push_back({"unexpected", 0x10u});
  if (check(raw, changed))
    return false;
  changed = source;
  changed.flags = 1u;
  if (check(raw, changed)
      || dx_layerstates_dwg_wire::observeAc1024EmptyNodDictionary(raw, source, 0x0du))
    return false;
  return check(raw, source);
}
// Generated bit streams: these are controls, not physical source evidence.
bool checkAc1024MLineStyleWire() {
  const auto commonRaw = [](std::uint8_t handle, int type, std::uint32_t owner) {
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1024; raw.m_objectType = type; raw.m_handle = handle;
    raw.m_parentHandle = owner; raw.m_numReactors = 1; raw.m_reactorHandles = {owner};
    raw.m_xDictFlag = 1u; raw.m_commonHandleDataValidated = true;
    raw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Validated;
    return raw;
  };
  BitWriter dict;
  dict.bits(0u,2u); dict.bits(42u,8u); dict.handle(0u,0x17u);
  dict.bits(2u,2u); dict.longValue(1u); dict.bits(1u,1u); dict.longValue(1u);
  dict.shortValue(1u); dict.bits(0u,8u); dict.shortValue(8u);
  for (const char c : std::string("Standard")) dict.little16(static_cast<unsigned char>(c));
  dict.little16(138u); dict.bits(1u,1u); dict.handle(12u,11u);
  dict.handle(4u,0x0cu); dict.handle(2u,0x18u); dict.bits(3u,2u);
  if (dict.size() != 272u) return false;
  auto rootRaw = commonRaw(0x17u,42,0x0cu);
  rootRaw.m_objectSize = 34u; rootRaw.m_bodyBitSize = 50u;
  rootRaw.m_rawBytes = std::move(dict).take();
  DRW_Dictionary root; root.handle=0x17u; root.parentHandle=0x0cu;
  root.cloning=1; root.hardOwner=0; root.reactorHandles={0x0cu};
  root.m_entries.push_back({"Standard",0x18u});
  root.setDwgCommonObjectState(1,1u,false); root.setDwgCommonLinkTailValidated(true);
  const auto rootCheck = [&](const DRW_UnsupportedObject& raw) {
    return dx_ac1024_mlinestyle_wire::observeRoot(raw,root);
  };
  if (!rootCheck(rootRaw)) return false;
  for (std::size_t bit=0; bit<272u; ++bit) {
    auto changed=rootRaw; changed.m_rawBytes[bit/8u] ^= static_cast<std::uint8_t>(1u << (7u-bit%8u));
    if (rootCheck(changed) || !rootCheck(rootRaw)) return false;
  }
  auto truncatedRoot=rootRaw; truncatedRoot.m_rawBytes.pop_back();
  if (rootCheck(truncatedRoot)) return false;
  auto changedRoot=root;
  changedRoot.m_entries.front().m_dxfHandleCode = DRW_Dictionary::Entry::DxfHandleCode::HardOwner;
  if (dx_ac1024_mlinestyle_wire::observeRoot(rootRaw,changedRoot)) return false;

  BitWriter wire;
  const auto little = [&wire](std::uint64_t x,unsigned n) {
    for(unsigned i=0;i<n;++i) wire.bits((x>>(8u*i))&0xffu,8u);
  };
  const auto color = [&wire,&little]() {
    wire.bits(2u,2u); wire.bits(0u,2u); little(0xc0000000u,4u); wire.bits(0u,8u);
  };
  const auto number = [&wire,&little](std::uint64_t bits) {
    wire.bits(0u,2u); little(bits,8u);
  };
  const auto index = [&wire]() { wire.bits(0u,2u); wire.little16(32767u); };
  wire.bits(0u,2u); wire.bits(73u,8u); wire.handle(0u,0x18u);
  wire.bits(2u,2u); wire.longValue(1u); wire.bits(1u,1u); wire.bits(2u,2u);
  color(); number(0x3ff921fb54442d18ULL); number(0x3ff921fb54442d18ULL);
  wire.bits(2u,8u); number(0x3fe0000000000000ULL); color(); index();
  number(0xbfe0000000000000ULL); color(); index();
  wire.shortValue(8u); for(const char c : std::string("STANDARD")) wire.little16(static_cast<unsigned char>(c));
  wire.bits(2u,2u); wire.little16(140u); wire.bits(1u,1u);
  wire.bits(0x80u,8u); wire.handle(4u,0x17u); wire.bits(3u,2u);
  if (wire.size()!=664u) return false;
  auto raw=commonRaw(0x18u,73,0x17u); raw.m_objectSize=83u; raw.m_bodyBitSize=26u;
  raw.m_rawBytes=std::move(wire).take();
  DRW_MLineStyle style; style.handle=0x18u; style.parentHandle=0x17u; style.name="STANDARD";
  style.reactorHandles={0x17u}; style.setDwgCommonObjectState(1,1u,false);
  style.startAngle=90.0; style.endAngle=90.0; style.fillColor=256; style.fillColor24=-1;
  style.fillColorMethod=192u;
  for (int i=0;i<2;++i) { DRW_MLineElement e; e.offset=i==0?0.5:-0.5;
    e.color=256; e.color24=-1; e.colorMethod=192u; e.linetypeIndex=32767;
    style.elements.push_back(e); }
  const auto check = [&](const DRW_UnsupportedObject& frame, const DRW_MLineStyle& model) {
    return dx_ac1024_mlinestyle_wire::observeStyle(frame,model);
  };
  if (!check(raw,style)) return false;
  for (std::size_t bit=0;bit<664u;++bit) {
    auto changed=raw; changed.m_rawBytes[bit/8u] ^= static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if (check(changed,style) || !check(raw,style)) return false;
  }
  auto truncated=raw; truncated.m_rawBytes.pop_back(); if(check(truncated,style)) return false;
  auto extended=raw; extended.m_rawBytes.push_back(0xffu); ++extended.m_objectSize; extended.m_bodyBitSize+=8u;
  if(check(extended,style)) return false;
  auto changedStyle=style; changedStyle.elements[0].linetypeIndex=7;
  if(check(raw,changedStyle)) return false;
  changedStyle=style; changedStyle.reactorHandles.front()=0x0cu;
  if(check(raw,changedStyle)) return false;
  return check(raw,style);
}
} // namespace

int main() {
  if (!checkAc1024MLineStyleWire()) {
    std::cerr << "AC1024 MLINESTYLE source wire control failed\n";
    return 1;
  }
  if (!checkAc1024EmptyNodDictionary(0x73u)
      || !checkAc1024EmptyNodDictionary(0x19u)) {
    std::cerr << "AC1024 empty NOD dictionary wire control failed\n";
    return 1;
  }
  const std::vector<std::pair<std::string, std::uint8_t>> orderA{
      {"ByLayer", 68u}, {"ByBlock", 69u}, {"Global", 70u}};
  const std::vector<std::pair<std::string, std::uint8_t>> orderB{
      {"ByBlock", 69u}, {"ByLayer", 68u}, {"Global", 70u}};
  const std::vector<std::pair<std::string, std::uint8_t>> duplicate{
      {"ByLayer", 68u}, {"ByLayer", 69u}, {"Global", 70u}};

  if (!matches(orderA, true) || !matches(orderB, true) ||
      !matches(duplicate, false))
    return 1;

  const DRW_UnsupportedObject raw = makeDictionaryFrame(orderA);
  auto reordered = orderA;
  std::swap(reordered[0], reordered[1]);
  const DRW_Dictionary typed = makeTypedDictionary(reordered);
  const char *reason = nullptr;
  if (dx_ac1027_material_wire::dictionary(raw, typed, 67u, &reason) ||
      reason == nullptr ||
      std::string(reason) != "dictionary string value/order") {
    std::cerr << "reordered typed keys were not bound to their wire order\n";
    return 1;
  }

  DRW_UnsupportedObject badReactor = makeDictionaryFrame(orderA);
  // Body (68) + three strings (350) + string footer (17) + parent (16).
  // Change the reactor token role from soft pointer (4) to soft owner (2).
  overwriteBits(badReactor.m_rawBytes, 451u, 2u, 4u);
  reason = nullptr;
  if (dx_ac1027_material_wire::dictionary(
          badReactor, makeTypedDictionary(orderA), 67u, &reason) ||
      reason == nullptr || std::string(reason) != "reactor handle token") {
    std::cerr << "wrong reactor wire role was not refused\n";
    return 1;
  }

  std::cout << "AC1027 MATERIAL dictionary wire: key permutations, exact"
               " ordered name/handle pairs, and duplicate refusal passed\n";
  return 0;
}
