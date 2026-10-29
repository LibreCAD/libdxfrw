#pragma once
// SPDX-License-Identifier: GPL-2.0-or-later
// Selected source-only dependency profile. A pass is not export, geometry,
// live-model, complete-format or native-semantic authorization.
// Unknown sections, records, fields, application scopes and payloads refuse.
// Exact ordering and spelling are deliberate admission restrictions.
#include "intern/dxfwholesource.h"
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string_view>
namespace dx_mline_source_dependency {
using WholeSourceV2::Source;using WholeSourceV2::Field;using K=DxfReadRecordView;
enum class Result { SourceCandidate, NoAuthority, Limit, Refused };
struct Limits {std::size_t fields=4096u,records=64u,retainedPeak=256u*1024u,work=256u*1024u*1024u;};
struct Proof {Result result=Result::Refused;std::size_t records=0u,fields=0u,work=0u;};
inline bool sum(std::size_t a,std::size_t b,std::size_t& out) {return WholeSourceV2::sum(a,b,out);}
inline bool product(std::size_t a,std::size_t b,std::size_t& out) {return WholeSourceV2::product(a,b,out);}
inline bool simpleName(std::string_view s) {
    if(s.empty()||s.size()>255u)return false;
    for(unsigned char c:s)if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c=='_'||c=='*'||c=='-'||c=='.'))return false;
    return true;
}
inline bool nameEq(std::string_view a,std::string_view b) {
    if(a.size()!=b.size())return false;for(std::size_t i=0;i<a.size();++i) {
        auto up=[](char c){return c>='a'&&c<='z'?static_cast<char>(c-'a'+'A'):c;};if(up(a[i])!=up(b[i]))return false;
    }return true;
}
inline bool handle(std::string_view s,std::uint32_t& value,bool allowZero=false) {
    if(s.empty()||s.size()>8u)return false;std::uint32_t x=0u;
    for(unsigned char c:s){unsigned n;if(c>='0'&&c<='9')n=c-'0';else if(c>='A'&&c<='F')n=c-'A'+10u;
        else if(c>='a'&&c<='f')n=c-'a'+10u;else return false;
        if(x>((std::numeric_limits<std::uint32_t>::max)()-n)/16u)return false;x=x*16u+n;
    }value=x;return x==0u?allowZero:x!=DRW::NoHandle;
}
struct Cursor {
    const Source& src;std::size_t pos=0u;bool ok=true;
    void skip(){while(pos<src.fieldCount()){auto f=src.field(pos);if(!f.ignored)break;
        if(f.code!=999||f.kind!=K::Text||!src.bounds(f)){ok=false;return;}++pos;}}
    Field peek(){skip();if(pos>=src.fieldCount()){ok=false;return Field{};}return src.field(pos);}
    bool marker(std::string_view expected) {auto f=peek();if(!ok||f.code!=0||f.kind!=K::Text||!src.bounds(f)||src.value(f)!=expected){ok=false;return false;}++pos;return true;}
    std::string_view text(int code) {auto f=peek();if(!ok||f.code!=code||f.kind!=K::Text||!src.bounds(f)){ok=false;return {};}
        ++pos;return src.value(f);}
    bool literal(int code,std::string_view value){return text(code)==value&&ok;}
    std::int32_t integer(int code){auto f=peek();if(!ok||f.code!=code||f.kind!=K::Integer){ok=false;return 0;}++pos;return f.integer;}
    double real(int code){auto f=peek();if(!ok||f.code!=code||f.kind!=K::Real||!std::isfinite(f.real)){ok=false;return 0.;}++pos;return f.real;}
    std::uint32_t h(int code,bool zero=false){std::uint32_t out=0u;auto s=text(code);if(!ok||!handle(s,out,zero))ok=false;return out;}
};
enum class Type {Table,Ltype,Layer,BlockRecord,Block,EndBlock,Mline,Line,Dict,Style};
struct Node {
    Type type=Type::Table;std::uint32_t self=0,owner=0,target=0;bool entities=false;std::string_view name,ref1,ref2;
    std::array<std::uint32_t,16> targets{};std::array<std::string_view,16> keys{};std::size_t entries=0;
};
inline Proof classify(const Source* source,std::uint32_t authenticatedRoot,std::uint32_t authenticatedStyleDictionary,Limits limits={}) noexcept {
    Proof p;if(!source||source->state()!=WholeSourceV2::State::ThroughEof||source->version()!=DRW::AC1027||(!authenticatedRoot||!authenticatedStyleDictionary)){p.result=Result::NoAuthority;return p;}
    try {
        // Logical precharge before walking fields. The profile has at most 64
        // nodes: a name field can participate in duplicate, provider and style
        // dictionary searches, each bounded by the node count. 512 passes over
        // the captured pool peak conservatively covers their aggregate
        // byte-sensitive comparisons; fixed 64*64*128 covers graph/pair work,
        // field*256 covers framing and scalar checks, and 4096 covers the
        // preceding bounded root lookup. This is not a CPU-time or RSS bound.
        // Node[64] and its 16-entry arrays use fixed stack storage, separately
        // from the retained-source cap; classification performs no heap alloc.
        p.fields=source->fieldCount();std::size_t a=0,b=0,cost=0;
        if(p.fields>limits.fields||limits.records>64u||source->peakBytes()>limits.retainedPeak
            ||!product(source->peakBytes(),512u,a)||!product(p.fields,256u,b)||!sum(a,b,cost)
            ||!sum(cost,64u*64u*128u,cost)||!sum(cost,4096u,cost)||cost>limits.work){p.result=Result::Limit;return p;}
        p.work=cost;std::size_t end=source->binary()?22u:0u;
        for(std::size_t i=0;i<p.fields;++i){const auto f=source->field(i);
            if(!source->bounds(f)||f.ordinal!=i+1u||f.begin!=end||f.end<=f.begin){p.result=Result::Refused;return p;}end=f.end;
        }
        Cursor c{*source};std::array<Node,64> nodes{};std::size_t used=0;
        auto add=[&](Type type)->Node*{if(used>=limits.records||used>=nodes.size()){p.result=Result::Limit;c.ok=false;return nullptr;}
            auto* n=&nodes[used++];n->type=type;return n;};
        auto markerName=[&]()->std::string_view{auto f=c.peek();if(!c.ok||f.code!=0||f.kind!=K::Text||!source->bounds(f)){c.ok=false;return {};}
            return source->value(f);};
        auto commonEntity=[&](Node& n,std::uint32_t owner)->bool{
            n.self=c.h(5);n.owner=c.h(330);if(owner&&n.owner!=owner)return false;
            return c.literal(100,"AcDbEntity");};
        auto readEntity=[&](std::uint32_t owner,bool entities)->bool {
            const auto type=markerName();
            if(type=="MLINE") {
                c.marker("MLINE");auto* n=add(Type::Mline);if(!n||!commonEntity(*n,owner))return false;n->entities=entities;
                const auto color=c.integer(62);const auto scale=c.real(48);n->name=c.text(8);
                if(color<0||color>256||scale<=0.||!simpleName(n->name)||!c.literal(100,"AcDbMline"))return false;
                n->ref1=c.text(2);if(!simpleName(n->ref1)||n->ref1.size()>32u)return false;n->target=c.h(340);const auto entityScale=c.real(40);const auto justify=c.integer(70);
                const auto flags=c.integer(71),vertices=c.integer(72),elements=c.integer(73);
                if(entityScale<=0.||justify<0||justify>2||flags!=1||vertices<2||vertices>32||elements!=2)return false;
                for(int code:{10,20,30,210,220,230})(void)c.real(code);
                for(int i=0;i<vertices;++i){for(int code:{11,21,31,12,22,32,13,23,33})(void)c.real(code);
                    for(int j=0;j<elements;++j){if(c.integer(74)!=2)return false;(void)c.real(41);(void)c.real(41);if(c.integer(75)!=0)return false;}}
                return c.ok;
            }
            if(type=="LINE") {
                c.marker("LINE");auto* n=add(Type::Line);if(!n||!commonEntity(*n,owner))return false;n->entities=entities;
                n->name=c.text(8);if(!simpleName(n->name)||!c.literal(100,"AcDbLine"))return false;
                for(int code:{10,20,30,11,21,31})(void)c.real(code);return c.ok;
            }
            return false;
        };
        if(!c.marker("SECTION")||!c.literal(2,"HEADER")||!c.literal(9,"$ACADVER")||!c.literal(1,"AC1027")
            ||!c.literal(9,"$DWGCODEPAGE")||!c.literal(3,"ANSI_1252")||!c.marker("ENDSEC")
            ||!c.marker("SECTION")||!c.literal(2,"TABLES"))return p;
        std::array<std::string_view,3> tables{};std::size_t tableCount=0u;
        while(c.ok&&markerName()=="TABLE") {
            c.marker("TABLE");const auto name=c.text(2);auto* table=add(Type::Table);if(!table||!simpleName(name))return p;
            table->name=name;table->self=c.h(5);if(!c.literal(100,"AcDbSymbolTable"))return p;
            const int maximum=c.integer(70);if(maximum<0||maximum>2048||tableCount>=tables.size())return p;
            for(std::size_t j=0;j<tableCount;++j)if(nameEq(tables[j],name))return p;tables[tableCount++]=name;
            std::size_t count=0;
            while(c.ok&&markerName()!="ENDTAB") {
                const auto kind=markerName();Type t;
                if(name=="LTYPE"&&kind=="LTYPE")t=Type::Ltype;
                else if(name=="LAYER"&&kind=="LAYER")t=Type::Layer;
                else if(name=="BLOCK_RECORD"&&kind=="BLOCK_RECORD")t=Type::BlockRecord;else return p;
                c.marker(kind);auto* n=add(t);if(!n)return p;n->self=c.h(5);n->owner=c.h(330);
                if(n->owner!=table->self||!c.literal(100,"AcDbSymbolTableRecord"))return p;
                if(t==Type::Ltype){if(!c.literal(100,"AcDbLinetypeTableRecord"))return p;n->name=c.text(2);
                    if(c.integer(70)!=0)return p;(void)c.text(3);if(c.integer(72)!=65||c.integer(73)!=0||c.real(40)!=0.)return p;}
                if(t==Type::Layer){if(!c.literal(100,"AcDbLayerTableRecord"))return p;n->name=c.text(2);
                    if(c.integer(70)!=0)return p;const auto color=c.integer(62);n->ref1=c.text(6);if(color<1||color>255||!simpleName(n->ref1))return p;}
                if(t==Type::BlockRecord){if(!c.literal(100,"AcDbBlockTableRecord"))return p;n->name=c.text(2);if(c.integer(70)!=0)return p;}
                if(!simpleName(n->name)||!c.ok||++count>static_cast<std::size_t>(maximum))return p;
            }
            if(!c.marker("ENDTAB"))return p;
        }
        if(tableCount!=3u||tables[0]!="LTYPE"||tables[1]!="LAYER"||tables[2]!="BLOCK_RECORD"
            ||!c.marker("ENDSEC")||!c.marker("SECTION")||!c.literal(2,"BLOCKS"))return p;
        while(c.ok&&markerName()=="BLOCK") {
            c.marker("BLOCK");auto* block=add(Type::Block);if(!block||!commonEntity(*block,0u))return p;
            block->ref1=c.text(8);if(!simpleName(block->ref1)||!c.literal(100,"AcDbBlockBegin"))return p;
            block->name=c.text(2);if(!simpleName(block->name)||c.integer(70)!=0)return p;
            for(int code:{10,20,30})(void)c.real(code);if(c.text(3)!=block->name||!c.text(1).empty())return p;
            while(c.ok&&markerName()!="ENDBLK")if(!readEntity(block->owner,false))return p;
            if(!c.marker("ENDBLK"))return p;auto* footer=add(Type::EndBlock);if(!footer||!commonEntity(*footer,block->owner))return p;
            footer->name=c.text(8);if(!simpleName(footer->name)||!c.literal(100,"AcDbBlockEnd"))return p;
        }
        if(!c.marker("ENDSEC")||!c.marker("SECTION")||!c.literal(2,"ENTITIES"))return p;
        while(c.ok&&markerName()!="ENDSEC")if(!readEntity(0u,true))return p;
        if(!c.marker("ENDSEC")||!c.marker("SECTION")||!c.literal(2,"OBJECTS"))return p;
        while(c.ok&&markerName()!="ENDSEC") {
            const auto type=markerName();
            if(type=="DICTIONARY") {
                c.marker("DICTIONARY");auto* n=add(Type::Dict);if(!n)return p;n->self=c.h(5);n->owner=c.h(330,true);
                if(!c.literal(100,"AcDbDictionary"))return p;const auto hard=c.integer(280),mode=c.integer(281);
                if(hard!=0||mode<0||mode>5)return p;
                while(c.ok&&c.peek().code==3){if(n->entries>=n->keys.size())return p;n->keys[n->entries]=c.text(3);
                    if(!simpleName(n->keys[n->entries]))return p;n->targets[n->entries]=c.h(350);++n->entries;}
            }else if(type=="MLINESTYLE") {
                c.marker("MLINESTYLE");auto* n=add(Type::Style);if(!n)return p;n->self=c.h(5);
                if(!c.literal(102,"{ACAD_REACTORS"))return p;const auto reactor=c.h(330);if(!c.literal(102,"}"))return p;
                n->owner=c.h(330);if(reactor!=n->owner||!c.literal(100,"AcDbMlineStyle"))return p;
                n->name=c.text(2);const auto flags=c.integer(70);auto description=c.text(3);const auto fill=c.integer(62);
                // Recognize only documented MLINESTYLE flag bits. This
                // source classification does not qualify their rendering or
                // any consumed-style geometry operation.
                constexpr std::uint32_t knownFlags=1u|2u|16u|32u|64u|256u|512u|1024u;
                (void)c.real(51);(void)c.real(52);
                if(!simpleName(n->name)||flags<0||(static_cast<std::uint32_t>(flags)&~knownFlags)!=0u
                    ||description.size()>255u||fill<0||fill>256||c.integer(71)!=2)return p;
                (void)c.real(49);const auto color1=c.integer(62);n->ref1=c.text(6);
                (void)c.real(49);const auto color2=c.integer(62);n->ref2=c.text(6);
                if(color1<0||color1>256||color2<0||color2>256||!simpleName(n->ref1)||!simpleName(n->ref2))return p;
            }else return p;
        }
        if(!c.marker("ENDSEC")||!c.marker("EOF"))return p;c.skip();if(!c.ok||c.pos!=source->fieldCount())return p;
        p.records=used;
        for(std::size_t i=0;i<used;++i)for(std::size_t j=i+1;j<used;++j) {
            if(nodes[i].self==nodes[j].self)return p;
            if(nodes[i].type==nodes[j].type&&(nodes[i].type==Type::Ltype||nodes[i].type==Type::Layer||nodes[i].type==Type::BlockRecord||nodes[i].type==Type::Style)
                &&nameEq(nodes[i].name,nodes[j].name))return p;
        }
        auto byHandle=[&](std::uint32_t h,Type type)->const Node*{for(std::size_t i=0;i<used;++i)if(nodes[i].self==h&&nodes[i].type==type)return &nodes[i];return nullptr;};
        auto byName=[&](std::string_view name,Type type)->const Node*{for(std::size_t i=0;i<used;++i)if(nodes[i].type==type&&nodes[i].name==name)return &nodes[i];return nullptr;};
        const auto* root=byHandle(authenticatedRoot,Type::Dict);if(!root||root->owner||root->entries!=1u||root->keys[0]!="ACAD_MLINESTYLE")return p;
        if(root->targets[0]!=authenticatedStyleDictionary)return p;
        const auto* styles=byHandle(root->targets[0],Type::Dict);if(!styles||styles->owner!=root->self||!styles->entries)return p;
        unsigned roots=0,dicts=0,styleCount=0,blockCount=0;std::uint32_t model=0;
        for(std::size_t i=0;i<used;++i){const auto& n=nodes[i];
            if(n.type==Type::Dict){++dicts;if(!n.owner)++roots;}
            if(n.type==Type::BlockRecord&&n.name=="*Model_Space")model=n.self;
            if(n.type==Type::Layer&&!byName(n.ref1,Type::Ltype))return p;
            if(n.type==Type::Block){++blockCount;const auto* b=byHandle(n.owner,Type::BlockRecord);
                if(!b||b->name!=n.name||!byName(n.ref1,Type::Layer))return p;}
            if(n.type==Type::EndBlock&&(!byHandle(n.owner,Type::BlockRecord)||!byName(n.name,Type::Layer)))return p;
            if(n.type==Type::Line||n.type==Type::Mline) {
                if(!byHandle(n.owner,Type::BlockRecord)||!byName(n.name,Type::Layer)||(n.entities&&n.owner!=model))return p;
                if(n.type==Type::Mline){const auto* style=byHandle(n.target,Type::Style);if(!style)return p;
                    unsigned matching=0u;for(std::size_t j=0;j<styles->entries;++j)
                        if(styles->targets[j]==n.target&&styles->keys[j]==n.ref1&&styles->keys[j]==style->name)++matching;
                    if(matching!=1u)return p;}
            }
            if(n.type==Type::Style){++styleCount;if(n.owner!=styles->self||!byName(n.ref1,Type::Ltype)||!byName(n.ref2,Type::Ltype))return p;
                unsigned hits=0;for(std::size_t j=0;j<styles->entries;++j) {
                    if(styles->targets[j]==n.self&&styles->keys[j]==n.name)++hits;
                }if(hits!=1u)return p;}
        }
        for(std::size_t i=0;i<used;++i)if(nodes[i].type==Type::BlockRecord){
            unsigned bodies=0,footers=0;for(std::size_t j=0;j<used;++j){if(nodes[j].owner!=nodes[i].self)continue;
                if(nodes[j].type==Type::Block)++bodies;else if(nodes[j].type==Type::EndBlock)++footers;}
            if(bodies!=1u||footers!=1u)return p;
        }
        for(std::size_t i=0;i<styles->entries;++i){if(!byHandle(styles->targets[i],Type::Style))return p;
            for(std::size_t j=i+1;j<styles->entries;++j)if(nameEq(styles->keys[i],styles->keys[j])||styles->targets[i]==styles->targets[j])return p;}
        if(roots!=1u||dicts!=2u||styleCount!=styles->entries||!model||!blockCount)return p;
        p.result=Result::SourceCandidate;return p;
    }catch(...) {p.result=Result::Refused;return p;}
}
}
