// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "intern/dwg_fixed_handles.h"
#include "lc3_compat_check.cpp"
#include <array>
#include <cstdlib>
#include <new>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
namespace { bool failLongStyleAllocation=false; bool longStyleAllocationTriggered=false; }
void* operator new(std::size_t size) {
    if (failLongStyleAllocation && size>=2049u && size<=2100u) {
        failLongStyleAllocation=false; longStyleAllocationTriggered=true; throw std::bad_alloc();
    }
    if (void* result=std::malloc(size ? size : 1u)) return result;
    throw std::bad_alloc();
}
void operator delete(void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
namespace {
bool sameHandle(const dwgHandle& a, const dwgHandle& b) {
    return a.code==b.code && a.size==b.size && a.ref==b.ref && a.ref64==b.ref64;
}
class InspectText final : public DRW_Text {
public: std::uint32_t observed=0u; dwgHandle layerHandle() const { return layerH; } dwgHandle ltypeHandle() const { return lTypeH; } std::uint16_t eedPage() const { return dwgEedCodePage; }
protected: bool encodeDwg(DRW::Version v,dwgBufferW* b,std::uint32_t n,dwgBufferW* t,dwgBufferW* h) override {
        observed=styleH.ref; return DRW_Text::encodeDwg(v,b,n,t,h);
    }
};
class InspectRText final : public DRW_RText {
public: std::uint32_t observed=999u;
protected: bool encodeDwg(DRW::Version v,dwgBufferW* b,std::uint32_t n,dwgBufferW* t,dwgBufferW* h) override {
        observed=styleH.ref; return DRW_RText::encodeDwg(v,b,n,t,h);
    }
};
class InspectArcText final : public DRW_ArcAlignedText {
public: std::uint32_t observed=999u;
protected: bool encodeDwg(DRW::Version v,dwgBufferW* b,std::uint32_t n,dwgBufferW* t,dwgBufferW* h) override {
        observed=styleH.ref; return DRW_ArcAlignedText::encodeDwg(v,b,n,t,h);
    }
};
class InspectAttrib final : public DRW_Attrib {
public: std::uint32_t observed=999u;
protected: bool encodeDwg(DRW::Version,dwgBufferW*,std::uint32_t,dwgBufferW*,dwgBufferW*) override { observed=styleH.ref; return false; }
};
class InspectAttdef final : public DRW_Attdef {
public: std::uint32_t observed=999u;
protected: bool encodeDwg(DRW::Version,dwgBufferW*,std::uint32_t,dwgBufferW*,dwgBufferW*) override { observed=styleH.ref; return false; }
};
class InspectMText final : public DRW_MText {
public: std::uint32_t observed=999u;
protected: bool encodeDwg(DRW::Version,dwgBufferW*,std::uint32_t,dwgBufferW*,dwgBufferW*) override { observed=styleH.ref; return false; }
};
class Writer final: public LC3CompatCheck {
public:
    explicit Writer(dwgRW& w,DRW::Version v):out(w),revision(v) {
        style.name="CUSTOM"; style.font="arial.ttf";
        style.xrefBlockHandle.code=DRW::DwgHardPointer;
        style.setDwgCommonObjectState(0,1,false);
        const std::array<std::string,4> names{{"CUSTOM","CuStOm","CUSTOM","MISSING"}};
        for(std::size_t i=0;i<items.size();++i) {
            items[i].style=names[i]; items[i].height=2.; items[i].text=std::to_string(i);
            items[i].basePoint={3.,4.,0.};items[i].secPoint={6.,7.,0.};
            items[i].styleH.code=DRW::DwgHardPointer;
        }
        items[3].style=std::string(2048u,'q');
        items[2].styleH.ref=DRW::DwgStandardTextStyleHandle;
        items[2].styleH.size=1u;
    }
    void writeDwgClasses() override {
        if (revision==DRW::AC1024)
            classesOk=out.registerDwgEntityClassInstance(DRW_RText::kDwgClassNum,0xed00u)
                && out.registerDwgEntityClassInstance(DRW_ArcAlignedText::kDwgClassNum,0xed01u);
        else classesOk=true;
    }
    void writeTextstyles() override { styleOk=out.addTextstyle(&style); }
    void writeEntities() override {
        ok=false;
        auto& first=items[0]; const dwgHandle original=first.styleH;
        first.parentHandle=0x0BADu;
        if(out.writeText(&first) || !sameHandle(first.styleH,original)) return;
        first.parentHandle=DRW::NoHandle;
        auto& longName=items[3]; const auto oldHandle=longName.handle;
        const auto oldLayer=longName.layerHandle(), oldLtype=longName.ltypeHandle(), oldStyle=longName.styleH;
        const auto oldOwner=longName.parentHandle; const auto oldEed=longName.eedPage();
        bool caught=false; failLongStyleAllocation=true; longStyleAllocationTriggered=false;
        try { (void)out.writeText(&longName); } catch (const std::bad_alloc&) { caught=true; }
        failLongStyleAllocation=false;
        if(!caught || !longStyleAllocationTriggered || longName.handle!=oldHandle
           || longName.parentHandle!=oldOwner || longName.eedPage()!=oldEed
           || !sameHandle(longName.layerHandle(),oldLayer) || !sameHandle(longName.ltypeHandle(),oldLtype)
           || !sameHandle(longName.styleH,oldStyle)) { std::cerr<<"allocation control caught="<<caught<<" triggered="<<longStyleAllocationTriggered<<" handle="<<longName.handle<<" old="<<oldHandle<<"\n"; return; }
        for(auto* spoof : {static_cast<DRW_Text*>(&attrib),static_cast<DRW_Text*>(&attdef),static_cast<DRW_Text*>(&mtext)}) {
            spoof->eType=DRW::TEXT;spoof->style="CUSTOM";
            if(out.writeText(spoof) || spoof->styleH.ref!=0u) {std::cerr<<"spoof write unexpected\n"; return;}
        }
        if(attrib.observed!=0u || attdef.observed!=0u || mtext.observed!=0u) {std::cerr<<"spoof observed "<<attrib.observed<<" "<<attdef.observed<<" "<<mtext.observed<<"\n"; return;}
        for(auto& item:items) {
            const dwgHandle before=item.styleH;
            if(!out.writeText(&item) || !sameHandle(item.styleH,before)) return;
        }
        custom.style="CUSTOM"; custom.text="custom-subclass";custom.height=2.;
        if(!out.writeText(&custom) || custom.observed!=style.handle || custom.styleH.ref!=0u) return;
        if(revision==DRW::AC1024) {
            rtext.handle=0xed00u; rtext.style="CUSTOM";rtext.text="rtext";rtext.height=2.;
            arctext.handle=0xed01u; arctext.style="CUSTOM";arctext.text="arctext";arctext.m_radius=3.;
            if(!out.writeRText(&rtext) || !out.writeArcAlignedText(&arctext)
               || rtext.observed!=0u || arctext.observed!=0u
               || rtext.styleH.ref!=0u || arctext.styleH.ref!=0u) return;
        }
        DRW_Line line;line.basePoint={10.,11.,0.};line.secPoint={12.,13.,0.};
        ok=out.writeLine(&line);
    }
    bool finalizeDwgWrite() override {return styleOk && classesOk && ok;}
    dwgRW& out;DRW::Version revision;DRW_Textstyle style;std::array<InspectText,4> items;InspectText custom;InspectRText rtext;InspectArcText arctext;InspectAttrib attrib;InspectAttdef attdef;InspectMText mtext;bool styleOk=false,classesOk=false,ok=false;
};
class Reader final: public LC3CompatCheck {
public:
    void addTextStyle(const DRW_Textstyle& v) override {styles[v.handle]=v.name;}
    void addText(const DRW_Text& v) override {if(v.text=="0"||v.text=="1"||v.text=="2"||v.text=="3") texts[v.text]=v.styleH.ref;}
    void addLine(const DRW_Line& v) override {if(v.basePoint.x==10. && v.secPoint.y==13.)++lines;}
    std::map<std::uint32_t,std::string> styles;
    std::map<std::string,std::uint32_t> texts;
    int lines=0;
};
}
int main(){
    auto path=std::filesystem::temp_directory_path()/("dwg-text-style-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        dwgRW w(path.string().c_str());Writer author(w,version);
        if(!w.write(&author,version,true)||!author.ok){std::cerr<<"write failed "<<static_cast<int>(version)<<'\n';return 1;}
        dwgRW r(path.string().c_str());Reader reader;
        if(!r.read(&reader,false)||reader.texts.size()!=4 || reader.lines!=1 || reader.styles[reader.texts["0"]]!="CUSTOM" || reader.styles[reader.texts["1"]]!="CUSTOM" || reader.styles[reader.texts["2"]]!="STANDARD" || reader.styles[reader.texts["3"]]!="STANDARD") {
            std::cerr<<"readback failed "<<static_cast<int>(version)<<'\n';return 1;
        }
    }
}
