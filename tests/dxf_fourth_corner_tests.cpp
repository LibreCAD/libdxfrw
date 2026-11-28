// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "intern/dxfwriter.h"
#include "libdxfrw.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

struct Capture : LC3CompatCheck {
    void addTrace(const DRW_Trace& value) override {save(value);}
    void addSolid(const DRW_Solid& value) override {save(value);}
    void add3dFace(const DRW_3Dface& value) override {save(value);}
    void addLine(const DRW_Line&) override {++lines;}
    void save(const DRW_Trace& value) {++count;third=value.thirdPoint;fourth=value.fourPoint;}
    int count=0,lines=0;DRW_Coord third,fourth;
};
struct ManualWriter : LC3CompatCheck {
    explicit ManualWriter(dxfRW& value,const char* type):writer(value),kind(type) {
        for(DRW_Trace* entity : {static_cast<DRW_Trace*>(&solid), &trace, static_cast<DRW_Trace*>(&face)}) {
            entity->basePoint={1.,2.,3.};entity->secPoint={2.,3.,4.};
            entity->thirdPoint={3.,4.,5.};entity->fourPoint={0.,0.,0.};
        }
    }
    void writeEntities() override {
        if(kind=="SOLID")accepted=writer.writeSolid(&solid);
        else if(kind=="TRACE")accepted=writer.writeTrace(&trace);
        else accepted=writer.write3dface(&face);
    }
    dxfRW& writer;std::string kind;bool accepted=false;DRW_Solid solid;DRW_Trace trace;DRW_3Dface face;
};
bool author(const std::filesystem::path& path,bool binary,const char* kind,int mode,
            const std::array<int,3>& order) {
    std::ofstream stream(path,std::ios::binary|std::ios::trunc);
    if(binary)stream.write("AutoCAD Binary DXF\r\n\032\0",22);
    std::unique_ptr<dxfWriter> tags=binary
        ?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&stream))
        :std::unique_ptr<dxfWriter>(new dxfWriterAscii(&stream));
    tags->setVersion("AC1027",true);
    tags->writeString(0,"SECTION");tags->writeString(2,"HEADER");
    tags->writeString(9,"$ACADVER");tags->writeString(1,"AC1027");
    tags->writeString(0,"ENDSEC");tags->writeString(0,"SECTION");tags->writeString(2,"ENTITIES");
    tags->writeString(0,kind);tags->writeString(100,"AcDbEntity");tags->writeString(8,"0");
    tags->writeString(100,std::string(kind)=="3DFACE"?"AcDbFace":"AcDbTrace");
    if(mode==1)tags->writeDouble(33,9.); // Present fourth Z without X/Y, before third corner.
    for(int i=0;i<3;++i){tags->writeDouble(10+i,i+1.);tags->writeDouble(20+i,i+2.);tags->writeDouble(30+i,i+3.);}
    if(mode==2)tags->writeDouble(33,9.); // Same invalid fourth-Z-only record, after third corner.
    if(mode==3)for(int component:order)tags->writeDouble(13+10*component,component==0?7.:component==1?8.:9.);
    if(mode==4){tags->writeDouble(13,0.);tags->writeDouble(23,0.);} // Omitted default Z.
    if(mode==5){tags->writeDouble(13,7.);tags->writeDouble(33,9.);} // X/Z only.
    if(mode==6){tags->writeDouble(23,8.);tags->writeDouble(33,9.);} // Y/Z only.
    tags->writeString(0,"LINE");tags->writeString(100,"AcDbEntity");tags->writeString(8,"0");
    tags->writeString(100,"AcDbLine");tags->writeDouble(10,11.);tags->writeDouble(20,12.);
    tags->writeDouble(11,14.);tags->writeDouble(21,15.);
    tags->writeString(0,"ENDSEC");tags->writeString(0,"EOF");
    return !tags->hasWriteError()&&stream.good();
}
bool point(const DRW_Coord& value,double x,double y,double z){return value.x==x&&value.y==y&&value.z==z;}
int main() {
    const auto path=std::filesystem::temp_directory_path()/
        ("libdxfrw-fourth-corner-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
    for(bool binary:{false,true})for(const char* kind:{"SOLID","TRACE","3DFACE"}) {
        dxfRW reader(path.string().c_str());
        const std::array<int,3> original{{0,1,2}};
        for(int invalid:{1,2,5,6}) {
            Capture capture;
            if(!author(path,binary,kind,invalid,original)||reader.read(&capture,false)||capture.count||capture.lines) {
                std::cerr<<kind<<" invalid fourth corner accepted, mode="<<invalid<<" binary="<<binary<<'\n';return 1;
            }
            if(!author(path,binary,kind,0,original)||!reader.read(&capture,false)
                ||capture.count!=1||capture.lines!=1||!point(capture.third,3.,4.,5.)
                ||!point(capture.fourth,3.,4.,5.)) {
                std::cerr<<kind<<" triangle retry failed\n";return 1;
            }
        }
        std::array<int,3> permutation{{0,1,2}};
        do {
            Capture capture;
            if(!author(path,binary,kind,3,permutation)||!reader.read(&capture,false)
                ||capture.count!=1||capture.lines!=1||!point(capture.fourth,7.,8.,9.)) {
                std::cerr<<kind<<" fourth corner permutation failed\n";return 1;
            }
        } while(std::next_permutation(permutation.begin(),permutation.end()));
        Capture zero;
        if(!author(path,binary,kind,4,original)||!reader.read(&zero,false)||zero.count!=1
            ||zero.lines!=1||!point(zero.fourth,0.,0.,0.)) {
            std::cerr<<kind<<" explicit fourth XY with omitted default Z failed\n";return 1;
        }
        dxfRW writer(path.string().c_str());ManualWriter manual(writer,kind);
        if(manual.kind=="3DFACE") {
            {std::ofstream sentinel(path,std::ios::binary|std::ios::trunc);sentinel<<"unchanged";}
            manual.face.invisibleflag=16;
            if(writer.write(&manual,DRW::AC1027,binary)||manual.accepted||manual.face.invisibleflag!=16) return 1;
            {std::ifstream sentinel(path,std::ios::binary);std::string value((std::istreambuf_iterator<char>(sentinel)),{});
                if(value!="unchanged")return 1;}
            manual.face.invisibleflag=0;
        }
        if(!writer.write(&manual,DRW::AC1027,binary)||!manual.accepted) {
            std::cerr<<kind<<" manual writer retry failed\n";return 1;
        }
        Capture manualRead;
        if(!reader.read(&manualRead,false)||manualRead.count!=1||!point(manualRead.fourth,0.,0.,0.)) {
            std::cerr<<kind<<" manual origin corner changed\n";return 1;
        }
    }
    std::cout<<"DXF fourth-corner validation and retry PASS\n";return 0;
}
