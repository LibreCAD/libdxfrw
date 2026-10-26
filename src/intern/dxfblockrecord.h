// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCKRECORD_H
#define DXFBLOCKRECORD_H

#include "drw_objects.h"
#include "dxfreader.h"
#include "dxffield.h" // shared bounded hex-chunk primitive, not FIELD grammar

#include <limits>
#include <unordered_set>

// Production BLOCK_RECORD body decoder. Batch publication and context-map
// ownership remain with dxfRW. Replay uses the same body grammar and quota
// owners, never a second interpretation of BLKREFS or preview bytes.
namespace dxfBlockRecord {
class Decoder {
    DRW_Block_Record& record;
    std::shared_ptr<DxfReadRetainedBudget> budget;
    DxfReadRetainedReservation indexReservation;
    std::unordered_set<std::uint32_t> insertHandles;
    std::size_t indexBytes=0u,insertBytes=0u,previewBytes=0u;
    bool blockReferences=false,extensionDictionary=false,dictionarySeen=false;
    int ignoredDepth=0;
    dxfReader* opaqueReader=nullptr;
    bool previousWideHandles=false;

    void restoreOpaqueHandles() noexcept {
        if(opaqueReader)opaqueReader->setAllowWideHandleLexemes(previousWideHandles);
        opaqueReader=nullptr;
    }

    bool context(const std::unique_ptr<dxfReader>& reader) const {
        if(!reader||(opaqueReader&&opaqueReader!=reader.get()))return false;
        if(reader->retainedByteBudget()!=budget) {
            if(reader->retainedByteBudget())reader->retainedByteBudget()->reject();
            return false;
        }
        return !budget||!budget->exceeded();
    }
public:
    Decoder(DRW_Block_Record& target,const std::unique_ptr<dxfReader>& reader)
        :record(target),budget(reader?reader->retainedByteBudget():nullptr),indexReservation(budget,0u){}
    Decoder(const Decoder&)=delete;
    Decoder& operator=(const Decoder&)=delete;
    ~Decoder(){restoreOpaqueHandles();}

    bool inApplication() const noexcept {return blockReferences||extensionDictionary||ignoredDepth!=0;}
    static std::pair<bool,std::uint8_t> observeCommonState(const DRW_TableEntry& value) noexcept {
        return {value.curr!=nullptr,value.currCoordMask};
    }
    static bool hasNoNativeLinks(const DRW_Block_Record& value) noexcept {
        return !value.block&&!value.endBlock&&value.firstEH==DRW::NoHandle
            &&value.lastEH==DRW::NoHandle&&value.entMap.empty();
    }
    bool finish(DRW::Version version) const noexcept {
        return !inApplication()&&!record.name.empty()
            &&(version==DRW::UNKNOWNV||version<=DRW::AC1009||record.handle!=DRW::NoHandle);
    }
    bool reset() {
        restoreOpaqueHandles();
        if(!indexReservation.resize(0u))return false;
        indexBytes=insertBytes=previewBytes=0u;
        insertHandles.clear();blockReferences=extensionDictionary=dictionarySeen=false;ignoredDepth=0;
        return true;
    }
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        if(!context(reader))return false;
        // A group's payload must not overwrite the enclosing row's identity,
        // name or scalars. Only its own documented reference role is decoded.
        if(code!=102) {
            if(ignoredDepth)return true;
            if(blockReferences&&code!=331)return true;
            if(extensionDictionary&&code!=360)return true;
        }
        switch(code) {
        case 102: {
            const auto control=reader->getStringView();
            if(blockReferences) {
                if(control!="}")return false;
                blockReferences=false;
            } else if(extensionDictionary) {
                if(control!="}"||!dictionarySeen)return false;
                extensionDictionary=false;
            } else if(ignoredDepth) {
                if(control=="}") {
                    if(!--ignoredDepth)restoreOpaqueHandles();
                }
                else if(!control.empty()&&control.front()=='{') {
                    if(ignoredDepth==DRW::kMaxDxfApplicationGroupNesting)return false;
                    ++ignoredDepth;
                } else return false;
            } else if(control=="{BLKREFS") {
                if(reader->getSourceVersion()!=DRW::UNKNOWNV&&reader->getSourceVersion()<=DRW::AC1014)return false;
                blockReferences=true;
            } else if(control=="{ACAD_XDICTIONARY") {
                extensionDictionary=true;dictionarySeen=false;
            } else if(control=="{ACAD_REACTORS") {
                // Retain the historical ignored reactor payload and its
                // native-width admission, not an opaque wide-handle scope.
                ignoredDepth=1;
            } else if(!control.empty()&&control.front()=='{') {
                // Only opaque application payloads may carry raw-width handles.
                // Restore before reading the next native field, on reset, and
                // on failure/unwind; known groups retain typed-width checks.
                opaqueReader=reader.get();previousWideHandles=reader->allowsWideHandleLexemes();
                reader->setAllowWideHandleLexemes(true);ignoredDepth=1;
            }
            else if(control=="}")return false;
            break;
        }
        case 331: {
            if(!blockReferences||!reader->isValidHandleString()
                ||insertHandles.size()>=DRW::kMaxDxfBlockRecordInsertHandles)return false;
            const auto handle=reader->getHandleString();
            if(handle==DRW::NoHandle)break;
            if(insertHandles.count(handle))return false;
            if(indexBytes>(std::numeric_limits<std::size_t>::max)()-sizeof(handle)) {
                if(budget)budget->reject();
                return false;
            }
            if(!indexReservation.resize(indexBytes+sizeof(handle)))return false;
            indexBytes+=sizeof(handle);
            if(!record.retainDxfRetainedStringBytes(reader,insertBytes,sizeof(handle)))return false;
            insertHandles.insert(handle);record.insertHandles.push_back(handle);break;
        }
        case 360:
            if(!extensionDictionary||dictionarySeen||!reader->isValidHandleString())return false;
            record.xDictHandle=reader->getHandleString();
            if(record.xDictHandle==DRW::NoHandle)return false;
            dictionarySeen=true;break;
        case 340:
            if(!reader->isValidHandleString())return false;
            record.layoutHandle=reader->getHandleString();break;
        case 330:
            if(!reader->isValidHandleString())return false;
            record.parentHandle=reader->getHandleString();break;
        case 2:return record.parseCode(code,reader);
        case 5:
            if(!reader->isValidHandleString())return false;
            record.handle=reader->getHandleString();break;
        case 70:record.insUnits=reader->getInt32();record.flags=record.insUnits;break;
        case 280: {
            const auto value=reader->getInt32();if(value<0||value>1)return false;
            record.canExplode=value!=0;break;
        }
        case 281: {
            const auto value=reader->getInt32();if(value<0||value>1)return false;
            record.blockScaling=static_cast<std::uint8_t>(value);break;
        }
        case 310: {
            const auto chunk=reader->getStringView();if(chunk.size()&1u)return false;
            if(!record.retainDxfRetainedStringBytes(reader,previewBytes,chunk.size()/2u)
                ||!dxfField::appendHexChunk(chunk,record.previewData))return false;
            break;
        }
        case 1000:case 1001:case 1002:case 1003:case 1004:case 1005:
        case 1010:case 1011:case 1012:case 1013:case 1020:case 1021:case 1022:case 1023:
        case 1030:case 1031:case 1032:case 1033:case 1040:case 1041:case 1042:case 1070:case 1071:
            return record.parseCode(code,reader);
        default:break;
        }
        return true;
    }
};
} // namespace dxfBlockRecord
#endif
