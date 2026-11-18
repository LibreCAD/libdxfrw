#include "dx_iface.h"
#include "dx_raster_dwg_wire.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
class DxIfaceStandardDwgCaptureTestAccess {
public:
  static const DRW_UnsupportedObject *frame(const dx_data &d, std::uint32_t h) {
    auto it = d.m_standardDwgObjectFrames.find({d.importGeneration, h});
    return it == d.m_standardDwgObjectFrames.end() ? nullptr : &it->second.raw;
  }
  static bool frameBound(const dx_data &d, std::uint32_t h) {
    auto key = dx_data::StandardDwgFrameKey{d.importGeneration, h};
    auto it = d.m_standardDwgObjectFrames.find(key);
    if (d.importGeneration == 0u ||
        d.m_opaqueDwgSourceFrameIdentities.count(key) != 1u ||
        it == d.m_standardDwgObjectFrames.end())
      return false;
    const auto &frame = it->second;
    const auto &raw = frame.raw;
    return frame.payloadMatched && frame.typedBaseline &&
           !frame.opaquePayload && raw.m_handle == h &&
           raw.m_version == DRW::AC1024 && raw.m_objectType == 42 &&
           !raw.m_isEntity && !raw.m_hasDataStorage &&
           raw.m_objectSize == raw.m_rawBytes.size();
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
      typed->reactorHandles.size() != (expected.handle == 0xD5u ? 0u : 1u))
    return false;
  for (std::size_t i = 0; i < expected.items.size(); ++i)
    if (typed->m_entries[i].m_name != expected.items[i].first ||
        typed->m_entries[i].m_handle != expected.items[i].second)
      return false;
  if (!typed->reactorHandles.empty() && typed->reactorHandles[0] != 0xD5u)
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
    for (const auto &entry : expected.items) {
      if (text.shortValue() != entry.first.size())
        return false;
      for (unsigned char c : entry.first)
        if (text.little(2u) != c)
          return false;
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
      owner.value != expected.parent || wire.pos != ownerStart + 16u)
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
    if (!wire.good || reactor.role != 4u || reactor.value != 0xD5u ||
        wire.pos != start + 16u)
      return false;
    if (verbose)
      std::cout << " reactor=" << start << ":" << wire.pos;
  }
  std::size_t itemIndex = 0u;
  for (const auto &entry : expected.items) {
    const auto start = wire.pos;
    const auto item = detail::handle(wire, 2u, expected.handle);
    if (!wire.good || item.role != 2u || item.value != entry.second ||
        wire.pos !=
            start + ((expected.handle == 0xD5u && itemIndex != 1u) ? 16u : 24u))
      return false;
    ++itemIndex;
    if (verbose)
      std::cout << " item=" << start << ":" << wire.pos << ":" << std::hex
                << item.value << std::dec;
  }
  const auto expectedEnd = expected.handle == 0xD5u    ? 930u
                           : expected.handle == 0x1B8u ? 494u
                           : expected.handle == 0xEEu  ? 92u
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
  const std::vector<DictExpectation> profiles{
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
      {0xEDu, 0xD5u, 174u, 173u, 0u, 12u, 0, {}}};
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
        {"owner-role", profile.bodyEnd + 1u},
        {"owner-target", profile.bodyEnd + 15u}};
    if (profile.handle != 0xD5u) {
      mutations.emplace_back("reactor-role", profile.bodyEnd + 17u);
      mutations.emplace_back("reactor-target", profile.bodyEnd + 31u);
    }
    if (!profile.items.empty()) {
      mutations.emplace_back("string-byte", profile.bodyFieldsEnd + 17u);
      const std::vector<std::uint32_t> itemStarts =
          profile.handle == 0xD5u ? std::vector<std::uint32_t>{874u, 890u, 914u}
                                  : std::vector<std::uint32_t>{470u};
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

int main(int argc, char **argv) {
  if (argc != 2)
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
  return !truncated || !retry || !testDictionaryFrames(data);
}
