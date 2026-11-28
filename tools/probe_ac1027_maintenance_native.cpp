#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include <array>
#include <iostream>
#include <iomanip>
#include <string>

struct Capture final : LC3CompatCheck {
 void addLine(const DRW_Line& p) override { ++lines;start=p.basePoint;end=p.secPoint;lineHandle=p.handle; }
 void addVport(const DRW_Vport& p) override { if(p.name=="*ACTIVE"){++active;height=p.height;ratio=p.ratio;} }
 void addLayer(const DRW_Layer& p) override { if(p.name=="0"){++layers;material=p.materialHandle.ref;} }
 void addMaterial(const DRW_Material& p) override { ++materials;if(p.hasDwgRawData())++raw; }
 void addHeader(const DRW_Header* h) override { seed=h->getHandSeed(); }
 unsigned lines=0,active=0,layers=0,materials=0,raw=0;std::uint32_t lineHandle=0,material=0,seed=0;double height=0,ratio=0;DRW_Coord start,end;
};
int main(int n,char** v) {
 if(n!=2)return 2;dwgRW reader(v[1]);Capture c;bool ok=reader.read(&c,false);
 std::array<unsigned,4> diag{};
 for(const auto& d:reader.getIntegrityDiagnostics()){
  if(d.kind==DwgIntegrityCheckKind::HeaderSectionCompatibility)++diag[0];
  if(d.kind==DwgIntegrityCheckKind::HeaderSectionUnclaimedData)++diag[1];
  if(d.kind==DwgIntegrityCheckKind::HeaderSectionCrc)++diag[2];
  if(d.kind==DwgIntegrityCheckKind::ObjectMapCrc)++diag[3];
 }
 std::cout<<std::setprecision(17)<<ok<<' '<<c.lines<<' '<<c.active<<' '<<c.layers<<' '<<c.materials<<' '<<c.raw<<' '<<c.lineHandle<<' '<<c.material<<' '<<c.seed<<' '<<c.start.x<<','<<c.start.y<<','<<c.start.z<<' '<<c.end.x<<','<<c.end.y<<','<<c.end.z<<' '<<c.height<<' '<<c.ratio;
 for(auto x:diag)std::cout<<' '<<x;
 const auto observations=reader.getReadObservations();
 std::cout << " obs="<<observations.entityParseFailures<<','<<observations.objectParseFailures<<','<<observations.classesCrcMismatch<<','<<observations.reader18HeaderCrcMismatch<<','<<observations.frames.sourceFrames;
 std::cout<<" integrity="; for(const auto& d:reader.getIntegrityDiagnostics())std::cout<<int(d.kind)<<':'<<int(d.severity)<<',';
 std::cout<<" dropped="<<reader.getIntegrityDiagnosticsDropped()<<'\n';return ok?0:1;
}
