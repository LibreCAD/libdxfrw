// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_WIDE_DICTIONARY_SOURCE_H
#define DX_WIDE_DICTIONARY_SOURCE_H

// Read-only source observation for a selected AC1027 dictionary/XRECORD graph.
// This prepass is not semantic import, output permission, or proof that the
// remaining drawing is lossless. Existing 32-bit consumer callbacks are not
// invoked. It retains original spellings and never narrows handle values.
#include "intern/dxfidentitysource.h"
#include "dx_symbol_name_key.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <istream>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace dx_wide_dictionary_source {
struct Field {
    int code{0};
    std::string text, raw;
    std::size_t ordinal{0}, begin{0}, end{0};
};
struct Chain {
    std::string dictionaryKey,recordKey;
    Field rootEntry,dictionarySelf,dictionaryOwner,dictionaryEntry,
        recordSelf,recordOwner,recordReference;
    std::uint64_t dictionary{0},record{0};
};
struct Graph {
    bool binary{false};
    std::string dictionaryKey,recordKey;
    Field rootSelf,rootOwner,rootEntry,dictionarySelf,dictionaryOwner,
        dictionaryEntry,recordSelf,recordOwner,recordReference,handSeed;
    std::vector<Field> handleCandidates;
    std::vector<Chain> chains;
    std::uint64_t root{0},dictionary{0},record{0};
};
struct ChainAlias {std::uint32_t dictionary{0},record{0};};
struct Normalized {
    std::string bytes;
    Graph source;
    std::vector<ChainAlias> chains;
    std::uint32_t root{0},dictionary{0},record{0},handSeed{0};
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
inline bool handleCandidate(int code) noexcept {
    return code==5||code==105||(code>=320&&code<=369)
        ||(code>=390&&code<=399)||(code>=480&&code<=481)||code==1005;
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
struct Entry {std::string name;Field target;std::uint64_t value{0};};
inline bool dictionaryEntries(const Object& object,std::size_t body,
        std::size_t limit,std::vector<Entry>& entries) {
    if(object.name!="DICTIONARY"||object.fields[body].text!="AcDbDictionary")return false;
    std::string pending;
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
            if(entries.size()>=limit)return false;
            entries.push_back({pending,f,value});
            pending.clear();
        } else if(!pending.empty())return false;
    }
    return pending.empty();
}
inline bool dictionaryEntry(const std::vector<Entry>& entries,
        std::string_view wanted,Field& entry) {
    for(const auto& item:entries)if(item.name==wanted){entry=item.target;return true;}
    return false;
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
        std::string_view recordKey,Graph& output,
        const std::vector<std::pair<std::string,std::string>>* selectedKeys=nullptr) noexcept {
    using namespace detail;
    output=Graph{};
    try {
        constexpr std::size_t maxBytes=8u*1024u*1024u,maxFields=200000u,maxObjects=10000u;
        if(dictionaryKey.empty()!=recordKey.empty()||dictionaryKey.size()>255u||recordKey.size()>255u
            ||(selectedKeys&&(!dictionaryKey.empty()||selectedKeys->empty()||selectedKeys->size()>8u)))return false;
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
        std::string version,headerVariable;Field handSeed;unsigned seeds=0u;
        std::vector<Field> handles;std::vector<Object> rows;std::size_t retained=0u;int code=0;
        while(reader->readRec(&code)){
            DxfReadRecordView v;if(!reader->currentRecordView(v))return false;
            if(handleCandidate(code)) {
                if(v.kind!=DxfReadRecordView::Text||sizeof(Field)>maxBytes-retained)return false;
                retained+=sizeof(Field);if(v.text.size()>maxBytes-retained)return false;
                retained+=v.text.size();if(v.raw.size()>maxBytes-retained)return false;retained+=v.raw.size();
                handles.push_back({code,std::string(v.text),std::string(v.raw),v.ordinal,v.beginOffset,v.endOffset});
            }
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
            if(header&&code==9){headerVariable=reader->getStringView();versionNext=headerVariable=="$ACADVER";}
            else if(header&&versionNext){if(code==1){if(++versions!=1u)return false;version=reader->getStringView();}versionNext=false;}
            if(header&&code==5&&headerVariable=="$HANDSEED") {
                if(++seeds!=1u||handles.empty())return false;
                handSeed=handles.back();headerVariable.clear();
            }
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
        Graph candidate;candidate.binary=binary;candidate.handSeed=std::move(handSeed);
        candidate.handleCandidates=std::move(handles);std::size_t body=0u;
        const Object& root=rows.front();
        if(!common(root,candidate.rootSelf,candidate.rootOwner,body)
            ||!hex(candidate.rootSelf.text,candidate.root)||!candidate.root)return false;
        std::uint64_t owner=0u;
        if(!hex(candidate.rootOwner.text,owner)||owner)return false;
        std::vector<Entry> rootEntries;
        if(!dictionaryEntries(root,body,16u,rootEntries))return false;
        std::unordered_map<std::uint64_t,const Object*> objectIndex;
        std::unordered_set<std::uint64_t> ambiguous;
        for(const auto& row:rows){Field self,own;std::size_t start=0u;std::uint64_t value=0u;
            if(common(row,self,own,start)&&hex(self.text,value)
                &&!objectIndex.emplace(value,&row).second)ambiguous.insert(value);
        }
        const auto findObject=[&](std::uint64_t handle)->const Object*{
            const auto found=objectIndex.find(handle);
            return ambiguous.count(handle)||found==objectIndex.end()?nullptr:found->second;
        };
        std::vector<std::pair<std::string,std::string>> selections;
        if(selectedKeys) {
            for(const auto& keys:*selectedKeys) {
                if(keys.first.empty()||keys.second.empty()||keys.first.size()>255u||keys.second.size()>255u)return false;
                selections.push_back(keys);
            }
        } else if(dictionaryKey.empty()) {
            for(const auto& rootEntry:rootEntries) {
                const auto* possibleDictionary=findObject(rootEntry.value);
                if(!possibleDictionary||possibleDictionary==&root)continue;
                Field self,parent;std::size_t start=0u;std::uint64_t value=0u;
                if(!common(*possibleDictionary,self,parent,start)||!hex(parent.text,value)
                    ||value!=candidate.root)continue;
                std::vector<Entry> entries;
                if(!dictionaryEntries(*possibleDictionary,start,256u,entries))continue;
                for(const auto& item:entries) {
                    if(rootEntry.value<UINT64_C(0xffffffff)&&item.value<UINT64_C(0xffffffff))continue;
                    const auto* possibleRecord=findObject(item.value);
                    if(!possibleRecord||possibleRecord==possibleDictionary||possibleRecord==&root)continue;
                    Field recordSelf,recordOwner,reference;std::size_t recordBody=0u;
                    if(!common(*possibleRecord,recordSelf,recordOwner,recordBody)
                        ||!hex(recordOwner.text,value)||value!=rootEntry.value
                        ||!recordReference(*possibleRecord,recordBody,reference)
                        ||!hex(reference.text,value)||value!=rootEntry.value)continue;
                    if(rootEntry.name.size()>255u||item.name.size()>255u||selections.size()>=8u)return false;
                    selections.emplace_back(rootEntry.name,item.name);
                }
            }
            if(selections.empty())return false;
        } else selections.emplace_back(dictionaryKey,recordKey);
        std::unordered_set<std::uint64_t> selectedValues{candidate.root};
        std::vector<std::pair<const Object*,std::uint64_t>> selectedObjects{{&root,candidate.root}};
        for(const auto& selection:selections) {
            Chain chain;chain.dictionaryKey=selection.first;chain.recordKey=selection.second;
            if(!dictionaryEntry(rootEntries,chain.dictionaryKey,chain.rootEntry)
                ||!hex(chain.rootEntry.text,chain.dictionary)||!chain.dictionary
                ||!selectedValues.insert(chain.dictionary).second)return false;
            const auto* dict=findObject(chain.dictionary);if(!dict||dict==&root)return false;
            std::vector<Entry> dictionaryEntriesFound;
            if(!common(*dict,chain.dictionarySelf,chain.dictionaryOwner,body)
                ||!hex(chain.dictionaryOwner.text,owner)||owner!=candidate.root
                ||!dictionaryEntries(*dict,body,256u,dictionaryEntriesFound)
                ||!dictionaryEntry(dictionaryEntriesFound,chain.recordKey,chain.dictionaryEntry)
                ||!hex(chain.dictionaryEntry.text,chain.record)||!chain.record
                ||!selectedValues.insert(chain.record).second)return false;
            const auto* record=findObject(chain.record);if(!record||record==dict||record==&root)return false;
            if(!common(*record,chain.recordSelf,chain.recordOwner,body)
                ||!hex(chain.recordOwner.text,owner)||owner!=chain.dictionary
                ||!recordReference(*record,body,chain.recordReference)
                ||!hex(chain.recordReference.text,owner)||owner!=chain.dictionary)return false;
            selectedObjects.emplace_back(dict,chain.dictionary);
            selectedObjects.emplace_back(record,chain.record);
            candidate.chains.push_back(std::move(chain));
        }
        // Each selected identity must correspond to exactly one physical object,
        // including collisions with records outside the OBJECTS section.
        struct IdentityCount {std::size_t index,global{0u},local{0u};};
        std::unordered_map<std::uint64_t,IdentityCount> identityCounts;
        for(const auto& selected:selectedObjects) {
            const auto index=physical(*selected.first);if(index==DRW_DxfIdentitySource::NoIndex)return false;
            if(!identityCounts.emplace(selected.second,IdentityCount{index}).second)return false;
        }
        for(const auto& c:census.candidates) {
            std::uint64_t value=0u;
            if(c.scope!=DRW_DxfIdentitySource::Scope::Outer||!hex(c.lexeme,value))continue;
            const auto found=identityCounts.find(value);if(found==identityCounts.end())continue;
            ++found->second.global;if(c.record==found->second.index&&c.code==5)++found->second.local;
        }
        for(const auto& count:identityCounts)
            if(count.second.global!=1u||count.second.local!=1u)return false;
        const auto& first=candidate.chains.front();
        candidate.dictionaryKey=first.dictionaryKey;candidate.recordKey=first.recordKey;
        candidate.rootEntry=first.rootEntry;candidate.dictionarySelf=first.dictionarySelf;
        candidate.dictionaryOwner=first.dictionaryOwner;candidate.dictionaryEntry=first.dictionaryEntry;
        candidate.recordSelf=first.recordSelf;candidate.recordOwner=first.recordOwner;
        candidate.recordReference=first.recordReference;candidate.dictionary=first.dictionary;candidate.record=first.record;
        output=std::move(candidate);return true;
    }catch(...){output=Graph{};return false;}
}

// Prepare an in-memory projection from the same immutable byte snapshot used
// for observation. Only the selected graph's proven handle roles and HANDSEED
// may change. Every other wide handle candidate is refused. This is still a
// transport preparation step: the caller must bind actual import, emitted
// handles and graph closure before treating the projection as a valid export.
inline bool normalize(std::string_view input,std::string_view dictionaryKey,
        std::string_view recordKey,Normalized& output) noexcept {
    using namespace detail;
    try {
        constexpr std::size_t maxBytes=8u*1024u*1024u;
        constexpr std::uint64_t reserved=UINT64_C(0xffffffff);
        if(input.size()>maxBytes){output=Normalized{};return false;}
        // Copy before clearing output: callers may pass a view of its bytes.
        const std::string snapshot(input);
        output=Normalized{};
        std::istringstream stream(snapshot,std::ios::binary);
        Normalized candidate;
        if(!capture(stream,dictionaryKey,recordKey,candidate.source))return false;
        auto& source=candidate.source;
        if(source.handSeed.code!=5)return false;
        std::uint64_t sourceSeed=0u;
        if(!hex(source.handSeed.text,sourceSeed)||sourceSeed<=source.root)return false;
        std::vector<std::uint64_t> graphHandles{source.root};
        for(const auto& chain:source.chains) {
            if(sourceSeed<=chain.dictionary||sourceSeed<=chain.record)return false;
            graphHandles.push_back(chain.dictionary);graphHandles.push_back(chain.record);
        }
        struct Binding {std::uint64_t source;std::uint32_t target;};
        std::unordered_set<std::uint32_t> occupied;
        std::uint32_t maxUsed=0u;
        for(const auto& field:source.handleCandidates) {
            if(field.ordinal==source.handSeed.ordinal)continue;
            std::uint64_t value=0u;
            if(hex(field.text,value)&&value<reserved) {
                occupied.insert(static_cast<std::uint32_t>(value));
                maxUsed=std::max(maxUsed,static_cast<std::uint32_t>(value));
            }
        }
        std::vector<Binding> bindings;
        std::uint32_t next=1u;
        for(const auto handle:graphHandles) {
            if(handle<reserved)continue;
            while(next<reserved&&occupied.count(next))++next;
            if(next>=reserved)return false;
            bindings.push_back({handle,next});occupied.insert(next);
            maxUsed=std::max(maxUsed,next);++next;
        }
        if(bindings.empty()||maxUsed>=reserved-1u)return false;
        const auto mapped=[&](std::uint64_t value)->std::uint32_t {
            for(const auto& binding:bindings)if(binding.source==value)return binding.target;
            return value<reserved?static_cast<std::uint32_t>(value):0u;
        };
        candidate.root=mapped(source.root);candidate.handSeed=maxUsed+1u;
        if(!candidate.root||!candidate.handSeed)return false;
        std::vector<const Field*> selected{&source.rootSelf};
        for(const auto& chain:source.chains) {
            const ChainAlias alias{mapped(chain.dictionary),mapped(chain.record)};
            if(!alias.dictionary||!alias.record)return false;
            candidate.chains.push_back(alias);
            for(const auto* field:{&chain.rootEntry,&chain.dictionarySelf,&chain.dictionaryOwner,
                    &chain.dictionaryEntry,&chain.recordSelf,&chain.recordOwner,&chain.recordReference})
                selected.push_back(field);
        }
        candidate.dictionary=candidate.chains.front().dictionary;
        candidate.record=candidate.chains.front().record;
        for(const auto& field:source.handleCandidates) {
            std::uint64_t value=0u;
            if(!hex(field.text,value)||value<reserved)continue;
            if(field.ordinal==source.handSeed.ordinal)continue;
            bool known=false;
            for(const auto* item:selected)if(item->ordinal==field.ordinal){known=true;break;}
            if(!known||!mapped(value))return false;
        }
        struct Edit {std::size_t begin,end;std::string text;};
        std::vector<Edit> edits;
        const auto append=[&](const Field& field,std::uint32_t value) {
            if(field.begin>=field.end||field.end>snapshot.size()||field.raw!=field.text)return false;
            std::size_t start=0u;
            if(source.binary) {
                if(field.end-field.begin<3u)return false;
                const auto code=static_cast<unsigned char>(snapshot[field.begin])
                    |(static_cast<unsigned char>(snapshot[field.begin+1u])<<8u);
                if(code!=field.code)return false;
                start=field.begin+2u;
                if(start+field.raw.size()+1u!=field.end||snapshot[field.end-1u]!='\0')return false;
            } else {
                const auto newline=snapshot.find('\n',field.begin);
                if(newline==std::string::npos||newline>=field.end)return false;
                start=newline+1u;
                if(field.raw.size()>field.end-start)return false;
                const auto tail=std::string_view(snapshot).substr(start+field.raw.size(),field.end-start-field.raw.size());
                if(tail!="\n"&&tail!="\r\n")return false;
            }
            if(field.raw.size()>field.end-start||snapshot.compare(start,field.raw.size(),field.raw)!=0)return false;
            char buffer[8];const auto converted=std::to_chars(buffer,buffer+sizeof(buffer),value,16);
            if(converted.ec!=std::errc{})return false;
            std::string text(buffer,converted.ptr);
            for(char& ch:text)if(ch>='a'&&ch<='f')ch=static_cast<char>(ch-'a'+'A');
            edits.push_back({start,start+field.raw.size(),std::move(text)});return true;
        };
        for(const auto* field:selected) {
            std::uint64_t value=0u;if(!hex(field->text,value))return false;
            if(value>=reserved&&!append(*field,mapped(value)))return false;
        }
        if(!append(source.handSeed,candidate.handSeed))return false;
        std::sort(edits.begin(),edits.end(),[](const auto& a,const auto& b){return a.begin>b.begin;});
        candidate.bytes=snapshot;std::size_t prior=snapshot.size();
        for(const auto& edit:edits) {
            if(edit.end>prior||edit.begin>edit.end)return false;
            candidate.bytes.replace(edit.begin,edit.end-edit.begin,edit.text);prior=edit.begin;
        }
        std::vector<std::pair<std::string,std::string>> keys;
        for(const auto& chain:source.chains)keys.emplace_back(chain.dictionaryKey,chain.recordKey);
        Graph check;std::istringstream transformed(candidate.bytes,std::ios::binary);
        if(!capture(transformed,"","",check,&keys)||check.root!=candidate.root
            ||check.chains.size()!=candidate.chains.size())return false;
        std::uint64_t seed=0u;if(!hex(check.handSeed.text,seed)||seed!=candidate.handSeed)return false;
        for(std::size_t i=0;i<source.chains.size();++i) {
            const auto& chain=source.chains[i];const auto& actual=check.chains[i];const auto& alias=candidate.chains[i];
            if(actual.dictionaryKey!=chain.dictionaryKey||actual.recordKey!=chain.recordKey
                ||actual.dictionary!=alias.dictionary||actual.record!=alias.record
                ||actual.rootEntry.code!=chain.rootEntry.code||actual.dictionaryEntry.code!=chain.dictionaryEntry.code
                ||actual.recordReference.code!=chain.recordReference.code)return false;
        }
        output=std::move(candidate);return true;
    }catch(...){output=Normalized{};return false;}
}
}
#endif
