// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFATTDEF_H
#define DXFATTDEF_H

#include "drw_entities.h"
#include "dxfreader.h"

// Production ATTDEF body admission/finalization, shared with bounded replay.
// Entity/subclass parsing remains in DRW_Attdef, including the existing 101
// and legacy-100 embedded MTEXT routes. No independent native layout claim.
namespace dxfAttdef {
class Decoder {
    DRW_Attdef& value;
    bool hasText=false,hasTag=false,hasX=false,hasY=false,hasHeight=false;
public:
    explicit Decoder(DRW_Attdef& target):value(target){}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader) {
        hasText=hasText||code==1;
        hasTag=hasTag||code==2;
        hasX=hasX||code==10;
        hasY=hasY||code==20;
        hasHeight=hasHeight||code==40;
        return value.parseCode(code,reader);
    }
    // The existing host parser delegates application groups to nested MTEXT
    // once present, even after the attribute-definition subclass. Capturing
    // source must preserve that routing, not attach them all to the host.
    DRW_Entity& applicationEntity() const noexcept {
        return value.mtext ? static_cast<DRW_Entity&>(*value.mtext)
                           : static_cast<DRW_Entity&>(value);
    }
    bool finish(bool applyExtrusion) {
        if(!hasText||!hasTag||value.tag.empty()||!hasX||!hasY||!hasHeight
            ||(value.mtext&&!value.mtext->finalizeDxfColumnObject()))return false;
        if(value.mtext)value.mtext->updateAngle();
        if(applyExtrusion)value.applyExtrusion();
        return true;
    }
};
} // namespace dxfAttdef
#endif
