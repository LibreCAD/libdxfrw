#ifndef TABLE_CHILD_CANDIDATE_FIXTURE_H
#define TABLE_CHILD_CANDIDATE_FIXTURE_H
#include "intern/dwg_table_child_xrecord_ac1018_candidate.h"
#include "intern/dwgbufferw.h"
#include <string>
namespace ChildFixture {
struct Options {std::uint32_t handle{0xbeu},owner{0xbdu};std::uint8_t role{4u},page{45u};bool empty{false};std::string value{"Title"};};
struct Data {DRW_UnsupportedObject raw;DRW_XRecord typed;};
inline void handle(dwgBufferW& b,std::uint8_t role,std::uint32_t value){dwgHandle h;h.code=role;h.ref=value;b.putHandle(h);}
inline void integer(dwgBufferW& b,unsigned code,int value){b.putRawShort16(code);b.putRawLong32(static_cast<std::uint32_t>(value));}
inline void text(dwgBufferW& b,unsigned code,const std::string& value,unsigned page) {
    b.putRawShort16(code);b.putRawShort16(static_cast<std::uint16_t>(value.size()));b.putRawChar8(page);
    for(auto c:value)b.putRawChar8(static_cast<std::uint8_t>(c));
}
inline Data build(const Options& o={}) {
    Data f;dwgBufferW payload;
    integer(payload,93,o.empty?7:4);integer(payload,90,o.empty?0:4);
    if(!o.empty)text(payload,1,o.value,o.page);
    integer(payload,94,0);text(payload,300,"",o.page);text(payload,302,o.empty?"":o.value,o.page);
    dwgBufferW b;b.putObjType(DRW::AC1018,79u);const auto end=b.bitCount();b.putRawLong32(0u);
    handle(b,0u,o.handle);b.putBitShort(0u);b.putBitLong(0);b.putBit(1u);b.putBitLong(static_cast<int>(payload.size()));
    b.putBytes(payload.data().data(),payload.size());b.putBitShort(1u);b.patchRawLong32AtBit(end,static_cast<std::uint32_t>(b.bitCount()));
    handle(b,o.role,o.owner);b.alignToByte();
    f.raw.m_version=DRW::AC1018;f.raw.m_objectType=79;f.raw.m_handle=o.handle;f.raw.m_rawBytes=b.data();f.raw.m_objectSize=static_cast<std::uint32_t>(f.raw.m_rawBytes.size());
    f.typed.handle=o.handle;f.typed.parentHandle=o.owner;f.typed.setDwgCommonObjectState(0,1u,false);f.typed.m_cloning=1;
    auto& e=f.typed.m_dataEntries;e.emplace_back(93,std::int32_t{o.empty?7:4});e.emplace_back(90,std::int32_t{o.empty?0:4});
    if(!o.empty)e.emplace_back(1,UTF8STRING(o.value));e.emplace_back(94,std::int32_t{0});e.emplace_back(300,UTF8STRING(""));e.emplace_back(302,UTF8STRING(o.empty?"":o.value));
    return f;
}
}
#endif
