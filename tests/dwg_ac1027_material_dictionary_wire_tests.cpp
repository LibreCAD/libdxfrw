#include "dx_ac1027_material_wire.h"
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
} // namespace

int main() {
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
