// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dwgobjectframe.h"
#include "intern/dwgreader.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter21.h"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
dwgHandle ref(std::uint8_t code,std::uint32_t value) {
    dwgHandle h;h.code=code;h.ref=value;h.ref64=value;
    for(auto n=value;n;n>>=8u)++h.size;
    return h;
}
std::vector<std::uint32_t> children(unsigned count, bool legacy = false) {
    if (legacy) {
        std::vector<std::uint32_t> result{DRW::DwgLtypeByBlockHandle,
            DRW::DwgLtypeByLayerHandle,DRW::DwgLtypeContinuousHandle};
        for(unsigned i=1;i<count;++i)result.push_back(0x1000u+i);
        return result;
    }
    std::vector<std::uint32_t> result{DRW::DwgLtypeContinuousHandle};
    for(unsigned i=1;i<count;++i)result.push_back(0x1000u+i);
    result.push_back(DRW::DwgLtypeByLayerHandle);
    result.push_back(DRW::DwgLtypeByBlockHandle);
    return result;
}
bool slots(const DRW_ObjControl& control,unsigned count,bool legacy = false) {
    const auto expected=children(count,legacy);
    return control.orderedHandleSlots.size()==expected.size()
        &&std::equal(control.orderedHandleSlots.begin(),control.orderedHandleSlots.end(),expected.begin());
}
// Generated controls model three distinct sources: ODA's split handles at
// the exact bit boundary, older libdxfrw's inline handles with full-frame RL,
// and the corrected writer's byte-aligned data/handle boundary.
enum class Framing { Physical, LegacyInline, WriterAligned,
                     LibreCADEmptyStringFooter };
std::vector<std::uint8_t> build(unsigned count,bool shortCount,Framing framing,int bad=0) {
    dwgBufferW b;b.putObjType(DRW::AC1021,DRW::DwgLTypeControlObjectType);
    const auto sizeBit=b.bitCount();b.putRawLong32(0);
    b.putHandle(ref(0,DRW::DwgLtypeControlHandle));b.putBitShort(0);b.putBitLong(0);b.putBit(0);
    if(shortCount)b.putBitShort(count);else b.putBitLong(count);
    if(bad==5)b.putBit(0); // extra bit in purported exact producer body
    b.putBit(bad==6?1:0);
    if(framing==Framing::WriterAligned
       ||framing==Framing::LibreCADEmptyStringFooter) {
        const auto pad=(8u-(b.bitCount()&7u))&7u;
        for(std::uint64_t i=0;i<pad;++i)b.putBit(bad==7&&i==0?1:0);
        if(framing==Framing::LibreCADEmptyStringFooter) {
            for(int i=0;i<7;++i)b.putBit(0);
            b.putRawShort16(bad==11?1u:0u);
            b.putBit(0);
        }
    }
    const auto boundary=static_cast<std::uint32_t>(b.bitCount());
    b.putHandle(ref(bad==1?0:DRW::DwgSoftPointer,0));b.putHandle(ref(DRW::DwgHardOwnership,0));
    const bool legacyGraph = framing==Framing::LegacyInline
        ||framing==Framing::LibreCADEmptyStringFooter;
    auto values=children(count,legacyGraph);
    if(bad==2)values[0]=DRW::DwgLtypeByLayerHandle;
    if(bad==3)values[3]=DRW::DwgLtypeContinuousHandle;
    for(std::size_t i=0;i<values.size();++i) {
        const auto role = bad==9&&i==0
            ? DRW::DwgHardOwnership
            : (!legacyGraph && i>=count ? DRW::DwgHardOwnership
                                        : DRW::DwgSoftOwnership);
        b.putHandle(ref(role,values[i]));
    }
    const auto tailPad=(8u-(b.bitCount()&7u))&7u;
    for(std::uint64_t i=0;i<tailPad;++i)b.putBit(bad==8&&i==0?1:0);
    auto declared=framing==Framing::LegacyInline
        ?static_cast<std::uint32_t>(b.bitCount()):boundary;
    if(bad==10)++declared;
    b.patchRawLong32AtBit(sizeBit,declared);
    auto data=b.data();if(bad==4)data.pop_back();return data;
}
std::vector<std::uint8_t> buildAc1021LayerControl(bool nonzeroTail = false) {
    constexpr std::uint32_t rootHandle = 0x30u;
    constexpr std::uint32_t layerHandle = 0x31u;
    dwgBufferW body;
    body.putObjType(DRW::AC1021, DRW::DwgLayerControlObjectType);
    const std::uint64_t boundaryField = body.bitCount();
    body.putRawLong32(0u);
    body.putFixedHandle(0u, 1u, rootHandle);
    body.putBitShort(0u); // no EED
    body.putBitLong(0);   // no persistent reactors
    body.putBit(0u);      // xdictionary handle is present
    body.putBitLong(1);   // one layer-table member
    body.putBit(0u);      // no string stream
    const auto objectDataEnd = static_cast<std::uint32_t>(body.bitCount());
    body.putFixedHandle(DRW::DwgSoftPointer, 0u, 0u);
    body.putFixedHandle(DRW::DwgHardOwnership, 0u, 0u);
    body.putFixedHandle(DRW::DwgSoftOwnership, 1u, layerHandle);
    const auto padding = (8u - (body.bitCount() & 7u)) & 7u;
    for (std::uint64_t bit = 0u; bit < padding; ++bit)
        body.putBit(nonzeroTail && bit == 0u ? 1u : 0u);
    body.patchRawLong32AtBit(boundaryField, objectDataEnd);
    return body.data();
}
bool parseAc1021LayerControl(const std::vector<std::uint8_t>& bytes,
                             DRW_ObjControl& control) {
    dwgBuffer input(bytes.data(), bytes.size());
    return control.parseDwg(DRW::AC1021, &input, 0u) && input.isGood()
        && control.handle == 0x30u
        && control.orderedHandleSlots == std::vector<std::uint32_t>{0x31u}
        && control.orderedHandleRoles
               == std::vector<std::uint8_t>{DRW::DwgSoftOwnership};
}
bool parse(std::vector<std::uint8_t>& body,unsigned count,
           DRW_ObjControl& control,bool legacy = false) {
    dwgBuffer input(body.data(),body.size());
    if (!control.parseDwg(DRW::AC1021,&input,0)
        || !slots(control,count,legacy)
        || control.orderedHandleRoles.size() != count + 2u)
        return false;
    for (std::size_t index = 0; index < control.orderedHandleRoles.size(); ++index) {
        const std::uint8_t expected = legacy
            ? DRW::DwgSoftOwnership
            : (index < count ? DRW::DwgSoftOwnership
                             : DRW::DwgHardOwnership);
        if (control.orderedHandleRoles[index] != expected)
            return false;
    }
    return true;
}
}
class DwgBlockRecordXrefWireTestAccess {
public:
    static bool check(unsigned count) {
        DRW_Header header;dwgWriter21 writer(nullptr,&header);const auto values=children(count);
        if(!writer.emitControlObject(DRW::DwgLTypeControlObjectType,DRW::DwgLtypeControlHandle,count,values))return false;
        const auto found=std::find_if(writer.m_objectMap.begin(),writer.m_objectMap.end(),
            [](const auto& item){return item.first==DRW::DwgLtypeControlHandle;});
        if(found==writer.m_objectMap.end())return false;
        auto& bytes=writer.m_buf.data();dwgBuffer input(bytes.data(),bytes.size());DwgObjectFrame frame;
        if(!frame.readAt(input,DRW::AC1021,found->second))return false;
        dwgBuffer body(frame.body().data(),frame.body().size());
        if(body.getObjType(DRW::AC1021)!=DRW::DwgLTypeControlObjectType)return false;
        const auto boundary=body.getRawLong32();
        if(body.getHandle().ref!=DRW::DwgLtypeControlHandle||body.getBitShort()!=0
            ||body.getBitLong()!=0||body.getBit()!=0)return false;
        auto tag=body.forkIndependent();
        if(tag.get2Bits()!=(count<256u?1:0)||body.getBitLong()!=static_cast<std::int32_t>(count)
            ||body.getBit()!=0)return false;
        const auto cursor=body.getPosition()*8u+body.getBitPos();
        const auto pad=(8u-(cursor&7u))&7u;
        if(cursor+pad!=boundary)return false;
        for(std::uint64_t i=0;i<pad;++i)if(body.getBit()!=0||!body.isGood())return false;
        const auto owner=body.getHandle(),xdict=body.getHandle();
        if(owner.code!=DRW::DwgSoftPointer||owner.ref!=0||xdict.code!=DRW::DwgHardOwnership||xdict.ref!=0)return false;
        for(std::size_t i=0;i<values.size();++i){const auto h=body.getHandle();
            const auto expectedRole = i < count ? DRW::DwgSoftOwnership
                                                : DRW::DwgHardOwnership;
            if(!body.isGood()||h.code!=expectedRole||h.ref!=values[i])return false;}
        dwgBuffer retry(frame.body().data(),frame.body().size());DRW_ObjControl control;
            if(!control.parseDwg(DRW::AC1021,&retry,0)||!slots(control,count))return false;
        // A second framed object proves the moved handle stream ends cleanly.
        constexpr std::uint32_t nextHandle=0x600u;
        if(!writer.emitControlObject(DRW::DwgLayerControlObjectType,nextHandle,0u,
                                      std::vector<std::uint32_t>{}))return false;
        const auto next=std::find_if(writer.m_objectMap.begin(),writer.m_objectMap.end(),
            [](const auto& item){return item.first==nextHandle;});
        if(next==writer.m_objectMap.end())return false;
        auto& bytesAfter=writer.m_buf.data();dwgBuffer section(bytesAfter.data(),bytesAfter.size());
        DwgObjectFrame nextFrame;if(!nextFrame.readAt(section,DRW::AC1021,next->second))return false;
        dwgBuffer nextBody(nextFrame.body().data(),nextFrame.body().size());
        return nextBody.getObjType(DRW::AC1021)==DRW::DwgLayerControlObjectType&&nextBody.isGood();
    }
};
int main(int argc,char**argv) {
    if(argc==4&&std::string(argv[1])=="--historical") {
        const auto count=static_cast<unsigned>(std::stoul(argv[2]));
        std::ifstream f(argv[3],std::ios::binary);
        std::vector<std::uint8_t> body((std::istreambuf_iterator<char>(f)),{});
        if(!f||body.empty())return 2;
        dwgBuffer input(body.data(),body.size());DRW_ObjControl control;
        if(!control.parseDwg(DRW::AC1021,&input,0)
            ||control.orderedHandleSlots.size()!=count+2u
            ||control.orderedHandleSlots[0]!=DRW::DwgLtypeByBlockHandle
            ||control.orderedHandleSlots[1]!=DRW::DwgLtypeByLayerHandle
            ||control.orderedHandleSlots[2]!=DRW::DwgLtypeContinuousHandle)return 1;
        std::cout<<"historical AC1021 LTYPE_CONTROL PASS\n";return 0;
    }
    if(argc==7&&std::string(argv[1])=="--physical") {
        const auto count=static_cast<unsigned>(std::stoul(argv[2]));const auto offset=std::stoull(argv[4]);const auto size=std::stoull(argv[5]);
        const auto expectedBoundary=std::stoull(argv[6]);
        std::ifstream f(argv[3],std::ios::binary);std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)),{});
        if(!f||offset>bytes.size()||size>bytes.size()-offset)return 2;
        dwgBuffer probe(bytes.data()+offset,size);
        if(probe.getObjType(DRW::AC1021)!=DRW::DwgLTypeControlObjectType)return 1;
        const auto boundary=probe.getRawLong32();
        if(boundary!=expectedBoundary)return 1;
        if(probe.getHandle().ref==0||probe.getBitShort()!=0||probe.getBitLong()!=0||probe.getBit()!=1
            ||probe.getBitLong()!=static_cast<std::int32_t>(count)||probe.getBit()!=0
            ||probe.getPosition()*8u+probe.getBitPos()!=boundary)return 1;
        const auto nullOwner=probe.getHandle();
        if(!probe.isGood()||nullOwner.code!=DRW::DwgSoftPointer||nullOwner.ref!=0)return 1;
        for(unsigned i=0;i<count+2u;++i) {
            const auto child=probe.getHandle();
            const auto expectedRole=i<count?DRW::DwgSoftOwnership
                                           :DRW::DwgHardOwnership;
            if(!probe.isGood()||child.code!=expectedRole)return 1;
        }
        dwgBuffer input(bytes.data()+offset,size);DRW_ObjControl c;
        if(!c.parseDwg(DRW::AC1021,&input,0)||c.orderedHandleSlots.size()!=count+2u)return 1;
        std::cout<<"physical ODA AC1021 LTYPE_CONTROL PASS\n";return 0;
    }
    if(argc!=1)return 2;
    for(unsigned count:{255u,256u,257u}) {
        if(!DwgBlockRecordXrefWireTestAccess::check(count)){
            std::cerr<<"writer boundary/readback failed count="<<count<<'\n';return 1;}
        for(auto framing:{Framing::Physical,Framing::LegacyInline,Framing::WriterAligned}) {
            const bool old=framing==Framing::LegacyInline;
            auto body=build(count,old,framing);DRW_ObjControl c;
            if(!parse(body,count,c,old)){std::cerr<<"generated failure count="<<count<<" framing="<<static_cast<int>(framing)<<" old="<<old<<'\n';return 1;}
        }
        for(int bad:{1,2,3,4,5,6,8,9,10}) {auto invalid=build(count,true,Framing::LegacyInline,bad);DRW_ObjControl c;
            if(parse(invalid,count,c,true)){std::cerr<<"invalid legacy accepted mode="<<bad<<'\n';return 1;}
            auto good=build(count,true,Framing::LegacyInline);
            if(!parse(good,count,c,true)){std::cerr<<"legacy retry failed\n";return 1;}}
        for(int bad:{5,6}){auto invalid=build(count,false,Framing::Physical,bad);DRW_ObjControl c;
            if(parse(invalid,count,c)){std::cerr<<"invalid physical framing accepted mode="<<bad<<'\n';return 1;}
            auto good=build(count,false,Framing::Physical);if(!parse(good,count,c))return 1;}
        auto invalid=build(count,false,Framing::WriterAligned,7);DRW_ObjControl c;
        if(parse(invalid,count,c)){std::cerr<<"nonzero writer alignment accepted\n";return 1;}
        auto goodAligned=build(count,false,Framing::WriterAligned);
        if(!parse(goodAligned,count,c)){std::cerr<<"aligned writer retry failed\n";return 1;}
        auto badBoundary=build(count,false,Framing::WriterAligned,10);
        if(parse(badBoundary,count,c)||!parse(goodAligned,count,c)){
            std::cerr<<"writer boundary rejection/retry failed\n";return 1;}
    }
    auto libreCadFooter=build(1,false,Framing::LibreCADEmptyStringFooter);
    DRW_ObjControl libreCadControl;
    if(!parse(libreCadFooter,1,libreCadControl,true)) {
        std::cerr<<"LibreCAD AC1021 empty-string footer rejected\n";return 1;
    }
    for(int bad:{4,7,9,11}) {
        auto invalid=build(1,false,Framing::LibreCADEmptyStringFooter,bad);
        const bool rejected=!parse(invalid,1,libreCadControl,true);
        if(!rejected) {
            std::cerr<<"invalid LibreCAD empty-string footer accepted mode="
                     <<bad<<'\n';return 1;
        }
        auto retry=build(1,false,Framing::LibreCADEmptyStringFooter);
        if(!parse(retry,1,libreCadControl,true)) {
            std::cerr<<"LibreCAD empty-string footer same-object retry failed mode="
                     <<bad<<'\n';return 1;
        }
    }
    DRW_ObjControl layerControl;
    const auto malformedLayer = buildAc1021LayerControl(true);
    if (parseAc1021LayerControl(malformedLayer, layerControl)
        || !layerControl.orderedHandleSlots.empty()
        || !parseAc1021LayerControl(buildAc1021LayerControl(), layerControl)) {
        std::cerr << "AC1021 detached table-control boundary refusal/retry failed\n";
        return 1;
    }
    std::cout<<"AC1021 LTYPE_CONTROL count, boundary and retry PASS\n";return 0;
}
