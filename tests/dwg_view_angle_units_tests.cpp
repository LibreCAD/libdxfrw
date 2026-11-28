#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter18.h"
#include "drw_header.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <limits>
struct ViewProbe:DRW_View {using DRW_View::parseDwg;using DRW_View::encodeDwg;};
class DwgBlockRecordXrefWireTestAccess {public:
 static bool frame(dwgWriter15& w,std::uint32_t id,std::vector<std::uint8_t>& b){auto i=std::find_if(w.m_objectMap.begin(),w.m_objectMap.end(),[id](const auto& x){return x.first==id;});if(i==w.m_objectMap.end())return false;const auto& d=w.buffer();dwgBuffer in(d.data()+i->second,d.size()-i->second);auto n=in.getModularShort();auto start=in.getPosition();if(!in.isGood()||n<=0||start+static_cast<std::size_t>(n)+2>d.size()-i->second)return false;b.assign(d.begin()+i->second+start,d.begin()+i->second+start+n);return true;}
 static bool emit(dwgWriter15& w,std::uint32_t id,const DRW_View& v){return w.emitViewRecord(id,v);}
};
static std::size_t findBD(const std::vector<std::uint8_t>& body,double value){dwgBufferW pattern;pattern.putBitDouble(value);const auto bit=[](const std::vector<std::uint8_t>& d,std::size_t i){return (d[i/8u]>>(7u-i%8u))&1u;};std::size_t found=body.size()*8u;for(std::size_t p=0;p+pattern.bitCount()<=body.size()*8u;++p){bool same=true;for(std::size_t i=0;i<pattern.bitCount();++i)if(bit(body,p+i)!=bit(pattern.data(),i)){same=false;break;}if(same){if(found!=body.size()*8u)return body.size()*8u;found=p;}}return found;}
static bool parseOverflow(){DRW_Header h;dwgWriter18 w(nullptr,&h);if(!w.prepareDwgClassManifest()||!w.writeFileHeaderStub()||!w.writeDwgHeader()||!w.writeDwgClasses()||!w.writeDwgObjects())return false;DRW_View v;v.name="ANGLECHECK";v.twistAngle=45.;if(!DwgBlockRecordXrefWireTestAccess::emit(w,0xB00u,v))return false;std::vector<std::uint8_t> body;if(!DwgBlockRecordXrefWireTestAccess::frame(w,0xB00u,body))return false;ViewProbe parsed;dwgBuffer good(body.data(),body.size());if(!parsed.parseDwg(DRW::AC1018,&good,0)||std::fabs(parsed.twistAngle-45.)>1e-10)return false;auto pos=findBD(body,45./ARAD);if(pos==body.size()*8u)return false;auto bytes=body;dwgBufferW huge;huge.putBitDouble(1.e308);if(huge.bitCount()!=66u)return false;for(std::size_t i=0;i<66u;++i){auto mask=static_cast<std::uint8_t>(0x80u>>((pos+i)%8u));if((huge.data()[i/8u]>>(7u-i%8u))&1u)bytes[(pos+i)/8u]|=mask;else bytes[(pos+i)/8u]&=static_cast<std::uint8_t>(~mask);}dwgBuffer bad(bytes.data(),bytes.size());if(parsed.parseDwg(DRW::AC1018,&bad,0)||parsed.twistAngle!=0.)return false;dwgBuffer retry(body.data(),body.size());return parsed.parseDwg(DRW::AC1018,&retry,0)&&std::fabs(parsed.twistAngle-45.)<1e-10;}
struct DerivedView:DRW_View {};
struct Adapter:LC3CompatCheck {
 dwgRW* out=nullptr;bool bad=false,tilt=false,derived=false,duplicate=false;int count=0;DRW_View got,value;DerivedView sub;
 void writeViews() override {DRW_View& v=derived?static_cast<DRW_View&>(sub):value;v.name="ANGLECHECK";v.twistAngle=bad?30.*std::numeric_limits<double>::denorm_min():45.;if(tilt){v.viewDirectionFromTarget={1,2,3};v.targetPoint={4,5,6};}(void)out->addView(&v);if(duplicate){v.name="anglecheck";v.twistAngle=30.*std::numeric_limits<double>::denorm_min();(void)out->addView(&v);}}
 void writeEntities() override {DRW_Line l;l.basePoint={1,2,3};l.secPoint={4,5,6};(void)out->writeLine(&l);}
 void addView(const DRW_View& v) override {if(v.name=="ANGLECHECK"){got=v;++count;}}
};
static bool wire(){ViewProbe v;v.name="ANGLECHECK";v.twistAngle=45.;v.lensLen=7.;v.viewDirectionFromTarget={1,2,3};v.targetPoint={4,5,6};dwgBufferW b,s,h;if(!v.encodeDwg(DRW::AC1018,&b,&s,&h))return false;dwgBuffer in(b.data().data(),b.size());if(in.getVariableText(DRW::AC1018)!="ANGLECHECK")return false;(void)in.getBit();(void)in.getBitShort();(void)in.getBit();(void)in.getBitDouble();(void)in.getBitDouble();(void)in.get2RawDouble();auto target=in.get3BitDouble();auto direction=in.get3BitDouble();return in.isGood()&&target.x==4.&&target.y==5.&&target.z==6.&&direction.x==1.&&direction.y==2.&&direction.z==3.&&std::fabs(in.getBitDouble()-45./ARAD)<1e-12&&in.getBitDouble()==7.;}
int main(){
 if(!wire()||!parseOverflow()){std::cerr<<"wire/overflow failed\n";return 1;}
 for(double value:{std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::denorm_min(),30.*std::numeric_limits<double>::denorm_min()}){ViewProbe v;v.name="ANGLECHECK";v.twistAngle=value;dwgBufferW b,s,h;b.putRawChar8(0xa5);b.putBit(1);s.putRawChar8(0x5a);h.putRawChar8(0x3c);auto bb=b.data(),ss=s.data(),hh=h.data();auto n=b.bitCount();if(v.encodeDwg(DRW::AC1021,&b,&s,&h)||bb!=b.data()||ss!=s.data()||hh!=h.data()||n!=b.bitCount())return 2;v.twistAngle=45.;if(!v.encodeDwg(DRW::AC1021,&b,&s,&h))return 3;}
 auto dir=std::filesystem::temp_directory_path()/("view-angle-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));std::filesystem::create_directory(dir);struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
 for(auto version:{DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032})for(bool tilt:{false,true}) {auto p=dir/(std::to_string(int(version))+(tilt?"t":"p")+".dwg");dwgRW out(p.string().c_str());Adapter a;a.out=&out;a.tilt=tilt;if(!out.write(&a,version,true)){std::cerr<<"write "<<int(version)<<"\n";return 4;}dwgRW in(p.string().c_str());Adapter r;if(!in.read(&r,false)||r.count!=1||std::fabs(r.got.twistAngle-45.)>1e-10||(tilt&&(r.got.viewDirectionFromTarget.x!=1||r.got.viewDirectionFromTarget.y!=2||r.got.viewDirectionFromTarget.z!=3||r.got.targetPoint.x!=4||r.got.targetPoint.y!=5||r.got.targetPoint.z!=6))){std::cerr<<"read "<<int(version)<<"\n";return 5;}}
 auto p=dir/"sentinel.dwg";{std::ofstream f(p);f<<"sentinel";}dwgRW out(p.string().c_str());for(bool derived:{false,true}){Adapter bad;bad.out=&out;bad.bad=true;bad.derived=derived;const auto oldHandle=derived?bad.sub.handle:bad.value.handle;const auto oldOwner=derived?bad.sub.parentHandle:bad.value.parentHandle;if(out.write(&bad,DRW::AC1021,true))return 6;if((derived?bad.sub.handle:bad.value.handle)!=oldHandle||(derived?bad.sub.parentHandle:bad.value.parentHandle)!=oldOwner)return 7;{std::ifstream f(p);std::string x((std::istreambuf_iterator<char>(f)),{});if(x!="sentinel")return 8;}}Adapter ok;ok.out=&out;if(!out.write(&ok,DRW::AC1021,true))return 9;Adapter duplicate;duplicate.out=&out;duplicate.duplicate=true;if(!out.write(&duplicate,DRW::AC1021,true))return 10;dwgRW in(p.string().c_str());Adapter readback;if(!in.read(&readback,false)||readback.count!=1||std::fabs(readback.got.twistAngle-45.)>1e-10)return 11;
 std::cout<<"VIEW angle controls pass\n";
}
