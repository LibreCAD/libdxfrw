// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfwriter.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

struct Capture final : LC3CompatCheck {
    void addLayer(const DRW_Layer& value) override {
        if(value.name=="COLORCHECK") { selected=value; ++count; }
        if(value.name=="FOLLOWING") { following=value; ++followingCount; }
    }
    DRW_Layer selected,following;int count=0,followingCount=0;
};
struct Export final : LC3CompatCheck {
    explicit Export(dxfRW& writer):out(writer){}
    void writeLayers() override { ok=out.writeLayer(&selected)&&(!includeFollowing || out.writeLayer(&following)); }
    bool finalizeDxfWrite() override {return ok;}
    dxfRW& out;DRW_Layer selected,following;bool ok=false,includeFollowing=true;
};
struct Field {int code;int number;const char* text;};
static bool createInput(const std::filesystem::path& path,bool binary,const std::vector<Field>& fields) {
    std::ofstream stream(path,std::ios::binary|std::ios::trunc);
    if(binary)stream<<"AutoCAD Binary DXF\r\n"<<static_cast<char>(26)<<'\0';
    std::unique_ptr<dxfWriter> w=binary
        ? std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterBinary>(&stream))
        : std::unique_ptr<dxfWriter>(std::make_unique<dxfWriterAscii>(&stream));
    w->setVersion("AC1027",true);
    if(!w->writeString(0,"SECTION")||!w->writeString(2,"HEADER")||!w->writeString(9,"$ACADVER")
       ||!w->writeString(1,"AC1027")||!w->writeString(0,"ENDSEC")||!w->writeString(0,"SECTION")
       ||!w->writeString(2,"TABLES")||!w->writeString(0,"TABLE")||!w->writeString(2,"LAYER")
       ||!w->writeInt16(70,2)||!w->writeString(0,"LAYER")||!w->writeString(5,"2F")
       ||!w->writeString(100,"AcDbSymbolTableRecord")||!w->writeString(100,"AcDbLayerTableRecord")
       ||!w->writeString(2,"COLORCHECK")||!w->writeInt16(70,0)||!w->writeString(6,"CONTINUOUS")) return false;
    for(const auto& f:fields) {
        if(f.code==62&&!w->writeInt16(62,f.number))return false;
        if(f.code==420&&!w->writeInt32(420,f.number))return false;
        if(f.code==430&&!w->writeUtf8String(430,f.text))return false;
    }
    return w->writeString(0,"LAYER")&&w->writeString(5,"30")&&w->writeString(100,"AcDbSymbolTableRecord")
        &&w->writeString(100,"AcDbLayerTableRecord")&&w->writeString(2,"FOLLOWING")&&w->writeInt16(70,0)
        &&w->writeInt16(62,3)&&w->writeString(6,"CONTINUOUS")&&w->writeString(0,"ENDTAB")
        &&w->writeString(0,"ENDSEC")&&w->writeString(0,"EOF")&&!w->hasWriteError();
}
static bool check(Capture& capture,int aci,int rgb,const std::string& name) {
    return capture.count==1&&capture.followingCount==1&&capture.selected.color==aci
        &&capture.selected.color24==rgb&&capture.selected.colorName==name
        &&capture.following.color==3&&capture.following.color24==-1&&capture.following.colorName.empty();
}
static bool read(const std::filesystem::path& p,Capture& capture) {
    dxfRW in(p.string().c_str());return in.read(&capture,false);
}
int main(int argc,char**argv){
    if(argc==2||argc==3){
        Capture c;if(!read(argv[1],c)||c.count!=1||c.selected.color!=-148||c.selected.color24!=1193046)return 1;
        const auto tmp=std::filesystem::temp_directory_path()/("layer-color-physical-"+
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(tmp);
        struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{tmp};
        for(bool binary:{false,true}) {
            auto path=tmp/(binary?"binary.dxf":"ascii.dxf");dxfRW output(path.string().c_str());
            Export adapter(output);adapter.selected=c.selected;adapter.includeFollowing=false;
            if(!output.write(&adapter,DRW::AC1027,binary)||!adapter.ok)return 15;
            Capture reread;if(!read(path,reread)||reread.count!=1||reread.selected.color!=-148
                ||reread.selected.color24!=1193046)return 16;
            if(argc==3){std::filesystem::create_directories(argv[2]);std::filesystem::copy_file(path,
                std::filesystem::path(argv[2])/(binary?"binary.dxf":"ascii.dxf"),std::filesystem::copy_options::overwrite_existing);}
        }
        return 0;
    }
    if(argc!=1)return 2;
    const auto dir=std::filesystem::temp_directory_path()/("layer-color-order-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
    for(bool binary:{false,true}) {
        auto path=dir/(binary?"input-binary.dxf":"input-ascii.dxf");
        std::array<int,3> order{{0,1,2}};
        do {
            const std::array<Field,3> values{{{62,-148,nullptr},{420,1193046,nullptr},{430,0,"BOOK$RED"}}};
            std::vector<Field> fields;for(int index:order)fields.push_back(values[static_cast<std::size_t>(index)]);
            if(!createInput(path,binary,fields))return 3;
            Capture capture;if(!read(path,capture)||!check(capture,-148,1193046,"BOOK$RED"))return 4;
            auto outputPath=dir/(binary?"out-binary.dxf":"out-ascii.dxf");
            dxfRW output(outputPath.string().c_str());Export adapter(output);
            adapter.selected=capture.selected;adapter.following=capture.following;
            if(!output.write(&adapter,DRW::AC1027,binary)||!adapter.ok)return 5;
            Capture after;if(!read(outputPath,after)||!check(after,-148,1193046,"BOOK$RED"))return 6;
        } while(std::next_permutation(order.begin(),order.end()));
        const std::vector<std::vector<Field>> cases{
            {{420,0,nullptr},{62,7,nullptr}},
            {{62,-7,nullptr}},
            {{430,0,"BOOK$ONLY"},{62,-7,nullptr}},
            {{430,0,"FIRST"},{62,-148,nullptr},{420,0,nullptr},{430,0,"SECOND"},{62,5,nullptr},{420,1193046,nullptr}}
        };
        const std::array<int,4> acis{{7,-7,-7,5}},rgbs{{0,-1,-1,1193046}};
        const std::array<std::string,4> names{{"","","BOOK$ONLY","SECOND"}};
        for(std::size_t i=0;i<cases.size();++i){
            if(!createInput(path,binary,cases[i]))return 7;
            Capture capture;if(!read(path,capture)||!check(capture,acis[i],rgbs[i],names[i]))return 8;
            auto outputPath=dir/(binary?"case-binary.dxf":"case-ascii.dxf");
            dxfRW output(outputPath.string().c_str());Export adapter(output);
            adapter.selected=capture.selected;adapter.following=capture.following;
            if(!output.write(&adapter,DRW::AC1027,binary)||!adapter.ok)return 9;
            Capture after;if(!read(outputPath,after)||!check(after,acis[i],rgbs[i],names[i]))return 10;
        }
        // A failed parse resets retained state before a corrected same-reader retry.
        if(!createInput(path,binary,{{420,0,nullptr},{62,300,nullptr}}))return 11;
        dxfRW retry(path.string().c_str());Capture failed;if(retry.read(&failed,false))return 12;
        if(!createInput(path,binary,{{420,0,nullptr},{62,7,nullptr}}))return 13;
        Capture corrected;if(!retry.read(&corrected,false)||!check(corrected,7,0,""))return 14;
    }
    return 0;
}
