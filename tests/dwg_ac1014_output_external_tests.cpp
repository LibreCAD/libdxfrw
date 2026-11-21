// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include "dx_legacy_dwg_wire.h"
#include "dx_legacy_mlinestyle_wire.h"
#include "test_scratch.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
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
    static std::vector<std::uint8_t>& legacyUtf8EedBytes(dx_data& data) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, 0x31u}).raw.m_rawBytes;
    }
    static std::vector<std::uint8_t>& r13EedBytes(dx_data& data) {
        return data.m_standardDwgObjectFrames.at({data.importGeneration, 0x30u}).raw.m_rawBytes;
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
            : fault == 13 ? 0x18u : fault == 14 ? 0xdu : 0x1bu;
        const dx_data::StandardDwgFrameKey key{data.importGeneration, handle};
        if (fault == 7 || fault == 8 || fault == 10 || fault == 12 || fault == 14)
            return data.m_standardDwgObjectFrames.at(key).raw.m_rawBytes.front();
        return data.m_opaqueDwgSourceFrames.at(key).m_rawBytes.front();
    }
};

namespace {
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

bool r14DictionaryEedOutput(const std::string& input, const std::filesystem::path& output) {
    LateMutationAdapter adapter;
    dx_data source;
    adapter.source = &source;
    if (!adapter.fileImport(input, &source, false) || !checkR14Generated(source)) return false;
    auto group = std::find_if(source.dictionaries.begin(), source.dictionaries.end(),
        [](const DRW_Dictionary& dictionary) { return dictionary.handle == 0xdu; });
    auto app = std::find_if(source.appIds.begin(), source.appIds.end(),
        [](const DRW_AppId& item) { return item.handle == 0x12u; });
    if (group == source.dictionaries.end() || app == source.appIds.end()
        || group->extData.size() != 3u || !group->extData[0] || !group->extData[1]
        || !group->extData[2] || group->extData[0]->code() != 1001
        || group->extData[1]->code() != 1000 || group->extData[2]->code() != 1070
        || group->extData[0]->type() != DRW_Variant::STRING
        || group->extData[1]->type() != DRW_Variant::STRING
        || group->extData[2]->type() != DRW_Variant::INTEGER
        || !group->extData[0]->content.s || *group->extData[0]->content.s != "ACAD"
        || !group->extData[1]->content.s || *group->extData[1]->content.s != "HELLO"
        || group->extData[2]->i_val() != 42 || app->name != "ACAD") return false;
    const DRW_Dictionary saved = *group;
    const std::string sentinel = "R14 EED destination preserved\n";
    for (const bool binary : {false, true}) {
        if (!write(adapter, source, output.string(), binary)) return false;
        { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
        if (write(adapter, source, output.string(), binary, DRW::AC1015)
            || bytes(output) != sentinel || !write(adapter, source, output.string(), binary)) return false;
        for (int fault = 0; fault < 6; ++fault) {
            if (fault == 0) group->extData[1]->addString(1000, "CHANGED");
            if (fault == 1) group->extData[2]->addInt(1070, 43);
            if (fault == 2) app->name = "CHANGED";
            if (fault == 3) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, 14) ^= 1u;
            if (fault == 4) adapter.lateFault = 14;
            if (fault == 5) group->extData[0]->addString(1001, "OTHER");
            { std::ofstream out(output, std::ios::binary | std::ios::trunc); out << sentinel; }
            const bool accepted = write(adapter, source, output.string(), binary);
            if (fault == 3) DxIfaceStandardDwgCaptureTestAccess::sourceBit(source, 14) ^= 1u;
            if (fault == 4) adapter.lateFault = 0;
            app->name = "ACAD";
            *group = saved;
            if (accepted || bytes(output) != sentinel
                || !write(adapter, source, output.string(), binary)) {
                std::cerr << "R14 EED refusal/retry failed: binary=" << binary
                          << " fault=" << fault << '\n';
                return false;
            }
        }
        dx_iface fresh;
        dx_data readback;
        if (!fresh.fileImport(output.string(), &readback, false) || !checkR14Generated(readback)) return false;
        const auto result = std::find_if(readback.dictionaries.begin(), readback.dictionaries.end(),
            [](const DRW_Dictionary& dictionary) { return dictionary.handle == 0xdu; });
        if (result == readback.dictionaries.end() || result->extData.size() != 3u
            || !result->extData[0] || !result->extData[1] || !result->extData[2]
            || result->extData[0]->type() != DRW_Variant::STRING
            || result->extData[1]->type() != DRW_Variant::STRING
            || result->extData[2]->type() != DRW_Variant::INTEGER
            || !result->extData[0]->content.s || *result->extData[0]->content.s != "ACAD"
            || !result->extData[1]->content.s || *result->extData[1]->content.s != "HELLO"
            || result->extData[2]->i_val() != 42) return false;
    }
    return true;
}

bool checkR14GeneratedFile(const std::string& input) {
    dx_iface reader;
    dx_data data;
    return reader.fileImport(input, &data, false) && checkR14Generated(data);
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
} // namespace

int main(int argc, char** argv) {
    const bool ok = argc == 5 && std::string(argv[1]) == "--legacy-malformed"
        ? checkLegacyMalformed(argv[2], argv[3], argv[4])
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
        : argc == 3 ? run(argv[1], argv[2])
        : argc == 4 && std::string(argv[1]) == "--partial"
            ? partial(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--legacy-bad-default-role"
                ? checkLegacyDefaultRoleFallback(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--r14-eed"
                ? r14DictionaryEedOutput(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--r14-generated"
                ? legacyGenerated(argv[2], argv[3], DRW::AC1014)
            : argc == 4 && std::string(argv[1]) == "--r13-generated"
                ? r13GeneratedGuard(argv[2], argv[3])
            : argc == 4 && std::string(argv[1]) == "--r13-partial"
                && r13Partial(argv[2], argv[3]);
    if (!ok) {
        std::cerr << "AC1014 external output profile test failed\n";
        return 1;
    }
    return 0;
}
