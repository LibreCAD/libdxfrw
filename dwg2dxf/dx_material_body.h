// SPDX-License-Identifier: GPL-2.0-or-later
// Bounded, current-model MATERIAL edit reconciliation. The import baseline is
// adapter-private and is not a producer/native-semantic support certificate.
#ifndef DX_MATERIAL_BODY_H
#define DX_MATERIAL_BODY_H
#include <map>
#include <set>
#include <sstream>
#include "dx_standard_output.h"
#include "intern/dxfmaterial.h"
namespace dx_material_body {
using Groups = std::vector<DRW_Variant>;
using Slots = std::map<int,std::vector<std::size_t>>;
// Ordered semantic keys, not just DXF group numbers: 43 and 270/271/272/273
// have multiple roles. Procedural values and self-illumination stay opaque.
inline const std::vector<int>& order() {
    static const std::vector<int> keys={1,2,70,40,90,71,41,91,42,72,3,73,74,75,43,
        44,76,45,92,46,77,4,78,79,170,47,48,171,6,172,173,174,49,
        140,141,175,7,176,177,178,142,143,179,8,270,271,272,144,
        145,146,273,9,274,275,276,147,460,461,462,463,290,
        1464,1270,1271,1465,1042,1072,1003,1073,1074,1075,1043,
        1293,1272,1273,2292,3270,3148,3468,3093,3094};
    return keys;
}
inline std::size_t rank(int key,bool strengthFirst=false) {
    if(strengthFirst) {
        if(key==1465)key=1270;
        else if(key==1270)key=1271;
        else if(key==1271)key=1465;
    }
    const auto& keys=order();return static_cast<std::size_t>(std::find(keys.begin(),keys.end(),key)-keys.begin());
}
inline bool slots(const Groups& groups,Slots& result,std::size_t& bodyEnd,std::size_t* procedure=nullptr) {
    unsigned depth=0u,phase=0u,markers=0u,refractionMatrix=0u;
    bool body=false,xdata=false,bumpProjection=false,bumpTiling=false;
    bodyEnd=groups.size();
    if(procedure)*procedure=groups.size();
    for(std::size_t i=0;i<groups.size();++i) {
        const auto& value=groups[i];const auto code=value.code();
        if(code==102) {
            if(value.type()!=DRW_Variant::STRING||!value.content.s)return false;
            const auto& text=*value.content.s;
            if(text=="}"){if(!depth)return false;--depth;}
            else if(!text.empty()&&text.front()=='{'){if(++depth>DRW::kMaxDxfApplicationGroupNesting)return false;}
            else return false;
            continue;
        }
        if(depth)continue;
        if(code>=1000){if(!xdata)bodyEnd=i;xdata=true;continue;}
        if(xdata)return false;
        if(code==100) {
            if(value.type()!=DRW_Variant::STRING||!value.content.s)return false;
            if(*value.content.s=="AcDbMaterial"){if(++markers!=1u)return false;body=true;}
            else if(body)return false;
            continue;
        }
        if(!body)continue;
        if(phase==0u&&code==147)++refractionMatrix;
        if(code==300){phase=2u;if(procedure)*procedure=std::min(*procedure,i);continue;}
        int key=-1;
        if(code==292){key=2292;phase=3u;}
        else if(phase<2u&&(code==464||code==465)){phase=1u;key=1000+code;}
        else if(phase<2u&&(code==290||(code>=460&&code<=463)))key=code;
        else if(phase==0u&&refractionMatrix==16u
            &&((code==270&&bumpProjection)||(code==271&&bumpTiling)))key=1000+code;
        else if(code==293&&phase<2u)key=1293;
        else if(phase==0u&&rank(code)<rank(1464))key=code;
        else if(phase==1u&&rank(1000+code)>=rank(1464)&&rank(1000+code)<rank(2292))key=1000+code;
        else if(phase==3u&&rank(3000+code)<order().size())key=3000+code;
        if(key==270)bumpProjection=true;
        if(key==271)bumpTiling=true;
        if(key>=0)result[key].push_back(i);
    }
    return depth==0u&&markers==1u&&phase!=2u;
}
inline bool equal(const DRW_Variant& a,const DRW_Variant& b) {
    if(a.code()!=b.code()||a.type()!=b.type())return false;
    switch(a.type()) {
    case DRW_Variant::STRING:return a.content.s&&b.content.s&&*a.content.s==*b.content.s;
    case DRW_Variant::INTEGER:return a.i_val()==b.i_val();
    case DRW_Variant::INTEGER64:return a.i64_val()==b.i64_val();
    case DRW_Variant::DOUBLE:return dxfMaterial::sameValue(a.d_val(),b.d_val());
    default:return false;
    }
}
inline bool fieldEqual(const Groups& a,const Slots& sa,const Groups& b,const Slots& sb,int key) {
    const auto ia=sa.find(key),ib=sb.find(key);
    if(ia==sa.end()||ib==sb.end())return ia==sa.end()&&ib==sb.end();
    if(ia->second.size()!=ib->second.size())return false;
    for(std::size_t n=0;n<ia->second.size();++n)if(!equal(a[ia->second[n]],b[ib->second[n]]))return false;
    return true;
}
class BodyWriter final:public dxfWriter {
public:
    Groups groups;
    explicit BodyWriter(bool binary,DRW::Version version=DRW::AC1027,std::string_view page="UTF-8")
        :dxfWriter(nullptr,binary) {
        for(const auto& row:DRW::dwgVersionStrings)if(row.second==version)setVersion(row.first,true);
        setCodePage(std::string(page));setTextWritePolicy(DRW::DxfTextWritePolicy::RequireLossless);
    }
    bool writeString(int code,std::string_view text) override{groups.emplace_back(code,UTF8STRING(text));return true;}
    bool writeInt16(int code,int value) override{groups.emplace_back(code,static_cast<std::int32_t>(value));return true;}
    bool writeInt32(int code,int value) override{
        if(code>=90&&code<=92){
            std::uint32_t bits=0u;const auto signedValue=static_cast<std::int32_t>(value);
            std::memcpy(&bits,&signedValue,sizeof(bits));groups.emplace_back(code,static_cast<std::int64_t>(bits));return true;
        }
        return writeInt16(code,value);
    }
    bool writeInt64(int,std::int64_t) override{markWriteError();return false;}
    bool writeDouble(int code,double value) override{groups.emplace_back(code,value);return true;}
    bool writeBool(int code,bool value) override{return writeInt16(code,value?1:0);}
};
inline bool integer(dxfWriter& writer,int code,std::int64_t number,bool binary=false) {
    switch(dxfValueKindForCode(code)) {
    case DxfValueKind::I16:return number>=INT16_MIN&&number<=UINT16_MAX&&writer.writeInt16(code,static_cast<int>(number));
    case DxfValueKind::Bln:return number>=0&&number<=1&&writer.writeBool(code,number!=0);
    case DxfValueKind::I32:
        if(code>=90&&code<=92&&number>=INT32_MIN&&number<=UINT32_MAX) {
            const auto bits=static_cast<std::uint32_t>(number);
            if(!binary)return writer.writeRawString(code,std::to_string(bits));
            std::int32_t value=0;std::memcpy(&value,&bits,sizeof(value));return writer.writeInt32(code,value);
        }
        return number>=INT32_MIN&&number<=INT32_MAX&&writer.writeInt32(code,static_cast<int>(number));
    case DxfValueKind::I64:return writer.writeInt64(code,number);
    default:return false;
    }
}
inline bool canonical(const DRW_Material& value,Groups& groups,Slots& roles,bool binary) {
    BodyWriter writer(binary);writer.groups.emplace_back(100,UTF8STRING("AcDbMaterial"));
    if(!dxfMaterial::writeBody(writer,value,true))return false;
    groups=std::move(writer.groups);std::size_t end=0;return slots(groups,roles,end);
}
// Decode physical values through the owning record parser in their captured
// wire format. Binary strings must not acquire ASCII-only caret semantics.
template<class Record> inline bool decodeRecord(const DRW_RawDxfObject& raw,Record& result,bool binary,
        std::string_view page="UTF-8",DRW::DxfTextEncoding encoding=DRW::DxfTextEncoding::Declared,
        DRW::DxfCodePagePolicy policy=DRW::DxfCodePagePolicy::Compatible,
        dxfRW::DxfCompatibilityProfile compatibility=dxfRW::DxfCompatibilityProfile::StandaloneSafe) {
    std::ostringstream bytes;
    std::unique_ptr<dxfWriter> writer=binary?std::unique_ptr<dxfWriter>(new dxfWriterBinary(&bytes)):std::unique_ptr<dxfWriter>(new dxfWriterAscii(&bytes));
    writer->setVersion("AC1027",true);
    for(const auto& value:raw.groups) {
        bool good=false;const auto code=value.code();
        if(value.type()==DRW_Variant::STRING&&value.content.s)good=writer->writeRawString(code,*value.content.s);
        else if(value.type()==DRW_Variant::INTEGER)good=integer(*writer,code,value.i_val(),binary);
        else if(value.type()==DRW_Variant::DOUBLE)good=writer->writeDouble(code,value.d_val());
        else if(value.type()==DRW_Variant::INTEGER64)good=integer(*writer,code,value.i64_val(),binary);
        if(!good)return false;
    }
    if(writer->hasWriteError())return false;
    const std::string storage=bytes.str();
    const dxfRW::DxfObjectsWriteView view{storage,raw.m_version,binary,page,compatibility};
    dx_standard_output::Decoder reader(view,true,encoding,policy);
    return reader.decodeRecord<Record>(storage,[&](const Record& value){
        result=value;return true;
    });
}
inline bool decode(const DRW_RawDxfObject& raw,DRW_Material& result,bool binary) {
    return decodeRecord(raw,result,binary)&&result.hasCompleteDxfMapTransforms();
}
inline bool spelling(const DRW_Variant& value,std::string& result) {
    std::ostringstream stream;dxfWriterAscii writer(&stream);const auto code=value.code();bool good=false;
    if(value.type()==DRW_Variant::STRING&&value.content.s)good=writer.writeString(code,*value.content.s);
    else if(value.type()==DRW_Variant::INTEGER)good=integer(writer,code,value.i_val());
    else if(value.type()==DRW_Variant::INTEGER64)good=integer(writer,code,value.i64_val());
    else if(value.type()==DRW_Variant::DOUBLE)good=writer.writeDouble(code,value.d_val());
    if(!good||writer.hasWriteError())return false;
    const auto text=stream.str();const auto begin=text.find('\n');
    if(begin==std::string::npos||text.empty()||text.back()!='\n')return false;
    result=text.substr(begin+1u,text.size()-begin-2u);return true;
}
// Reconcile each represented field against an immutable import baseline.
// Disjoint raw/typed edits compose; a same-field disagreement refuses. A raw
// edit owns unknown groups. Only changed represented slots are rewritten.
template<class CommonEqual>
inline bool reconcile(const DRW_RawDxfObject& baseline,const DRW_RawDxfObject& raw,
        const DRW_Material& typed,DRW_RawDxfObject& output,UTF8STRING& name,bool binary,
        const CommonEqual& commonEqual) {
    const bool sourceBinary=!baseline.hasRawValues;
    DRW_Material original,parsed;
    if(!decode(baseline,original,sourceBinary)||!decode(raw,parsed,sourceBinary)
        ||!commonEqual(original,typed,baseline.m_version))return false;
    bool unchanged=baseline.name==raw.name&&baseline.handle==raw.handle
        &&baseline.parentHandle==raw.parentHandle&&baseline.m_version==raw.m_version
        &&baseline.hasRawValues==raw.hasRawValues&&baseline.rawValues==raw.rawValues
        &&baseline.groups.size()==raw.groups.size();
    for(std::size_t i=0;unchanged&&i<raw.groups.size();++i)unchanged=equal(baseline.groups[i],raw.groups[i]);
    bool crossCaret=false;
    if(sourceBinary!=binary)for(const auto& value:raw.groups)
        if(value.type()==DRW_Variant::STRING&&value.content.s&&value.content.s->find('^')!=std::string::npos)crossCaret=true;
    if(unchanged&&dxfMaterial::sameState(typed,original)&&!crossCaret) {
        // Exact replay does not require inventing a schema for an opaque tail
        // or an extension subclass. Only edits require represented field roles.
        output=raw;if(binary){output.hasRawValues=false;output.rawValues.clear();}
        name=original.m_name;return true;
    }
    // The reader also accepts one unmarked normal-map profile, when a
    // complete diffuse tuple and refraction prefix make the second group 42
    // unambiguous. The general canonical writer deliberately does not emit
    // markerless normal maps. For this captured, unchanged source only, allow
    // composed typed edits of both blends, projections, tilings, auto transforms,
    // file-backed diffuse/normal filenames and both mapper matrices without
    // synthesizing a marker or opening the general writer grammar.
    if(unchanged&&!crossCaret&&!original.m_hasLuminance&&!original.m_hasNormalMapStrength
        &&original.m_hasDiffuseMapBlendFactor&&original.m_hasNormalMapBlendFactor
        &&original.m_hasNormalMapSource&&original.m_hasNormalMapFileName
        &&original.m_hasNormalMapProjection&&original.m_hasNormalMapTiling
        &&original.m_hasNormalMapAutoTransform&&std::isfinite(typed.m_normalMapBlendFactor)) {
        const bool blendEdit=!dxfMaterial::sameValue(original.m_normalMapBlendFactor,typed.m_normalMapBlendFactor);
        const bool diffuseBlendEdit=!dxfMaterial::sameValue(original.m_diffuseMapBlendFactor,typed.m_diffuseMapBlendFactor);
        const bool diffuseProjectionEdit=original.m_diffuseMapProjection!=typed.m_diffuseMapProjection;
        const bool diffuseTilingEdit=original.m_diffuseMapTiling!=typed.m_diffuseMapTiling;
        const bool diffuseAutoEdit=original.m_diffuseMapAutoTransform!=typed.m_diffuseMapAutoTransform;
        const bool projectionEdit=original.m_normalMapProjection!=typed.m_normalMapProjection;
        const bool tilingEdit=original.m_normalMapTiling!=typed.m_normalMapTiling;
        const bool autoEdit=original.m_normalMapAutoTransform!=typed.m_normalMapAutoTransform;
        const bool filenameEdit=original.m_normalMapFileName!=typed.m_normalMapFileName;
        const bool diffuseFilenameEdit=original.m_diffuseMapFileName!=typed.m_diffuseMapFileName;
        const bool matrixEdit=!dxfMaterial::sameValue(original.m_normalMapTransform,typed.m_normalMapTransform);
        const bool diffuseMatrixEdit=!dxfMaterial::sameValue(original.m_diffuseMapTransform,typed.m_diffuseMapTransform);
        DRW_Material desired=original;
        if(blendEdit)desired.m_normalMapBlendFactor=typed.m_normalMapBlendFactor;
        if(diffuseBlendEdit)desired.m_diffuseMapBlendFactor=typed.m_diffuseMapBlendFactor;
        if(diffuseProjectionEdit)desired.m_diffuseMapProjection=typed.m_diffuseMapProjection;
        if(diffuseTilingEdit)desired.m_diffuseMapTiling=typed.m_diffuseMapTiling;
        if(diffuseAutoEdit)desired.m_diffuseMapAutoTransform=typed.m_diffuseMapAutoTransform;
        if(projectionEdit)desired.m_normalMapProjection=typed.m_normalMapProjection;
        if(tilingEdit)desired.m_normalMapTiling=typed.m_normalMapTiling;
        if(autoEdit)desired.m_normalMapAutoTransform=typed.m_normalMapAutoTransform;
        if(filenameEdit)desired.m_normalMapFileName=typed.m_normalMapFileName;
        if(diffuseFilenameEdit)desired.m_diffuseMapFileName=typed.m_diffuseMapFileName;
        if(matrixEdit)desired.m_normalMapTransform=typed.m_normalMapTransform;
        if(diffuseMatrixEdit)desired.m_diffuseMapTransform=typed.m_diffuseMapTransform;
        if(!(blendEdit||projectionEdit||tilingEdit||autoEdit||filenameEdit||diffuseFilenameEdit||matrixEdit||diffuseMatrixEdit||diffuseBlendEdit||diffuseProjectionEdit||diffuseTilingEdit||diffuseAutoEdit)
            || (diffuseBlendEdit&&!std::isfinite(typed.m_diffuseMapBlendFactor))
            || (diffuseProjectionEdit&&(typed.m_diffuseMapProjection<1u||typed.m_diffuseMapProjection>4u))
            || (diffuseTilingEdit&&(typed.m_diffuseMapTiling<1u||typed.m_diffuseMapTiling>3u))
            || (diffuseAutoEdit&&(typed.m_diffuseMapAutoTransform & ~7u))
            || (projectionEdit&&(typed.m_normalMapProjection<1u||typed.m_normalMapProjection>4u))
            || (tilingEdit&&(typed.m_normalMapTiling<1u||typed.m_normalMapTiling>3u))
            || (autoEdit&&(typed.m_normalMapAutoTransform & ~7u))
            || (diffuseFilenameEdit&&(!original.m_hasDiffuseMapSource||!original.m_hasDiffuseMapFileName
                || original.m_diffuseMapSource!=1u || typed.m_diffuseMapSource!=1u))
            || (filenameEdit&&(!original.m_hasDiffuseMapFileName
                || original.m_normalMapSource!=1u || typed.m_normalMapSource!=1u))
            || ((matrixEdit||diffuseMatrixEdit)&&(!original.m_hasDiffuseMapTransform||!original.m_hasNormalMapTransform
                ||!std::all_of(typed.m_normalMapTransform.begin(),typed.m_normalMapTransform.end(),
                    [](double value){return std::isfinite(value);})
                ||!std::all_of(typed.m_diffuseMapTransform.begin(),typed.m_diffuseMapTransform.end(),
                    [](double value){return std::isfinite(value);}))))
            return false;
        if(dxfMaterial::sameState(desired,typed)) {
            bool body=false,xdata=false,ok=true;unsigned depth=0u,markers=0u;
            std::array<std::size_t,2> blends{};std::size_t count=0u;
            std::array<std::array<std::size_t,2>,3> maps{};
            std::array<std::size_t,3> mapCount{};
            std::array<std::size_t,2> filenames{};std::size_t filenameCount=0u;
            std::array<std::size_t,32> matrixSlots{};std::size_t matrixCount=0u;
            for(std::size_t i=0u;i<raw.groups.size()&&ok;++i) {
                const auto& field=raw.groups[i];const int code=field.code();
                if(code==102) {
                    if(field.type()!=DRW_Variant::STRING||!field.content.s){ok=false;break;}
                    const auto& marker=*field.content.s;
                    if(marker=="}"){if(!depth){ok=false;break;}--depth;}
                    else if(!marker.empty()&&marker.front()=='{')++depth;
                    else ok=false;
                    continue;
                }
                if(depth)continue;
                if(code>=1000){xdata=true;continue;}
                if(xdata){ok=false;break;}
                if(code==100) {
                    if(field.type()!=DRW_Variant::STRING||!field.content.s){ok=false;break;}
                    if(*field.content.s=="AcDbMaterial"){if(++markers!=1u){ok=false;break;}body=true;}
                    else if(body){ok=false;break;}
                    continue;
                }
                if(!body)continue;
                // No procedural-table source profile has been qualified for
                // this edit. Reject both its delimiters and its exclusively
                // procedural value codes even when a malformed producer
                // omitted the table's opening or closing marker. The reused
                // group 271 remains scoped by the owning MATERIAL decoder.
                if(code==464||code==465||code==300||code==291||code==301
                    ||code==469||code==292||code==62||code==420||code==430){ok=false;break;}
                if(code==42){if(count==blends.size()||field.type()!=DRW_Variant::DOUBLE){ok=false;break;}blends[count++]=i;}
                if((matrixEdit||diffuseMatrixEdit)&&code==43) {
                    if(matrixCount==matrixSlots.size()||field.type()!=DRW_Variant::DOUBLE){ok=false;break;}
                    matrixSlots[matrixCount++]=i;
                }
                if((filenameEdit||diffuseFilenameEdit)&&code==3) {
                    if(filenameCount==filenames.size()||field.type()!=DRW_Variant::STRING||!field.content.s){ok=false;break;}
                    filenames[filenameCount++]=i;
                }
                if(((projectionEdit||diffuseProjectionEdit)&&code==73)||((tilingEdit||diffuseTilingEdit)&&code==74)||((autoEdit||diffuseAutoEdit)&&code==75)) {
                    const std::size_t role=code==73?0u:code==74?1u:2u;
                    if(mapCount[role]==maps[role].size()||field.type()!=DRW_Variant::INTEGER){ok=false;break;}
                    maps[role][mapCount[role]++]=i;
                }
            }
            const auto bound=[&](std::size_t role,bool active,bool present,int diffuse,int normal) {
                return !active || (mapCount[role]==2u&&present&&maps[role][0]<blends[1]&&maps[role][1]>blends[1]
                    &&raw.groups[maps[role][0]].i_val()==diffuse&&raw.groups[maps[role][1]].i_val()==normal);
            };
            if(ok&&!depth&&markers==1u&&count==2u
                &&dxfMaterial::sameValue(raw.groups[blends[0]].d_val(),original.m_diffuseMapBlendFactor)
                &&dxfMaterial::sameValue(raw.groups[blends[1]].d_val(),original.m_normalMapBlendFactor)
                &&bound(0u,projectionEdit||diffuseProjectionEdit,original.m_hasDiffuseMapProjection,original.m_diffuseMapProjection,original.m_normalMapProjection)
                &&bound(1u,tilingEdit||diffuseTilingEdit,original.m_hasDiffuseMapTiling,original.m_diffuseMapTiling,original.m_normalMapTiling)
                &&bound(2u,autoEdit||diffuseAutoEdit,original.m_hasDiffuseMapAutoTransform,original.m_diffuseMapAutoTransform,original.m_normalMapAutoTransform)
                &&(!(filenameEdit||diffuseFilenameEdit) || (filenameCount==2u && filenames[0]<blends[1] && filenames[1]>blends[1]))
                &&(!(matrixEdit||diffuseMatrixEdit) || (matrixCount==32u && matrixSlots[15]<blends[1] && matrixSlots[16]>blends[1]))) {
                output=raw;
                const auto replace=[&](std::size_t index,DRW_Variant field) {
                    output.groups[index]=std::move(field);
                    if(output.hasRawValues&&!binary) {
                        if(output.rawValues.size()!=output.groups.size())return false;
                        std::string text;if(!spelling(output.groups[index],text))return false;
                        output.rawValues[index]=std::move(text);
                    }
                    return true;
                };
                if(matrixEdit||diffuseMatrixEdit) {
                    for(std::size_t i=0u;i<16u;++i) {
                        if(!dxfMaterial::sameValue(raw.groups[matrixSlots[i]].d_val(),original.m_diffuseMapTransform[i])
                            ||!dxfMaterial::sameValue(raw.groups[matrixSlots[16u+i]].d_val(),original.m_normalMapTransform[i])) return false;
                        if(!dxfMaterial::sameValue(original.m_diffuseMapTransform[i],typed.m_diffuseMapTransform[i])
                            &&!replace(matrixSlots[i],DRW_Variant(43,typed.m_diffuseMapTransform[i])))return false;
                        if(!dxfMaterial::sameValue(original.m_normalMapTransform[i],typed.m_normalMapTransform[i])
                            &&!replace(matrixSlots[16u+i],DRW_Variant(43,typed.m_normalMapTransform[i])))return false;
                    }
                }
                if(diffuseFilenameEdit) {
                    BodyWriter filenameWriter(binary,baseline.m_version);
                    if(!filenameWriter.writeUtf8String(3,typed.m_diffuseMapFileName)
                        ||filenameWriter.hasWriteError()||filenameWriter.groups.size()!=1u
                        ||!replace(filenames[0],std::move(filenameWriter.groups.front()))) return false;
                }
                if(filenameEdit) {
                    BodyWriter filenameWriter(binary,baseline.m_version);
                    if(!filenameWriter.writeUtf8String(3,typed.m_normalMapFileName)
                        ||filenameWriter.hasWriteError()||filenameWriter.groups.size()!=1u
                        ||!replace(filenames[1],std::move(filenameWriter.groups.front()))) return false;
                }
                if((diffuseBlendEdit&&!replace(blends[0],DRW_Variant(42,typed.m_diffuseMapBlendFactor)))
                    ||(diffuseProjectionEdit&&!replace(maps[0][0],DRW_Variant(73,static_cast<std::int32_t>(typed.m_diffuseMapProjection))))
                    ||(diffuseTilingEdit&&!replace(maps[1][0],DRW_Variant(74,static_cast<std::int32_t>(typed.m_diffuseMapTiling))))
                    ||(diffuseAutoEdit&&!replace(maps[2][0],DRW_Variant(75,static_cast<std::int32_t>(typed.m_diffuseMapAutoTransform))))
                    ||(blendEdit&&!replace(blends[1],DRW_Variant(42,typed.m_normalMapBlendFactor)))
                    ||(projectionEdit&&!replace(maps[0][1],DRW_Variant(73,static_cast<std::int32_t>(typed.m_normalMapProjection))))
                    ||(tilingEdit&&!replace(maps[1][1],DRW_Variant(74,static_cast<std::int32_t>(typed.m_normalMapTiling))))
                    ||(autoEdit&&!replace(maps[2][1],DRW_Variant(75,static_cast<std::int32_t>(typed.m_normalMapAutoTransform))))) return false;
                if(binary){output.hasRawValues=false;output.rawValues.clear();}
                DRW_Material actual;
                if(!decode(output,actual,binary)||!dxfMaterial::sameState(actual,desired))return false;
                name=actual.m_name;return true;
            }
        }
    }
    Groups oldBody,typedBody,rawBody;Slots oldFields,typedFields,rawFields,physical;
    std::size_t bodyEnd=0,procedure=raw.groups.size();
    if(!canonical(original,oldBody,oldFields,binary)||!canonical(typed,typedBody,typedFields,binary)
        ||!canonical(parsed,rawBody,rawFields,binary)||!slots(raw.groups,physical,bodyEnd,&procedure))return false;
    std::set<int> edits,typedEdits;
    for(int key:order()) {
        const bool typedEdit=!fieldEqual(oldBody,oldFields,typedBody,typedFields,key);
        const bool rawEdit=!fieldEqual(oldBody,oldFields,rawBody,rawFields,key);
        if(typedEdit&&rawEdit&&!fieldEqual(typedBody,typedFields,rawBody,rawFields,key))return false;
        if(typedEdit&&!fieldEqual(typedBody,typedFields,rawBody,rawFields,key)){edits.insert(key);typedEdits.insert(key);}
    }
    const auto present=[&](int key){return (typedEdits.count(key)?typedFields:rawFields).count(key)!=0u;};
    const bool strengthFirst=!present(1464)&&present(1465);
    bool relocateStrength=false,keepStrengthSpelling=false;
    if(strengthFirst) {
        const auto strength=physical.find(1465);
        if(strength!=physical.end())for(int key:{1270,1271}) {
            const auto field=physical.find(key);
            if(field!=physical.end()&&field->second.front()<strength->second.front())relocateStrength=true;
        }
        if(relocateStrength) {
            if(strength->second.size()!=1u)return false;
            keepStrengthSpelling=edits.count(1465)==0u;
            edits.insert(1465);
        }
    }
    // Caret escapes are ASCII-only. Known text fields must be transcoded when
    // changing wire format, even without a typed edit. Other opaque bytes stay
    // raw; their semantic transcoding is not certified by this transaction.
    if(sourceBinary!=binary)for(const auto& field:physical) {
        bool caret=false;for(auto index:field.second){const auto& value=raw.groups[index];
            if(value.type()==DRW_Variant::STRING&&value.content.s&&value.content.s->find('^')!=std::string::npos)caret=true;}
        if(!caret||typedEdits.count(field.first))continue;
        const auto desired=rawFields.find(field.first);
        if(field.second.size()!=1u||desired==rawFields.end()||desired->second.size()!=1u)return false;
        if(!equal(raw.groups[field.second.front()],rawBody[desired->second.front()]))edits.insert(field.first);
    }
    output=raw;
    const bool keepSpellings=raw.hasRawValues&&!binary;
    if(!keepSpellings){output.hasRawValues=false;output.rawValues.clear();}
    if(!edits.empty()) {
        std::map<std::size_t,std::vector<int>> insertions;
        std::set<std::size_t> removed;
        for(int key:edits) {
            const auto found=physical.find(key);std::size_t insertion=bodyEnd;
            if(found!=physical.end()) {
                insertion=found->second.front();removed.insert(found->second.begin(),found->second.end());
                if(key==1465&&relocateStrength)for(int boundary:{1464,1270,1271}) {
                    const auto field=physical.find(boundary);
                    if(field!=physical.end())insertion=std::min(insertion,field->second.front());
                }
            } else {
                for(const auto& field:physical)if(rank(field.first,strengthFirst)>rank(key,strengthFirst))insertion=std::min(insertion,field.second.front());
                // Insert pre-procedure fields before its opaque table too.
                if(rank(key)<rank(2292))insertion=std::min(insertion,procedure);
            }
            insertions[insertion].push_back(key);
        }
        Groups changed;changed.reserve(raw.groups.size()+typedBody.size());
        std::vector<std::string> spellings;
        if(keepSpellings){if(raw.rawValues.size()!=raw.groups.size())return false;spellings.reserve(changed.capacity());}
        for(std::size_t i=0;i<=raw.groups.size();++i) {
            auto found=insertions.find(i);
            if(found!=insertions.end()) {
                auto& keys=found->second;std::sort(keys.begin(),keys.end(),[&](int a,int b){return rank(a,strengthFirst)<rank(b,strengthFirst);});
                for(int key:keys){
                    // Boundary relocation is a layout edit. Keep the caller's
                    // actual value and lexical spelling if its value is unchanged.
                    if(key==1465&&keepStrengthSpelling) {
                        const auto index=physical.find(key)->second.front();changed.push_back(raw.groups[index]);
                        if(keepSpellings)spellings.push_back(raw.rawValues[index]);continue;
                    }
                    const auto& fields=typedEdits.count(key)?typedFields:rawFields;
                    const auto& values=typedEdits.count(key)?typedBody:rawBody;
                    const auto foundField=fields.find(key);if(foundField!=fields.end())for(auto index:foundField->second){
                    changed.push_back(values[index]);if(keepSpellings){
                        std::string text;if(!spelling(values[index],text))return false;spellings.push_back(std::move(text));
                    }
                }}
            }
            if(i<raw.groups.size()&&!removed.count(i)){changed.push_back(raw.groups[i]);if(keepSpellings)spellings.push_back(raw.rawValues[i]);}
        }
        // Retain every untouched lexical value, including opaque procedural
        // values; synthesize spelling only for the slots actually changed.
        output.groups=std::move(changed);output.rawValues=std::move(spellings);
    }
    DRW_Material actual;Groups actualBody;Slots actualFields;
    if(!decode(output,actual,binary)||!canonical(actual,actualBody,actualFields,binary))return false;
    for(int key:order()) {
        const bool edit=typedEdits.count(key)!=0u;
        if(!fieldEqual(actualBody,actualFields,edit?typedBody:rawBody,edit?typedFields:rawFields,key))return false;
    }
    name=actual.m_name;return true;
}
} // namespace dx_material_body
#endif
