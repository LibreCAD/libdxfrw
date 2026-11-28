// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dwgreader27.h"
#include "intern/dwgsafety.h"
#include <iostream>
#include <atomic>
#include <cstdlib>
#include <new>
#include <memory>
#include <utility>

static std::atomic<int> failAfter{-1};
void* operator new(std::size_t n) {if(failAfter>=0 && failAfter--==0)throw std::bad_alloc();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete(void* p,std::size_t) noexcept {std::free(p);}

struct Reader final : dwgReader27 {
 explicit Reader(DRW::Version v=DRW::AC1027) : dwgReader27(std::unique_ptr<dwgBuffer>{},nullptr) {version=v;}
 using dwgReader::parseAttribs;
 using dwgReader::resolveLayerLineTypeReference;
 void addRow(DwgIntegritySeverity severity,const char* section=nullptr) {
  recordIntegrityDiagnostic(severity,DwgIntegrityAddressSpace::None,DwgIntegrityPhase::SectionParser,
    DwgIntegrityCheckKind::EntityLayerReference,-1,-1,section);
 }
};
template<class T> struct Entity : T {
 void setup(std::uint32_t layer,std::uint32_t ltype,unsigned flags) {
  this->handle=0x44;this->layerH.ref=layer;this->layerH.ref64=layer;
  this->layerH.rawCode=5;this->lTypeH.ref=ltype;this->lTypeH.ref64=ltype;
  this->lTypeH.rawCode=5;this->ltFlags=static_cast<std::uint8_t>(flags);
 }
 void wideLayer(std::uint64_t value) {this->layerH.ref64=value;this->layerH.ref=static_cast<std::uint32_t>(value);}
 void wideLtype(std::uint64_t value) {this->lTypeH.ref64=value;this->lTypeH.ref=static_cast<std::uint32_t>(value);}
 void roles(std::uint8_t layer,std::uint8_t ltype) {this->layerH.rawCode=layer;this->lTypeH.rawCode=ltype;}
 void relative() {this->layerH.code=7;this->layerH.rawCode=0x0a;this->lTypeH.code=7;this->lTypeH.rawCode=0x0a;}
};
static bool checkRow(const Reader& r,DwgIntegrityCheckKind kind,std::uint64_t target,DRW::Version version);
static bool checkRole(const Reader& r,DwgIntegrityCheckKind kind,std::uint8_t observed,DRW::Version version) {
 if(r.m_integrityDiagnostics.size()!=1u)return false;const auto& d=r.m_integrityDiagnostics.front();
 return d.kind==kind && d.severity==DwgIntegritySeverity::Error && d.phase==DwgIntegrityPhase::SectionParser
     && d.version==version && d.hasLogicalHandle && d.logicalHandle==0x44
     && d.hasExpected && d.expected==DRW::DwgHardPointer && d.hasObserved && d.observed==observed;
}
static bool layerControls(DRW::Version version) {
 Reader r(version);
 auto* ltype=new DRW_LType;ltype->name="Continuous";r.ltypemap.emplace(0x22,ltype);
 DRW_Layer layer;layer.handle=0x44;layer.lTypeH.ref=0x22;layer.lTypeH.ref64=0x22;layer.lTypeH.rawCode=5;
 r.resolveLayerLineTypeReference(&layer);
 if(!r.m_integrityDiagnostics.empty()||layer.lineType!="Continuous")return false;
 layer.lTypeH.ref=0x7e;layer.lTypeH.ref64=0x7e;r.resolveLayerLineTypeReference(&layer);
 if(!checkRow(r,DwgIntegrityCheckKind::LayerLineTypeReference,0x7e,version))return false;
 r.m_integrityDiagnostics.clear();layer.lTypeH.ref=0x22u;layer.lTypeH.ref64=0x100000022ull;
 r.resolveLayerLineTypeReference(&layer);
 if(!checkRow(r,DwgIntegrityCheckKind::LayerLineTypeReference,0x100000022ull,version))return false;
 r.m_integrityDiagnostics.clear();layer.lTypeH.ref=0;layer.lTypeH.ref64=0x100000000ull;
 r.resolveLayerLineTypeReference(&layer);
 if(!checkRow(r,DwgIntegrityCheckKind::LayerLineTypeReference,0x100000000ull,version))return false;
 r.m_integrityDiagnostics.clear();layer.lTypeH.ref=0x22;layer.lTypeH.ref64=0x22;layer.lTypeH.rawCode=4;
 r.resolveLayerLineTypeReference(&layer);
 return checkRole(r,DwgIntegrityCheckKind::LayerLineTypeHandleRole,4,version);
}
static bool checkRow(const Reader& r,DwgIntegrityCheckKind kind,std::uint64_t target,DRW::Version version) {
 if(r.m_integrityDiagnostics.size()!=1u)return false;const auto& d=r.m_integrityDiagnostics.front();
 return d.kind==kind && d.severity==DwgIntegritySeverity::Error && d.phase==DwgIntegrityPhase::SectionParser
     && d.version==version && d.hasLogicalHandle && d.logicalHandle==0x44
     && d.hasExpected && d.expected==target && d.hasObserved && d.observed==0;
}
template<class T> bool controls(DRW::Version version) {
 Reader r(version);auto* layer=new DRW_Layer;layer->name="ValidLayer";r.layermap.emplace(0x11,layer);
 auto* ltype=new DRW_LType;ltype->name="ValidType";r.ltypemap.emplace(0x22,ltype);
 Entity<T> e;e.setup(0x11,0x22,3);r.parseAttribs(&e);
 if(!r.m_integrityDiagnostics.empty()||e.layer!="ValidLayer"||e.lineType!="ValidType")return false;
 e.setup(0x7e,0x22,3);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLayerReference,0x7e,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x22,0x22,3);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLayerReference,0x22,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0x7e,3);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLineTypeReference,0x7e,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0x11,3);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLineTypeReference,0x11,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0x22,3);e.wideLayer(0x100000011ull);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLayerReference,0x100000011ull,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0,0x22,3);e.wideLayer(0x100000000ull);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLayerReference,0x100000000ull,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0x22,3);e.wideLtype(0x100000022ull);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLineTypeReference,0x100000022ull,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0,3);e.wideLtype(0x100000000ull);r.parseAttribs(&e);if(!checkRow(r,DwgIntegrityCheckKind::EntityLineTypeReference,0x100000000ull,version))return false;r.m_integrityDiagnostics.clear();
 e.setup(0x11,0x22,3);e.relative();r.parseAttribs(&e);
 if(r.m_integrityDiagnostics.size()!=2u)return false;
 if(r.m_integrityDiagnostics[0].kind!=DwgIntegrityCheckKind::EntityLineTypeHandleRole
    ||r.m_integrityDiagnostics[1].kind!=DwgIntegrityCheckKind::EntityLayerHandleRole)return false;
 r.m_integrityDiagnostics.clear();e.setup(0x11,0x22,3);e.roles(4,4);r.parseAttribs(&e);
 if(r.m_integrityDiagnostics.size()!=2u
    ||r.m_integrityDiagnostics[0].kind!=DwgIntegrityCheckKind::EntityLineTypeHandleRole
    ||r.m_integrityDiagnostics[1].kind!=DwgIntegrityCheckKind::EntityLayerHandleRole)return false;
 r.m_integrityDiagnostics.clear();
 for(unsigned flag : {0u,1u,2u}) { e.setup(0x11,0x7e,flag);e.wideLtype(0x100000022ull);r.parseAttribs(&e);if(!r.m_integrityDiagnostics.empty())return false; }
 e.setup(0,0,3);r.parseAttribs(&e);return r.m_integrityDiagnostics.empty();
}
int main(){
 for(const auto version : {DRW::AC1015,DRW::AC1018,DRW::AC1021,DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
 if(!controls<DRW_Point>(version) || !controls<DRW_Line>(version) || !controls<DRW_Arc>(version) || !controls<DRW_Circle>(version)
    || !controls<DRW_Ellipse>(version) || !controls<DRW_Solid>(version) || !controls<DRW_Trace>(version) || !controls<DRW_3Dface>(version)
    || !controls<DRW_Block>(version) || !controls<DRW_Insert>(version) || !controls<DRW_Attrib>(version) || !controls<DRW_Attdef>(version)
    || !controls<DRW_Vertex>(version) || !controls<DRW_SeqEnd>(version) || !controls<DRW_Polyline>(version)
    || !controls<DRW_LWPolyline>(version) || !controls<DRW_Spline>(version) || !controls<DRW_Hatch>(version)
    || !controls<DRW_Text>(version) || !controls<DRW_MText>(version) || !controls<DRW_DimLinear>(version) || !controls<DRW_DimRadial>(version)
    || !layerControls(version))return 1;
 }

 Reader reader;Entity<DRW_Line> line;line.setup(0x7e,0,0);
 for(std::size_t i=0;i<dwgSafety::MaxIntegrityDiagnostics;++i)reader.addRow(DwgIntegritySeverity::Warning);
 reader.parseAttribs(&line);
 if(reader.m_integrityDiagnostics.size()!=dwgSafety::MaxIntegrityDiagnostics || reader.m_integrityDiagnosticsDropped!=1u)return 2;
 Reader vectorFailure;failAfter=0;vectorFailure.parseAttribs(&line);failAfter=-1;
 if(!vectorFailure.m_integrityDiagnostics.empty() || vectorFailure.m_integrityDiagnosticsDropped!=1u)return 3;
 Reader constructionFailure;failAfter=0;
 constructionFailure.addRow(DwgIntegritySeverity::Error,"an intentionally long section name that cannot fit small string storage");failAfter=-1;
 if(!constructionFailure.m_integrityDiagnostics.empty() || constructionFailure.m_integrityDiagnosticsDropped!=1u)return 4;
 return 0;
}
