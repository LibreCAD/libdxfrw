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
  return !truncated || !retry;
}
