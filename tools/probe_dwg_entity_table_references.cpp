// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "lc3_compat_check.cpp"
#include <iostream>
#include <string>
struct Capture : LC3CompatCheck {
 void addLine(const DRW_Line& line) override {++lines;layer=line.layer;linetype=line.lineType;}
 int lines=0;std::string layer,linetype;
};
int main(int argc,char**argv) {
 if(argc<4 || argc>6)return 2;dwgRW reader(argv[1]);Capture capture;
 const bool ok=reader.read(&capture,false);const auto rows=reader.getIntegrityDiagnostics();
 const std::string name(argv[2]);DwgIntegrityCheckKind kind;
 if(name=="layer")kind=DwgIntegrityCheckKind::EntityLayerReference;
 else if(name=="ltype")kind=DwgIntegrityCheckKind::EntityLineTypeReference;
 else if(name=="layer-role")kind=DwgIntegrityCheckKind::EntityLayerHandleRole;
 else if(name=="ltype-role")kind=DwgIntegrityCheckKind::EntityLineTypeHandleRole;
 else if(name=="table-ltype")kind=DwgIntegrityCheckKind::LayerLineTypeReference;
 else if(name=="table-ltype-role")kind=DwgIntegrityCheckKind::LayerLineTypeHandleRole;
 else return 2;
 const auto target=static_cast<std::uint32_t>(std::stoul(argv[3],nullptr,16));
 const auto entity=argc>=5?static_cast<std::uint32_t>(std::stoul(argv[4],nullptr,16)):0x37u;
 const bool matchObserved=argc==6;
 const auto observed=matchObserved?static_cast<std::uint32_t>(std::stoul(argv[5],nullptr,16)):0u;unsigned hits=0;
 for(const auto& row:rows)if(row.kind==kind && row.severity==DwgIntegritySeverity::Error
    && row.phase==DwgIntegrityPhase::SectionParser && row.hasLogicalHandle && row.logicalHandle==entity
    && row.hasExpected && row.expected==target
    && (!matchObserved || (row.hasObserved && row.observed==observed)))++hits;
 std::cout<<"read="<<ok<<" lines="<<capture.lines<<" layer="<<capture.layer<<" linetype="<<capture.linetype<<" matching_diagnostics="<<hits<<" dropped="<<reader.getIntegrityDiagnosticsDropped()<<'\n';
 return ok&&capture.lines==1&&hits==1?0:1;
}
