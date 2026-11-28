// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgbuffer.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <vector>
namespace {
template<class T> class Probe : public T {public: using T::encodeDwg; using DRW_Hatch::encodeDwgBoundaryData; int rawType() const { return this->oType; }};
// Even direct callers of the boundary helper must encode the selected tag.
template<class T> bool tagDispatch(DRW::Version version) {
    Probe<T> hatch; auto path=std::make_shared<DRW_HatchLoop>(1);
    auto ellipse=std::make_shared<DRW_Ellipse>(); ellipse->eType=DRW::LINE;
    path->objlist={ellipse}; path->update(); hatch.appendLoop(path);
    dwgBufferW out; if (!hatch.encodeDwgBoundaryData(version,&out,true)) return false;
    dwgBuffer in(out.data().data(),out.data().size());
    return in.getBitLong()==1 && in.getBitLong()==1 && in.getBitLong()==1 && in.getRawChar8()==1u && in.isGood();
}
template<class T> void spoofLine(std::vector<std::shared_ptr<DRW_Entity>>& v) {
    auto p=std::make_shared<T>(); p->eType=DRW::LINE; v.push_back(p);
}
struct CustomLine : DRW_Line {};
struct CustomSpline : DRW_Spline {};
std::vector<std::shared_ptr<DRW_Entity>> invalid() {
    std::vector<std::shared_ptr<DRW_Entity>> v{std::make_shared<DRW_Ray>(),std::make_shared<DRW_Xline>(),std::make_shared<DRW_3DLine>(),std::make_shared<DRW_Trace>(),std::make_shared<DRW_Text>(),std::make_shared<DRW_Image>(),std::make_shared<DRW_Helix>()};
    auto line=std::make_shared<DRW_Line>(); line->eType=DRW::ARC; v.push_back(line);
    auto arc=std::make_shared<DRW_Arc>(); arc->eType=DRW::SPLINE; v.push_back(arc);
    spoofLine<DRW_Ellipse>(v); spoofLine<DRW_3DLine>(v); spoofLine<DRW_Ray>(v);
    spoofLine<DRW_Xline>(v); spoofLine<DRW_Trace>(v); spoofLine<DRW_Solid>(v);
    spoofLine<DRW_3Dface>(v); spoofLine<DRW_Text>(v); spoofLine<DRW_Attrib>(v);
    spoofLine<DRW_Attdef>(v); spoofLine<DRW_MText>(v); spoofLine<DRW_RText>(v); spoofLine<DRW_ArcAlignedText>(v);
    spoofLine<DRW_Image>(v); spoofLine<DRW_Wipeout>(v);
    auto helix=std::make_shared<DRW_Helix>(); helix->eType=DRW::SPLINE; v.push_back(helix);
    v.push_back(nullptr); return v;
}
std::shared_ptr<DRW_Spline> spline() {
    auto s=std::make_shared<DRW_Spline>(); s->degree=2; s->knotslist={0.,0.,0.,1.,1.,1.};
    s->controllist={std::make_shared<DRW_Coord>(0.,0.,0.),std::make_shared<DRW_Coord>(1.,2.,0.),std::make_shared<DRW_Coord>(3.,0.,0.)}; return s;
}
template<class T> bool direct(DRW::Version version) {
    Probe<T> hatch; hatch.name="SOLID"; hatch.solid=1;
    auto loop=std::make_shared<DRW_HatchLoop>(1); hatch.appendLoop(loop);
    for (const auto& entity : invalid()) {
        loop->objlist={entity}; loop->update(); dwgBufferW body, text, handles;
        body.putRawChar8(0xa5); text.putRawChar8(0x5a); handles.putRawChar8(0x3c);
        body.putBit(1u); text.putBit(0u); handles.putBit(1u);
        const auto oldBody=body.data(), oldText=text.data(), oldHandles=handles.data();
        const auto oldBodyBit=body.bitPos(), oldTextBit=text.bitPos(), oldHandlesBit=handles.bitPos();
        const auto oldType=hatch.rawType(); const auto oldHandle=hatch.handle;
        if (hatch.encodeDwg(version,&body,0,&text,&handles) || body.data()!=oldBody || text.data()!=oldText || handles.data()!=oldHandles || body.bitPos()!=oldBodyBit || text.bitPos()!=oldTextBit || handles.bitPos()!=oldHandlesBit || hatch.rawType()!=oldType || hatch.handle!=oldHandle) return false;
        loop->objlist={std::make_shared<DRW_Line>()}; loop->update();
        if (!hatch.encodeDwg(version,&body,0,&text,&handles)) return false;
    }
    for (const auto& entity : std::vector<std::shared_ptr<DRW_Entity>>{std::make_shared<DRW_Line>(),std::make_shared<DRW_Arc>(),std::make_shared<DRW_Ellipse>(),spline(),std::make_shared<CustomLine>(),std::make_shared<CustomSpline>()}) {
        Probe<T> candidate; candidate.name="SOLID"; candidate.solid=1;
        auto path=std::make_shared<DRW_HatchLoop>(1);path->objlist={entity};path->update();candidate.appendLoop(path);
        dwgBufferW body, text, handles; if(!candidate.encodeDwg(version,&body,0,&text,&handles)) return false;
    }
    return true;
}
class Writer final:public LC3CompatCheck {
public:
    explicit Writer(dwgRW& w):out(w){}
    void writeEntities() override {
        ok=false; DRW_Hatch h;h.name="SOLID";h.solid=1;auto loop=std::make_shared<DRW_HatchLoop>(1);h.appendLoop(loop);
        for(const auto& entity:invalid()) {
            loop->objlist={entity};loop->update();const auto before=h.handle;
            if(out.writeHatch(&h) || h.handle!=before) return;
        }
        loop->objlist={std::make_shared<DRW_Line>()};loop->update();
        if(!out.writeHatch(&h))return;
        DRW_Line line;line.basePoint={10.,11.,0.};line.secPoint={12.,13.,0.};
        ok=out.writeLine(&line);
    }
    bool finalizeDwgWrite() override{return ok;}
    dwgRW& out;bool ok=false;
};
class Reader final:public LC3CompatCheck {
public: void addHatch(const DRW_Hatch*) override{++hatches;}
    void addLine(const DRW_Line& l) override{if(l.basePoint.x==10.&&l.secPoint.y==13.)++lines;}
    int hatches=0,lines=0;
};
}
int main(){
    auto path=std::filesystem::temp_directory_path()/("hatch-subtypes-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{path};
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        if(!direct<DRW_Hatch>(version)||!direct<DRW_MPolygon>(version)||!tagDispatch<DRW_Hatch>(version)||!tagDispatch<DRW_MPolygon>(version)) {std::cerr<<"direct validation failure "<<static_cast<int>(version)<<'\n';return 1;}
        dwgRW w(path.string().c_str());Writer writer(w);if(!w.write(&writer,version,true)||!writer.ok){std::cerr<<"write failure\n";return 1;}
        dwgRW r(path.string().c_str());Reader reader;if(!r.read(&reader,false)||reader.hatches!=1||reader.lines!=1){std::cerr<<"readback failure\n";return 1;}
    }
}
