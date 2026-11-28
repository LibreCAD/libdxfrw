// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_legacy_dwg_wire.h"
#include "dx_legacy_mlinestyle_wire.h"
#include "dx_legacy_layout_wire.h"
#include "dx_legacy_block_record_wire.h"
#include "test_scratch.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <set>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static dx_legacy_frame_census::Capture& legacyCensus(dx_data& data) {
        return data.m_legacyFrameTypeCensus;
    }
    static dx_legacy_frame_census::Capture emptyLayoutCensus(const dx_data& data) {
        dx_legacy_frame_census::Capture capture;
        capture.begin(data.importGeneration, data.m_classEvidenceAttempt);
        DRW_DwgFramePublication row;
        row.m_version = data.sourceVersion;
        row.m_handle = 1u;
        row.m_encodedType = 42;
        row.m_hasSourceLocation = true;
        row.m_sourceOffset = 100u;
        row.m_sourceOffsetSpace = DRW_DwgFrameOffsetSpace::PhysicalFile;
        capture.observe(row, data.importGeneration, data.m_classEvidenceAttempt);
        DwgReadFrameObservations report;
        report.status = DRW_DwgFrameCoverageStatus::FinalizedComplete;
        report.sourceFrames = 1u;
        report.dispositions[static_cast<std::size_t>(DRW_DwgFrameDisposition::Published)] = 1u;
        capture.seal(data.sourceVersion, true, report);
        return capture;
    }
    static bool legacyClassCount(const dx_data& data, std::uint16_t number,
                                 std::size_t& count) {
        return data.legacyClassFrameCount(number, count);
    }
    static bool boundLegacyLayout(const dx_iface& adapter) {
        return adapter.hasBoundLegacyLayoutSources();
    }
    static bool legacyBlockFrames(const dx_data& data, const std::string& input) {
        if (data.m_legacyBlockRecordSourceFailed || data.blockRecords.empty()
            || data.m_legacyBlockRecordSources.size() != data.blockRecords.size()) return false;
        dwgBuffer source(reinterpret_cast<const std::uint8_t*>(input.data()), input.size());
        std::set<std::uint32_t> offsets;
        std::set<std::uint64_t> ordinals;
        for (const auto& record : data.blockRecords) {
            const auto found = data.m_legacyBlockRecordSources.find(record.handle);
            if (found == data.m_legacyBlockRecordSources.end()) return false;
            const auto& captured = found->second;
            DwgObjectFrame frame;
            if (captured.generation != data.importGeneration || captured.version != data.sourceVersion
                || captured.handle != record.handle
                || captured.space != DRW_DwgFrameOffsetSpace::PhysicalFile
                || !offsets.insert(captured.offset).second
                || !ordinals.insert(captured.ordinal).second
                || !frame.readAt(source, captured.version, captured.offset)
                || frame.body() != captured.body) return false;
        }
        return true;
    }
    static bool legacyBlockWire(const dx_data& data) {
        for (const auto& record : data.blockRecords) {
            const auto found = data.m_legacyBlockRecordSources.find(record.handle);
            if (found == data.m_legacyBlockRecordSources.end()) return false;
            const auto& source = found->second;
            dx_legacy_block_record_wire::Proof proof;
            if (!dx_legacy_block_record_wire::record(source.version, source.body, record, &proof)) {
                std::cerr << "BLOCK_HEADER wire mismatch handle=" << record.handle
                          << " name=" << record.name << " flags=" << record.flags
                          << " xrefIndex=" << record.xrefIndex << " parent=" << record.parentHandle
                          << " spans=" << record.dwgHandleFieldSpans().size() << '\n';
                return false;
            }
            std::cout << "legacy-block handle=" << record.handle
                      << " bodyBitEnd=" << proof.bodyEnd << " objectOffset=" << source.offset
                      << " spans=";
            for (const auto& span : proof.handles)
                std::cout << span.begin << ':' << span.end << ',';
            std::cout << '\n';
            DRW_Block_Record edited = record;
            edited.basePoint.x += 1.0;
            if (dx_legacy_block_record_wire::record(source.version, source.body, edited)) return false;
            edited = record; edited.parentHandle ^= 1u;
            if (dx_legacy_block_record_wire::record(source.version, source.body, edited)) return false;
            for (const auto field : {dx_legacy_block_record_wire::Field::Control,
                    dx_legacy_block_record_wire::Field::XDictionary, dx_legacy_block_record_wire::Field::Xref,
                    dx_legacy_block_record_wire::Field::Block, dx_legacy_block_record_wire::Field::First,
                    dx_legacy_block_record_wire::Field::Last, dx_legacy_block_record_wire::Field::End}) {
                const auto span = proof.handles[static_cast<std::size_t>(field)];
                if (!span.present) continue;
                auto damaged = source.body;
                damaged[span.begin / 8u] ^= static_cast<std::uint8_t>(0x80u >> (span.begin % 8u));
                if (dx_legacy_block_record_wire::record(source.version, damaged, record)) return false;
            }
            if (!dx_legacy_block_record_wire::record(source.version, source.body, record)) return false;
        }
        return true;
    }
    static std::vector<std::uint8_t>& blockBytes(dx_data& data, std::uint32_t handle) {
        return data.m_legacyBlockRecordSources.at(handle).body;
    }
    static std::uint64_t& blockGeneration(dx_data& data, std::uint32_t handle) {
        return data.m_legacyBlockRecordSources.at(handle).generation;
    }
    static bool noLegacyBlockFrames(const dx_data& data) {
        return data.m_legacyBlockRecordSources.empty()
            && data.m_legacyBlockRecordSourceBytes == 0u;
    }
    static std::vector<std::uint8_t>& layoutBytes(dx_data& data, std::uint32_t handle) {
        return data.m_opaqueDwgSourceFrames.at({data.importGeneration, handle}).m_rawBytes;
    }
    static std::vector<std::uint8_t>& dictionaryBytes(dx_data& data, std::uint32_t handle) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, handle}).raw.m_rawBytes;
    }
    static bool legacyLayoutWire(const dx_data& data, const std::string& source) {
        if ((data.sourceVersion != DRW::AC1012 && data.sourceVersion != DRW::AC1014)
            || data.layouts.size() != 2u || !data.hasUnchangedDwgClassEvidence()
            || !data.dwgClassCoverage.m_complete
            || data.dwgClassCoverage.m_status != DRW_DwgClassCoverageStatus::FinalizedComplete)
            return false;
        for (const auto& layout : data.layouts) {
            const dx_data::StandardDwgFrameKey key{data.importGeneration, layout.handle};
            const auto found = data.m_opaqueDwgSourceFrames.find(key);
            if (found == data.m_opaqueDwgSourceFrames.end()
                || data.m_opaqueDwgSourceFrameIdentities.count(key) != 1u)
                return false;
            const auto& raw = found->second;
            dx_legacy_layout_wire::Proof proof;
            if (!dx_legacy_layout_wire::layout(raw, layout, &proof)) {
                std::cerr << "legacy LAYOUT wire mismatch: handle=" << layout.handle
                          << " type=" << raw.m_objectType << " record=" << raw.m_recordName
                          << " class=" << raw.m_className << " custom=" << raw.m_isCustomClass
                          << " body=" << raw.m_bodyBitSize << " size=" << raw.m_objectSize
                          << " common=" << layout.hasDwgCommonLinkTail() << '\n';
                return false;
            }
            if (raw.m_objectOffset > source.size()
                || source.size() - raw.m_objectOffset < 2u) return false;
            const auto offset = raw.m_objectOffset + 2u;
            if (offset > source.size() || raw.m_rawBytes.size() > source.size() - offset
                || !std::equal(raw.m_rawBytes.begin(), raw.m_rawBytes.end(),
                    source.begin() + static_cast<std::ptrdiff_t>(offset),
                    [](std::uint8_t a, char b) { return a == static_cast<std::uint8_t>(b); }))
                return false;
            if (!dx_legacy_layout_wire::matchesClass(data.dwgClassCoverage,
                                                     proof.sourceClass)) return false;
            std::cout << "legacy-layout handle=" << layout.handle
                      << " class=" << proof.sourceClass << " bodyBitEnd=" << proof.bodyEnd
                      << " objectOffset=" << raw.m_objectOffset << " spans=";
            for (const auto& span : proof.handles)
                std::cout << span.begin << ':' << span.end << ',';
            std::cout << '\n';
            DRW_Layout changed = layout;
            ++changed.paperSpaceBlockRecordHandle.ref;
            if (dx_legacy_layout_wire::layout(raw, changed)) return false;
            changed = layout;
            changed.tabOrder += 1;
            if (dx_legacy_layout_wire::layout(raw, changed)) return false;
            for (const auto field : {dx_legacy_layout_wire::Field::Owner,
                    dx_legacy_layout_wire::Field::XDictionary, dx_legacy_layout_wire::Field::BlockRecord,
                    dx_legacy_layout_wire::Field::Viewport, dx_legacy_layout_wire::Field::BaseUcs,
                    dx_legacy_layout_wire::Field::NamedUcs}) {
                auto damaged = raw;
                const auto span = proof.handles[static_cast<std::size_t>(field)];
                if (!span.present || span.end <= span.begin) return false;
                damaged.m_rawBytes[span.begin / 8u] ^= static_cast<std::uint8_t>(0x80u >> (span.begin % 8u));
                if (dx_legacy_layout_wire::layout(damaged, layout)) return false;
            }
            if (!dx_legacy_layout_wire::layout(raw, layout)) return false;
        }
        return true;
    }
    static bool legacyRawFrame(const dx_data& data, std::uint32_t handle,
                               const std::string& source, std::size_t bodyOffset,
                               std::size_t size, const char* record, const char* name) {
        const dx_data::StandardDwgFrameKey key{data.importGeneration, handle};
        const auto found = data.m_opaqueDwgSourceFrames.find(key);
        if (found == data.m_opaqueDwgSourceFrames.end()) return false;
        const auto& raw = found->second;
        return raw.m_version == data.sourceVersion && raw.m_handle == handle
            && raw.m_isCustomClass && !raw.m_isEntity
            && raw.m_recordName == record && raw.m_className == name
            && !raw.m_typedPayloadValidated && bodyOffset >= 2u
            && raw.m_objectOffset == bodyOffset - 2u && raw.m_objectSize == size
            && raw.m_rawBytes.size() == size && bodyOffset <= source.size()
            && size <= source.size() - bodyOffset
            && std::equal(raw.m_rawBytes.begin(), raw.m_rawBytes.end(),
                source.begin() + static_cast<std::ptrdiff_t>(bodyOffset),
                [](std::uint8_t left, char right) {
                    return left == static_cast<std::uint8_t>(right);
                })
            && data.m_opaqueDwgSourceFrameIdentities.count(key) == 1u
            && data.m_opaqueDwgOutputQualifiedFrameIdentities.count(key) == 0u;
    }
    static bool legacyRawPair(const dx_data& data, const std::string& source,
                               std::uint32_t cell, std::size_t cellOffset,
                               std::size_t defaultOffset, std::size_t defaultSize) {
        const auto defaultDictionary = std::find_if(
            data.dictionaryWithDefaults.begin(), data.dictionaryWithDefaults.end(),
            [](const DRW_DictionaryWithDefault& item) { return item.handle == 0x12u; });
        return legacyRawFrame(data, cell, source, cellOffset, 708u,
                              "CELLSTYLEMAP", "AcDbCellStyleMap")
            && legacyRawFrame(data, 0x12u, source, defaultOffset, defaultSize,
                              "ACDBDICTIONARYWDFLT", "AcDbDictionaryWithDefault")
            && std::none_of(data.cellStyleMaps.begin(), data.cellStyleMaps.end(),
                [cell](const DRW_CellStyleMap& item) { return item.handle == cell; })
            && defaultDictionary != data.dictionaryWithDefaults.end()
            && std::count_if(data.dictionaryWithDefaults.begin(), data.dictionaryWithDefaults.end(),
                [](const DRW_DictionaryWithDefault& item) { return item.handle == 0x12u; }) == 1
            && defaultDictionary->cloning == 1 && defaultDictionary->hardOwner == 0
            && defaultDictionary->parentHandle == 0xau
            && defaultDictionary->m_defaultEntryHandle == 0x13u
            && defaultDictionary->m_entries.size() == 1u
            && defaultDictionary->m_entries[0].m_name == "NORMAL"
            && defaultDictionary->m_entries[0].m_handle == 0x13u;
    }
    static bool hasOpaqueSource(const dx_data& data, std::uint32_t handle) {
        return data.m_opaqueDwgSourceFrameIdentities.count({data.importGeneration, handle}) != 0u;
    }
    static bool unchangedAppId(const dx_data& data, std::uint32_t handle,
                               const std::string& name) noexcept {
        const auto snapshot = data.m_standardDwgAppIdSources.find({data.importGeneration, handle});
        if (snapshot == data.m_standardDwgAppIdSources.end() || !snapshot->second
            || snapshot->second->name != name || !snapshot->second->hasUnchangedDwgSourceName()
            || snapshot->second->dwgNameDecodeIssues() != 0u) return false;
        const DRW_AppId* live = nullptr;
        for (const auto& candidate : data.appIds) {
            if (candidate.handle != handle) continue;
            if (live) return false;
            live = &candidate;
        }
        return live && live->name == name && live->sameDwgNameSource(*snapshot->second);
    }
    // Literal wire observation only; EED semantics and output remain unproved.
    static bool r13EedSource(const dx_data& data) {
        const dx_data::StandardDwgFrameKey key{data.importGeneration, 0x30u};
        const auto found = data.m_standardDwgObjectFrames.find(key);
        if (found == data.m_standardDwgObjectFrames.end()) return false;
        const auto& frame = found->second;
        const auto* model = dynamic_cast<const DRW_Dictionary*>(frame.typedBaseline.get());
        if (!model || frame.generation != data.importGeneration || !frame.payloadMatched
            || !frame.sourceProfileBound || !frame.textDecodeObserved
            || frame.textDecodeIssues != 0u
            || frame.appIdDependencies != std::vector<std::uint32_t>{0x2au}
            || data.m_opaqueDwgSourceFrameIdentities.count(key) != 1u) return false;
        const auto app = [&data](std::uint32_t handle, const std::string& name) noexcept {
            return handle == 0x2au && name == "ACAD" && unchangedAppId(data, handle, name);
        };
        const auto literalPage = [](std::uint16_t be, std::uint16_t le) noexcept {
            return be == 45u && le == 0x2d00u;
        };
        return !dx_legacy_dwg_wire::dictionary(frame.raw, *model, false)
            && dx_legacy_dwg_wire::observeDictionaryEed(frame.raw, *model, false, app, literalPage);
    }
    // This checks the physical UTF-8 EED profile and unchanged APPID identity;
    // it does not qualify the dictionary for output.
    static bool legacyUtf8EedSource(const dx_data& data) {
        const dx_data::StandardDwgFrameKey key{data.importGeneration, 0x31u};
        const auto found = data.m_standardDwgObjectFrames.find(key);
        if (found == data.m_standardDwgObjectFrames.end()) return false;
        const auto& frame = found->second;
        const auto* model = dynamic_cast<const DRW_Dictionary*>(frame.typedBaseline.get());
        if (!model || frame.generation != data.importGeneration || !frame.payloadMatched
            || !frame.sourceProfileBound || !frame.textDecodeObserved
            || frame.textDecodeIssues != 0u
            || data.m_opaqueDwgSourceFrameIdentities.count(key) != 1u
            || data.m_opaqueDwgOutputQualifiedFrameIdentities.count(key) != 0u) return false;
        const auto app = [&data](std::uint32_t handle, const std::string& name) noexcept {
            return ((handle == 0x2fu && name == "CODEPAGE_TEST")
                || (handle == 0x2au && name == "ACAD")) && unchangedAppId(data, handle, name);
        };
        const auto page = [](std::uint16_t be, std::uint16_t le) noexcept {
            return be == 45u && le == 0x2d00u;
        };
        return !dx_legacy_dwg_wire::dictionary(frame.raw, *model, false)
            && dx_legacy_dwg_wire::observeDictionaryEed(frame.raw, *model, false, app, page);
    }
    static bool importBoundGroupEed(const dx_data& data, std::uint32_t handle = 0xdu) {
        const auto frame = data.m_standardDwgObjectFrames.find({data.importGeneration, handle});
        return frame != data.m_standardDwgObjectFrames.end() && frame->second.legacyEedWireAtImport;
    }

    static std::vector<std::uint8_t>& legacyGroupEedBytes(dx_data& data, std::uint32_t handle) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, handle}).raw.m_rawBytes;
    }
    static std::size_t legacyLayerBit(const dx_data& data, std::uint32_t handle) {
        const auto& raw = data.m_standardDwgObjectFrames.at({data.importGeneration, handle}).raw;
        if (raw.m_rawBytes.empty() || raw.m_rawBytes.size() > UINT32_MAX / 8u)
            return (std::numeric_limits<std::size_t>::max)();
        dx_raster_wire::detail::Bits bits{raw.m_rawBytes.data(), 0u,
            static_cast<std::uint32_t>(raw.m_rawBytes.size() * 8u)};
        if (bits.shortValue() != 42u || !dx_legacy_dwg_wire::reference(bits, 0u, 0u, handle, true))
            return (std::numeric_limits<std::size_t>::max)();
        dx_raster_wire::Proof proof;
        std::size_t found = (std::numeric_limits<std::size_t>::max)();
        const auto visitor = [&found](const dx_raster_wire::EedToken& token) noexcept {
            if (token.code != 1003u) return true;
            if (found != (std::numeric_limits<std::size_t>::max)() || token.valueSpan.width() != 64u)
                return false;
            found = token.valueSpan.begin + 63u;
            return true;
        };
        const auto revision = raw.m_version == DRW::AC1012 ? 13u : 14u;
        return dx_raster_wire::detail::eed(bits, revision, proof, visitor) == dx_raster_wire::Status::Proven
            ? found : (std::numeric_limits<std::size_t>::max)();
    }
    static std::vector<std::uint8_t>& legacyUtf8EedBytes(dx_data& data) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, 0x31u}).raw.m_rawBytes;
    }
    static std::vector<std::uint8_t>& r13EedBytes(dx_data& data) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, 0x30u}).raw.m_rawBytes;
    }
    static std::vector<std::uint8_t>& mlineSourceBytes(dx_data& data, std::uint32_t handle) {
        return data.m_opaqueDwgSourceFrames.at({data.importGeneration, handle}).m_rawBytes;
    }
    static bool cleanDictionaryText(const dx_data& data, std::uint32_t handle) {
        const auto found = data.m_standardDwgObjectFrames.find({data.importGeneration, handle});
        return found != data.m_standardDwgObjectFrames.end() && found->second.payloadMatched
            && found->second.sourceProfileBound && found->second.hasNoObservedTextDecodeIssues();
    }
    static bool dictionaryProof(const dx_data& data, std::uint32_t handle) {
        const auto frame = data.m_standardDwgObjectFrames.find({data.importGeneration, handle});
        if (frame == data.m_standardDwgObjectFrames.end() || !frame->second.typedBaseline)
            return false;
        const auto* model = dynamic_cast<const DRW_Dictionary*>(frame->second.typedBaseline.get());
        return model && dx_legacy_dwg_wire::dictionary(frame->second.raw, *model,
                                                       model->parentHandle == 0u);
    }
    static bool styleProof(const dx_data& data, std::uint32_t handle) {
        const auto frame = data.m_opaqueDwgSourceFrames.find({data.importGeneration, handle});
        return frame != data.m_opaqueDwgSourceFrames.end() && data.mLineStyles.size() == 1u
            && dx_legacy_mlinestyle_wire::style(frame->second, data.mLineStyles.front());
    }
    static std::uint8_t& sourceBit(dx_data& data, int fault) {
        const std::uint32_t handle = fault == 7 ? 0xcu : fault == 8 ? 0x22u
            : fault == 9 ? 0x23u : fault == 10 ? 0x1au : fault == 12 ? 0x19u
            : fault == 13 ? 0x18u : fault == 14 ? 0xdu : fault == 15 ? 0x34u : fault == 16 ? 0x35u
            : fault == 17 ? 0x17u : 0x1bu;
        const dx_data::StandardDwgFrameKey key{data.importGeneration, handle};
        if (fault == 7 || fault == 8 || fault == 10 || fault == 12 || fault == 14 || fault == 15 || fault == 16 || fault == 17)
            return data.m_standardDwgObjectFrames.at(key).raw.m_rawBytes.front();
        return data.m_opaqueDwgSourceFrames.at(key).m_rawBytes.front();
    }
};

namespace {
class FailingLegacyBlockCaptureAdapter final : public dx_iface {
public:
    bool injectFailure = true;
    void legacyBlockRecordSource(const DwgLegacyBlockRecordSourceTicket& ticket) noexcept override {
        if (injectFailure) legacyBlockRecordSourceFailed();
        else dx_iface::legacyBlockRecordSource(ticket);
    }
};

class LateMutationAdapter final : public dx_iface {
public:
    dx_data* source = nullptr;
    int lateFault = 0;
    bool finalizeDxfWrite() override {
        if (lateFault >= 7)
            DxIfaceStandardDwgCaptureTestAccess::sourceBit(*source, lateFault) ^= 1u;
        const bool result = dx_iface::finalizeDxfWrite();
        if (lateFault >= 7)
            DxIfaceStandardDwgCaptureTestAccess::sourceBit(*source, lateFault) ^= 1u;
        return result;
    }
};

bool write(dx_iface& adapter, dx_data& source, const std::string& path, bool binary,
           DRW::Version version = DRW::AC1014,
           dx_dwgReadCompletenessPolicy policy = dx_dwgReadCompletenessPolicy::RequireComplete) {
    return adapter.fileExport(path, version, binary, &source, false, false,
        dx_dxfSourceTextPolicy::Compatible, DRW::DxfTextWritePolicy::Compatible,
        dx_dxfProxyGraphicsPolicy::Compatible, policy);
}

std::string bytes(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

bool run(const std::string& input, const std::filesystem::path& output) {
    LateMutationAdapter adapter;
    dx_data source;
    adapter.source = &source;
    if (!adapter.fileImport(input, &source, false)
        || source.sourceVersion != DRW::AC1014
        || source.dictionaryVars.size() != 1u || source.mLineStyles.size() != 1u)
        return false;
    auto& variable = source.dictionaryVars.front();
    auto& style = source.mLineStyles.front();
    if (variable.handle != 0x23u || variable.m_value != "0"
        || style.handle != 0x1bu || style.elements.size() != 2u)
        return false;
    const std::string sentinel = "AC1014 destination preserved\n";
    for (const bool binary : {false, true}) {
        if (!write(adapter, source, output.string(), binary)) return false;
        for (const auto version : {DRW::AC1015, DRW::AC1024}) {
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            if (write(adapter, source, output.string(), binary, version)
                || bytes(output) != sentinel
                || !write(adapter, source, output.string(), binary)) {
                std::cerr << "AC1014 cross-version refusal/retry failed\n";
                return false;
            }
        }
        for (int fault = 0; fault < 12; ++fault) {
            const DRW_DictionaryVar originalVariable = variable;
            const DRW_MLineStyle originalStyle = style;
            const DRW_Dictionary originalNod = source.namedObjectDictionary;
            if (fault == 0) variable.m_value = "1";
            if (fault == 1) variable.m_schema = 1;
            if (fault == 2) variable.parentHandle = 0x1au;
            if (fault == 3) variable.reactorHandles.front() = 0x1au;
            if (fault == 4) style.elements.front().offset = 0.6;
            if (fault == 5) style.parentHandle = 0x22u;
            if (fault == 6) source.namedObjectDictionary.m_entries.back().m_name += "X";
            if (fault >= 7) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            const bool accepted = write(adapter, source, output.string(), binary);
            variable = originalVariable;
            style = originalStyle;
            source.namedObjectDictionary = originalNod;
            if (fault >= 7) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
            if (accepted || bytes(output) != sentinel
                || !write(adapter, source, output.string(), binary)) {
                std::cerr << "AC1014 refusal/preservation/retry failed: binary="
                          << binary << " fault=" << fault << '\n';
                return false;
            }
        }
        for (int fault = 7; fault < 12; ++fault) {
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            adapter.lateFault = fault;
            const bool accepted = write(adapter, source, output.string(), binary);
            adapter.lateFault = 0;
            if (accepted || bytes(output) != sentinel
                || !write(adapter, source, output.string(), binary)) {
                std::cerr << "AC1014 late refusal/preservation/retry failed: binary="
                          << binary << " fault=" << fault << '\n';
                return false;
            }
        }
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)
            || readback.sourceVersion != DRW::AC1014
            || readback.dictionaryVars.size() != 1u
            || readback.dictionaryVars.front().handle != 0x23u
            || readback.dictionaryVars.front().parentHandle != 0x22u
            || readback.dictionaryVars.front().m_value != "0"
            || readback.mLineStyles.size() != 1u
            || readback.mLineStyles.front().handle != 0x1bu
            || readback.mLineStyles.front().parentHandle != 0x1au
            || readback.mLineStyles.front().elements.size() != 2u
            || readback.mLineStyles.front().elements[0].offset != 0.5
            || readback.mLineStyles.front().elements[1].offset != -0.5)
            return false;
    }
    return true;
}

bool partial(const std::string& input, const std::filesystem::path& output) {
    LateMutationAdapter adapter;
    dx_data source;
    adapter.source = &source;
    if (!adapter.fileImport(input, &source, false)
        || source.sourceVersion != DRW::AC1014
        || source.dwgReadOutcome.observations.entityParseFailures != 1u
        || source.dwgReadOutcome.observations.objectParseFailures != 0u
        || source.dwgReadOutcome.observations.frames.sourceFrames != 51u
        || source.mBlock == nullptr || source.dictionaryVars.size() != 1u)
        return false;
    const auto countLines = [](const dx_data& data) {
        std::size_t count = 0u;
        for (const DRW_Entity* entity : data.mBlock->ent)
            if (entity != nullptr && entity->eType == DRW::LINE) ++count;
        return count;
    };
    if (countLines(source) != 15u) return false;
    bool laterLine = false;
    for (const DRW_Entity* entity : source.mBlock->ent)
        if (entity != nullptr && entity->eType == DRW::LINE && entity->handle == 0x25u)
            laterLine = true;
    if (!laterLine) return false;
    const std::string sentinel = "AC1014 partial destination preserved\n";
    const auto allow = dx_dwgReadCompletenessPolicy::AllowPartial;
    for (const bool binary : {false, true}) {
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, source, output.string(), binary)
            || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary, DRW::AC1014, allow))
            return false;
        auto& variable = source.dictionaryVars.front();
        variable.m_value = "invalid";
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        const bool accepted = write(adapter, source, output.string(), binary, DRW::AC1014, allow);
        variable.m_value = "0";
        if (accepted || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary, DRW::AC1014, allow))
            return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)
            || readback.sourceVersion != DRW::AC1014 || readback.mBlock == nullptr
            || countLines(readback) != 15u || readback.dictionaryVars.size() != 1u
            || readback.dictionaryVars.front().m_value != "0")
            return false;
    }
    return true;
}

bool checkPartialReadback(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false) || data.sourceVersion != DRW::AC1014
        || data.mBlock == nullptr || data.dictionaryVars.size() != 1u
        || data.dictionaryVars.front().m_value != "0") return false;
    std::size_t lines = 0u, texts = 0u;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        if (entity == nullptr) return false;
        if (entity->eType == DRW::LINE) ++lines;
        if (entity->eType == DRW::TEXT) ++texts;
    }
    return lines == 15u && texts == 1u;
}

bool r13Partial(const std::string& input, const std::filesystem::path& output) {
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(input, &source, false)
        || source.sourceVersion != DRW::AC1012 || source.mBlock == nullptr
        || source.dwgReadOutcome.observations.entityParseFailures != 1u
        || source.dwgReadOutcome.observations.objectParseFailures != 0u
        || source.dwgReadOutcome.observations.frames.sourceFrames != 33u)
        return false;
    DRW_Line* line = nullptr;
    std::size_t count = 0u;
    bool later = false;
    for (DRW_Entity* entity : source.mBlock->ent) {
        if (entity != nullptr && entity->eType == DRW::LINE) {
            ++count;
            if (entity->handle == 0x2fu) {
                later = true;
                line = dynamic_cast<DRW_Line*>(entity);
            }
        }
    }
    if (!later || line == nullptr || count != 2u) return false;
    const auto allow = dx_dwgReadCompletenessPolicy::AllowPartial;
    const std::string sentinel = "AC1012 partial destination preserved\n";
    for (const bool binary : {false, true}) {
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, source, output.string(), binary)
            || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary, DRW::AC1014, allow))
            return false;
        const double old = line->basePoint.x;
        line->basePoint.x = std::numeric_limits<double>::quiet_NaN();
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        const bool accepted = write(adapter, source, output.string(), binary, DRW::AC1014, allow);
        line->basePoint.x = old;
        if (accepted || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary, DRW::AC1014, allow))
            return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)
            || readback.sourceVersion != DRW::AC1014 || readback.mBlock == nullptr)
            return false;
        std::size_t readLines = 0u;
        for (const DRW_Entity* entity : readback.mBlock->ent)
            if (entity != nullptr && entity->eType == DRW::LINE) ++readLines;
        if (readLines != 2u) return false;
    }
    return true;
}

bool checkR13PartialReadback(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false) || data.sourceVersion != DRW::AC1014
        || data.mBlock == nullptr) return false;
    bool first = false, second = false;
    std::size_t lines = 0u;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        const auto* line = dynamic_cast<const DRW_Line*>(entity);
        if (line == nullptr || entity->eType != DRW::LINE) continue;
        ++lines;
        if (line->basePoint.x == 2.0 && line->basePoint.y == 3.0
            && line->basePoint.z == 4.0 && line->secPoint.x == 5.0
            && line->secPoint.y == 7.0 && line->secPoint.z == 10.0) first = true;
        if (line->basePoint.x == 3.0 && line->basePoint.y == 4.0
            && line->basePoint.z == 5.0 && line->secPoint.x == 6.0
            && line->secPoint.y == 8.0 && line->secPoint.z == 11.0) second = true;
    }
    return lines == 2u && first && second;
}

bool checkR14Generated(const dx_data& data, DRW::Version sourceVersion = DRW::AC1014) {
    if (data.sourceVersion != sourceVersion || data.mBlock == nullptr
        || data.mLineStyles.size() != 1u || data.mLineStyles.front().name != "Standard") return false;
    bool seen[3] = {false, false, false};
    std::size_t count = 0u;
    for (const DRW_Entity* entity : data.mBlock->ent) {
        const auto* line = dynamic_cast<const DRW_Line*>(entity);
        if (!line || entity->eType != DRW::LINE) continue;
        ++count;
        for (int i = 0; i < 3; ++i)
            if (line->basePoint.x == i + 1.0 && line->basePoint.y == i + 2.0
                && line->basePoint.z == i + 3.0 && line->secPoint.x == i + 4.0
                && line->secPoint.y == i + 6.0 && line->secPoint.z == i + 9.0)
                seen[i] = true;
    }
    return count == 3u && seen[0] && seen[1] && seen[2];
}

bool legacyGenerated(const std::string& input, const std::filesystem::path& output,
                     DRW::Version sourceVersion) {
    LateMutationAdapter adapter;
    dx_data source;
    adapter.source = &source;
    if (!adapter.fileImport(input, &source, false) || !checkR14Generated(source, sourceVersion)) return false;
    const std::string sentinel = "AC1014 generated destination preserved\n";
    for (const bool binary : {false, true}) {
        if (!write(adapter, source, output.string(), binary)) return false;
        const DRW_Dictionary savedRoot = source.namedObjectDictionary;
        DRW_Dictionary detachedRoot;
        static_cast<DRW_TableEntry&>(detachedRoot) = static_cast<const DRW_TableEntry&>(savedRoot);
        detachedRoot.cloning = savedRoot.cloning;
        detachedRoot.hardOwner = savedRoot.hardOwner;
        detachedRoot.m_entries = savedRoot.m_entries;
        detachedRoot.countCap = savedRoot.countCap;
        source.namedObjectDictionary = detachedRoot;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        const bool detachedAccepted = write(adapter, source, output.string(), binary);
        source.namedObjectDictionary = savedRoot;
        if (detachedAccepted || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary)) return false;
        for (const int fault : {12, 13}) {
            for (const bool late : {false, true}) {
                { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
                if (late) adapter.lateFault = fault;
                else DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
                const bool accepted = write(adapter, source, output.string(), binary);
                if (late) adapter.lateFault = 0;
                else DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
                if (accepted || bytes(output) != sentinel
                    || !write(adapter, source, output.string(), binary)) return false;
            }
        }
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false) || !checkR14Generated(readback)) return false;
        const auto root = std::find_if(readback.namedObjectDictionary.m_entries.begin(),
            readback.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
                return entry.m_name == "ACAD_PLOTSETTINGS";
            });
        if (root == readback.namedObjectDictionary.m_entries.end()
            || std::count_if(readback.dictionaries.begin(), readback.dictionaries.end(),
                [&root](const auto& dictionary) {
                    return dictionary.handle == root->m_handle && dictionary.m_entries.empty();
                }) != 1) return false;
    }
    return true;
}

bool legacyDictionaryEedOutput(const std::string& input, const std::filesystem::path& output,
                               DRW::Version sourceVersion, const std::string& expectedText,
                               std::uint32_t appHandle = 0x12u, std::uint32_t groupHandle = 0xdu,
                               bool nonemptyGroup = false, bool nod = false, bool mlineOwner = false,
                               bool longInts = false, bool reals = false, bool groups = false, bool binaryEed = false, bool pointEed = false, bool handleEed = false) {
    LateMutationAdapter adapter;
    dx_data source;
    adapter.source = &source;
    if (!adapter.fileImport(input, &source, false) || !checkR14Generated(source, sourceVersion)
        || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(source, groupHandle)) return false;
    auto foundGroup = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [groupHandle](const DRW_Dictionary& dictionary) { return dictionary.handle == groupHandle; });
    DRW_Dictionary* group = nod ? &source.namedObjectDictionary
        : foundGroup == source.dictionaries.end() ? nullptr : &*foundGroup;
    auto app = std::find_if(source.appIds.begin(), source.appIds.end(),
        [appHandle](const DRW_AppId& item) { return item.handle == appHandle; });
    if (!group || app == source.appIds.end()
        || group->extData.size() != (groups || pointEed ? 7u : (longInts || reals) ? 6u : (binaryEed || handleEed) ? 4u : 3u) || !group->extData[0] || !group->extData[1]
        || !group->extData[2] || group->extData[0]->code() != 1001
        || group->extData[1]->code() != 1000 || group->extData[2]->code() != 1070
        || group->extData[0]->type() != DRW_Variant::STRING
        || group->extData[1]->type() != DRW_Variant::STRING
        || group->extData[2]->type() != DRW_Variant::INTEGER
        || !group->extData[0]->content.s || *group->extData[0]->content.s != "ACAD"
        || !group->extData[1]->content.s || *group->extData[1]->content.s != expectedText
        || group->extData[2]->i_val() != 42 || app->name != "ACAD") return false;
    const auto longValuesMatch = [](const DRW_Dictionary& dictionary) {
        const std::int32_t expected[] = {123456, (std::numeric_limits<std::int32_t>::min)(),
                                        (std::numeric_limits<std::int32_t>::max)()};
        if (dictionary.extData.size() != 6u) return false;
        for (std::size_t i = 0; i < 3u; ++i) {
            const auto* item = dictionary.extData[i + 3u];
            if (!item || item->code() != 1071 || item->type() != DRW_Variant::INTEGER
                || item->i_val() != expected[i]) return false;
        }
        return true;
    };
    const auto realValuesMatch = [](const DRW_Dictionary& dictionary) {
        const double expected[] = {1.25, -0.5, 42.125};
        if (dictionary.extData.size() != 6u) return false;
        for (std::size_t i = 0; i < 3u; ++i) {
            const auto* item = dictionary.extData[i + 3u];
            if (!item || item->code() != static_cast<int>(1040u + i)
                || item->type() != DRW_Variant::DOUBLE || item->d_val() != expected[i]) return false;
        }
        return true;
    };
    const auto groupValuesMatch = [](const DRW_Dictionary& dictionary) {
        const char* expected[] = {"{", "{", "}", "}"};
        if (dictionary.extData.size() != 7u) return false;
        for (std::size_t i = 0; i < 4u; ++i) {
            const auto* item = dictionary.extData[i + 3u];
            if (!item || item->code() != 1002 || item->type() != DRW_Variant::STRING
                || !item->content.s || *item->content.s != expected[i]) return false;
        }
        return true;
    };
    const auto handleMatches = [](const DRW_Dictionary& dictionary, const dx_data& data) {
        if (dictionary.extData.size() != 4u || !dictionary.extData[3]
            || dictionary.extData[3]->code() != 1005
            || dictionary.extData[3]->type() != DRW_Variant::STRING
            || !dictionary.extData[3]->content.s || !data.mBlock) return false;
        const auto& text = *dictionary.extData[3]->content.s;
        std::uint64_t target = 0u;
        const auto parsed = std::from_chars(text.data(), text.data() + text.size(), target, 16);
        return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size()
            && std::count_if(data.mBlock->ent.begin(), data.mBlock->ent.end(),
                [target](const DRW_Entity* entity) {
                    return entity && entity->handle == target && dynamic_cast<const DRW_Line*>(entity);
                }) == 1;
    };
    const auto pointValuesMatch = [](const DRW_Dictionary& dictionary) {
        if (dictionary.extData.size() != 7u) return false;
        for (std::size_t i = 0u; i < 4u; ++i) {
            const auto* item = dictionary.extData[i + 3u];
            const double code = static_cast<double>(10u + i);
            if (!item || item->code() != 1010 + static_cast<int>(i)
                || item->type() != DRW_Variant::COORD || !item->coord()
                || item->coord()->x != code + 0.25 || item->coord()->y != -code - 0.5
                || item->coord()->z != code + 0.75) return false;
        }
        return true;
    };
    const auto binarySourceMatches = [](const DRW_Dictionary& dictionary) {
        const auto* item = dictionary.extData[3];
        if (!item || item->code() != 1004 || item->type() != DRW_Variant::BINARY
            || !item->binary() || item->binary()->size() != 128u) return false;
        for (std::size_t i = 0u; i < 128u; ++i)
            if ((*item->binary())[i] != i) return false;
        return true;
    };
    const auto binaryOutputMatches = [](const DRW_Dictionary& dictionary) {
        if (dictionary.extData.size() != 5u) return false;
        std::string expected;
        const char digits[] = "0123456789ABCDEF";
        for (unsigned i = 0u; i < 128u; ++i) {
            expected.push_back(digits[i >> 4u]);
            expected.push_back(digits[i & 15u]);
        }
        const auto* first = dictionary.extData[3];
        const auto* second = dictionary.extData[4];
        return first && second && first->code() == 1004 && second->code() == 1004
            && first->type() == DRW_Variant::STRING && second->type() == DRW_Variant::STRING
            && first->content.s && second->content.s && first->content.s->size() == 254u
            && second->content.s->size() == 2u && *first->content.s + *second->content.s == expected;
    };
    if ((longInts && !longValuesMatch(*group)) || (reals && !realValuesMatch(*group))
        || (groups && !groupValuesMatch(*group)) || (binaryEed && !binarySourceMatches(*group))
        || (pointEed && !pointValuesMatch(*group)) || (handleEed && !handleMatches(*group, source))) return false;
    auto groupRecord = std::find_if(source.groups.begin(), source.groups.end(),
        [](const DRW_Group& item) { return item.handle == 0x31u; });
    if (nonemptyGroup && (group->m_entries.size() != 1u
        || group->m_entries[0].m_name != "MYGROUP" || group->m_entries[0].m_handle != 0x31u
        || groupRecord == source.groups.end() || groupRecord->parentHandle != groupHandle
        || groupRecord->m_entityHandles != std::vector<std::uint32_t>{0x2eu})) return false;
    const DRW_Dictionary saved = *group;
    DRW_Group savedGroup;
    if (nonemptyGroup) savedGroup = *groupRecord;
    const DRW_Dictionary savedRoot = source.namedObjectDictionary;
    const DRW_MLineStyle savedStyle = source.mLineStyles.front();
    const int sourceFault = nod ? (groupHandle == 0xcu ? 7 : 16)
                                : mlineOwner ? 17 : (groupHandle == 0xdu ? 14 : 15);
    const std::string sentinel = "Legacy EED destination preserved\n";
    const auto policy = sourceVersion == DRW::AC1012 ? dx_dwgReadCompletenessPolicy::AllowPartial
                                                    : dx_dwgReadCompletenessPolicy::RequireComplete;
    const auto emit = [&](bool binary, DRW::Version target = DRW::AC1014) {
        return write(adapter, source, output.string(), binary, target, policy);
    };
    for (const bool binary : {false, true}) {
        if (sourceVersion == DRW::AC1012) {
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            if (write(adapter, source, output.string(), binary) || bytes(output) != sentinel) return false;
        }
        if (!emit(binary)) return false;
        if (sourceVersion == DRW::AC1014) {
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            if (emit(binary, DRW::AC1015)
                || bytes(output) != sentinel || !emit(binary)) return false;
        }
        for (int fault = 0; fault < ((longInts || reals || groups || binaryEed || pointEed || handleEed) ? 11 : (nod || mlineOwner) ? 9 : nonemptyGroup ? 10 : 8); ++fault) {
            // R13 explicit partial output does not certify its MLINESTYLE;
            // edits to that unqualified object remain outside this proof.
            if ((nod || mlineOwner) && sourceVersion == DRW::AC1012 && fault == 8) continue;
            if (!nod && !mlineOwner && !nonemptyGroup && (fault == 8 || fault == 9)) continue;
            if ((nod || mlineOwner) && fault == 9) continue;
            if (fault == 0) group->extData[1]->addString(1000, "CHANGED");
            if (fault == 1) group->extData[2]->addInt(1070, 43);
            if (fault == 2) app->name = "CHANGED";
            if (fault == 3) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, sourceFault) ^= 1u;
            if (fault == 4) adapter.lateFault = sourceFault;
            if (fault == 5) group->extData[0]->addString(1001, "OTHER");
            if (fault == 6) {
                auto edge = std::find_if(source.namedObjectDictionary.m_entries.begin(),
                    source.namedObjectDictionary.m_entries.end(), [mlineOwner](const DRW_Dictionary::Entry& item) {
                        return item.m_name == (mlineOwner ? "ACAD_MLINESTYLE" : "ACAD_GROUP");
                    });
                if (edge == source.namedObjectDictionary.m_entries.end()) return false;
                edge->m_handle ^= 1u;
            }
            if (fault == 7) group->parentHandle ^= 1u;
            if (fault == 8 && (nod || mlineOwner)) source.mLineStyles.front().name += "X";
            else if (fault == 8) group->m_entries[0].m_handle ^= 1u;
            if (fault == 9) groupRecord->m_entityHandles[0] ^= 1u;
            if (fault == 10 && longInts) group->extData[4]->addInt(1071, 0);
            if (fault == 10 && reals) group->extData[4]->addDouble(1041, 0.5);
            if (fault == 10 && groups) group->extData[4]->addString(1002, "}");
            if (fault == 10 && binaryEed) group->extData[3]->addBinary(1004, {1u, 2u});
            if (fault == 10 && pointEed) group->extData[4]->addCoord(1011, {1.0, 2.0, 3.0});
            if (fault == 10 && handleEed) group->extData[3]->addString(1005, "FA");
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            const bool accepted = emit(binary);
            if (fault == 3) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, sourceFault) ^= 1u;
            if (fault == 4) adapter.lateFault = 0;
            app->name = "ACAD";
            *group = saved;
            if (nonemptyGroup) *groupRecord = savedGroup;
            source.namedObjectDictionary = savedRoot;
            if (nod || mlineOwner) source.mLineStyles.front() = savedStyle;
            if (accepted || bytes(output) != sentinel
                || !emit(binary)) {
                std::cerr << "Legacy EED refusal/retry failed: binary=" << binary
                          << " fault=" << fault << '\n';
                return false;
            }
        }
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false) || !checkR14Generated(readback)) return false;
        std::uint32_t outputHandle = 0xdu;
        if (mlineOwner) {
            const auto edge = std::find_if(readback.namedObjectDictionary.m_entries.begin(),
                readback.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& item) {
                    return item.m_name == "ACAD_MLINESTYLE";
                });
            if (edge == readback.namedObjectDictionary.m_entries.end()) return false;
            outputHandle = edge->m_handle;
        }
        const auto foundResult = std::find_if(readback.dictionaries.begin(), readback.dictionaries.end(),
            [outputHandle](const DRW_Dictionary& dictionary) { return dictionary.handle == outputHandle; });
        const DRW_Dictionary* result = nod ? &readback.namedObjectDictionary
            : foundResult == readback.dictionaries.end() ? nullptr : &*foundResult;
        if (!result || result->extData.size() != (groups || pointEed ? 7u : (longInts || reals) ? 6u : binaryEed ? 5u : handleEed ? 4u : 3u)
            || !result->extData[0] || !result->extData[1] || !result->extData[2]
            || result->extData[0]->type() != DRW_Variant::STRING
            || result->extData[1]->type() != DRW_Variant::STRING
            || result->extData[2]->type() != DRW_Variant::INTEGER
            || !result->extData[0]->content.s || *result->extData[0]->content.s != "ACAD"
            || !result->extData[1]->content.s || *result->extData[1]->content.s != expectedText
            || result->extData[2]->i_val() != 42 || (longInts && !longValuesMatch(*result))
            || (reals && !realValuesMatch(*result)) || (groups && !groupValuesMatch(*result))
            || (binaryEed && !binaryOutputMatches(*result)) || (pointEed && !pointValuesMatch(*result))
            || (handleEed && !handleMatches(*result, readback))) return false;
        if (nonemptyGroup) {
            const auto member = std::find_if(readback.groups.begin(), readback.groups.end(),
                [](const DRW_Group& item) { return item.name == "MYGROUP"; });
            if (result->m_entries.size() != 1u || result->m_entries[0].m_name != "MYGROUP"
                || member == readback.groups.end() || result->m_entries[0].m_handle != member->handle
                || member->parentHandle != result->handle || member->m_entityHandles.size() != 1u
                || !readback.mBlock || std::none_of(readback.mBlock->ent.begin(), readback.mBlock->ent.end(),
                    [&member](const DRW_Entity* entity) {
                        return entity && entity->handle == member->m_entityHandles[0]
                            && dynamic_cast<const DRW_Line*>(entity) != nullptr;
                    })) return false;
        }
    }
    return true;
}

bool legacyDanglingEedHandle(const std::string& badInput, const std::string& goodInput,
                             const std::filesystem::path& output, DRW::Version version) {
    dx_iface adapter;
    const auto policy = version == DRW::AC1012 ? dx_dwgReadCompletenessPolicy::AllowPartial
                                               : dx_dwgReadCompletenessPolicy::RequireComplete;
    const std::string sentinel = "dangling EED handle destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        if (!adapter.fileImport(badInput, &bad, false) || bad.sourceVersion != version) return false;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary, DRW::AC1014, policy) || bytes(output) != sentinel) {
            std::cerr << "Dangling EED target was not refused atomically\n";
            return false;
        }
        dx_data corrected;
        if (!adapter.fileImport(goodInput, &corrected, false)
            || !write(adapter, corrected, output.string(), binary, DRW::AC1014, policy)) {
            std::cerr << "Corrected EED target retry failed\n";
            return false;
        }
    }
    return true;
}

bool legacyLayerEedReadPolicy(const std::string& input, bool sourceBig, bool missing = false,
                              bool oda = false, bool wide = false, bool zero = false) {
    const auto little = DRW::DwgEedLayerByteOrder::LegacyLittleEndian;
    const auto big = DRW::DwgEedLayerByteOrder::BigEndian;
    dwgRW api(input.c_str());
    dwgR legacyApi(input.c_str());
    const auto invalid = static_cast<DRW::DwgEedLayerByteOrder>(99);
    if (api.dwgEedLayerByteOrder() != little || legacyApi.dwgEedLayerByteOrder() != little
        || api.setDwgEedLayerByteOrder(invalid) || legacyApi.setDwgEedLayerByteOrder(invalid)
        || !api.setDwgEedLayerByteOrder(big) || !legacyApi.setDwgEedLayerByteOrder(big)
        || api.dwgEedLayerByteOrder() != big || legacyApi.dwgEedLayerByteOrder() != big) return false;
    DRW_Variant equalName(1003, std::string("10"), true);
    equalName.setDwgRawLayerReference(16u, DRW::AC1012);
    if (!equalName.activeDwgLayerReference()) return false;
    equalName.setLayerRefName("10");
    if (equalName.activeDwgLayerReference() || equalName.rawDwgLayerReference() != 16u) return false;
    dx_iface adapter;
    for (const auto order : {little, big, little}) {
        if (!adapter.setDwgEedLayerByteOrder(order) || adapter.setDwgEedLayerByteOrder(invalid)) return false;
        dx_data data;
        if (!adapter.fileImport(input, &data, false) || !data.hasNamedObjectDictionary
            || data.dwgReadOutcome.standardObjectPayloadValidationFailed) return false;
        const auto entry = std::find_if(data.namedObjectDictionary.m_entries.begin(),
            data.namedObjectDictionary.m_entries.end(), [oda](const DRW_Dictionary::Entry& item) {
                return item.m_name == (oda ? "TEST_DICT" : "ACAD_GROUP");
            });
        if (entry == data.namedObjectDictionary.m_entries.end()) return false;
        const auto dictionary = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
            [&entry](const DRW_Dictionary& item) { return item.handle == entry->m_handle; });
        if (dictionary == data.dictionaries.end()) return false;
        const auto found = std::find_if(dictionary->extData.begin(), dictionary->extData.end(),
            [](const DRW_Variant* item) { return item && item->code() == 1003; });
        if (found == dictionary->extData.end() || !(*found)->content.s || !(*found)->isLayerRef()) return false;
        const std::uint64_t raw = zero ? 0u : wide ? UINT64_C(0x0807060504030201)
            : sourceBig ? (missing ? UINT64_C(0xfa00000000000000)
            : oda ? UINT64_C(0x3000000000000000) : UINT64_C(0x1000000000000000)) : 16u;
        const bool resolved = !missing && !wide && !zero && ((order == big) == sourceBig);
        const char* layerName = oda ? "PROBE_LAYER" : "0";
        if ((*found)->rawDwgLayerReference() != raw
            || (resolved ? (!(*found)->stringEquals(layerName) || (*found)->activeDwgLayerReference())
                         : !(*found)->activeDwgLayerReference())) return false;
        if (oda) {
            if (!data.mBlock) return false;
            const auto line = std::find_if(data.mBlock->ent.begin(), data.mBlock->ent.end(),
                [](const DRW_Entity* item) { return dynamic_cast<const DRW_Line*>(item) != nullptr; });
            if (line == data.mBlock->ent.end()) return false;
            const auto eed = std::find_if((*line)->extData.begin(), (*line)->extData.end(),
                [](const std::shared_ptr<DRW_Variant>& item) { return item && item->code() == 1003; });
            if (eed == (*line)->extData.end() || (*eed)->rawDwgLayerReference() != raw
                || ((*eed)->stringEquals(layerName) != resolved)) return false;
        }
    }
    return true;
}

bool legacyXRecordLengthControls(const std::string& goodInput,
                                 const std::string& badInput) {
    dx_iface adapter;
    dx_data bad;
    if (!adapter.fileImport(badInput, &bad, false)
        || bad.dwgReadOutcome.observations.objectParseFailures == 0u) {
        std::cerr << "oversized XRECORD string did not record an object failure\n";
        return false;
    }
    dx_data source;
    if (!adapter.fileImport(goodInput, &source, false)
        || (source.sourceVersion != DRW::AC1012 && source.sourceVersion != DRW::AC1014)
        || source.dwgReadOutcome.observations.objectParseFailures != 0u
        || !source.hasNamedObjectDictionary) return false;
    const auto parentEdge = std::find_if(source.namedObjectDictionary.m_entries.begin(),
        source.namedObjectDictionary.m_entries.end(), [](const auto& entry) {
            return entry.m_name == "LEN_PROBE";
        });
    if (parentEdge == source.namedObjectDictionary.m_entries.end()) return false;
    const auto parent = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [&parentEdge](const DRW_Dictionary& dictionary) { return dictionary.handle == parentEdge->m_handle; });
    if (parent == source.dictionaries.end()) return false;
    for (const int length : {1, 254, 255, 256, 300}) {
        const auto name = "LEN_" + std::to_string(length);
        const auto edge = std::find_if(parent->m_entries.begin(), parent->m_entries.end(),
            [&name](const auto& entry) { return entry.m_name == name; });
        if (edge == parent->m_entries.end()) return false;
        const auto record = std::find_if(source.xRecords.begin(), source.xRecords.end(),
            [&edge](const DRW_XRecord& item) { return item.handle == edge->m_handle; });
        if (record == source.xRecords.end() || record->m_dataEntries.size() != 2u
            || record->m_dataEntries[0].code() != 1
            || record->m_dataEntries[0].type() != DRW_Variant::STRING
            || !record->m_dataEntries[0].content.s
            || *record->m_dataEntries[0].content.s != std::string(static_cast<std::size_t>(length), 'A')
            || record->m_dataEntries[1].code() != 90
            || record->m_dataEntries[1].type() != DRW_Variant::INTEGER
            || record->m_dataEntries[1].i_val() != length || record->m_rawDataValid) return false;
    }
    return true;
}

bool checkLayerDxf(const std::string& input, std::uint32_t handle = 0xdu) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false)) return false;
    const auto entry = std::find_if(data.namedObjectDictionary.m_entries.begin(), data.namedObjectDictionary.m_entries.end(),
        [handle](const auto& item) { return handle == 0x17u && item.m_name == "ACAD_MLINESTYLE"; });
    if (handle == 0x17u && entry == data.namedObjectDictionary.m_entries.end()) return false;
    const auto emittedHandle = handle == 0x17u ? entry->m_handle : handle;
    const auto found = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
        [emittedHandle](const DRW_Dictionary& item) { return item.handle == emittedHandle; });
    const DRW_Dictionary* group = handle == 0xcu ? &data.namedObjectDictionary
        : found == data.dictionaries.end() ? nullptr : &*found;
    if (!group) return false;
    const auto layer = std::find_if(group->extData.begin(), group->extData.end(),
        [](const DRW_Variant* item) { return item && item->code() == 1003; });
    return layer != group->extData.end() && (*layer)->stringEquals("0");
}

bool legacyBigLayerPartial(const std::string& goodInput, const std::string& badInput,
                           const std::filesystem::path& output, std::uint32_t handle = 0xdu) {
    dx_iface adapter;
    if (!adapter.setDwgEedLayerByteOrder(DRW::DwgEedLayerByteOrder::BigEndian)) return false;
    const std::string sentinel = "unresolved EED layer destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        if (!adapter.fileImport(badInput, &bad, false)
            || DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(bad, handle)) return false;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary, DRW::AC1014,
                  dx_dwgReadCompletenessPolicy::AllowPartial) || bytes(output) != sentinel) {
            std::cerr << "Unresolved EED layer did not preserve the destination\n";
            return false;
        }
        dx_data source;
        if (!adapter.fileImport(goodInput, &source, false)) {
            std::cerr << "Resolved EED layer import failed\n";
            return false;
        }
        if (!DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(source, handle)) {
            std::cerr << "Resolved EED layer source was not bound\n";
            return false;
        }
        const auto layer = std::find_if(source.layers.begin(), source.layers.end(),
            [](const DRW_Layer& item) { return item.handle == 16u; });
        if (layer == source.layers.end()) return false;
        const auto policy = dx_dwgReadCompletenessPolicy::AllowPartial;
        const auto rejectMutation = [&](bool wire) {
            auto& raw = DxIfaceStandardDwgCaptureTestAccess::legacyGroupEedBytes(source, handle);
            const auto bit = DxIfaceStandardDwgCaptureTestAccess::legacyLayerBit(source, handle);
            if (bit == (std::numeric_limits<std::size_t>::max)() || bit / 8u >= raw.size()) return false;
            const auto mask = static_cast<std::uint8_t>(0x80u >> (bit % 8u));
            const auto originalName = layer->name;
            if (wire) raw[bit / 8u] ^= mask;
            else layer->name = "MUTATED_LAYER";
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            const bool accepted = write(adapter, source, output.string(), binary, DRW::AC1014, policy);
            if (wire) raw[bit / 8u] ^= mask;
            else layer->name = originalName;
            return !accepted && bytes(output) == sentinel;
        };
        if (!rejectMutation(false) || !rejectMutation(true)) return false;
        const auto found = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
            [handle](const DRW_Dictionary& item) { return item.handle == handle; });
        DRW_Dictionary* group = handle == 0xcu ? &source.namedObjectDictionary
            : found == source.dictionaries.end() ? nullptr : &*found;
        if (!group || group->extData.empty()
            || !group->extData.back() || group->extData.back()->code() != 1003) return false;
        DRW_Variant saved = *group->extData.back();
        group->extData.back()->setLayerRefName("WRONG_LAYER");
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        const bool acceptedName = write(adapter, source, output.string(), binary, DRW::AC1014, policy);
        *group->extData.back() = saved;
        if (acceptedName || bytes(output) != sentinel) return false;
        source.layers.push_back(*layer); // Duplicate source identity must not be guessed.
        const bool acceptedDuplicate = write(adapter, source, output.string(), binary, DRW::AC1014, policy);
        source.layers.pop_back();
        const bool preserved = bytes(output) == sentinel;
        const bool retried = write(adapter, source, output.string(), binary, DRW::AC1014, policy);
        if (acceptedDuplicate || !preserved || !retried) {
            std::cerr << "EED layer duplicate=" << acceptedDuplicate << " preserved=" << preserved
                      << " retried=" << retried << " binary=" << binary << '\n';
            return false;
        }
        if (!checkLayerDxf(output.string(), handle)) return false;
    }
    return true;
}

bool legacyBadGroupMember(const std::string& badInput, const std::string& goodInput,
                          const std::filesystem::path& output, DRW::Version version) {
    dx_iface adapter;
    const auto policy = version == DRW::AC1012 ? dx_dwgReadCompletenessPolicy::AllowPartial
                                               : dx_dwgReadCompletenessPolicy::RequireComplete;
    const std::string sentinel = "wrong GROUP member role destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        const bool imported = adapter.fileImport(badInput, &bad, false);
        if (!imported || bad.sourceVersion != version || bad.dwgReadOutcome.observations.objectParseFailures == 0u) {
            std::cerr << "Wrong-role GROUP import did not record a failed object\n";
            return false;
        }
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary) || bytes(output) != sentinel) {
            std::cerr << "Wrong-role GROUP did not preserve the destination\n";
            return false;
        }
        dx_data corrected;
        if (!adapter.fileImport(goodInput, &corrected, false)
            || corrected.dwgReadOutcome.observations.objectParseFailures != 0u
            || !write(adapter, corrected, output.string(), binary, DRW::AC1014, policy)) {
            std::cerr << "Corrected GROUP source retry failed\n";
            return false;
        }
    }
    return true;
}

bool r13OwnershipMarkerRefusal(const std::string& badInput, const std::string& goodInput,
                               const std::filesystem::path& output) {
    dx_iface adapter;
    const std::string sentinel = "R13 ownership destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        if (!adapter.fileImport(badInput, &bad, false)
            || bad.sourceVersion != DRW::AC1012
            || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(bad, 0xdu)) return false;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary, DRW::AC1014,
                  dx_dwgReadCompletenessPolicy::AllowPartial) || bytes(output) != sentinel)
            return false;
        dx_data corrected;
        if (!adapter.fileImport(goodInput, &corrected, false)
            || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(corrected, 0xdu)
            || !write(adapter, corrected, output.string(), binary, DRW::AC1014,
                      dx_dwgReadCompletenessPolicy::AllowPartial)) return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)) return false;
        const auto group = std::find_if(readback.dictionaries.begin(), readback.dictionaries.end(),
            [](const DRW_Dictionary& value) { return value.handle == 0xdu; });
        if (group == readback.dictionaries.end() || group->hardOwner != 0
            || group->extData.size() != 3u || !group->extData[1] || !group->extData[2]
            || group->extData[1]->type() != DRW_Variant::STRING
            || !group->extData[1]->content.s || *group->extData[1]->content.s != "TREAT_AS_HARD"
            || group->extData[2]->code() != 1070
            || group->extData[2]->type() != DRW_Variant::INTEGER
            || group->extData[2]->i_val() != 0)
            return false;
    }
    return true;
}

bool legacyBadEedGraph(const std::string& badInput, const std::string& goodInput,
                       const std::filesystem::path& output, DRW::Version version,
                       std::uint32_t sourceHandle = 0x34u) {
    dx_iface adapter;
    const auto policy = version == DRW::AC1012 ? dx_dwgReadCompletenessPolicy::AllowPartial
                                               : dx_dwgReadCompletenessPolicy::RequireComplete;
    const std::string sentinel = "bad legacy EED graph destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        const bool imported = adapter.fileImport(badInput, &bad, false);
        const bool bound = DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(bad, sourceHandle);
        if (!imported || bad.sourceVersion != version || bound) {
            std::cerr << "Bad-graph import=" << imported << " source proof=" << bound << '\n';
            return false;
        }
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary, DRW::AC1014, policy)
            || bytes(output) != sentinel) { std::cerr << "Bad-graph refusal failed\n"; return false; }
        dx_data corrected;
        if (!adapter.fileImport(goodInput, &corrected, false)
            || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(corrected, sourceHandle)
            || !write(adapter, corrected, output.string(), binary, DRW::AC1014, policy)) {
            std::cerr << "Bad-graph corrected retry failed\n"; return false;
        }
    }
    return true;
}

bool legacyWideEedRefusal(const std::string& badInput, const std::string& goodInput,
                          const std::filesystem::path& output, DRW::Version sourceVersion,
                          std::uint32_t groupHandle = 0xdu, bool nod = false, bool mlineOwner = false) {
    dx_iface adapter;
    const auto policy = sourceVersion == DRW::AC1012 ? dx_dwgReadCompletenessPolicy::AllowPartial
                                                    : dx_dwgReadCompletenessPolicy::RequireComplete;
    const std::string sentinel = "unrepresentable EED destination preserved\n";
    for (bool binary : {false, true}) {
        dx_data bad;
        if (!adapter.fileImport(badInput, &bad, false)
            || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(bad, groupHandle)) return false;
        const auto foundBadGroup = std::find_if(bad.dictionaries.begin(), bad.dictionaries.end(),
            [groupHandle](const DRW_Dictionary& dictionary) { return dictionary.handle == groupHandle; });
        const DRW_Dictionary* badGroup = nod ? &bad.namedObjectDictionary
            : foundBadGroup == bad.dictionaries.end() ? nullptr : &*foundBadGroup;
        if (!badGroup || badGroup->extData.size() != 3u
            || !badGroup->extData[1] || badGroup->extData[1]->type() != DRW_Variant::STRING
            || !badGroup->extData[1]->content.s
            || *badGroup->extData[1]->content.s != "\xE6\xBC\xA2\xC3\xA9") return false;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, bad, output.string(), binary, DRW::AC1014, policy)
            || bytes(output) != sentinel) return false;
        dx_data corrected;
        if (!adapter.fileImport(goodInput, &corrected, false)
            || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(corrected, groupHandle)
            || !write(adapter, corrected, output.string(), binary, DRW::AC1014, policy)) return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)) return false;
        std::uint32_t outputHandle = 0xdu;
        if (mlineOwner) {
            const auto edge = std::find_if(readback.namedObjectDictionary.m_entries.begin(),
                readback.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& item) {
                    return item.m_name == "ACAD_MLINESTYLE";
                });
            if (edge == readback.namedObjectDictionary.m_entries.end()) return false;
            outputHandle = edge->m_handle;
        }
        const auto foundGroup = std::find_if(readback.dictionaries.begin(), readback.dictionaries.end(),
            [outputHandle](const DRW_Dictionary& dictionary) { return dictionary.handle == outputHandle; });
        const DRW_Dictionary* group = nod ? &readback.namedObjectDictionary
            : foundGroup == readback.dictionaries.end() ? nullptr : &*foundGroup;
        if (!group || group->extData.size() != 3u
            || !group->extData[1] || group->extData[1]->type() != DRW_Variant::STRING
            || !group->extData[1]->content.s || *group->extData[1]->content.s != "caf\xC3\xA9") return false;
    }
    return true;
}

// A source outside the new proof retains the established explicit-partial
// route. Its successful projection is not a complete-source certificate.
bool legacyLayerEedPartial(const std::string& input, const std::filesystem::path& output) {
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(input, &source, false)
        || (source.sourceVersion != DRW::AC1012 && source.sourceVersion != DRW::AC1014)
        || !checkR14Generated(source, source.sourceVersion)
        || !DxIfaceStandardDwgCaptureTestAccess::importBoundGroupEed(source)) return false;
    for (bool binary : {false, true}) {
        if (!write(adapter, source, output.string(), binary, DRW::AC1014,
                   dx_dwgReadCompletenessPolicy::AllowPartial)) return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)) return false;
        const auto group = std::find_if(readback.dictionaries.begin(), readback.dictionaries.end(),
            [](const DRW_Dictionary& dictionary) { return dictionary.handle == 0xdu; });
        if (group == readback.dictionaries.end() || group->extData.size() != 4u
            || !group->extData[3] || group->extData[3]->code() != 1003
            || group->extData[3]->type() != DRW_Variant::STRING
            || !group->extData[3]->content.s || *group->extData[3]->content.s != "0") return false;
    }
    return true;
}

bool checkR14GeneratedFile(const std::string& input) {
    dx_iface reader;
    dx_data data;
    return reader.fileImport(input, &data, false) && checkR14Generated(data);
}

bool checkLegacyLayoutWire(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false)) return false;
    const auto source = bytes(input);
    if (!DxIfaceStandardDwgCaptureTestAccess::legacyLayoutWire(data, source)) return false;
    const auto row = std::find_if(data.dwgClassCoverage.m_entries.begin(),
        data.dwgClassCoverage.m_entries.end(),
        [](const DRW_DwgClassCoverageEntry& item) { return item.m_recordName == "LAYOUT"; });
    if (row == data.dwgClassCoverage.m_entries.end()) return false;
    const auto original = row->m_className;
    row->m_className = "ChangedLayout";
    const bool refused = !DxIfaceStandardDwgCaptureTestAccess::legacyLayoutWire(data, source);
    row->m_className = original;
    return refused && DxIfaceStandardDwgCaptureTestAccess::legacyLayoutWire(data, source);
}

// A locally generated AC1012 pair has no LAYOUT records. Validate its two
// source-bound BLOCK_HEADER frames without assuming a layout graph exists.
bool checkLegacyBlockPairWire(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false) || data.sourceVersion != DRW::AC1012
        || data.blockRecords.size() != 2u) return false;
    const auto source = bytes(input);
    if (!data.hasCompleteDwgBlockRecordControlMembership()
        || !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source)
        || !DxIfaceStandardDwgCaptureTestAccess::legacyBlockWire(data)) return false;
    data.blockRecords.front().parentHandle ^= 1u;
    const bool rejectedOwner = !data.hasCompleteDwgBlockRecordControlMembership();
    data.blockRecords.front().parentHandle ^= 1u;
    if (!rejectedOwner || !data.hasCompleteDwgBlockRecordControlMembership()) return false;
    std::uint32_t layerControl = 0, layerExtension = 0;
    if (data.layers.size() != 1u || data.layers.front().handle != 0x10u
        || !data.getLayerTableControlExtensionDictionary(layerControl, layerExtension)
        || layerControl != 2u || layerExtension != 0u) return false;
    data.layers.front().parentHandle ^= 1u;
    const bool rejectedLayerOwner = !data.getLayerTableControlExtensionDictionary(layerControl, layerExtension);
    data.layers.front().parentHandle ^= 1u;
    if (!rejectedLayerOwner || !data.getLayerTableControlExtensionDictionary(layerControl, layerExtension)
        || layerControl != 2u || layerExtension != 0u) return false;
    if (data.lineTypes.size() != 3u || !data.hasCompleteDwgLineTypeControlMembership()) return false;
    data.lineTypes.front().parentHandle ^= 1u;
    const bool rejectedLtypeOwner = !data.hasCompleteDwgLineTypeControlMembership();
    data.lineTypes.front().parentHandle ^= 1u;
    if (!rejectedLtypeOwner || !data.hasCompleteDwgLineTypeControlMembership()) return false;
    if (data.textStyles.size() != 1u || data.textStyles.front().handle != 0x11u
        || data.textStyles.front().parentHandle != 3u || data.textStyles.front().name != "Standard"
        || data.textStyles.front().font != "txt" || data.textStyles.front().height != 0.0
        || data.textStyles.front().width != 1.0 || data.textStyles.front().lastHeight != 0.2
        || data.appIds.size() != 1u || data.appIds.front().handle != 0x12u
        || data.appIds.front().parentHandle != 9u || data.appIds.front().name != "ACAD") return false;
    if (data.VPorts.size() != 1u || data.VPorts.front().handle != 0x26u
        || data.VPorts.front().parentHandle != 8u || data.VPorts.front().name != "*Active"
        || data.dimStyles.size() != 1u || data.dimStyles.front().handle != 0x2du
        || data.dimStyles.front().parentHandle != 10u || data.dimStyles.front().name != "Standard"
        || !data.views.empty() || !data.ucsList.empty()) return false;
    const auto& dimension = data.dimStyles.front();
    if (dimension.dimscale != 1.0 || dimension.dimasz != 0.18 || dimension.dimtxt != 0.18
        || dimension.dimtxstyH.ref != 0x11u) return false;
    const auto& viewport = data.VPorts.front();
    if (viewport.height != 13314.951254 || std::abs(viewport.ratio - 1.42723004699631) > 1e-12
        || viewport.center.x != 5771.99757 || viewport.center.y != 789.325613
        || viewport.circleZoom != 1000 || viewport.viewMode != 0) return false;
    const auto model = std::find_if(data.blockRecords.begin(), data.blockRecords.end(),
        [](const DRW_Block_Record& record) { return record.name == "*MODEL_SPACE"; });
    const auto paper = std::find_if(data.blockRecords.begin(), data.blockRecords.end(),
        [](const DRW_Block_Record& record) { return record.name == "*PAPER_SPACE"; });
    if (model == data.blockRecords.end() || paper == data.blockRecords.end()
        || DRW_DwgBlockRecordWireAccess::block(*model) != 0x24u
        || DRW_DwgBlockRecordWireAccess::end(*model) != 0x25u
        || DRW_DwgBlockRecordWireAccess::block(*paper) != 0x21u
        || DRW_DwgBlockRecordWireAccess::end(*paper) != 0x22u
        || DRW_DwgBlockRecordWireAccess::first(*model) != 0x2cu
        || DRW_DwgBlockRecordWireAccess::last(*model) != 0x2cu
        || DRW_DwgBlockRecordWireAccess::first(*paper) != 0u
        || DRW_DwgBlockRecordWireAccess::last(*paper) != 0u || !data.mBlock
        || std::count_if(data.mBlock->ent.begin(), data.mBlock->ent.end(),
            [](const DRW_Entity* entity) { return entity && entity->handle == 0x2cu
                && dynamic_cast<const DRW_Line*>(entity) != nullptr; }) != 1) return false;
    auto& captured = DxIfaceStandardDwgCaptureTestAccess::blockBytes(data, model->handle);
    if (captured.empty()) return false;
    captured.front() ^= 0x80u;
    const bool rejectedBytes = !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source);
    captured.front() ^= 0x80u;
    auto& generation = DxIfaceStandardDwgCaptureTestAccess::blockGeneration(data, model->handle);
    generation ^= 1u;
    const bool rejectedGeneration = !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source);
    generation ^= 1u;
    return rejectedBytes && rejectedGeneration
        && DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source)
        && DxIfaceStandardDwgCaptureTestAccess::legacyBlockWire(data);
}

bool checkLegacyLayoutGraph(const std::string& input) {
    dx_iface adapter;
    dx_data data;
    if (!adapter.fileImport(input, &data, false)
        || !DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(adapter)
        || adapter.wantsLegacyBlockRecordSource(DRW::AC1015)) return false;
    const auto source = bytes(input);
    if (!DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source)
        || !DxIfaceStandardDwgCaptureTestAccess::legacyBlockWire(data)) return false;
    auto& capturedBlock = DxIfaceStandardDwgCaptureTestAccess::blockBytes(data, data.blockRecords.front().handle);
    if (capturedBlock.empty()) return false;
    capturedBlock.front() ^= 0x80u;
    const bool rejectedBlock = !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source);
    capturedBlock.front() ^= 0x80u;
    if (!rejectedBlock || !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source)) return false;
    auto& capturedGeneration = DxIfaceStandardDwgCaptureTestAccess::blockGeneration(data, data.blockRecords.front().handle);
    capturedGeneration ^= 1u;
    const bool rejectedGeneration = !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source);
    capturedGeneration ^= 1u;
    if (!rejectedGeneration || !DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(data, source)) return false;
    const auto rootEntry = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(),
        [](const DRW_Dictionary::Entry& item) { return item.m_name == "ACAD_LAYOUT"; });
    if (rootEntry == data.namedObjectDictionary.m_entries.end() || data.layouts.empty()) return false;
    const auto dictionaryHandle = rootEntry->m_handle;
    auto dictionary = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
        [dictionaryHandle](const DRW_Dictionary& value) { return value.handle == dictionaryHandle; });
    if (dictionary == data.dictionaries.end() || dictionary->m_entries.empty()) return false;
    auto record = std::find_if(data.blockRecords.begin(), data.blockRecords.end(),
        [&data](const DRW_Block_Record& value) {
            return value.handle == data.layouts.front().paperSpaceBlockRecordHandle.ref;
        });
    if (record == data.blockRecords.end()) return false;
    const auto classRow = std::find_if(data.dwgClassCoverage.m_entries.begin(),
        data.dwgClassCoverage.m_entries.end(),
        [](const DRW_DwgClassCoverageEntry& value) { return value.m_recordName == "LAYOUT"; });
    if (classRow == data.dwgClassCoverage.m_entries.end()) return false;
    std::error_code error;
    const auto scratch = libdxfrw_test::reserveTestScratch(
        std::filesystem::temp_directory_path(), "libdxfrw-legacy-layout-graph", error);
    if (error || scratch.empty()) return false;
    const auto output = scratch / "out.dxf";
    const std::string sentinel = "legacy layout destination preserved\n";
    const auto refuses = [&]() {
        for (const bool binary : {false, true}) {
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            if (write(adapter, data, output.string(), binary, DRW::AC1015,
                      dx_dwgReadCompletenessPolicy::AllowPartial) || bytes(output) != sentinel)
                return false;
        }
        return true;
    };
    const auto check = [&](const auto& mutate) {
        mutate();
        const bool rejected = !DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(adapter)
            && refuses();
        mutate();
        return rejected && DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(adapter)
            && refuses();
    };
    auto& rawLayout = DxIfaceStandardDwgCaptureTestAccess::layoutBytes(data, data.layouts.front().handle);
    auto& rawDictionary = DxIfaceStandardDwgCaptureTestAccess::dictionaryBytes(data, dictionaryHandle);
    dx_legacy_frame_census::Capture missingCensus;
    auto emptyCensus = DxIfaceStandardDwgCaptureTestAccess::emptyLayoutCensus(data);
    if (rawLayout.empty() || rawDictionary.empty()) return false;
    double changedBase = record->basePoint.x + 1.0;
    std::string changedName = record->name + "_changed";
    const bool ok = check([&]() { capturedBlock.front() ^= 0x80u; })
        && check([&]() { std::swap(DxIfaceStandardDwgCaptureTestAccess::legacyCensus(data), missingCensus); })
        && check([&]() { std::swap(DxIfaceStandardDwgCaptureTestAccess::legacyCensus(data), emptyCensus); })
        && check([&]() { capturedGeneration ^= 1u; })
        && check([&]() { std::swap(record->basePoint.x, changedBase); })
        && check([&]() { std::swap(record->name, changedName); })
        && check([&]() { record->xrefBlockHandle.rawCode ^= 1u; })
        && check([&]() { rawLayout.front() ^= 0x80u; })
        && check([&]() { rawDictionary.front() ^= 0x80u; })
        && check([&]() { classRow->m_classNumber ^= 1u; })
        && check([&]() { rootEntry->m_handle ^= 1u; })
        && check([&]() { dictionary->parentHandle ^= 1u; })
        && check([&]() { dictionary->m_entries.front().m_handle ^= 1u; })
        && check([&]() { data.layouts.front().paperSpaceBlockRecordHandle.ref ^= 1u; })
        && check([&]() { record->layoutHandle ^= 1u; });
    if (ok) adapter.legacyBlockRecordSourceFailed();
    const bool stopped = ok && DxIfaceStandardDwgCaptureTestAccess::noLegacyBlockFrames(data)
        && !adapter.wantsLegacyBlockRecordSource(data.sourceVersion);
    const bool missingRejected = stopped && !adapter.fileImport((scratch / "missing.dwg").string(), &data, false)
        && DxIfaceStandardDwgCaptureTestAccess::noLegacyBlockFrames(data)
        && !DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(adapter);
    dx_data retry;
    const bool retried = missingRejected && adapter.fileImport(input, &retry, false)
        && DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(retry, source)
        && DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(adapter);
    FailingLegacyBlockCaptureAdapter failing;
    dx_data unretained;
    const bool tolerantRead = retried && failing.fileImport(input, &unretained, false)
        && DxIfaceStandardDwgCaptureTestAccess::noLegacyBlockFrames(unretained)
        && !failing.wantsLegacyBlockRecordSource(unretained.sourceVersion)
        && !DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(failing);
    failing.injectFailure = false;
    dx_data recovered;
    const bool recoveredRead = tolerantRead && failing.fileImport(input, &recovered, false)
        && DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(recovered, source)
        && !DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(failing);
    dx_iface trusted;
    dx_data trustedData;
    const bool trustedRetry = recoveredRead && trusted.fileImport(input, &trustedData, false)
        && DxIfaceStandardDwgCaptureTestAccess::legacyBlockFrames(trustedData, source)
        && DxIfaceStandardDwgCaptureTestAccess::boundLegacyLayout(trusted);
    std::filesystem::remove_all(scratch, error);
    return trustedRetry && !error;
}

bool strictOutputRefuses(dx_iface& reader, dx_data& data) {
    std::error_code error;
    const auto scratch = libdxfrw_test::reserveTestScratch(
        std::filesystem::temp_directory_path(), "libdxfrw-legacy-raw-output", error);
    if (error || scratch.empty()) return false;
    const auto destination = scratch / "out.dxf";
    const std::string sentinel = "unqualified legacy source destination preserved\n";
    bool preserved = true;
    for (bool binary : {false, true}) {
        { std::ofstream out(destination, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(reader, data, destination.string(), binary)
            || bytes(destination) != sentinel) preserved = false;
    }
    std::filesystem::remove_all(scratch, error);
    return preserved && !error;
}

bool checkLegacyMalformed(const std::string& input, const std::string& corrected,
                          const std::string& damaged) {
    if (damaged != "dictionary" && damaged != "cellmap") return false;
    dx_iface reader;
    dx_data bad;
    if (!reader.fileImport(input, &bad, false) || bad.sourceVersion != DRW::AC1014
        || bad.dwgReadOutcome.observations.objectParseFailures != 1u) return false;
    const auto badHandle = damaged == "dictionary" ? 0x12u : 0xc1u;
    if (DxIfaceStandardDwgCaptureTestAccess::hasOpaqueSource(bad, badHandle)) return false;
    const auto source = bytes(input);
    const bool otherRetained = damaged == "dictionary"
        ? DxIfaceStandardDwgCaptureTestAccess::legacyRawFrame(
              bad, 0xc1u, source, 17516u, 708u, "CELLSTYLEMAP", "AcDbCellStyleMap")
        : DxIfaceStandardDwgCaptureTestAccess::legacyRawFrame(
              bad, 0x12u, source, 5221u, 28u, "ACDBDICTIONARYWDFLT", "AcDbDictionaryWithDefault");
    if (!otherRetained || !strictOutputRefuses(reader, bad)) return false;
    dx_data retry;
    return reader.fileImport(corrected, &retry, false)
        && retry.dwgReadOutcome.observations.objectParseFailures == 0u
        && DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
            retry, bytes(corrected), 0xc1u, 17516u, 5221u, 28u);
}

bool checkLegacyDefaultRoleFallback(const std::string& input, const std::string& corrected) {
    dx_iface reader;
    dx_data bad;
    if (!reader.fileImport(input, &bad, false) || bad.sourceVersion != DRW::AC1014
        || bad.dwgReadOutcome.observations.objectParseFailures != 0u
        || !DxIfaceStandardDwgCaptureTestAccess::legacyRawFrame(
            bad, 0x12u, bytes(input), 5221u, 28u,
            "ACDBDICTIONARYWDFLT", "AcDbDictionaryWithDefault")
        || std::any_of(bad.dictionaryWithDefaults.begin(), bad.dictionaryWithDefaults.end(),
            [](const DRW_DictionaryWithDefault& item) { return item.handle == 0x12u; })
        || !strictOutputRefuses(reader, bad)) return false;
    dx_data retry;
    return reader.fileImport(corrected, &retry, false)
        && retry.dwgReadOutcome.observations.objectParseFailures == 0u
        && DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
            retry, bytes(corrected), 0xc1u, 17516u, 5221u, 28u);
}

bool checkLegacyEedUtf8Source(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false)
        || (data.sourceVersion != DRW::AC1012 && data.sourceVersion != DRW::AC1014)
        || data.dwgReadOutcome.observations.objectParseFailures != 0u) return false;
    if (data.sourceVersion == DRW::AC1014
        && !DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
            data, bytes(input), 0xc1u, 17516u, 5221u, 28u)) return false;
    const auto dictionary = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
        [](const DRW_Dictionary& item) { return item.handle == 0x31u; });
    if (dictionary == data.dictionaries.end() || dictionary->extData.size() < 4u
        || !DxIfaceStandardDwgCaptureTestAccess::cleanDictionaryText(data, 0x31u)) return false;
    const auto text = [](const DRW_Variant* value, int code, const char* expected) {
        return value && value->code() == code && value->type() == DRW_Variant::STRING
            && value->content.s && *value->content.s == expected;
    };
    if (!data.mBlock) return false;
    const auto line = std::find_if(data.mBlock->ent.begin(), data.mBlock->ent.end(),
        [](const DRW_Entity* entity) { return entity && entity->handle == 0x30u; });
    if (line == data.mBlock->ent.end() || !dynamic_cast<const DRW_Line*>(*line)
        || (*line)->extData.size() != 5u) return false;
    const auto& entityEed = (*line)->extData;
    const bool decoded = text(entityEed[0].get(), 1001, "CODEPAGE_TEST")
        && text(entityEed[1].get(), 1000, "ASCII")
        && text(entityEed[2].get(), 1000, "caf\xC3\xA9")
        && text(entityEed[3].get(), 1000, "\xE2\x82\xAC\xE6\xBC\xA2")
        && entityEed[4] && entityEed[4]->code() == 1070
        && entityEed[4]->type() == DRW_Variant::INTEGER && entityEed[4]->i_val() == 42
        && text(dictionary->extData[0], 1001, "CODEPAGE_TEST")
        && text(dictionary->extData[1], 1000, "caf\xC3\xA9")
        && text(dictionary->extData[2], 1000, "\xE2\x82\xAC\xE6\xBC\xA2")
        && dictionary->extData[3] && dictionary->extData[3]->code() == 1070
        && dictionary->extData[3]->type() == DRW_Variant::INTEGER
        && dictionary->extData[3]->i_val() == -42;
    if (!decoded || !DxIfaceStandardDwgCaptureTestAccess::legacyUtf8EedSource(data)) return false;
    auto& frameBytes = DxIfaceStandardDwgCaptureTestAccess::legacyUtf8EedBytes(data);
    const std::size_t padding = data.sourceVersion == DRW::AC1012 ? 4u : 6u;
    if (frameBytes.size() != (data.sourceVersion == DRW::AC1012 ? 60u : 38u)) return false;
    for (std::size_t bit = 0; bit < frameBytes.size() * 8u; ++bit) {
        frameBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        const bool accepted = DxIfaceStandardDwgCaptureTestAccess::legacyUtf8EedSource(data);
        frameBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (accepted != (bit >= frameBytes.size() * 8u - padding)) return false;
    }
    auto app = std::find_if(data.appIds.begin(), data.appIds.end(),
        [](const DRW_AppId& item) { return item.handle == 0x2fu; });
    if (app == data.appIds.end()) return false;
    const std::string originalName = app->name;
    app->name += "X";
    const bool changedAccepted = DxIfaceStandardDwgCaptureTestAccess::legacyUtf8EedSource(data);
    app->name = originalName;
    if (changedAccepted || !DxIfaceStandardDwgCaptureTestAccess::legacyUtf8EedSource(data)) return false;
    return strictOutputRefuses(reader, data)
        && (data.sourceVersion != DRW::AC1014
            || DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
                data, bytes(input), 0xc1u, 17516u, 5221u, 28u));
}

bool checkR13EedSource(const std::string& input) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(input, &data, false) || data.sourceVersion != DRW::AC1012
        || data.dwgReadOutcome.observations.objectParseFailures != 0u
        || !DxIfaceStandardDwgCaptureTestAccess::r13EedSource(data)
        || !DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
            data, bytes(input), 0xbbu, 17278u, 5190u, 51u)) return false;
    auto& frameBytes = DxIfaceStandardDwgCaptureTestAccess::r13EedBytes(data);
    if (frameBytes.size() != 35u) return false;
    for (std::size_t bit = 0; bit < frameBytes.size() * 8u; ++bit) {
        frameBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        const bool accepted = DxIfaceStandardDwgCaptureTestAccess::r13EedSource(data);
        frameBytes[bit / 8u] ^= static_cast<std::uint8_t>(0x80u >> (bit % 8u));
        if (accepted != (bit >= frameBytes.size() * 8u - 6u)) return false;
    }
    auto app = std::find_if(data.appIds.begin(), data.appIds.end(),
        [](const DRW_AppId& candidate) { return candidate.handle == 0x2au; });
    if (app == data.appIds.end()) return false;
    const auto original = app->name;
    app->name += "X";
    const bool changedAccepted = DxIfaceStandardDwgCaptureTestAccess::r13EedSource(data);
    app->name = original;
    if (changedAccepted || !DxIfaceStandardDwgCaptureTestAccess::r13EedSource(data)) return false;
    return strictOutputRefuses(reader, data)
        && DxIfaceStandardDwgCaptureTestAccess::legacyRawPair(
            data, bytes(input), 0xbbu, 17278u, 5190u, 51u);
}

bool r13GeneratedGuard(const std::string& input, const std::filesystem::path& output) {
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(input, &source, false)
        || !checkR14Generated(source, DRW::AC1012)) return false;
    for (const std::uint32_t handle : {0xcu, 0x17u, 0x19u})
        if (!DxIfaceStandardDwgCaptureTestAccess::dictionaryProof(source, handle)) return false;
    if (!DxIfaceStandardDwgCaptureTestAccess::styleProof(source, 0x18u)) return false;
    for (const int fault : {12, 13}) {
        DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
        const bool accepted = fault == 12
            ? DxIfaceStandardDwgCaptureTestAccess::dictionaryProof(source, 0x19u)
            : DxIfaceStandardDwgCaptureTestAccess::styleProof(source, 0x18u);
        DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, fault) ^= 1u;
        if (accepted) return false;
    }
    const std::string sentinel = "AC1012 strict destination preserved\n";
    for (const bool binary : {false, true}) {
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, source, output.string(), binary) || bytes(output) != sentinel
            || !write(adapter, source, output.string(), binary, DRW::AC1014,
                      dx_dwgReadCompletenessPolicy::AllowPartial)) return false;
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false)
            || !checkR14Generated(readback)) return false;
    }
    return true;
}

bool generatedInsertRetry(const std::string& input, const std::filesystem::path& output) {
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(input, &source, false)
        || source.sourceVersion != DRW::AC1014
        || source.mLineStyles.size() != 1u) return false;
    auto& style = source.mLineStyles.front();
    const double original = style.startAngle;
    const std::string sentinel = "generated INSERT retry destination\n";
    for (const bool binary : {false, true}) {
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        style.startAngle = original + 1.0;
        const bool failed = !write(adapter, source, output.string(), binary)
            && bytes(output) == sentinel;
        style.startAngle = original;
        if (!failed || !write(adapter, source, output.string(), binary)) return false;
        dx_iface fresh;
        dx_data reopened;
        if (!fresh.fileImport(output.string(), &reopened, false)
            || reopened.mLineStyles.size() != 1u) return false;
    }
    return true;
}

bool generatedR2000DefaultRetry(const std::string& input, const std::filesystem::path& output) {
    dx_iface adapter;
    dx_data source;
    if (!adapter.fileImport(input, &source, false)
        || source.sourceVersion != DRW::AC1015
        || source.dictionaryWithDefaults.size() != 1u
        || source.placeholders.size() != 1u
        || source.mLineStyles.size() != 1u
        || source.mLineStyles.front().elements.size() != 2u
        || source.layouts.size() != 2u) return false;
    auto& placeholder = source.placeholders.front();
    auto& element = source.mLineStyles.front().elements.front();
    auto& style = source.mLineStyles.front();
    auto& layout = source.layouts.front();
    const auto reactors = placeholder.reactorHandles;
    const auto linetype = element.linetypeIndex;
    const auto layoutOwner = layout.parentHandle;
    const auto angle = style.startAngle;
    auto& styleBytes = DxIfaceStandardDwgCaptureTestAccess::mlineSourceBytes(source, style.handle);
    if (styleBytes.empty()) return false;
    const std::string sentinel = "generated R2000 default graph retry\n";
    for (const DRW::Version target : {DRW::AC1015, DRW::AC1024}) for (const bool binary : {false, true})
    for (const int fault : {0, 1, 2, 3, 4}) {
        if (target != DRW::AC1015 && fault >= 3) continue;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (fault == 0) placeholder.reactorHandles = {DRW::NoHandle};
        else if (fault == 1) element.linetypeIndex = 7;
        else if (fault == 2) layout.parentHandle = DRW::NoHandle;
        else if (fault == 3) styleBytes.front() ^= 1u;
        else style.startAngle = angle + 1.0;
        const bool failed = !write(adapter, source, output.string(), binary, target,
                                   dx_dwgReadCompletenessPolicy::RequireComplete)
            && bytes(output) == sentinel;
        placeholder.reactorHandles = reactors;
        element.linetypeIndex = linetype;
        layout.parentHandle = layoutOwner;
        if (fault == 3) styleBytes.front() ^= 1u;
        style.startAngle = angle;
        if (!failed || !write(adapter, source, output.string(), binary, target,
                              dx_dwgReadCompletenessPolicy::RequireComplete)) return false;
        dx_iface fresh;
        dx_data reopened;
        if (!fresh.fileImport(output.string(), &reopened, false)
            || reopened.sourceVersion != target
            || !reopened.hasNamedObjectDictionary
            || reopened.dictionaryWithDefaults.size() != 1u
            || reopened.rawPlaceholders.size() != 1u) return false;
        const auto& dictionary = reopened.dictionaryWithDefaults.front();
        const auto& child = reopened.rawPlaceholders.front();
        const auto root = std::find_if(reopened.namedObjectDictionary.m_entries.begin(),
            reopened.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& entry) {
                return entry.m_name == "ACAD_PLOTSTYLENAME";
            });
        if (root == reopened.namedObjectDictionary.m_entries.end()
            || root->m_handle != dictionary.handle
            || dictionary.parentHandle != reopened.namedObjectDictionary.handle
            || dictionary.m_entries.size() != 1u
            || dictionary.m_entries.front().m_name != "Normal"
            || dictionary.m_entries.front().m_handle != child.handle
            || dictionary.m_defaultEntryHandle != child.handle
            || child.parentHandle != dictionary.handle) return false;
    }
    return true;
}
} // namespace

bool legacyClassFrameCensus(const std::string& source) {
    dx_iface adapter;
    dx_data data;
    if (!adapter.fileImport(source, &data, false)
        || (data.sourceVersion != DRW::AC1012 && data.sourceVersion != DRW::AC1014))
        return false;
    const auto findClass = [&data](const char* name) {
        return std::find_if(data.dwgClassCoverage.m_entries.begin(),
            data.dwgClassCoverage.m_entries.end(), [name](const auto& row) {
                return row.m_recordName == name;
            });
    };
    const auto geodata = findClass("GEODATA");
    const auto raster = findClass("RASTERVARIABLES");
    const auto wipeout = findClass("WIPEOUTVARIABLES");
    const auto layout = findClass("LAYOUT");
    const auto dictionary = findClass("ACDBDICTIONARYWDFLT");
    if (geodata == data.dwgClassCoverage.m_entries.end()
        || raster == data.dwgClassCoverage.m_entries.end()
        || wipeout == data.dwgClassCoverage.m_entries.end()
        || layout == data.dwgClassCoverage.m_entries.end()
        || dictionary == data.dwgClassCoverage.m_entries.end())
        return false;
    std::size_t count = 99u;
    if (!DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(data, geodata->m_classNumber, count)
        || count != 0u
        || !DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(data, raster->m_classNumber, count)
        || count != 0u
        || !DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(data, wipeout->m_classNumber, count)
        || count != 0u
        || !DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(data, layout->m_classNumber, count)
        || count != 2u
        || !DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(data, dictionary->m_classNumber, count)
        || count != 1u)
        return false;
    ++data.importGeneration;
    count = 99u;
    const bool rejected = !DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(
        data, geodata->m_classNumber, count) && count == 99u;
    --data.importGeneration;
    return rejected && DxIfaceStandardDwgCaptureTestAccess::legacyClassCount(
        data, geodata->m_classNumber, count) && count == 0u;
}

int main(int argc, char** argv) {
    const bool ok = argc == 3 && std::string(argv[1]) == "--legacy-class-frame-census"
                ? legacyClassFrameCensus(argv[2])
            : argc == 3 && std::string(argv[1]) == "--legacy-block-pair-wire"
        ? checkLegacyBlockPairWire(argv[2])
        : argc == 3 && std::string(argv[1]) == "--legacy-layout-graph"
        ? checkLegacyLayoutGraph(argv[2])
        : argc == 3 && std::string(argv[1]) == "--legacy-layout-wire"
        ? checkLegacyLayoutWire(argv[2])
        : argc == 4 && std::string(argv[1]) == "--generated-r2000-default-retry"
        ? generatedR2000DefaultRetry(argv[2], argv[3])
        : argc == 4 && std::string(argv[1]) == "--generated-insert-retry"
        ? generatedInsertRetry(argv[2], argv[3])
        : argc == 5 && std::string(argv[1]) == "--legacy-malformed"
        ? checkLegacyMalformed(argv[2], argv[3], argv[4])
        : argc == 5 && std::string(argv[1]) == "--layer-big-partial"
            ? legacyBigLayerPartial(argv[2], argv[3], argv[4])
        : argc == 5 && std::string(argv[1]) == "--layer-big-nod"
            ? legacyBigLayerPartial(argv[2], argv[3], argv[4], 0xcu)
        : argc == 5 && std::string(argv[1]) == "--layer-big-mline"
            ? legacyBigLayerPartial(argv[2], argv[3], argv[4], 0x17u)
        : argc == 4 && std::string(argv[1]) == "--legacy-xrecord-lengths"
            ? legacyXRecordLengthControls(argv[2], argv[3])
        : argc == 3 && std::string(argv[1]) == "--check-layer-dxf"
            ? checkLayerDxf(argv[2])
        : argc == 3 && std::string(argv[1]) == "--check-partial"
        ? checkPartialReadback(argv[2])
        : argc == 3 && std::string(argv[1]) == "--legacy-eed-utf8-source"
            ? checkLegacyEedUtf8Source(argv[2])
        : argc == 3 && std::string(argv[1]) == "--r13-eed-source"
            ? checkR13EedSource(argv[2])
        : argc == 3 && std::string(argv[1]) == "--check-r14-generated"
            ? checkR14GeneratedFile(argv[2])
        : argc == 3 && std::string(argv[1]) == "--check-r13-partial"
            ? checkR13PartialReadback(argv[2])
            : argc == 3 && std::string(argv[1]) == "--layer-policy-little"
                ? legacyLayerEedReadPolicy(argv[2], false)
            : argc == 3 && std::string(argv[1]) == "--layer-policy-big"
                ? legacyLayerEedReadPolicy(argv[2], true)
            : argc == 3 && std::string(argv[1]) == "--layer-policy-missing"
                ? legacyLayerEedReadPolicy(argv[2], true, true)
            : argc == 3 && std::string(argv[1]) == "--layer-policy-oda"
                ? legacyLayerEedReadPolicy(argv[2], true, false, true)
            : argc == 3 && std::string(argv[1]) == "--layer-policy-wide"
                ? legacyLayerEedReadPolicy(argv[2], true, false, false, true)
            : argc == 3 && std::string(argv[1]) == "--layer-policy-zero"
                ? legacyLayerEedReadPolicy(argv[2], true, false, false, false, true)
        : argc == 3 ? run(argv[1], argv[2])
        : argc == 4 && std::string(argv[1]) == "--partial"
            ? partial(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--legacy-bad-default-role"
                ? checkLegacyDefaultRoleFallback(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--r14-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO")
            : argc == 4 && std::string(argv[1]) == "--r13-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO")
            : argc == 5 && std::string(argv[1]) == "--r14-eed-dangling"
                ? legacyDanglingEedHandle(argv[2], argv[3], argv[4], DRW::AC1014)
            : argc == 5 && std::string(argv[1]) == "--r13-eed-dangling"
                ? legacyDanglingEedHandle(argv[2], argv[3], argv[4], DRW::AC1012)
            : argc == 4 && std::string(argv[1]) == "--r14-group-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-handle"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-group-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-points"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-group-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-binary"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-group-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-groups"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-group-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-reals"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-group-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-group-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-int32"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x12u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x12u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x12u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x12u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-mline-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x33u, 0x17u, false, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-mline-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x33u, 0x17u, false, false, true)
            : argc == 5 && std::string(argv[1]) == "--r14-mline-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1014, 0x17u, false, true)
            : argc == 5 && std::string(argv[1]) == "--r13-mline-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1012, 0x17u, false, true)
            : argc == 5 && std::string(argv[1]) == "--r14-mline-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1014, 0x17u)
            : argc == 5 && std::string(argv[1]) == "--r13-mline-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1012, 0x17u)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x12u, 0xcu, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-eed"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x12u, 0xcu, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x12u, 0xcu, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x12u, 0xcu, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x35u, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x35u, false, true)
            : argc == 4 && std::string(argv[1]) == "--r14-nod-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x33u, 0x35u, false, true)
            : argc == 4 && std::string(argv[1]) == "--r13-nod-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x33u, 0x35u, false, true)
            : argc == 5 && std::string(argv[1]) == "--r14-nod-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1014, 0x35u)
            : argc == 5 && std::string(argv[1]) == "--r13-nod-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1012, 0x35u)
            : argc == 5 && std::string(argv[1]) == "--r14-nod-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1014, 0xcu, true)
            : argc == 5 && std::string(argv[1]) == "--r13-nod-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1012, 0xcu, true)
            : argc == 5 && std::string(argv[1]) == "--r14-nod-wide-remapped"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1014, 0x35u, true)
            : argc == 5 && std::string(argv[1]) == "--r13-nod-wide-remapped"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1012, 0x35u, true)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-nonempty"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x12u, 0xdu, true)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-nonempty"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x12u, 0xdu, true)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-utf8-nonempty"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x12u, 0xdu, true)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-utf8-nonempty"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x12u, 0xdu, true)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-nonempty-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u, true)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-nonempty-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u, true)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-utf8-nonempty-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x33u, 0x34u, true)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-utf8-nonempty-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x33u, 0x34u, true)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u, 0x34u)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u, 0x34u)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x33u, 0x34u)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-utf8-remapped"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x33u, 0x34u)
            : argc == 5 && std::string(argv[1]) == "--r14-eed-bad-member"
                ? legacyBadGroupMember(argv[2], argv[3], argv[4], DRW::AC1014)
            : argc == 5 && std::string(argv[1]) == "--r13-eed-bad-member"
                ? legacyBadGroupMember(argv[2], argv[3], argv[4], DRW::AC1012)
            : argc == 5 && std::string(argv[1]) == "--r14-eed-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1014)
            : argc == 5 && std::string(argv[1]) == "--r13-eed-bad-graph"
                ? legacyBadEedGraph(argv[2], argv[3], argv[4], DRW::AC1012)
            : argc == 5 && std::string(argv[1]) == "--r14-eed-wide-remapped"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1014, 0x34u)
            : argc == 5 && std::string(argv[1]) == "--r13-eed-wide-remapped"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1012, 0x34u)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-alternate-app"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "HELLO", 0x33u)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-alternate-app"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "HELLO", 0x33u)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-utf8-alternate-app"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9", 0x33u)
            : argc == 4 && std::string(argv[1]) == "--r13-eed-utf8-alternate-app"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9", 0x33u)
            : argc == 4 && std::string(argv[1]) == "--r14-eed-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1014, "caf\xC3\xA9")
            : argc == 4 && std::string(argv[1]) == "--r13-eed-utf8"
                ? legacyDictionaryEedOutput(argv[2], argv[3], DRW::AC1012, "caf\xC3\xA9")
            : argc == 5 && std::string(argv[1]) == "--r14-eed-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1014)
            : argc == 5 && std::string(argv[1]) == "--r13-eed-wide"
                ? legacyWideEedRefusal(argv[2], argv[3], argv[4], DRW::AC1012)
            : argc == 4 && std::string(argv[1]) == "--legacy-little-layer-eed"
                ? legacyLayerEedPartial(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--r14-generated"
                ? legacyGenerated(argv[2], argv[3], DRW::AC1014)
            : argc == 4 && std::string(argv[1]) == "--r13-generated"
                ? r13GeneratedGuard(argv[2], argv[3])
            : argc == 5 && std::string(argv[1]) == "--r13-ownership-marker"
                ? r13OwnershipMarkerRefusal(argv[2], argv[3], argv[4])
            : argc == 4 && std::string(argv[1]) == "--r13-partial"
                && r13Partial(argv[2], argv[3]);
    if (!ok) {
        std::cerr << "AC1014 external output profile test failed\n";
        return 1;
    }
    return 0;
}
