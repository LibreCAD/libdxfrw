// Scratch-only source observation; not output or semantic qualification.
#include "dx_iface.h"
#include <iostream>
#include <vector>

int main(int argc,char**argv) {
    if(argc!=2)return 2;
    const std::vector<std::uint32_t> xrecords{0xbcu,0xbeu,0xbfu,0xc0u,0xc1u,
                                             0xc2u,0xc3u,0xc4u,0xc5u,0xc6u};
    dx_iface reader;dx_data data;
    if(!reader.enableVerifiedTableExtensionDictionary(xrecords,0xb9u,0xbdu,0xbau,0x9cu,0xbbu))return 3;
    if(!reader.fileImport(argv[1],&data,false))return 4;
    DwgTableExtensionDictionaryAc1018Candidate::Result dictionary;
    if(!dx_iface::readVerifiedTableExtensionDictionary(data,xrecords,0xb9u,0xbdu,0xbau,0x9cu,0xbbu,dictionary))return 5;
    if(dictionary.handle!=0xbbu || dictionary.owner!=0x9cu || dictionary.root!=0xbcu
       || dictionary.dataEndBit!=253u || dictionary.ownerStartBit!=253u || dictionary.ownerEndBit!=269u
       || dictionary.rootStartBit!=269u || dictionary.rootEndBit!=285u)return 6;
    std::cout<<"same-import observation passed: 10 XRECORDs, 3 custom frames, 9 child links, 3 ordered root references, 9 child wire profiles and one-sided dictionary edges; "
             <<"entity failures="<<data.dwgReadOutcome.observations.entityParseFailures
             <<" object failures="<<data.dwgReadOutcome.observations.objectParseFailures<<'\n';
    return 0;
}
