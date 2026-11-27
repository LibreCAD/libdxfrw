// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_WIDE_DICTIONARY_SOURCE_H
#define DX_WIDE_DICTIONARY_SOURCE_H

// Read-only source observation for a selected AC1027 dictionary/XRECORD graph.
// This prepass is not semantic import, output permission, or proof that the
// remaining drawing is lossless. Existing 32-bit consumer callbacks are not
// invoked. It retains original spellings and never narrows handle values.
#include "intern/dxfidentitysource.h"
#include "dx_symbol_name_key.h"
#include <charconv>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace dx_wide_dictionary_source {
struct Field {
    int code{0};
    std::string text, raw;
    std::size_t ordinal{0}, begin{0}, end{0};
};
struct Graph {
    bool binary{false};
    Field rootSelf,rootOwner,rootEntry,dictionarySelf,dictionaryOwner,
        dictionaryEntry,recordSelf,recordOwner,recordReference;
    std::uint64_t root{0},dictionary{0},record{0};
};
namespace detail {
struct Object {std::string name;std::size_t marker{0};std::vector<Field> fields;};
inline bool hex(std::string_view s,std::uint64_t& out) noexcept {
    if(s.empty()||s.size()>16u)return false;
    out=0u;const auto r=std::from_chars(s.data(),s.data()+s.size(),out,16);
    return r.ec==std::errc{}&&r.ptr==s.data()+s.size();
}
inline bool relevant(int code) noexcept {
    return code==3||code==5||code==100||code==101||code==102||code==105
        ||(code>=320&&code<=369)||(code>=390&&code<=399)
        ||(code>=480&&code<=481)||code==1001||code==1005;
}
inline bool common(const Object& object,Field& self,Field& owner,std::size_t& body) {
    unsigned identities=0u,owners=0u;body=object.fields.size();
    for(std::size_t i=0;i<object.fields.size();++i) {
        const auto& field=object.fields[i];
        if(field.code==102||field.code==101||field.code==1001||field.code==1005
            ||(field.code>=320&&field.code<=369&&field.code!=330)
            ||(field.code>=390&&field.code<=399)||(field.code>=480&&field.code<=481)
            ||field.code==105)return false;
        if(field.code==100){body=i;break;}
        if(field.code==5){if(++identities!=1u)return false;self=field;}
        if(field.code==330){if(++owners!=1u)return false;owner=field;}
    }
    return identities==1u&&owners==1u&&body<object.fields.size();
}
inline bool dictionaryEntry(const Object& object,std::size_t body,
        std::string_view wanted,Field& entry) {
    if(object.name!="DICTIONARY"||object.fields[body].text!="AcDbDictionary")return false;
    std::string pending;unsigned matches=0u;
    std::unordered_set<std::string> names;
    std::unordered_set<std::uint64_t> targets;
    for(std::size_t i=body+1u;i<object.fields.size();++i) {
        const auto& f=object.fields[i];
        if(f.code==102||f.code==1001||f.code==1005||f.code==101||f.code==100
            ||f.code==5||f.code==105||(f.code>=320&&f.code<=369&&f.code!=350&&f.code!=360)
            ||(f.code>=390&&f.code<=399)||(f.code>=480&&f.code<=481))return false;
        if(f.code==3){if(!pending.empty()||f.text.empty())return false;pending=f.text;continue;}
        if(f.code==350||f.code==360){
            if(pending.empty())return false;
            std::uint64_t value=0u;if(!hex(f.text,value)||!value)return false;
            std::string key;
            if(!dx_symbol_output::nameKey(pending,key)||key.empty()
                ||!names.insert(std::move(key)).second||!targets.insert(value).second)return false;
            if(pending==wanted){if(++matches!=1u)return false;entry=f;}
            pending.clear();
        } else if(!pending.empty())return false;
    }
    return pending.empty()&&matches==1u;
}
inline bool recordReference(const Object& object,std::size_t body,Field& reference) {
    if(object.name!="XRECORD"||object.fields[body].text!="AcDbXrecord")return false;
    unsigned references=0u;
    for(std::size_t i=body+1u;i<object.fields.size();++i){
        const auto& f=object.fields[i];
        // Unknown application data, XDATA and additional known reference
        // roles need separate closure proof. Do not treat them as harmless.
        if(f.code==102||f.code==101||f.code==1001||f.code==100
            ||f.code==5||f.code==105||f.code==1005
            ||(f.code>=320&&f.code<=369&&f.code!=340)
            ||(f.code>=390&&f.code<=399)||(f.code>=480&&f.code<=481))return false;
        if(f.code==340){if(++references!=1u)return false;reference=f;}
    }
    return references==1u;
}
}
inline bool capture(std::istream& input,std::string_view dictionaryKey,
        std::string_view recordKey,Graph& output) noexcept {
    using namespace detail;
    output=Graph{};
    try {
        constexpr std::size_t maxBytes=8u*1024u*1024u,maxFields=200000u,maxObjects=10000u;
        if(dictionaryKey.empty()||recordKey.empty()||dictionaryKey.size()>255u||recordKey.size()>255u)return false;
        char prefix[22]{};input.read(prefix,sizeof(prefix));
        if(input.gcount()!=sizeof(prefix))return false;
        const bool binary=std::string_view(prefix,sizeof(prefix))==std::string_view("AutoCAD Binary DXF\r\n\x1a\0",22);
        input.clear();input.seekg(binary?22:0,std::ios::beg);if(!input.good())return false;
        std::unique_ptr<dxfReader> reader=binary?std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
            :std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
        reader->setAllowWideHandleLexemes(true);reader->setAllowDimstyleNames(true);
        reader->setRecordBudget(maxFields);reader->setInputByteBudget(maxBytes,binary?22u:0u);
        reader->setTokenByteBudget(65536u);reader->setDecodeWorkBudget(maxBytes*8u);
        dxfIdentitySource::Capture identity(*reader);
        DRW_DxfIdentitySourceLimits limits;limits.bytes=maxBytes;limits.work=maxBytes*8u;
        limits.records=maxObjects;limits.candidates=maxFields;identity.start(limits);
        bool sectionName=false,objects=false,header=false,eof=false,versionNext=false;
        std::size_t headerSections=0u,objectSections=0u,versions=0u;
        std::string version;std::vector<Object> rows;std::size_t retained=0u;int code=0;
        while(reader->readRec(&code)){
            DxfReadRecordView v;if(!reader->currentRecordView(v))return false;
            if(code==0){const auto name=reader->getStringView();
                if(name=="SECTION"){sectionName=true;objects=header=false;}
                else if(name=="ENDSEC"){sectionName=objects=header=false;}
                else if(name=="EOF"){eof=true;break;}
                else if(objects){if(rows.size()>=maxObjects||sizeof(Object)>maxBytes-retained)return false;
                    retained+=sizeof(Object);if(name.size()>maxBytes-retained)return false;
                    retained+=name.size();rows.push_back({std::string(name),v.ordinal,{}});}
                continue;
            }
            if(sectionName){if(code!=2)return false;objects=reader->getStringView()=="OBJECTS";
                header=reader->getStringView()=="HEADER";
                if(objects&&++objectSections!=1u)return false;
                if(header&&++headerSections!=1u)return false;
                sectionName=false;continue;}
            if(header&&code==9)versionNext=reader->getStringView()=="$ACADVER";
            else if(header&&versionNext){if(code==1){if(++versions!=1u)return false;version=reader->getStringView();}versionNext=false;}
            if(objects&&!rows.empty()&&relevant(code)){
                if(v.kind!=DxfReadRecordView::Text||sizeof(Field)>maxBytes-retained)return false;
                retained+=sizeof(Field);if(v.text.size()>maxBytes-retained)return false;
                retained+=v.text.size();if(v.raw.size()>maxBytes-retained)return false;retained+=v.raw.size();
                rows.back().fields.push_back({code,std::string(v.text),std::string(v.raw),v.ordinal,v.beginOffset,v.endOffset});
            }
        }
        identity.finish();const auto& census=identity.source();
        if(!eof||headerSections!=1u||objectSections!=1u||versions!=1u||version!="AC1027"||census.status!=DRW_DxfIdentitySource::Status::Complete
            ||input.peek()!=std::char_traits<char>::eof()||input.bad()||rows.empty())return false;
        const auto physical=[&](const Object& object){
            std::size_t found=DRW_DxfIdentitySource::NoIndex;
            for(std::size_t i=0;i<census.records.size();++i)if(census.records[i].marker.ordinal==object.marker){
                if(found!=DRW_DxfIdentitySource::NoIndex||census.records[i].kind!=DRW_DxfIdentitySource::Kind::Object)return DRW_DxfIdentitySource::NoIndex;
                found=i;
            }
            return found;
        };
        Graph candidate;candidate.binary=binary;std::size_t body=0u;
        const Object& root=rows.front();
        if(!common(root,candidate.rootSelf,candidate.rootOwner,body)
            ||!dictionaryEntry(root,body,dictionaryKey,candidate.rootEntry)
            ||!hex(candidate.rootSelf.text,candidate.root)||!candidate.root)return false;
        std::uint64_t owner=0u;
        if(!hex(candidate.rootOwner.text,owner)||owner||!hex(candidate.rootEntry.text,candidate.dictionary)||!candidate.dictionary)return false;
        const auto findObject=[&](std::uint64_t handle)->const Object*{
            const Object* result=nullptr;
            for(const auto& row:rows){Field self,own;std::size_t start=0u;std::uint64_t value=0u;
                if(!common(row,self,own,start)||!hex(self.text,value)||value!=handle)continue;
                if(result)return nullptr;
                result=&row;
            }
            return result;
        };
        const auto* dict=findObject(candidate.dictionary);if(!dict||dict==&root)return false;
        if(!common(*dict,candidate.dictionarySelf,candidate.dictionaryOwner,body)
            ||!hex(candidate.dictionaryOwner.text,owner)||owner!=candidate.root
            ||!dictionaryEntry(*dict,body,recordKey,candidate.dictionaryEntry)
            ||!hex(candidate.dictionaryEntry.text,candidate.record)||!candidate.record||candidate.record==candidate.root||candidate.record==candidate.dictionary)return false;
        const auto* record=findObject(candidate.record);if(!record||record==dict||record==&root)return false;
        if(!common(*record,candidate.recordSelf,candidate.recordOwner,body)
            ||!hex(candidate.recordOwner.text,owner)||owner!=candidate.dictionary
            ||!recordReference(*record,body,candidate.recordReference)
            ||!hex(candidate.recordReference.text,owner)||owner!=candidate.dictionary)return false;
        const Object* selected[]{&root,dict,record};const std::uint64_t values[]{candidate.root,candidate.dictionary,candidate.record};
        for(std::size_t n=0;n<3u;++n){const auto index=physical(*selected[n]);if(index==DRW_DxfIdentitySource::NoIndex)return false;
            std::size_t global=0u,local=0u;
            for(const auto& c:census.candidates){std::uint64_t value=0u;
                if(c.scope!=DRW_DxfIdentitySource::Scope::Outer||!hex(c.lexeme,value)||value!=values[n])continue;
                ++global;if(c.record==index&&c.code==5)++local;
            }
            if(global!=1u||local!=1u)return false;
        }
        output=std::move(candidate);return true;
    }catch(...){output=Graph{};return false;}
}
}
#endif
