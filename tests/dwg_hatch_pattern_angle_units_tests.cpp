// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter21.h"
#include "drw_header.h"
#include <algorithm>
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <fstream>
#include <iostream>
#include <limits>
#include <iterator>
#include <memory>

class DwgBlockRecordXrefWireTestAccess {public:static bool frame(dwgWriter15& w,std::uint32_t h,std::vector<std::uint8_t>& b){auto i=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[h](const auto& x){return x.first==h;});if(i==w.m_objectMap.end())return false;const auto& data=w.buffer();dwgBuffer f(data.data()+i->second,data.size()-i->second);auto n=f.getModularShort();auto start=f.getPosition();if(!f.isGood()||n<=0||start+static_cast<std::size_t>(n)+2>data.size()-i->second)return false;b.assign(data.begin()+i->second+start,data.begin()+i->second+start+n);return true;}};
namespace {
template<class T> class BoundaryProbe : public T {
public:
    using DRW_Hatch::encodeDwgBoundaryData;
    using T::encodeDwg;
    using T::parseDwg;
    bool common(dwgBuffer& b){return DRW_Entity::parseDwgCommon(DRW::AC1021,&b,0u);}
};

template<class T> bool checkBoundary(DRW::Version version) {
    BoundaryProbe<T> h;
    auto loop=std::make_shared<DRW_HatchLoop>(1);
    auto line=std::make_shared<DRW_Line>(); line->basePoint={0.,1.,0.}; line->secPoint={2.,3.,0.};
    auto arc=std::make_shared<DRW_Arc>(); arc->basePoint={4.,5.,0.}; arc->radious=2.; arc->staangle=.25; arc->endangle=1.5; arc->isccw=1;
    auto ellipse=std::make_shared<DRW_Ellipse>(); ellipse->basePoint={6.,7.,0.}; ellipse->secPoint={4.,0.,0.}; ellipse->ratio=.5; ellipse->staparam=.25; ellipse->endparam=1.5; ellipse->isccw=1;
    auto spline=std::make_shared<DRW_Spline>(); spline->degree=2; spline->knotslist={0.,0.,0.,1.,1.,1.};
    spline->controllist={std::make_shared<DRW_Coord>(0.,0.,0.),std::make_shared<DRW_Coord>(1.,2.,0.),std::make_shared<DRW_Coord>(3.,0.,0.)};
    loop->objlist={line,arc,ellipse,spline}; loop->m_boundaryHandles={0x51u,0x52u}; loop->update(); h.appendLoop(loop);
    dwgBufferW out;
    if (!h.encodeDwgBoundaryData(version,&out,true)) return false;
    out.putRawLong32(0xA1B2C3D4u);
    dwgBuffer in(out.data().data(),out.data().size());
    if (in.getBitLong()!=1 || in.getBitLong()!=1 || in.getBitLong()!=4) return false;
    if (in.getRawChar8()!=1u || in.getRawDouble()!=0. || in.getRawDouble()!=1.
        || in.getRawDouble()!=2. || in.getRawDouble()!=3.) return false;
    if (in.getRawChar8()!=2u || in.getRawDouble()!=4. || in.getRawDouble()!=5.
        || in.getBitDouble()!=2. || in.getBitDouble()!=.25 || in.getBitDouble()!=1.5 || in.getBit()!=1u) return false;
    if (in.getRawChar8()!=3u || in.getRawDouble()!=6. || in.getRawDouble()!=7.
        || in.getRawDouble()!=4. || in.getRawDouble()!=0. || in.getBitDouble()!=.5
        || in.getBitDouble()!=.25 || in.getBitDouble()!=1.5 || in.getBit()!=1u) return false;
    if (in.getRawChar8()!=4u || in.getBitLong()!=2 || in.getBit()!=0u || in.getBit()!=0u
        || in.getBitLong()!=6 || in.getBitLong()!=3) return false;
    for (double expected : {0.,0.,0.,1.,1.,1.}) if (in.getBitDouble()!=expected) return false;
    for (double expected : {0.,0.,1.,2.,3.,0.}) if (in.getRawDouble()!=expected) return false;
    if (version > DRW::AC1021) {
        if (in.getBitLong()!=0) return false;
        for (int i=0; i<4; ++i) if (in.getRawDouble()!=0.) return false;
    }
    return in.getBitLong()==2 && in.getRawLong32()==0xA1B2C3D4u && in.isGood();
}

static bool hasBD(const dwgBufferW& stream,double value) {
 dwgBufferW probe;probe.putBitDouble(value);const auto bit=[](const std::vector<std::uint8_t>& b,std::size_t i){return (b[i/8]>>(7-i%8))&1u;};
 if(probe.bitCount()>stream.bitCount())return false;
 for(std::size_t start=0;start+probe.bitCount()<=stream.bitCount();++start){bool match=true;for(std::size_t j=0;j<probe.bitCount();++j)if(bit(stream.data(),start+j)!=bit(probe.data(),j)){match=false;break;}if(match)return true;}return false;
}
template<class T> bool literal(DRW::Version version,bool isMPolygon) {
 BoundaryProbe<T> h;h.name="ANSI31";h.solid=0;h.angle=30./ARAD;h.scale=1.;DRW_Hatch::PatternLine p;p.angle=75./ARAD;h.patternLines.push_back(p);
 dwgBufferW b,s,handles;if(!h.encodeDwg(version,&b,0,&s,&handles))return false;
 (void)isMPolygon;
 return hasBD(b,30./ARAD)&&hasBD(b,75./ARAD);
}
template<class T> bool guards(DRW::Version version,bool isMPolygon) {
 BoundaryProbe<T> h;h.name="ANSI31";h.solid=0;h.angle=30./ARAD;h.scale=1.;DRW_Hatch::PatternLine p;p.angle=75./ARAD;h.patternLines.push_back(p);
 if(isMPolygon)return true;
 for(int field=0;field<2;++field)for(double v:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),30.*std::numeric_limits<double>::denorm_min()}) {
  auto& target=field?h.patternLines[0].angle:h.angle;double old=target;target=v;dwgBufferW b,s,handles;b.putRawChar8(0xa5);b.putBit(1);s.putRawChar8(0x5a);s.putBit(0);handles.putRawChar8(0x3c);handles.putBit(1);auto oldb=b.data(),olds=s.data(),oldh=handles.data();auto bb=b.bitCount(),sb=s.bitCount(),hb=handles.bitCount();if(h.encodeDwg(version,&b,0,&s,&handles)||b.data()!=oldb||s.data()!=olds||handles.data()!=oldh||b.bitCount()!=bb||s.bitCount()!=sb||handles.bitCount()!=hb)return false;target=old;if(!h.encodeDwg(version,&b,0,&s,&handles))return false;
 }
 return true;
}
bool mpolygonDwgAngleRoundTrip() {
    DRW_Header header;
    dwgWriter21 writer(nullptr, &header);
    if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader() || !writer.writeDwgClasses()
        || !writer.writeDwgObjects())
        return false;
    DRW_MPolygon source;
    source.name = "ANSI31";
    source.solid = 0;
    source.angle = 30.0 / ARAD;
    source.scale = 1.0;
    DRW_Hatch::PatternLine line;
    line.angle = 75.0 / ARAD;
    source.patternLines.push_back(line);
    if (!writer.encodeEntity(&source)) return false;
    std::vector<std::uint8_t> bytes;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, source.handle, bytes))
        return false;
    BoundaryProbe<DRW_MPolygon> parsed;
    dwgBuffer input(bytes.data(), bytes.size());
    return parsed.parseDwg(DRW::AC1021, &input, 0)
        && std::fabs(parsed.angle - 30.0 / ARAD) < 1e-10
        && parsed.patternLines.size() == 1u
        && std::fabs(parsed.patternLines.front().angle - 75.0 / ARAD) < 1e-10;
}

template<class T> bool widePatternLineCountRoundTrip() {
    constexpr std::uint32_t lineCount = 405;
    DRW_Header header;
    dwgWriter21 writer(nullptr, &header);
    if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
        || !writer.writeDwgHeader() || !writer.writeDwgClasses()
        || !writer.writeDwgObjects())
        return false;

    T source;
    source.name = "ANSI31";
    source.solid = 0;
    source.scale = 1.0;
    source.patternLines.reserve(lineCount);
    for (std::uint32_t i = 0; i < lineCount; ++i) {
        DRW_Hatch::PatternLine line;
        line.angle = static_cast<double>(i) * 0.5;
        line.baseX = static_cast<double>(i);
        line.baseY = -static_cast<double>(i);
        line.offsetX = 0.25;
        line.offsetY = -0.75;
        line.dashList.push_back(static_cast<double>(i) + 1.0);
        source.patternLines.push_back(std::move(line));
    }
    if (!writer.encodeEntity(&source))
        return false;

    std::vector<std::uint8_t> bytes;
    if (!DwgBlockRecordXrefWireTestAccess::frame(writer, source.handle, bytes))
        return false;
    BoundaryProbe<T> parsed;
    auto truncated = bytes;
    truncated.pop_back();
    dwgBuffer shortInput(truncated.data(), truncated.size());
    if (parsed.parseDwg(DRW::AC1021, &shortInput, 0)
        || !parsed.patternLines.empty())
        return false;

    dwgBuffer input(bytes.data(), bytes.size());
    if (!parsed.parseDwg(DRW::AC1021, &input, 0)
        || parsed.deflines != static_cast<int>(lineCount)
        || parsed.patternLines.size() != lineCount)
        return false;
    for (std::uint32_t i = 0; i < lineCount; ++i) {
        const auto& line = parsed.patternLines[i];
        if (line.angle != static_cast<double>(i) * 0.5
            || line.baseX != static_cast<double>(i)
            || line.baseY != -static_cast<double>(i)
            || line.offsetX != 0.25 || line.offsetY != -0.75
            || line.dashList.size() != 1u
            || line.dashList.front() != static_cast<double>(i) + 1.0)
            return false;
    }
    return true;
}

struct DerivedReject:DRW_Hatch {bool encodeDwg(DRW::Version,dwgBufferW*,std::uint32_t,dwgBufferW*,dwgBufferW*) override{return false;}};
struct IgnoredWriter:LC3CompatCheck {dwgRW& w;bool bad,derived,viaPoint;IgnoredWriter(dwgRW& x,bool b,bool d=false,bool point=false):w(x),bad(b),derived(d),viaPoint(point){}void writeEntities() override {DRW_Hatch hatch;DerivedReject custom;DRW_Hatch* h=derived?static_cast<DRW_Hatch*>(&custom):&hatch;h->name="ANSI31";h->solid=0;h->angle=bad?std::numeric_limits<double>::denorm_min():30./ARAD;h->scale=1.;DRW_Hatch::PatternLine p;p.angle=75./ARAD;h->patternLines.push_back(p);if(viaPoint)(void)w.writePoint(h);else (void)w.writeHatch(h);}};
class Writer final : public LC3CompatCheck {
public:
    explicit Writer(dwgRW& w):out(w) {}
    void writeEntities() override {
        DRW_Hatch h; h.name="ANSI31"; h.solid=0; h.angle=30./ARAD; h.scale=1.; DRW_Hatch::PatternLine pat;pat.angle=75./ARAD;pat.offsetX=1.;pat.offsetY=2.;h.patternLines.push_back(pat);
        auto loop=std::make_shared<DRW_HatchLoop>(1);
        auto ellipse=std::make_shared<DRW_Ellipse>(); ellipse->basePoint={3.,4.,0.}; ellipse->secPoint={4.,0.,0.};
        ellipse->ratio=.5; ellipse->staparam=.25; ellipse->endparam=1.5; ellipse->isccw=1;
        loop->objlist.push_back(nullptr); loop->update(); h.appendLoop(loop);
        const bool rejected=!out.writeHatch(&h) && h.handle==0;
        loop->objlist[0]=ellipse;
        const bool written=out.writeHatch(&h);
        DRW_Line next; next.basePoint={10.,11.,0.}; next.secPoint={12.,13.,0.};
        ok=rejected && written && out.writeLine(&next);
    }
    bool finalizeDwgWrite() override {return ok;}
    dwgRW& out; bool ok=false;
};
class Reader final : public LC3CompatCheck {
public:
    void addHatch(const DRW_Hatch* h) override {
        if (!h || h->looplist.size()!=1 || h->looplist[0]->objlist.size()!=1) return;
        const auto* e=dynamic_cast<const DRW_Ellipse*>(h->looplist[0]->objlist[0].get());
        found=std::fabs(h->angle-30./ARAD)<1e-10 && h->patternLines.size()==1 && std::fabs(h->patternLines[0].angle-75./ARAD)<1e-10 && e && e->basePoint.x==3. && e->basePoint.y==4. && e->secPoint.x==4.
            && e->secPoint.y==0. && e->ratio==.5 && e->staparam==.25 && e->endparam==1.5 && e->isccw==1;
    }
    void addLine(const DRW_Line& l) override {if(l.basePoint.x==10.&&l.secPoint.y==13.) ++lines;}
    bool found=false; int lines=0;
};
}
int main(){
    if (!mpolygonDwgAngleRoundTrip()) return 33;
    if (!widePatternLineCountRoundTrip<DRW_Hatch>()) return 34;
    if (!widePatternLineCountRoundTrip<DRW_MPolygon>()) return 35;
    {DRW_Header h;dwgWriter21 w(nullptr,&h);if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects())return 8;DRW_Hatch h1;h1.name="ANSI31";h1.solid=0;h1.angle=30./ARAD;h1.scale=1.;DRW_Hatch::PatternLine p;p.angle=75./ARAD;h1.patternLines.push_back(p);if(!w.encodeEntity(&h1))return 9;std::vector<std::uint8_t> bytes;if(!DwgBlockRecordXrefWireTestAccess::frame(w,h1.handle,bytes))return 10;BoundaryProbe<DRW_Hatch> parsed;dwgBuffer structural(bytes.data(),bytes.size());if(!parsed.common(structural))return 24;for(int i=0;i<2;++i)(void)structural.getBitLong();(void)structural.getBitDouble();(void)structural.getBitDouble();(void)structural.getBitLong();(void)structural.getBitDouble();if(structural.getBitLong()!=0)return 25;(void)structural.getBitDouble();(void)structural.get3BitDouble();if(structural.getBit()!=0||structural.getBit()!=0||structural.getBitLong()!=0)return 26;(void)structural.getBitShort();(void)structural.getBitShort();const auto overallStart=structural.getPosition()*8u+structural.getBitPos();if(std::fabs(structural.getBitDouble()-30./ARAD)>1e-12||structural.getBitDouble()!=1.||structural.getBit()!=0||structural.getBitShort()!=1)return 27;const auto lineStart=structural.getPosition()*8u+structural.getBitPos();if(std::fabs(structural.getBitDouble()-75./ARAD)>1e-12)return 28;for(int i=0;i<4;++i)if(structural.getBitDouble()!=0.)return 29;if(structural.getBitShort()!=0||structural.getBitLong()!=0||!structural.isGood())return 30;for(auto start:{overallStart,lineStart}){auto corrupt=bytes;dwgBufferW encoded;encoded.putBitDouble(std::numeric_limits<double>::infinity());if(encoded.bitCount()!=66u)return 31;for(std::size_t i=0;i<encoded.bitCount();++i){const auto mask=static_cast<std::uint8_t>(0x80u>>((start+i)%8u));if((encoded.data()[i/8u]>>(7u-i%8u))&1u)corrupt[(start+i)/8u]|=mask;else corrupt[(start+i)/8u]&=static_cast<std::uint8_t>(~mask);}dwgBuffer invalid(corrupt.data(),corrupt.size());if(parsed.parseDwg(DRW::AC1021,&invalid,0)||!parsed.patternLines.empty())return 32;}dwgBuffer b1(bytes.data(),bytes.size());if(!parsed.parseDwg(DRW::AC1021,&b1,0)||std::fabs(parsed.angle-30./ARAD)>1e-10||parsed.patternLines.size()!=1||std::fabs(parsed.patternLines[0].angle-75./ARAD)>1e-10)return 11;auto truncated=bytes;truncated.pop_back();dwgBuffer shortBuf(truncated.data(),truncated.size());if(parsed.parseDwg(DRW::AC1021,&shortBuf,0)||!parsed.patternLines.empty())return 12;dwgBuffer retry(bytes.data(),bytes.size());if(!parsed.parseDwg(DRW::AC1021,&retry,0)||std::fabs(parsed.angle-30./ARAD)>1e-10||parsed.patternLines.size()!=1)return 13;}
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        if(!checkBoundary<DRW_Hatch>(version)||!checkBoundary<DRW_MPolygon>(version)||!literal<DRW_Hatch>(version,false)||!literal<DRW_MPolygon>(version,true)||!guards<DRW_Hatch>(version,false)) {
            std::cerr<<"boundary fields failed revision "<<static_cast<int>(version)<<'\n'; return 1;
        }
    }
    const auto path=std::filesystem::temp_directory_path()/ ("dwg-hatch-ellipse-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{path};
    { {std::ofstream f(path,std::ios::binary);f<<"sentinel";} dwgRW out(path.string().c_str());IgnoredWriter bad(out,true);if(out.write(&bad,DRW::AC1021,true))return 4;{std::ifstream f(path,std::ios::binary);std::string value((std::istreambuf_iterator<char>(f)),{});if(value!="sentinel")return 5;} IgnoredWriter point(out,true,false,true);if(out.write(&point,DRW::AC1021,true))return 22;{std::ifstream f(path,std::ios::binary);std::string value((std::istreambuf_iterator<char>(f)),{});if(value!="sentinel")return 23;}IgnoredWriter good(out,false);if(!out.write(&good,DRW::AC1021,true))return 6;IgnoredWriter custom(out,true,true);if(!out.write(&custom,DRW::AC1021,true))return 7; }
    for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        dwgRW out(path.string().c_str()); Writer writer(out);
        if(!out.write(&writer,version,true)||!writer.ok) {std::cerr<<"write failed "<<static_cast<int>(version)<<'\n';return 1;}
        dwgRW in(path.string().c_str()); Reader reader;
        if(!in.read(&reader,false)||!reader.found||reader.lines!=1) {std::cerr<<"read failed "<<static_cast<int>(version)<<'\n';return 1;}
    }
    return 0;
}
