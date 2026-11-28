// SPDX-License-Identifier: GPL-2.0-or-later
// Distinguishing, locally-authored MATERIAL diffuse and normal map roles.
#include "dx_iface.h"
#include "intern/dxfreader.h"
#include "intern/dxfwriter.h"

#include <chrono>
#include <cmath>
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
            int normalProjection = 4, bool sourceCommon = false, int normalTiling = 3,
            unsigned matrixMode = 0u, int diffuseSource = 0, int normalSource = 1,
            bool methodBeforeStrength = false) {
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
    if (!partial) ok = ok && writer->writeInt16(72, diffuseSource);
    if (malformed == 3) ok = ok && writer->writeInt16(72, 1);
    ok = ok && writer->writeString(3, "diffuse-sentinel.png")
        && writer->writeInt16(73, 2)
        && writer->writeInt16(74, 2)
        && writer->writeInt16(75, 2);
    if(matrixMode) for(unsigned i=0u;i<(matrixMode==2u?15u:16u);++i)ok=ok&&writer->writeDouble(43,100.0+i);
    ok = ok && writer->writeDouble(44, 0.5)
        && writer->writeInt16(76, 0)
        && (!methodBeforeStrength || writer->writeInt16(271, 1))
        && writer->writeDouble(145, 1.0)
        && writer->writeDouble(146, 1.0)
        && writer->writeInt16(273, 1)
        && writer->writeInt16(274, 1)
        && writer->writeInt16(275, 1)
        && writer->writeInt16(276, 1);
    if (ok && methodBeforeStrength)
        ok = writer->writeInt16(271, 7);
    if (boundary) ok = ok && writer->writeDouble(465, 0.625);
    if (procedural) ok = ok && writer->writeString(300, "later-procedure");
    ok = ok && writer->writeDouble(42, 0.75);
    // Existing explicit-marker readers retain the last normal blend value.
    if (malformed == 5) ok = ok && writer->writeDouble(42, 0.875);
    // A late explicit boundary must not waive inferred-tuple completeness.
    if (malformed == 2) ok = ok && writer->writeDouble(465, 0.625);
    ok = ok && writer->writeInt16(72, normalSource);
    if (malformed == 4) ok = ok && writer->writeInt16(72, 0);
    ok = ok && writer->writeString(3, "normal-sentinel.png")
        && writer->writeInt16(73, normalProjection)
        && writer->writeInt16(74, normalTiling);
    if (!incompleteNormal && malformed != 2) ok = ok && writer->writeInt16(75, 4);
    if(matrixMode)for(unsigned i=0u;i<(matrixMode==3u?15u:matrixMode==4u?17u:16u);++i)ok=ok&&writer->writeDouble(43,200.0+i);
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

bool typed(const dx_data& data, bool boundary, double normalBlend = 0.75,
           int normalProjection = 4, int normalTiling = 3, int normalAuto = 4,
           const std::string& normalFile = "normal-sentinel.png", int normalSource = 1,
           const std::string& diffuseFile = "diffuse-sentinel.png", int diffuseSource = 0) {
    const auto* m = target(data);
    return m && m->m_hasDiffuseMapBlendFactor && m->m_hasDiffuseMapSource
        && m->m_hasDiffuseMapFileName && m->m_hasDiffuseMapProjection
        && m->m_hasDiffuseMapTiling && m->m_hasDiffuseMapAutoTransform
        && m->m_diffuseMapBlendFactor == 0.25 && m->m_diffuseMapSource == diffuseSource
        && m->m_diffuseMapFileName == diffuseFile
        && m->m_diffuseMapProjection == 2 && m->m_diffuseMapTiling == 2
        && m->m_diffuseMapAutoTransform == 2
        && m->m_hasNormalMapBlendFactor && m->m_hasNormalMapSource
        && m->m_hasNormalMapFileName && m->m_hasNormalMapProjection
        && m->m_hasNormalMapTiling && m->m_hasNormalMapAutoTransform
        && m->m_normalMapBlendFactor == normalBlend && m->m_normalMapSource == normalSource
        && m->m_normalMapFileName == normalFile
        && m->m_normalMapProjection == normalProjection && m->m_normalMapTiling == normalTiling
        && m->m_normalMapAutoTransform == normalAuto
        && m->m_hasNormalMapStrength == boundary
        && (!boundary || m->m_normalMapStrength == 0.625);
}

bool preStrengthMethodTyped(const dx_data& data, int method = 7,
                            double normalBlend = 0.75) {
    const auto* material = target(data);
    return typed(data, true, normalBlend) && material && material->m_hasBumpMapTiling
        && material->m_bumpMapTiling == 1 && material->m_hasNormalMapMethod
        && material->m_normalMapMethod == method;
}

bool preStrengthMethodPhysical(const std::filesystem::path& path, bool binary,
                               int method = 7, bool sourceOrder = false) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    reader->setVersion("AC1027", true);
    bool material = false, selected = false;
    std::vector<int> methods;
    unsigned strengths = 0, refractionMatrices = 0;
    std::size_t methodPosition = 0, strengthPosition = 0, position = 0;
    int code = 0;
    while (reader->readRec(&code)) {
        if (code == 0) {
            const auto type = reader->getString();
            if (material && selected) break;
            material = type == "MATERIAL";
            selected = false;
            continue;
        }
        if (!material || code == 0) continue;
        if (code == 1) {
            selected = reader->getUtf8String() == "PeerMaterial";
            continue;
        }
        if (!selected) continue;
        if (code == 271) {
            const int value = reader->getInt32();
            methods.emplace_back(value);
            if (value == method) methodPosition = position;
        } else if (code == 465) {
            ++strengths;
            strengthPosition = position;
        } else if (code == 147) {
            ++refractionMatrices;
        }
        ++position;
    }
    return selected && methods == std::vector<int>{1, method} && strengths == 1
        && refractionMatrices == 0
        && (!sourceOrder || methodPosition < strengthPosition);
}

bool physical(const std::filesystem::path& path, bool binary, bool boundary,
              bool duplicateNormalBlend = false, double normalBlend = 0.75,
              int normalProjection = 4, int normalTiling = 3, int normalAuto = 4,
              const std::string& normalFile = "normal-sentinel.png", int normalSource = 1,
              const std::string& diffuseFile = "diffuse-sentinel.png", int diffuseSource = 0) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) return false;
    if (binary) stream.seekg(22);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    reader->setVersion("AC1027",true);
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
            case 3: files.push_back(reader->getUtf8String()); break;
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
        && sources == std::vector<int>{diffuseSource, normalSource}
        && files == std::vector<std::string>{diffuseFile, normalFile}
        && projections == std::vector<int>{2, normalProjection}
        && tilings == std::vector<int>{2, normalTiling}
        && transforms == std::vector<int>{2, normalAuto}
        && strengths == (boundary ? 1u : 0u);
}

bool matrixState(const dx_data& data,std::size_t changed=16u,double value=0.0,const std::array<double,16>* expected=nullptr) {
    const auto* m=target(data);if(!m||!m->m_hasDiffuseMapTransform||!m->m_hasNormalMapTransform)return false;
    for(std::size_t i=0u;i<16u;++i) {
        const double normal=expected?(*expected)[i]:(i==changed?value:200.0+i);
        if(m->m_diffuseMapTransform[i]!=100.0+i||std::memcmp(&m->m_normalMapTransform[i],&normal,sizeof(double))!=0)return false;
    }
    return true;
}
bool physicalMatrices(const std::filesystem::path& path,bool binary,std::size_t changed=16u,double value=0.0,const std::array<double,16>* expected=nullptr) {
    std::ifstream stream(path,std::ios::binary);if(!stream)return false;if(binary)stream.seekg(22);
    std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        :std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    bool material=false,selected=false;int code=0;std::vector<double> values;
    while(reader->readRec(&code)) {
        if(code==0){const auto type=reader->getString();if(material&&selected)break;material=type=="MATERIAL";selected=false;}
        else if(material&&code==1)selected=reader->getString()=="PeerMaterial";
        else if(material&&selected&&code==43)values.push_back(reader->getDouble());
    }
    if(!selected||values.size()!=32u)return false;
    for(std::size_t i=0u;i<16u;++i) {
        const double normal=expected?(*expected)[i]:(i==changed?value:200.0+i);
        if(values[i]!=100.0+i||std::memcmp(&values[16u+i],&normal,sizeof(double))!=0)return false;
    }
    return true;
}
bool bothMatrices(const dx_data& data,const std::array<double,16>& diffuse,const std::array<double,16>& normal) {
    const auto* m=target(data);return m&&m->m_hasDiffuseMapTransform&&m->m_hasNormalMapTransform
        &&std::memcmp(m->m_diffuseMapTransform.data(),diffuse.data(),sizeof(diffuse))==0
        &&std::memcmp(m->m_normalMapTransform.data(),normal.data(),sizeof(normal))==0;
}
bool physicalBoth(const std::filesystem::path& path,bool binary,const std::array<double,16>& diffuse,const std::array<double,16>& normal) {
    std::ifstream stream(path,std::ios::binary);if(!stream)return false;if(binary)stream.seekg(22);
    std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        :std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    bool material=false,selected=false;int code=0;std::vector<double> values;
    while(reader->readRec(&code)) {
        if(code==0){const auto type=reader->getString();if(material&&selected)break;material=type=="MATERIAL";selected=false;}
        else if(material&&code==1)selected=reader->getString()=="PeerMaterial";
        else if(material&&selected&&code==43)values.push_back(reader->getDouble());
    }
    return selected&&values.size()==32u&&std::memcmp(values.data(),diffuse.data(),sizeof(diffuse))==0
        &&std::memcmp(values.data()+16u,normal.data(),sizeof(normal))==0;
}
bool physicalDiffuseRoles(const std::filesystem::path& path,bool binary,double blend,int projection,int tiling,int automatic,
        double normalBlend=.75,int normalProjection=4,int normalTiling=3,int normalAuto=4) {
    std::ifstream stream(path,std::ios::binary);if(!stream)return false;if(binary)stream.seekg(22);
    std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        :std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    bool material=false,selected=false;int code=0;std::vector<double> blends;std::vector<int> projections,tilings,automatics;
    while(reader->readRec(&code)) {
        if(code==0){const auto type=reader->getString();if(material&&selected)break;material=type=="MATERIAL";selected=false;}
        else if(material&&code==1)selected=reader->getString()=="PeerMaterial";
        else if(material&&selected&&code==42)blends.push_back(reader->getDouble());
        else if(material&&selected&&code==73)projections.push_back(reader->getInt32());
        else if(material&&selected&&code==74)tilings.push_back(reader->getInt32());
        else if(material&&selected&&code==75)automatics.push_back(reader->getInt32());
    }
    return selected&&blends==std::vector<double>{blend,normalBlend}&&projections==std::vector<int>{projection,normalProjection}
        &&tilings==std::vector<int>{tiling,normalTiling}&&automatics==std::vector<int>{automatic,normalAuto};
}
bool physicalNames(const std::filesystem::path& path,bool binary,const std::string& diffuse,const std::string& normal) {
    std::ifstream stream(path,std::ios::binary);if(!stream)return false;if(binary)stream.seekg(22);
    std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&stream))
        :std::unique_ptr<dxfReader>(new dxfReaderAscii(&stream));
    reader->setVersion("AC1027",true);
    bool material=false,selected=false;int code=0;std::vector<std::string> names;std::vector<int> sources;
    while(reader->readRec(&code)) {
        if(code==0){const auto type=reader->getString();if(material&&selected)break;material=type=="MATERIAL";selected=false;}
        else if(material&&code==1)selected=reader->getString()=="PeerMaterial";
        else if(material&&selected&&code==3)names.push_back(reader->getUtf8String());
        else if(material&&selected&&code==72)sources.push_back(reader->getInt32());
    }
    return selected&&names==std::vector<std::string>{diffuse,normal}&&sources==std::vector<int>{1,1};
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
    bool active = true, hit = false, projection = false, tiling = false, autoTransform = false,
        filename = false, matrix = false, diffuseMatrix = false, diffuseBlend = false,
        diffuseFilename = false, normalSource = false;
    int sourceValue = 1;
    unsigned phase = 0;
    void mutate() {
        if (!active || !cData) return;
        for (auto& material : cData->materials) if (material.m_name == "PeerMaterial") {
            if (diffuseFilename) material.m_diffuseMapFileName = "late-diffuse.png";
            else if (normalSource) material.m_normalMapSource = static_cast<std::uint8_t>(sourceValue);
            else if (diffuseMatrix) material.m_diffuseMapTransform[9] = 999.0;
            else if (diffuseBlend) material.m_diffuseMapBlendFactor = 0.9375;
            else if (matrix) material.m_normalMapTransform[9] = 999.0;
            else if (filename) material.m_normalMapFileName = "late-normal.png";
            else if (autoTransform) material.m_normalMapAutoTransform = 3;
            else if (projection) material.m_normalMapProjection = 2;
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
        // Group 72 is reused for diffuse and normal map sources. On this
        // inferred-boundary profile, edit only the normal occurrence and keep
        // its associated group-3 filename exactly bound to that role.
        for (bool outputBinary : {false, true}) for (int originalSource : {0, 1}) {
            const int wantedSource = 1 - originalSource;
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4,
                        false, 3, 0u, 0, originalSource)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false)
                || !typed(source, false, 0.75, 4, 3, 4, "normal-sentinel.png", originalSource)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") { if (selected) return false; selected = &material; }
            if (!selected) return false;
            const auto originalRaw = source.rawMaterialObjects.front();
            selected->m_normalMapSource = static_cast<std::uint8_t>(wantedSource);
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, 3, 4,
                             "normal-sentinel.png", wantedSource)
                || !typed(source, false, 0.75, 4, 3, 4, "normal-sentinel.png", wantedSource)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, 0.75, 4, 3, 4,
                          "normal-sentinel.png", wantedSource)) return false;

            auto& raw = source.rawMaterialObjects.front();
            unsigned normalSourceFields = 0u;
            for (std::size_t i = 0u; i < raw.groups.size(); ++i) {
                if (raw.groups[i].code() != 72 || ++normalSourceFields != 2u) continue;
                raw.groups[i] = DRW_Variant(72, std::int32_t(2));
                if (raw.hasRawValues) raw.rawValues.at(i) = "2";
                break;
            }
            if (normalSourceFields != 2u) return false;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true)) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination"
                || selected->m_normalMapSource != wantedSource
                || raw.groups.size() != originalRaw.groups.size()) return false;
            raw = originalRaw;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, 3, 4,
                             "normal-sentinel.png", wantedSource)) return false;
            dx_data retry; dx_iface retryVerifier;
            if (!retryVerifier.fileImport(output.string(), &retry, false)
                || !typed(retry, false, 0.75, 4, 3, 4,
                          "normal-sentinel.png", wantedSource)) return false;
        }
        // Qualify a composed source-selector and file-name edit on the same
        // inferred-boundary tuple. This is a transport/profile proof only;
        // the native meaning of selector 0/1 remains outside this test.
        for (bool outputBinary : {false, true}) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0,
                        4, false, 3, 0u, 0, 0)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false)
                || !typed(source, false, 0.75, 4, 3, 4,
                          "normal-sentinel.png", 0)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") {
                    if (selected) return false;
                    selected = &material;
                }
            if (!selected) return false;
            const auto originalRaw = source.rawMaterialObjects.front();
            const std::string wantedFile = "edited-normal-Ω.png";
            selected->m_normalMapSource = 1u;
            selected->m_normalMapFileName = wantedFile;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, 3,
                             4, wantedFile, 1)
                || !typed(source, false, 0.75, 4, 3, 4, wantedFile, 1)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, 0.75, 4, 3, 4, wantedFile, 1)) return false;

            // A raw source-selector contradiction still refuses atomically;
            // restoring the exact captured body permits same-adapter retry.
            auto& raw = source.rawMaterialObjects.front();
            unsigned selectorFields = 0u;
            for (std::size_t i = 0u; i < raw.groups.size(); ++i) {
                if (raw.groups[i].code() != 72 || ++selectorFields != 2u) continue;
                raw.groups[i] = DRW_Variant(72, std::int32_t(2));
                if (raw.hasRawValues) raw.rawValues.at(i) = "2";
                break;
            }
            if (selectorFields != 2u) return false;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc);
              sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true)) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {});
            saved.close();
            if (savedBytes != "keep destination"
                || selected->m_normalMapSource != 1u
                || selected->m_normalMapFileName != wantedFile) return false;
            raw = originalRaw;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, 3,
                             4, wantedFile, 1)) return false;
            dx_data retry; dx_iface retryVerifier;
            if (!retryVerifier.fileImport(output.string(), &retry, false)
                || !typed(retry, false, 0.75, 4, 3, 4, wantedFile, 1)) return false;
        }
        // A diffuse Scene-to-File transition is admitted only together with
        // its bound filename. Check the selector-only refusal, exact physical
        // fields, readback, raw contradiction, rollback, and same-adapter retry.
        for (bool outputBinary : {false, true}) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0,
                        4, false, 3, 0u, 0, 1)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false)
                || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") {
                    if (selected) return false;
                    selected = &material;
                }
            if (!selected) return false;
            const auto originalRaw = source.rawMaterialObjects.front();
            selected->m_diffuseMapSource = 1u;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc);
              sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true)) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {});
            saved.close();
            if (savedBytes != "keep destination"
                || selected->m_diffuseMapSource != 1u
                || selected->m_diffuseMapFileName != "diffuse-sentinel.png") return false;

            const std::string wantedFile = "edited-diffuse-Ω.png";
            selected->m_diffuseMapFileName = wantedFile;
            const auto exactOutput = [&]() {
                return physical(output, outputBinary, false, false, 0.75, 4, 3,
                                4, "normal-sentinel.png", 1, wantedFile, 1)
                    && typed(source, false, 0.75, 4, 3, 4,
                             "normal-sentinel.png", 1, wantedFile, 1);
            };
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true) || !exactOutput()) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, 0.75, 4, 3, 4,
                          "normal-sentinel.png", 1, wantedFile, 1)) return false;

            auto& raw = source.rawMaterialObjects.front();
            unsigned selectorFields = 0u;
            for (std::size_t i = 0u; i < raw.groups.size(); ++i) {
                if (raw.groups[i].code() != 72 || ++selectorFields != 1u) continue;
                raw.groups[i] = DRW_Variant(72, std::int32_t(2));
                if (raw.hasRawValues) raw.rawValues.at(i) = "2";
                break;
            }
            if (selectorFields != 1u) return false;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc);
              sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true)) return false;
            std::ifstream conflictSaved(output, std::ios::binary);
            const std::string conflictBytes(std::istreambuf_iterator<char>(conflictSaved), {});
            conflictSaved.close();
            if (conflictBytes != "keep destination"
                || selected->m_diffuseMapSource != 1u
                || selected->m_diffuseMapFileName != wantedFile) return false;
            raw = originalRaw;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true) || !exactOutput()) return false;
            dx_data retry; dx_iface retryVerifier;
            if (!retryVerifier.fileImport(output.string(), &retry, false)
                || !typed(retry, false, 0.75, 4, 3, 4,
                          "normal-sentinel.png", 1, wantedFile, 1)) return false;
        }
        // A callback that changes the edited source after planning must not
        // publish; restoring the requested source permits same-adapter retry.
        for (bool outputBinary : {false, true}) for (unsigned phase = 0u; phase != 3u; ++phase) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4,
                        false, 3, 0u, 0, 1)) return false;
            dx_data source; LateMaterialEdit adapter;
            adapter.phase = phase; adapter.normalSource = true; adapter.sourceValue = 1;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapSource = 0u;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true) || !adapter.hit
                || selected->m_normalMapSource != 1u) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active = false; selected->m_normalMapSource = 0u;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !physical(output, outputBinary, false, false, 0.75, 4, 3, 4,
                             "normal-sentinel.png", 0)) return false;
            dx_data retry; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &retry, false)
                || !typed(retry, false, 0.75, 4, 3, 4, "normal-sentinel.png", 0)) return false;
        }
        for (bool outputBinary : {false, true}) for (unsigned mode = 0; mode != 5; ++mode) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4, true)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected || !originalCommon(*selected)) return false;
            if (mode == 1 || mode == 4) selected->m_normalMapBlendFactor = 0.875;
            if (mode == 2 || mode == 4) selected->m_normalMapProjection = 1;
            if (mode == 3 || mode == 4) selected->m_normalMapTiling = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !originalCommon(*selected)
                || !physical(output, outputBinary, false, false, mode == 1 || mode == 4 ? 0.875 : 0.75, mode == 2 || mode == 4 ? 1 : 4, mode == 3 || mode == 4 ? 1 : 3)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, mode == 1 || mode == 4 ? 0.875 : 0.75, mode == 2 || mode == 4 ? 1 : 4, mode == 3 || mode == 4 ? 1 : 3)) return false;
            const auto* result = target(readback);
            if (!result || !originalCommon(*result)) return false;
        }
        // A live MATERIAL's common carrier must agree with its immutable raw
        // baseline even when its body is untouched or has a selected edit.
        // Otherwise replay would silently drop typed application data, XDATA,
        // or other common fields that the body-only comparison cannot see.
        for (bool outputBinary : {false, true}) for (unsigned mode = 0; mode != 5; ++mode)
            for (unsigned fault = 0; fault != 6; ++fault) {
                if (!author(input, sourceBinary, false, false)) return false;
                dx_data source; dx_iface adapter;
                if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
                DRW_Material* selected = nullptr;
                for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
                if (!selected) return false;
                if (mode == 1 || mode == 4) selected->m_normalMapBlendFactor = 0.875;
                if (mode == 2 || mode == 4) selected->m_normalMapProjection = 1;
                if (mode == 3 || mode == 4) selected->m_normalMapTiling = 1;
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
                    || !physical(output, outputBinary, false, false, mode == 1 || mode == 4 ? 0.875 : 0.75, mode == 2 || mode == 4 ? 1 : 4, mode == 3 || mode == 4 ? 1 : 3)
                    || !typed(source, false, mode == 1 || mode == 4 ? 0.875 : 0.75, mode == 2 || mode == 4 ? 1 : 4, mode == 3 || mode == 4 ? 1 : 3)) return false;
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
            selected->m_normalMapBlendFactor = 0.875; selected->m_normalMapProjection = 1; selected->m_normalMapTiling = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
            std::ifstream composedSaved(output, std::ios::binary);
            const std::string composedBytes(std::istreambuf_iterator<char>(composedSaved), {}); composedSaved.close();
            if (composedBytes != "keep destination" || !typed(source, false, 0.875, 1, 1)) return false;
            selected->m_normalMapBlendFactor = 0.75; selected->m_normalMapProjection = 4; selected->m_normalMapTiling = 3;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false) || !orphanField(output, outputBinary, code)) return false;
        }
        // The first 3 belongs to file-backed diffuse and the second to normal.
        // Both export policies use strict encoding of the changed typed value.
        for(bool outputBinary:{false,true})for(auto policy:{DRW::DxfTextWritePolicy::Compatible,DRW::DxfTextWritePolicy::RequireLossless})
            for(const auto& wanted:std::vector<std::string>{"edited-diffuse.png","edited-diffuse-Ω.png","escaped^A.png","literal-\\U+1234.png",""}) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m||m->m_diffuseMapSource!=1u)return false;
            m->m_diffuseMapFileName=wanted;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true,dx_dxfSourceTextPolicy::Compatible,policy))return false;
            if(!physicalNames(output,outputBinary,wanted,"normal-sentinel.png"))return false;
            dx_data readback;dx_iface verifier;if(!verifier.fileImport(output.string(),&readback,false))return false;
            const auto* value=target(readback);if(!value||value->m_diffuseMapFileName!=wanted||value->m_normalMapFileName!="normal-sentinel.png"
                ||value->m_diffuseMapSource!=1u||value->m_normalMapSource!=1u)return false;
        }
        for(bool outputBinary:{false,true}) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;
            m->m_diffuseMapFileName="diffuse-Ω.png";m->m_normalMapFileName="normal-Ω.png";
            m->m_diffuseMapBlendFactor=.5;m->m_normalMapBlendFactor=.875;m->m_diffuseMapProjection=1;m->m_normalMapProjection=1;
            m->m_diffuseMapTiling=1;m->m_normalMapTiling=1;m->m_diffuseMapAutoTransform=1;m->m_normalMapAutoTransform=1;
            m->m_diffuseMapTransform[9]=777.;m->m_normalMapTransform[9]=888.;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalNames(output,outputBinary,"diffuse-Ω.png","normal-Ω.png"))return false;
            dx_data readback;dx_iface verifier;if(!verifier.fileImport(output.string(),&readback,false))return false;
            const auto* value=target(readback);if(!value||value->m_diffuseMapTransform[9]!=777.||value->m_normalMapTransform[9]!=888.
                ||value->m_diffuseMapBlendFactor!=.5||value->m_normalMapBlendFactor!=.875)return false;
        }
        for(bool outputBinary:{false,true})for(unsigned fault=0;fault<9u;++fault) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;
            auto& raw=source.rawMaterialObjects.front();const auto savedRaw=raw;m->m_diffuseMapFileName="edited-diffuse.png";
            if(fault==0u)m->m_diffuseMapFileName=std::string("nul\0name",8u);
            if(fault==1u)m->m_diffuseMapFileName=std::string("invalid\x80",8u);
            if(fault==2u)m->m_diffuseMapFileName.assign(2u*1024u*1024u,'x');
            if(fault==3u)m->m_diffuseMapSource=0u;if(fault==4u)m->m_hasDiffuseMapFileName=false;
            if(fault==5u)m->m_hasDiffuseMapSource=false;if(fault==6u)m->m_normalMapSource=0u;
            if(fault>=7u){unsigned count=0u;for(std::size_t i=0;i<raw.groups.size();++i)if(raw.groups[i].code()==3&&++count==(fault==7u?1u:2u)){
                raw.groups[i]=DRW_Variant(3,UTF8STRING("conflicting.png"));if(raw.hasRawValues)raw.rawValues[i]="conflicting.png";}}
            const auto failedName=m->m_diffuseMapFileName;const auto failedSource=m->m_diffuseMapSource;
            const bool failedNamePresence=m->m_hasDiffuseMapFileName,failedSourcePresence=m->m_hasDiffuseMapSource;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||m->m_diffuseMapFileName!=failedName||m->m_diffuseMapSource!=failedSource
                ||m->m_hasDiffuseMapFileName!=failedNamePresence||m->m_hasDiffuseMapSource!=failedSourcePresence)return false;
            raw=savedRaw;m->m_diffuseMapFileName="edited-diffuse.png";m->m_diffuseMapSource=1u;m->m_hasDiffuseMapFileName=true;m->m_hasDiffuseMapSource=true;m->m_normalMapSource=1u;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)||!physicalNames(output,outputBinary,"edited-diffuse.png","normal-sentinel.png"))return false;
        }
        // The same filename edit cannot infer inactive scene-source semantics.
        for(bool outputBinary:{false,true}) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1,0))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m||m->m_diffuseMapSource!=0u)return false;
            m->m_diffuseMapFileName="edited-diffuse.png";
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||m->m_diffuseMapFileName!="edited-diffuse.png")return false;
            m->m_diffuseMapFileName="diffuse-sentinel.png";
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
        }
        for(unsigned phase=0;phase<3u;++phase) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1,1))return false;
            dx_data source;LateMaterialEdit adapter;adapter.phase=phase;adapter.diffuseFilename=true;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;m->m_diffuseMapFileName="edited-diffuse.png";
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!adapter.hit)return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||m->m_diffuseMapFileName!="late-diffuse.png")return false;adapter.active=false;m->m_diffuseMapFileName="edited-diffuse.png";
            if(!adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!physicalNames(output,sourceBinary,"edited-diffuse.png","normal-sentinel.png"))return false;
        }
        // Complete diffuse mapper scalars use the first of each repeated role.
        // Every subset composes with a simultaneous qualified normal edit.
        for(bool outputBinary:{false,true})for(unsigned mask=1u;mask<16u;++mask) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;
            if(mask&1u)m->m_diffuseMapBlendFactor=.5;if(mask&2u)m->m_diffuseMapProjection=1;
            if(mask&4u)m->m_diffuseMapTiling=1;if(mask&8u)m->m_diffuseMapAutoTransform=1;
            m->m_normalMapBlendFactor=.875;m->m_normalMapProjection=1;m->m_normalMapTiling=1;m->m_normalMapAutoTransform=1;
            m->m_diffuseMapTransform[9]=777.;m->m_normalMapTransform[9]=888.;
            const double blend=mask&1u?.5:.25;const int projection=mask&2u?1:2,tiling=mask&4u?1:2,automatic=mask&8u?1:2;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalDiffuseRoles(output,outputBinary,blend,projection,tiling,automatic,.875,1,1,1))return false;
            dx_data readback;dx_iface verifier;if(!verifier.fileImport(output.string(),&readback,false))return false;
            const auto* value=target(readback);if(!value||value->m_diffuseMapBlendFactor!=blend||value->m_diffuseMapProjection!=projection
                ||value->m_diffuseMapTiling!=tiling||value->m_diffuseMapAutoTransform!=automatic
                ||value->m_diffuseMapTransform[9]!=777.||value->m_normalMapTransform[9]!=888.)return false;
        }
        for(bool outputBinary:{false,true})for(unsigned role=0u;role<3u;++role)for(unsigned wanted=role==2u?0u:1u;wanted<=(role==0u?4u:role==1u?3u:7u);++wanted) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;
            if(role==0u)m->m_diffuseMapProjection=static_cast<std::uint8_t>(wanted);
            else if(role==1u)m->m_diffuseMapTiling=static_cast<std::uint8_t>(wanted);
            else m->m_diffuseMapAutoTransform=static_cast<std::uint8_t>(wanted);
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalDiffuseRoles(output,outputBinary,.25,role==0u?wanted:2,role==1u?wanted:2,role==2u?wanted:2))return false;
        }
        for(bool outputBinary:{false,true})for(unsigned fault=0u;fault<16u;++fault) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;
            auto& raw=source.rawMaterialObjects.front();const auto savedRaw=raw;
            m->m_diffuseMapBlendFactor=.5;m->m_diffuseMapProjection=1;m->m_diffuseMapTiling=1;m->m_diffuseMapAutoTransform=1;
            if(fault==0u)m->m_diffuseMapBlendFactor=(std::numeric_limits<double>::infinity)();
            if(fault==1u)m->m_diffuseMapBlendFactor=(std::numeric_limits<double>::quiet_NaN)();
            if(fault==2u)m->m_diffuseMapProjection=0u;if(fault==3u)m->m_diffuseMapProjection=5u;
            if(fault==4u)m->m_diffuseMapTiling=0u;if(fault==5u)m->m_diffuseMapTiling=4u;
            if(fault==6u)m->m_diffuseMapAutoTransform=8u;
            if(fault==7u)m->m_hasDiffuseMapProjection=false;
            if(fault>=8u) {const int field=fault<10u?42:fault<12u?73:fault<14u?74:75;const unsigned wanted=(fault%2u)?2u:1u;unsigned count=0;
                for(std::size_t i=0;i<raw.groups.size();++i)if(raw.groups[i].code()==field&&++count==wanted){
                    raw.groups[i]=field==42?DRW_Variant(42,.125):DRW_Variant(field,static_cast<std::int32_t>(5));
                    if(raw.hasRawValues)raw.rawValues[i]=field==42?"0.125":"5";break;}}
            const auto failed=*m;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||std::memcmp(&m->m_diffuseMapBlendFactor,&failed.m_diffuseMapBlendFactor,sizeof(double))
                ||m->m_diffuseMapProjection!=failed.m_diffuseMapProjection||m->m_diffuseMapTiling!=failed.m_diffuseMapTiling
                ||m->m_diffuseMapAutoTransform!=failed.m_diffuseMapAutoTransform||m->m_hasDiffuseMapProjection!=failed.m_hasDiffuseMapProjection)return false;
            raw=savedRaw;m->m_diffuseMapBlendFactor=.5;m->m_diffuseMapProjection=1;m->m_diffuseMapTiling=1;m->m_diffuseMapAutoTransform=1;m->m_hasDiffuseMapProjection=true;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)||!physicalDiffuseRoles(output,outputBinary,.5,1,1,1))return false;
        }
        for(unsigned phase=0;phase<3u;++phase) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;LateMaterialEdit adapter;adapter.phase=phase;adapter.diffuseBlend=true;
            if(!adapter.fileImport(input.string(),&source,false))return false;
            DRW_Material* m=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")m=&item;if(!m)return false;m->m_diffuseMapBlendFactor=.5;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!adapter.hit)return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||m->m_diffuseMapBlendFactor!=.9375)return false;
            adapter.active=false;m->m_diffuseMapBlendFactor=.5;
            if(!adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!physicalDiffuseRoles(output,sourceBinary,.5,2,2,2))return false;
        }
        // Both matrices have distinct source roles. Test individual, multiple,
        // signed-zero and composed diffuse/normal edits on all wire routes.
        for(bool outputBinary:{false,true})for(unsigned pattern=0;pattern<20u;++pattern) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;auto diffuse=selected->m_diffuseMapTransform,normal=selected->m_normalMapTransform;
            if(pattern<16u)diffuse[pattern]=700.0+pattern;
            else if(pattern==16u)for(std::size_t i=0;i<16u;++i)diffuse[i]=-500.0-i;
            else if(pattern==17u){diffuse[0]=11.0;diffuse[5]=22.0;diffuse[15]=33.0;}
            else if(pattern==18u)diffuse[9]=-0.0;
            else {diffuse[9]=777.0;normal[9]=888.0;selected->m_normalMapBlendFactor=.875;
                selected->m_normalMapProjection=1;selected->m_normalMapTiling=1;selected->m_normalMapAutoTransform=1;
                selected->m_normalMapFileName="edited-normal.png";}
            selected->m_diffuseMapTransform=diffuse;selected->m_normalMapTransform=normal;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalBoth(output,outputBinary,diffuse,normal)||!bothMatrices(source,diffuse,normal))return false;
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!bothMatrices(readback,diffuse,normal))return false;
        }
        for(bool outputBinary:{false,true})for(unsigned fault=0;fault<7u;++fault) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;auto& raw=source.rawMaterialObjects.front();const auto savedRaw=raw;
            selected->m_diffuseMapTransform[9]=777.0;
            if(fault==0u)selected->m_diffuseMapTransform[9]=(std::numeric_limits<double>::quiet_NaN)();
            if(fault==1u)selected->m_diffuseMapTransform[4]=(std::numeric_limits<double>::infinity)();
            if(fault==2u)selected->m_normalMapTransform[4]=(std::numeric_limits<double>::infinity)();
            if(fault==3u)selected->m_hasDiffuseMapTransform=false;
            if(fault==4u)selected->m_hasNormalMapTransform=false;
            if(fault>=5u){unsigned count=0u;for(std::size_t i=0u;i<raw.groups.size();++i)if(raw.groups[i].code()==43&&++count==(fault==5u?10u:26u)){
                raw.groups[i]=DRW_Variant(43,888.0);if(raw.hasRawValues)raw.rawValues.at(i)="888";}}
            const auto failedDiffuse=selected->m_diffuseMapTransform,failedNormal=selected->m_normalMapTransform;
            const bool failedDiffusePresent=selected->m_hasDiffuseMapTransform,failedNormalPresent=selected->m_hasNormalMapTransform;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||std::memcmp(failedDiffuse.data(),selected->m_diffuseMapTransform.data(),sizeof(failedDiffuse))
                ||std::memcmp(failedNormal.data(),selected->m_normalMapTransform.data(),sizeof(failedNormal))
                ||failedDiffusePresent!=selected->m_hasDiffuseMapTransform||failedNormalPresent!=selected->m_hasNormalMapTransform)return false;
            raw=savedRaw;for(std::size_t i=0;i<16u;++i){selected->m_diffuseMapTransform[i]=100.0+i;selected->m_normalMapTransform[i]=200.0+i;}
            selected->m_diffuseMapTransform[9]=777.0;selected->m_hasDiffuseMapTransform=true;selected->m_hasNormalMapTransform=true;
            const auto diffuse=selected->m_diffuseMapTransform,normal=selected->m_normalMapTransform;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)||!physicalBoth(output,outputBinary,diffuse,normal))return false;
        }
        for(unsigned phase=0;phase<3u;++phase) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;LateMaterialEdit adapter;adapter.phase=phase;adapter.diffuseMatrix=true;
            if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;selected->m_diffuseMapTransform[9]=777.0;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!adapter.hit)return false;
            std::ifstream saved(output,std::ios::binary);const std::string bytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(bytes!="keep destination"||selected->m_diffuseMapTransform[9]!=999.0)return false;
            adapter.active=false;selected->m_diffuseMapTransform[9]=777.0;
            const auto diffuse=selected->m_diffuseMapTransform,normal=selected->m_normalMapTransform;
            if(!adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!physicalBoth(output,sourceBinary,diffuse,normal))return false;
        }
        // Two complete mapper matrices occur on opposite sides of the
        // inferred second 42. An edit replaces only the selected normal slot.
        for(bool outputBinary:{false,true})for(std::size_t index=0u;index<16u;++index) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;const double wanted=700.0+index;selected->m_normalMapTransform[index]=wanted;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalMatrices(output,outputBinary,index,wanted)||!matrixState(source,index,wanted))return false;
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!matrixState(readback,index,wanted))return false;
        }
        // Multiple matrix slots and signed zero retain their exact represented
        // values, including a simultaneous edit of all sixteen slots.
        for(bool outputBinary:{false,true})for(unsigned pattern=0u;pattern<3u;++pattern) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;auto wanted=selected->m_normalMapTransform;
            for(std::size_t i=0u;i<16u;++i)if(pattern==0u||(pattern==1u&&(i==0u||i==5u||i==15u)))wanted[i]=-500.0-i;
            if(pattern==2u)wanted[9]=-0.0;selected->m_normalMapTransform=wanted;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalMatrices(output,outputBinary,16u,0.0,&wanted)||!matrixState(source,16u,0.0,&wanted))return false;
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!matrixState(readback,16u,0.0,&wanted))return false;
        }
        // Matrix edits compose with every subset of the five previously
        // qualified unmarked roles; retained common application data stays.
        for(bool outputBinary:{false,true})for(unsigned mask=0u;mask<32u;++mask) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,true,3,1))return false;
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected||!originalCommon(*selected))return false;
            selected->m_normalMapTransform[9]=777.0;
            if(mask&1u)selected->m_normalMapBlendFactor=.875;
            if(mask&2u)selected->m_normalMapProjection=1;
            if(mask&4u)selected->m_normalMapTiling=1;
            if(mask&8u)selected->m_normalMapAutoTransform=1;
            if(mask&16u)selected->m_normalMapFileName="edited-normal-Ω.png";
            const double blend=mask&1u?.875:.75;const int projection=mask&2u?1:4,tiling=mask&4u?1:3,automatic=mask&8u?1:4;
            const std::string filename=mask&16u?"edited-normal-Ω.png":"normal-sentinel.png";
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physical(output,outputBinary,false,false,blend,projection,tiling,automatic,filename)
                ||!physicalMatrices(output,outputBinary,9u,777.0)||!matrixState(source,9u,777.0)||!originalCommon(*selected))return false;
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!typed(readback,false,blend,projection,tiling,automatic,filename)
                ||!matrixState(readback,9u,777.0)||!originalCommon(*target(readback)))return false;
        }
        for(bool outputBinary:{false,true})for(unsigned fault=0u;fault<8u;++fault) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;auto& raw=source.rawMaterialObjects.front();const auto savedRaw=raw;
            selected->m_normalMapTransform[9]=777.0;
            if(fault==0u)selected->m_normalMapTransform[9]=(std::numeric_limits<double>::quiet_NaN)();
            if(fault==1u)selected->m_normalMapTransform[4]=(std::numeric_limits<double>::infinity)();
            if(fault==2u)selected->m_diffuseMapTransform[9]=(std::numeric_limits<double>::infinity)();
            if(fault==3u)selected->m_hasNormalMapTransform=false;
            if(fault==4u||fault==5u){unsigned count=0u;for(std::size_t i=0u;i<raw.groups.size();++i)if(raw.groups[i].code()==43&&++count==(fault==4u?26u:10u)){
                raw.groups[i]=DRW_Variant(43,888.0);if(raw.hasRawValues)raw.rawValues.at(i)="888";}}
            if(fault==6u)selected->m_normalMapSource=0u;
            if(fault==7u){unsigned count=0u;for(std::size_t i=0u;i<raw.groups.size();++i)if(raw.groups[i].code()==43&&++count==32u){
                raw.groups.insert(raw.groups.begin()+static_cast<std::ptrdiff_t>(i+1u),DRW_Variant(43,888.0));
                if(raw.hasRawValues)raw.rawValues.insert(raw.rawValues.begin()+static_cast<std::ptrdiff_t>(i+1u),"888");break;}}
            const auto failedNormal=selected->m_normalMapTransform,failedDiffuse=selected->m_diffuseMapTransform;
            const auto failedPresent=selected->m_hasNormalMapTransform;const auto failedSource=selected->m_normalMapSource;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return false;
            std::ifstream saved(output,std::ios::binary);const std::string savedBytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(savedBytes!="keep destination"||std::memcmp(selected->m_normalMapTransform.data(),failedNormal.data(),sizeof(failedNormal))!=0
                ||std::memcmp(selected->m_diffuseMapTransform.data(),failedDiffuse.data(),sizeof(failedDiffuse))!=0
                ||selected->m_hasNormalMapTransform!=failedPresent||selected->m_normalMapSource!=failedSource)return false;
            raw=savedRaw;for(std::size_t i=0u;i<16u;++i){selected->m_normalMapTransform[i]=200.0+i;selected->m_diffuseMapTransform[i]=100.0+i;}
            selected->m_normalMapTransform[9]=777.0;selected->m_hasNormalMapTransform=true;selected->m_normalMapSource=1u;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physicalMatrices(output,outputBinary,9u,777.0)||!matrixState(source,9u,777.0))return false;
        }
        for(unsigned phase=0u;phase<3u;++phase) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data source;LateMaterialEdit adapter;adapter.phase=phase;adapter.matrix=true;
            if(!adapter.fileImport(input.string(),&source,false)||!matrixState(source))return false;
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return false;selected->m_normalMapTransform[9]=777.0;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!adapter.hit)return false;
            std::ifstream saved(output,std::ios::binary);const std::string savedBytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(savedBytes!="keep destination")return false;adapter.active=false;selected->m_normalMapTransform[9]=777.0;
            if(!adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)
                ||!physicalMatrices(output,sourceBinary,9u,777.0)||!matrixState(source,9u,777.0))return false;
        }
        for(unsigned malformedMatrix=2u;malformedMatrix<=4u;++malformedMatrix) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,malformedMatrix))return false;
            dx_data rejected;dx_iface adapter;if(adapter.fileImport(input.string(),&rejected,false))return false;
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,false,3,1))return false;
            dx_data corrected;if(!adapter.fileImport(input.string(),&corrected,false)||!matrixState(corrected))return false;
        }
        // The two 3 fields belong to diffuse and file-backed normal map
        // roles; preserve the first while editing the second with the target
        // encoding's strict UTF-8/caret conversion.
        for (bool outputBinary : {false,true}) for (const auto& wanted : std::vector<std::string>{
                "edited-normal.png", "edited-normal-Ω.png", "escaped^A-normal.png", "literal-\\U+1234.png", ""}) {
            if (!author(input,sourceBinary,false,false)) return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            dx_data source; dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);selected->m_normalMapFileName=wanted;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physical(output,outputBinary,false,false,.75,4,3,4,wanted)
                ||!typed(source,false,.75,4,3,4,wanted))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!typed(readback,false,.75,4,3,4,wanted))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
        }
        for(bool outputBinary:{false,true}) {
            if(!author(input,sourceBinary,false,false,false,false,0,0,4,true))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected||!originalCommon(*selected))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            const std::string wanted="edited-normal-Ω^A.png";selected->m_normalMapFileName=wanted;
            selected->m_normalMapBlendFactor=.875;selected->m_normalMapProjection=1;selected->m_normalMapTiling=1;selected->m_normalMapAutoTransform=1;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physical(output,outputBinary,false,false,.875,1,1,1,wanted)||!typed(source,false,.875,1,1,1,wanted)
                ||!originalCommon(*selected))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            dx_data readback;dx_iface verifier;
            if(!verifier.fileImport(output.string(),&readback,false)||!typed(readback,false,.875,1,1,1,wanted)
                ||!originalCommon(*target(readback)))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
        }
        for(bool outputBinary:{false,true})for(unsigned fault=0u;fault!=7u;++fault) {
            if(!author(input,sourceBinary,false,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);auto& raw=source.rawMaterialObjects.front();const auto originalRaw=raw;
            const std::string wanted="edited-normal-Ω.png";selected->m_normalMapFileName=wanted;
            if(fault==0u)selected->m_normalMapFileName=std::string("nul\0name",8u);
            if(fault==1u)selected->m_normalMapFileName=std::string("invalid\x80",8u);
            if(fault==2u)selected->m_normalMapFileName.assign(2u*1024u*1024u,'x');
            if(fault==3u)selected->m_diffuseMapFileName="different-diffuse.png";
            if(fault==4u)selected->m_normalMapSource=0u;
            if(fault==5u)selected->m_hasNormalMapFileName=false;
            if(fault==6u){unsigned count=0u;for(std::size_t i=0u;i<raw.groups.size();++i)if(raw.groups[i].code()==3&&++count==2u){
                    raw.groups[i]=DRW_Variant(3,UTF8STRING("conflicting-normal.png"));if(raw.hasRawValues)raw.rawValues.at(i)="conflicting-normal.png";}
                if(count!=2u)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);}
            const auto failedName=selected->m_normalMapFileName,failedDiffuse=selected->m_diffuseMapFileName;
            const auto failedSource=selected->m_normalMapSource;const auto failedPresence=selected->m_hasNormalMapFileName;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            std::ifstream saved(output,std::ios::binary);const std::string savedBytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(savedBytes!="keep destination"||selected->m_normalMapFileName!=failedName||selected->m_diffuseMapFileName!=failedDiffuse
                ||selected->m_normalMapSource!=failedSource||selected->m_hasNormalMapFileName!=failedPresence)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            raw=originalRaw;selected->m_normalMapFileName=wanted;selected->m_diffuseMapFileName="diffuse-sentinel.png";
            selected->m_normalMapSource=1u;selected->m_hasNormalMapFileName=true;
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physical(output,outputBinary,false,false,.75,4,3,4,wanted)||!typed(source,false,.75,4,3,4,wanted))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
        }
        for(unsigned phase=0u;phase!=3u;++phase) {
            if(!author(input,sourceBinary,false,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);dx_data source;LateMaterialEdit adapter;adapter.phase=phase;adapter.filename=true;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);const std::string wanted="edited-normal-Ω.png";selected->m_normalMapFileName=wanted;
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)||!adapter.hit)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            std::ifstream saved(output,std::ios::binary);const std::string savedBytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(savedBytes!="keep destination")return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);adapter.active=false;selected->m_normalMapFileName=wanted;
            if(!adapter.fileExport(output.string(),DRW::AC1027,sourceBinary,&source,false,true)
                ||!physical(output,sourceBinary,false,false,.75,4,3,4,wanted)||!typed(source,false,.75,4,3,4,wanted))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
        }
        for(bool outputBinary:{false,true})for(int code:{291,301,469,62,420,430}) {
            if(!author(input,sourceBinary,false,false,false,false,0,code))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);dx_data source;dx_iface adapter;
            if(!adapter.fileImport(input.string(),&source,false)||!typed(source,false))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            DRW_Material* selected=nullptr;for(auto& item:source.materials)if(item.m_name=="PeerMaterial")selected=&item;
            if(!selected)return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);selected->m_normalMapFileName="edited-normal.png";
            {std::ofstream sentinel(output,std::ios::binary|std::ios::trunc);sentinel<<"keep destination";}
            if(adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
            std::ifstream saved(output,std::ios::binary);const std::string savedBytes(std::istreambuf_iterator<char>(saved),{});saved.close();
            if(savedBytes!="keep destination")return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);selected->m_normalMapFileName="normal-sentinel.png";
            if(!adapter.fileExport(output.string(),DRW::AC1027,outputBinary,&source,false,true)
                ||!physical(output,outputBinary,false)||!orphanField(output,outputBinary,code))return (std::cerr<<"NAME_EDIT_FAIL line="<<__LINE__<<" source="<<sourceBinary<<"\n",false);
        }
        // Unmarked normal mapper 75 is source-bound after the second 42.
        // The 0x07 mask is the existing writer admission policy, not native
        // interpretation of the normal map's bit meanings.
        for (bool outputBinary : {false, true}) for (int requested = 0; requested != 8; ++requested) {
            if (requested == 4) continue;
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapAutoTransform = static_cast<std::uint8_t>(requested);
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, .75, 4, 3, requested)
                || !typed(source, false, .75, 4, 3, requested)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, .75, 4, 3, requested)) return false;
        }
        for (bool outputBinary : {false, true}) {
            if (!author(input, sourceBinary, false, false, false, false, 0, 0, 4, true)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected || !originalCommon(*selected)) return false;
            selected->m_normalMapAutoTransform = 1; selected->m_normalMapBlendFactor = .875;
            selected->m_normalMapProjection = 1; selected->m_normalMapTiling = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !originalCommon(*selected) || !physical(output, outputBinary, false, false, .875, 1, 1, 1)
                || !typed(source, false, .875, 1, 1, 1)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !typed(readback, false, .875, 1, 1, 1) || !originalCommon(*target(readback))) return false;
        }
        for (bool outputBinary : {false, true}) for (unsigned fault = 0; fault != 5; ++fault) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            auto& raw = source.rawMaterialObjects.front(); const auto originalRaw = raw;
            selected->m_normalMapAutoTransform = 1;
            if (fault == 0) selected->m_normalMapAutoTransform = 8;
            if (fault == 1) selected->m_normalMapAutoTransform = 255;
            if (fault == 2) selected->m_diffuseMapAutoTransform = 8;
            if (fault == 3) selected->m_hasNormalMapAutoTransform = false;
            if (fault == 4) {
                unsigned count=0;
                for (std::size_t i=0; i<raw.groups.size(); ++i) if (raw.groups[i].code()==75 && ++count==2) {
                    raw.groups[i]=DRW_Variant(75,static_cast<std::int32_t>(3));
                    if(raw.hasRawValues)raw.rawValues.at(i)="3";
                }
                if(count!=2) return false;
            }
            const auto failedAuto=selected->m_normalMapAutoTransform;
            const auto failedDiffuse=selected->m_diffuseMapAutoTransform;
            const auto failedPresent=selected->m_hasNormalMapAutoTransform;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination" || selected->m_normalMapAutoTransform != failedAuto
                || selected->m_diffuseMapAutoTransform != failedDiffuse || selected->m_hasNormalMapAutoTransform != failedPresent) return false;
            raw=originalRaw; selected->m_normalMapAutoTransform=1;
            selected->m_diffuseMapAutoTransform=2; selected->m_hasNormalMapAutoTransform=true;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, .75, 4, 3, 1)
                || !typed(source, false, .75, 4, 3, 1)) return false;
        }
        for (unsigned phase=0; phase!=3; ++phase) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; LateMaterialEdit adapter; adapter.phase=phase; adapter.autoTransform=true;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapAutoTransform=1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true) || !adapter.hit) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active=false; selected->m_normalMapAutoTransform=1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true)
                || !physical(output, sourceBinary, false, false, .75, 4, 3, 1)
                || !typed(source, false, .75, 4, 3, 1)) return false;
        }
        // All nonempty subsets compose against the same immutable source;
        // the diffuse tuple remains unchanged in the physical output.
        for (bool outputBinary : {false, true}) for (unsigned mask = 1; mask != 8; ++mask) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            if (mask & 1u) selected->m_normalMapBlendFactor = 0.875;
            if (mask & 2u) selected->m_normalMapProjection = 1;
            if (mask & 4u) selected->m_normalMapTiling = 1;
            const double blend = mask & 1u ? 0.875 : 0.75;
            const int projection = mask & 2u ? 1 : 4, tiling = mask & 4u ? 1 : 3;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, blend, projection, tiling)
                || !typed(source, false, blend, projection, tiling)) return false;
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false) || !typed(readback, false, blend, projection, tiling)) return false;
        }
        for (bool outputBinary : {false, true}) for (unsigned fault = 0; fault != 10; ++fault) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapBlendFactor = 0.875; selected->m_normalMapProjection = 1; selected->m_normalMapTiling = 1;
            auto& raw = source.rawMaterialObjects.front(); const DRW_RawDxfObject originalRaw = raw;
            if (fault == 0) selected->m_normalMapProjection = 0;
            if (fault == 1) selected->m_normalMapProjection = 5;
            if (fault == 2) selected->m_normalMapTiling = 0;
            if (fault == 3) selected->m_normalMapTiling = 4;
            if (fault == 4) selected->m_normalMapBlendFactor = std::numeric_limits<double>::quiet_NaN();
            if (fault == 5) selected->m_normalMapSource = 0;
            if (fault >= 6) {
                const int wanted = fault == 6 ? 42 : fault == 7 ? 73 : fault == 8 ? 74 : 44;
                unsigned occurrence = 0;
                for (std::size_t i = 0; i < raw.groups.size(); ++i) if (raw.groups[i].code() == wanted) {
                    ++occurrence; if (occurrence != (wanted == 44 ? 1u : 2u)) continue;
                    if (wanted == 42 || wanted == 44) raw.groups[i] = DRW_Variant(wanted, 0.9375);
                    else raw.groups[i] = DRW_Variant(wanted, static_cast<std::int32_t>(2));
                    if (raw.hasRawValues) raw.rawValues.at(i) = wanted == 42 || wanted == 44 ? "0.9375" : "2";
                }
                if (occurrence != (wanted == 44 ? 1u : 2u)) return false;
            }
            const auto failedProjection = selected->m_normalMapProjection, failedTiling = selected->m_normalMapTiling;
            const auto failedSource = selected->m_normalMapSource; const double failedBlend = selected->m_normalMapBlendFactor;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination" || selected->m_normalMapProjection != failedProjection
                || selected->m_normalMapTiling != failedTiling || selected->m_normalMapSource != failedSource
                || !(std::isnan(failedBlend) ? std::isnan(selected->m_normalMapBlendFactor) : selected->m_normalMapBlendFactor == failedBlend)) return false;
            raw = originalRaw; selected->m_normalMapBlendFactor = 0.875; selected->m_normalMapProjection = 1;
            selected->m_normalMapTiling = 1; selected->m_normalMapSource = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)
                || !physical(output, outputBinary, false, false, 0.875, 1, 1) || !typed(source, false, 0.875, 1, 1)) return false;
        }
        for (unsigned phase = 0; phase != 3; ++phase) {
            if (!author(input, sourceBinary, false, false)) return false;
            dx_data source; LateMaterialEdit adapter; adapter.phase = phase; adapter.tiling = true;
            if (!adapter.fileImport(input.string(), &source, false) || !typed(source, false)) return false;
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials) if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapBlendFactor = 0.875; selected->m_normalMapProjection = 1; selected->m_normalMapTiling = 1;
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true) || !adapter.hit) return false;
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
            if (savedBytes != "keep destination") return false;
            adapter.active = false; selected->m_normalMapTiling = 1;
            if (!adapter.fileExport(output.string(), DRW::AC1027, sourceBinary, &source, false, true)
                || !physical(output, sourceBinary, false, false, 0.875, 1, 1) || !typed(source, false, 0.875, 1, 1)) return false;
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
                if (fault == 2) selected->m_normalMapSource = 0;
                if (fault == 3) {selected->m_normalMapFileName = "changed-normal.png";selected->m_hasNormalMapFileName=false;}
                if (fault == 4) selected->m_diffuseMapTiling = 4;
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
                const auto failedSource = selected->m_normalMapSource;
                const auto failedName = selected->m_normalMapFileName;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination" || selected->m_normalMapTiling != failedTiling
                    || selected->m_diffuseMapTiling != failedDiffuse || selected->m_normalMapProjection != failedProjection
                    || selected->m_normalMapBlendFactor != failedBlend || selected->m_normalMapSource != failedSource
                    || selected->m_normalMapFileName != failedName) return false;
                raw = originalRaw; selected->m_normalMapTiling = static_cast<std::uint8_t>(to);
                selected->m_diffuseMapTiling = 2; selected->m_normalMapProjection = 4; selected->m_normalMapBlendFactor = 0.75;
                selected->m_normalMapSource = 1; selected->m_normalMapFileName = "normal-sentinel.png"; selected->m_hasNormalMapFileName=true;
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
                if (fault == 2) selected->m_normalMapSource = 0;
                if (fault == 3) selected->m_diffuseMapProjection = 5;
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
                const auto failedSource = selected->m_normalMapSource;
                { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc); sentinel << "keep destination"; }
                if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary, &source, false, true)) return false;
                std::ifstream saved(output, std::ios::binary);
                const std::string savedBytes(std::istreambuf_iterator<char>(saved), {}); saved.close();
                if (savedBytes != "keep destination" || selected->m_normalMapProjection != failedProjection
                    || selected->m_diffuseMapProjection != failedDiffuse || selected->m_normalMapBlendFactor != failedBlend
                    || selected->m_normalMapSource != failedSource) return false;
                raw = originalRaw; selected->m_normalMapProjection = static_cast<std::uint8_t>(to);
                selected->m_diffuseMapProjection = 2; selected->m_normalMapBlendFactor = 0.75;
                selected->m_normalMapSource = 1;
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
                if (fault == 0) selected->m_diffuseMapBlendFactor = (std::numeric_limits<double>::infinity)();
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

        // ezdxf 1.4.5 emits this valid field order in MATERIAL/NormalOnly:
        // bump tiling, a repeated group 271 normal method, strength, then the
        // normal-map tuple. The normal map has no group-147 refraction matrix.
        for (bool outputBinary : {false, true}) {
            if (!author(input, sourceBinary, true, false, false, false, 0, 0,
                        4, false, 3, 0u, 0, 1, true)) return false;
            if (!preStrengthMethodPhysical(input, sourceBinary, 7, true)) {
                std::cerr << "pre-strength source field sequence was not authored\n";
                return false;
            }
            if (!physical(input, sourceBinary, true)) {
                std::cerr << "pre-strength source map fields were not authored\n";
                return false;
            }
            dx_data source; dx_iface adapter;
            if (!adapter.fileImport(input.string(), &source, false)
                || !preStrengthMethodTyped(source)) {
                std::cerr << "pre-strength normal method source was rejected\n";
                return false;
            }
            DRW_Material* selected = nullptr;
            for (auto& material : source.materials)
                if (material.m_name == "PeerMaterial") selected = &material;
            if (!selected) return false;
            selected->m_normalMapMethod = 5;
            selected->m_normalMapBlendFactor = 0.875;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)) {
                std::cerr << "pre-strength normal edit export failed\n";
                return false;
            }
            if (!preStrengthMethodPhysical(output, outputBinary, 5)) {
                std::cerr << "pre-strength normal method output role failed\n";
                return false;
            }
            if (!physical(output, outputBinary, true, false, 0.875)) {
                std::cerr << "pre-strength physical map output mismatch\n";
                return false;
            }
            if (!preStrengthMethodTyped(source, 5, 0.875)) {
                std::cerr << "pre-strength typed source edit mismatch\n";
                return false;
            }
            dx_data readback; dx_iface verifier;
            if (!verifier.fileImport(output.string(), &readback, false)
                || !preStrengthMethodTyped(readback, 5, 0.875)
                || !physical(output, outputBinary, true, false, 0.875)) {
                std::cerr << "pre-strength normal edit readback failed\n";
                return false;
            }

            // A changed raw normal method conflicts with the edited typed
            // value. Refuse without replacing the destination, then retry the
            // same adapter after restoring the captured method.
            auto& raw = source.rawMaterialObjects.front();
            const auto savedRaw = raw;
            unsigned methods = 0;
            for (std::size_t i = 0; i < raw.groups.size(); ++i) {
                if (raw.groups[i].code() != 271 || ++methods != 2u) continue;
                raw.groups[i] = DRW_Variant(271, std::int32_t(6));
                if (raw.hasRawValues) raw.rawValues.at(i) = "6";
                break;
            }
            if (methods != 2u) {
                std::cerr << "pre-strength method role not found in raw body\n";
                return false;
            }
            { std::ofstream sentinel(output, std::ios::binary | std::ios::trunc);
              sentinel << "keep destination"; }
            if (adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                   &source, false, true)) {
                std::cerr << "pre-strength conflicting raw method was accepted\n";
                return false;
            }
            std::ifstream saved(output, std::ios::binary);
            const std::string savedBytes(std::istreambuf_iterator<char>(saved), {});
            if (savedBytes != "keep destination"
                || !preStrengthMethodTyped(source, 5, 0.875)) {
                std::cerr << "pre-strength refusal changed destination/model\n";
                return false;
            }
            raw = savedRaw;
            if (!adapter.fileExport(output.string(), DRW::AC1027, outputBinary,
                                    &source, false, true)
                || !preStrengthMethodPhysical(output, outputBinary, 5)) {
                std::cerr << "pre-strength same-adapter retry failed\n";
                return false;
            }
        }

        for (unsigned incomplete = 0; incomplete != 2; ++incomplete) {
            if (!author(input, sourceBinary, incomplete != 0,
                        false, incomplete != 0, false, 0, 0, 4, false, 3,
                        0u, 0, 1, true)) return false;
            dx_data bad; dx_iface sameAdapter;
            if (sameAdapter.fileImport(input.string(), &bad, false)) {
                std::cerr << (incomplete == 0
                    ? "pre-strength method without strength was accepted\n"
                    : "incomplete pre-strength normal tuple was accepted\n");
                return false;
            }
            if (!author(input, sourceBinary, true, false, false, false, 0, 0,
                        4, false, 3, 0u, 0, 1, true)) return false;
            dx_data corrected;
            if (!sameAdapter.fileImport(input.string(), &corrected, false)
                || !preStrengthMethodTyped(corrected)) {
                std::cerr << "pre-strength normal retry failed\n";
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
