// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dwgobjectframe.h"
#include "intern/dwgreader.h"
#include "intern/dwgbufferw.h"
#include "intern/dwgwriter18.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace {
class LTypeParser : public DRW_LType {
public:
    bool parseBody(DRW::Version version, dwgBuffer *buffer) {
        return parseDwg(version, buffer, 0u);
    }
};

dwgHandle ref(std::uint8_t code, std::uint32_t value) {
    dwgHandle h; h.code=code; h.ref=value; h.ref64=value;
    for (auto n=value; n; n >>= 8u) ++h.size;
    return h;
}
std::vector<std::uint32_t> children(unsigned count, bool legacyOrder = false) {
    if (legacyOrder) {
        std::vector<std::uint32_t> result{DRW::DwgLtypeByBlockHandle,
            DRW::DwgLtypeByLayerHandle, DRW::DwgLtypeContinuousHandle};
        for (unsigned i=1; i<count; ++i) result.push_back(0x1000u+i);
        return result;
    }
    std::vector<std::uint32_t> result{DRW::DwgLtypeContinuousHandle};
    for (unsigned i=1; i<count; ++i) result.push_back(0x1000u+i);
    result.push_back(DRW::DwgLtypeByLayerHandle);
    result.push_back(DRW::DwgLtypeByBlockHandle);
    return result;
}
bool slots(const DRW_ObjControl& obj, unsigned count,
           bool legacyOrder = false) {
    const auto expected=children(count, legacyOrder);
    return obj.orderedHandleSlots.size()==expected.size()
        && std::equal(obj.orderedHandleSlots.begin(),obj.orderedHandleSlots.end(),expected.begin());
}
struct Body {std::vector<std::uint8_t> data;std::uint32_t handleBits;};
Body build(DRW::Version version,unsigned count,bool legacy,int corruption=0,
           bool forceLong=false,bool legacyOrder=false,
           bool previousCurrentWriterOrder=false,
           bool historicalSoftPointerTail=false) {
    dwgBufferW b;
    b.putObjType(version,DRW::DwgLTypeControlObjectType);
    const auto sizeBit=b.bitCount();
    if(version<DRW::AC1024)b.putRawLong32(0);
    b.putHandle(ref(0,DRW::DwgLtypeControlHandle));
    b.putBitShort(0);b.putBitLong(0);
    if(version>DRW::AC1015)b.putBit(0);
    if(version>DRW::AC1024)b.putBit(0);
    if(legacy)b.putBitShort(count);
    else if(forceLong){b.put2Bits(0);b.putRawLong32(count);}
    else b.putBitLong(count);
    if(corruption==5)b.putBit(0);
    if(version>DRW::AC1018)b.putBit(corruption==6 ? 1 : 0);
    if(version>=DRW::AC1024 && corruption>=7 && corruption<=12) {
        const auto pad=(8u-(b.bitCount()&7u))&7u;
        for(std::uint64_t bit=0;bit<pad;++bit)
            b.putBit(corruption==10 && bit==0 ? 1 : 0);
        for(int bit=0;bit<(corruption==9 ? 23 : 24);++bit)
            b.putBit(corruption==8 && bit==12 ? 1 : 0);
    }
    const auto boundary=static_cast<std::uint32_t>(b.bitCount());
    if(version<DRW::AC1024)b.patchRawLong32AtBit(sizeBit,boundary);
    b.putHandle(ref(corruption==1?0:DRW::DwgSoftPointer,
                    corruption==14?0x99u:0u));
    b.putHandle(ref(corruption==13?0:DRW::DwgHardOwnership,
                    corruption==15?0x98u:0u));
    auto values=children(count, legacyOrder);
    if (previousCurrentWriterOrder && !legacyOrder)
        std::swap(values[count], values[count + 1u]);
    if(corruption==2)values[0]=DRW::DwgLtypeByLayerHandle;
    if(corruption==3)values[3]=DRW::DwgLtypeContinuousHandle;
    for(std::size_t i=0;i<values.size();++i) {
        const auto role = corruption==12 && i==count
            ? DRW::DwgSoftOwnership
            : (historicalSoftPointerTail ? DRW::DwgSoftPointer
               : (legacyOrder ? DRW::DwgSoftOwnership
                           : (i >= count ? DRW::DwgHardOwnership
                                         : DRW::DwgSoftOwnership)));
        b.putHandle(ref(role,values[i]));
    }
    b.alignToByte();
    if(version==DRW::AC1015)
        b.patchRawLong32AtBit(sizeBit,static_cast<std::uint32_t>(b.bitCount()));
    auto data=b.data();
    if(corruption==4)data.pop_back();
    const auto handleBits=version>=DRW::AC1024
        ? static_cast<std::uint32_t>(data.size()*8u-boundary):0u;
    return {std::move(data),handleBits};
}
bool parse(const Body& b,DRW::Version version,unsigned count,
           DRW_ObjControl& object,bool legacyOrder=false,
           bool previousCurrentWriterOrder=false,
           bool historicalSoftPointerTail=false) {
    dwgBuffer in(const_cast<std::uint8_t*>(b.data.data()),b.data.size());
    if (!object.parseDwg(version,&in,b.handleBits)
        || object.orderedHandleRoles.size() != count + 2u)
        return false;
    auto expected=children(count,legacyOrder);
    if(previousCurrentWriterOrder&&!legacyOrder)
        std::swap(expected[count],expected[count+1u]);
    if(object.orderedHandleSlots!=expected)return false;
    for (std::size_t index = 0; index < object.orderedHandleRoles.size(); ++index) {
        const std::uint8_t expected = historicalSoftPointerTail
            ? DRW::DwgSoftPointer
            : (legacyOrder ? DRW::DwgSoftOwnership
            : (index < count ? DRW::DwgSoftOwnership
                             : DRW::DwgHardOwnership));
        if (object.orderedHandleRoles[index] != expected)
            return false;
    }
    return true;
}

bool checkAc1015InlineLtypeHandleTail() {
    const auto makeNameOnlyBuiltin = [](const std::string& name,
                                        std::uint32_t handle) {
        dwgBufferW framed;
        framed.putObjType(DRW::AC1015, DRW::DwgLTypeObjectType);
        const std::size_t sizeFieldBit = framed.bitCount();
        framed.putRawLong32(0u);
        framed.putHandle(ref(DRW::DwgSoftPointer, handle));
        framed.putBitShort(0); // EED terminator
        framed.putBitLong(0);  // reactor count
        framed.putVariableText(DRW::AC1015, name);
        framed.patchRawLong32AtBit(sizeFieldBit,
            static_cast<std::uint32_t>(framed.bitCount()));
        framed.alignToByte();
        return framed.data();
    };
    for (const auto& builtin : std::vector<std::pair<std::string, std::uint32_t>>{
             {"BYBLOCK", 0x0Fu}, {"BYLAYER", 0x10u}, {"CONTINUOUS", 0x11u}}) {
        auto bytes = makeNameOnlyBuiltin(builtin.first, builtin.second);
        dwgBuffer input(bytes.data(), bytes.size());
        LTypeParser parsed;
        if (!parsed.parseBody(DRW::AC1015, &input) || !input.isGood()
            || parsed.name != builtin.first || parsed.handle != builtin.second
            || parsed.size != 0 || !parsed.path.empty()
            || parsed.length != 0.0 || parsed.alignment != 'A'
            || parsed.hasDwgCommonLinkTail())
            return false;
    }

    DRW_LType source;
    source.updateValues("GENERATED_DASH", "generated AC1015 simple pattern",
                        2, 3.0, std::vector<double>{2.0, -1.0});
    source.handle = 0x5A0u;
    dwgBufferW body;
    dwgBufferW handleStream;
    const bool encoded = source.encodeDwg(DRW::AC1015, &body, nullptr,
                                          &handleStream);
    if (!encoded || body.data().empty() || handleStream.data().empty()) {
        std::cerr << "generated AC1015 LTYPE encode=" << encoded
                  << " body=" << body.data().size() << " handles="
                  << handleStream.data().size() << '\n';
        return false;
    }

    const auto makeFrame = [&](std::uint64_t omittedHandleBits) {
        dwgBufferW framed;
        framed.putObjType(DRW::AC1015, DRW::DwgLTypeObjectType);
        const std::size_t sizeFieldBit = framed.bitCount();
        framed.putRawLong32(0u);
        framed.putHandle(ref(DRW::DwgSoftPointer, source.handle));
        framed.putBitShort(0); // EED terminator
        framed.putBitLong(0);  // reactor count
        const auto appendBits = [&framed](const dwgBufferW& stream,
                                          std::uint64_t bitCount) {
            const auto& bytes = stream.data();
            for (std::uint64_t bit = 0; bit < bitCount; ++bit) {
                const std::uint8_t value = static_cast<std::uint8_t>(
                    (bytes[static_cast<std::size_t>(bit >> 3u)]
                     >> (7u - static_cast<unsigned>(bit & 7u))) & 1u);
                framed.putBit(value);
            }
        };
        appendBits(body, body.bitCount());
        appendBits(handleStream, handleStream.bitCount() - omittedHandleBits);
        framed.patchRawLong32AtBit(sizeFieldBit,
            static_cast<std::uint32_t>(framed.bitCount()));
        framed.alignToByte();
        return framed.data();
    };

    const auto parseExpected = [&](const std::vector<std::uint8_t>& bytes,
                                   LTypeParser& parsed) {
        dwgBuffer input(const_cast<std::uint8_t*>(bytes.data()), bytes.size());
        return parsed.parseBody(DRW::AC1015, &input) && input.isGood()
            && parsed.name == source.name && parsed.desc == source.desc
            && parsed.handle == source.handle && parsed.size == 2
            && parsed.path.size() == 2u && parsed.path[0] == 2.0
            && parsed.path[1] == -1.0 && parsed.length == 3.0
            && !parsed.hasDwgCommonLinkTail();
    };

    LTypeParser parsed;
    if (!parseExpected(makeFrame(0u), parsed)
        || !parseExpected(makeFrame(8u), parsed))
        return false;
    if (parseExpected(makeFrame(handleStream.bitCount()), parsed))
        return false;
    if (!parseExpected(makeFrame(0u), parsed))
        return false;

    DRW_LType complexSource;
    complexSource.updateValues(
        "GENERATED_SHAPE", "generated AC1015 shape resource", 1, 2.0,
        std::vector<double>{2.0});
    complexSource.handle = 0x5A1u;
    DRW_LTypeSegment shapeSegment;
    shapeSegment.length = 2.0;
    shapeSegment.complexShapeCode = 42;
    shapeSegment.styleHandle = ref(DRW::DwgHardPointer, 0x31u);
    complexSource.segments.push_back(shapeSegment);
    complexSource.shapeHandles.push_back(0x31u);
    source = complexSource;
    body = dwgBufferW{};
    handleStream = dwgBufferW{};
    if (!source.encodeDwg(DRW::AC1015, &body, nullptr, &handleStream))
        return false;

    // Unlike simple dash segments, a complex shape segment requires its
    // non-null style reference. An exhausted inline tail must not synthesize
    // a null handle for this resource-bearing record. The same parser accepts
    // the complete frame after refusing the truncated resource reference.
    const auto missingShapeHandleBytes = makeFrame(8u);
    dwgBuffer truncatedShapeInput(
        const_cast<std::uint8_t*>(missingShapeHandleBytes.data()),
        missingShapeHandleBytes.size());
    LTypeParser complexParsed;
    if (complexParsed.parseBody(DRW::AC1015, &truncatedShapeInput))
        return false;
    const auto completeShapeBytes = makeFrame(0u);
    dwgBuffer completeShapeInput(
        const_cast<std::uint8_t*>(completeShapeBytes.data()),
        completeShapeBytes.size());
    return complexParsed.parseBody(DRW::AC1015, &completeShapeInput)
        && completeShapeInput.isGood()
        && complexParsed.name == source.name
        && complexParsed.size == 1
        && complexParsed.segments.size() == 1u
        && complexParsed.segments[0].complexShapeCode == 42
        && complexParsed.segments[0].styleHandle.ref == 0x31u
        && complexParsed.shapeHandles == std::vector<std::uint32_t>{0x31u};
}
}

class DwgBlockRecordXrefWireTestAccess {
public:
    template<class Writer> static bool check(unsigned count) {
        DRW_Header header;Writer writer(nullptr,&header);
        auto values=children(count);
        if(!writer.emitControlObject(DRW::DwgLTypeControlObjectType,
                                     DRW::DwgLtypeControlHandle,count,values)) {
            std::cerr<<"emit failed version="<<writer.m_version<<" count="<<count<<'\n';
            return false;
        }
        const auto found=std::find_if(writer.m_objectMap.begin(),writer.m_objectMap.end(),
            [](const auto& e){return e.first==DRW::DwgLtypeControlHandle;});
        if(found==writer.m_objectMap.end())return false;
        auto& data=writer.m_buf.data();dwgBuffer in(data.data(),data.size());DwgObjectFrame frame;
        if(!frame.readAt(in,writer.m_version,found->second))return false;
        dwgBuffer body(frame.body().data(),frame.body().size());
        if(body.getObjType(writer.m_version)!=DRW::DwgLTypeControlObjectType)return false;
        std::uint32_t boundary=0;
        if(writer.m_version<DRW::AC1024)boundary=body.getRawLong32();
        else boundary=frame.objectDataBitSize();
        if(body.getHandle().ref!=DRW::DwgLtypeControlHandle||body.getBitShort()!=0
            ||body.getBitLong()!=0||body.getBit()!=0)return false;
        if(writer.m_version>DRW::AC1024&&body.getBit()!=0)return false;
        dwgBuffer tag=body.forkIndependent();
        if(tag.get2Bits()!=(count<256?1:0)||body.getBitLong()!=static_cast<std::int32_t>(count))return false;
        if(writer.m_version>DRW::AC1018&&body.getBit()!=0)return false;
        const auto cursor=body.getPosition()*8u+body.getBitPos();
        if(writer.m_version<DRW::AC1024) {
            if(cursor!=boundary)return false;
        } else {
            // The existing modern writer pads to a byte, then writes its
            // 24-bit absent-string trailer before the handle section.
            const auto pad=(8u-(cursor&7u))&7u;
            if(cursor+pad+24u!=boundary)return false;
            for(std::uint64_t bit=0;bit<pad+24u;++bit)
                if(body.getBit()!=0||!body.isGood())return false;
        }
        const auto owner=body.getHandle(),xdict=body.getHandle();
        if(owner.code!=DRW::DwgSoftPointer||owner.ref!=0
            ||xdict.code!=DRW::DwgHardOwnership||xdict.ref!=0)return false;
        for(std::size_t i=0;i<values.size();++i){const auto h=body.getHandle();
            const auto expectedRole = i < count ? DRW::DwgSoftOwnership
                                                : DRW::DwgHardOwnership;
            if(!body.isGood()||h.code!=expectedRole||h.ref!=values[i]) {
                std::cerr<<"child mismatch version="<<writer.m_version<<" index="<<i
                    <<" role="<<static_cast<unsigned>(h.code)<<" ref="<<h.ref
                    <<" expectedRole="<<static_cast<unsigned>(expectedRole)
                    <<" expectedRef="<<values[i]<<'\n';
                return false;
            }}
        dwgBuffer again(frame.body().data(),frame.body().size());DRW_ObjControl parsed;
        const bool parsedOkay=parsed.parseDwg(writer.m_version,&again,frame.bodyBitSize());
        if(!parsedOkay||!slots(parsed,count)) {
            std::cerr<<"readback mismatch version="<<writer.m_version<<" count="<<count
                <<" parsed="<<parsedOkay<<" size="<<parsed.orderedHandleSlots.size()<<'\n';
            return false;
        }
        return true;
    }

    template<class Writer> static bool checkAppIdCount(unsigned count) {
        DRW_Header header;
        Writer writer(nullptr, &header);
        std::vector<std::uint32_t> values;
        values.reserve(count);
        for (unsigned index = 0; index < count; ++index)
            values.push_back(0x2000u + index);
        if (!writer.emitControlObject(DRW::DwgAppIdControlObjectType,
                                      DRW::DwgAppIdControlHandle, count,
                                      values))
            return false;
        const auto found = std::find_if(
            writer.m_objectMap.begin(), writer.m_objectMap.end(),
            [](const auto& entry) {
                return entry.first == DRW::DwgAppIdControlHandle;
            });
        if (found == writer.m_objectMap.end())
            return false;

        auto& encoded = writer.m_buf.data();
        dwgBuffer source(encoded.data(), encoded.size());
        DwgObjectFrame frame;
        if (!frame.readAt(source, writer.m_version, found->second))
            return false;
        dwgBuffer body(frame.body().data(), frame.body().size());
        if (body.getObjType(writer.m_version)
                != DRW::DwgAppIdControlObjectType)
            return false;
        const std::uint32_t objectDataEnd = writer.m_version < DRW::AC1024
            ? body.getRawLong32() : frame.objectDataBitSize();
        if (body.getHandle().ref != DRW::DwgAppIdControlHandle
            || body.getBitShort() != 0u || body.getBitLong() != 0
            || (writer.m_version > DRW::AC1015 && body.getBit() != 0u)
            || (writer.m_version > DRW::AC1024 && body.getBit() != 0u))
            return false;
        const auto countStart = body.getPosition() * 8u + body.getBitPos();
        if (body.getBitLong() != static_cast<std::int32_t>(count))
            return false;
        if (writer.m_version > DRW::AC1018 && body.getBit() != 0u)
            return false;
        const auto fieldsEnd = body.getPosition() * 8u + body.getBitPos();
        if (countStart >= fieldsEnd)
            return false;
        if (writer.m_version < DRW::AC1024) {
            // R2004/R2007 writer stores a direct object-data bit boundary.
            if (objectDataEnd != fieldsEnd)
                return false;
        } else {
            if (fieldsEnd > objectDataEnd)
                return false;
            while (body.getPosition() * 8u + body.getBitPos() < objectDataEnd)
                if (body.getBit() != 0u || !body.isGood())
                    return false;
        }

        dwgBuffer retry(frame.body().data(), frame.body().size());
        DRW_ObjControl parsed;
        const bool parseOk = parsed.parseDwg(
            writer.m_version, &retry, frame.bodyBitSize());
        if (!parseOk || parsed.orderedHandleSlots != values
            || parsed.orderedHandleRoles.size() != count
            || !std::all_of(parsed.orderedHandleRoles.begin(),
                            parsed.orderedHandleRoles.end(),
                            [](std::uint8_t role) {
                                return role == DRW::DwgSoftOwnership;
                            })) {
            std::cerr << "diagnostic: parse=" << parseOk
                      << " slots=" << parsed.orderedHandleSlots.size()
                      << " roles=" << parsed.orderedHandleRoles.size()
                      << " bits=" << frame.objectDataBitSize()
                      << " bodyBits=" << frame.bodyBitSize()
                      << " count=" << count << '\n';
            return false;
        }
        return true;
    }

    template<class Writer> static bool checkAppIdCountBoundaries() {
        for (unsigned count : {255u, 256u, 257u})
            if (!checkAppIdCount<Writer>(count))
                return false;
        return true;
    }
};

int main(int argc,char** argv){
    if(argc==8&&std::string(argv[1])=="--physical") {
        const std::string v(argv[2]);
        const DRW::Version version=v=="ac1018"?DRW::AC1018:v=="ac1024"?DRW::AC1024:
            v=="ac1027"?DRW::AC1027:v=="ac1032"?DRW::AC1032:DRW::UNKNOWNV;
        if(version==DRW::UNKNOWNV)return 2;
        const unsigned count=static_cast<unsigned>(std::stoul(argv[3]));
        const auto offset=std::stoull(argv[5]);const auto size=std::stoull(argv[6]);
        const auto suppliedBoundary=std::stoull(argv[7]);
        std::ifstream file(argv[4],std::ios::binary);std::vector<std::uint8_t> bytes(
            (std::istreambuf_iterator<char>(file)),{});
        if(!file||offset>bytes.size()||size>bytes.size()-offset)return 2;
        dwgBuffer input(bytes.data()+offset,size);
        // The caller supplies the independently extracted physical data/handle
        // boundary, so the count decoder cannot define its own expected end.
        dwgBuffer probe=input.forkIndependent();
        if(probe.getObjType(version)!=DRW::DwgLTypeControlObjectType)return 1;
        std::uint64_t boundary=0;
        if(version<DRW::AC1024)boundary=probe.getRawLong32();
        if(probe.getHandle().ref==0||probe.getBitShort()!=0||probe.getBitLong()!=0)return 1;
        if(probe.getBit()!=1)return 1; // ODA producer's omitted XDICTIONARY.
        if(version>DRW::AC1024&&probe.getBit()!=0)return 1;
        if(probe.getBitLong()!=static_cast<std::int32_t>(count))return 1;
        if(version>DRW::AC1018&&probe.getBit()!=0)return 1;
        const auto end=probe.getPosition()*8u+probe.getBitPos();
        if(end!=suppliedBoundary)return 1;
        if(version<DRW::AC1024&&boundary!=suppliedBoundary)return 1;
        boundary=suppliedBoundary;
        if(boundary>size*8u)return 1;
        const auto nullOwner=probe.getHandle();
        if(!probe.isGood()||nullOwner.code!=DRW::DwgSoftPointer||nullOwner.ref!=0)return 1;
        for(unsigned i=0;i<count+2u;++i) {
            const auto child=probe.getHandle();
            const auto expectedRole=i<count?DRW::DwgSoftOwnership
                                           :DRW::DwgHardOwnership;
            if(!probe.isGood()||child.code!=expectedRole)return 1;
        }
        const auto bs=version>=DRW::AC1024?static_cast<std::uint32_t>(size*8u-boundary):0u;
        DRW_ObjControl parsed;
        if(!parsed.parseDwg(version,&input,bs)||parsed.orderedHandleSlots.size()!=count+2u)return 1;
        std::cout<<"physical ODA LTYPE_CONTROL PASS\n";return 0;
    }
    if(argc!=1)return 2;
    if (!checkAc1015InlineLtypeHandleTail()) {
        std::cerr << "AC1015 inline LTYPE handle-tail profile failed\n";
        return 1;
    }
    if (!DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter15>()) {
        std::cerr << "AC1015 APPID_CONTROL BL writer boundary failure\n";
        return 1;
    }
    for(auto version:{DRW::AC1018,
                      DRW::AC1024,DRW::AC1027,DRW::AC1032}) {
        for(unsigned count:{255u,256u,257u}) {
            bool wire=false;
            if(version==DRW::AC1018)wire=DwgBlockRecordXrefWireTestAccess::check<dwgWriter18>(count);
            if(version==DRW::AC1024)wire=DwgBlockRecordXrefWireTestAccess::check<dwgWriter24>(count);
            if(version==DRW::AC1027)wire=DwgBlockRecordXrefWireTestAccess::check<dwgWriter27>(count);
            if(version==DRW::AC1032)wire=DwgBlockRecordXrefWireTestAccess::check<dwgWriter32>(count);
            if(!wire){std::cerr<<"writer wire failure version="<<version<<" count="<<count<<'\n';return 1;}
            for(bool legacy:{false,true}){auto body=build(version,count,legacy);DRW_ObjControl c;
                if(!parse(body,version,count,c)){std::cerr<<"reader failure version="<<version<<" count="<<count<<" legacy="<<legacy<<'\n';return 1;}}
        }
        bool appIdWire=false;
        if(version==DRW::AC1018)appIdWire=DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter18>();
        if(version==DRW::AC1024)appIdWire=DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter24>();
        if(version==DRW::AC1027)appIdWire=DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter27>();
        if(version==DRW::AC1032)appIdWire=DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter32>();
        if(!appIdWire){std::cerr<<"APPID_CONTROL BL writer boundary failure version="<<version<<'\n';return 1;}
        for(int bad:{1,2,3,4,5}){auto invalid=build(version,257,true,bad);DRW_ObjControl c;
            if(parse(invalid,version,257,c)){std::cerr<<"invalid legacy accepted version="<<version<<" mode="<<bad<<'\n';return 1;}
            auto retry=build(version,257,false);if(!parse(retry,version,257,c)){std::cerr<<"retry failure\n";return 1;}}
        if(version>DRW::AC1018){auto invalid=build(version,257,true,6);DRW_ObjControl c;
            if(parse(invalid,version,257,c)){std::cerr<<"string footer accepted\n";return 1;}}
        if(version>=DRW::AC1024)for(unsigned count:{255u,256u,257u})for(bool legacy:{false,true}) {
            auto padded=build(version,count,legacy,7);DRW_ObjControl c;
            if(!parse(padded,version,count,c)){std::cerr<<"old writer zero footer rejected\n";return 1;}
            for(int bad:{8,9,10}) {auto invalid=build(version,count,legacy,bad);
                if(parse(invalid,version,count,c)){std::cerr<<"malformed old writer footer accepted\n";return 1;}
                if(!parse(padded,version,count,c)){std::cerr<<"footer retry failed\n";return 1;}}
        }
        if(version>=DRW::AC1024) {
            auto wrongTailRole=build(version,257,false,12);DRW_ObjControl c;
            if(parse(wrongTailRole,version,257,c)) {
                std::cerr<<"ODA-profile phantom owner role accepted version="
                    <<version<<'\n';return 1;
            }
            auto validTail=build(version,257,false,7);
            if(!parse(validTail,version,257,c)) {
                std::cerr<<"ODA-profile phantom role retry failed version="
                    <<version<<'\n';return 1;
            }
            auto oldWriterProfile=build(version,257,false,7,false,true);
            if(!parse(oldWriterProfile,version,257,c,true)) {
                std::cerr<<"legacy writer-order profile rejected version="
                    <<version<<'\n';return 1;
            }
        }
        {
            auto previousWriterProfile=build(version,257,false,7,false,false,true);
            DRW_ObjControl control;
            if(!parse(previousWriterProfile,version,257,control,false,true)) {
                std::cerr<<"previous current-writer phantom order rejected version="
                    <<version<<'\n';return 1;
            }
        }
        for(unsigned count:{1u,255u}){auto bad=build(version,count,false,5);DRW_ObjControl c;
            if(parse(bad,version,count,c)){std::cerr<<"compact body gap accepted\n";return 1;}
            auto good=build(version,count,false);if(!parse(good,version,count,c))return 1;
            auto full=build(version,count,false,0,true);if(!parse(full,version,count,c))return 1;}
    }
    {
        constexpr unsigned count=25;
        DRW_ObjControl control;
        const auto historical=build(DRW::AC1015,count,false,0,false,true,
                                    false,true);
        if(!parse(historical,DRW::AC1015,count,control,true,false,true)) {
            std::cerr<<"AC1015 historical soft-pointer LTYPE_CONTROL rejected\n";
            return 1;
        }
        for(int nullHeader:{1,13}) {
            const auto nullProfile=build(DRW::AC1015,count,false,nullHeader,
                                         false,true,false,true);
            if(!parse(nullProfile,DRW::AC1015,count,control,true,false,true)) {
                std::cerr<<"AC1015 canonical-null LTYPE_CONTROL header rejected\n";
                return 1;
            }
        }
        for(int bad:{2,3,4,12,14,15}) {
            const auto invalid=build(DRW::AC1015,count,false,bad,false,true,
                                     false,true);
            if(parse(invalid,DRW::AC1015,count,control,true,false,true)) {
                std::cerr<<"AC1015 malformed historical LTYPE_CONTROL accepted mode="
                         <<bad<<'\n';
                return 1;
            }
            if(!parse(historical,DRW::AC1015,count,control,true,false,true)) {
                std::cerr<<"AC1015 historical LTYPE_CONTROL retry failed\n";
                return 1;
            }
        }
    }
    if (!DwgBlockRecordXrefWireTestAccess::checkAppIdCountBoundaries<dwgWriter21>()) {
        std::cerr << "AC1021 APPID_CONTROL BL writer boundary failure\n";
        return 1;
    }
    std::cout<<"multi-version LTYPE_CONTROL and APPID_CONTROL count/retry PASS\n";return 0;
}
