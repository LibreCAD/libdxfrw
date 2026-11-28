#include "dx_iface.h"
#include "intern/dwg_tablestyle_ac1018_candidate.h"
#include <iostream>
#include <memory>
#include <optional>
class P final:public dx_iface{public:
 void addTableStyle(const DRW_TableStyle& t)override{dx_iface::addTableStyle(t);if(t.handle==0x6bu){if(model)duplicate=true;model=t;}}
 void addUnsupportedObject(const DRW_UnsupportedObject& r)override{dx_iface::addUnsupportedObject(r);if(r.m_handle==0x6bu){if(raw)duplicate=true;raw=r;}}
 std::optional<DRW_TableStyle> model;std::optional<DRW_UnsupportedObject> raw;bool duplicate=false;
};
int main(int argc,char**argv){if(argc!=2)return 2;auto d=std::make_unique<dx_data>();auto p=std::make_unique<P>();const bool read=p->fileImport(argv[1],d.get(),false);DwgTableStyleAc1018Candidate::Result r;
 bool ok=read&&!p->duplicate&&p->raw&&p->model&&DwgTableStyleAc1018Candidate::inspect(*p->raw,*p->model,r);
 std::cout<<"read="<<read<<" matched="<<ok<<" owner="<<r.owner<<" table="<<r.tableReactor<<" xdict="<<r.xdictionary<<" style="<<r.textStyles[0]<<"\n";return !ok;}
