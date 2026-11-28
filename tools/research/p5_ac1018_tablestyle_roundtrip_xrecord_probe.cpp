// Observer-only correlation of callbacks from a caller-supplied DWG.
// Callback association is not a private CRC provenance ticket or output proof.
#include "dx_iface.h"
#include "intern/dwg_tablestyle_roundtrip_xrecord_ac1018_candidate.h"
#include <iostream>
#include <memory>
class Capture final:public dx_iface {
public:
    void addXRecord(const DRW_XRecord& value) override {
        dx_iface::addXRecord(value);
        if(value.handle==0xdcu){typed=value;++typedCount;}
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        dx_iface::addUnsupportedObject(value);
        if(value.m_handle==0xdcu){raw=value;++rawCount;}
    }
    DRW_XRecord typed;DRW_UnsupportedObject raw;unsigned typedCount{0u},rawCount{0u};
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    auto data=std::make_unique<dx_data>();auto capture=std::make_unique<Capture>();
    if(!capture->fileImport(argv[1],data.get(),false) || capture->typedCount!=1u || capture->rawCount!=1u)return 3;
    DwgTableStyleRoundtripXRecordAc1018Candidate::Result r;
    if(!DwgTableStyleRoundtripXRecordAc1018Candidate::inspect(capture->raw,capture->typed,r)
        || r.handle!=0xdcu || r.owner!=0xdbu || r.reactor!=0xdbu || r.dataEndBit!=803u
        || r.entries.front().begin!=81u || r.entries.back().end!=793u
        || r.ownerSpan.begin!=803u || r.ownerSpan.end!=819u
        || r.reactorSpan.begin!=819u || r.reactorSpan.end!=835u)return 4;
    std::cout<<"XRECORD source bytes and typed callbacks matched; no source capability or output authority established\n";return 0;
}
