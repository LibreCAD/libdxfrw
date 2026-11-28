// SPDX-License-Identifier: GPL-2.0-or-later
#include "drw_entities.h"
#include "libdwgr.h"
#include "lc3_compat_check.cpp"
#include "intern/dwgbufferw.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

struct TextProbe : DRW_Text { using DRW_Text::encodeDwg; };
bool drwHasDwgTextAlignmentElevation(const DRW_Text&) noexcept;
class CustomText : public DRW_Text {
protected:
 bool encodeDwg(DRW::Version v,dwgBufferW* b,std::uint32_t n,dwgBufferW* s,dwgBufferW* h) override {
  return DRW_Line::encodeDwg(v,b,n,s,h);
 }
};
class Writer final : public LC3CompatCheck {
public:
 explicit Writer(dwgRW& w):out(w){}
 void writeEntities() override {
  DRW_Line before;before.basePoint={0.,0.,0.};before.secPoint={1.,1.,0.};
  if (!out.writeLine(&before)) return;
  wrote=entry==0?out.writeText(&text):entry==1?out.writePoint(&text):out.writeLine(&text);
 }
 bool finalizeDwgWrite() override {return true;} // Simulate legacy ignored false.
 dwgRW& out;DRW_Text text;int entry=0;bool wrote=false;
};
class Readback final : public LC3CompatCheck {
public:
 void addText(const DRW_Text& t) override {++count; z=t.basePoint.z;alignZ=t.secPoint.z;}
 int count=0;double z=0.,alignZ=0.;
};
class CustomWriter final : public LC3CompatCheck {
public:
 explicit CustomWriter(dwgRW& w):out(w){}
 void writeEntities() override {wrote=out.writeText(&text);}
 bool finalizeDwgWrite() override {return true;}
 dwgRW& out;CustomText text;bool wrote=false;
};
class LineReader final : public LC3CompatCheck {
public:
 void addLine(const DRW_Line& v) override {ok=v.basePoint.z==3. && v.secPoint.z==7.;}
 bool ok=false;
};
int main() {
 const auto dir=std::filesystem::temp_directory_path()/("dwg-text-elevation-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
 std::filesystem::create_directories(dir);
 struct Cleanup {std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove_all(p,e);}} cleanup{dir};
 for (auto horizontal : {DRW_Text::HCenter,DRW_Text::HRight,DRW_Text::HAligned,
                          DRW_Text::HMiddle,DRW_Text::HFit}) {
  DRW_Text t;t.basePoint.z=3.;t.secPoint.z=7.;t.alignH=horizontal;
  if(drwHasDwgTextAlignmentElevation(t))return 11;
  t.secPoint.z=3.;if(!drwHasDwgTextAlignmentElevation(t))return 12;
  t.secPoint.z=-0.;if(!drwHasDwgTextAlignmentElevation(t))return 13;
 }
 for (auto vertical : {DRW_Text::VBottom,DRW_Text::VMiddle,DRW_Text::VTop}) {
  DRW_Text t;t.basePoint.z=3.;t.secPoint.z=7.;t.alignV=vertical;
  if(drwHasDwgTextAlignmentElevation(t))return 14;
  t.secPoint.z=std::numeric_limits<double>::quiet_NaN();if(drwHasDwgTextAlignmentElevation(t))return 15;
  t.secPoint.z=3.;t.basePoint.z=std::numeric_limits<double>::infinity();if(drwHasDwgTextAlignmentElevation(t))return 16;
 }
 for (auto version : {DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
  TextProbe direct;direct.basePoint.z=3.;direct.secPoint.z=7.;direct.alignH=DRW_Text::HCenter;
  for (double invalid : {7.,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
   direct.secPoint.z=invalid;dwgBufferW body,strings,handles;body.putBit(1);const auto bytes=body.data();const auto bits=body.bitCount();
   if(direct.encodeDwg(version,&body,0u,&strings,&handles) || body.data()!=bytes || body.bitCount()!=bits || strings.bitCount() || handles.bitCount()) return 1;
  }
  for (int entry=0;entry<3;++entry) {
   const auto path=dir/(std::to_string(static_cast<int>(version))+"-"+std::to_string(entry)+".dwg");
   {std::ofstream f(path,std::ios::binary);f<<"sentinel";}
   dwgRW out(path.string().c_str());Writer w(out);w.entry=entry;w.text.basePoint={1.,2.,3.};w.text.secPoint={4.,5.,7.};w.text.height=2.5;w.text.text="WITNESS";w.text.alignH=DRW_Text::HCenter;w.text.alignV=DRW_Text::VMiddle;if(entry==2)w.text.extPoint={0.,1.,0.};
   const auto original=w.text.handle;
   if(out.write(&w,version,true) || w.wrote || w.text.handle!=original)return 2;
   std::ifstream f(path,std::ios::binary);if(std::string(std::istreambuf_iterator<char>(f),{})!="sentinel")return 3;f.close();
   w.text.secPoint.z=3.;if(!out.write(&w,version,true)||!w.wrote)return 4;
   dwgRW in(path.string().c_str());Readback r;if(!in.read(&r,false)||r.count!=1||r.z!=3.||r.alignZ!=3.)return 5;
   w.text.secPoint.z=0.;if(!out.write(&w,version,true)||!w.wrote)return 6;
   dwgRW in2(path.string().c_str());Readback r2;if(!in2.read(&r2,false)||r2.count!=1||r2.z!=3.||r2.alignZ!=3.)return 7;
   w.text.alignH=DRW_Text::HLeft;w.text.alignV=DRW_Text::VBaseLine;w.text.secPoint.z=7.;if(!out.write(&w,version,true)||!w.wrote)return 8;
  }
  const auto path=dir/("custom-"+std::to_string(static_cast<int>(version))+".dwg");
  dwgRW out(path.string().c_str());CustomWriter custom(out);custom.text.basePoint={1.,2.,3.};custom.text.secPoint={4.,5.,7.};custom.text.alignH=DRW_Text::HCenter;
  if(!out.write(&custom,version,true)||!custom.wrote)return 9;
  dwgRW in(path.string().c_str());LineReader line;if(!in.read(&line,false)||!line.ok)return 10;
 }
 return 0;
}
