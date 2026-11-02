// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_DWG_CLASS_EVIDENCE_H
#define DX_DWG_CLASS_EVIDENCE_H

#include "drw_objects.h"
#include "dx_dwg_class_section_wire.h"
#include <algorithm>
#include <limits>
#include <string_view>

// Converter-owned immutable metadata and independent source correlation. A
// joined result is NOT profile/semantic qualification or output admission.
namespace dx_class_evidence {
enum class State : std::uint8_t {
    NoAttempt, NotDwg, Pending, Joined, CaptureFailed, TooLarge, ContextMismatch,
    ReadFailed, MissingRows, MissingSource, IncompleteRows, SourceUnavailable,
    FramingRefused, RowMismatch, LateCallback
};
enum Issue : std::uint32_t {
    UnselectedSchema = 1u, FalseHeaderFlag = 2u, ScalarAlignment = 4u,
    UnknownTrailer = 8u, UnknownItemClass = 16u
};
struct Result {
    State state{State::NoAttempt};
    std::uint64_t attempt{0}, generation{0};
    DRW::Version version{DRW::UNKNOWNV};
    std::uint32_t rows{0}, framedCandidates{0}, matchedCandidates{0}, profileIssues{0};
    std::uint64_t candidateMask{0}, matchedMask{0};
};
struct Name { std::size_t offset{0}, size{0}; };
struct RasterRoleIndex {
    std::uint32_t matches{0}, ordinal{(std::numeric_limits<std::uint32_t>::max)()};
};
struct Row {
    std::uint64_t ordinal{0};
    std::uint16_t number{0}, itemClass{0};
    std::int32_t proxy{0}, wasProxy{0}, entity{0}, instances{0}, version{0}, maintenance{0};
    std::int32_t unknown1{0}, unknown2{0}, descriptor{-1};
    DRW_DwgClassBitRange data, strings;
    DRW_DwgClassCoverageState state{DRW_DwgClassCoverageState::Parsed};
    DRW_DwgClassCoverageReason reason{DRW_DwgClassCoverageReason::NoReason};
    std::array<Name, 3> names{};
};
struct Rows {
    DRW_DwgClassCoverageStatus status{DRW_DwgClassCoverageStatus::NotAvailable};
    bool complete{false};
    std::vector<Row> entries;
    std::vector<char> text; // One bounded arena, not three allocations per row.
    std::array<RasterRoleIndex, 2> rasterRoles{}; // Definition, reactor; built once before freezing.
    std::string_view name(const Row& row, unsigned index) const noexcept {
        if (index >= row.names.size()) return {};
        const auto span = row.names[index];
        if (span.offset > text.size() || span.size > text.size() - span.offset) return {};
        return {text.empty() ? "" : text.data() + span.offset, span.size};
    }
};
inline std::string_view rasterRecord(bool definition) noexcept {
    return definition ? "IMAGEDEF" : "IMAGEDEF_REACTOR";
}
inline std::string_view rasterClass(bool definition) noexcept {
    return definition ? "AcDbRasterImageDef" : "AcDbRasterImageDefReactor";
}
inline void indexRasterRoles(Rows& rows) noexcept {
    rows.rasterRoles = {};
    for (std::size_t n = 0; n < rows.entries.size(); ++n) {
        const auto& row = rows.entries[n];
        for (unsigned role = 0u; role < 2u; ++role) {
            const bool definition = role == 0u;
            const auto record = rows.name(row, 2u), cls = rows.name(row, 1u);
            if (record != rasterRecord(definition) && cls != rasterClass(definition)) continue;
            auto& index = rows.rasterRoles[role];
            ++index.matches;
            if (record == rasterRecord(definition) && cls == rasterClass(definition)
                && n < (std::numeric_limits<std::uint32_t>::max)()) index.ordinal = static_cast<std::uint32_t>(n);
        }
    }
}
enum class RasterState : std::uint8_t {
    Unobserved, FixedType, NoReadContext, TableUnavailable, AliasedIdentity, Joined, Contradiction
};
struct RasterBinding {
    RasterState state{RasterState::Unobserved};
    std::uint64_t attempt{0}, ordinal{(std::numeric_limits<std::uint64_t>::max)()};
    std::uint16_t number{0};
    std::uint32_t profileIssues{0};
    std::shared_ptr<const Rows> rows;
};
inline bool sameRasterBinding(const RasterBinding& a, const RasterBinding& b) noexcept {
    return a.state == b.state && a.attempt == b.attempt && a.ordinal == b.ordinal
        && a.number == b.number && a.profileIssues == b.profileIssues && a.rows == b.rows;
}
// The caller must first bind the actual read/table authority. This compares
// one custom class row, not the object's body, output or source qualification.
inline RasterState matchRasterRow(const Rows& rows, const DRW_UnsupportedObject& raw,
                                  const DRW_DwgFramePublication& publication, bool definition) noexcept {
    if (!raw.m_isCustomClass || !raw.m_hasClassDefinition || raw.m_isEntity || publication.m_isEntity
        || !publication.m_isCustomClass || !publication.m_classStreamOrdinal
        || publication.m_encodedType != raw.m_objectType || raw.m_objectType < 500
        || *publication.m_classStreamOrdinal >= rows.entries.size()
        || raw.m_recordName != rasterRecord(definition) || raw.m_className != rasterClass(definition)
        || publication.m_recordName != raw.m_recordName || publication.m_className != raw.m_className)
        return RasterState::Contradiction;
    const auto ordinal = *publication.m_classStreamOrdinal;
    const auto& row = rows.entries[static_cast<std::size_t>(ordinal)];
    for (const auto name : row.names)
        if (name.offset > rows.text.size() || name.size > rows.text.size() - name.offset) return RasterState::Contradiction;
    if (row.ordinal != ordinal || row.number != publication.m_encodedType
        || row.state != DRW_DwgClassCoverageState::Published || row.reason != DRW_DwgClassCoverageReason::NoReason
        || row.descriptor < 0 || !row.data.m_present || !row.strings.m_present
        || row.proxy != raw.m_classProxyFlag || row.wasProxy != static_cast<int>(raw.m_classWasProxy)
        || row.itemClass != raw.m_classEntityFlagRaw || row.entity != static_cast<int>(raw.m_isEntity)
        || row.entity != (row.itemClass == 0x1F2u ? 1 : 0)
        || row.version != raw.m_classDwgVersion || row.maintenance != raw.m_classMaintenanceVersion
        || row.unknown1 != raw.m_classUnknown1 || row.unknown2 != raw.m_classUnknown2
        || rows.name(row, 0u) != raw.m_classAppName || rows.name(row, 1u) != raw.m_className
        || rows.name(row, 2u) != raw.m_recordName) return RasterState::Contradiction;
    // Instance count is observed in the table, but intentionally absent from
    // raw object carriers: the writer recomputes emitted counts, never guesses.
    const auto& index = rows.rasterRoles[definition ? 0u : 1u];
    if (index.matches > 1u) return RasterState::AliasedIdentity;
    return index.matches == 1u && index.ordinal == ordinal ? RasterState::Joined : RasterState::Contradiction;
}
inline Row fields(const DRW_DwgClassCoverageEntry& input) noexcept {
    Row row;
    row.ordinal = input.m_streamOrdinal; row.number = input.m_classNumber;
    row.itemClass = input.m_entityFlagRaw; row.proxy = input.m_proxyFlag;
    row.wasProxy = input.m_wasAProxyFlag; row.entity = input.m_entityFlag;
    row.instances = input.m_instanceCount; row.version = input.m_dwgVersion;
    row.maintenance = input.m_maintenanceVersion; row.unknown1 = input.m_unknown1;
    row.unknown2 = input.m_unknown2; row.descriptor = input.m_sectionDescriptorId;
    row.data = input.m_dataRange; row.strings = input.m_stringRange;
    row.state = input.m_state; row.reason = input.m_reason;
    return row;
}
inline bool sameRange(const DRW_DwgClassBitRange& a, const DRW_DwgClassBitRange& b) noexcept {
    return a.m_startBit == b.m_startBit && a.m_endBit == b.m_endBit
        && a.m_present == b.m_present && a.m_sectionRelative == b.m_sectionRelative
        && a.m_offsetSpace == b.m_offsetSpace;
}
inline bool sameFields(const Row& a, const Row& b) noexcept {
    return a.ordinal == b.ordinal && a.number == b.number && a.itemClass == b.itemClass
        && a.proxy == b.proxy && a.wasProxy == b.wasProxy && a.entity == b.entity
        && a.instances == b.instances && a.version == b.version && a.maintenance == b.maintenance
        && a.unknown1 == b.unknown1 && a.unknown2 == b.unknown2 && a.descriptor == b.descriptor
        && a.state == b.state && a.reason == b.reason && sameRange(a.data, b.data) && sameRange(a.strings, b.strings);
}
inline bool matches(const Rows& frozen, const DRW_DwgClassCoverageReport& current) noexcept {
    if (frozen.status != current.m_status || frozen.complete != current.m_complete
        || frozen.entries.size() != current.m_entries.size()) return false;
    for (std::size_t n = 0; n < frozen.entries.size(); ++n) {
        const auto& row = frozen.entries[n]; const auto& input = current.m_entries[n];
        for (const auto span : row.names)
            if (span.offset > frozen.text.size() || span.size > frozen.text.size() - span.offset) return false;
        if (!sameFields(row, fields(input)) || frozen.name(row, 0u) != input.m_appName
            || frozen.name(row, 1u) != input.m_className || frozen.name(row, 2u) != input.m_recordName) return false;
    }
    return true;
}
inline unsigned revision(DRW::Version value) noexcept {
    switch (value) {
    case DRW::AC1012: return 13u; case DRW::AC1014: return 14u; case DRW::AC1015: return 2000u;
    case DRW::AC1018: return 2004u; case DRW::AC1021: return 2007u; case DRW::AC1024: return 2010u;
    case DRW::AC1027: return 2013u; case DRW::AC1032: return 2018u; default: return 0u;
    }
}
enum class TextConvention { Unknown, Exact, SingleNull };
// Select from encoded units BEFORE comparing to any expected/decoded name.
inline TextConvention convention(dx_class_wire::View view, const dx_class_wire::Text& text, bool unicode) noexcept {
    namespace wire = dx_class_wire;
    if (text.issues & (wire::InvalidUnicode | wire::LegacyCodepageUnknown)) return TextConvention::Unknown;
    if (!(text.issues & wire::EmbeddedNull)) return TextConvention::Exact;
    const auto width = unicode ? 16u : 8u;
    if (!wire::detail::extentValid(view) || !text.count || text.units.begin < view.begin
        || text.units.end > view.end || text.units.end < text.units.begin
        || text.units.end - text.units.begin != static_cast<std::uint32_t>(text.count) * width)
        return TextConvention::Unknown;
    wire::detail::Bits units({view.bytes, view.size, text.units.begin, text.units.end});
    for (unsigned n = 0; n < text.count; ++n) {
        const auto value = units.little(unicode ? 2u : 1u);
        if ((value == 0u) != (n + 1u == text.count)) return TextConvention::Unknown;
    }
    return units.good ? TextConvention::SingleNull : TextConvention::Unknown;
}
inline bool nameMatches(dx_class_wire::View view, const dx_class_wire::Text& text,
                        bool unicode, std::string_view expected) noexcept {
    const auto selected = convention(view, text, unicode);
    return selected == TextConvention::Exact
        ? dx_class_wire::equalsUtf8(view, text, unicode, expected.data(), expected.size())
        : selected == TextConvention::SingleNull
            && dx_class_wire::equalsTerminatedUtf8(view, text, unicode, expected.data(), expected.size());
}
inline bool joins(const DRW_DwgClassSourceReport& source, const Rows& rows,
                  const dx_class_wire::SectionProof& proof) noexcept {
    namespace wire = dx_class_wire;
    if (proof.status != wire::Status::Observed || proof.records != rows.entries.size()
        || source.m_baseByteOffset > (std::numeric_limits<std::uint64_t>::max)() / 8u) return false;
    const auto base = source.m_baseByteOffset * 8u;
    auto data = proof.data, strings = proof.strings;
    for (std::size_t n = 0; n < rows.entries.size(); ++n) {
        const auto& row = rows.entries[n];
        const auto record = wire::scan(data, strings, proof.profile.revision, proof.profile.trailer);
        if (record.status != wire::Status::Observed || row.ordinal != n || row.descriptor != source.m_sectionDescriptorId
            || row.state != DRW_DwgClassCoverageState::Published || row.reason != DRW_DwgClassCoverageReason::NoReason
            || record.number != row.number || record.proxy != row.proxy || static_cast<int>(record.zombie) != row.wasProxy
            || record.itemClass != row.itemClass || row.entity != (record.itemClass == 0x1F2u ? 1 : 0)
            || record.instances != row.instances || record.dwgVersion != row.version || record.maintenance != row.maintenance
            || record.unknown1 != row.unknown1 || record.unknown2 != row.unknown2) return false;
        const auto names = record.split ? strings : data;
        for (unsigned name = 0; name < 3u; ++name) {
            const auto span = row.names[name];
            if (span.offset > rows.text.size() || span.size > rows.text.size() - span.offset) return false;
            if (!nameMatches(names, record.text[name], record.split, rows.name(row, name))) return false;
        }
        const auto range = [&](const DRW_DwgClassBitRange& value, std::uint32_t begin, std::uint32_t end) noexcept {
            return value.m_present && value.m_offsetSpace == source.m_offsetSpace
                && value.m_sectionRelative == (source.m_offsetSpace == DRW_DwgFrameOffsetSpace::DecodedBuffer)
                && begin <= (std::numeric_limits<std::uint64_t>::max)() - base
                && end <= (std::numeric_limits<std::uint64_t>::max)() - base
                && value.m_startBit == base + begin && value.m_endBit == base + end;
        };
        if (!range(row.data, data.begin, record.dataEnd)
            || !range(row.strings, record.split ? strings.begin : data.begin, record.stringEnd)) return false;
        data.begin = record.dataEnd;
        if (record.split) strings.begin = record.stringEnd;
    }
    return data.begin == data.end && (!proof.split || strings.begin == strings.end);
}
inline Result observe(const DRW_DwgClassSourceReport& source, const Rows& rows) noexcept {
    namespace wire = dx_class_wire;
    Result result; result.version = source.m_version;
    if (rows.entries.size() > 65036u) { result.state = State::TooLarge; return result; }
    result.rows = static_cast<std::uint32_t>(rows.entries.size());
    if (!rows.complete || rows.status != DRW_DwgClassCoverageStatus::FinalizedComplete) {
        result.state = State::IncompleteRows; return result;
    }
    if (source.m_status != DRW_DwgClassSourceStatus::Captured || !source.m_bytes
        || source.m_byteCount == 0u || source.m_byteCount != source.m_bytes->size()
        || source.m_sectionDescriptorId < 0 || source.m_declaredSectionSize == 0u
        || source.m_declaredSectionSize > source.m_byteCount
        || (source.m_offsetSpace != DRW_DwgFrameOffsetSpace::PhysicalFile
            && source.m_offsetSpace != DRW_DwgFrameOffsetSpace::DecodedBuffer)
        || (source.m_offsetSpace == DRW_DwgFrameOffsetSpace::DecodedBuffer && source.m_baseByteOffset != 0u)
        || (source.m_offsetSpace == DRW_DwgFrameOffsetSpace::PhysicalFile && source.m_declaredSectionSize != source.m_byteCount)
        || !source.m_classReadCompleted || !source.m_parserCoverageComplete) {
        result.state = State::SourceUnavailable; return result;
    }
    wire::SectionProfile profile;
    profile.revision = revision(source.m_version); profile.maintenance = source.m_appMaintenanceVersion;
    profile.requireTrueHeaderFlag = false; profile.scalarByteAlignment = true;
    profile.zeroEnvelopePadding = source.m_offsetSpace == DRW_DwgFrameOffsetSpace::DecodedBuffer;
    unsigned candidate = 0u;
    for (auto trailer : {wire::Trailer::OdaBitShort, wire::Trailer::ProducerBitLong}) {
        if (profile.revision < 2004u && trailer == wire::Trailer::ProducerBitLong) continue;
        profile.trailer = trailer;
        const bool uncertainHigh = profile.revision >= 2018u && profile.maintenance <= 3u;
        for (unsigned rule = 0; rule < (uncertainHigh ? 2u : 1u); ++rule) {
            profile.highSizeRule = rule ? wire::ClassHighSizeRule::R2018Always : wire::ClassHighSizeRule::MaintenanceGate;
            const bool high = rule || (profile.revision >= 2010u && profile.maintenance > 3u);
            for (unsigned size = 0; size < (high ? 2u : 1u); ++size) {
                profile.sizeLayout = size ? wire::ClassSizeLayout::ExcludesHighWord : wire::ClassSizeLayout::IncludesHighWord;
                const auto proofs = wire::observeSectionTails(source.m_bytes->data(), source.m_bytes->size(), profile);
                bool joined = false, joinChecked = false;
                for (const auto& proof : proofs) {
                    const auto tail = proof.profile.tail;
                    if (profile.revision < 2004u && tail != wire::ClassTail::SentinelOnly) continue;
                    const auto id = candidate++;
                    if (id >= 64u) { result.state = State::FramingRefused; return result; }
                    if (proof.status != wire::Status::Observed) continue;
                    ++result.framedCandidates; result.candidateMask |= UINT64_C(1) << id;
                    if (profile.revision >= 2004u && !proof.headerFlag) result.profileIssues |= FalseHeaderFlag;
                    if (proof.split && proof.scalarPadding.begin != proof.scalarPadding.end) result.profileIssues |= ScalarAlignment;
                    if (tail != wire::ClassTail::SentinelOnly) result.profileIssues |= UnknownTrailer;
                    if (proof.unknownItemClass) result.profileIssues |= UnknownItemClass;
                    // Tail variants share record/text streams, not semantic
                    // meaning. Match once, but retain every framing candidate.
                    if (!joinChecked) { joined = joins(source, rows, proof); joinChecked = true; }
                    if (joined) {
                        ++result.matchedCandidates; result.matchedMask |= UINT64_C(1) << id;
                    }
                }
            }
        }
    }
    if (result.framedCandidates > 1u) result.profileIssues |= UnselectedSchema;
    result.rows = static_cast<std::uint32_t>(rows.entries.size());
    result.state = result.framedCandidates == 0u ? State::FramingRefused
        : result.candidateMask != result.matchedMask ? State::RowMismatch : State::Joined;
    return result;
}
} // namespace dx_class_evidence
#endif
