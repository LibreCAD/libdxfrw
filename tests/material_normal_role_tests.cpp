// SPDX-License-Identifier: GPL-2.0-or-later
// Distinguishing, locally-authored MATERIAL diffuse and normal map roles.
#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

namespace {

bool author(const std::filesystem::path& path, bool binary, bool boundary,
            bool partial, bool incompleteNormal = false,
            bool procedural = false, unsigned malformed = 0, int orphanProcedure = 0,
            int normalProjection = 4, bool sourceCommon = false, int normalTiling = 3) {
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
        && writer->writeString(0, "ENDSEC");
    if (sourceCommon) ok = ok && writer->writeString(0, "SECTION") && writer->writeString(2, "TABLES")
        && writer->writeString(0, "TABLE") && writer->writeString(2, "APPID")
        && writer->writeString(5, "A1") && writer->writeInt16(70, 1)
        && writer->writeString(0, "APPID") && writer->writeString(5, "A2") && writer->writeString(330, "A1")
        && writer->writeString(100, "AcDbSymbolTableRecord") && writer->writeString(100, "AcDbRegAppTableRecord")
        && writer->writeString(2, "REVIEW_APP") && writer->writeInt16(70, 0)
        && writer->writeString(0, "ENDTAB") && writer->writeString(0, "ENDSEC");
    ok = ok && writer->writeString(0, "SECTION")
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
        && writer->writeString(330, "1");
    if (sourceCommon) ok = ok && writer->writeString(102, "{REVIEW_APP")
        && writer->writeString(1, "original-application") && writer->writeString(102, "}");
    ok = ok && writer->writeString(100, "AcDbMaterial")
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
        && writer->writeInt16(73, normalProjection)
        && writer->writeInt16(74, normalTiling);
    if (!incompleteNormal && malformed != 2) ok = ok && writer->writeInt16(75, 4);
    if (orphanProcedure == 291) ok = ok && writer->writeBool(291, true);
    if (orphanProcedure == 301) ok = ok && writer->writeString(301, "orphan-procedure");
    if (orphanProcedure == 469) ok = ok && writer->writeDouble(469, 2.0);
    if (orphanProcedure == 62) ok = ok && writer->writeInt16(62, 7);
    if (orphanProcedure == 420) ok = ok && writer->writeInt32(420, 0x123456);
    if (orphanProcedure == 430) ok = ok && writer->writeString(430, "book$color");
    if (sourceCommon) ok = ok && writer->writeString(1001, "REVIEW_APP")
        && writer->writeString(1000, "original-xdata");
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

bool originalCommon(const DRW_Material& value) {
    if (value.appData.size() != 1u || value.extData.size() != 2u) return false;
    const auto& app = value.appData.front();
    if (app.size() != 3u) return false;
    auto field = app.begin();
    if (field->code() != 102 || !field->content.s || *field->content.s != "REVIEW_APP") return false;
    ++field;
    if (field->code() != 1 || !field->content.s || *field->content.s != "original-application") return false;
    ++field;
    if (field->code() != 102 || !field->content.s || *field->content.s != "}") return false;
    return value.extData[0] && value.extData[0]->code() == 1001 && value.extData[0]->content.s
        && *value.extData[0]->content.s == "REVIEW_APP" && value.extData[1]
        && value.extData[1]->code() == 1000 && value.extData[1]->content.s
        && *value.extData[1]->content.s == "original-xdata";
}

bool typed(const dx_data& data, bool boundary, double normalBlend = 0.75, int normalProjection = 4, int normalTiling = 3) {
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
        && m->m_normalMapProjection == normalProjection && m->m_normalMapTiling == normalTiling
        && m->m_normalMapAutoTransform == 4
        && m->m_hasNormalMapStrength == boundary
        && (!boundary || m->m_normalMapStrength == 0.625);
}

bool physical(const std::filesystem::path& path, bool binary, bool boundary,
              bool duplicateNormalBlend = false, double normalBlend = 0.75, int normalProjection = 4, int normalTiling = 3) {
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
        : std::vector<double>{0.25, normalBlend};
    return selected && blends == expectedBlends
        && sources == std::vector<int>{0, 1}
        && files == std::vector<std::string>{"diffuse-sentinel.png", "normal-sentinel.png"}
        && projections == std::vector<int>{2, normalProjection}
        && tilings == std::vector<int>{2, normalTiling}
        && transforms == std::vector<int>{2, 4}
        && strengths == (boundary ? 1u : 0u);
}

bool orphanField(const std::filesystem::path& path, bool binary, int wanted) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    bool material = false, selected = false, matched = false;
    unsigned count = 0; int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const auto type = reader->getString();
            if (material && selected) break;
            material = type == "MATERIAL"; selected = false;
        } else if (material && code == 1) selected = reader->getString() == "PeerMaterial";
        else if (material && selected && code == wanted) {
            ++count;
            if (code == 301) matched = reader->getString() == "orphan-procedure";
            else if (code == 430) matched = reader->getString() == "book$color";
            else if (code == 469) matched = reader->getDouble() == 2.0;
            else matched = reader->getInt32() == (code == 291 ? 1 : code == 62 ? 7 : 0x123456);
        }
    }
    return selected && count == 1u && matched;
}

struct LateMaterialEdit final : dx_iface {
    bool active = true, hit = false, projection = false, tiling = false; unsigned phase = 0;
    void mutate() {
        if (!active || !cData) return;
        for (auto& material : cData->materials) if (material.m_name == "PeerMaterial") {
            if (projection) material.m_normalMapProjection = 2;
            else if (tiling) material.m_normalMapTiling = 2;
            else material.m_normalMapBlendFactor = 0.9375;
            hit = true;
        }
    }
    void writeEntities() override { dx_iface::writeEntities(); if (phase == 0) mutate(); }
    void writeObjects() override {
        if (phase == 1) mutate();
        dx_iface::writeObjects();
        if (phase == 2) mutate();
    }
};

bool run(const std::filesystem::path& dir) {
    const auto input = dir / "input.dxf", output = dir / "output.dxf";
    for (bool sourceBinary : {false, true}) {
        for (bool outputBinary : {false, true}) for (unsigned mode = 0; mode != 4; ++mode) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4, true)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected || !originalCommon(*selected)) return false;
            if (mode == 1) selected->m_normalMapBlendFactor = 0.875;
            if (mode == 2) selected->m_normalMapProjection = 1;
            if (mode == 3) selected->m_normalMapTiling = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !originalCommon(*selected)
                || !physical(output, outputBinary, false, false, mode == 1 ? 0.875 : 0.75, mode == 2 ? 1 : 4, mode == 3 ? 1 : 3)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, mode == 1 ? 0.875 : 0.75, mode == 2 ? 1 : 4, mode == 3 ? 1 : 3)) return false;
            const auto* result = target(readback);
            if (!result || !originalCommon(*result)) return false;
        }
        // A live MATERIAL's common carrier must agree with its immutable raw
        // baseline even when its body is untouched or has a selected edit.
        // Otherwise replay would silently drop typed application data, XDATA,
        // or other common fields that the body-only comparison cannot see.
        for (bool outputBinary : {false, true}) for (unsigned mode = 0; mode != 4; ++mode)
            for (unsigned fault = 0; fault != 6; ++fault) {
                if (!author(input, sourceBinary, false, false)) return false;
                dx_data source; dx_iface adapter;
                if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
                DRW_Material* selected = nullptr;
                for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
                if (!selected) return false;
                if (mode == 1) selected->m_normalMapBlendFactor = 0.875;
                if (mode == 2) selected->m_normalMapProjection = 1;
                if (mode == 3) selected->m_normalMapTiling = 1;
                const DRW_Material corrected = *selected;
                if (fault == 0) selected->appData.push_back({DRW_Variant(102, std::string("REVIEW_APP")),
                    DRW_Variant(1, std::string("application-value")), DRW_Variant(102, std::string("}"))});
                if (fault == 1 && (!selected->addExtData(std::make_unique<DRW_Variant>(1001, std::string("REVIEW_APP")))
                    || !selected->addExtData(std::make_unique<DRW_Variant>(1000, std::string("xdata-value"))))) return false;
                if (fault == 2) selected->flags = 1;
                if (fault == 3) selected->name = "changed-base-name";
                if (fault == 4) selected->reactorHandles.push_back(0x1234u);
                if (fault == 5) selected->xDictHandle = 0x1234u;
                const DRW_Material caller = *selected;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) {
                    std::cerr << "MATERIAL common edit silently accepted mode=" << mode << " fault=" << fault << '\n'; return false;
                }
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination" || selected->flags != caller.flags || selected->name != caller.name
                    || selected->appData.size() != caller.appData.size() || selected->extData.size() != caller.extData.size()
                    || selected->reactorHandles != caller.reactorHandles || selected->xDictHandle != caller.xDictHandle
                    || selected->m_normalMapBlendFactor != caller.m_normalMapBlendFactor
                    || selected->m_normalMapProjection != caller.m_normalMapProjection
                    || selected->m_normalMapTiling != caller.m_normalMapTiling) return false;
                *selected = corrected;
                if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                    || !physical(output, outputBinary, false, false, mode == 1 ? 0.875 : 0.75, mode == 2 ? 1 : 4, mode == 3 ? 1 : 3)
                    || !typed(source, false, mode == 1 ? 0.875 : 0.75, mode == 2 ? 1 : 4, mode == 3 ? 1 : 3)) return false;
            }
        // These uniquely procedural host codes do not acquire a qualified
        // role merely because a malformed source omitted its 300/292 markers.
        // Unchanged replay stays available; the selected edit refuses and a
        // same-facade retry succeeds after restoring the original model.
        for (bool outputBinary : {false, true}) for (int code : {291, 301, 469, 62, 420, 430}) {
            if (!author(input, sourceBinary, false, false, false, false, 0, code)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected || !adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false) || !orphanField(output, outputBinary, code)) return false;
            selected->m_normalMapBlendFactor = 0.875;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) {
                std::cerr << "unqualified procedural code " << code << " accepted\n"; return false;
            }
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination"
                || !typed(source, false, 0.875)) return false;
            selected->m_normalMapBlendFactor = 0.75;
            selected->m_normalMapProjection = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
            std::ifstream projectionSaved(output, std::ios::binary);
            const std::string projectionBytes(std::istreambuf_iterator<char>(projectionSaved), {}); projectionSaved.close();
            if (projectionBytes != "keep destination" || !typed(source, false, 0.75, 1)) return false;
            selected->m_normalMapProjection = 4;
            selected->m_normalMapTiling = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
            std::ifstream tilingSaved(output, std::ios::binary);
            const std::string tilingBytes(std::istreambuf_iterator<char>(tilingSaved), {}); tilingSaved.close();
            if (tilingBytes != "keep destination" || !typed(source, false, 0.75, 4, 1)) return false;
            selected->m_normalMapTiling = 3;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false) || !orphanField(output, outputBinary, code)) return false;
        }
        // The normal tiling group 74 is distinct from the diffuse group 74.
        for (bool outputBinary : {false, true}) for (int from : {1, 3}) {
            const int to = from == 1 ? 3 : 1;
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4, false, from)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false, 0.75, 4, from)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") { if (selected) return false; selected = &material; }
            if (!selected) return false;
            selected->m_normalMapTiling = static_cast<std::uint8_t>(to);
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, to) || !typed(source, false, 0.75, 4, to)) {
                std::cerr << "unmarked normal tiling edit failed sourceBinary=" << sourceBinary
                          << " outputBinary=" << outputBinary << " from=" << from << '\n'; return false;
            }
            dx_data readback; dx_iface reader;
            if (!reader.fileImport(output.string(), &readback, false) || !typed(readback, false, 0.75, 4, to)) return false;
            for (unsigned fault = 0; fault != 6; ++fault) {
                auto& raw = source.rawMaterialObjects.front(); const DRW_RawDxfObject originalRaw = raw;
                if (fault == 0) selected->m_normalMapTiling = 0;
                if (fault == 1) selected->m_normalMapTiling = 4;
                if (fault == 2) selected->m_normalMapBlendFactor = 0.875;
                if (fault == 3) selected->m_normalMapProjection = 1;
                if (fault == 4) selected->m_diffuseMapTiling = 3;
                if (fault == 5) {
                    unsigned count = 0;
                    for (std::size_t i = 0; i < raw.groups.size(); ++i) if (raw.groups[i].code() == 74 && ++count == 2) {
                        raw.groups[i] = DRW_Variant(74, static_cast<std::int32_t>(2));
                        if (raw.hasRawValues) raw.rawValues.at(i) = "2";
                    }
                    if (count != 2) return false;
                }
                const auto failedTiling = selected->m_normalMapTiling;
                const auto failedDiffuse = selected->m_diffuseMapTiling;
                const auto failedProjection = selected->m_normalMapProjection;
                const auto failedBlend = selected->m_normalMapBlendFactor;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination" || selected->m_normalMapTiling != failedTiling
                    || selected->m_diffuseMapTiling != failedDiffuse || selected->m_normalMapProjection != failedProjection
                    || selected->m_normalMapBlendFactor != failedBlend) return false;
                raw = originalRaw; selected->m_normalMapTiling = static_cast<std::uint8_t>(to);
                selected->m_diffuseMapTiling = 2; selected->m_normalMapProjection = 4; selected->m_normalMapBlendFactor = 0.75;
                if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                    || !physical(output, outputBinary, false, false, 0.75, 4, to) || !typed(source, false, 0.75, 4, to)) return false;
            }
        }
        for (unsigned phase = 0; phase != 3; ++phase) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; LateMaterialEdit adapter; adapter.phase = phase; adapter.tiling = true;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapTiling = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true) || !adapter.hit) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active = false; selected->m_normalMapTiling = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true)
                || !physical(output, sourceBinary, false, false, 0.75, 4, 1) || !typed(source, false, 0.75, 4, 1)) return false;
        }
        // The first 73 is diffuse projection; only the later proven 73 may
        // carry an isolated normal projection edit. Exercise both legal
        // endpoints, all encodings, conflicting edits and retry.
        for (bool outputBinary : {false, true}) for (int from : {1, 4}) {
            const int to = from == 1 ? 4 : 1;
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, from)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false, 0.75, from)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") { if (selected) return false; selected = &material; }
            if (!selected) return false;
            selected->m_normalMapProjection = static_cast<std::uint8_t>(to);
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, to) || !typed(source, false, 0.75, to)) {
                std::cerr << "unmarked normal projection edit failed sourceBinary=" << sourceBinary
                          << " outputBinary=" << outputBinary << " from=" << from << '\n'; return false;
            }
            dx_data readback; dx_iface reader;
            if (!reader.fileImport(output.string(), &readback, false) || !typed(readback, false, 0.75, to)) return false;
            for (unsigned fault = 0; fault != 5; ++fault) {
                auto& raw = source.rawMaterialObjects.front(); const DRW_RawDxfObject originalRaw = raw;
                if (fault == 0) selected->m_normalMapProjection = 0;
                if (fault == 1) selected->m_normalMapProjection = 5;
                if (fault == 2) selected->m_normalMapBlendFactor = 0.875;
                if (fault == 3) selected->m_diffuseMapProjection = 3;
                if (fault == 4) {
                    unsigned count = 0;
                    for (std::size_t i = 0; i < raw.groups.size(); ++i) if (raw.groups[i].code() == 73 && ++count == 2) {
                        raw.groups[i] = DRW_Variant(73, static_cast<std::int32_t>(2));
                        if (raw.hasRawValues) raw.rawValues.at(i) = "2";
                    }
                    if (count != 2) return false;
                }
                const auto failedProjection = selected->m_normalMapProjection;
                const auto failedDiffuse = selected->m_diffuseMapProjection;
                const auto failedBlend = selected->m_normalMapBlendFactor;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination" || selected->m_normalMapProjection != failedProjection
                    || selected->m_diffuseMapProjection != failedDiffuse || selected->m_normalMapBlendFactor != failedBlend) return false;
                raw = originalRaw; selected->m_normalMapProjection = static_cast<std::uint8_t>(to);
                selected->m_diffuseMapProjection = 2; selected->m_normalMapBlendFactor = 0.75;
                if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                    || !physical(output, outputBinary, false, false, 0.75, to) || !typed(source, false, 0.75, to)) return false;
            }
        }
        for (unsigned phase = 0; phase != 3; ++phase) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; LateMaterialEdit adapter; adapter.phase = phase; adapter.projection = true;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapProjection = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true) || !adapter.hit) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active = false; selected->m_normalMapProjection = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true)
                || !physical(output, sourceBinary, false, false, 0.75, 1) || !typed(source, false, 0.75, 1)) return false;
        }
        // The complete unmarked source already has distinct typed diffuse and
        // normal roles. Edit only the second owning group 42, preserve the
        // first, and verify the actual output and readback in both encodings.
        for (bool outputBinary : {false, true}) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") { if (selected) return false; selected = &material; }
            if (!selected) return false;
            selected->m_normalMapBlendFactor = 0.875;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, 0.875) || !typed(source, false, 0.875)) {
                std::cerr << "unmarked normal-blend edit failed sourceBinary=" << sourceBinary
                          << " outputBinary=" << outputBinary << '\n'; return false;
            }
            dx_data readback; dx_iface reader;
            if (!reader.fileImport(output.string(), &readback, false) || !typed(readback, false, 0.875)) return false;
            for (unsigned fault = 0; fault != 3; ++fault) {
                // Additional typed edits, nonfinite values and conflicting raw
                // edits remain refused; correct and retry the same adapter.
                auto& raw = source.rawMaterialObjects.front();
                const DRW_RawDxfObject originalRaw = raw;
                if (fault == 0) selected->m_diffuseMapBlendFactor = 0.5;
                if (fault == 1) selected->m_normalMapBlendFactor = std::numeric_limits<double>::infinity();
                if (fault == 2) {
                    unsigned count = 0;
                    for (std::size_t i = 0; i < raw.groups.size(); ++i) if (raw.groups[i].code() == 42 && ++count == 2) {
                        raw.groups[i] = DRW_Variant(42, 0.9375);
                        if (raw.hasRawValues) raw.rawValues.at(i) = "0.9375";
                    }
                    if (count != 2) return false;
                }
                const double failedNormal = selected->m_normalMapBlendFactor;
                const double failedDiffuse = selected->m_diffuseMapBlendFactor;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination"
                    || selected->m_normalMapBlendFactor != failedNormal || selected->m_diffuseMapBlendFactor != failedDiffuse) return false;
                raw = originalRaw; selected->m_diffuseMapBlendFactor = 0.25; selected->m_normalMapBlendFactor = 0.875;
                if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                    || !physical(output, outputBinary, false, false, 0.875) || !typed(source, false, 0.875)) return false;
            }
        }
        for (unsigned phase = 0; phase != 3; ++phase) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; LateMaterialEdit adapter; adapter.phase = phase;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapBlendFactor = 0.875;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true) || !adapter.hit) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active = false; selected->m_normalMapBlendFactor = 0.875;
            if (!adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true)
                || !physical(output, sourceBinary, false, false, 0.875) || !typed(source, false, 0.875)) return false;
        }
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
