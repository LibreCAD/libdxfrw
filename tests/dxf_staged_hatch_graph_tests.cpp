// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "intern/dxf_raster_output.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
using Links = dxfRW::DxfHatchOutputLinks;

struct Exporter : LC3CompatCheck {
    Exporter(dxfRW& output, bool reverse) : output(output), reverse(reverse) {}
    dxfRW& output;
    bool reverse;
    unsigned extraReference = 0;
    Links links;
    bool ok = false;

    void writeEntities() override {
        const std::uint32_t hatchSource = 0xA1u, boundarySource = 0xB1u;
        links.hatch = output.preallocateEntityHandle(hatchSource);
        links.boundary = output.preallocateEntityHandle(boundarySource);
        links.owner = DRW::DxfModelSpaceBlockRecordHandle;
        if (!links.hatch || !links.boundary) { ok = false; return; }
        DRW_Hatch hatch;
        hatch.handle = hatchSource;
        hatch.name = "SOLID";
        hatch.solid = 1;
        hatch.associative = 1;
        auto loop = std::make_shared<DRW_HatchLoop>(1);
        const DRW_Coord points[] = {{0, 0, 0}, {10, 0, 0}, {10, 10, 0}, {0, 10, 0}};
        for (unsigned i = 0; i < 4; ++i) {
            auto edge = std::make_shared<DRW_Line>();
            edge->basePoint = points[i];
            edge->secPoint = points[(i + 1u) % 4u];
            loop->objlist.push_back(edge);
        }
        loop->m_boundaryHandles = {boundarySource};
        loop->update();
        hatch.appendLoop(loop);
        DRW_LWPolyline boundary;
        boundary.handle = boundarySource;
        boundary.flags = 1;
        boundary.reactorHandles = {hatchSource};
        for (unsigned i = 0; i < 4; ++i) {
            const auto& p = points[reverse ? (6u - i) % 4u : (i + 1u) % 4u];
            boundary.addVertex(DRW_Vertex2D(p.x, p.y, 0));
        }
        DRW_Line line;
        line.handle = 0xC1u;
        line.basePoint = DRW_Coord(20, 20, 0);
        line.secPoint = DRW_Coord(30, 20, 0);
        DRW_Line other;
        other.handle = 0xC2u;
        other.basePoint = DRW_Coord(20, 30, 0);
        other.secPoint = DRW_Coord(30, 30, 0);
        line.reactorHandles = {extraReference == 1u ? hatchSource : extraReference == 2u ? boundarySource : other.handle};
        if (extraReference == 3u) {
            line.extData.push_back(std::make_shared<DRW_Variant>(1001, std::string("ACAD")));
            line.extData.push_back(std::make_shared<DRW_Variant>(1000, std::string("opaque")));
        }
        if (extraReference == 4u) line.color24 = 0x112233;
        ok = output.preallocateEntityHandle(line.handle) && output.preallocateEntityHandle(other.handle)
            && output.writeHatch(&hatch) && output.writeLWPolyline(&boundary)
            && output.writeLine(&line) && output.writeLine(&other);
    }
    bool finalizeDxfWrite() override { return ok; }
};

std::string read(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), {}};
}
void write(const std::filesystem::path& path, const std::string& value) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    output.write(value.data(), static_cast<std::streamsize>(value.size()));
}

bool mutate(std::string& bytes, bool binary, dx_raster_output::Profile profile, unsigned mode) {
    dx_raster_output::Cursor cursor(bytes, binary, profile);
    dx_raster_output::Token token;
    bool hatch = false, boundary = false, reactor = false, afterCount = false, inPath = false;
    std::vector<std::size_t> edgeOffsets;
    while (!cursor.done()) {
        const auto start = cursor.position();
        if (!cursor.next(token)) return false;
        if (token.code == 0) { hatch = token.text == "HATCH"; boundary = token.text == "LWPOLYLINE"; reactor = false; afterCount = false; inPath = false; continue; }
        if (mode == 23 && hatch && inPath && token.code == 72) {
            edgeOffsets.push_back(start);
            if (edgeOffsets.size() == 3u) {
                const auto first = bytes.substr(edgeOffsets[0], edgeOffsets[1] - edgeOffsets[0]);
                const auto second = bytes.substr(edgeOffsets[1], edgeOffsets[2] - edgeOffsets[1]);
                bytes.replace(edgeOffsets[0], edgeOffsets[2] - edgeOffsets[0], second + first);
                return true;
            }
        }
        if (boundary && token.code == 102) {
            if (token.text == "{ACAD_REACTORS") reactor = true;
            else if (token.text == "}") reactor = false;
        }
        const bool numeric = (hatch && ((mode == 6 && token.code == 71)
            || (mode == 7 && token.code == 97) || (mode == 10 && token.code == 72)
            || (mode == 11 && token.code == 92) || (mode == 12 && token.code == 70)
            || ((mode == 21 || mode == 22) && inPath && token.code == 93)))
            || (mode == 15 && boundary && token.code == 70);
        const bool real = (mode == 13 && hatch && inPath && token.code == 10)
            || (mode == 14 && boundary && token.code == 10)
            || (mode == 16 && hatch && !inPath && token.code == 30)
            || (mode == 17 && boundary && token.code == 43);
        const bool handle = (mode == 8 && hatch && afterCount && token.code == 330)
            || (mode == 9 && boundary && reactor && token.code == 330);
        if (numeric) {
            if (binary) {
                if (start + 2u >= bytes.size()) return false;
                if (mode == 21) {
                    if (start + 6u > bytes.size()) return false;
                    bytes[start + 2u] = 1; bytes[start + 3u] = 16;
                    bytes[start + 4u] = 0; bytes[start + 5u] = 0;
                } else bytes[start + 2u] = mode == 10 || mode == 11 || mode == 22 ? 2 : 0;
            } else {
                if (token.text.empty()) return false;
                const auto pos = static_cast<std::size_t>(token.text.data() - bytes.data()) + token.text.size() - 1u;
                if (mode == 21) {
                    if (token.text.size() < 4u) return false;
                    bytes.replace(pos - 3u, 4u, "4097");
                } else bytes[pos] = mode == 10 || mode == 11 || mode == 22 ? '2' : '0';
            }
            return true;
        }
        if (real) {
            if (binary) {
                if (start + 10u > bytes.size()) return false;
                const double value = token.real + 1.0;
                std::uint64_t bits = 0;
                std::memcpy(&bits, &value, sizeof(bits));
                for (unsigned i = 0; i < 8u; ++i)
                    bytes[start + 2u + i] = static_cast<char>((bits >> (8u * i)) & 0xFFu);
            } else {
                if (token.text.empty()) return false;
                const auto pos = static_cast<std::size_t>(token.text.data() - bytes.data()) + token.text.size() - 1u;
                bytes[pos] = bytes[pos] == '9' ? '8' : '9';
            }
            return true;
        }
        if (handle) {
            if (token.text.empty()) return false;
            const auto pos = static_cast<std::size_t>(token.text.data() - bytes.data()) + token.text.size() - 1u;
            bytes[pos] = bytes[pos] == 'A' ? 'B' : 'A';
            return true;
        }
        if (hatch && token.code == 92) inPath = true;
        if (hatch && token.code == 97) afterCount = true;
    }
    return false;
}

bool run(const std::filesystem::path& path, DRW::Version version, bool binary,
         dxfRW::DxfCompatibilityProfile profile, bool reverse) {
    dxfRW writer(path.string().c_str());
    const dxfRW::DxfObjectsWriteView outside{{}, version, binary, {}, profile};
    if (writer.inspectDxfHatchOutputLinks(outside, {})) return false;
    if (writer.inspectDxfHatchOutputIsolation(outside, {{1, 2, 3}}, false)) return false;
    writer.setDxfCompatibilityProfile(profile);
    writer.setReferenceResolver([&](std::uint32_t source) { return writer.remapEntityHandle(source); });
    Exporter adapter(writer, reverse);
    unsigned mode = 0;
    bool mutationApplied = false;
    if (!writer.setDxfEntitySectionsWriteInspector([&](const auto& view, std::string_view section) {
        std::vector<Links> links;
        if (section == "ENTITIES") links.push_back(adapter.links);
        if (section == "ENTITIES" && mode == 1) ++links.front().boundary;
        if (section == "ENTITIES" && mode == 2) ++links.front().hatch;
        if (section == "ENTITIES" && mode == 3) ++links.front().owner;
        if (section == "ENTITIES" && mode == 4) links.push_back(links.front());
        if (section == "ENTITIES" && mode == 5) links.front().owner = links.front().hatch;
        if (section == "ENTITIES" && mode >= 18 && mode <= 20) {
            auto altered = view;
            if (mode == 18) altered.version = DRW::AC1032;
            if (mode == 19) altered.binary = !binary;
            if (mode == 20) altered.compatibilityProfile = static_cast<dxfRW::DxfCompatibilityProfile>(255);
            mutationApplied = true;
            return writer.inspectDxfHatchOutputLinks(altered, links);
        }
        if (section == "ENTITIES" && mode == 28) {
            auto altered = view;
            altered.compatibilityProfile = profile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                ? dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy
                : dxfRW::DxfCompatibilityProfile::StandaloneSafe;
            mutationApplied = true;
            return writer.inspectDxfHatchOutputLinks(altered, links)
                || writer.inspectDxfHatchOutputIsolation(altered, links, true);
        }
        if (section == "ENTITIES" && mode >= 24 && mode <= 27) {
            mutationApplied = true;
            return writer.inspectDxfHatchOutputIsolation(view, links, true);
        }
        if (section == "ENTITIES" && mode >= 6) {
            std::string changed(view.bytes);
            const auto p = profile == dxfRW::DxfCompatibilityProfile::StandaloneSafe
                ? dx_raster_output::Profile::StandaloneSafe : dx_raster_output::Profile::LibreCadMasterLegacy;
            if (!mutate(changed, binary, p, mode)) return false;
            mutationApplied = true;
            auto altered = view;
            altered.bytes = changed;
            return writer.inspectDxfHatchOutputLinks(altered, links);
        }
        if (!writer.inspectDxfHatchOutputLinks(view, links)) return false;
        if (section == "BLOCKS") {
            const std::vector<Links> absent{{0xFFF001u, 0xFFF002u, 0xFFF003u}};
            return !writer.inspectDxfHatchOutputIsolation(view, absent, true)
                && writer.inspectDxfHatchOutputIsolation(view, absent, false);
        }
        if (mode == 0 && (writer.inspectDxfHatchOutputIsolation(view, links, false)
            || writer.inspectDxfHatchOutputIsolation(view, {links.front(), links.front()}, true))) return false;
        return writer.inspectDxfHatchOutputIsolation(view, links, true);
    })) return false;
    const std::string sentinel = "preserve destination";
    for (unsigned fault = 1; fault <= 28; ++fault) {
        if (fault == 27 && version < DRW::AC1018) continue; // true color is not written before R2004
        mode = fault;
        adapter.extraReference = fault >= 24 && fault <= 27 ? fault - 23u : 0u;
        mutationApplied = false;
        write(path, sentinel);
        if (writer.write(&adapter, version, binary) || read(path) != sentinel
            || (fault >= 6 && !mutationApplied)) {
            std::cerr << "fault " << fault << " was published\n";
            return false;
        }
        mode = 0;
        adapter.extraReference = 0u;
        if (!writer.write(&adapter, version, binary)) {
            std::cerr << "corrected retry failed after fault " << fault << '\n';
            return false;
        }
    }
    struct Reader : LC3CompatCheck {
        std::vector<DRW_Hatch> hatches;
        std::vector<DRW_LWPolyline> boundaries;
        void addHatch(const DRW_Hatch* hatch) override { if (hatch) hatches.push_back(*hatch); }
        void addLWPolyline(const DRW_LWPolyline& boundary) override { boundaries.push_back(boundary); }
    } readerAdapter;
    dxfRW reader(path.string().c_str());
    reader.setDxfCompatibilityProfile(profile);
    if (!reader.read(&readerAdapter, false) || readerAdapter.hatches.size() != 1u
        || readerAdapter.boundaries.size() != 1u) return false;
    const auto& hatch = readerAdapter.hatches.front();
    const auto& boundary = readerAdapter.boundaries.front();
    if (const char* prefix = std::getenv("LIBDXFRW_KEEP_HATCH_GRAPH_OUTPUT")) {
        const std::string copy = std::string(prefix) + ".v" + std::to_string(static_cast<int>(version))
            + (binary ? ".binary" : ".ascii") + ".p" + std::to_string(static_cast<int>(profile))
            + (reverse ? ".reverse" : ".rotated") + ".dxf";
        std::filesystem::copy_file(path, copy, std::filesystem::copy_options::overwrite_existing);
    }
    return hatch.associative == 1 && hatch.looplist.size() == 1u
        && hatch.looplist.front() && hatch.looplist.front()->m_boundaryHandles == std::vector<std::uint32_t>{boundary.handle}
        && boundary.reactorHandles == std::vector<std::uint32_t>{hatch.handle};
}
}

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path() / ("libdxfrw-hatch-graph-" + std::to_string(stamp));
    std::filesystem::create_directories(directory);
    const auto cleanup = [&] { std::filesystem::remove_all(directory); };
    for (const auto version : {DRW::AC1015, DRW::AC1024})
        for (const bool binary : {false, true})
            for (const auto profile : {dxfRW::DxfCompatibilityProfile::StandaloneSafe,
                                       dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy})
                for (const bool reverse : {false, true})
                    if (!run(directory / "hatch.dxf", version, binary, profile, reverse)) {
                        std::cerr << "staged HATCH graph failed version=" << static_cast<int>(version)
                                  << " binary=" << binary << " profile=" << static_cast<int>(profile)
                                  << " reverse=" << reverse << '\n';
                        cleanup(); return 1;
                    }
    cleanup();
    return 0;
}
