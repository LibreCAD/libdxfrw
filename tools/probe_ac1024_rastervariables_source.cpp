// SPDX-License-Identifier: GPL-2.0-or-later
// Source-specific scratch probe: offsets describe only the ODA source bound
// to the SHA-256 in the accompanying research receipt.
#include "drw_objects.h"
#include "intern/drw_textcodec.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgutil.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

class DrwObjectEncodeTestAccess {
public:
    static bool parse(DRW_RasterVariables& value,dwgBuffer& buf,std::uint32_t bits) {
        return value.parseDwg(DRW::AC1024,&buf,bits);
    }
    static bool parse(DRW_Dictionary& value,dwgBuffer& buf,std::uint32_t bits) {
        return value.parseDwg(DRW::AC1024,&buf,bits);
    }
};
struct Frame {
    std::vector<std::uint8_t> body;
    std::uint32_t handleBits=0;
};
bool frameAt(const std::vector<std::uint8_t>& page,std::size_t offset,Frame& result) {
    if(offset>=page.size())return false;
    dwgBuffer input(page.data()+offset,page.size()-offset);
    const auto length=input.getModularShort();const auto bits=input.getUModularChar();
    const auto prefix=input.getPosition();
    if(length<=0||prefix+static_cast<std::size_t>(length)+2u>page.size()-offset)return false;
    const auto crcPos=prefix+static_cast<std::size_t>(length);
    dwgBuffer crc(page.data()+offset+crcPos,2u),payload(page.data()+offset,crcPos);
    if(crc.getRawShort16()!=payload.crc8(0xC0C1u,0,static_cast<std::int32_t>(crcPos)))return false;
    result.body.assign(page.begin()+static_cast<std::ptrdiff_t>(offset+prefix),
                       page.begin()+static_cast<std::ptrdiff_t>(offset+prefix+length));
    result.handleBits=bits;return true;
}
int main(int argc,char** argv) {
    if(argc!=2){std::cerr<<"usage: probe_ac1024_rastervariables_source <documented-source.dwg>\n";return 2;}
    std::ifstream source(argv[1],std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(source)),{});
    constexpr std::size_t pageStart=1696,compressedSize=6670,pageSize=29696;
    if(bytes.size()<pageStart+compressedSize)return 2;
    std::vector<std::uint8_t> page(pageSize);dwgCompressor codec;
    if(!codec.decompress18(bytes.data()+pageStart,page.data(),compressedSize,page.size()))return 2;
    Frame raster,nod;if(!frameAt(page,2608,raster)||!frameAt(page,136,nod)){std::cerr<<"frame or CRC failed\n";return 1;}
    if(raster.body.size()!=12u||raster.handleBits!=32u||nod.body.size()!=584u||nod.handleBits!=282u){std::cerr<<"frame sizes failed\n";return 1;}
    DRW_TextCodec text;text.setVersion(DRW::AC1024,false);
    dwgBuffer raw(raster.body.data(),raster.body.size(),&text);
    if(raw.getObjType(DRW::AC1024)!=512u||raw.getHandle().ref!=0x2Fu
       ||raw.getBitShort()!=0u||raw.getBitLong()!=1||raw.getBit()!=1u
       ||raw.getBitLong()!=0||raw.getBitShort()!=0u||raw.getBitShort()!=1u
       ||raw.getBitShort()!=3u||raw.getPosition()*8u+raw.getBitPos()!=63u
       ||raw.getBit()!=0u){std::cerr<<"raw scalar span failed\n";return 1;}
    const auto parent=raw.getHandle(),reactor=raw.getHandle();
    if(!raw.isGood()||parent.code!=4u||parent.ref!=0xAu||reactor.code!=4u||reactor.ref!=0xAu){std::cerr<<"raw handles failed\n";return 1;}
    dwgBuffer rasterParser(raster.body.data(),raster.body.size(),&text);
    DRW_RasterVariables value;
    const bool parsed=DrwObjectEncodeTestAccess::parse(value,rasterParser,raster.handleBits);
    if(!parsed ||value.handle!=0x2Fu||value.parentHandle!=0xAu||value.reactorCount()!=1
       ||value.reactorHandles!=std::vector<std::uint32_t>{0xAu}||value.extensionDictionaryFlag()!=1u
       ||value.m_classVersion!=0||value.m_imageFrame!=0||value.m_imageQuality!=1||value.m_units!=3){std::cerr<<"local raster parser failed parsed="<<parsed<<" good="<<rasterParser.isGood()<<" handle="<<value.handle<<" owner="<<value.parentHandle<<" reactor="<<value.reactorCount()<<" flag="<<unsigned(value.extensionDictionaryFlag())<<" fields="<<value.m_classVersion<<','<<value.m_imageFrame<<','<<value.m_imageQuality<<','<<value.m_units<<'\n';return 1;}
    dwgBuffer nodParser(nod.body.data(),nod.body.size(),&text);
    DRW_Dictionary dictionary;
    if(!DrwObjectEncodeTestAccess::parse(dictionary,nodParser,nod.handleBits)
        ||dictionary.handle!=0xAu||!dictionary.hasCompleteDwgEntries()){std::cerr<<"local NOD parser failed\n";return 1;}
    const auto matches=std::count_if(dictionary.m_entries.begin(),dictionary.m_entries.end(),
        [](const auto& entry){return entry.m_name=="ACAD_IMAGE_VARS"&&entry.m_handle==0x2Fu;});
    if(matches!=1){std::cerr<<"NOD binding failed\n";return 1;}
    std::cout<<"AC1024 source RASTERVARIABLES and NOD field/role spans PASS\n";return 0;
}
