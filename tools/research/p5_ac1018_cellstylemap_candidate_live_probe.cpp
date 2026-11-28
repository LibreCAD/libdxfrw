// Actual callback comparison for a caller-supplied drawing; callbacks are
// public/untrusted and this is not a reader-private source or CRC ticket.
#include "dx_iface.h"
#include "intern/dwg_cellstylemap_ac1018_candidate.h"
#include <iostream>
#include <memory>
#include <optional>
class Probe final:public dx_iface {
public:
 void addCellStyleMap(const DRW_CellStyleMap& v) override {dx_iface::addCellStyleMap(v);if(v.handle==0xddu){if(model)duplicate=true;model=v;}}
 void addUnsupportedObject(const DRW_UnsupportedObject& v) override {dx_iface::addUnsupportedObject(v);if(v.m_handle==0xddu){if(raw)duplicate=true;raw=v;}}
 std::optional<DRW_CellStyleMap> model;std::optional<DRW_UnsupportedObject> raw;bool duplicate=false;
};
int main(int argc,char** argv){if(argc!=2)return 2;auto data=std::make_unique<dx_data>();auto p=std::make_unique<Probe>();
 const bool read=p->fileImport(argv[1],data.get(),false);DwgCellStyleMapAc1018Candidate::Result r;
 const bool ok=read&&!p->duplicate&&p->raw&&p->model&&DwgCellStyleMapAc1018Candidate::inspect(*p->raw,*p->model,r);
 std::cout<<"read="<<read<<" matched="<<ok<<" handle="<<r.handle<<" owner="<<r.owner
          <<" sourceOrder="<<r.styles[0].id<<','<<r.styles[1].id<<','<<r.styles[2].id<<'\n';return !ok;}
