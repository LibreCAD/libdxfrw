// SPDX-License-Identifier: GPL-2.0-or-later
// Source-bound probe; extraction and CRC rely on the in-tree reader.
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include "dwg2dxf/dx_raster_dwg_wire.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <vector>
class Probe final : public dx_iface {
public:
 void addRasterVariables(const DRW_RasterVariables& v) override {
  ++typed; typedMatch=v.handle==0x2Fu && v.parentHandle==0xAu && v.m_classVersion==0
   && v.m_imageFrame==1 && v.m_imageQuality==1 && v.m_units==3 && v.reactorCount()==1
   && v.reactorHandles==std::vector<std::uint32_t>{0xAu} && v.extensionDictionaryFlag()==1u;
  dx_iface::addRasterVariables(v);
 }
 void addUnsupportedObject(const DRW_UnsupportedObject& v) override {
  if(v.m_recordName=="RASTERVARIABLES") {
   ++raw;const auto& b=v.m_rawBytes;
   rawMatch=v.m_version==DRW::AC1032 && v.m_handle==0x2Fu && v.m_objectType==512
    && b.size()==14u && v.m_bodyBitSize==39u && v.m_isCustomClass;
   if(rawMatch) {
    using namespace dx_raster_wire::detail;
    Bits body{b.data(),0u,73u};
    rawMatch=body.take(2u)==1u && body.take(8u)+0x1F0u==512u
     && handle(body,0u,0u,true).value==0x2Fu && body.shortValue()==0u
     && body.longValue()==1u && body.take(1u)==1u && body.take(1u)==0u
     && body.longValue()==0u && body.good && body.pos==42u;
    const auto start=body.pos;
    rawMatch=rawMatch && body.shortValue()==1u && body.pos-start==10u
     && body.shortValue()==1u && body.shortValue()==3u && body.pos==72u
     && body.take(1u)==0u && body.good && body.pos==73u;
    Bits handles{b.data(),73u,112u};
    const auto owner=handle(handles,4u,0x2Fu),reactor=handle(handles,4u,0x2Fu);
    rawMatch=rawMatch && handles.good && owner.role==4u && owner.value==0xAu
     && reactor.role==4u && reactor.value==0xAu && handles.pos==105u
     && handles.take(7u)==0u && handles.good && handles.pos==112u;
   }
  }
  dx_iface::addUnsupportedObject(v);
 }
 unsigned typed=0u,raw=0u; bool typedMatch=false,rawMatch=false;
};
int main(int argc,char** argv) {
 if(argc!=2) return 2;
 dx_data data;Probe reader;const bool imported=reader.fileImport(argv[1],&data,false);
 const auto root=std::count_if(data.namedObjectDictionary.m_entries.begin(),data.namedObjectDictionary.m_entries.end(),
  [](const DRW_Dictionary::Entry& e){return e.m_name=="ACAD_IMAGE_VARS" && e.m_handle==0x2Fu;});
 const auto classes=std::count_if(data.dwgClassCoverage.m_entries.begin(),data.dwgClassCoverage.m_entries.end(),
  [](const DRW_DwgClassCoverageEntry& e){return e.m_classNumber==512 && e.m_className=="AcDbRasterVariables" && e.m_recordName=="RASTERVARIABLES" && e.m_instanceCount==1;});
 const bool good=imported && data.sourceVersion==DRW::AC1032 && data.hasNamedObjectDictionary
  && data.namedObjectDictionary.handle==0xAu && root==1 && data.dwgClassCoverage.m_complete
  && data.hasUnchangedDwgClassEvidence() && classes==1 && reader.typed==1u && reader.raw==1u
  && reader.typedMatch && reader.rawMatch;
 std::cout<<"AC1032 frame=1 RASTERVARIABLES source proof: "<<(good?"PASS":"FAIL")<<'\n';return good?0:1;
}
