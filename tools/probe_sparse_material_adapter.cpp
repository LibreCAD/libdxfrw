// SPDX-License-Identifier: GPL-2.0-or-later
#include "dwg2dxf/dx_iface.h"
#include "dwg2dxf/dx_data.h"
#include <iostream>
class DxIfaceMaterialReferenceTestAccess {
public:
 static bool prepare(dx_iface& iface,dx_data& data) {
  iface.cData=&data;return iface.prepareDxfMaterialSources(DRW::AC1027,false);
 }
};
class LegacyDerived final : public dx_iface {};
class ExplicitDerived final : public dx_iface {
public: bool wantsDxfSparseMaterialRawCapture() const noexcept override {return true;}
};
int main(int argc,char**argv){if(argc!=2)return 2;
 dx_data legacyData;LegacyDerived legacy;if(legacy.fileImport(argv[1],&legacyData,false))return 1;
 dx_data explicitData;ExplicitDerived explicitReader;if(!explicitReader.fileImport(argv[1],&explicitData,false)||explicitData.materials.size()!=3u)return 1;
 dx_data data;dx_iface reader;
 if(!reader.fileImport(argv[1],&data,false)||data.materials.size()!=3u)return 1;
 for(const auto& m:data.materials)if(!m.requiresRawDxfMaterialReplay())return 1;
 if(DxIfaceMaterialReferenceTestAccess::prepare(reader,data))return 1;
 std::cout<<"sparse MATERIAL adapter replay refused PASS\n";return 0;}
