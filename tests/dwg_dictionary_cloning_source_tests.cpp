// SPDX-License-Identifier: GPL-2.0-or-later
// Source-bound DICTIONARY cloning-field presence; this does not qualify output.
#include "dx_iface.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
bool unavailable(const dx_data& data, std::uint32_t handle) {
    dx_dwgDictionaryCloningSource observation;
    observation.presence = dx_dwgDictionaryCloningPresence::Present;
    observation.version = DRW::AC1032;
    observation.sourceValue = 7;
    observation.currentValueMatchesBaseline = true;
    return !data.getDwgDictionaryCloningSource(handle, observation)
        && observation.presence == dx_dwgDictionaryCloningPresence::Unknown
        && observation.version == DRW::UNKNOWNV
        && !observation.sourceValue
        && !observation.currentValueMatchesBaseline;
}

bool checkOne(const char* path, DRW::Version expectedVersion,
              dx_dwgDictionaryCloningPresence expectedPresence,
              std::optional<int> expectedSourceValue) {
    dx_iface reader;
    dx_data data;
    if (!reader.fileImport(path, &data, false) || data.sourceVersion != expectedVersion
        || !data.hasNamedObjectDictionary || data.namedObjectDictionary.handle == 0u)
        return false;
    const auto root = std::find_if(data.namedObjectDictionary.m_entries.begin(),
        data.namedObjectDictionary.m_entries.end(), [](const DRW_Dictionary::Entry& entry) {
            return entry.m_name == "ACAD_MLINESTYLE";
        });
    if (root == data.namedObjectDictionary.m_entries.end()) return false;
    auto nested = std::find_if(data.dictionaries.begin(), data.dictionaries.end(),
        [&root](const DRW_Dictionary& value) { return value.handle == root->m_handle; });
    if (nested == data.dictionaries.end()) return false;
    dx_dwgDictionaryCloningSource observation;
    const auto check = [&](const DRW_Dictionary& live) {
        return data.getDwgDictionaryCloningSource(live.handle, observation)
            && observation.version == expectedVersion
            && observation.presence == expectedPresence
            && (expectedPresence == dx_dwgDictionaryCloningPresence::Absent
                ? !observation.sourceValue
                : observation.sourceValue == expectedSourceValue
                    && observation.sourceValue == live.cloning)
            && observation.currentValueMatchesBaseline;
    };
    if (!check(data.namedObjectDictionary)) {
        std::cerr << "root source observation failed: " << path
                  << " handle=" << data.namedObjectDictionary.handle << '\n';
        return false;
    }
    if (!check(*nested)) {
        std::cerr << "nested source observation failed: " << path
                  << " handle=" << nested->handle << '\n';
        return false;
    }
    const int original = nested->cloning;
    nested->cloning = original == 2 ? 1 : 2;
    if (!data.getDwgDictionaryCloningSource(nested->handle, observation)
        || (expectedPresence == dx_dwgDictionaryCloningPresence::Absent
            ? observation.sourceValue.has_value()
            : observation.sourceValue != original)
        || observation.currentValueMatchesBaseline)
        return false;
    nested->cloning = original;
    if (!check(*nested) || !unavailable(data, 0u)
        || !unavailable(data, DRW::NoHandle)) return false;
    const auto handle = nested->handle;
    // A duplicate live identity cannot be bound to the immutable source.
    data.dictionaries.push_back(*nested);
    if (!unavailable(data, handle)) return false;
    data.dictionaries.pop_back();
    const auto generation = data.importGeneration;
    ++data.importGeneration;
    if (!unavailable(data, handle)) return false;
    data.importGeneration = generation;
    if (!data.getDwgDictionaryCloningSource(handle, observation)
        || !observation.currentValueMatchesBaseline) return false;
    const auto format = data.sourceFormat;
    data.sourceFormat = dx_source_format::DXF;
    if (!unavailable(data, handle)) return false;
    data.sourceFormat = format;
    if (!data.getDwgDictionaryCloningSource(handle, observation)) return false;
    data.dwgReadOutcome.operationSucceeded = false;
    if (!unavailable(data, handle)) return false;
    data.dwgReadOutcome.operationSucceeded = true;
    if (!data.getDwgDictionaryCloningSource(handle, observation)) return false;
    const auto version = data.sourceVersion;
    data.sourceVersion = DRW::UNKNOWNV;
    if (!unavailable(data, handle)) return false;
    data.sourceVersion = version;
    if (!data.getDwgDictionaryCloningSource(handle, observation)) return false;
    // This equality is not an edit detector: assigning the same value is
    // indistinguishable from leaving the public scalar untouched.
    std::cout << "source version=" << static_cast<int>(version)
              << " presence=" << static_cast<int>(observation.presence)
              << " value=" << (observation.sourceValue
                    ? std::to_string(*observation.sourceValue) : "absent") << '\n';
    return true;
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 5) return 1;
    if (!checkOne(argv[1], DRW::AC1012, dx_dwgDictionaryCloningPresence::Absent, std::nullopt)
        || !checkOne(argv[2], DRW::AC1014, dx_dwgDictionaryCloningPresence::Absent, std::nullopt)
        || !checkOne(argv[3], DRW::AC1015, dx_dwgDictionaryCloningPresence::Present, 0)
        || !checkOne(argv[4], DRW::AC1032, dx_dwgDictionaryCloningPresence::Present, 1)) {
        std::cerr << "DICTIONARY cloning source observation failed\n";
        return 2;
    }
    dx_data constructed;
    if (!unavailable(constructed, 1u)) return 3;
    return 0;
}
