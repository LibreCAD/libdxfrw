// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFFIELD_H
#define DXFFIELD_H

#include "drw_objects.h"
#include "dxfreader.h"
#include "drw_reserve.h"
#include "dxfvalue.h"

#include <algorithm>
#include <array>
#include <limits>
#include <string_view>

// Shared production FIELD grammar. Common application/XDATA capture stays
// with the caller; neither transport, expression evaluation nor DWG layout is
// inferred here. Reference events describe the just-consumed DXF scalar only.
namespace dxfField {
enum class ReferenceKind {None, ChildField, Object, CadValueObject};

using dxfValue::appendHexChunk;

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
    std::shared_ptr<DxfReadRetainedBudget> budget;
    bool budgetContextSet=false;
    std::array<std::size_t,5> scalarBytes{};
    struct ValueBytes {
        std::size_t text=0u,format=0u,cache=0u,raw=0u,binary=0u;
        std::size_t total() const {return text+format+cache+raw+binary;}
    } activeBytes;
    std::size_t childKeyBytes=0u;
    std::shared_ptr<DxfReadRetainedLease> pendingLease;

    bool context(const std::unique_ptr<dxfReader>& reader) {
        if(!reader)return false;
        const auto& incoming=reader->retainedByteBudget();
        if(!budgetContextSet){budget=incoming;budgetContextSet=true;}
        if(incoming!=budget || (field.m_dxfRetainedStringLease&&field.m_dxfRetainedStringLease->budget()!=budget)
            || (childOpen&&pendingLease&&pendingLease!=field.m_dxfRetainedStringLease)) {
            if(incoming)incoming->reject();
            return false;
        }
        return !budget||!budget->exceeded();
    }
    bool admit(std::size_t bytes) {
        return (!budget||!budget->exceeded())&&retainDxfReadBytes(field.m_dxfRetainedStringLease,budget,bytes);
    }
    void retainPending() {
        if(childOpen)pendingLease=field.m_dxfRetainedStringLease;
    }
    bool replaceText(const std::unique_ptr<dxfReader>& reader,std::size_t& owner,
                     UTF8STRING& destination) {
        if(!field.replaceDxfRetainedStringField(reader,owner,destination,reader->getUtf8String()))return false;
        retainPending();return true;
    }
    bool appendText(UTF8STRING& destination,UTF8STRING decoded,std::size_t& owner) {
        const auto bytes=decoded.size();
        DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
        if(!admit(bytes))return false;
        if(destination.empty())destination=std::move(decoded);else destination+=decoded;
        if(budget)owner+=bytes;
        checkpoint.commit();retainPending();return true;
    }
    bool appendValueText(UTF8STRING decoded) {
        const auto bytes=decoded.size();
        DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
        if(!admit(bytes))return false;
        activeValue->m_value.appendString(1,std::move(decoded));
        if(budget)activeBytes.text+=bytes;
        checkpoint.commit();retainPending();return true;
    }
    bool appendBinary(std::string_view text) {
        if((text.size()&1u)!=0u)return false;
        DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
        if(!admit(text.size()/2u)||!appendHexChunk(text,activeValue->m_rawData))return false;
        activeBytes.raw=budget?activeValue->m_rawData.size():0u;
        checkpoint.commit();retainPending();return true;
    }
    bool closeBinary() {
        const auto bytes=activeValue->m_rawData.size();
        DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
        if(!admit(bytes))return false;
        activeValue->m_value.addBinary(310,activeValue->m_rawData);
        if(activeBytes.binary&&!field.releaseDxfRetainedStringBytes(activeBytes.binary))return false;
        activeBytes.binary=budget?bytes:0u;
        checkpoint.commit();retainPending();return true;
    }
    template<class T> bool appendItem(std::vector<T>& destination,T value) {
        DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
        if(!admit(sizeof(T)))return false;
        if(destination.size()==destination.capacity()
            && !DRW::reserve(destination,static_cast<int>(std::min(DRW_Field::kMaxItems,
                std::max(std::size_t{1u},destination.size()*2u)))))return false;
        destination.push_back(std::move(value));checkpoint.commit();return true;
    }
    void resetActive() {
        activeHasFlags=activeHasType=activeHasDataSize=false;
        activeHasX=activeHasY=activeHasZ=false;
        activeBytes={};
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
        if(activeValue->m_dataType==8&&!closeBinary())return false;
        if(primaryOpen)primaryOpen=false;
        else {
            if(!childOpen || field.m_childValues.size()>=DRW_Field::kMaxItems)return false;
            DxfReadRetainedLeaseCheckpoint checkpoint(field.m_dxfRetainedStringLease);
            if(!admit(sizeof(DRW_Field::ChildValue)))return false;
            if(field.m_childValues.size()==field.m_childValues.capacity()
                && !DRW::reserve(field.m_childValues,static_cast<int>(std::min(DRW_Field::kMaxItems,
                    std::max(std::size_t{1u},field.m_childValues.size()*2u)))))return false;
            field.m_childValues.push_back(std::move(child));
            checkpoint.commit();
            childOpen=false;
            childKeyBytes=0u;pendingLease.reset();
        }
        activeValue=nullptr;resetActive();return true;
    }
public:
    Decoder(DRW_Field& target,DRW::Version revision):field(target),version(revision){}
    ~Decoder() noexcept {
        // Unpublished child storage is owned lexically by this decoder. Closed
        // children moved to the model keep their charge on its existing lease.
        const auto bytes=childOpen?childKeyBytes+activeBytes.total():0u;
        if(bytes&&pendingLease) {
            if(bytes<=pendingLease->bytes())(void)pendingLease->resizeTo(pendingLease->bytes()-bytes);
            else if(budget)budget->reject();
        }
    }
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
        if(!context(reader))return false;
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
                if(!appendValueText(reader->getUtf8String()))return false;
                break;
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
            case 300:if(!replaceText(reader,activeBytes.format,activeValue->m_formatString))return false;break;
            case 302:if(!replaceText(reader,activeBytes.cache,activeValue->m_valueString))return false;break;
            case 304:
                if(reader->getStringView()!="ACVALUE_END" || !closeValue())return false;
                break;
            case 310:
                if(activeValue->m_dataType!=8 || !appendBinary(reader->getStringView()))return false;
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
        case 1:if(!replaceText(reader,scalarBytes[0],field.m_evaluatorId))return false;++evaluatorFields;break;
        case 2:if(!appendText(field.m_fieldCode,reader->getUtf8String(),scalarBytes[1]))return false;++codeFields;break;
        case 3:if(!appendText(field.m_fieldCode,reader->getUtf8String(),scalarBytes[1]))return false;break;
        case 4:if(!replaceText(reader,scalarBytes[2],field.m_formatString))return false;break;
        case 6:
            if(primarySeen || childValueCount<0 || field.m_childValues.size()>=static_cast<std::size_t>(childValueCount))return false;
            child=DRW_Field::ChildValue();childOpen=true;childKeyBytes=0u;
            activeValue=&child.m_value;resetActive();
            if(!replaceText(reader,childKeyBytes,child.m_key))return false;
            break;
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
        case 300:if(!replaceText(reader,scalarBytes[3],field.m_evaluationErrorMessage))return false;break;
        case 9:case 301:if(!appendText(field.m_valueString,reader->getUtf8String(),scalarBytes[4]))return false;break;
        case 98:field.m_valueStringLength=reader->getInt32();break;
        case 360: {
            const auto handle=reader->getHandleString();
            if(!handle || field.m_childHandles.size()>=DRW_Field::kMaxItems)return false;
            if(!appendItem(field.m_childHandles,handle))return false;
            reference=ReferenceKind::ChildField;break;
        }
        case 331: {
            const auto handle=reader->getHandleString();
            if(!handle || field.m_objectHandles.size()>=DRW_Field::kMaxItems)return false;
            if(!appendItem(field.m_objectHandles,handle))return false;
            reference=ReferenceKind::Object;break;
        }
        default:break;
        }
        return true;
    }
};
} // namespace dxfField
#endif
