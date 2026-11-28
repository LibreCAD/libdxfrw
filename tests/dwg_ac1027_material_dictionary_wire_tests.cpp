#include "dx_ac1027_material_wire.h"
#include "dx_ac1024_renderglobal_wire.h"
#include "dx_ac1024_mentalray_wire.h"
#include "dx_ac1024_mleaderstyle_wire.h"
#include "dx_ac1024_dictionaryvar_wire.h"
#include "dx_ac1021_dictionaryvar_wire.h"
#include "dx_current_tablestyle_reference.h"
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
// Locally generated controls for the selected AC1021 variable graph.
bool checkAc1021DictionaryVarWire() {
  using namespace dx_ac1021_dictionaryvar_wire;
  const auto h = [](BitWriter& w, std::uint8_t role, std::uint32_t value, unsigned length) {
    w.bits((role << 4u) | length, 8u);
    for (unsigned i = length; i > 0u; --i) w.bits((value >> ((i - 1u) * 8u)) & 255u, 8u);
  };
  const auto rl = [](BitWriter& w, std::uint32_t value) {
    for (unsigned i = 0; i < 4u; ++i) w.bits((value >> (8u * i)) & 255u, 8u);
  };
  const auto rawFor = [](std::uint32_t self, std::uint32_t owner, int type) {
    DRW_UnsupportedObject raw; raw.m_version = DRW::AC1021; raw.m_objectType = type;
    raw.m_handle = self; raw.m_parentHandle = owner; raw.m_numReactors = 1;
    raw.m_reactorHandles = {owner}; raw.m_xDictFlag = 1u;
    raw.m_commonHandleDataValidated = true;
    raw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Validated;
    if (type == 504) { raw.m_isCustomClass = true; raw.m_hasClassDefinition = true;
      raw.m_recordName = "DICTIONARYVAR"; raw.m_className = "AcDbDictionaryVar"; }
    return raw;
  };
  BitWriter d; d.shortValue(42u); rl(d, 310u); h(d, 0u, 0x5abu, 2u);
  d.bits(2u, 2u); d.longValue(1u); d.bits(1u, 1u); d.longValue(1u);
  d.shortValue(1u); d.bits(0u, 8u); const std::string key = "CTABLESTYLE";
  d.shortValue(static_cast<std::uint8_t>(key.size()));
  for (unsigned char c : key) d.little16(c);
  d.little16(186u); d.bits(1u, 1u); h(d, 4u, 0xcu, 1u);
  h(d, 4u, 0xcu, 1u); h(d, 2u, 0x5acu, 2u); d.bits(3u, 2u);
  if (d.size() != 368u) return false;
  auto owner = rawFor(0x5abu, 0xcu, 42); owner.m_objectSize = 46u;
  owner.m_rawBytes = std::move(d).take();
  DRW_Dictionary dictionary; dictionary.handle = 0x5abu; dictionary.parentHandle = 0xcu;
  dictionary.cloning = 1; dictionary.hardOwner = 0; dictionary.reactorHandles = {0xcu};
  dictionary.m_entries.push_back({key, 0x5acu}); dictionary.setDwgCommonObjectState(1, 1u, false);
  dictionary.setDwgCommonLinkTailValidated(true);
  if (!observeOwner(owner, dictionary)) return false;
  for (std::size_t bit = 0; bit < 368u; ++bit) {
    auto changed = owner; changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - bit % 8u));
    if (observeOwner(changed, dictionary) || !observeOwner(owner, dictionary)) return false;
  }
  BitWriter v; v.bits(0u, 2u); v.little16(504u); rl(v, 250u); h(v, 0u, 0x5acu, 2u);
  v.bits(2u, 2u); v.longValue(1u); v.bits(1u, 1u); v.bits(0u, 8u);
  const std::string text = "STANDARD"; v.shortValue(static_cast<std::uint8_t>(text.size()));
  for (unsigned char c : text) v.little16(c);
  v.little16(138u); v.bits(1u, 1u); h(v, 8u, 0u, 0u); h(v, 4u, 0x5abu, 2u); v.bits(63u, 6u);
  if (v.size() != 288u) return false;
  auto raw = rawFor(0x5acu, 0x5abu, 504); raw.m_objectSize = 36u;
  raw.m_rawBytes = std::move(v).take();
  DRW_DictionaryVar value; value.handle = 0x5acu; value.parentHandle = 0x5abu;
  value.m_value = text; value.reactorHandles = {0x5abu}; value.setDwgCommonObjectState(1, 1u, false);
  value.setDwgCommonLinkTailValidated(true);
  if (!observeVariable(raw, value)) return false;
  for (std::size_t bit = 0; bit < 288u; ++bit) {
    auto changed = raw; changed.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - bit % 8u));
    if (observeVariable(changed, value) || !observeVariable(raw, value)) return false;
  }
  auto changed = value; changed.m_value = "Standard";
  auto truncated = raw; truncated.m_rawBytes.pop_back();
  auto extended = raw; extended.m_rawBytes.push_back(0xffu); ++extended.m_objectSize;
  auto otherVersion = raw; otherVersion.m_version = DRW::AC1024;
  auto otherClass = raw; otherClass.m_className = "AcDbOther";
  auto noClass = raw; noClass.m_hasClassDefinition = false;
  auto wrongParent = raw; wrongParent.m_parentHandle = 0xcu;
  auto badOwner = dictionary; badOwner.m_entries.front().m_handle = 0x5adu;
  auto ownerTruncated = owner; ownerTruncated.m_rawBytes.pop_back();
  return !observeVariable(raw, changed) && !observeVariable(truncated, value)
      && !observeVariable(extended, value) && !observeVariable(otherVersion, value)
      && !observeVariable(otherClass, value) && !observeVariable(noClass, value)
      && !observeVariable(wrongParent, value) && !observeOwner(owner, badOwner)
      && !observeOwner(ownerTruncated, dictionary)
      && observeOwner(owner, dictionary) && observeVariable(raw, value);
}

// Generated source-format controls for the pinned AC1024 variable graph.
bool checkAc1024DictionaryVarWire() {
  using namespace dx_ac1024_dictionaryvar_wire;
  const auto handle = [](BitWriter& bits, std::uint8_t role, std::uint32_t value,
                         std::uint8_t length) {
    bits.bits((role << 4u) | length,8u);
    for (unsigned i=length;i>0u;--i) bits.bits((value >> ((i-1u)*8u)) & 0xffu,8u);
  };
  const auto common = [&handle](BitWriter& bits, std::uint16_t type,
                                 std::uint32_t self, std::uint8_t length) {
    if(type==42u){bits.bits(0u,2u);bits.bits(type,8u);}
    else {bits.bits(1u,2u);bits.bits(type-496u,8u);}
    handle(bits,0u,self,length); bits.bits(2u,2u); bits.longValue(1u); bits.bits(1u,1u);
  };
  const auto text = [](BitWriter& bits, const std::string& value) {
    bits.shortValue(static_cast<std::uint8_t>(value.size()));
    for(unsigned char c:value) bits.little16(c);
  };
  const auto rawFor = [](std::uint32_t self,std::uint32_t owner,int type) {
    DRW_UnsupportedObject raw; raw.m_version=DRW::AC1024;raw.m_objectType=type;
    raw.m_handle=self;raw.m_parentHandle=owner;raw.m_numReactors=1;
    raw.m_reactorHandles={owner};raw.m_xDictFlag=1u;raw.m_commonHandleDataValidated=true;
    raw.m_commonLinkEvidence=DRW_DwgCommonLinkEvidence::Validated;
    if(type==503){raw.m_isCustomClass=true;raw.m_hasClassDefinition=true;
      raw.m_recordName="DICTIONARYVAR";raw.m_className="AcDbDictionaryVar";}
    return raw;
  };
  BitWriter wire;common(wire,42u,0x66u,1u);wire.longValue(7u);
  wire.shortValue(1u);wire.bits(0u,8u);
  DRW_Dictionary root;root.handle=0x66u;root.parentHandle=0x0cu;root.cloning=1;
  root.reactorHandles={0x0cu};root.setDwgCommonObjectState(1,1u,false);
  root.setDwgCommonLinkTailValidated(true);
  for(const auto& item:entries){text(wire,item.name);root.m_entries.push_back({item.name,item.handle});}
  wire.little16(1190u);wire.bits(1u,1u);handle(wire,4u,0x0cu,1u);handle(wire,4u,0x0cu,1u);
  for(const auto& item:entries)handle(wire,2u,item.handle,item.handle>0xffu?2u:1u);
  wire.bits(63u,6u);if(wire.size()!=1456u)return false;
  auto raw=rawFor(0x66u,0x0cu,42);raw.m_objectSize=182u;raw.m_bodyBitSize=182u;
  raw.m_rawBytes=std::move(wire).take();
  if(!matchesProfile(root)||!observeRoot(raw,root))return false;
  for(std::size_t bit=0;bit<1456u;++bit){auto bad=raw;
    bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if(observeRoot(bad,root)||!observeRoot(raw,root))return false;}
  auto badRoot=root;badRoot.m_entries[0].m_handle=0x161u;
  if(matchesProfile(badRoot)||observeRoot(raw,badRoot))return false;
  auto truncated=raw;truncated.m_rawBytes.pop_back();if(observeRoot(truncated,root))return false;
  for(const auto& item:entries){
    BitWriter value;common(value,503u,item.handle,item.selfLength);value.bits(0u,8u);
    text(value,item.value);value.little16(static_cast<std::uint16_t>(10u+16u*std::string(item.value).size()));
    value.bits(1u,1u);
    const auto ownerValue=item.ownerRole==12u?item.handle-0x66u:(item.ownerRole==8u?0u:0x66u);
    handle(value,item.ownerRole,ownerValue,item.ownerLength);handle(value,4u,0x66u,1u);value.bits(63u,6u);
    if(value.size()!=item.bytes*8u)return false;
    auto frame=rawFor(item.handle,0x66u,503);frame.m_objectSize=item.bytes;
    frame.m_bodyBitSize=item.handle==0x67u?30u:38u;frame.m_rawBytes=std::move(value).take();
    DRW_DictionaryVar model;model.handle=item.handle;model.parentHandle=0x66u;
    model.m_value=item.value;model.reactorHandles={0x66u};model.setDwgCommonObjectState(1,1u,false);
    model.setDwgCommonLinkTailValidated(true);
    if(!matchesProfile(model)||!observeVariable(frame,model))return false;
    for(std::size_t bit=0;bit<frame.m_rawBytes.size()*8u;++bit){auto bad=frame;
      bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
      if(observeVariable(bad,model)||!observeVariable(frame,model))return false;}
    auto badFrame=frame;badFrame.m_rawBytes.pop_back();if(observeVariable(badFrame,model))return false;
    badFrame=frame;badFrame.m_rawBytes.push_back(0xffu);++badFrame.m_objectSize;badFrame.m_bodyBitSize+=8u;
    if(observeVariable(badFrame,model))return false;
    auto badModel=model;badModel.m_value="other";
    if(matchesProfile(badModel)||observeVariable(frame,badModel))return false;
    badModel=model;badModel.reactorHandles.front()=0x0cu;
    if(matchesProfile(badModel)||observeVariable(frame,badModel))return false;
    if(!observeVariable(frame,model))return false;
  }
  return true;
}
// Generated exact source-layout controls; no drawing bytes are embedded.
bool checkAc1024RenderGlobalWire() {
  using namespace dx_ac1024_renderglobal_wire;
  const auto h=[](BitWriter& w,std::uint8_t role,std::uint32_t value,std::uint8_t length) {
    w.bits((role<<4u)|length,8u);
    for(unsigned i=length;i>0u;--i)w.bits((value>>((i-1u)*8u))&0xffu,8u);
  };
  const auto common=[&h](BitWriter& w,int type,std::uint32_t self,int reactors,int xflag){
    w.bits(type>=496?1u:0u,2u);w.bits(type>=496?type-496:type,8u);h(w,0u,self,2u);
    w.bits(2u,2u);if(reactors)w.longValue(1u);else w.bits(2u,2u);w.bits(xflag,1u);
  };
  const auto long32=[](BitWriter& w,std::uint32_t value){w.bits(0u,2u);for(unsigned i=0;i<4u;++i)w.bits((value>>(8u*i))&0xffu,8u);};
  const auto makeRaw=[](std::uint32_t self,int type,std::uint32_t owner,int reactors,int xflag){
    DRW_UnsupportedObject raw;raw.m_version=DRW::AC1024;raw.m_handle=self;raw.m_objectType=type;
    raw.m_parentHandle=owner;raw.m_numReactors=reactors;if(reactors)raw.m_reactorHandles={owner};
    raw.m_xDictFlag=xflag;raw.m_commonHandleDataValidated=true;
    raw.m_commonLinkEvidence=DRW_DwgCommonLinkEvidence::Validated;
    if(type==521){raw.m_isCustomClass=true;raw.m_hasClassDefinition=true;
      raw.m_recordName="RENDERGLOBAL";raw.m_className="AcDbRenderGlobal";raw.m_xDictHandle=0x251u;}
    return raw;
  };
  BitWriter global;common(global,521,0x237u,1,0);global.longValue(2u);global.bits(2u,2u);
  global.bits(2u,2u);global.bits(0u,1u);long32(global,640u);long32(global,480u);
  global.bits(1u,1u);global.bits(1u,1u);global.bits(2u,2u);global.little16(2u);global.bits(1u,1u);
  h(global,4u,0x0cu,1u);h(global,4u,0x0cu,1u);h(global,3u,0x251u,2u);global.bits(1u,1u);
  if(global.size()!=208u)return false;
  auto gr=makeRaw(0x237u,521,0x0cu,1,0);gr.m_objectSize=26u;gr.m_bodyBitSize=57u;gr.m_rawBytes=std::move(global).take();
  DRW_RenderSettings g;g.handle=0x237u;g.parentHandle=0x0cu;g.m_kind=DRW_RenderSettings::Global;
  g.reactorHandles={0x0cu};g.xDictHandle=0x251u;g.setDwgCommonObjectState(1,0u,false);
  g.m_classVersion=2;g.m_longs={2,0,0,640,480};g.m_strings={""};g.m_bools={false,true,true};
  if(!matchesProfile(g)||!observeGlobal(gr,g))return false;
  for(std::size_t bit=0;bit<208u;++bit){auto bad=gr;bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if(observeGlobal(bad,g)||!observeGlobal(gr,g))return false;}
  auto bg=g;bg.m_longs[3]=641;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_name="unexpected";if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_environmentImageFilename="unexpected";if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_description="unexpected";if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_displayIndex=1;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogEnabled=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogBackgroundEnabled=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_backfacesEnabled=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_environmentImageEnabled=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_hasPredefined=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_hasPredefinedPresent=true;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogColorR=1;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogColorG=1;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogColorB=1;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogDensityNear=1.0;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogDensityFar=1.0;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogDistanceNear=1.0;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.m_fogDistanceFar=1.0;if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  bg=g;bg.setDwgCommonLinkTailValidated(true);if(matchesProfile(bg)||observeGlobal(gr,bg))return false;
  auto gt=gr;gt.m_rawBytes.pop_back();if(observeGlobal(gt,g))return false;

  BitWriter dictionary;common(dictionary,42,0x251u,0,1);dictionary.longValue(1u);dictionary.shortValue(1u);dictionary.bits(1u,8u);
  const std::string key="ADSK_XREC_RENDERGLOBALINFO";dictionary.shortValue(static_cast<std::uint8_t>(key.size()));
  for (unsigned char c : key) dictionary.little16(c);
  dictionary.little16(426u); dictionary.bits(1u, 1u);
  h(dictionary,12u,0x1au,1u);h(dictionary,2u,0x252u,2u);dictionary.bits(3u,2u);
  if(dictionary.size()!=552u)return false;
  auto dr=makeRaw(0x251u,42,0x237u,0,1);dr.m_objectSize=69u;dr.m_bodyBitSize=42u;dr.m_rawBytes=std::move(dictionary).take();
  DRW_Dictionary d;d.handle=0x251u;d.parentHandle=0x237u;d.cloning=1;d.hardOwner=1;
  d.m_entries.push_back({key,0x252u});d.setDwgCommonObjectState(0,1u,false);d.setDwgCommonLinkTailValidated(true);
  if(!matchesProfile(d)||!observeDictionary(dr,d))return false;
  for(std::size_t bit=0;bit<552u;++bit){auto bad=dr;bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if(observeDictionary(bad,d)||!observeDictionary(dr,d))return false;}
  auto bd=d;bd.m_entries.front().m_handle=0x253u;if(matchesProfile(bd)||observeDictionary(dr,bd))return false;
  auto dt=dr;dt.m_rawBytes.pop_back();if(observeDictionary(dt,d))return false;

  BitWriter record;common(record,79,0x252u,1,1);record.longValue(4u);record.little16(70u);record.little16(0u);
  record.shortValue(1u);record.bits(0u,1u);h(record,8u,0u,0u);h(record,4u,0x251u,2u);record.bits(15u,4u);
  if(record.size()!=136u)return false;
  auto xr=makeRaw(0x252u,79,0x251u,1,1);xr.m_objectSize=17u;xr.m_bodyBitSize=36u;xr.m_rawBytes=std::move(record).take();
  DRW_XRecord x;x.handle=0x252u;x.parentHandle=0x251u;x.m_cloning=1;x.reactorHandles={0x251u};
  x.setDwgCommonObjectState(1,1u,false);x.setDwgCommonLinkTailValidated(true);
  x.m_values.emplace_back(70,static_cast<std::int32_t>(0));x.m_dataEntries.emplace_back(70,static_cast<std::int32_t>(0));
  if(!matchesProfile(x)||!observeXrecord(xr,x))return false;
  for(std::size_t bit=0;bit<136u;++bit){auto bad=xr;bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if(observeXrecord(bad,x)||!observeXrecord(xr,x))return false;}
  auto bx=x;bx.m_values.front()=DRW_Variant(70,static_cast<std::int32_t>(1));if(matchesProfile(bx)||observeXrecord(xr,bx))return false;
  auto xt=xr;xt.m_rawBytes.pop_back();if(observeXrecord(xt,x))return false;
  return observeGlobal(gr,g)&&observeDictionary(dr,d)&&observeXrecord(xr,x);
}
bool checkAc1024MentalRayCompanionWire() {
  BitWriter w;
  const auto h=[](BitWriter& b,unsigned role,unsigned value,unsigned n) {
    b.bits((role<<4u)|n,8u);for(unsigned i=n;i>0u;--i)b.bits((value>>((i-1u)*8u))&255u,8u);
  };
  w.bits(0u,2u);w.bits(79u,8u);h(w,0u,0x1ceu,2u);w.bits(2u,2u);
  w.longValue(1u);w.bits(1u,1u);w.longValue(12u);
  for(unsigned i=0;i<3u;++i){w.little16(static_cast<std::uint16_t>(70u+i));w.little16(i==0u?2u:i==1u?4u:0u);}
  w.shortValue(1u);w.bits(0u,1u);h(w,8u,0u,0u);h(w,4u,0x1cdu,2u);w.bits(15u,4u);
  if(w.size()!=200u)return false;
  DRW_UnsupportedObject raw;raw.m_version=DRW::AC1024;raw.m_objectType=79;raw.m_handle=0x1ceu;
  raw.m_parentHandle=0x1cdu;raw.m_numReactors=1;raw.m_reactorHandles={0x1cdu};raw.m_xDictFlag=1u;
  raw.m_commonHandleDataValidated=true;raw.m_commonLinkEvidence=DRW_DwgCommonLinkEvidence::Validated;
  raw.m_objectSize=25u;raw.m_bodyBitSize=36u;raw.m_rawBytes=std::move(w).take();
  DRW_XRecord source;source.handle=0x1ceu;source.parentHandle=0x1cdu;source.m_cloning=1;
  source.reactorHandles={0x1cdu};source.setDwgCommonObjectState(1,1u,false);source.setDwgCommonLinkTailValidated(true);
  for(int i=0;i<3;++i){const int value=i==0?2:i==1?4:0;source.m_values.emplace_back(70+i,value);source.m_dataEntries.emplace_back(70+i,value);}
  if(!dx_ac1024_mentalray_wire::observeXrecord(raw,source))return false;
  for(std::size_t bit=0;bit<200u;++bit){auto bad=raw;bad.m_rawBytes[bit/8u]^=static_cast<std::uint8_t>(1u<<(7u-bit%8u));
    if(dx_ac1024_mentalray_wire::observeXrecord(bad,source)||!dx_ac1024_mentalray_wire::observeXrecord(raw,source))return false;}
  auto truncated=raw;truncated.m_rawBytes.pop_back();
  auto edited=source;edited.m_values[1]=DRW_Variant(71,5);
  return !dx_ac1024_mentalray_wire::observeXrecord(truncated,source)
      && !dx_ac1024_mentalray_wire::observeXrecord(raw,edited)
      && dx_ac1024_mentalray_wire::observeXrecord(raw,source);
}
bool checkAc1024MLeaderStyleWire() {
  using namespace dx_ac1024_mleaderstyle_wire;
  const auto h = [](BitWriter& b, unsigned role, unsigned value, unsigned n) {
    b.bits((role << 4u) | n, 8u);
    for (unsigned i = n; i > 0u; --i) {
      b.bits((value >> ((i - 1u) * 8u)) & 255u, 8u);
    }
  };
  const auto common = [&h](BitWriter& b, unsigned type, unsigned self) {
    b.bits(type >= 496u ? 1u : 0u, 2u);
    b.bits(type >= 496u ? type - 496u : type, 8u);
    h(b, 0u, self, 2u);
    b.bits(2u, 2u);
    b.longValue(1u);
    b.bits(1u, 1u);
  };
  const auto long32 = [](BitWriter& b, std::uint32_t v) {
    b.bits(0u, 2u);
    for (unsigned i = 0; i < 4u; ++i) {
      b.bits((v >> (8u * i)) & 255u, 8u);
    }
  };
  const auto bd = [](BitWriter& b, double v) {
    if (v == 0.0) {
      b.bits(2u, 2u);
      return;
    }
    if (v == 1.0) {
      b.bits(1u, 2u);
      return;
    }
    b.bits(0u, 2u);
    std::uint64_t u = 0;
    std::memcpy(&u, &v, sizeof(u));
    for (unsigned i = 0; i < 8u; ++i) {
      b.bits((u >> (8u * i)) & 255u, 8u);
    }
  };
  const auto cmc = [&long32](BitWriter& b) {
    b.bits(2u, 2u);
    long32(b, 0xc1000000u);
    b.bits(0u, 8u);
  };
  for (const bool standard : {true, false}) {
    const unsigned handle = standard ? 0x148u : 0x157u;
    BitWriter b;
    common(b, 513u, handle);
    b.shortValue(2u);
    b.shortValue(2u);
    b.shortValue(1u);
    b.bits(2u, 2u);
    b.longValue(2u);
    bd(b, 0.0);
    bd(b, 0.0);
    b.shortValue(1u);
    cmc(b);
    long32(b, 0xfffffffeu);
    b.bits(1u, 1u);
    bd(b, 0.09);
    b.bits(1u, 1u);
    bd(b, 0.36);
    bd(b, 0.18);
    b.shortValue(1u);
    b.shortValue(1u);
    b.shortValue(1u);
    b.bits(2u, 2u);
    cmc(b);
    bd(b, 0.18);
    b.bits(0u, 1u);
    b.bits(0u, 1u);
    bd(b, 0.18);
    cmc(b);
    bd(b, 1.0);
    bd(b, 1.0);
    bd(b, 1.0);
    b.bits(1u, 1u);
    bd(b, 0.0);
    b.bits(1u, 1u);
    b.bits(2u, 2u);
    bd(b, 1.0);
    b.bits(standard ? 0u : 1u, 1u);
    b.bits(1u, 1u);
    bd(b, 0.125);
    b.bits(2u, 2u);
    b.shortValue(9u);
    b.shortValue(9u);
    if (b.size() != 739u) {
      return false;
    }
    b.shortValue(8u);
    for (unsigned char c : std::string("Standard")) {
      b.little16(c);
    }
    b.bits(2u, 2u);
    b.little16(140u);
    b.bits(1u, 1u);
    if (standard) {
      h(b, 8u, 0u, 0u);
    } else {
      h(b, 12u, 0x10u, 1u);
    }
    h(b, 4u, 0x147u, 2u);
    h(b, 5u, 0x14u, 1u);
    h(b, 5u, 0u, 0u);
    h(b, 5u, 0x11u, 1u);
    h(b, 5u, 0u, 0u);
    const unsigned size = standard ? 122u : 123u;
    if (b.size() != size * 8u) {
      return false;
    }
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1024;
    raw.m_objectType = 513;
    raw.m_handle = handle;
    raw.m_parentHandle = 0x147u;
    raw.m_numReactors = 1;
    raw.m_reactorHandles = {0x147u};
    raw.m_xDictFlag = 1u;
    raw.m_isCustomClass = true;
    raw.m_hasClassDefinition = true;
    raw.m_recordName = "MLEADERSTYLE";
    raw.m_className = "AcDbMLeaderStyle";
    raw.m_classAppName = "ACDB_MLEADERSTYLE_CLASS";
    raw.m_commonHandleDataValidated = true;
    raw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Validated;
    raw.m_objectSize = size;
    raw.m_bodyBitSize = size * 8u - 896u;
    raw.m_rawBytes = std::move(b).take();
    DRW_MLeaderStyle s;
    s.handle = handle;
    s.parentHandle = 0x147u;
    s.reactorHandles = {0x147u};
    s.setDwgCommonObjectState(1, 1u, false);
    s.styleVersion = 2;
    s.contentType = 2;
    s.drawMLeaderOrder = 1;
    s.drawLeaderOrder = 0;
    s.maxLeaderPoints = 2;
    s.leaderType = 1;
    s.leaderColor = 0;
    s.leaderColorDwgMethod = 0xc1u;
    s.leaderColorDwgMetadataPresent = true;
    s.leaderLineWeight = -2;
    s.landingEnabled = true;
    s.landingGap = 0.09;
    s.autoIncludeLanding = true;
    s.landingDistance = 0.36;
    s.description = "Standard";
    s.arrowHeadSize = 0.18;
    s.leftAttachment = 1;
    s.rightAttachment = 1;
    s.textAngleType = 1;
    s.textAlignmentType = 0;
    s.textColor = 0;
    s.textColorDwgMethod = 0xc1u;
    s.textColorDwgMetadataPresent = true;
    s.textHeight = 0.18;
    s.alignSpace = 0.18;
    s.blockColor = 0;
    s.blockColorDwgMethod = 0xc1u;
    s.blockColorDwgMetadataPresent = true;
    s.blockScaleEnabled = true;
    s.blockRotationEnabled = true;
    s.scaleFactor = 1;
    s.propertyChanged = !standard;
    s.isAnnotative = true;
    s.breakSize = 0.125;
    s.attachmentDirection = 0;
    s.topAttachment = 9;
    s.bottomAttachment = 9;
    const auto typed = [](dwgHandle& target, unsigned n, unsigned ref) {
      target.code = 5;
      target.rawCode = 5;
      target.size = n;
      target.ref = ref;
      target.ref64 = ref;
    };
    typed(s.leaderLineTypeHandle, 1, 0x14);
    typed(s.arrowHeadBlockHandle, 0, 0);
    typed(s.textStyleHandle, 1, 0x11);
    typed(s.blockHandle, 0, 0);
    if (!observeStyle(raw, s)) {
      return false;
    }
    for (std::size_t bit = 0; bit < raw.m_rawBytes.size() * 8u; ++bit) {
      auto x = raw;
      x.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - bit % 8u));
      if (observeStyle(x, s)) {
        return false;
      }
    }
    auto truncated = raw;
    truncated.m_rawBytes.pop_back();
    auto edited = s;
    edited.textHeight = 0.5;
    if (observeStyle(truncated, s) || observeStyle(raw, edited) || !observeStyle(raw, s)) {
      return false;
    }
  }
  BitWriter b;
  common(b, 42u, 0x147u);
  b.longValue(2u);
  b.shortValue(1u);
  b.bits(0u, 8u);
  for (const auto& name : {std::string("Annotative"), std::string("Standard")}) {
    b.shortValue(static_cast<std::uint8_t>(name.size()));
    for (unsigned char c : name) {
      b.little16(c);
    }
  }
  b.little16(308u);
  b.bits(1u, 1u);
  h(b, 4u, 0xcu, 1u);
  h(b, 4u, 0xcu, 1u);
  h(b, 2u, 0x157u, 2u);
  h(b, 2u, 0x148u, 2u);
  if (b.size() != 480u) {
    return false;
  }
  DRW_UnsupportedObject raw;
  raw.m_version = DRW::AC1024;
  raw.m_objectType = 42;
  raw.m_handle = 0x147u;
  raw.m_parentHandle = 0xcu;
  raw.m_numReactors = 1;
  raw.m_reactorHandles = {0xcu};
  raw.m_xDictFlag = 1u;
  raw.m_commonHandleDataValidated = true;
  raw.m_commonLinkEvidence = DRW_DwgCommonLinkEvidence::Validated;
  raw.m_objectSize = 60u;
  raw.m_bodyBitSize = 80u;
  raw.m_rawBytes = std::move(b).take();
  DRW_Dictionary d;
  d.handle = 0x147u;
  d.parentHandle = 0xcu;
  d.reactorHandles = {0xcu};
  d.setDwgCommonObjectState(1, 1u, false);
  d.setDwgCommonLinkTailValidated(true);
  d.cloning = 1;
  d.hardOwner = 0;
  d.m_entries.push_back({"Annotative", 0x157u});
  d.m_entries.push_back({"Standard", 0x148u});
  if (!observeDictionary(raw, d)) {
    return false;
  }
  for (std::size_t bit = 0; bit < 480u; ++bit) {
    auto x = raw;
    x.m_rawBytes[bit / 8u] ^= static_cast<std::uint8_t>(1u << (7u - bit % 8u));
    if (observeDictionary(x, d)) {
      return false;
    }
  }
  auto truncated = raw;
  truncated.m_rawBytes.pop_back();
  auto edited = d;
  edited.m_entries[0].m_handle = 0x148u;
  return !observeDictionary(truncated, d) && !observeDictionary(raw, edited) && observeDictionary(raw, d);
}
bool checkCurrentTableStyleReference() {
  using dx_current_tablestyle_reference::resolves;
  DRW_Dictionary dictionary;
  dictionary.handle = 0x20u;
  dictionary.m_entries.push_back({"Standard", 0x21u});
  DRW_TableStyle style;
  style.handle = 0x21u;
  style.parentHandle = dictionary.handle;
  style.m_name = "Standard";
  std::map<std::uint32_t, DRW_TableStyle> styles{{style.handle, style}};
  if (!resolves("STANDARD", dictionary, styles)
      || !resolves("sTaNdArD", dictionary, styles)
      || resolves("", dictionary, styles)
      || resolves("NO_SUCH_STYLE", dictionary, styles)) return false;
  dictionary.m_entries.push_back({"sTaNdArD", 0x21u});
  if (resolves("STANDARD", dictionary, styles)) return false;
  dictionary.m_entries.pop_back();
  styles.at(0x21u).parentHandle = 0x22u;
  if (resolves("STANDARD", dictionary, styles)) return false;
  styles.at(0x21u).parentHandle = dictionary.handle;
  styles.at(0x21u).handle = 0x22u;
  if (resolves("STANDARD", dictionary, styles)) return false;
  styles.at(0x21u).handle = 0x21u;
  styles.at(0x21u).m_name = "Different";
  if (resolves("STANDARD", dictionary, styles)) return false;
  styles.at(0x21u).m_name = "Standard";
  dictionary.m_entries.front().m_handle = 0x22u;
  if (resolves("STANDARD", dictionary, styles)) return false;
  dictionary.m_entries.front().m_handle = 0x21u;
  return resolves("STANDARD", dictionary, styles);
}
} // namespace

int main() {
  if (!checkCurrentTableStyleReference()) {
    std::cerr << "CTABLESTYLE target-name resolution control failed\n";
    return 1;
  }
  if (!checkAc1024MLeaderStyleWire()) {
    std::cerr << "AC1024 MLEADERSTYLE source wire control failed\n";
    return 1;
  }
  if (!checkAc1024MentalRayCompanionWire()) {
    std::cerr << "AC1024 MentalRay companion wire control failed\n";
    return 1;
  }
  if (!checkAc1024RenderGlobalWire()) {
    std::cerr << "AC1024 RENDERGLOBAL source wire control failed\n";
    return 1;
  }
  if (!checkAc1021DictionaryVarWire()) {
    std::cerr << "AC1021 DICTIONARYVAR source wire control failed\n";
    return 1;
  }
  if (!checkAc1024DictionaryVarWire()) {
    std::cerr << "AC1024 DICTIONARYVAR source wire control failed\n";
    return 1;
  }
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
