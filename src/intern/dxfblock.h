// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFBLOCK_H
#define DXFBLOCK_H
#include "drw_entities.h"
#include "dxfreader.h"

// Shared production/replay admission, not a strict source-profile certificate.
// Preserve tolerant BLOCK behavior and the existing stricter ENDBLK policy.
// Applications consume their own records; none can supply an outer field.
namespace dxfBlock {
class Decoder {
    DRW_Block& value;
    bool end;
    bool name=false,handle=false,owner=false,layer=false,entityClass=false,endClass=false;
public:
    Decoder(DRW_Block& block,bool footer):value(block),end(footer) {value.setIsEnd(footer);}
    bool hasDeclaredName() const noexcept {return name;}
    void prepareCapturedApplication() {value.clearParserCursor();}
    static bool xdataCode(int code) noexcept {
        return (code>=1000&&code<=1005)||(code>=1010&&code<=1013)
            ||(code>=1020&&code<=1023)||(code>=1030&&code<=1033)
            ||(code>=1040&&code<=1042)||code==1070||code==1071;
    }
    static bool endCode(int code) noexcept {
        return code==5||code==330||code==8||code==100||code==102||xdataCode(code)
            ||code==6||code==48||code==60||code==62||code==67||code==284||code==347
            ||code==348||code==360||code==370||code==390||code==420||code==430||code==440;
    }
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        if(!end) {
            name=name||code==2;handle=handle||code==5;
            return value.parseCode(code,reader);
        }
        if(!endCode(code))return false;
        if(code==5) {
            if(handle||!reader->isValidHandleString())return false;
            const auto id=reader->getHandleString();
            if(id==DRW::NoHandle||!reader->registerSelfHandle())return false;
            value.handle=id;handle=true;return true;
        }
        if(code==330) {
            if(owner||!reader->isValidHandleString())return false;
            const auto id=reader->getHandleString();
            if(id==DRW::NoHandle)return false;
            value.parentHandle=id;owner=true;return true;
        }
        if(code==8) {if(layer)return false;layer=true;}
        if(code==100) {
            const auto marker=reader->getStringView();
            if(marker=="AcDbEntity") {if(entityClass)return false;entityClass=true;}
            else if(marker=="AcDbBlockEnd") {if(endClass)return false;endClass=true;}
            else return false;
            return true;
        }
        return value.parseCode(code,reader);
    }
    bool finish(DRW::Version version,bool declared,std::uint32_t beginOwner=DRW::NoHandle) const noexcept {
        const bool needsHandle=version!=DRW::UNKNOWNV&&version>DRW::AC1009;
        if(needsHandle&&(!handle||value.handle==DRW::NoHandle))return false;
        // Older producers may use a distinct space owner. Preserve it here;
        // selected source/graph qualification must check actual owner roles.
        return !end||!declared||version<=DRW::AC1014||beginOwner==DRW::NoHandle||owner;
    }
};
}
#endif
