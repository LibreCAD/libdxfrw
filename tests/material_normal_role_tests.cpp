// SPDX-License-Identifier: GPL-2.0-or-later
// Distinguishing, locally-authored MATERIAL diffuse and normal map roles.
#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

bool author(const std::filesystem::path& path, bool binary, bool boundary,
            bool partial, bool incompleteNormal = false,
            bool procedural = false, unsigned malformed = 0) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) return false;
    if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0", 22);
    std::unique_ptr<dxfWriter> writer = binary
        ? std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
        : std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
    writer->setVersion("AC1027", true);
    bool ok = writer->writeString(0, "SECTION")
        && writer->writeString(2, "HEADER")
        && writer->writeString(9, "$ACADVER")
        && writer->writeString(1, "AC1027")
        && writer->writeString(0, "ENDSEC")
        && writer->writeString(0, "SECTION")
        && writer->writeString(2, "OBJECTS")
        && writer->writeString(0, "DICTIONARY")
        && writer->writeString(5, "C")
        && writer->writeString(330, "0")
        && writer->writeString(100, "AcDbDictionary")
        && writer->writeInt16(280, 1)
        && writer->writeInt16(281, 1)
        && writer->writeString(3, "ACAD_MATERIAL")
        && writer->writeString(350, "1")
        && writer->writeString(0, "DICTIONARY")
        && writer->writeString(5, "1")
        && writer->writeString(330, "C")
        && writer->writeString(100, "AcDbDictionary")
        && writer->writeInt16(280, 1)
        && writer->writeInt16(281, 1)
        && writer->writeString(3, "PeerMaterial")
        && writer->writeString(350, "2")
        && writer->writeString(0, "MATERIAL")
        && writer->writeString(5, "2")
        && writer->writeString(330, "1")
        && writer->writeString(100, "AcDbMaterial")
        && writer->writeString(1, "PeerMaterial")
        && writer->writeDouble(42, 0.25);
    // First refraction marker before the diffuse tuple is complete, followed
    // by a second marker after it: the later one must not requalify the tuple.
    if (malformed == 1) ok = ok && writer->writeDouble(145, 1.0);
    if (!partial) ok = ok && writer->writeInt16(72, 0);
    if (malformed == 3) ok = ok && writer->writeInt16(72, 1);
    ok = ok && writer->writeString(3, "diffuse-sentinel.png")
        && writer->writeInt16(73, 2)
        && writer->writeInt16(74, 2)
        && writer->writeInt16(75, 2)
        && writer->writeDouble(44, 0.5)
        && writer->writeInt16(76, 0)
        && writer->writeDouble(145, 1.0)
        && writer->writeDouble(146, 1.0)
        && writer->writeInt16(273, 1)
        && writer->writeInt16(274, 1)
        && writer->writeInt16(275, 1)
        && writer->writeInt16(276, 1);
    if (boundary) ok = ok && writer->writeDouble(465, 0.625);
    if (procedural) ok = ok && writer->writeString(300, "later-procedure");
    ok = ok && writer->writeDouble(42, 0.75);
    // Existing explicit-marker readers retain the last normal blend value.
    if (malformed == 5) ok = ok && writer->writeDouble(42, 0.875);
    // A late explicit boundary must not waive inferred-tuple completeness.
    if (malformed == 2) ok = ok && writer->writeDouble(465, 0.625);
    ok = ok && writer->writeInt16(72, 1);
    if (malformed == 4) ok = ok && writer->writeInt16(72, 0);
    ok = ok && writer->writeString(3, "normal-sentinel.png")
        && writer->writeInt16(73, 4)
        && writer->writeInt16(74, 3);
    if (!incompleteNormal && malformed != 2) ok = ok && writer->writeInt16(75, 4);
    ok = ok && writer->writeString(0, "ENDSEC")
        && writer->writeString(0, "SECTION")
        && writer->writeString(2, "ENTITIES")
        && writer->writeString(0, "LINE")
        && writer->writeString(5, "200")
        && writer->writeString(8, "0")
        && writer->writeDouble(10, 1.0)
        && writer->writeDouble(20, 2.0)
        && writer->writeDouble(11, 3.0)
        && writer->writeDouble(21, 4.0)
        && writer->writeString(347, "2")
        && writer->writeString(0, "ENDSEC")
        && writer->writeString(0, "EOF");
    stream.flush();
    return ok && !writer->hasWriteError() && stream.good();
}

const DRW_Material* target(const dx_data& data) {
    const DRW_Material* result = nullptr;
    for (const auto& m : data.materials) {
        if (m.m_name != "PeerMaterial") continue;
        if (result) return nullptr;
        result = &m;
    }
    return result;
}

bool typed(const dx_data& data, bool boundary, double normalBlend = 0.75) {
    const auto* m = target(data);
    return m && m->m_hasDiffuseMapBlendFactor && m->m_hasDiffuseMapSource
        && m->m_hasDiffuseMapFileName && m->m_hasDiffuseMapProjection
        && m->m_hasDiffuseMapTiling && m->m_hasDiffuseMapAutoTransform
        && m->m_diffuseMapBlendFactor == 0.25 && m->m_diffuseMapSource == 0
        && m->m_diffuseMapFileName == "diffuse-sentinel.png"
        && m->m_diffuseMapProjection == 2 && m->m_diffuseMapTiling == 2
        && m->m_diffuseMapAutoTransform == 2
        && m->m_hasNormalMapBlendFactor && m->m_hasNormalMapSource
        && m->m_hasNormalMapFileName && m->m_hasNormalMapProjection
        && m->m_hasNormalMapTiling && m->m_hasNormalMapAutoTransform
        && m->m_normalMapBlendFactor == normalBlend && m->m_normalMapSource == 1
        && m->m_normalMapFileName == "normal-sentinel.png"
        && m->m_normalMapProjection == 4 && m->m_normalMapTiling == 3
        && m->m_normalMapAutoTransform == 4
        && m->m_hasNormalMapStrength == boundary
        && (!boundary || m->m_normalMapStrength == 0.625);
}

bool physical(const std::filesystem::path& path, bool binary, bool boundary,
              bool duplicateNormalBlend = false) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    bool material = false, selected = false, eof = false;
    std::vector<double> blends;
    std::vector<int> sources, projections, tilings, transforms;
    std::vector<std::string> files;
    unsigned strengths = 0;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const auto type = reader->getString();
            if (material && selected) break;
            material = type == "MATERIAL";
            selected = false;
            eof = type == "EOF";
        } else if (material && code == 1) {
            selected = reader->getString() == "PeerMaterial";
        } else if (material && selected) {
            switch (code) {
            case 42: blends.push_back(reader->getDouble()); break;
            case 72: sources.push_back(reader->getInt32()); break;
            case 3: files.push_back(reader->getString()); break;
            case 73: projections.push_back(reader->getInt32()); break;
            case 74: tilings.push_back(reader->getInt32()); break;
            case 75: transforms.push_back(reader->getInt32()); break;
            case 465: ++strengths; break;
            default: break;
            }
        }
    }
    (void)eof;
    const std::vector<double> expectedBlends = duplicateNormalBlend
        ? std::vector<double>{0.25, 0.75, 0.875}
        : std::vector<double>{0.25, 0.75};
    return selected && blends == expectedBlends
        && sources == std::vector<int>{0, 1}
        && files == std::vector<std::string>{"diffuse-sentinel.png", "normal-sentinel.png"}
        && projections == std::vector<int>{2, 4}
        && tilings == std::vector<int>{2, 3}
        && transforms == std::vector<int>{2, 4}
        && strengths == (boundary ? 1u : 0u);
}

bool run(const std::filesystem::path& dir) {
    const auto input = dir / "input.dxf", output = dir / "output.dxf";
    for (bool sourceBinary : {false, true}) {
        if (!author(input, sourceBinary, true, false, false, false, 5)
            || !physical(input, sourceBinary, true, true)) return false;
        dx_data explicitSource; dx_iface explicitAdapter;
        if (!explicitAdapter.fileImport(input.string(), &explicitSource, false)
            || !typed(explicitSource, true, 0.875)) {
            std::cerr << "explicit normal-map duplicate blend behavior changed\n";
            return false;
        }
        for (bool outputBinary : {false, true}) {
            dx_data explicitReadback; dx_iface explicitReader;
            if (!explicitAdapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                            &explicitSource, false)
                || !physical(output, outputBinary, true, true)
                || !explicitReader.fileImport(output.string(), &explicitReadback, false)
                || !typed(explicitReadback, true, 0.875)
                || !typed(explicitSource, true, 0.875)) {
                std::cerr << "explicit normal-map duplicate output/readback changed\n";
                return false;
            }
        }
        for (bool boundary : {true, false}) {
            if (!author(input, sourceBinary, boundary, false)
                || !physical(input, sourceBinary, boundary)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, boundary)) {
                std::cerr << "typed source tuple mismatch, boundary=" << boundary << '\n';
                return false;
            }
            for (bool outputBinary : {false, true}) {
                dx_data again; dx_iface reader;
                if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false)
                    || !physical(output, outputBinary, boundary)
                    || !reader.fileImport(output.string(), &again, false) || !typed(again, boundary)
                    || !typed(source, boundary)) {
                    std::cerr << "output/readback mismatch, boundary=" << boundary
                              << " outputBinary=" << outputBinary << '\n';
                    return false;
                }
            }
        }
        if (!author(input, sourceBinary, false, true)) return false;
        dx_data rejected; dx_iface retry;
        if (retry.fileImport(input.string(), &rejected, false)) {
            std::cerr << "ambiguous partial diffuse tuple was accepted\n";
            return false;
        }
        if (!author(input, sourceBinary, false, false)) return false;
        dx_data corrected;
        if (!retry.fileImport(input.string(), &corrected, false) || !typed(corrected, false)) {
            std::cerr << "corrected read retry failed\n";
            return false;
        }
        for (unsigned mode = 0; mode != 2; ++mode) {
            if (!author(input, sourceBinary, false, false, mode == 0, mode == 1)) return false;
            dx_data bad; dx_iface sameAdapter;
            if (sameAdapter.fileImport(input.string(), &bad, false)) {
                std::cerr << (mode == 0 ? "incomplete inferred normal tuple was accepted\n"
                                       : "procedural-stage group 42 was interpreted as a normal map\n");
                return false;
            }
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data retryData;
            if (!sameAdapter.fileImport(input.string(), &retryData, false)
                || !typed(retryData, false)) {
                std::cerr << "inferred-boundary rejection retry failed\n";
                return false;
            }
        }
        for (unsigned malformed = 1; malformed <= 4; ++malformed) {
            if (!author(input, sourceBinary, false, false, false, false, malformed)) return false;
            dx_data bad; dx_iface sameAdapter;
            if (sameAdapter.fileImport(input.string(), &bad, false)) {
                std::cerr << "ambiguous duplicate or late boundary was accepted, case="
                          << malformed << '\n';
                return false;
            }
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data retryData;
            if (!sameAdapter.fileImport(input.string(), &retryData, false)
                || !typed(retryData, false)) {
                std::cerr << "ambiguous duplicate/boundary retry failed, case="
                          << malformed << '\n';
                return false;
            }
        }
    }
    return true;
}
} // namespace

int main() {
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-material-role-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(dir, error) || error) return 1;
    if (!run(dir)) { std::cerr << "Evidence: " << dir << '\n'; return 1; }
    std::filesystem::remove_all(dir, error);
    return 0;
}
