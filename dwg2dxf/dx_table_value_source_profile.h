// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_TABLE_VALUE_SOURCE_PROFILE_H
#define DX_TABLE_VALUE_SOURCE_PROFILE_H
#include "dx_attdef_source_profile.h"

// Independent structured CELL_VALUE physical grammar. Pinned ACadSharp
// 3a52a520 source mappings and hashed original ASCII inventories corroborate
// these fields; they do not establish producer identity, date semantics,
// actual reference targets, binary producer profiles or output qualification.
// No production decoder/model is consulted and no payload is copied/allocated.
namespace dx_table_source {
struct ValueProfile {
    unsigned phase{0},payload{0};
    int flags{0},type{0};
    std::size_t size{0},bytes{0};
    bool block{false};
    bool closed() const noexcept {return phase==6u;}
    bool payloadComplete() const noexcept {
        if(flags&1)return payload==0u;
        switch(type) {
        case 1:case 2:case 4:case 64:return payload==1u;
        case 8:return payload==2u&&bytes==size;
        case 16:return payload==2u;
        case 32:return payload==3u;
        default:return false;
        }
    }
    bool take(const DRW_Variant& v,dx_attdef_source::Role& role,std::uint64_t& handle) noexcept {
        using namespace dx_attdef_source;
        const int code=v.code();const auto s=text(v);
        if(phase==0u) {
            if(code!=93||v.type()!=DRW_Variant::INTEGER||v.i_val()<0||v.i_val()>7)return false;
            flags=v.i_val();phase=1u;return true;
        }
        if(phase==1u) {
          if (code != 90 || v.type() != DRW_Variant::INTEGER)
            return false;
          type = v.i_val();
          switch (type) {
          case 0:
            if (!(flags & 1))
              return false;
            break;
          case 1:
          case 2:
          case 4:
          case 8:
          case 16:
          case 32:
          case 64:
            break;
          default:
            return false;
          }
            // The inventoried block-associated Value is specifically an empty
            // string. Other block Value kinds remain unqualified.
            if (block && (type != 4 || flags != 1))
              return false;
            phase = 2u;
            return true;
        }
        if(phase==3u){if(code!=300||(block&&!s.empty()))return false;phase=4u;return true;}
        if(phase==4u){if(code!=302||(block&&!s.empty()))return false;phase=5u;return true;}
        if(phase==5u){if(code!=304||s!="ACVALUE_END")return false;phase=6u;return true;}
        if(phase!=2u)return false;
        if(code==94) {
            if(!payloadComplete()||v.type()!=DRW_Variant::INTEGER)return false;
            const auto unit=v.i_val();
            if(unit!=0&&unit!=1&&unit!=2&&unit!=4&&unit!=8&&unit!=16&&unit!=32)return false;
            if (block && unit)
              return false;
            phase = 3u;
            return true;
        }
        if(flags&1)return false;
        switch(type) {
        case 1:if(code!=91||payload)return false;payload=1u;return true;
        case 2:if(code!=140||payload)return false;payload=1u;return true;
        case 4:
            // Prefix chunks2, then exactly one final1. Tolerant append-only
            // replay also accepts1 followed by2; that is not this profile.
            if((code!=1&&code!=2)||payload)return false;
            if (code == 1)
              payload = 1u;
            return true;
        case 8:
            if(code==92) {
                if(payload||v.type()!=DRW_Variant::INTEGER||(v.i_val()!=8&&v.i_val()!=16))return false;
                size=static_cast<std::size_t>(v.i_val());payload=1u;return true;
            }
            if(code!=310||!payload||s.empty()||s.size()>254u||!hexBytes(s)||s.size()/2u>size-bytes)return false;
            bytes+=s.size()/2u;payload=2u;return true;
        case 16:case 32:
            // Observed DXF points have no native DWG byte-count tag. Keep
            // explicit counts unqualified instead of treating absence as zero.
            if(payload>=(type==16?2u:3u)||code!=11+10*static_cast<int>(payload))return false;
            ++payload;return true;
        case 64:
            if(code!=330||payload||!hexHandle(v,handle)||handle>UINT32_MAX)return false;
            role=Role::TableValueObject;payload=1u;return true;
        default:return false;
        }
    }
};
}
#endif
