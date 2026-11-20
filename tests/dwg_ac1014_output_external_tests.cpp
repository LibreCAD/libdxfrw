// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static std::uint8_t& sourceBit(dx_data& data, int fault) {
        const std::uint32_t handle = fault == 7 ? 0xcu : fault == 8 ? 0x22u
            : fault == 9 ? 0x23u : fault == 10 ? 0x1au : 0x1bu;
        const dx_data::StandardDwgFrameKey key{data.importGeneration, handle};
        if (fault == 7 || fault == 8 || fault == 10)
            return data.m_standardDwgObjectFrames.at(key).raw.m_rawBytes.back();
        return data.m_opaqueDwgSourceFrames.at(key).m_rawBytes.back();
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
} // namespace

int main(int argc, char** argv) {
    const bool ok = argc == 3 && std::string(argv[1]) == "--check-partial"
        ? checkPartialReadback(argv[2])
        : argc == 3 ? run(argv[1], argv[2])
        : argc == 4 && std::string(argv[1]) == "--partial"
            && partial(argv[2], argv[3]);
    if (!ok) {
        std::cerr << "AC1014 external output profile test failed\n";
        return 1;
    }
    return 0;
}
