// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFFIELD_H
#define DXFFIELD_H

#include "drw_objects.h"
#include "dxfreader.h"
#include "drw_reserve.h"

#include <limits>
#include <string_view>

// Shared production FIELD grammar. Common application/XDATA capture stays
// with the caller; neither transport, expression evaluation nor DWG layout is
// inferred here. Reference events describe the just-consumed DXF scalar only.
namespace dxfField {
enum class ReferenceKind {None, ChildField, Object, CadValueObject};

inline bool appendHexChunk(std::string_view text,std::vector<std::uint8_t>& target) {
    if(target.size()>DRW::kMaxDxfBinaryPayloadBytes || (text.size()&1u)!=0u
        || text.size()/2u>DRW::kMaxDxfBinaryPayloadBytes-target.size())return false;
    const auto digit=[](char ch)->int {
        if(ch>='0'&&ch<='9')return ch-'0';
        if(ch>='a'&&ch<='f')return ch-'a'+10;
        if(ch>='A'&&ch<='F')return ch-'A'+10;
        return -1;
    };
    for(std::size_t i=0;i<text.size();i+=2u)if(digit(text[i])<0||digit(text[i+1u])<0)return false;
    const auto size=target.size()+text.size()/2u;
    if(size>static_cast<std::size_t>(std::numeric_limits<int>::max())
        || !DRW::reserve(target,static_cast<int>(size)))return false;
    for(std::size_t i=0;i<text.size();i+=2u)
        target.push_back(static_cast<std::uint8_t>((digit(text[i])<<4)|digit(text[i+1u])));
    return true;
}

class Decoder {
    DRW_Field& field;
    DRW::Version version;
    bool inSubclass=false,childOpen=false,primaryOpen=false,primarySeen=false;
    bool activeHasFlags=false,activeHasType=false,activeHasDataSize=false;
    bool activeHasX=false,activeHasY=false,activeHasZ=false;
    DRW_CadValue* activeValue=nullptr;
    int childHandleCount=-1,objectHandleCount=-1,childValueCount=-1;
    unsigned evaluatorFields=0u,codeFields=0u;
    DRW_Field::ChildValue child;

    static void appendText(UTF8STRING& destination,UTF8STRING decoded) {
        if(destination.empty())destination=std::move(decoded);else destination+=decoded;
    }
    void resetActive() {
        activeHasFlags=activeHasType=activeHasDataSize=false;
        activeHasX=activeHasY=activeHasZ=false;
    }
    bool valueHasRequiredPayload() const {
        if(!activeValue || !activeHasType || !activeValue->hasDwgPayload(version))
            return activeValue && activeHasType;
        switch(activeValue->m_dataType) {
        case 0:case 1:case 64:return activeValue->m_value.type()==DRW_Variant::INTEGER;
        case 2:return activeValue->m_value.type()==DRW_Variant::DOUBLE;
        case 4:return activeValue->m_value.type()==DRW_Variant::STRING;
        case 8:return activeHasDataSize && activeValue->m_dataSize==activeValue->m_rawData.size();
        case 16:return activeValue->m_value.type()==DRW_Variant::COORD && activeHasX && activeHasY
            && (activeValue->m_dataSize==0u || activeValue->m_dataSize==16u);
        case 32:return activeValue->m_value.type()==DRW_Variant::COORD && activeHasX && activeHasY && activeHasZ
            && (activeValue->m_dataSize==0u || activeValue->m_dataSize==24u);
        default:return false;
        }
    }
    bool closeValue() {
        if(!valueHasRequiredPayload())return false;
        if(activeValue->m_dataType==8)activeValue->m_value.addBinary(310,activeValue->m_rawData);
        if(primaryOpen)primaryOpen=false;
        else {
            if(!childOpen || field.m_childValues.size()>=DRW_Field::kMaxItems)return false;
            field.m_childValues.push_back(child);
            childOpen=false;
        }
        activeValue=nullptr;resetActive();return true;
    }
public:
    Decoder(DRW_Field& target,DRW::Version revision):field(target),version(revision){}
    Decoder(const Decoder&)=delete;
    Decoder& operator=(const Decoder&)=delete;

    bool beforeRecord(int code) {
        return !activeValue || activeHasFlags
            || (code!=0 && code!=6 && code!=7 && code!=9 && code!=98 && code!=301)
            || closeValue();
    }
    bool finish() {
        if(!beforeRecord(0) || !inSubclass || activeValue || !field.handle
            || childHandleCount<0 || objectHandleCount<0 || childValueCount<0
            || static_cast<std::size_t>(childHandleCount)!=field.m_childHandles.size()
            || static_cast<std::size_t>(objectHandleCount)!=field.m_objectHandles.size()
            || static_cast<std::size_t>(childValueCount)!=field.m_childValues.size())return false;
        field.setDwgCommonObjectState(static_cast<std::int32_t>(field.reactorHandles.size()),field.xDictHandle?1:0,false);
        return true;
    }
    // A source-proof profile, not an additional tolerant import requirement.
    bool sourceCorePresent() const {return evaluatorFields==1u && codeFields==1u;}

    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader,ReferenceKind& reference) {
        reference=ReferenceKind::None;
        if(code==100){if(reader->getStringView()=="AcDbField")inSubclass=true;return true;}
        if(!inSubclass) {
            if(code==5)field.handle=reader->getHandleString();
            else if(code==330)field.parentHandle=reader->getHandleString();
            return true;
        }
        if(activeValue) {
            switch(code) {
            case 1:case 2:
                if(activeValue->m_dataType!=4)return false;
                activeValue->m_value.appendString(1,reader->getUtf8String());break;
            case 11:
                if(activeValue->m_dataType!=16 && activeValue->m_dataType!=32)return false;
                activeValue->m_value.addCoord(11,DRW_Coord(reader->getDouble(),0.0,0.0));activeHasX=true;break;
            case 21:
                if((activeValue->m_dataType!=16 && activeValue->m_dataType!=32) || !activeHasX || !activeValue->m_value.coord())return false;
                activeValue->m_value.setCoordY(reader->getDouble());activeHasY=true;break;
            case 31:
                if(activeValue->m_dataType!=32 || !activeHasX || !activeHasY || !activeValue->m_value.coord())return false;
                activeValue->m_value.setCoordZ(reader->getDouble());activeHasZ=true;break;
            case 90:
                if(activeHasType)return false;
                activeValue->m_dataType=reader->getInt32();activeHasType=true;break;
            case 91:
                if(activeValue->m_dataType!=0 && activeValue->m_dataType!=1)return false;
                activeValue->m_value.addInt(91,reader->getInt32());break;
            case 92:
                if((activeValue->m_dataType!=8 && activeValue->m_dataType!=16 && activeValue->m_dataType!=32) || activeHasDataSize)return false;
                activeValue->m_dataSize=static_cast<std::uint32_t>(reader->getInt32());
                if(activeValue->m_dataSize>DRW::kMaxDxfBinaryPayloadBytes)return false;
                activeHasDataSize=true;break;
            case 93:
                if(activeHasType)return false;
                activeValue->m_formatFlags=reader->getInt32();activeHasFlags=true;break;
            case 94:activeValue->m_unitType=reader->getInt32();break;
            case 140:
                if(activeValue->m_dataType!=2)return false;
                activeValue->m_value.addDouble(140,reader->getDouble());break;
            case 300:activeValue->m_formatString=reader->getUtf8String();break;
            case 302:activeValue->m_valueString=reader->getUtf8String();break;
            case 304:
                if(reader->getStringView()!="ACVALUE_END" || !closeValue())return false;
                break;
            case 310:
                if(activeValue->m_dataType!=8 || !appendHexChunk(reader->getStringView(),activeValue->m_rawData))return false;
                break;
            case 330:
                if(activeValue->m_dataType!=64)return false;
                activeValue->m_handle=reader->getHandleString();activeValue->m_value.addInt(330,activeValue->m_handle);
                reference=ReferenceKind::CadValueObject;break;
            default:return false;
            }
            return true;
        }
        switch(code) {
        case 1:field.m_evaluatorId=reader->getUtf8String();++evaluatorFields;break;
        case 2:++codeFields;appendText(field.m_fieldCode,reader->getUtf8String());break;
        case 3:appendText(field.m_fieldCode,reader->getUtf8String());break;
        case 4:field.m_formatString=reader->getUtf8String();break;
        case 6:
            if(primarySeen || childValueCount<0 || field.m_childValues.size()>=static_cast<std::size_t>(childValueCount))return false;
            child=DRW_Field::ChildValue();child.m_key=reader->getUtf8String();childOpen=true;
            activeValue=&child.m_value;resetActive();break;
        case 7:
            if(primarySeen || childValueCount<0 || field.m_childValues.size()!=static_cast<std::size_t>(childValueCount)
                || reader->getStringView()!="ACFD_FIELD_VALUE")return false;
            field.m_value=DRW_CadValue();activeValue=&field.m_value;primaryOpen=true;primarySeen=true;resetActive();break;
        case 90:
            if(childHandleCount>=0)return false;
            childHandleCount=reader->getInt32();
            if(childHandleCount<0 || childHandleCount>static_cast<int>(DRW_Field::kMaxItems))return false;
            break;
        case 93:
            if(childValueCount>=0)return false;
            childValueCount=reader->getInt32();
            if(childValueCount<0 || childValueCount>static_cast<int>(DRW_Field::kMaxItems))return false;
            break;
        case 97:
            if(objectHandleCount>=0)return false;
            objectHandleCount=reader->getInt32();
            if(objectHandleCount<0 || objectHandleCount>static_cast<int>(DRW_Field::kMaxItems))return false;
            break;
        case 91:field.m_evaluationOptionFlags=reader->getInt32();break;
        case 92:field.m_filingOptionFlags=reader->getInt32();break;
        case 94:field.m_fieldStateFlags=reader->getInt32();break;
        case 95:field.m_evaluationStatusFlags=reader->getInt32();break;
        case 96:field.m_evaluationErrorCode=reader->getInt32();break;
        case 300:field.m_evaluationErrorMessage=reader->getUtf8String();break;
        case 9:case 301:appendText(field.m_valueString,reader->getUtf8String());break;
        case 98:field.m_valueStringLength=reader->getInt32();break;
        case 360: {
            const auto handle=reader->getHandleString();
            if(!handle || field.m_childHandles.size()>=DRW_Field::kMaxItems)return false;
            field.m_childHandles.push_back(handle);reference=ReferenceKind::ChildField;break;
        }
        case 331: {
            const auto handle=reader->getHandleString();
            if(!handle || field.m_objectHandles.size()>=DRW_Field::kMaxItems)return false;
            field.m_objectHandles.push_back(handle);reference=ReferenceKind::Object;break;
        }
        default:break;
        }
        return true;
    }
};
} // namespace dxfField
#endif
