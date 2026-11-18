#include "dx_iface.h"
#include "dx_raster_dwg_wire.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
class DxIfaceStandardDwgCaptureTestAccess {
public:
  static const DRW_UnsupportedObject *frame(const dx_data &d, std::uint32_t h) {
    auto it = d.m_standardDwgObjectFrames.find({d.importGeneration, h});
    return it == d.m_standardDwgObjectFrames.end() ? nullptr : &it->second.raw;
  }
  static bool frameBound(const dx_data &d, std::uint32_t h,
                         int objectType = 42) {
    auto key = dx_data::StandardDwgFrameKey{d.importGeneration, h};
    auto it = d.m_standardDwgObjectFrames.find(key);
    if (d.importGeneration == 0u ||
        d.m_opaqueDwgSourceFrameIdentities.count(key) != 1u ||
        it == d.m_standardDwgObjectFrames.end())
      return false;
    const auto &frame = it->second;
    const auto &raw = frame.raw;
    return frame.generation == d.importGeneration && frame.payloadMatched &&
           frame.typedBaseline && !frame.opaquePayload && raw.m_handle == h &&
           raw.m_version == DRW::AC1024 && raw.m_objectType == objectType &&
           !raw.m_isEntity && !raw.m_hasDataStorage &&
           raw.m_objectSize == raw.m_rawBytes.size();
  }
  static const DRW_UnsupportedObject *opaque(const dx_data &d,
                                             std::uint32_t h) {
    auto key = dx_data::StandardDwgFrameKey{d.importGeneration, h};
    auto it = d.m_opaqueDwgSourceFrames.find(key);
    return d.importGeneration == 0u ||
                   d.m_opaqueDwgSourceFrameIdentities.count(key) != 1u ||
                   it == d.m_opaqueDwgSourceFrames.end()
               ? nullptr
               : &it->second;
  }
  static const DRW_AppId *app(const dx_data &d, std::uint32_t h) {
    auto it = d.m_standardDwgAppIdSources.find({d.importGeneration, h});
    return it == d.m_standardDwgAppIdSources.end() ? nullptr : it->second.get();
  }
};
struct DictExpectation {
  std::uint32_t handle, parent, bodyEnd, bodyFieldsEnd, stringBits;
  unsigned ownerRole;
  int hardOwner;
  std::vector<std::pair<std::string, std::uint32_t>> items;
};

bool observeDictionary(const dx_data &data, const DictExpectation &expected,
                       const std::vector<std::uint8_t> &bytes,
                       std::uint32_t handleBits, bool verbose) {
  using namespace dx_raster_wire;
  if (bytes.empty() || bytes.size() > 1024u || handleBits >= bytes.size() * 8u)
    return false;
  const DRW_Dictionary *typed = nullptr;
  if (expected.handle == 0xCu && data.hasNamedObjectDictionary) {
    if (std::any_of(
            data.dictionaries.begin(), data.dictionaries.end(),
            [](const DRW_Dictionary &row) { return row.handle == 0xCu; }))
      return false;
    typed = &data.namedObjectDictionary;
  } else
    for (const auto &row : data.dictionaries)
      if (row.handle == expected.handle) {
        if (typed)
          return false;
        typed = &row;
      }
  if (!typed ||
      !DxIfaceStandardDwgCaptureTestAccess::frameBound(data, expected.handle) ||
      typed->parentHandle != expected.parent || typed->xDictHandle != 0u ||
      !typed->appData.empty() || typed->cloning != 1 ||
      typed->hardOwner != expected.hardOwner ||
      typed->m_entries.size() != expected.items.size() ||
      typed->reactorHandles.size() !=
          ((expected.handle == 0xD5u || expected.handle == 0x24Au ||
            expected.handle == 0xCu)
               ? 0u
               : 1u))
    return false;
  for (std::size_t i = 0; i < expected.items.size(); ++i)
    if (typed->m_entries[i].m_name != expected.items[i].first ||
        typed->m_entries[i].m_handle != expected.items[i].second)
      return false;
  if (!typed->reactorHandles.empty() &&
      typed->reactorHandles[0] != (expected.handle == 0x126u ? 0xCu : 0xD5u))
    return false;

  const auto total = static_cast<std::uint32_t>(bytes.size() * 8u);
  const auto bodyEnd = total - handleBits;
  if (bodyEnd != expected.bodyEnd || bodyEnd < 1u)
    return false;
  detail::Bits body{bytes.data(), 0u, bodyEnd};
  const auto tag = body.take(2u);
  const auto type = tag == 0u   ? body.take(8u)
                    : tag == 1u ? body.take(8u) + 0x1f0u
                                : body.little(2u);
  const auto self = detail::handle(body, 0u, 0u, true);
  if (!body.good || type != 42u || self.value != expected.handle)
    return false;
  Proof eedProof;
  auto visitEed = [&](const EedToken &token) noexcept {
    if (expected.handle != 0xEDu || token.index >= typed->extData.size() ||
        !typed->extData[token.index] ||
        typed->extData[token.index]->code() != static_cast<int>(token.code) ||
        typed->extData[token.index]->type() != DRW_Variant::STRING ||
        !typed->extData[token.index]->content.s)
      return false;
    const auto &value = *typed->extData[token.index]->content.s;
    if (token.index == 0u)
      return token.code == 1001u && token.role == 5u && token.value == 0x12u &&
             token.span.begin == 36u && token.span.end == 52u &&
             value == "ACAD";
    if (token.index != 1u || token.code != 1000u || value != "<All" ||
        token.valueSpan.begin != 60u || token.valueSpan.end != 140u)
      return false;
    detail::Bits text{bytes.data(), token.valueSpan.begin, token.valueSpan.end};
    if (text.little(2u) != value.size())
      return false;
    for (unsigned char c : value)
      if (text.little(2u) != c)
        return false;
    return text.good && text.pos == text.end;
  };
  if (detail::eed(body, 2010u, eedProof, visitEed) != Status::Proven ||
      eedProof.eedVariants != (expected.handle == 0xEDu ? 2u : 0u) ||
      typed->extData.size() != eedProof.eedVariants)
    return false;
  if (body.longValue() != typed->reactorHandles.size() || body.take(1u) != 1u ||
      body.longValue() != expected.items.size() || body.shortValue() != 1u ||
      body.take(8u) != static_cast<unsigned>(expected.hardOwner) ||
      !body.good || body.pos != expected.bodyFieldsEnd)
    return false;
  detail::Bits presence{bytes.data(), bodyEnd - 1u, bodyEnd};
  const bool hasStrings = presence.take(1u) != 0u;
  if (!presence.good || hasStrings != !expected.items.empty())
    return false;
  if (hasStrings) {
    if (bodyEnd < 17u)
      return false;
    detail::Bits footer{bytes.data(), bodyEnd - 17u, bodyEnd - 1u};
    const auto stringBits = footer.little(2u);
    if (!footer.good || stringBits != expected.stringBits ||
        stringBits > bodyEnd - 17u || body.pos != bodyEnd - 17u - stringBits)
      return false;
    detail::Bits text{bytes.data(), body.pos, bodyEnd - 17u};
    std::size_t textIndex = 0u;
    for (const auto &entry : expected.items) {
      const auto start = text.pos;
      if (text.shortValue() != entry.first.size())
        return false;
      for (unsigned char c : entry.first)
        if (text.little(2u) != c)
          return false;
      if (expected.handle == 0xCu && textIndex == 12u &&
          (start != 3235u || text.pos != 3469u))
        return false;
      if (expected.handle == 0xCu && verbose)
        std::cout << "nod-string=" << textIndex << " span=" << start << ":"
                  << text.pos << " name=" << entry.first << "\n";
      ++textIndex;
    }
    if (!text.good || text.pos != text.end)
      return false;
  } else if (body.pos != bodyEnd - 1u || expected.stringBits != 0u) {
    return false;
  }
  detail::Bits wire{bytes.data(), bodyEnd, total};
  const auto ownerStart = wire.pos;
  const auto owner = detail::handle(wire, 4u, expected.handle);
  if (!wire.good || owner.role != expected.ownerRole ||
      owner.value != expected.parent ||
      wire.pos !=
          ownerStart + ((expected.ownerRole == 8u || expected.handle == 0xCu)
                            ? 8u
                            : 16u))
    return false;
  if (verbose)
    std::cout << "dictionary=" << std::hex << expected.handle << std::dec
              << " body=0:" << expected.bodyFieldsEnd
              << " strings=" << expected.bodyFieldsEnd << ":"
              << bodyEnd - (hasStrings ? 17u : 1u)
              << " footer=" << bodyEnd - (hasStrings ? 17u : 1u) << ":"
              << bodyEnd << " owner=" << ownerStart << ":" << wire.pos
              << " role=" << unsigned(owner.role) << " target=" << std::hex
              << owner.value << std::dec;
  if (!typed->reactorHandles.empty()) {
    const auto start = wire.pos;
    const auto reactor = detail::handle(wire, 4u, expected.handle);
    if (!wire.good || reactor.role != 4u ||
        reactor.value != (expected.handle == 0x126u ? 0xCu : 0xD5u) ||
        wire.pos != start + 16u)
      return false;
    if (verbose)
      std::cout << " reactor=" << start << ":" << wire.pos;
  }
  for (const auto &entry : expected.items) {
    const auto start = wire.pos;
    const auto item = detail::handle(wire, 2u, expected.handle);
    if (!wire.good || item.role != 2u || item.value != entry.second ||
        wire.pos != start + (entry.second <= 0xFFu ? 16u : 24u))
      return false;
    if (verbose)
      std::cout << " item=" << start << ":" << wire.pos << ":" << std::hex
                << item.value << std::dec;
  }
  const auto expectedEnd =
      expected.handle == 0xCu                                    ? 4668u
      : expected.handle == 0x126u                                ? 2236u
      : expected.handle == 0xD5u                                 ? 930u
      : (expected.handle == 0x1B8u || expected.handle == 0x24Au) ? 494u
      : expected.handle == 0xEEu                                 ? 92u
                                                                 : 206u;
  if (wire.pos != expectedEnd)
    return false;
  const auto padding = wire.end - wire.pos;
  if (padding >= 8u)
    return false;
  if (verbose)
    std::cout << " padding=" << wire.pos << ":" << wire.end << "\n";
  return wire.skip(padding) && wire.good;
}

bool testDictionaryFrames(const dx_data &data) {
  std::vector<DictExpectation> profiles{
      {0xD5u,
       2u,
       858u,
       59u,
       782u,
       4u,
       1,
       {{"ACAD_LAYERFILTERS", 0xEEu},
        {"ACAD_LAYERSTATES", 0x1B8u},
        {"ACLYDICTIONARY", 0xEDu}}},
      {0x1B8u,
       0xD5u,
       438u,
       75u,
       346u,
       4u,
       0,
       {{"ACAD_VIEWS_RenderView", 0x249u}}},
      {0xEEu, 0xD5u, 60u, 59u, 0u, 12u, 0, {}},
      {0xEDu, 0xD5u, 174u, 173u, 0u, 12u, 0, {}},
      {0x24Au,
       0x249u,
       462u,
       67u,
       378u,
       8u,
       1,
       {{"AcadLayerStateAnnoScale", 0x24Bu}}}};
  const auto &nod = data.namedObjectDictionary;
  if (!data.hasNamedObjectDictionary || nod.handle != 0xCu ||
      nod.m_entries.size() != 16u || !nod.hasCompleteDwgEntries() ||
      !DxIfaceStandardDwgCaptureTestAccess::frameBound(data, 0xCu) ||
      nod.m_entries[12].m_name != "ACAD_SCALELIST" ||
      nod.m_entries[12].m_handle != 0x126u ||
      std::count_if(nod.m_entries.begin(), nod.m_entries.end(),
                    [](const DRW_Dictionary::Entry &entry) {
                      return entry.m_name == "ACAD_SCALELIST";
                    }) != 1)
    return false;
  profiles.push_back({0xCu,
                      0u,
                      4364u,
                      59u,
                      4288u,
                      4u,
                      0,
                      {{"ACAD_BACKGROUND", 0xAEu},
                       {"ACAD_COLOR", 0x73u},
                       {"ACAD_GROUP", 0xDu},
                       {"ACAD_LAYOUT", 0x1Au},
                       {"ACAD_MATERIAL", 0x72u},
                       {"ACAD_MLEADERSTYLE", 0x147u},
                       {"ACAD_MLINESTYLE", 0x17u},
                       {"ACAD_PLOTSETTINGS", 0x19u},
                       {"ACAD_PLOTSTYLENAME", 0xEu},
                       {"ACAD_RENDER_ACTIVE_SETTINGS", 0x1CCu},
                       {"ACAD_RENDER_GLOBAL", 0x237u},
                       {"ACAD_RENDER_SETTINGS", 0x255u},
                       {"ACAD_SCALELIST", 0x126u},
                       {"ACAD_TABLESTYLE", 0x86u},
                       {"ACAD_VISUALSTYLE", 0x99u},
                       {"AcDbVariableDictionary", 0x66u}}});
  DictExpectation scaleList{0x126u, 0xCu, 1436u, 75u, 1344u, 4u, 0, {}};
  for (unsigned i = 0u; i < 32u; ++i) {
    std::string key;
    key.push_back(static_cast<char>('A' + i / 10u));
    key.push_back(static_cast<char>('0' + i % 10u));
    const auto handle = 0x127u + i;
    const auto matches = std::count_if(
        data.scales.begin(), data.scales.end(),
        [handle](const DRW_Scale &row) { return row.handle == handle; });
    const auto row = std::find_if(
        data.scales.begin(), data.scales.end(),
        [handle](const DRW_Scale &value) { return value.handle == handle; });
    if (matches != 1 || row == data.scales.end() || row->parentHandle != 0x126u)
      return false;
    scaleList.items.emplace_back(key, 0x127u + i);
  }
  profiles.push_back(std::move(scaleList));
  std::cout
      << "nod=c entry=12 name=ACAD_SCALELIST target=126 typed_scales=32\n";
  for (const auto &profile : profiles) {
    const auto *raw =
        DxIfaceStandardDwgCaptureTestAccess::frame(data, profile.handle);
    if (!raw || !observeDictionary(data, profile, raw->m_rawBytes,
                                   raw->m_bodyBitSize, true))
      return false;
    // Every mutation affects a verified source bit and is followed by a retry.
    std::vector<std::pair<const char *, std::uint32_t>> mutations{
        {"body", profile.bodyFieldsEnd - 2u},
        {"footer", profile.bodyEnd - 1u},
        {"owner-role", profile.bodyEnd + 1u}};
    if (profile.handle == 0xCu)
      mutations.emplace_back("owner-null-length", profile.bodyEnd + 7u);
    else if (profile.handle != 0x24Au)
      mutations.emplace_back("owner-target", profile.bodyEnd + 15u);
    else
      mutations.emplace_back("self-handle-relative-owner-base", 25u);
    if (profile.handle != 0xD5u && profile.handle != 0x24Au &&
        profile.handle != 0xCu) {
      mutations.emplace_back("reactor-role", profile.bodyEnd + 17u);
      mutations.emplace_back("reactor-target", profile.bodyEnd + 31u);
    }
    if (!profile.items.empty()) {
      mutations.emplace_back("string-byte", profile.bodyFieldsEnd + 17u);
      std::vector<std::uint32_t> itemStarts =
          profile.handle == 0xD5u ? std::vector<std::uint32_t>{874u, 890u, 914u}
                                  : std::vector<std::uint32_t>{470u};
      if (profile.handle == 0xCu) {
        std::uint32_t stringCursor = 59u;
        for (const auto &entry : profile.items) {
          mutations.emplace_back("nod-string-byte", stringCursor + 17u);
          stringCursor +=
              10u + 16u * static_cast<std::uint32_t>(entry.first.size());
        }
        if (stringCursor != 4347u)
          return false;
        itemStarts.clear();
        std::uint32_t cursor = 4372u;
        for (const auto &entry : profile.items) {
          itemStarts.push_back(cursor);
          cursor += entry.second <= 0xFFu ? 16u : 24u;
        }
      }
      if (profile.handle == 0x126u) {
        itemStarts.clear();
        for (unsigned i = 0u; i < 32u; ++i)
          itemStarts.push_back(1468u + 24u * i);
      }
      for (auto start : itemStarts) {
        mutations.emplace_back("item-role", start + 1u);
        mutations.emplace_back("item-target", start + 15u);
      }
    }
    for (const auto &mutation : mutations) {
      auto changed = raw->m_rawBytes;
      changed[mutation.second / 8u] ^=
          static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
      if (observeDictionary(data, profile, changed, raw->m_bodyBitSize,
                            false) ||
          !observeDictionary(data, profile, raw->m_rawBytes, raw->m_bodyBitSize,
                             false))
        return false;
      std::cout << "dictionary-negative=" << mutation.first
                << " handle=" << std::hex << profile.handle << std::dec
                << " bit=" << mutation.second << " rejected=1 retry=1\n";
    }
    // Insert an extra bit before the detached strings (or absent-string
    // marker) and consume one padding bit, adjusting the declared body end.
    // This is a research-only mutation; it must never widen source admission.
    auto inserted = raw->m_rawBytes;
    const auto insertion = profile.bodyFieldsEnd;
    const auto totalBits = static_cast<std::uint32_t>(inserted.size() * 8u);
    for (auto bit = totalBits - 1u; bit > insertion; --bit) {
      const auto previous =
          (inserted[(bit - 1u) / 8u] >> (7u - (bit - 1u) % 8u)) & 1u;
      const auto mask = static_cast<std::uint8_t>(1u << (7u - bit % 8u));
      inserted[bit / 8u] = static_cast<std::uint8_t>(
          (inserted[bit / 8u] & ~mask) | (previous ? mask : 0u));
    }
    inserted[insertion / 8u] &=
        static_cast<std::uint8_t>(~(1u << (7u - insertion % 8u)));
    if (raw->m_bodyBitSize == 0u ||
        observeDictionary(data, profile, inserted, raw->m_bodyBitSize - 1u,
                          false) ||
        !observeDictionary(data, profile, raw->m_rawBytes, raw->m_bodyBitSize,
                           false))
      return false;
    std::cout << "dictionary-negative=extra-body-bit handle=" << std::hex
              << profile.handle << std::dec << " rejected=1 retry=1\n";
    // Cut the final object byte while keeping the same body boundary. This
    // removes handle-stream bits, not merely the declared padding.
    auto truncatedFrame = raw->m_rawBytes;
    truncatedFrame.pop_back();
    if (raw->m_bodyBitSize < 8u ||
        observeDictionary(data, profile, truncatedFrame,
                          raw->m_bodyBitSize - 8u, false) ||
        !observeDictionary(data, profile, raw->m_rawBytes, raw->m_bodyBitSize,
                           false))
      return false;
    std::cout << "dictionary-negative=truncated-handle-stream handle="
              << std::hex << profile.handle << std::dec
              << " rejected=1 retry=1\n";
    auto appended = raw->m_rawBytes;
    appended.push_back(0u);
    if (observeDictionary(data, profile, appended, raw->m_bodyBitSize + 8u,
                          false))
      return false;
    std::cout << "dictionary-negative=extra-handle-byte handle=" << std::hex
              << profile.handle << std::dec << " rejected=1\n";
  }
  return true;
}

bool observeNestedXrecord(const dx_data &data,
                          const std::vector<std::uint8_t> &bytes,
                          std::uint32_t handleBits, bool verbose) {
  using namespace dx_raster_wire;
  const DRW_XRecord *record = nullptr;
  for (const auto &row : data.xRecords)
    if (row.handle == 0x24Bu) {
      if (record)
        return false;
      record = &row;
    }
  if (!record ||
      !DxIfaceStandardDwgCaptureTestAccess::frameBound(data, 0x24Bu, 79u) ||
      record->parentHandle != 0x24Au || record->xDictHandle != 0u ||
      !record->appData.empty() || !record->extData.empty() ||
      record->reactorHandles != std::vector<std::uint32_t>{0x24Au} ||
      record->m_rawDataValid || record->m_cloning != 1 ||
      record->m_dataEntries.size() != 1u ||
      record->m_dataEntries[0].code() != 330 ||
      record->m_dataEntries[0].type() != DRW_Variant::INTEGER64 ||
      record->m_dataEntries[0].i64_val() != 0x127 ||
      record->m_handleValues.size() != 1u ||
      record->m_handleValues[0] != std::make_pair(330, 0x127u) ||
      bytes.empty() || bytes.size() > 1024u || handleBits >= bytes.size() * 8u)
    return false;
  const auto total = static_cast<std::uint32_t>(bytes.size() * 8u);
  const auto bodyEnd = total - handleBits;
  if (bodyEnd != 148u)
    return false;
  detail::Bits body{bytes.data(), 0u, bodyEnd};
  const auto tag = body.take(2u);
  const auto type = tag == 0u   ? body.take(8u)
                    : tag == 1u ? body.take(8u) + 0x1f0u
                                : body.little(2u);
  const auto self = detail::handle(body, 0u, 0u, true);
  if (!body.good || type != 79u || self.value != 0x24Bu ||
      body.shortValue() != 0u || body.longValue() != 1u ||
      body.take(1u) != 1u || body.longValue() != 10u)
    return false;
  const auto dataStart = body.pos;
  if (dataStart != 57u || body.little(2u) != 330u || body.little(8u) != 0x127u)
    return false;
  const auto dataEnd = body.pos;
  if (dataEnd != 137u || body.shortValue() != 1u || !body.good ||
      body.pos != bodyEnd - 1u)
    return false;
  detail::Bits presence{bytes.data(), bodyEnd - 1u, bodyEnd};
  if (presence.take(1u) != 0u || !presence.good)
    return false;
  detail::Bits wire{bytes.data(), bodyEnd, total};
  const auto ownerStart = wire.pos;
  const auto owner = detail::handle(wire, 4u, 0x24Bu);
  if (!wire.good || owner.role != 8u || owner.value != 0x24Au ||
      wire.pos != 156u)
    return false;
  const auto ownerEnd = wire.pos;
  const auto reactor = detail::handle(wire, 4u, 0x24Bu);
  if (!wire.good || reactor.role != 4u || reactor.value != 0x24Au ||
      wire.pos != 180u)
    return false;
  const auto terminal = wire.pos;
  if (wire.end - wire.pos >= 8u || !wire.skip(wire.end - wire.pos))
    return false;
  if (verbose)
    std::cout << "xrecord=24b body=0:" << bodyEnd - 1u << " data=" << dataStart
              << ":" << dataEnd << " presence=" << bodyEnd - 1u << ":"
              << bodyEnd << " owner=" << ownerStart << ":" << ownerEnd
              << " role=8 target=24a"
              << " reactor=" << ownerEnd << ":" << terminal
              << " role=4 target=24a"
              << " padding=" << terminal << ":" << wire.end << "\n";
  return true;
}

bool testNestedXrecord(const dx_data &data) {
  const auto *raw = DxIfaceStandardDwgCaptureTestAccess::frame(data, 0x24Bu);
  if (!raw ||
      !observeNestedXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize, true))
    return false;
  // Exact bit locations are fixed after a successful observation.
  for (const auto &mutation : std::vector<std::pair<const char *, unsigned>>{
           {"self", 25u},
           {"eed", 34u},
           {"xdict-flag", 46u},
           {"byte-count", 56u},
           {"data-group", 58u},
           {"data-target", 80u},
           {"cloning", 146u},
           {"presence", 147u},
           {"owner-role", 149u},
           {"reactor-role", 157u},
           {"reactor-target", 179u}}) {
    auto changed = raw->m_rawBytes;
    changed[mutation.second / 8u] ^=
        static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
    if (observeNestedXrecord(data, changed, raw->m_bodyBitSize, false) ||
        !observeNestedXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize, false))
      return false;
    std::cout << "xrecord-negative=" << mutation.first
              << " bit=" << mutation.second << " rejected=1 retry=1\n";
  }
  auto truncated = raw->m_rawBytes;
  truncated.pop_back();
  if (raw->m_bodyBitSize < 8u ||
      observeNestedXrecord(data, truncated, raw->m_bodyBitSize - 8u, false) ||
      !observeNestedXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize, false))
    return false;
  auto appended = raw->m_rawBytes;
  appended.push_back(0u);
  if (observeNestedXrecord(data, appended, raw->m_bodyBitSize + 8u, false))
    return false;
  std::cout << "xrecord-negative=truncated-handle-stream rejected=1 retry=1\n"
            << "xrecord-negative=extra-handle-byte rejected=1\n";
  return true;
}

bool observeScale(const dx_data &data, const std::vector<std::uint8_t> &bytes,
                  std::uint32_t handleBits, bool verbose) {
  using namespace dx_raster_wire;
  const auto *raw = DxIfaceStandardDwgCaptureTestAccess::opaque(data, 0x127u);
  const DRW_Scale *typed = nullptr;
  for (const auto &row : data.scales)
    if (row.handle == 0x127u) {
      if (typed)
        return false;
      typed = &row;
    }
  if (!raw || !typed || raw->m_handle != 0x127u || raw->m_objectType != 512 ||
      raw->m_version != DRW::AC1024 || raw->m_recordName != "SCALE" ||
      raw->m_className != "AcDbScale" || raw->m_isEntity ||
      raw->m_hasDataStorage || raw->m_objectSize != raw->m_rawBytes.size() ||
      typed->parentHandle != 0x126u || typed->xDictHandle != 0u ||
      typed->reactorHandles != std::vector<std::uint32_t>{0x126u} ||
      !typed->appData.empty() || !typed->extData.empty() || typed->flag != 0u ||
      typed->name != "1:1" || typed->paperUnits != 1.0 ||
      typed->drawingUnits != 1.0 || !typed->isUnitScale || bytes.empty() ||
      bytes.size() > 1024u || handleBits >= bytes.size() * 8u)
    return false;
  const auto total = static_cast<std::uint32_t>(bytes.size() * 8u);
  const auto bodyEnd = total - handleBits;
  if (bodyEnd != 129u)
    return false;
  detail::Bits body{bytes.data(), 0u, bodyEnd};
  const auto tag = body.take(2u);
  const auto type = tag == 0u   ? body.take(8u)
                    : tag == 1u ? body.take(8u) + 0x1f0u
                                : body.little(2u);
  const auto self = detail::handle(body, 0u, 0u, true);
  if (!body.good || type != 512u || self.value != 0x127u ||
      body.shortValue() != 0u || body.longValue() != 1u ||
      body.take(1u) != 1u || body.shortValue() != 0u || body.take(2u) != 1u ||
      body.take(2u) != 1u || body.take(1u) != 1u || !body.good ||
      body.pos != 54u)
    return false;
  detail::Bits text{bytes.data(), 54u, 112u};
  if (text.shortValue() != 3u || text.little(2u) != '1' ||
      text.little(2u) != ':' || text.little(2u) != '1' || !text.good ||
      text.pos != text.end)
    return false;
  detail::Bits footer{bytes.data(), 112u, 128u};
  detail::Bits presence{bytes.data(), 128u, 129u};
  if (footer.little(2u) != 58u || !footer.good || presence.take(1u) != 1u ||
      !presence.good)
    return false;
  detail::Bits wire{bytes.data(), bodyEnd, total};
  const auto owner = detail::handle(wire, 4u, 0x127u);
  if (!wire.good || owner.role != 8u || owner.value != 0x126u ||
      wire.pos != 137u)
    return false;
  const auto reactor = detail::handle(wire, 4u, 0x127u);
  if (!wire.good || reactor.role != 4u || reactor.value != 0x126u ||
      wire.pos != 161u)
    return false;
  if (wire.end - wire.pos >= 8u || !wire.skip(wire.end - wire.pos))
    return false;
  if (verbose)
    std::cout << "scale=127 body=0:54 strings=54:112 footer=112:129 "
                 "owner=129:137 role=8 target=126 reactor=137:161 role=4 "
                 "target=126 padding=161:168\n";
  return true;
}
bool testScale(const dx_data &data) {
  const auto *raw = DxIfaceStandardDwgCaptureTestAccess::opaque(data, 0x127u);
  if (!raw || !observeScale(data, raw->m_rawBytes, raw->m_bodyBitSize, true))
    return false;
  for (const auto &mutation : std::vector<std::pair<const char *, unsigned>>{
           {"self", 25u},
           {"flag", 48u},
           {"paper-unit", 49u},
           {"drawing-unit", 51u},
           {"unit-flag", 53u},
           {"name", 71u},
           {"footer", 128u},
           {"owner-role", 130u},
           {"reactor-role", 138u},
           {"reactor-target", 160u}}) {
    auto changed = raw->m_rawBytes;
    changed[mutation.second / 8u] ^=
        static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
    if (observeScale(data, changed, raw->m_bodyBitSize, false) ||
        !observeScale(data, raw->m_rawBytes, raw->m_bodyBitSize, false))
      return false;
    std::cout << "scale-negative=" << mutation.first
              << " bit=" << mutation.second << " rejected=1 retry=1\n";
  }
  auto truncated = raw->m_rawBytes;
  truncated.pop_back();
  if (raw->m_bodyBitSize < 8u ||
      observeScale(data, truncated, raw->m_bodyBitSize - 8u, false) ||
      !observeScale(data, raw->m_rawBytes, raw->m_bodyBitSize, false))
    return false;
  auto appended = raw->m_rawBytes;
  appended.push_back(0u);
  if (observeScale(data, appended, raw->m_bodyBitSize + 8u, false))
    return false;
  std::cout << "scale-negative=truncated-handle-stream rejected=1 "
               "retry=1\nscale-negative=extra-handle-byte rejected=1\n";
  return true;
}

bool observePrimaryXrecord(const dx_data &data,
                           const std::vector<std::uint8_t> &bytes,
                           std::uint32_t handleBits, bool verbose) {
  using namespace dx_raster_wire;
  const DRW_XRecord *record = nullptr;
  for (const auto &row : data.xRecords)
    if (row.handle == 0x249u) {
      if (record)
        return false;
      record = &row;
    }
  if (!record ||
      !DxIfaceStandardDwgCaptureTestAccess::frameBound(data, 0x249u, 79) ||
      record->parentHandle != 0x1B8u || record->xDictHandle != 0x24Au ||
      record->reactorHandles != std::vector<std::uint32_t>{0x1B8u} ||
      !record->appData.empty() || !record->extData.empty() ||
      record->m_rawDataValid || record->m_cloning != 1 ||
      record->m_dataEntries.size() != 11u ||
      record->m_handleValues !=
          std::vector<std::pair<int, std::uint32_t>>{{330, 0x10u},
                                                     {331, 0x16u}} ||
      bytes.empty() || bytes.size() > 1024u || handleBits >= bytes.size() * 8u)
    return false;
  const auto total = static_cast<std::uint32_t>(bytes.size() * 8u);
  const auto bodyEnd = total - handleBits;
  if (bodyEnd != 684u)
    return false;
  detail::Bits body{bytes.data(), 0u, bodyEnd};
  const auto tag = body.take(2u);
  const auto type = tag == 0u   ? body.take(8u)
                    : tag == 1u ? body.take(8u) + 0x1f0u
                                : body.little(2u);
  const auto self = detail::handle(body, 0u, 0u, true);
  if (!body.good || type != 79u || self.value != 0x249u ||
      body.shortValue() != 0u || body.longValue() != 1u ||
      body.take(1u) != 0u || body.longValue() != 77u || body.pos != 57u)
    return false;
  const std::uint32_t dataEnd = 673u;
  const auto numeric = [&](std::size_t i, unsigned code, std::uint64_t expected,
                           unsigned bytesToRead, bool isHandle) {
    const auto &v = record->m_dataEntries[i];
    if (body.pos > dataEnd || body.little(2u) != code ||
        body.little(bytesToRead) != expected || !body.good ||
        body.pos > dataEnd || v.code() != static_cast<int>(code))
      return false;
    if (isHandle)
      return v.type() == DRW_Variant::INTEGER64 &&
             static_cast<std::uint64_t>(v.i64_val()) == expected;
    if (v.type() != DRW_Variant::INTEGER)
      return false;
    const auto signedExpected =
        code == 370u  ? static_cast<std::int64_t>(expected) - 65536
        : code == 92u ? static_cast<std::int64_t>(expected) - 4294967296LL
                      : static_cast<std::int64_t>(expected);
    return v.i_val() == signedExpected;
  };
  const auto text = [&](std::size_t i, unsigned code,
                        const std::string &expected) {
    const auto &v = record->m_dataEntries[i];
    if (body.pos > dataEnd || body.little(2u) != code ||
        body.little(2u) != expected.size())
      return false;
    for (unsigned char c : expected)
      if (body.little(2u) != c)
        return false;
    return body.good && body.pos <= dataEnd &&
           v.code() == static_cast<int>(code) &&
           v.type() == DRW_Variant::STRING && v.content.s &&
           *v.content.s == expected;
  };
  if (!numeric(0, 91u, 33280u, 4u, false) || !text(1, 301u, "") ||
      !numeric(2, 290u, 0u, 1u, false) || !text(3, 302u, "0") ||
      !numeric(4, 330u, 0x10u, 8u, true) || !numeric(5, 90u, 8u, 4u, false) ||
      !numeric(6, 62u, 7u, 2u, false) || !numeric(7, 370u, 65533u, 2u, false) ||
      !numeric(8, 331u, 0x16u, 8u, true) || !text(9, 1u, "Color_7") ||
      !numeric(10, 92u, 3269980415u, 4u, false) || body.pos != dataEnd ||
      body.shortValue() != 1u || !body.good || body.pos != 683u)
    return false;
  detail::Bits presence{bytes.data(), 683u, 684u};
  if (presence.take(1u) != 0u || !presence.good)
    return false;
  detail::Bits wire{bytes.data(), 684u, total};
  const auto owner = detail::handle(wire, 4u, 0x249u);
  if (!wire.good || owner.role != 12u || owner.value != 0x1B8u ||
      wire.pos != 700u)
    return false;
  const auto reactor = detail::handle(wire, 4u, 0x249u);
  if (!wire.good || reactor.role != 4u || reactor.value != 0x1B8u ||
      wire.pos != 724u)
    return false;
  const auto xdict = detail::handle(wire, 3u, 0x249u);
  if (!wire.good || xdict.role != 3u || xdict.value != 0x24Au ||
      wire.pos != 748u || wire.end - wire.pos >= 8u ||
      !wire.skip(wire.end - wire.pos))
    return false;
  if (verbose)
    std::cout << "xrecord=249 body=0:683 data=57:673 presence=683:684 "
                 "owner=684:700 role=12 target=1b8 reactor=700:724 role=4 "
                 "target=1b8 xdict=724:748 role=3 target=24a padding=748:752\n";
  return true;
}
bool testPrimaryXrecord(const dx_data &data) {
  const auto *raw = DxIfaceStandardDwgCaptureTestAccess::frame(data, 0x249u);
  if (!raw ||
      !observePrimaryXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize, true))
    return false;
  const std::vector<std::pair<const char *, unsigned>> mutations{
      {"self", 25u},
      {"eed", 34u},
      {"xdict-flag", 46u},
      {"byte-count", 56u},
      {"group-91", 73u},
      {"group-301", 120u},
      {"group-290", 150u},
      {"group-302", 190u},
      {"group-330-target", 248u},
      {"group-90", 320u},
      {"group-62", 355u},
      {"group-370", 390u},
      {"group-331-target", 450u},
      {"group-1", 550u},
      {"group-92", 660u},
      {"cloning", 682u},
      {"presence", 683u},
      {"owner-role", 685u},
      {"owner-target", 699u},
      {"reactor-role", 701u},
      {"reactor-target", 723u},
      {"xdict-role", 725u},
      {"xdict-target", 747u}};
  for (const auto &mutation : mutations) {
    auto changed = raw->m_rawBytes;
    changed[mutation.second / 8u] ^=
        static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
    if (observePrimaryXrecord(data, changed, raw->m_bodyBitSize, false) ||
        !observePrimaryXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize,
                               false))
      return false;
    std::cout << "primary-negative=" << mutation.first
              << " bit=" << mutation.second << " rejected=1 retry=1\n";
  }
  auto truncated = raw->m_rawBytes;
  truncated.pop_back();
  if (raw->m_bodyBitSize < 8u ||
      observePrimaryXrecord(data, truncated, raw->m_bodyBitSize - 8u, false) ||
      !observePrimaryXrecord(data, raw->m_rawBytes, raw->m_bodyBitSize, false))
    return false;
  auto appended = raw->m_rawBytes;
  appended.push_back(0u);
  if (observePrimaryXrecord(data, appended, raw->m_bodyBitSize + 8u, false))
    return false;
  std::cout << "primary-negative=truncated-handle-stream rejected=1 "
               "retry=1\nprimary-negative=extra-handle-byte rejected=1\n";
  return true;
}

struct PeerScale {
  std::uint32_t handle, bodyEnd, size, ownerRole, ownerLen, ownerRaw;
  std::uint32_t reactorRole, reactorLen, reactorRaw;
  std::string name;
  double paper, drawing;
  bool unit;
};
struct ScaleOffsets {
  std::uint32_t paper, drawing, unit, name, footer, owner, reactor, reactorEnd;
};

bool observePeerScale(const dx_data &data, const PeerScale &peer,
                      const std::vector<std::uint8_t> &bytes,
                      std::uint32_t handleBits, ScaleOffsets &offsets,
                      bool verbose) {
  using namespace dx_raster_wire;
  const auto *raw =
      DxIfaceStandardDwgCaptureTestAccess::opaque(data, peer.handle);
  const DRW_Scale *typed = nullptr;
  for (const auto &row : data.scales)
    if (row.handle == peer.handle) {
      if (typed)
        return false;
      typed = &row;
    }
  if (!raw || !typed || raw->m_handle != peer.handle ||
      raw->m_objectType != 512 || raw->m_version != DRW::AC1024 ||
      raw->m_recordName != "SCALE" || raw->m_className != "AcDbScale" ||
      raw->m_isEntity || raw->m_hasDataStorage ||
      raw->m_objectSize != raw->m_rawBytes.size() ||
      raw->m_objectSize != peer.size || typed->parentHandle != 0x126u ||
      typed->xDictHandle != 0u ||
      typed->reactorHandles != std::vector<std::uint32_t>{0x126u} ||
      !typed->appData.empty() || !typed->extData.empty() || typed->flag != 0u ||
      typed->name != peer.name || typed->paperUnits != peer.paper ||
      typed->drawingUnits != peer.drawing || typed->isUnitScale != peer.unit ||
      bytes.empty() || bytes.size() > 1024u || handleBits >= bytes.size() * 8u)
    return false;
  const auto total = static_cast<std::uint32_t>(bytes.size() * 8u);
  const auto bodyEnd = total - handleBits;
  if (bodyEnd != peer.bodyEnd || bodyEnd < 17u)
    return false;
  detail::Bits body{bytes.data(), 0u, bodyEnd};
  const auto tag = body.take(2u);
  const auto type = tag == 0u   ? body.take(8u)
                    : tag == 1u ? body.take(8u) + 0x1f0u
                                : body.little(2u);
  const auto self = detail::handle(body, 0u, 0u, true);
  if (!body.good || type != 512u || self.value != peer.handle ||
      body.shortValue() != 0u || body.longValue() != 1u ||
      body.take(1u) != 1u || body.shortValue() != 0u)
    return false;
  const auto bitDouble = [&](double expected) {
    const auto code = body.take(2u);
    double value = 0.0;
    if (code == 1u)
      value = 1.0;
    else if (code == 2u)
      value = 0.0;
    else if (code == 0u) {
      const std::uint64_t rawBits = body.little(8u);
      static_assert(sizeof(value) == sizeof(rawBits), "64-bit double required");
      std::memcpy(&value, &rawBits, sizeof(value));
    } else
      return false;
    return body.good && std::isfinite(value) && value == expected;
  };
  offsets.paper = body.pos;
  if (!bitDouble(peer.paper))
    return false;
  offsets.drawing = body.pos;
  if (!bitDouble(peer.drawing))
    return false;
  offsets.unit = body.pos;
  if (body.take(1u) != static_cast<unsigned>(peer.unit) || !body.good)
    return false;
  const auto stringStart = body.pos;
  detail::Bits footer{bytes.data(), bodyEnd - 17u, bodyEnd - 1u};
  const auto stringBits = footer.little(2u);
  detail::Bits presence{bytes.data(), bodyEnd - 1u, bodyEnd};
  if (!footer.good || stringBits > bodyEnd - 17u ||
      stringStart != bodyEnd - 17u - stringBits || presence.take(1u) != 1u ||
      !presence.good)
    return false;
  detail::Bits text{bytes.data(), stringStart, bodyEnd - 17u};
  if (text.shortValue() != peer.name.size())
    return false;
  offsets.name = text.pos;
  for (unsigned char c : peer.name)
    if (text.little(2u) != c)
      return false;
  if (!text.good || text.pos != text.end)
    return false;
  offsets.footer = bodyEnd - 1u;
  detail::Bits wire{bytes.data(), bodyEnd, total};
  offsets.owner = wire.pos;
  const auto owner = detail::handle(wire, 4u, peer.handle);
  if (!wire.good || owner.role != peer.ownerRole ||
      owner.length != peer.ownerLen || owner.value != 0x126u ||
      (peer.ownerRole == 12u && peer.ownerRaw != peer.handle - 0x126u) ||
      (peer.ownerRole == 8u && peer.ownerRaw != 0u))
    return false;
  offsets.reactor = wire.pos;
  const auto reactor = detail::handle(wire, 4u, peer.handle);
  if (!wire.good || reactor.role != peer.reactorRole ||
      reactor.length != peer.reactorLen || reactor.value != peer.reactorRaw ||
      reactor.value != 0x126u)
    return false;
  offsets.reactorEnd = wire.pos;
  if (wire.end - wire.pos >= 8u || !wire.skip(wire.end - wire.pos) ||
      !wire.good)
    return false;
  if (verbose)
    std::cout << "scale-peer=" << std::hex << peer.handle << std::dec
              << " body=0:" << stringStart << " strings=" << stringStart << ":"
              << bodyEnd - 17u << " footer=" << bodyEnd - 17u << ":" << bodyEnd
              << " owner=" << offsets.owner << ":" << offsets.reactor
              << " reactor=" << offsets.reactor << ":" << offsets.reactorEnd
              << " padding=" << offsets.reactorEnd << ":" << total
              << " name=" << peer.name << " paper=" << peer.paper
              << " drawing=" << peer.drawing << "\n";
  return true;
}

bool testAllScales(const dx_data &data, const std::string &manifest) {
  if (!std::numeric_limits<double>::is_iec559 || sizeof(double) != 8u)
    return false;
  std::ifstream input(manifest);
  if (!input)
    return false;
  for (std::uint32_t index = 0u; index < 32u; ++index) {
    PeerScale peer{};
    if (!(input >> peer.handle >> peer.bodyEnd >> peer.size >> peer.ownerRole >>
          peer.ownerLen >> peer.ownerRaw >> peer.reactorRole >>
          peer.reactorLen >> peer.reactorRaw >> std::quoted(peer.name) >>
          peer.paper >> peer.drawing >> peer.unit) ||
        peer.handle != 0x127u + index || !std::isfinite(peer.paper) ||
        !std::isfinite(peer.drawing) || peer.name.empty())
      return false;
    const auto *raw =
        DxIfaceStandardDwgCaptureTestAccess::opaque(data, peer.handle);
    ScaleOffsets offsets{};
    if (!raw || !observePeerScale(data, peer, raw->m_rawBytes,
                                  raw->m_bodyBitSize, offsets, true))
      return false;
    std::vector<std::pair<const char *, std::uint32_t>> mutations{
        {"eed", 34u},
        {"xdict-flag", 46u},
        {"paper-bd", offsets.paper},
        {"drawing-bd", offsets.drawing},
        {"unit-flag", offsets.unit},
        {"name", offsets.name + 7u},
        {"footer-size", offsets.footer - 1u},
        {"footer", offsets.footer},
        {"owner-role", offsets.owner + 1u},
        {"owner-target", peer.ownerRole == 8u ? 25u : offsets.owner + 15u},
        {"reactor-role", offsets.reactor + 1u},
        {"reactor-target", offsets.reactorEnd - 1u}};
    dx_raster_wire::detail::Bits paperTag{raw->m_rawBytes.data(), offsets.paper,
                                          offsets.paper + 2u};
    dx_raster_wire::detail::Bits drawingTag{
        raw->m_rawBytes.data(), offsets.drawing, offsets.drawing + 2u};
    if (paperTag.take(2u) == 0u)
      mutations.emplace_back("paper-bd-payload", offsets.paper + 9u);
    if (drawingTag.take(2u) == 0u)
      mutations.emplace_back("drawing-bd-payload", offsets.drawing + 9u);
    for (const auto &mutation : mutations) {
      auto changed = raw->m_rawBytes;
      if (mutation.second >= changed.size() * 8u)
        return false;
      changed[mutation.second / 8u] ^=
          static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
      ScaleOffsets ignored{};
      if (observePeerScale(data, peer, changed, raw->m_bodyBitSize, ignored,
                           false) ||
          !observePeerScale(data, peer, raw->m_rawBytes, raw->m_bodyBitSize,
                            ignored, false))
        return false;
      std::cout << "scale-peer-negative=" << mutation.first
                << " handle=" << std::hex << peer.handle << std::dec
                << " rejected=1 retry=1\n";
    }
    auto truncated = raw->m_rawBytes;
    truncated.pop_back();
    ScaleOffsets ignored{};
    if (raw->m_bodyBitSize < 8u ||
        observePeerScale(data, peer, truncated, raw->m_bodyBitSize - 8u,
                         ignored, false) ||
        !observePeerScale(data, peer, raw->m_rawBytes, raw->m_bodyBitSize,
                          ignored, false))
      return false;
    auto appended = raw->m_rawBytes;
    appended.push_back(0u);
    if (observePeerScale(data, peer, appended, raw->m_bodyBitSize + 8u, ignored,
                         false))
      return false;
    std::cout << "scale-peer-negative=truncated-handle-stream handle="
              << std::hex << peer.handle << std::dec << " rejected=1 retry=1\n"
              << "scale-peer-negative=extra-handle-byte handle=" << std::hex
              << peer.handle << std::dec << " rejected=1\n";
  }
  input >> std::ws;
  return input.eof();
}

int main(int argc, char **argv) {
  if (argc != 3)
    return 2;
  dx_iface iface;
  dx_data data;
  if (!iface.fileImport(argv[1], &data, false))
    return 3;
  auto raw = DxIfaceStandardDwgCaptureTestAccess::frame(data, 0xED);
  if (!raw || !DxIfaceStandardDwgCaptureTestAccess::frameBound(data, 0xED) ||
      raw->m_rawBytes.empty() || raw->m_rawBytes.size() > 1024u)
    return 4;
  const DRW_Dictionary *dict = nullptr;
  for (const auto &d : data.dictionaries)
    if (d.handle == 0xED) {
      if (dict)
        return 5;
      dict = &d;
    }
  if (!dict)
    return 6;
  const auto total = static_cast<std::uint32_t>(raw->m_rawBytes.size() * 8u);
  if (raw->m_bodyBitSize >= total)
    return 7;
  dx_raster_wire::detail::Bits bits{raw->m_rawBytes.data(), 0u,
                                    total - raw->m_bodyBitSize};
  auto tag = bits.take(2u);
  auto type = tag == 0u   ? bits.take(8u)
              : tag == 1u ? bits.take(8u) + 0x1f0u
                          : bits.little(2u);
  auto self = dx_raster_wire::detail::handle(bits, 0u, 0u, true);
  const auto eedStart = bits.pos;
  dx_raster_wire::Proof proof;
  std::cout << "object=" << std::hex << self.value << std::dec
            << " type=" << type << " body_end=" << bits.end
            << " eed_start=" << bits.pos << "\n";
  bool valid = true;
  const auto visitor = [&](const dx_raster_wire::EedToken &t) noexcept {
    std::cout << "token=" << t.index << " code=" << t.code
              << " span=" << t.span.begin << ":" << t.span.end
              << " value_span=" << t.valueSpan.begin << ":" << t.valueSpan.end
              << " role=" << unsigned(t.role) << " value=" << t.value;
    if (t.index == 0u &&
        (t.code != 1001u || t.span.begin != 36u || t.span.end != 52u ||
         t.value != 0x12u || t.role != 5u))
      valid = false;
    if (t.index == 1u &&
        (t.code != 1000u || t.span.begin != 52u || t.span.end != 140u ||
         t.valueSpan.begin != 60u || t.valueSpan.end != 140u || t.value != 4u))
      valid = false;
    if (t.index >= dict->extData.size() || !dict->extData[t.index] ||
        dict->extData[t.index]->code() != static_cast<int>(t.code))
      valid = false;
    else if (dict->extData[t.index]->type() == DRW_Variant::STRING &&
             dict->extData[t.index]->content.s)
      std::cout << " typed=" << *dict->extData[t.index]->content.s;
    if (t.code == 1000u) {
      dx_raster_wire::detail::Bits text{raw->m_rawBytes.data(),
                                        t.valueSpan.begin, t.valueSpan.end};
      auto n = text.little(2u);
      std::string v;
      for (std::uint32_t i = 0; i < n; ++i) {
        auto c = text.little(2u);
        if (c > 127)
          valid = false;
        v.push_back(static_cast<char>(c));
      }
      std::cout << " raw_ascii=" << v << " raw_end=" << text.pos;
      if (!text.good || text.pos != text.end ||
          t.index >= dict->extData.size() || !dict->extData[t.index] ||
          dict->extData[t.index]->type() != DRW_Variant::STRING ||
          !dict->extData[t.index]->content.s ||
          v != *dict->extData[t.index]->content.s)
        valid = false;
    }
    std::cout << "\n";
    return true;
  };
  auto status = dx_raster_wire::detail::eed(bits, 2010u, proof, visitor);
  auto app = DxIfaceStandardDwgCaptureTestAccess::app(data, 0x12);
  std::cout << "status=" << static_cast<unsigned>(status)
            << " eed_end=" << bits.pos << " variants=" << proof.eedVariants
            << " typed=" << dict->extData.size()
            << " app_source=" << (app ? app->name : "<absent>") << "\n";
  if (app)
    std::cout << "app_unchanged=" << app->hasUnchangedDwgSourceName()
              << " app_bound="
              << app->hasBoundDwgNameProfile(DRW::AC1024, "UTF-16")
              << " app_decode_issues=" << app->dwgNameDecodeIssues() << "\n";

  const auto appCount =
      std::count_if(data.appIds.begin(), data.appIds.end(),
                    [](const DRW_AppId &row) { return row.handle == 0x12u; });
  if (type != 42u || self.value != 0xEDu || eedStart != 26u || appCount != 1 ||
      bits.end != 174u || bits.pos != 142u || proof.eedVariants != 2u ||
      dict->extData.size() != 2u || !valid ||
      status != dx_raster_wire::Status::Proven || !app ||
      !app->hasUnchangedDwgSourceName() ||
      !app->hasBoundDwgNameProfile(DRW::AC1024, "UTF-16") ||
      app->dwgNameDecodeIssues() || app->handle != 0x12u ||
      app->name != "ACAD" ||
      std::none_of(data.appIds.begin(), data.appIds.end(),
                   [](const DRW_AppId &row) {
                     return row.handle == 0x12u && row.name == "ACAD";
                   }))
    return 8;
  const auto check = [&](const std::vector<std::uint8_t> &bytes,
                         std::uint32_t end) {
    dx_raster_wire::detail::Bits b{bytes.data(), 26u, end};
    dx_raster_wire::Proof p;
    bool matched = true;
    unsigned seen = 0u;
    const auto visit = [&](const dx_raster_wire::EedToken &t) noexcept {
      ++seen;
      if (t.index >= dict->extData.size() || !dict->extData[t.index] ||
          dict->extData[t.index]->code() != static_cast<int>(t.code))
        return false;
      if (t.code == 1001u &&
          (t.value != 0x12u || t.role != 5u ||
           dict->extData[t.index]->type() != DRW_Variant::STRING ||
           !dict->extData[t.index]->content.s ||
           *dict->extData[t.index]->content.s != app->name))
        return false;
      if (t.code == 1000u) {
        dx_raster_wire::detail::Bits text{bytes.data(), t.valueSpan.begin,
                                          t.valueSpan.end};
        auto n = text.little(2u);
        std::string v;
        for (std::uint32_t i = 0; i < n; ++i) {
          auto c = text.little(2u);
          if (c > 127u)
            return false;
          v.push_back(static_cast<char>(c));
        }
        if (!text.good || text.pos != text.end ||
            dict->extData[t.index]->type() != DRW_Variant::STRING ||
            !dict->extData[t.index]->content.s ||
            v != *dict->extData[t.index]->content.s)
          return false;
      }
      return true;
    };
    auto result = dx_raster_wire::detail::eed(b, 2010u, p, visit);
    matched =
        result == dx_raster_wire::Status::Proven && seen == 2u && b.pos == 142u;
    return matched;
  };
  const auto original = raw->m_rawBytes;
  if (!check(original, 174u))
    return 9;
  for (const auto &mutation :
       {std::pair<const char *, unsigned>{"wrong-role", 37u},
        {"wrong-appid", 51u},
        {"text-byte", 83u},
        {"unknown-code", 52u}}) {
    auto changed = original;
    changed[mutation.second / 8u] ^=
        static_cast<std::uint8_t>(1u << (7u - mutation.second % 8u));
    const bool rejected = !check(changed, 174u);
    std::cout << "negative=" << mutation.first << " rejected=" << rejected
              << "\n";
    if (!rejected)
      return 10;
  }
  const bool truncated = !check(original, 141u);
  std::cout << "negative=truncated rejected=" << truncated << "\n";
  const bool retry = check(original, 174u);
  std::cout << "corrected_retry=" << retry << "\n";
  return !truncated || !retry || !testDictionaryFrames(data) ||
         !testNestedXrecord(data) || !testScale(data) ||
         !testPrimaryXrecord(data) || !testAllScales(data, argv[2]);
}
