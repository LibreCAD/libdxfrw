// Scratch-only source observation; not output or semantic qualification.
#include "dx_iface.h"
#include <iostream>
#include <vector>

int main(int argc,char**argv) {
    if(argc!=2)return 2;
    const std::vector<std::uint32_t> xrecords{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,
                                             0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
    dx_iface reader;dx_data data;
    if(!reader.enableVerifiedTableRoundtripChildren(xrecords,0xb9u,0xbdu,0xbau))return 3;
    if(!reader.fileImport(argv[1],&data,false))return 4;
    DwgTableContentAc1018Candidate::Result content;
    DwgDataTableAc1018Candidate::Result table;
    DwgTableGeometryAc1018Candidate::Result geometry;
    DwgTableRoundtripXRecordAc1018Candidate::Result root;
    std::array<DwgTableChildXRecordAc1018Candidate::Result,9> children;
    if(!dx_iface::readVerifiedTableRoundtripChildren(data,xrecords,0xb9u,0xbdu,0xbau,content,table,geometry,root,children))return 5;
    if(content.handle!=0xb9u || table.handle!=0xbdu || content.parentHandle!=0xbcu
        || table.parentHandle!=0xbcu || geometry.handle!=0xbau || geometry.parentHandle!=0xbcu
        || geometry.dataEndBit!=173u || geometry.handleEndBit!=261u || table.ownedChildren.size()!=9u
        || content.dataEndBit!=13313u || content.handleEndBit!=13745u
        || table.dataEndBit!=877u || table.handleEndBit!=1037u)return 6;
    for(std::size_t i=0;i<table.ownedChildren.size();++i)
        if(table.ownedChildren[i]!=0xbeu+i)return 7;
    if(root.handle!=0xbcu || root.references[0].groupCode!=360 || root.references[0].target!=0xb9u
        || root.references[1].groupCode!=361 || root.references[1].target!=0xbau
        || root.references[2].groupCode!=360 || root.references[2].target!=0xbdu)return 9;
    for(std::size_t i=0;i<children.size();++i)
        if(children[i].handle!=0xbeu+i || children[i].owner!=0xbdu)return 10;
    if(!children[0].hasText || children[0].text!="Title" || children[1].hasText || children[2].hasText)return 11;
    for(std::size_t i=3;i<children.size();++i)if(children[i].text!=std::to_string(i-2u))return 12;
    for(const auto& cell:geometry.cells)if(cell.unknownHandle!=0u)return 8;
    std::cout<<"same-import observation passed: 10 XRECORDs, 3 custom frames, 9 child links, 3 ordered root references, 9 child wire profiles; "
             <<"entity failures="<<data.dwgReadOutcome.observations.entityParseFailures
             <<" object failures="<<data.dwgReadOutcome.observations.objectParseFailures<<'\n';
    return 0;
}
