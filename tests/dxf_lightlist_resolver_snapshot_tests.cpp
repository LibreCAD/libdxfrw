// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0u;
struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};
}

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

static DRW_LightList makeList() {
    DRW_LightList list; list.handle=0xD00u; list.name="caller-name"; list.parentHandle=0xA02u;
    list.xDictHandle=0xA03u; list.m_classVersion=1u; list.m_lightCount=2u;
    list.m_lights={{0xB00u,"key-light"},{0xB01u,"fill-light"}};
    list.reactorHandles={0xA01u};
    std::list<DRW_Variant> group;
    group.emplace_back(102,std::string("ACAD_REACTORS"));
    group.emplace_back(330,std::string("A00")); group.emplace_back(330,std::string("A01"));
    group.emplace_back(102,std::string("}")); list.appData.push_back(std::move(group));
    list.extData.push_back(new DRW_Variant(1001,std::string("ACAD")));
    list.extData.push_back(new DRW_Variant(1000,std::string("original")));
    return list;
}
struct Writer final : LC3CompatCheck {
    explicit Writer(dxfRW& w):writer(w) {}
    void writeEntities() override {
        if (!precedingLine) return;
        DRW_Line line; line.basePoint={1.,2.,0.};line.secPoint={3.,4.,0.};
        line.extData.push_back(std::make_shared<DRW_Variant>(1001,std::string("ACAD")));
        line.extData.push_back(std::make_shared<DRW_Variant>(1005,std::string("D00")));
        lineAccepted=writer.writeLine(&line);
    }
    void writeObjects() override {
        if (failAt < 0) {
            accepted = writer.writeLightList(&list);
        } else {
            const std::size_t before = failedAllocations;
            AllocationFailure failure(failAt);
            try {
                accepted = writer.writeLightList(&list);
            } catch (const std::bad_alloc&) {
                reached = failedAllocations != before
                    ? static_cast<long>(failedAllocations - before)
                    : failAt - allocationsBeforeFailure;
                throw;
            }
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        }
        if (!accepted)
            (void)writer.writeRawDxfObject(nullptr);
    }
    bool finalizeDxfWrite() override { return accepted && (!precedingLine || lineAccepted); }
    dxfRW& writer; DRW_LightList list=makeList(); bool accepted=false;
    bool precedingLine=false, lineAccepted=false;
    long failAt=-1, reached=0;
};
struct Reader final : LC3CompatCheck {
    void addLightList(const DRW_LightList& value) override { list=value; ++count; }
    DRW_LightList list; int count=0;
};
static void sentinel(const std::filesystem::path& p) {
    std::ofstream out(p,std::ios::binary|std::ios::trunc);out<<"SENTINEL";
}
static bool unchanged(const std::filesystem::path& p) {
    std::ifstream in(p,std::ios::binary);return std::string(std::istreambuf_iterator<char>(in),{})=="SENTINEL";
}
static bool readSnapshot(const std::filesystem::path& p, std::uint32_t offset=0x10000u) {
    dxfRW input(p.string().c_str()); Reader r;
    if(!input.read(&r,false)||r.count!=1)return false;
    std::uint32_t dictionary=0;
    return r.list.handle==0xAD00u && r.list.parentHandle==0xA02u+offset
        && r.list.xDictHandle==0xA03u+offset && r.list.m_classVersion==1u && r.list.m_lightCount==2u
        && r.list.getAcLightDictionaryHandle(dictionary) && dictionary==0xA00u+offset
        && r.list.reactorHandles==std::vector<std::uint32_t>{0xA01u+offset}
        && r.list.m_lights.size()==2u && r.list.m_lights[0].m_handle==0xB00u+offset
        && r.list.m_lights[0].m_name=="key-light" && r.list.m_lights[1].m_handle==0xB01u+offset
        && r.list.m_lights[1].m_name=="fill-light" && r.list.extData.size()==2u
        && r.list.extData[1] && r.list.extData[1]->content.s && *r.list.extData[1]->content.s=="original";
}
static int physical(const std::filesystem::path& source, const std::filesystem::path& preserve) {
    dxfRW input(source.string().c_str()); Reader capture;
    if(!input.read(&capture,false)||capture.count!=1||capture.list.handle!=0xF000u
       ||capture.list.parentHandle!=0x30u||capture.list.m_lights.size()!=1u
       ||capture.list.m_lights[0].m_handle!=0x2Fu||capture.list.m_lights[0].m_name!="LOCAL_LIGHT")return 7;
    const auto dir=std::filesystem::temp_directory_path()/("physical-lightlist-snapshot-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{dir};
    for(bool binary:{false,true}) {
        auto path=dir/(binary?"binary.dxf":"ascii.dxf");dxfRW output(path.string().c_str());Writer writer(output);
        writer.list=capture.list;bool mutated=false;
        output.setReferenceResolver([&](std::uint32_t h) {
            if(h==0x30u&&!mutated){mutated=true;writer.list.m_lights.clear();writer.list.appData.clear();writer.list.parentHandle=0xA0u;}
            return h;
        });
        if(!output.write(&writer,DRW::AC1027,binary)||!writer.accepted||!mutated)return 8;
        dxfRW reinput(path.string().c_str());Reader reread;
        std::uint32_t dictionary=0;
        if(!reinput.read(&reread,false)||reread.count!=1||reread.list.handle!=0xF000u
           ||reread.list.parentHandle!=0x30u||!reread.list.getAcLightDictionaryHandle(dictionary)||dictionary!=0x30u
           ||reread.list.m_lights.size()!=1u||reread.list.m_lights[0].m_handle!=0x2Fu
           ||reread.list.m_lights[0].m_name!="LOCAL_LIGHT")return 9;
        if(!preserve.empty()) {
            std::filesystem::create_directories(preserve);
            std::filesystem::copy_file(path,preserve/(binary?"binary.dxf":"ascii.dxf"),
                                       std::filesystem::copy_options::overwrite_existing);
        }
    }
    return 0;
}

static bool readBytes(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input},
                 std::istreambuf_iterator<char>{});
    return !input.bad();
}

static bool allocationSweep(const std::filesystem::path& directory) {
    const auto path = directory / "allocation-lightlist.dxf";
    const std::string prior = "prior LIGHTLIST bytes\n";
    for (const bool binary : {false, true}) {
        long allocationCount = 0;
        std::string expectedBytes;
        {
            dxfRW codec(path.string().c_str());
            Writer exporter(codec);
            codec.setHandleRemap({{0xD00u, 0xAD00u}});
            codec.setReferenceResolver([](std::uint32_t handle) {
                return handle + 0x10000u;
            });
            exporter.failAt = 1000000;
            const bool written = codec.write(&exporter, DRW::AC1027, binary);
            if (!written || !exporter.accepted || !readSnapshot(path)
                || !readBytes(path, expectedBytes))
                return false;
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
            Writer exporter(codec);
            codec.setHandleRemap({{0xD00u, 0xAD00u}});
            codec.setReferenceResolver([](std::uint32_t handle) {
                return handle + 0x10000u;
            });
            exporter.failAt = offset;
            const std::string expectedName = "caller-name";
            const auto expectedLights = exporter.list.m_lights;
            const bool written = codec.write(&exporter, DRW::AC1027, binary);
            exporter.failAt = -1;
            std::string current;
            if (written || exporter.accepted || exporter.reached != 1
                || !readBytes(path, current) || current != prior
                || exporter.list.handle != 0xD00u
                || exporter.list.name != expectedName
                || exporter.list.parentHandle != 0xA02u
                || exporter.list.xDictHandle != 0xA03u
                || exporter.list.m_classVersion != 1u
                || exporter.list.m_lightCount != 2u
                || exporter.list.m_lights.size() != expectedLights.size()
                || exporter.list.m_lights[0].m_handle != expectedLights[0].m_handle
                || exporter.list.m_lights[0].m_name != expectedLights[0].m_name
                || exporter.list.m_lights[1].m_handle != expectedLights[1].m_handle
                || exporter.list.m_lights[1].m_name != expectedLights[1].m_name)
                return false;

            const bool retried = codec.write(&exporter, DRW::AC1027, binary);
            std::string retryBytes;
            if (!retried || !exporter.accepted
                || !readBytes(path, retryBytes) || retryBytes != expectedBytes
                || !readSnapshot(path))
                return false;
        }
        std::cout << "LIGHTLIST DXF " << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
    }
    return true;
}

int main(int argc,char** argv) {
    if(argc==2 || argc==3)return physical(argv[1],argc==3?argv[2]:"");
    if(argc!=1)return 10;
    const auto dir=std::filesystem::temp_directory_path()/("lightlist-snapshot-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code ec;std::filesystem::remove_all(p,ec);}} cleanup{dir};
    for(bool binary:{false,true}) {
        const auto path=dir/(binary?"binary.dxf":"ascii.dxf");dxfRW output(path.string().c_str()); Writer writer(output);
        output.setHandleRemap({{0xD00u,0xAD00u}});
        for(int phase:{0,1,2,3,4}) {
            writer.list=makeList();int seen=0;bool mutated=false;
            output.setReferenceResolver([&](std::uint32_t h) {
                if ((phase==2 && h==0xB00u)
                    || (phase<=1 && h==0xA00u && ++seen==phase+1)
                    || (phase==3 && h==0xA02u && ++seen==2)
                    || (phase==4 && h==0xA03u && ++seen==2)) {
                    if(!mutated) {
                        mutated=true;writer.list.handle=0xF03u;writer.list.name="mutated";writer.list.m_lights.clear();writer.list.m_lightCount=0u;
                        writer.list.m_classVersion=9u;writer.list.parentHandle=0xF01u;
                        writer.list.xDictHandle=0xF02u;writer.list.reactorHandles.clear();
                        writer.list.appData.clear();*writer.list.extData[1]=DRW_Variant(1000,std::string("mutated"));
                    }
                }
                return h+0x10000u;
            });
            if(!output.write(&writer,DRW::AC1027,binary)||!writer.accepted||!mutated||!readSnapshot(path)) {
                std::cerr<<"snapshot phase="<<phase<<" binary="<<binary<<'\n';return 1;
            }
        }
        writer.list=makeList(); writer.accepted=false; output.setReferenceResolver({});
        if(!output.write(&writer,DRW::AC1027,binary)||!writer.accepted||!readSnapshot(path,0u)
            ||writer.list.handle!=0xD00u||writer.list.name!="caller-name")return 4;
        // A separate writer may complete while this record's resolver is active.
        const auto otherPath=dir/(binary?"other-binary.dxf":"other-ascii.dxf");
        dxfRW other(otherPath.string().c_str()); Writer otherWriter(other);
        other.setHandleRemap({{0xD00u,0xAD00u}});
        other.setReferenceResolver([](std::uint32_t h){return h+0x10000u;});
        writer.list=makeList(); bool nestedOther=false;
        output.setReferenceResolver([&](std::uint32_t h) {
            if(h==0xA00u && !nestedOther) {
                nestedOther=true;
                if(!other.write(&otherWriter,DRW::AC1027,binary)||!otherWriter.accepted) return 0u;
            }
            return h+0x10000u;
        });
        if(!output.write(&writer,DRW::AC1027,binary)||!nestedOther||!readSnapshot(path)||!readSnapshot(otherPath))return 5;
        // A->B->A remains rejected even while B has its own resolver scope.
        writer.list=makeList(); otherWriter.list=makeList(); bool bounce=false,refused=false,otherOk=false;
        other.setReferenceResolver([&](std::uint32_t h) {
            if(h==0xA00u && !bounce) {bounce=true;DRW_Line nested;nested.handle=0xE002u;refused=!output.writeLine(&nested);}
            return h+0x10000u;
        });
        output.setReferenceResolver([&](std::uint32_t h) {
            if(h==0xA00u && !bounce) otherOk=other.write(&otherWriter,DRW::AC1027,binary)&&otherWriter.accepted;
            return h+0x10000u;
        });
        sentinel(path);
        if(output.write(&writer,DRW::AC1027,binary)||!bounce||!refused||!otherOk||!unchanged(path))return 6;
        // A caller may redirect a public variant content pointer away from
        // its owned backing value; the ordinary copy constructor would then
        // change the value. Refuse this lossy snapshot atomically.
        for(int redirected:{0,1}) {
            writer.list=makeList();std::string external=redirected?"replacement":"{ACAD_REACTORS";
            if(redirected)writer.list.extData[1]->content.s=&external;
            else writer.list.appData.front().front().content.s=&external;
            output.setReferenceResolver([](std::uint32_t h){return h+0x10000u;});
            sentinel(path);writer.accepted=false;
            if(output.write(&writer,DRW::AC1027,binary)||!unchanged(path))return 11;
            writer.list=makeList();
            if(!output.write(&writer,DRW::AC1027,binary)||!writer.accepted||!readSnapshot(path))return 12;
        }
        // The legacy inline adopted-raw return bypasses setup preflight.
        // Detect its vector swap even for consumers built with old headers.
        {
            const auto rawPath=dir/(binary?"raw-binary.dxf":"raw-ascii.dxf");
            dxfRW rawOutput(rawPath.string().c_str()); Writer rawWriter(rawOutput);
            rawOutput.setHandleRemap({{0xD00u,0xAD00u}});rawWriter.precedingLine=true;
            DRW_RawDxfSection raw;raw.m_name="TEST_PRIVATE";raw.m_version=DRW::AC1027;
            raw.m_groups={{0,std::string("TEST_RECORD")},{1,std::string("important")}};
            std::vector<DRW_RawDxfSection> input{raw},returned;
            if(!rawOutput.adoptRawDxfSectionsForWrite(std::move(input)))return 13;
            bool swapped=false;
            rawOutput.setReferenceResolver([&](std::uint32_t h) {
                if(h==0xA00u&&!swapped){swapped=true;rawOutput.returnAdoptedRawDxfSections(returned);}
                return h==0xD00u ? 0xAD00u : h+0x10000u;
            });
            sentinel(rawPath);
            if(rawOutput.write(&rawWriter,DRW::AC1027,binary)||!swapped||returned.size()!=1u
               ||!rawWriter.lineAccepted||!unchanged(rawPath))return 14;
            // The callback owns the returned carrier; put it back before retry.
            if(!rawOutput.adoptRawDxfSectionsForWrite(std::move(returned)))return 15;
            rawWriter.list=makeList();rawWriter.accepted=false;
            rawOutput.setReferenceResolver([](std::uint32_t h){return h==0xD00u?0xAD00u:h+0x10000u;});
            if(!rawOutput.write(&rawWriter,DRW::AC1027,binary)||!rawWriter.accepted)return 16;
            std::ifstream in(rawPath,std::ios::binary);const std::string bytes{std::istreambuf_iterator<char>(in),{}};
            if(bytes.find("TEST_PRIVATE")==std::string::npos)return 17;
        }
        // The analogous legacy GROUP return moves/clears an active input
        // vector; detect that transfer and require the caller to re-adopt it.
        {
            const auto groupPath=dir/(binary?"group-binary.dxf":"group-ascii.dxf");
            dxfRW groupOutput(groupPath.string().c_str());Writer groupWriter(groupOutput);
            groupOutput.setHandleRemap({{0xD00u,0xAD00u}});
            std::list<DRW_Group> groups;groups.emplace_back();groups.back().handle=0xE100u;
            groups.back().name="INPUT_GROUP";
            if(!groupOutput.adoptGroupsForWrite(groups))return 18;
            bool returned=false;
            groupOutput.setReferenceResolver([&](std::uint32_t h) {
                if(h==0xA00u&&!returned){returned=true;groupOutput.returnAdoptedGroupsForWrite(groups);}
                return h+0x10000u;
            });
            sentinel(groupPath);
            if(groupOutput.write(&groupWriter,DRW::AC1027,binary)||!returned||!unchanged(groupPath))return 19;
            if(!groupOutput.adoptGroupsForWrite(groups))return 20;
            groupWriter.list=makeList();groupWriter.accepted=false;
            groupOutput.setReferenceResolver([](std::uint32_t h){return h+0x10000u;});
            if(!groupOutput.write(&groupWriter,DRW::AC1027,binary)||!groupWriter.accepted)return 21;
            std::ifstream in(groupPath,std::ios::binary);const std::string bytes{std::istreambuf_iterator<char>(in),{}};
            if(bytes.find("INPUT_GROUP")==std::string::npos)return 22;
        }
        for(int mode:{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14}) {
            writer.list=makeList();writer.precedingLine=(mode==7);writer.lineAccepted=false;sentinel(path);int seen=0;bool nestedRefused=false;
            output.setReferenceResolver([&](std::uint32_t h) -> std::uint32_t {
                if(h==0xA00u) {
                    ++seen;
                    if(mode==0 && seen==2)return h+0x10001u;
                    if(mode==1)throw std::runtime_error("injected resolver failure");
                    if(mode==2 && seen==1) { DRW_Line nested;nested.handle=0xE001u;nestedRefused=!output.writeLine(&nested); }
                    if(mode==3 && seen==1) nestedRefused=!output.writeLightList(&writer.list);
                    if(mode==4 && seen==1) { auto nested=makeList();nested.handle=0xE002u;nestedRefused=!output.writeLightList(&nested); }
                    if(mode==5 && seen==1) output.setReferenceResolver({});
                    if(mode==6 && seen==1) output.setReferenceResolver([](std::uint32_t source){return source;});
                    if(mode==7 && seen==1) output.setHandleRemap({{0xD00u,0xBD00u}});
                    if(mode==8 && seen==1) output.setRootDictEntries(std::vector<std::pair<std::string,std::string>>{{"LATE","A00"}});
                    if(mode==9 && seen==1) output.setNamedDictObjects(std::vector<DRW_Dictionary>{});
                    if(mode==10 && seen==1) output.setRawDxfSections(std::vector<DRW_RawDxfSection>{});
                    if(mode==11 && seen==1) output.setResolvedTableStyleHandles(std::vector<std::uint32_t>{0xD00u});
                    if(mode==12 && seen==1) (void)output.reserveHandle(0xE003u);
                    if(mode==13 && seen==1) output.setBinary(!binary);
                    if(mode==14 && seen==1) output.setEllipseParts(-99);
                }
                return h==0xD00u ? 0xAD00u : h+0x10000u;
            });
            writer.accepted=false;
            if(output.write(&writer,DRW::AC1027,binary)||!unchanged(path)||seen==0 || (mode==7 && !writer.lineAccepted)
                || (mode>=2 && mode<=4 && !nestedRefused)
                ||writer.list.handle!=0xD00u||writer.list.name!="caller-name") {
                std::cerr<<"failure mode="<<mode<<" binary="<<binary<<'\n';return 2;
            }
            writer.list=makeList();writer.accepted=false;writer.precedingLine=false;
            output.setReferenceResolver([](std::uint32_t h){return h+0x10000u;});
            if(!output.write(&writer,DRW::AC1027,binary)||!writer.accepted||!readSnapshot(path))return 3;
        }
    }
    if (!allocationSweep(dir)) {
        std::cerr << "LIGHTLIST DXF allocation rollback/retry failed\n";
        return 30;
    }
    return 0;
}
