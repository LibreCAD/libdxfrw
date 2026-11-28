// Scratch-output source observer; no fixture or output qualification.
#include "dx_iface.h"
#include "intern/dwg_tablecontent_ac1018_candidate.h"
#include <iostream>

class Adapter final : public dx_iface {
public:
    bool found{false},ok{false};
    DwgTableContentAc1018Candidate::Result result;
    void addUnsupportedObject(const DRW_UnsupportedObject& raw) override {
        if (raw.m_handle==0xb9u) {
            if (found) ok=false;
            else {found=true;ok=DwgTableContentAc1018Candidate::inspect(raw,result);}
        }
        dx_iface::addUnsupportedObject(raw);
    }
};
int main(int argc,char**argv) {
    if(argc!=2)return 2;
    dx_data data;Adapter adapter;
    if(!adapter.fileImport(argv[1],&data,false) || !adapter.found || !adapter.ok)return 1;
    const auto& result=adapter.result;
    std::cout<<"{\"body\":"<<result.dataEndBit<<",\"tail\":"<<result.handleEndBit
             <<",\"parent\":"<<result.parentHandle<<",\"text\":[";
    bool first=true;const char hex[]="0123456789abcdef";
    for(const auto& span:result.textSpans) {
        if(!first)std::cout<<',';first=false;
        std::cout<<"{\"startBit\":"<<span.startBit<<",\"endBit\":"<<span.endBit<<",\"bytesHex\":\"";
        for(auto byte:span.bytes)std::cout<<hex[byte>>4]<<hex[byte&15u];
        std::cout<<"\"}";
    }
    std::cout<<"],\"handles\":[";first=true;
    for(const auto& span:result.handleSpans) {
        if(!first)std::cout<<',';first=false;
        using Kind=DwgTableContentAc1018Candidate::HandleKind;
        const char* name=span.kind==Kind::TextStyle?"textstyle":
            span.kind==Kind::BorderLinetype?"border-linetype":"tablestyle";
        std::cout<<"{\"field\":\""<<name<<"\",\"startBit\":"<<span.startBit
                 <<",\"endBit\":"<<span.endBit<<",\"role\":"<<unsigned(span.role)
                 <<",\"size\":"<<unsigned(span.size)<<",\"value\":"<<span.reference<<"}";
    }
    std::cout<<"]}\n";
    return 0;
}
