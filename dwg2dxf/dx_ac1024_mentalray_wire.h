// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_AC1024_MENTALRAY_WIRE_H
#define DX_AC1024_MENTALRAY_WIRE_H

#include "dx_layerstates_dwg_wire.h"
#include "intern/dxfrendersettings.h"
#include <cstring>

// Source-only checks for two physically observed AC1024 records and their
// three companions. Callers must separately prove same-import publication,
// class, graph, ledger and staged-output identity. In particular, TU strings
// are checked only against the supplied live model; callers must bind both
// raw frame and typed values to an immutable original import snapshot, or a
// coordinated same-length string/raw edit passes. No native render claim.
// ODA DWG v5.4.1 sections 2.12, 2.13, 20.1, 20.4.44 and 20.4.104.
namespace dx_ac1024_mentalray_wire {
namespace detail {
using dx_raster_wire::detail::Bits;
inline bool text(Bits& bits, const std::string& s, std::size_t expected) noexcept {
    if (s.size() != expected || bits.shortValue() != expected) return false;
    for (unsigned char c : s)
        if (c > 127u || bits.little(2u) != c) return false;
    return bits.good;
}
inline bool number(Bits& bits, double value) noexcept {
    const auto tag = bits.take(2u);
    double actual = 0.0;
    if (tag == 0u) {
        const std::uint64_t wire = bits.little(8u);
        static_assert(sizeof(wire) == sizeof(actual), "64-bit double required");
        std::memcpy(&actual, &wire, sizeof(actual));
    } else if (tag == 1u) actual = 1.0;
    else if (tag != 2u) return false;
    return bits.good && actual == value;
}
}
inline bool observeMentalRay(const DRW_UnsupportedObject& raw,
                             const DRW_RenderSettings& source) noexcept {
    using namespace dx_raster_wire;
    const bool active = source.handle == 0x1ccu;
    if (!active && source.handle != 0x256u) return false;
    const auto owner = active ? 0xcu : 0x255u;
    const auto xdict = active ? 0x1cdu : 0u;
    const std::size_t bytes = active ? 413u : 292u;
    const std::uint32_t textStart = active ? 931u : 875u;
    const std::uint32_t bodyEnd = active ? 3244u : 2300u;
    if (source.tType != DRW::RENDERSETTINGS || source.m_kind != DRW_RenderSettings::MentalRay
        || source.parentHandle != owner || source.reactorCount() != 1
        || source.reactorHandles.size() != 1u || source.reactorHandles[0] != owner
        || source.xDictHandle != xdict || source.extensionDictionaryFlag() != (active ? 0u : 1u)
        || source.flags != 0u || !source.name.empty() || source.hasDataStorageBinaryData()
        || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.hasDwgCommonLinkTail() || source.hasDwgRawData() || !source.dwgRawData().empty()
        || source.dwgRawDataBitSize() != 0u || source.dwgRawDataVersion() != DRW::UNKNOWNV
        || source.m_longs.size() != 16u || source.m_bools.size() != 14u
        || source.m_shorts.size() != 7u || source.m_doubles.size() != 12u
        || source.m_strings.size() != 4u || !source.m_bytes.empty()
        || source.m_fogColorR != 0 || source.m_fogColorG != 0 || source.m_fogColorB != 0
        || source.m_fogDensityNear != 0.0 || source.m_fogDensityFar != 0.0
        || source.m_fogDistanceNear != 0.0 || source.m_fogDistanceFar != 0.0
        || source.m_procedure != 0 || source.m_destination != 0
        || dxfRenderSettings::presetProfile(source) != dxfRenderSettings::PresetProfile::ExtendedMentalRay
        || !dxfRenderSettings::presetProjectionMatches(source, dxfRenderSettings::PresetProfile::ExtendedMentalRay)
        || raw.m_version != DRW::AC1024 || raw.m_objectType != 519u
        || raw.m_handle != source.handle || raw.m_isEntity || !raw.m_isCustomClass
        || !raw.m_hasClassDefinition || raw.m_recordName != "MENTALRAYRENDERSETTINGS"
        || raw.m_className != "AcDbMentalRayRenderSettings" || raw.m_classAppName != "SCENEOE"
        || raw.m_parentHandle != owner || raw.m_numReactors != 1
        || raw.m_reactorHandles != source.reactorHandles || raw.m_xDictHandle != xdict
        || raw.m_xDictFlag != (active ? 0u : 1u) || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != bytes || raw.m_rawBytes.size() != bytes
        || raw.m_bodyBitSize != bytes * 8u - bodyEnd) return false;
    detail::Bits b{raw.m_rawBytes.data(), 0u, textStart};
    if (b.take(2u) != 1u || b.take(8u) + 0x1f0u != 519u) return false;
    const auto self = dx_raster_wire::detail::handle(b, 0u, 0u, true);
    if (self.role != 0u || self.length != 2u || self.value != source.handle
        || b.shortValue() != 0u || b.longValue() != 1u || b.take(1u) != (active ? 0u : 1u)) return false;
    std::size_t li=0, bi=0, si=0, di=0;
    const auto l = [&](int expected) noexcept {return b.longValue() == static_cast<unsigned>(expected) && source.m_longs[li++] == expected;};
    const auto bo = [&](bool expected) noexcept {return b.take(1u) == static_cast<unsigned>(expected) && source.m_bools[bi++] == expected;};
    const auto sh = [&](int expected) noexcept {return b.shortValue() == static_cast<unsigned>(expected) && source.m_shorts[si++] == expected;};
    const auto d = [&](double expected) noexcept {return detail::number(b, expected) && source.m_doubles[di++] == expected;};
    if (!l(2) || !bo(true) || !bo(true) || !bo(true) || !bo(true) || !l(0)
        || !l(2) || !l(active ? 1 : 0) || !l(active ? 2 : 1) || !sh(active ? 4 : 2)
        || !d(active ? 4.0 : 3.0) || !d(active ? 4.0 : 3.0)) return false;
    for (int i=0;i<4;++i) if (!d(0.05000000074505806)) return false;
    if (!sh(0) || !bo(false) || !bo(true)) return false;
    for (int i=0;i<3;++i) if (!l(active ? 9 : 5)) return false;
    if (!bo(!active) || !l(active ? 500 : 250) || !bo(false) || !d(1.0) || !l(10000)) return false;
    for (int i=0;i<3;++i) if (!l(5)) return false;
    if (!bo(false) || !l(active ? 1000 : 100)) return false;
    for (int i=0;i<3;++i) if (!bo(false)) return false;
    if (!d(0.1) || !d(1.0) || !d(1500.0) || !sh(0) || !sh(0) || !d(10.0)
        || !sh(0) || !sh(0) || !bo(!active) || !l(32) || !sh(0) || !l(1048)
        || !bo(false) || !d(1.0) || !b.good || b.pos != b.end
        || li != 16u || bi != 14u || si != 7u || di != 12u) return false;
    detail::Bits strings{raw.m_rawBytes.data(),textStart,bodyEnd-17u};
    if (!detail::text(strings,source.m_strings[0],12u) || !detail::text(strings,source.m_strings[1],0u)
        || !detail::text(strings,source.m_strings[2],active ? 130u : 18u)
        || !detail::text(strings,source.m_strings[3],active ? 0u : 56u)
        || !strings.good || strings.pos != strings.end) return false;
    detail::Bits footer{raw.m_rawBytes.data(),bodyEnd-17u,bodyEnd};
    if (footer.little(2u) != (active ? 2296u : 1408u) || footer.take(1u) != 1u || !footer.good) return false;
    detail::Bits links{raw.m_rawBytes.data(),bodyEnd,static_cast<std::uint32_t>(bytes*8u)};
    const auto own = dx_raster_wire::detail::handle(links,4u,source.handle);
    const auto reactor = dx_raster_wire::detail::handle(links,4u,source.handle);
    if (!links.good || own.role != (active ? 4u : 8u) || own.length != (active ? 1u : 0u) || own.value != owner
        || reactor.role != 4u || reactor.length != (active ? 1u : 2u) || reactor.value != owner) return false;
    if (active) {
        const auto ext=dx_raster_wire::detail::handle(links,3u,source.handle);
        if (!links.good || ext.role != 3u || ext.length != 2u || ext.value != xdict) return false;
    }
    return links.take(4u) == 15u && links.good && links.pos == links.end;
}
inline bool observeDictionary(const DRW_UnsupportedObject& raw,
                              const DRW_Dictionary& source) noexcept {
    using dx_layerstates_dwg_wire::LinkKind;
    const bool extension = source.handle == 0x1cdu;
    if (!extension && source.handle != 0x255u) return false;
    const auto owner = extension ? 0x1ccu : 0xcu;
    if (source.tType != DRW::DICTIONARY || source.parentHandle != owner
        || !source.name.empty() || source.flags != 0u || source.reactorCount() != (extension ? 0 : 1)
        || source.reactorHandles.size() != (extension ? 0u : 1u)
        || (!extension && source.reactorHandles[0] != owner)
        || source.extensionDictionaryFlag() != 1u || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.appData.empty() || !source.extData.empty()
        || !source.pendingAppIdResolutions.empty() || !source.pendingLayerRefResolutions.empty()
        || source.countCap || !source.hasCompleteDwgEntries() || !source.hasDwgCommonLinkTail()
        || source.cloning != 1 || source.hardOwner != (extension ? 1 : 0) || source.m_entries.size() != 1u
        || source.m_entries[0].m_name != (extension ? "ADSK_XREC_MENTALRAYSETTINGSINFO" : "{A9A775EB-42C1-4CA7-972E-D8663542C041}")
        || source.m_entries[0].m_handle != (extension ? 0x1ceu : 0x256u)
        || source.m_entries[0].m_dxfHandleCode != DRW_Dictionary::Entry::DxfHandleCode::Unspecified
        || raw.m_objectSize != (extension ? 78u : 96u) || raw.m_rawBytes.size() != raw.m_objectSize
        || raw.m_bodyBitSize != (extension ? 34u : 58u) || (raw.m_rawBytes.back() & 3u) != 3u) return false;
    return dx_layerstates_dwg_wire::observeDictionary(raw,source,
        [](std::uint32_t,const std::string&) noexcept {return false;},
        [extension](LinkKind kind,std::size_t index,std::uint8_t role,std::uint8_t length) noexcept {
            if (index != 0u) return false;
            if (kind == LinkKind::Owner) return role == (extension ? 8u : 4u) && length == (extension ? 0u : 1u);
            if (kind == LinkKind::Reactor) return !extension && role == 4u && length == 1u;
            return kind == LinkKind::Item && role == 2u && length == 2u;
        });
}
inline bool observeXrecord(const DRW_UnsupportedObject& raw,const DRW_XRecord& source) noexcept {
    using namespace dx_raster_wire;
    if (source.tType != DRW::XRECORD || source.handle != 0x1ceu || source.parentHandle != 0x1cdu
        || !source.name.empty() || source.flags != 0u || source.reactorCount() != 1
        || source.reactorHandles.size() != 1u || source.reactorHandles[0] != 0x1cdu
        || source.extensionDictionaryFlag() != 1u || source.xDictHandle != 0u
        || source.hasDataStorageBinaryData() || !source.hasDwgCommonLinkTail()
        || !source.appData.empty() || !source.extData.empty() || !source.pendingAppIdResolutions.empty()
        || !source.pendingLayerRefResolutions.empty() || source.m_cloning != 1
        || source.m_rawDataValid || !source.m_rawData.empty() || source.hasDwgRawDataSourceRecord()
        || !source.m_handleValues.empty() || source.m_values.size() != 3u || source.m_dataEntries.size() != 3u) return false;
    for (std::size_t i=0;i<3u;++i) {
        const int code=70+static_cast<int>(i), val=i==0u ? 2 : i==1u ? 4 : 0;
        for (const DRW_Variant* v : {&source.m_values[i],&source.m_dataEntries[i]})
            if (v->code()!=code || v->type()!=DRW_Variant::INTEGER || v->i_val()!=val) return false;
    }
    if (raw.m_version != DRW::AC1024 || raw.m_objectType != 79u || raw.m_handle != source.handle
        || raw.m_isEntity || raw.m_isCustomClass || raw.m_hasClassDefinition
        || !raw.m_recordName.empty() || !raw.m_className.empty() || !raw.m_classAppName.empty()
        || raw.m_parentHandle != 0x1cdu || raw.m_numReactors != 1 || raw.m_reactorHandles != source.reactorHandles
        || raw.m_xDictFlag != 1u || raw.m_xDictHandle != 0u || raw.m_hasDataStorage
        || !raw.m_commonHandleDataValidated || !drwDwgCommonLinkEvidenceValidated(raw.m_commonLinkEvidence)
        || raw.m_objectSize != 25u || raw.m_rawBytes.size() != 25u || raw.m_bodyBitSize != 36u) return false;
    detail::Bits b{raw.m_rawBytes.data(),0u,164u};
    if (b.take(2u)!=0u || b.take(8u)!=79u) return false;
    const auto self=dx_raster_wire::detail::handle(b,0u,0u,true);
    if (self.role!=0u || self.length!=2u || self.value!=0x1ceu || b.shortValue()!=0u
        || b.longValue()!=1u || b.take(1u)!=1u || b.longValue()!=12u) return false;
    for (std::size_t i=0;i<3u;++i) {
        const unsigned expected = i==0u ? 2u : i==1u ? 4u : 0u;
        if (b.little(2u)!=70u+i || b.little(2u)!=expected) return false;
    }
    if (b.shortValue()!=1u || b.take(1u)!=0u || !b.good || b.pos!=b.end) return false;
    detail::Bits h{raw.m_rawBytes.data(),164u,200u};
    const auto own=dx_raster_wire::detail::handle(h,4u,source.handle);
    const auto reactor=dx_raster_wire::detail::handle(h,4u,source.handle);
    return h.good && own.role==8u && own.length==0u && own.value==0x1cdu
        && reactor.role==4u && reactor.length==2u && reactor.value==0x1cdu
        && h.take(4u)==15u && h.good && h.pos==h.end;
}
} // namespace dx_ac1024_mentalray_wire
#endif
