#include "dx_iface.h"
#include <iostream>
#include <memory>
class Probe : public dx_iface {
public:
 void addTable(const DRW_Table& t) override { dx_iface::addTable(t); if(t.handle==0x9c) { ++tables; std::cout<<"TABLE handle="<<t.handle<<" xdict="<<t.xDictHandle<<" owner="<<t.parentHandle<<" validated="<<t.hasDwgCommonLinkTail()<<"\n"; } }
 void addDwgFramePublication(const DRW_DwgFramePublication& p) override { dx_iface::addDwgFramePublication(p); if(p.m_handle==0x9c) {++pubs;std::cout<<"PUB handle="<<p.m_handle<<" version="<<p.m_version<<" offset="<<p.m_sourceOffset<<" ordinal="<<p.m_sourceMapOrdinal<<" space="<<int(p.m_sourceOffsetSpace)<<" encoded="<<p.m_encodedType<<" resolved="<<p.m_resolvedType<<" entity="<<p.m_isEntity<<" custom="<<p.m_isCustomClass<<" rec="<<p.m_recordName<<" class="<<p.m_className<<" evidence="<<int(p.m_commonLinkEvidence)<<" xdict="<<p.m_xDictHandle<<" xflag="<<int(p.m_xDictFlag)<<" carrier="<<int(p.m_carrier)<<"\n"; } }
 int tables=0,pubs=0;
};
int main(int argc,char**argv){if(argc!=2)return 2;auto data=std::make_unique<dx_data>();auto p=std::make_unique<Probe>();bool ok=p->fileImport(argv[1],data.get(),false);std::cout<<"ok="<<ok<<" tables="<<p->tables<<" publications="<<p->pubs<<"\n";return !ok || p->tables!=1 || p->pubs!=1;}
