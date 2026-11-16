// SPDX-License-Identifier: GPL-2.0-or-later
// Generated LTYPE table-record allocation rollback/retry control.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t allocationAttempts = 0u;
bool allocationFailed = false;

void armAllocationFailure(long offset) noexcept {
    allocationsBeforeFailure = offset;
    allocationAttempts = 0u;
    allocationFailed = false;
}
void disarmAllocationFailure() noexcept { allocationsBeforeFailure = -1; }

DRW_LType makeLType() {
    DRW_LType lineType;
    lineType.handle = 0xD730u;
    lineType.name = "Generated allocation retry LTYPE";
    lineType.desc = "Generated dash profile";
    lineType.alignment = 'A';
    lineType.path = {10.0, -5.0};
    return lineType;
}

bool sameCaller(const DRW_LType& actual) {
    const DRW_LType expected = makeLType();
    return actual.tType == DRW::LTYPE
        && actual.handle == expected.handle
        && actual.name == expected.name
        && actual.desc == expected.desc
        && actual.alignment == expected.alignment
        && actual.path == expected.path
        && actual.size == expected.size
        && actual.length == expected.length
        && actual.segments.empty();
}

bool sameReadback(const DRW_LType& actual) {
    return actual.tType == DRW::LTYPE && actual.handle != 0u
        && actual.name == "Generated allocation retry LTYPE"
        && actual.desc == "Generated dash profile"
        && actual.alignment == 'A'
        && actual.path == std::vector<double>{10.0, -5.0}
        && actual.size == 2 && actual.length == 15.0;
}

bool hasEditMetadata(const DRW_LType& lineType) {
    if (lineType.appData.size() != 1u || lineType.appData.front().size() != 4u)
        return false;
    auto value = lineType.appData.front().cbegin();
    if (value->code() != 102 || value->c_str() == nullptr
        || std::string(value->c_str()) != "LTYPE_EDIT_METADATA")
        return false;
    ++value;
    if (value->code() != 1 || value->c_str() == nullptr
        || std::string(value->c_str())
            != "preserve while editing definition")
        return false;
    ++value;
    if (value->code() != 310 || value->c_str() == nullptr
        || std::string(value->c_str()) != "CAFE")
        return false;
    ++value;
    return value->code() == 102 && value->c_str() != nullptr
        && std::string(value->c_str()) == "}";
}

bool sameEditedReadback(const DRW_LType& actual, const DRW_LType& edited) {
    return actual.tType == DRW::LTYPE && actual.handle != 0u
        && actual.name == edited.name && actual.desc == edited.desc
        && actual.alignment == edited.alignment
        && actual.path == std::vector<double>{3.0, -1.0}
        && actual.size == 2 && actual.length == 4.0
        && actual.segments.size() == 2u
        && actual.segments[0].length == 3.0
        && actual.segments[0].shapeFlags == 0x02
        && actual.segments[0].text == "X"
        && actual.segments[1].length == -1.0
        && actual.segments[1].shapeFlags == 0
        && actual.appData.size() == edited.appData.size()
        && hasEditMetadata(actual);
}

class Export final : public LC3CompatCheck {
public:
    explicit Export(dxfRW& codec) : codec(codec), lineType(makeLType()) {}
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override {
        if (failAt < 0) {
            accepted = codec.writeLineType(&lineType);
            return;
        }
        armAllocationFailure(failAt);
        try {
            accepted = codec.writeLineType(&lineType);
        } catch (const std::bad_alloc&) {
            accepted = false;
        }
        reached = static_cast<long>(allocationAttempts);
        disarmAllocationFailure();
        if (!accepted)
            (void)codec.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_LType lineType;
    long failAt = -1;
    long reached = 0;
    bool accepted = false;
};

bool readBytes(const std::filesystem::path& path, std::string& bytes);

class CanonicalLTypeExport final : public LC3CompatCheck {
public:
    CanonicalLTypeExport(dxfRW& writer, const std::string& name) : codec(writer) {
        row.name = name;
        row.handle = 0xD731u;
        if (name == "CONTINUOUS") row.desc = "Solid line";
    }
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override { accepted = codec.writeLineType(&row); }
    bool finalizeDxfWrite() override { return accepted; }
    dxfRW& codec;
    DRW_LType row;
    bool accepted = false;
};

class CanonicalLTypeReader final : public LC3CompatCheck {
public:
    explicit CanonicalLTypeReader(std::string expectedName) : name(std::move(expectedName)) {}
    void addLType(const DRW_LType& row) override {
        if (row.name == name) { ++count; flags = row.flags; }
    }
    std::string name;
    unsigned count = 0;
    int flags = -1;
};

bool canonicalFlags(const std::filesystem::path& directory) {
    const auto path = directory / "canonical-ltype-flags.dxf";
    const auto cleanPath = directory / "canonical-ltype-clean.dxf";
    const std::string sentinel = "prior canonical linetype output\n";
    for (const auto version : {DRW::AC1009, DRW::AC1014, DRW::AC1032}) {
        for (const bool binary : {false, true}) {
            for (const std::string name : {"BYBLOCK", "BYLAYER", "CONTINUOUS"}) {
                dxfRW cleanCodec(cleanPath.string().c_str());
                CanonicalLTypeExport clean(cleanCodec, name);
                if (!cleanCodec.write(&clean, version, binary) || !clean.accepted)
                    return false;
                std::string expected;
                if (!readBytes(cleanPath, expected)) return false;
                { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << sentinel; }
                dxfRW codec(path.string().c_str());
                CanonicalLTypeExport candidate(codec, name);
                for (unsigned fault = 0; fault != 6; ++fault) {
                    candidate.row = clean.row;
                    if (fault == 0) candidate.row.size = 1;
                    if (fault == 1) candidate.row.length = 1.0;
                    if (fault == 2) candidate.row.alignment = 'B';
                    if (fault == 3) candidate.row.path.push_back(1.0);
                    if (fault == 4) candidate.row.segments.emplace_back();
                    if (fault == 5) candidate.row.shapeHandles.push_back(0x123u);
                    const DRW_LType before = candidate.row;
                    std::string current;
                    { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << sentinel; }
                    if (codec.write(&candidate, version, binary) || candidate.accepted
                        || !readBytes(path, current) || current != sentinel
                        || candidate.row.size != before.size || candidate.row.length != before.length
                        || candidate.row.alignment != before.alignment || candidate.row.path != before.path
                        || candidate.row.segments.size() != before.segments.size()
                        || candidate.row.shapeHandles != before.shapeHandles)
                        return false;
                    codec.setCanonicalLineTypeMetadata({before});
                    candidate.row = clean.row;
                    if (codec.write(&candidate, version, binary)
                        || !readBytes(path, current) || current != sentinel)
                        return false;
                    codec.setCanonicalLineTypeMetadata(std::vector<DRW_LType>{});
                    if (!codec.write(&candidate, version, binary) || !candidate.accepted
                        || !readBytes(path, current) || current != expected)
                        return false;
                }
                { std::ofstream out(path, std::ios::binary | std::ios::trunc); out << sentinel; }
                candidate.row.flags = 64;
                std::string current;
                if (codec.write(&candidate, version, binary) || candidate.accepted
                    || candidate.row.flags != 64 || !readBytes(path, current) || current != sentinel) {
                    std::cerr << "unseeded canonical LTYPE flags were not refused: " << name << '\n';
                    return false;
                }
                candidate.row.flags = 0;
                if (!codec.write(&candidate, version, binary) || !candidate.accepted
                    || !readBytes(path, current) || current != expected)
                    return false;
                candidate.row.flags = 64;
                codec.setCanonicalLineTypeMetadata({candidate.row});
                if (!codec.write(&candidate, version, binary) || !candidate.accepted)
                    return false;
                const std::string wireName = version == DRW::AC1009 ? name
                    : name == "BYBLOCK" ? "ByBlock" : name == "BYLAYER" ? "ByLayer" : "Continuous";
                dxfRW readerCodec(path.string().c_str());
                CanonicalLTypeReader reader(wireName);
                if (!readerCodec.read(&reader, false) || reader.count != 1u || reader.flags != 64)
                    return false;
                std::string seeded;
                if (!readBytes(path, seeded)) return false;
                for (const int flags : {16, 48, 80, 112}) {
                    candidate.row.flags = flags;
                    codec.setCanonicalLineTypeMetadata({candidate.row});
                    if (!codec.write(&candidate, version, binary) || !candidate.accepted)
                        return false;
                    dxfRW variantCodec(path.string().c_str());
                    CanonicalLTypeReader variant(wireName);
                    if (!variantCodec.read(&variant, false) || variant.count != 1u || variant.flags != flags)
                        return false;
                }
                candidate.row.flags = 64;
                codec.setCanonicalLineTypeMetadata({candidate.row});
                if (!codec.write(&candidate, version, binary) || !candidate.accepted
                    || !readBytes(path, current) || current != seeded)
                    return false;
                // Predeclared metadata is authoritative when an adapter
                // supplies the ordinary default canonical callback row.
                candidate.row.flags = 0;
                if (!codec.write(&candidate, version, binary) || !candidate.accepted
                    || !readBytes(path, current) || current != seeded)
                    return false;
                candidate.row.flags = 16;
                if (codec.write(&candidate, version, binary) || candidate.accepted
                    || candidate.row.flags != 16 || !readBytes(path, current) || current != seeded)
                    return false;
                candidate.row.flags = 64;
                if (!codec.write(&candidate, version, binary) || !candidate.accepted
                    || !readBytes(path, current) || current != seeded)
                    return false;
                candidate.row.flags = 32; // Resolved-xref bit without xref bit.
                codec.setCanonicalLineTypeMetadata({candidate.row});
                if (codec.write(&candidate, version, binary)
                    || !readBytes(path, current) || current != seeded)
                    return false;
                candidate.row.flags = 64;
                codec.setCanonicalLineTypeMetadata({candidate.row});
                if (!codec.write(&candidate, version, binary) || !candidate.accepted
                    || !readBytes(path, current) || current != seeded)
                    return false;
            }
        }
    }
    return true;
}

class EditedExport final : public LC3CompatCheck {
public:
    EditedExport(dxfRW& codec, const DRW_LType& lineType)
        : codec(codec), lineType(lineType) {}
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override {
        accepted = codec.writeLineType(&lineType);
    }
    bool finalizeDxfWrite() override { return accepted; }

    dxfRW& codec;
    DRW_LType lineType;
    bool accepted = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addLType(const DRW_LType& value) override {
        if (value.name == "Generated allocation retry LTYPE")
            lineTypes.push_back(value);
    }
    std::vector<DRW_LType> lineTypes;
};

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

constexpr std::array<DRW::Version, 7> ltypeVersions{{
    DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
    DRW::AC1024, DRW::AC1027, DRW::AC1032}};

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
    return codec.read(&reader, false) && reader.lineTypes.size() == 1u
        && sameReadback(reader.lineTypes.front());
}

bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "ltype-table-allocation.dxf";
    const std::string prior = "prior LTYPE destination bytes\n";
    constexpr auto& versions = ltypeVersions;
    for (const DRW::Version version : versions) {
        for (const bool binary : {false, true}) {
            long allocationCount = 0;
            std::string expectedBytes;
            {
                dxfRW codec(path.string().c_str());
                Export exporter(codec);
                exporter.failAt = 1000000;
                const bool written = codec.write(&exporter, version, binary);
                if (!written || !exporter.accepted
                    || !sameCaller(exporter.lineType)
                    || !readbackMatches(path)
                    || !readBytes(path, expectedBytes)) {
                    std::cerr << "clean LTYPE write failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " written=" << written
                              << " accepted=" << exporter.accepted << '\n';
                    return false;
                }
                allocationCount = exporter.reached;
            }
            if (allocationCount <= 0)
                return false;
            for (long offset = 0; offset < allocationCount; ++offset) {
                {
                    std::ofstream output(path,
                                         std::ios::binary | std::ios::trunc);
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
                if (written || exporter.accepted
                    || exporter.reached != offset + 1
                    || !allocationFailed || !readBytes(path, current)
                    || current != prior || !sameCaller(exporter.lineType)) {
                    std::cerr << "LTYPE rollback failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset
                              << " reached=" << exporter.reached
                              << " failed=" << allocationFailed << '\n';
                    return false;
                }
                const bool retried = codec.write(&exporter, version, binary);
                std::string retryBytes;
                if (!retried || !exporter.accepted
                    || !sameCaller(exporter.lineType)
                    || !readBytes(path, retryBytes)
                    || retryBytes != expectedBytes
                    || !readbackMatches(path)) {
                    std::cerr << "LTYPE retry failed: version="
                              << versionCode(version) << " binary=" << binary
                              << " offset=" << offset << '\n';
                    return false;
                }
            }
            std::cout << "LTYPE " << versionCode(version)
                      << (binary ? " Binary" : " ASCII")
                      << " table-writer allocation offsets="
                      << allocationCount << '\n';
        }
    }
    return true;
}

bool editedImportedRecordRoundTrip(const std::filesystem::path& directory) {
    const auto source = directory / "ltype-edit-source.dxf";
    const auto output = directory / "ltype-edit-output.dxf";
    DRW_LType sourceLineType = makeLType();
    sourceLineType.appData.push_back({
        {102, std::string("{LTYPE_EDIT_METADATA")},
        {1, std::string("preserve while editing definition")},
        {310, std::string("CAFE")},
        {102, std::string("}")}});
    DRW_LTypeSegment textElement;
    textElement.length = 10.0;
    textElement.shapeFlags = 0x02;
    textElement.text = "X";
    DRW_LTypeSegment trailingDash;
    trailingDash.length = -5.0;
    sourceLineType.segments = {textElement, trailingDash};
    {
        dxfRW codec(source.string().c_str());
        EditedExport exporter(codec, sourceLineType);
        if (!codec.write(&exporter, DRW::AC1027, false) || !exporter.accepted)
            return false;
    }

    dxfRW sourceReader(source.string().c_str());
    Reader imported;
    if (!sourceReader.read(&imported, false) || imported.lineTypes.size() != 1u) {
        std::cerr << "LTYPE metadata source did not import: rows="
                  << imported.lineTypes.size() << '\n';
        return false;
    }
    const DRW_LType& original = imported.lineTypes.front();
    if (!hasEditMetadata(original)
        || original.segments.size() != 2u
        || original.segments.front().shapeFlags != 0x02
        || original.segments.front().text != "X") {
        std::cerr << "LTYPE source metadata/complex fields mismatch: appGroups="
                  << original.appData.size() << " segments="
                  << original.segments.size();
        if (!original.appData.empty())
            std::cerr << " firstAppGroupSize=" << original.appData.front().size()
                      << " opener="
                      << (original.appData.front().front().c_str() == nullptr
                              ? "<null>"
                              : original.appData.front().front().c_str())
                      << " closer="
                      << (original.appData.front().back().c_str() == nullptr
                              ? "<null>"
                              : original.appData.front().back().c_str());
        if (!original.segments.empty())
            std::cerr << " shape=" << original.segments.front().shapeFlags
                      << " text=" << original.segments.front().text.c_str();
        std::cerr << '\n';
        return false;
    }
    DRW_LType edited = original;
    edited.desc = "Edited after import";
    edited.path = {3.0, -1.0};

    for (const DRW::Version version : ltypeVersions) {
        for (const bool binary : {false, true}) {
            dxfRW codec(output.string().c_str());
            EditedExport exporter(codec, edited);
            if (!codec.write(&exporter, version, binary) || !exporter.accepted) {
                std::cerr << "edited LTYPE output failed to write: version="
                          << versionCode(version) << " binary=" << binary << '\n';
                return false;
            }
            dxfRW outputReader(output.string().c_str());
            Reader readback;
            if (!outputReader.read(&readback, false)
                || readback.lineTypes.size() != 1u) {
                std::cerr << "edited LTYPE output failed to import: version="
                          << versionCode(version) << " binary=" << binary
                          << " rows=" << readback.lineTypes.size() << '\n';
                return false;
            }
            const DRW_LType& actual = readback.lineTypes.front();
            if (!sameEditedReadback(actual, edited)) {
                std::cerr << "edited imported LTYPE fields were not re-emitted: "
                             "version=" << versionCode(version)
                          << " binary=" << binary << '\n';
                return false;
            }
        }
    }
    std::cout << "edited imported LTYPE description, complex segments, and "
                 "application metadata readback passed for seven revisions "
                 "in ASCII and Binary\n";
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++allocationAttempts;
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0) {
        --allocationsBeforeFailure;
        ++allocationAttempts;
    }
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
        / ("ltype-table-allocation-"
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
    return canonicalFlags(directory) && allocationSweep(directory)
            && editedImportedRecordRoundTrip(directory)
        ? 0 : 2;
}
