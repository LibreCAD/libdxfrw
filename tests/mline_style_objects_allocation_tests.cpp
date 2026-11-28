// SPDX-License-Identifier: GPL-2.0-or-later
// Generated MLINESTYLE OBJECTS allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

DRW_MLineStyle makeStyle() {
    DRW_MLineStyle style;
    style.handle = 0xD740u;
    style.parentHandle = DRW::DwgNamedObjectsDictionaryHandle;
    style.name = "Generated allocation retry style";
    style.description = "Two independently resolved parallel lines";
    style.flags = 1;
    style.startAngle = 0.125;
    style.endAngle = 2.75;
    style.fillColor = 3;
    DRW_MLineElement first;
    first.offset = 0.375;
    first.color = 5;
    first.linetypeIndex = -1;
    DRW_MLineElement second;
    second.offset = -0.625;
    second.color = 256;
    second.linetypeIndex = 32766;
    style.elements = {first, second};
    for (const char* name : {"BYLAYER", "BYBLOCK", "CONTINUOUS"}) {
        DRW_MLineElement element;
        element.linetype = name;
        if (element.linetype == "BYBLOCK")
            element.linetypeHandle = 0x14u;
        style.elements.push_back(element);
    }
    style.extData.push_back(new DRW_Variant(1001, std::string("ACAD")));
    style.extData.push_back(new DRW_Variant(
        1000, std::string("mline-style-allocation-retry")));
    return style;
}

bool sameStyle(const DRW_MLineStyle& actual, bool readback = false) {
    const DRW_MLineStyle expected = makeStyle();
    if (actual.handle != expected.handle
        || actual.parentHandle != expected.parentHandle
        || actual.name != expected.name
        || actual.description != expected.description
        || actual.flags != expected.flags
        || actual.startAngle != expected.startAngle
        || actual.endAngle != expected.endAngle
        || actual.fillColor != expected.fillColor
        || actual.elements.size() != expected.elements.size()
        || actual.extData.size() != expected.extData.size())
        return false;
    for (std::size_t i = 0; i < expected.elements.size(); ++i) {
        const DRW_MLineElement& left = actual.elements[i];
        const DRW_MLineElement& right = expected.elements[i];
        std::string expectedLineType = right.linetype;
        if (readback && expectedLineType.empty()) {
            switch (right.linetypeIndex) {
            case -1:
            case 32767:
                expectedLineType = "BYLAYER";
                break;
            case 32766:
                expectedLineType = "BYBLOCK";
                break;
            case 0:
                expectedLineType = "CONTINUOUS";
                break;
            default:
                return false;
            }
        }
        if (left.offset != right.offset || left.color != right.color
            || (!readback && left.linetypeHandle != right.linetypeHandle)
            || left.linetype != expectedLineType)
            return false;
    }
    for (std::size_t i = 0; i < expected.extData.size(); ++i) {
        if (!actual.extData[i] || !expected.extData[i]
            || !actual.extData[i]->content.s || !expected.extData[i]->content.s
            || actual.extData[i]->code() != expected.extData[i]->code()
            || *actual.extData[i]->content.s != *expected.extData[i]->content.s)
            return false;
    }
    return true;
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), style(makeStyle()) {}

    void writeObjects() override {
        if (failAt < 0) {
            accepted = codec.writeMLineStyle(&style);
            return;
        }
        const std::size_t before = failedAllocations;
        AllocationFailure failure(failAt);
        try {
            accepted = codec.writeMLineStyle(&style);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = failedAllocations != before
            ? static_cast<long>(failedAllocations - before)
            : failAt - allocationsBeforeFailure;
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }

    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_MLineStyle style;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addMLineStyle(const DRW_MLineStyle& style) override {
        styles.push_back(style);
    }
    std::vector<DRW_MLineStyle> styles;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

bool readbackMatches(const std::filesystem::path& path) {
    dxfRW codec(path.string().c_str());
    Reader reader;
    return codec.read(&reader, false) && reader.styles.size() == 1u
        && sameStyle(reader.styles.front(), true);
}

const char* versionCode(DRW::Version version) {
    switch (version) {
    case DRW::AC1014: return "AC1014";
    case DRW::AC1015: return "AC1015";
    case DRW::AC1018: return "AC1018";
    case DRW::AC1021: return "AC1021";
    case DRW::AC1024: return "AC1024";
    case DRW::AC1027: return "AC1027";
    case DRW::AC1032: return "AC1032";
    default: return "unknown";
    }
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "mline-style-allocation.dxf";
    const std::string prior = "prior MLINESTYLE destination\n";
    constexpr std::array<DRW::Version, 7> versions{{
        DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
        DRW::AC1024, DRW::AC1027, DRW::AC1032}};
    for (const DRW::Version version : versions) {
      for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            exporter.failAt = 1000000;
            const bool written = codec.write(&exporter, version, binary);
            if (!written || !exporter.accepted || !sameStyle(exporter.style)
                || !readbackMatches(path) || !readBytes(path, expectedBytes)) {
                std::cerr << "clean MLINESTYLE write failed: write=" << written
                          << " accepted=" << exporter.accepted
                          << " source=" << sameStyle(exporter.style)
                          << " readback=" << readbackMatches(path) << '\n';
                return false;
            }
            allocationCount = exporter.reached;
        }
        if (allocationCount <= 0)
            return false;

        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream output(path, std::ios::binary | std::ios::trunc);
                output << prior;
                if (!output)
                    return false;
            }
            dxfRW codec(path.string().c_str());
            Export exporter(codec);
            exporter.failAt = offset;
            const bool written = codec.write(&exporter, version, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != 1
                || !readBytes(path, current) || current != prior
                || !sameStyle(exporter.style))
                return false;

            const bool retried = codec.write(&exporter, version, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted || !sameStyle(exporter.style)
                || !readBytes(path, retryBytes)
                || retryBytes != expectedBytes || !readbackMatches(path))
                return false;
        }
        std::cout << versionCode(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << " MLINESTYLE writer allocation offsets="
                  << allocationCount << '\n';
      }
    }
    return true;
}

bool refusalAndRetry(const std::filesystem::path& directory) {
    for (const bool binary : {false, true}) {
        const auto path = directory / (binary ? "refusal-binary.dxf" : "refusal-ascii.dxf");
        const std::string sentinel = "prior destination\n";
        {
            std::ofstream output(path, std::ios::binary | std::ios::trunc);
            output << sentinel;
        }
        dxfRW codec(path.string().c_str());
        Export exporter(codec);
        exporter.style.elements[2].linetypeHandle = 0x16u;
        std::string bytes;
        if (codec.write(&exporter, DRW::AC1024, binary)
            || !readBytes(path, bytes) || bytes != sentinel)
            return false;
        exporter.style.elements[2].linetypeHandle = 0xD751u;
        codec.setHandleRemap({{0xD751u, 0u}});
        if (codec.write(&exporter, DRW::AC1024, binary)
            || !readBytes(path, bytes) || bytes != sentinel)
            return false;
        codec.setHandleRemap(std::map<std::uint32_t, std::uint32_t>{});
        exporter.style.elements[2].linetypeHandle = 0;
        exporter.style.elements[2].linetype = "MISSING_CUSTOM";
        if (codec.write(&exporter, DRW::AC1024, binary)
            || !readBytes(path, bytes) || bytes != sentinel)
            return false;
        exporter.style.elements[2].linetype = "BYLAYER";
        if (!codec.write(&exporter, DRW::AC1024, binary)
            || !sameStyle(exporter.style) || !readbackMatches(path))
            return false;
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        allocationsBeforeFailure = -1;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0u ? 1u : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main() {
    const auto directory = std::filesystem::temp_directory_path()
        / ("mline-style-objects-allocation-"
           + std::to_string(static_cast<long long>(
                 std::filesystem::file_time_type::clock::now()
                     .time_since_epoch().count())));
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return 1;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove_all(path, error);
        }
    } cleanup{directory};
    return allocationSweep(directory) && refusalAndRetry(directory) ? 0 : 2;
}
