#include "dx_iface.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <vector>

namespace {
struct Tag { int code; std::string value; };
using Tags = std::vector<Tag>;

class Probe final : public dx_iface {
public:
    std::vector<DRW_TableGeometry> geometry;
    std::vector<DRW_RawDxfObject> raw;
    std::map<std::string, unsigned> unknown;
    void addTableGeometry(const DRW_TableGeometry& value) override {
        geometry.push_back(value);
        dx_iface::addTableGeometry(value);
    }
    void addRawDxfObject(const DRW_RawDxfObject& value) override {
        if (value.name == "TABLEGEOMETRY" || value.name == "aCdBtAbLeGeOmEtRy") raw.push_back(value);
        dx_iface::addRawDxfObject(value);
    }
    void addUnmodeledDxfObject(const DRW_RawDxfObject& value) override {
        ++unknown[value.name];
        dx_iface::addUnmodeledDxfObject(value);
    }
    void clearObservations() { geometry.clear(); raw.clear(); unknown.clear(); }
};

Tags source(const std::string& version, const std::string& name,
            bool nullCellReferences = false, bool witnessedProfile = false,
            std::array<int,2> witnessedFlags = {{3,3}}) {
    Tags result{{0,"SECTION"},{2,"HEADER"},{9,"$ACADVER"},{1,version},{0,"ENDSEC"},
        {0,"SECTION"},{2,"OBJECTS"},{0,name},{5,"A0"},{330,"F0"},{100,"AcDbTableGeometry"},
        {90,"1"},{91,"2"},{92,"2"}};
    const std::array<int,10> codes{{10,20,30,11,21,31,43,44,45,46}};
    unsigned seed = 1u;
    for (int cell = 0; cell != 2; ++cell) {
        result.insert(result.end(), {{93,witnessedProfile
                ? std::to_string(witnessedFlags[static_cast<std::size_t>(cell)])
                : std::to_string(-3-cell)},
            {40,std::to_string(25+cell)},{41,std::to_string(9+cell)},
            {330,nullCellReferences ? "0" : (cell == 0 ? "A1" : "A2")},
            {94,std::to_string(cell+1)}});
        for (int content = 0; content <= cell; ++content, ++seed) {
            for (std::size_t i = 0; i != codes.size(); ++i) {
                const bool zeroZ = witnessedProfile && (codes[i] == 30 || codes[i] == 31);
                result.push_back({codes[i],zeroZ ? "0"
                    : std::to_string(seed*16u+i)+".25"});
            }
            result.push_back({95,witnessedProfile ? "0"
                : std::to_string(-7-static_cast<int>(seed))});
        }
    }
    result.insert(result.end(), {{0,"ENDSEC"},{0,"EOF"}});
    return result;
}

bool write(const std::filesystem::path& path, const Tags& tags, bool binary) {
    std::ofstream stream(path,std::ios::binary);
    const auto word = [&](std::uint64_t bits, unsigned width) {
        for (unsigned i = 0; i != width; ++i) stream.put(static_cast<char>(bits >> (8u*i)));
    };
    if (binary) stream.write("AutoCAD Binary DXF\r\n\x1a\0",22);
    for (const auto& tag : tags) {
        if (!binary) stream << tag.code << '\n' << tag.value << '\n';
        else {
            word(static_cast<unsigned>(tag.code),2u);
            if (tag.code >= 90 && tag.code <= 99) word(static_cast<std::uint32_t>(std::stoll(tag.value)),4u);
            else if (tag.code >= 10 && tag.code <= 59) {
                const double value = std::stod(tag.value); std::uint64_t bits = 0u;
                std::memcpy(&bits,&value,8u); word(bits,8u);
            } else { stream << tag.value; stream.put('\0'); }
        }
    }
    return stream.good();
}

std::string bytes(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream),{});
}

bool expected(const Probe& probe, const dx_data& data, std::uint32_t owner = 0xF0u,
              bool nullCellReferences = false, bool witnessedProfile = false,
              std::array<int,2> witnessedFlags = {{3,3}}) {
    if (probe.geometry.size() != 1u || probe.raw.size() != 1u || !probe.unknown.empty()
        || data.unmodeledDxfObjectSeen || !data.standaloneTableGeometryPresent) return false;
    const auto& value = probe.geometry.front();
    if (value.handle != 0xA0u || value.parentHandle != owner || probe.raw.front().parentHandle != owner
        || value.m_rowCount != 1 || value.m_columnCount != 2 || value.m_cellCount != 2
        || value.m_cells.size() != 2u) return false;
    unsigned seed = 1u;
    for (std::size_t cellIndex = 0; cellIndex != 2u; ++cellIndex) {
        const auto& cell = value.m_cells[cellIndex];
        if (cell.m_flags != (witnessedProfile
                ? witnessedFlags[cellIndex] : -3-static_cast<int>(cellIndex))
            || cell.m_widthWithGap != 25+cellIndex
            || cell.m_heightWithGap != 9+cellIndex
            || cell.m_unknownHandle != (nullCellReferences ? 0u : 0xA1u+cellIndex)
            || cell.m_contents.size() != cellIndex+1u) return false;
        for (const auto& content : cell.m_contents) {
            const std::array<double,10> actual{{content.m_topLeft.x,content.m_topLeft.y,content.m_topLeft.z,
                content.m_center.x,content.m_center.y,content.m_center.z,content.m_contentWidth,
                content.m_contentHeight,content.m_width,content.m_height}};
            for (std::size_t i = 0; i != actual.size(); ++i)
                if (actual[i] != ((witnessedProfile && (i == 2u || i == 5u))
                        ? 0.0 : seed*16u+i+0.25)) return false;
            if (content.m_unknown != (witnessedProfile ? 0 : -7-static_cast<int>(seed)))
                return false;
            ++seed;
        }
    }
    return true;
}

bool rawObjectMatches(const DRW_RawDxfObject& raw, const Tags& sourceTags) {
    const auto object = std::find_if(sourceTags.cbegin(), sourceTags.cend(),
        [](const Tag& tag) { return tag.code == 0 && tag.value == "TABLEGEOMETRY"; });
    if (object == sourceTags.cend()) return false;
    const auto end = std::find_if(std::next(object), sourceTags.cend(),
        [](const Tag& tag) { return tag.code == 0; });
    const auto first = static_cast<std::size_t>(std::distance(sourceTags.cbegin(), object)) + 1u;
    const auto last = static_cast<std::size_t>(std::distance(sourceTags.cbegin(), end));
    if (raw.groups.size() != last - first) return false;
    for (std::size_t i = 0; i != raw.groups.size(); ++i) {
        const Tag& expectedTag = sourceTags[first + i];
        const DRW_Variant& actual = raw.groups[i];
        if (actual.code() != expectedTag.code) return false;
        if (expectedTag.code >= 10 && expectedTag.code <= 59) {
            if (actual.type() != DRW_Variant::DOUBLE
                || actual.d_val() != std::stod(expectedTag.value)) return false;
        } else if (expectedTag.code >= 90 && expectedTag.code <= 99) {
            if ((actual.type() != DRW_Variant::INTEGER
                    && actual.type() != DRW_Variant::INTEGER64)
                || (actual.type() == DRW_Variant::INTEGER
                    ? static_cast<std::int64_t>(actual.i_val())
                    : actual.i64_val()) != std::stoll(expectedTag.value)) return false;
        } else if (actual.type() != DRW_Variant::STRING || !actual.content.s
                   || *actual.content.s != expectedTag.value) {
            return false;
        }
    }
    return raw.name == "TABLEGEOMETRY";
}

std::size_t slot(const Tags& tags, int code) {
    for (std::size_t i = 0; i != tags.size(); ++i) if (tags[i].code == code) return i;
    throw std::runtime_error("missing mutation slot");
}

bool tests(const std::filesystem::path& directory) {
    const auto input = directory / "source.dxf", output = directory / "output.dxf";
    const auto fail = [&](const char* message) { std::cerr << message << '\n'; return false; };
    const std::vector<std::function<void(Tags&)>> unsupported{
        [](Tags& t){t[slot(t,90)].value="-1";},
        [](Tags& t){t[slot(t,90)].value="5001";},
        [](Tags& t){t[slot(t,91)].value="5001";},
        [](Tags& t){t[slot(t,92)].value="10001";},
        [](Tags& t){t[slot(t,92)].value="1";},
        [](Tags& t){t[slot(t,94)].value="-1";},
        [](Tags& t){t[slot(t,94)].value="10001";},
        [](Tags& t){t[slot(t,94)].value="9999";},
        [](Tags& t){const auto i=slot(t,93); t.insert(t.begin()+static_cast<std::ptrdiff_t>(i),t[i]);},
        [](Tags& t){t.erase(t.begin()+static_cast<std::ptrdiff_t>(slot(t,10)));},
        [](Tags& t){std::swap(t[slot(t,10)],t[slot(t,20)]);},
        [](Tags& t){t[slot(t,100)].value="AcDbFutureTableGeometry";},
        [](Tags& t){t.insert(t.end()-2,{100,"AcDbFutureTail"});},
        [](Tags& t){t.insert(t.end()-2,{1001,"FUTURE_APP"});},
        [](Tags& t){t.erase(t.begin()+static_cast<std::ptrdiff_t>(slot(t,95)));},
        [](Tags& t){t[slot(t,5)].value="1000000A0";},
        [](Tags& t){t[slot(t,330)].value="1000000F0";},
        [](Tags& t){t[slot(t,100)+1u].code=91;},
        [](Tags& t){t[slot(t,90)].value="0";},
        [](Tags& t){t.insert(t.begin()+static_cast<std::ptrdiff_t>(slot(t,100)),
            {{102,"{ACAD_REACTORS"},{330,"B1"},{102,"}"}});}
    };
    unsigned positives=0u, fallbacks=0u, retries=0u, refusals=0u;
    for (const auto* version : {"AC1021","AC1024","AC1027","AC1032"})
        for (bool binary : {false,true}) for (const auto* name : {"TABLEGEOMETRY","aCdBtAbLeGeOmEtRy"}) {
            const auto tags = source(version,name);
            Probe probe; dx_data data;
            if (!write(input,tags,binary)) return false;
            const auto original = bytes(input);
            if (!probe.fileImport(input.string(),&data,false) || !expected(probe,data)
                || bytes(input)!=original) return fail("complete TABLEGEOMETRY projection mismatch");
            ++positives;
            for (const auto& mutate : unsupported) {
                auto broken = tags; mutate(broken);
                probe.clearObservations(); dx_data bad;
                if (!write(input,broken,binary) || !probe.fileImport(input.string(),&bad,false)
                    || !probe.geometry.empty() || probe.raw.size()!=1u || probe.unknown.size()!=1u
                    || !bad.unmodeledDxfObjectSeen || !bad.standaloneTableGeometryPresent)
                    return fail("unsupported cache published a partial typed projection or lost raw fallback");
                ++fallbacks;
                probe.clearObservations(); dx_data restored;
                if (!write(input,tags,binary) || !probe.fileImport(input.string(),&restored,false)
                    || !expected(probe,restored) || !bad.unmodeledDxfObjectSeen
                    || !bad.standaloneTableGeometryPresent)
                    return fail("corrected import did not recover or cleared an earlier container's failure");
                ++retries;
            }
            auto nullOwner=tags; nullOwner[slot(nullOwner,330)].value="0";
            probe.clearObservations(); dx_data nullData;
            if (!write(input,nullOwner,binary) || !probe.fileImport(input.string(),&nullData,false)
                || !expected(probe,nullData,0u)) return fail("body soft pointer overwrote explicit null header owner");
            nullOwner.insert(nullOwner.end()-2,{1001,"FUTURE_APP"});
            probe.clearObservations(); dx_data nullFallback;
            if (!write(input,nullOwner,binary) || !probe.fileImport(input.string(),&nullFallback,false)
                || !probe.geometry.empty() || probe.raw.size()!=1u || probe.raw.front().parentHandle!=0u
                || !nullFallback.unmodeledDxfObjectSeen)
                return fail("raw fallback body pointer overwrote explicit null header owner");
        }
    for (bool binary : {false,true}) {
        auto corner=source("AC1032","TABLEGEOMETRY");
        corner.erase(corner.begin()+static_cast<std::ptrdiff_t>(slot(corner,93)),corner.end()-2);
        corner[slot(corner,90)].value="0"; corner[slot(corner,91)].value="0"; corner[slot(corner,92)].value="0";
        Probe probe; dx_data empty;
        if (!write(input,corner,binary) || !probe.fileImport(input.string(),&empty,false)
            || probe.geometry.size()!=1u || !probe.geometry.front().m_cells.empty())
            return fail("empty geometry cache was not decoded");
        corner[slot(corner,90)].value="5000"; corner[slot(corner,91)].value="2"; corner[slot(corner,92)].value="10000";
        corner[slot(corner,330)].value="0";
        corner.erase(corner.end()-2,corner.end());
        for (unsigned i=0; i!=10000u; ++i)
            corner.insert(corner.end(),{{93,"3"},{40,"1"},{41,"2"},{330,"0"},{94,"0"}});
        corner.insert(corner.end(),{{0,"ENDSEC"},{0,"EOF"}});
        probe.clearObservations(); dx_data boundary;
        if (!write(input,corner,binary) || !probe.fileImport(input.string(),&boundary,false)
            || probe.geometry.size()!=1u || probe.geometry.front().m_cells.size()!=10000u
            || probe.geometry.front().m_rowCount!=5000 || probe.raw.front().parentHandle!=0u)
            return fail("bounded maximum cell count was not decoded");
    }
    unsigned writerRoutes=0u,writerFailures=0u,writerRetries=0u;
    unsigned versionRefusals=0u,versionRetries=0u;
    unsigned flagRefusals=0u,flagRetries=0u;
    unsigned unknownValueRefusals=0u,unknownValueRetries=0u;
    const std::array<DRW::Version,4> outputVersions{{
        DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}};
    const std::array<std::array<int,2>,2> flagProfiles{{{3,3},{4,7}}};
    for (const auto& flags : flagProfiles)
    for (DRW::Version outputVersion : outputVersions)
    for (bool inputBinary : {false,true}) for (bool outputBinary : {false,true}) {
        const Tags witnessed = source("AC1032", "TABLEGEOMETRY", true, true, flags);
        Probe sourceReader; dx_data retained;
        if (!write(input,witnessed,inputBinary)
            || !sourceReader.fileImport(input.string(),&retained,false)
            || !expected(sourceReader,retained,0xF0u,true,true,flags))
            return fail("bounded writer source profile was not captured");

        dx_iface exporter;
        const auto readback = [&]() {
            Probe reader; dx_data decoded;
            return reader.fileImport(output.string(),&decoded,false)
                && expected(reader,decoded,0xF0u,true,true,flags)
                && reader.raw.size()==1u
                && rawObjectMatches(reader.raw.front(),witnessed);
        };
        if (outputVersion == DRW::AC1021) {
            const std::string sentinel="KEEP_UNSUPPORTED_TABLE_GEOMETRY_VERSION\n";
            {std::ofstream stream(output,std::ios::binary); stream<<sentinel;}
            if (exporter.fileExport(output.string(),DRW::AC1018,outputBinary,
                                    &retained,false,false)
                || bytes(output)!=sentinel)
                return fail("pre-AC1021 TABLEGEOMETRY output did not fail atomically");
            ++versionRefusals;
        }
        if (!exporter.fileExport(output.string(),outputVersion,outputBinary,
                                 &retained,false,false)
            || !readback())
            return fail("bounded TABLEGEOMETRY ASCII/Binary writer fields did not read back");
        if (outputVersion == DRW::AC1021) ++versionRetries;
        ++writerRoutes;

        if (flags[0] == 4 && outputVersion == DRW::AC1021
            && !inputBinary && !outputBinary) {
            retained.tableGeometries.front().m_cells.front().m_flags=5;
            const std::string flagSentinel="KEEP_UNQUALIFIED_TABLE_GEOMETRY_FLAGS\n";
            {std::ofstream stream(output,std::ios::binary); stream<<flagSentinel;}
            if (exporter.fileExport(output.string(),outputVersion,outputBinary,
                                    &retained,false,false)
                || bytes(output)!=flagSentinel)
                return fail("unqualified TABLEGEOMETRY flags did not fail atomically");
            ++flagRefusals;
            retained.tableGeometries.front().m_cells.front().m_flags=flags[0];
            if (!exporter.fileExport(output.string(),outputVersion,outputBinary,
                                     &retained,false,false)
                || !readback())
                return fail("same-exporter TABLEGEOMETRY flag correction did not retry");
            ++flagRetries;
        }

        if (flags[0] == 4 && outputVersion == DRW::AC1021
            && !inputBinary && !outputBinary) {
            auto& content = retained.tableGeometries.front()
                                .m_cells.front().m_contents.front();
            content.m_dwgUnknownValue = 12.5;
            content.m_hasDwgUnknownValue = true;
            const std::string unknownSentinel=
                "KEEP_UNREPRESENTABLE_TABLE_GEOMETRY_BD\n";
            {std::ofstream stream(output,std::ios::binary); stream<<unknownSentinel;}
            if (exporter.fileExport(output.string(),outputVersion,outputBinary,
                                    &retained,false,false)
                || bytes(output)!=unknownSentinel)
                return fail("unrepresentable TABLEGEOMETRY DWG BD value was not refused atomically");
            ++unknownValueRefusals;
            content.m_dwgUnknownValue = 0.0;
            if (!exporter.fileExport(output.string(),outputVersion,outputBinary,
                                     &retained,false,false)
                || !readback())
                return fail("corrected TABLEGEOMETRY DWG BD value did not retry");
            content.m_hasDwgUnknownValue = false;
            ++unknownValueRetries;
        }

        retained.tableGeometries.front().m_cells.front().m_unknownHandle=0xA1u;
        const std::string sentinel="KEEP_TABLE_GEOMETRY_DESTINATION\n";
        {std::ofstream stream(output,std::ios::binary); stream<<sentinel;}
        if (exporter.fileExport(output.string(),outputVersion,outputBinary,
                                &retained,false,false)
            || bytes(output)!=sentinel)
            return fail("unqualified TABLEGEOMETRY cell reference did not fail atomically");
        ++writerFailures;

        retained.tableGeometries.front().m_cells.front().m_unknownHandle=0u;
        if (!exporter.fileExport(output.string(),outputVersion,outputBinary,
                                 &retained,false,false)
            || !readback())
            return fail("same-exporter TABLEGEOMETRY correction did not retry/read back");
        ++writerRetries;
    }
    for (bool binary : {false,true}) for (bool strict : {false,true}) for (int kind=0; kind!=6; ++kind) {
        const bool content=kind==0 || kind==2;
        dx_data data, clean; dx_iface importer, exporter;
        importer.cData=&data;
        if (kind<2) {
            data.sourceFormat=dx_source_format::DWG;
            if (content) importer.addTableContent(DRW_TableContentObject{});
            else importer.addTableGeometry(DRW_TableGeometry{});
        } else if (kind<4) {
            data.sourceFormat=dx_source_format::DXF;
            DRW_RawDxfObject raw; raw.name=content?"TABLECONTENT":"aCdBtAbLeGeOmEtRy";
            importer.addUnmodeledDxfObject(raw);
        } else {
            auto tags=source("AC1032","TABLEGEOMETRY");
            if (kind==5) tags.insert(tags.end()-2,{1001,"FUTURE_APP"});
            if (!write(input,tags,!binary) || !importer.fileImport(input.string(),&data,false)) return false;
        }
        if (!(content ? data.standaloneTableContentPresent : data.standaloneTableGeometryPresent)) return false;
        const std::string sentinel="PRESERVE_TABLE_DEPENDENCY_FAILURE\n";
        {std::ofstream stream(output,std::ios::binary); stream<<sentinel;}
        if (exporter.fileExport(output.string(),DRW::AC1032,binary,&data,false,strict)
            || bytes(output)!=sentinel) return fail("converter silently dropped a standalone native table dependency");
        importer.cData=&clean; importer.currentBlock=clean.mBlock;
        DRW_Line line; line.basePoint={1,2,3}; line.secPoint={4,5,6}; importer.addLine(line);
        if (!exporter.fileExport(output.string(),DRW::AC1032,binary,&clean,false,strict))
            return fail("same exporter did not recover with a complete source");
        dx_data actual; dx_iface readback;
        if (!readback.fileImport(output.string(),&actual,false) || actual.mBlock->ent.size()!=1u)
            return fail("corrected export was not actually readable");
        const auto* actualLine=dynamic_cast<const DRW_Line*>(actual.mBlock->ent.front());
        if (actualLine==nullptr || actualLine->basePoint.x!=1 || actualLine->basePoint.y!=2
            || actualLine->basePoint.z!=3 || actualLine->secPoint.x!=4 || actualLine->secPoint.y!=5
            || actualLine->secPoint.z!=6
            || !(content ? data.standaloneTableContentPresent : data.standaloneTableGeometryPresent))
            return fail("corrected readback changed LINE coordinates or cleared prior dependency state");
        ++refusals;
    }
    std::cout<<"TABLEGEOMETRY: "<<positives<<" projections; "<<fallbacks<<" raw fallbacks/"<<retries
             <<" corrected imports; 16 typed/16 raw null-owner and 4 empty/maximum-cell controls; "
             <<writerRoutes<<" ASCII/Binary writer/readbacks, "<<writerFailures
             <<" atomic refusals/"<<writerRetries<<" same-exporter retries; "
             <<flagRefusals<<" unqualified-flag refusals/"<<flagRetries
             <<" same-exporter retries; "
             <<unknownValueRefusals<<" unrepresentable-DWG-BD refusals/"
             <<unknownValueRetries<<" same-exporter retries; "
             <<versionRefusals<<" pre-AC1021 refusals/"<<versionRetries
             <<" AC1021 retries; "<<refusals<<" atomic table-dependency refusals/readbacks\n";
    return positives==16u && fallbacks==320u && retries==320u && refusals==24u
        && writerRoutes==32u && writerFailures==32u && writerRetries==32u
        && flagRefusals==1u && flagRetries==1u
        && unknownValueRefusals==1u && unknownValueRetries==1u
        && versionRefusals==8u && versionRetries==8u;
}

int inspect(const char* path) {
    Probe probe; dx_data data;
    const bool ok=probe.fileImport(path,&data,false);
    std::cout<<std::setprecision(17)<<"{\"imported\":"<<(ok?"true":"false")<<",\"geometry\":[";
    bool first=true;
    for (const auto& geometry : probe.geometry) {
        if (!first) std::cout<<',';
        first=false;
        std::cout<<"{\"handle\":"<<geometry.handle<<",\"owner\":"<<geometry.parentHandle
                 <<",\"rows\":"<<geometry.m_rowCount<<",\"columns\":"<<geometry.m_columnCount<<",\"cells\":[";
        bool firstCell=true;
        for (const auto& cell : geometry.m_cells) {
            if (!firstCell) std::cout<<',';
            firstCell=false;
            std::cout<<"{\"flags\":"<<cell.m_flags<<",\"widthWithGap\":"<<cell.m_widthWithGap
                     <<",\"heightWithGap\":"<<cell.m_heightWithGap<<",\"softPointer\":"<<cell.m_unknownHandle<<",\"contents\":[";
            bool firstContent=true;
            for (const auto& content : cell.m_contents) {
                if (!firstContent) std::cout<<',';
                firstContent=false;
                std::cout<<'['<<content.m_topLeft.x<<','<<content.m_topLeft.y<<','<<content.m_topLeft.z<<','
                         <<content.m_center.x<<','<<content.m_center.y<<','<<content.m_center.z<<','
                         <<content.m_contentWidth<<','<<content.m_contentHeight<<','<<content.m_width<<','
                         <<content.m_height<<','<<content.m_unknown<<']';
            }
            std::cout<<"]}";
        }
        std::cout<<"]}";
    }
    std::cout<<"]}\n";
    return ok?0:1;
}
} // namespace

int main(int argc,char** argv) {
    if (argc==3 && std::string(argv[1])=="--inspect") return inspect(argv[2]);
    if (argc!=1) return 2;
    const auto directory=std::filesystem::temp_directory_path()
        / ("libdxfrw-table-geometry-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code error;
    if (!std::filesystem::create_directory(directory,error) || error) return 1;
    const bool ok=tests(directory);
    if (ok) std::filesystem::remove_all(directory,error);
    else std::cerr<<"Evidence: "<<directory<<'\n';
    return ok?0:1;
}
