// SPDX-License-Identifier: GPL-2.0-or-later
// Internal implementation included once by dx_iface.cpp, after the original
// provider audits. Original reference targets are not recursive/live/output proof.
namespace {
using DependencyRole=dx_tableResourceRole;
using DependencyFamily=dx_tableResourceFamily;
unsigned dependencyNameFamily(DependencyFamily f) noexcept {
    using F=DependencyFamily;
    return f==F::TextStyle?0u:f==F::LineType?1u:f==F::Layer?2u:f==F::AppId?3u:f==F::BlockRecord?4u:5u;
}
DependencyFamily dependencyPhysicalFamily(std::string_view name) noexcept {
    using F=DependencyFamily;
    if(asciiIdentifierEquals(name,"STYLE"))return F::TextStyle;
    if(asciiIdentifierEquals(name,"LTYPE"))return F::LineType;
    if(asciiIdentifierEquals(name,"LAYER"))return F::Layer;
    if(asciiIdentifierEquals(name,"APPID"))return F::AppId;
    if(asciiIdentifierEquals(name,"BLOCK_RECORD"))return F::BlockRecord;
    return F::Entity;
}
bool dependencyNameSpelling(std::string_view a,std::string_view b) noexcept {
    if(a.size()!=b.size())return false;
    const auto fold=[](unsigned char c){return c>='A'&&c<='Z'?c+('a'-'A'):c;};
    for(std::size_t i=0u;i<a.size();++i)if(fold(static_cast<unsigned char>(a[i]))!=fold(static_cast<unsigned char>(b[i])))return false;
    return true;
}
template<class N> const auto& dependencyGroups(const N& n) {
    if(n.resource)return n.resource->input->raw->groups;
    if(n.structural)return n.structural->input->raw->groups;
    if(n.plot)return n.plot->input->raw->groups;
    const auto& t=*n.table;const auto at=t.first+n.tableRecord;
    return t.source->inBlock?t.source->blockPhysical->records.at(at).groups:t.entitiesPhysical->records.at(at).groups;
}
template<class N> const auto& dependencyContext(const N& n) {
    return n.resource?n.resource->input->context:n.structural?n.structural->input->context:n.plot?n.plot->input->context:n.table->source->context;
}
template<class N> const DRW_TableEntry* dependencyModel(const N& n) {
    return n.resource?n.resource->input->model.get():n.structural?n.structural->input->model.get():n.plot?n.plot->input->model.get():nullptr;
}
template<class C> bool dependencyNameKey(std::string_view wire,const C& c,std::string& key,std::string* decoded=nullptr) {
    if(wire.empty()||wire.size()>512u*1024u||wire.find('\0')!=std::string_view::npos||!c.captured)return false;
    DRW_TextCodec codec;codec.setVersion(c.codecVersion,true);codec.setCodePage(c.codecCodePage,true);
    codec.setDxfTextEncoding(c.textEncoding);codec.setDxfCodePagePolicy(c.codePagePolicy);
    auto name=codec.toUtf8(wire);
    if(codec.lastDecodeObservation().issues||name.empty()||name.find('\0')!=std::string::npos||!dx_symbol_output::nameKey(name,key))return false;
    if(decoded)*decoded=std::move(name);return true;
}
DependencyRole dependencyEntityRole(dx_attdef_source::Role role) noexcept {
    using E=dx_attdef_source::Role;using R=DependencyRole;
    switch(role) {
    case E::Self:return R::Self;case E::Owner:return R::Owner;
    case E::LayerName:case E::XdataLayerName:return R::LayerName;
    case E::LineTypeName:return R::LineTypeName;case E::TextStyleName:return R::TextStyleName;
    case E::Material:return R::Material;case E::FullVisualStyle:return R::VisualStyle;case E::PlotStyle:return R::PlotStyle;
    case E::Reactor:return R::Reactor;case E::ExtensionDictionary:return R::ExtensionDictionary;
    case E::AppIdName:return R::AppIdName;case E::XdataObjectId:return R::XdataObjectId;
    case E::BlockName:case E::RedundantBlockName:return R::BlockName;case E::LayoutName:return R::LayoutName;
    case E::TableStyle:return R::TableStyle;case E::TableGeometryBlock:return R::TableGeometryBlock;
    case E::TableCellBlock:return R::TableCellBlock;case E::TableCellField:return R::TableCellField;
    case E::TableCellAttributeDefinition:return R::TableCellAttributeDefinition;case E::TableValueObject:return R::TableValueObject;
    default:return R::Unknown;
    }
}
bool dependencyHardOwner(DependencyRole role) noexcept {
    using R=DependencyRole;return role==R::ExtensionDictionary||role==R::FieldChild||role==R::DataLinkHardOwner
        ||role==R::DictionaryHardOwner||role==R::XRecordHardOwner;
}
struct DependencyDictionaryFlag {
    bool known{false},hard{false};std::size_t group{DRW_DxfIdentitySource::None};
};
// Use the original subclass/body flag, never an application-group lookalike
// or the model's normalized omitted-field default. No scan allocation/state.
template<class N> DependencyDictionaryFlag dependencyDictionaryFlag(const N& n) noexcept {
    DependencyDictionaryFlag result;
    if(n.family!=DependencyFamily::Dictionary)return result;
    const auto* model=dynamic_cast<const DRW_Dictionary*>(dependencyModel(n));if(!model)return result;
    unsigned application=0u,stage=0u,seen=0u;bool xdata=false;
    const auto& groups=dependencyGroups(n);
    for(std::size_t i=0u;i<groups.size();++i) {
        const auto& value=groups[i];const int code=value.code();const auto text=dx_attdef_source::text(value);
        if(code==102){if(text=="}"){if(!application)return {};--application;}else ++application;continue;}
        if(application)continue;if(code==1001)xdata=true;if(xdata)continue;
        if(code==100){if(text=="AcDbDictionary")stage=1u;else if(stage)stage=2u;continue;}
        if(code!=280)continue;
        if(stage!=1u||seen++||value.type()!=DRW_Variant::INTEGER||(value.i_val()!=0&&value.i_val()!=1)||value.i_val()!=model->hardOwner)return {};
        result.known=true;result.hard=value.i_val()==1;result.group=i;
    }
    if(application)return {};return result;
}
template<class O> bool dependencyEffectiveHardOwner(O ownership) noexcept {
    return ownership==O::PhysicalHard||ownership==O::DictionaryFlagHard;
}
bool dependencyDispositionMatched(dx_iface::DependencyReason reason) noexcept {
    using R=dx_iface::DependencyReason;return reason==R::Matched||reason==R::NullOptional||reason==R::Inherited
        ||reason==R::Literal||reason==R::OriginalTableOwner;
}
bool dependencyReferencesEqual(const std::vector<dx_tableResourceReference>& a,const std::vector<dx_tableResourceReference>& b) noexcept {
    if(a.size()!=b.size())return false;
    for(std::size_t i=0;i<a.size();++i)if(a[i].kind!=b[i].kind||a[i].groupIndex!=b[i].groupIndex||a[i].code!=b[i].code||a[i].value!=b[i].value)return false;
    return true;
}
}

void dx_iface::finishDependencies(dx_data& data) noexcept {
    using A=dx_data::TableDxfSourceCapture::DependencyAudit;using S=A::State;using R=A::Reason;
    using F=DependencyFamily;using Role=DependencyRole;using I=DRW_DxfIdentitySource;using O=A::Ownership;
    auto& s=data.m_tableDxfSource;if(s.dependencyCaptureFailed)return;
    try {
        const auto prereq=tableReferenceReport(data);
        if(!s.tableReferenceAudit||!s.identityInput||!s.identityAudit)return;
        const auto& identity=*s.identityInput->source;
        std::size_t groups=s.resourceProofGroups,bytes=sizeof(A),work=0u,capacity=0u;
        for(const auto& p:s.structuralAudits)if(p.second&&p.second->input&&p.second->input->raw)
            if(!addAssociativeRetainedBytes(p.second->input->raw->groups.size(),groups)){s.dependencyCaptureFailed=true;return;}
        if(!addAssociativeRetainedBytes(s.plotSourceGroups,groups)){s.dependencyCaptureFailed=true;return;}
        for(const auto& t:s.tableBodies)if(t.second)for(const auto& p:t.second->profiles)
            if(!addAssociativeRetainedBytes(p.spans.size(),groups)){s.dependencyCaptureFailed=true;return;}
        capacity=groups;
        if(!addAssociativeSlotBytes(identity.records.size(),3u,capacity)||capacity>s.maxDependencyRows
            ||!addAssociativeSlotBytes(identity.records.size(),sizeof(A::Node)+256u,bytes)
            ||!addAssociativeSlotBytes(capacity,sizeof(A::Row)+2u*sizeof(dx_tableResourceReference)+64u,bytes)
            ||!addAssociativeSlotBytes(s.resourceProofBytes,16u,bytes)||!addAssociativeSlotBytes(s.resourceBytes,4u,bytes)
            ||!addAssociativeSlotBytes(s.plotSourceBytes,4u,bytes)
            ||!addAssociativeSlotBytes(s.tableBodyBytes,4u,bytes)||bytes>s.maxDependencyBytes
            ||!addAssociativeRetainedBytes(bytes,work)||!addAssociativeSlotBytes(groups,128u,work)
            ||!addAssociativeSlotBytes(identity.records.size(),256u,work)||work>s.maxDependencyWork){s.dependencyCaptureFailed=true;return;}
        s.dependencyBytes=bytes;s.dependencyWork=work;s.dependencyRowsReserved=capacity;
        auto a=std::make_shared<A>();a->container=&data;a->generation=s.generation;a->containerId=data.m_entitiesContainerIdentity.value;a->targets=s.tableReferenceAudit;
        const auto save=[&](S state){a->state=state;s.dependencyAudit=std::move(a);s.dependencyOwner=s.dependencyAudit;};
        if(prereq.state==TableReferenceState::ContextMismatch){save(S::ContextMismatch);return;}
        if(prereq.state==TableReferenceState::SourceNotReady||prereq.state==TableReferenceState::CaptureFailed
            ||identitySourceReport(data).state!=IdentitySourceState::NamespaceReady){save(S::SourceNotReady);return;}
        std::vector<std::uint32_t> ids(identity.records.size(),0u);
        for(const auto& p:s.identityAudit->identities)if(p.second<ids.size())ids[p.second]=p.first;
        for(const auto& p:s.resourceOriginAudits) {
            const auto& o=p.second;if(!o||!o->originBound||!o->bodyMatched||!o->stateComplete||!o->input||!o->input->paired)continue;
            const auto found=s.resources.find(p.first);if(found==s.resources.end())continue;
            A::Node n;n.handle=p.first;n.physical=o->physical;n.family=found->second.family;n.resource=o;n.references=found->second.references;
            a->nodes.emplace(p.first,std::move(n));
        }
        for(const auto& p:s.structuralAudits) {
            const auto& o=p.second;if(!o||!o->originBound||!o->bodyMatched||!o->stateComplete||!o->input||!o->input->paired)continue;
            A::Node n;n.handle=p.first;n.physical=o->physical;n.structural=o;
            n.family=dependencyPhysicalFamily(o->input->raw->name);
            if(n.family==F::BlockRecord){if(!blockRecordSourceReferences(*o->input->raw,n.references))continue;}
            else if(asciiIdentifierEquals(o->input->raw->name,"DICTIONARY")){n.family=F::Dictionary;if(!spineSourceReferences(*o->input->raw,true,n.references))continue;}
            else continue;
            a->nodes.emplace(p.first,std::move(n));
        }
        if(s.tableReferenceAudit->plots)for(const auto& p:s.tableReferenceAudit->plots->records) {
            const auto& o=p.second;if(!o||!o->bodyMatched||!o->originBound)continue;
            A::Node n;n.handle=p.first;n.physical=o->physical;n.family=o->input->dictionary?F::Dictionary:F::Entity;n.plot=o;n.references=o->references;
            a->nodes.emplace(p.first,std::move(n));
        }
        for(const auto& p:s.tableBodies) {
            const auto& t=p.second;if(!t||!t->profilesMatched)continue;
            for(std::size_t record=0u;record<t->profiles.size();++record) {
                A::Node n;n.table=t;n.tableRecord=record;n.family=F::Table;
                for(const auto& span:t->profiles[record].spans)if(span.role!=dx_attdef_source::Role::None) {
                    const auto role=dependencyEntityRole(span.role);
                    n.references.push_back({role,span.groupIndex,span.code,static_cast<std::uint32_t>(span.handle)});
                    if(role==Role::Self)n.handle=static_cast<std::uint32_t>(span.handle);
                }
                const auto original=s.identityAudit->identities.find(n.handle);if(original==s.identityAudit->identities.end())continue;
                n.physical=original->second;a->nodes.emplace(n.handle,std::move(n));
            }
        }
        // Census every physical row, including dropped/unqualified producers.
        // An unobserved competitor cannot disappear from a successful name lookup.
        for(std::size_t i=0u;i<identity.records.size();++i) {
            const auto& p=identity.records[i];const auto family=dependencyPhysicalFamily(p.name);const auto named=dependencyNameFamily(family);
            const bool symbol=p.kind==I::Kind::TableRecord&&named<5u;
            const bool object=p.kind==I::Kind::Object&&(asciiIdentifierEquals(p.name,"DICTIONARY")||plotSourceDictionaryName(p.name)||asciiIdentifierEquals(p.name,"ACDBPLACEHOLDER")||resourceOriginName(dxfRW::DxfResourceSourceView::Kind::TableStyle,p.name)
                ||resourceOriginName(dxfRW::DxfResourceSourceView::Kind::Field,p.name)||resourceOriginName(dxfRW::DxfResourceSourceView::Kind::XRecord,p.name)||resourceOriginName(dxfRW::DxfResourceSourceView::Kind::DataLink,p.name));
            const bool table=p.kind==I::Kind::Entity&&(asciiIdentifierEquals(p.name,"ACAD_TABLE")||asciiIdentifierEquals(p.name,"TABLE"));
            if(symbol)++a->physicalNames[named];
            if(!symbol&&!object&&!table)continue;
            const auto node=a->nodes.find(ids[i]);if(node==a->nodes.end()||node->second.physical!=i){++a->unavailableSources;continue;}
            if(!symbol)continue;
            const auto& n=node->second;const auto* model=dependencyModel(n);if(!model||!dependencyContext(n))continue;
            unsigned depth=0u,names=0u;bool xdata=false,emptyNames=true;std::string key,decoded;bool valid=true;
            for(const auto& v:dependencyGroups(n)) {
                const auto text=dx_attdef_source::text(v);
                if(v.code()==102){if(text=="}"){if(!depth){valid=false;break;}--depth;}else ++depth;continue;}
                if(depth)continue;if(v.code()==1001)xdata=true;
                if(v.code()==2&&!xdata){++names;emptyNames=emptyNames&&text.empty();valid=valid&&dependencyNameKey(text,*dependencyContext(n),key,&decoded);}
            }
            const auto* style=dynamic_cast<const DRW_Textstyle*>(model);
            if(style&&(style->flags&1)&&model->name.empty()&&(names==0u||(names==1u&&emptyNames))){++a->backedNames[named];continue;}
            if(!valid||names!=1u||decoded!=model->name)continue;
            ++a->backedNames[named];const auto inserted=a->names.emplace(std::make_pair(family,std::move(key)),A::NamedTarget{n.handle,false});
            if(!inserted.second)inserted.first->second.ambiguous=true;
        }
        for(unsigned i=0u;i<5u;++i)a->nameCensusComplete[i]=a->physicalNames[i]==a->backedNames[i];
        std::map<std::uint32_t,std::uint32_t> layouts;
        if(a->targets->entities)for(const auto& row:a->targets->entities->rows)
            if(row.reason==dx_data::TableDxfSourceCapture::EntitiesRoleAudit::Reason::Matched)layouts.emplace(row.self,row.layout);
        if(a->targets->blocks)for(const auto& row:a->targets->blocks->rows)
            if(row.reason==dx_data::TableDxfSourceCapture::BlockRoleAudit::Reason::Matched)layouts.emplace(row.self,row.layout);
        std::map<std::pair<std::uint32_t,std::size_t>,TableReferenceReason> tableRoles;
        for(const auto& row:a->targets->rows)tableRoles.emplace(std::make_pair(row.table,row.group),row.reason);
        a->rows.reserve(capacity);
        for(const auto& item:a->nodes) {
            const auto& n=item.second;const auto& groups=dependencyGroups(n);const auto& context=dependencyContext(n);
            const auto firstRow=a->rows.size();
            const auto ownershipFlag=dependencyDictionaryFlag(n);
            for(const auto& ref:n.references) {
                if(ref.kind==Role::Self)continue;
                if(a->rows.size()>=capacity||ref.groupIndex>=groups.size()){s.dependencyCaptureFailed=true;return;}
                A::Row row;row.source=n.handle;row.target=ref.value;row.group=ref.groupIndex;row.code=ref.code;row.role=ref.kind;
                if(dependencyHardOwner(ref.kind))row.ownership=O::PhysicalHard;
                if(ref.kind==Role::DictionarySoftOwner||ref.kind==Role::DictionaryHardOwner) {
                    // Keep the physical edge role/code; 280 may strengthen a
                    // 350 edge without changing it into a source360 field.
                    row.dictionaryFlagGroup=ownershipFlag.group;
                    if(ref.kind==Role::DictionarySoftOwner)row.ownership=ownershipFlag.known?(ownershipFlag.hard?O::DictionaryFlagHard:O::ExplicitSoft):O::Unqualified;
                }
                bool blocked=false;
                if(n.table&&!n.tableRecord) {
                    const auto old=tableRoles.find({n.handle,ref.groupIndex});
                    if(old!=tableRoles.end()&&old->second!=TableReferenceReason::Matched&&old->second!=TableReferenceReason::NullValue) {
                        blocked=true;row.reason=old->second==TableReferenceReason::WrongCellBlock?R::WrongOwner:
                            old->second==TableReferenceReason::WrongKind?R::WrongKind:old->second==TableReferenceReason::MissingTarget?R::MissingTarget:
                            old->second==TableReferenceReason::NullRequired?R::NullRequired:R::OriginalUnavailable;
                    }
                }
                F nameFamily=F::Entity;
                if(ref.kind==Role::AppIdName)nameFamily=F::AppId;else if(ref.kind==Role::LayerName)nameFamily=F::Layer;
                else if(ref.kind==Role::TextStyleName)nameFamily=F::TextStyle;else if(ref.kind==Role::LineTypeName)nameFamily=F::LineType;
                else if(ref.kind==Role::BlockName)nameFamily=F::BlockRecord;
                row.name=dependencyNameFamily(nameFamily)<5u||ref.kind==Role::LayoutName;
                if(ref.kind==Role::Unknown)row.reason=R::UnknownRole;
                else if(ref.kind==Role::XRecordHandle)row.reason=R::Literal;
                else if(row.name) {
                    std::string decoded;
                    if(!context||!dependencyNameKey(dx_attdef_source::text(groups[ref.groupIndex]),*context,row.key,&decoded))row.reason=R::InvalidName;
                    else if(ref.kind==Role::LineTypeName&&n.table&&(row.key=="bylayer"||row.key=="byblock"))row.reason=R::Inherited;
                    else if(ref.kind==Role::LayoutName) {const auto found=layouts.find(n.handle);if(found!=layouts.end())row.target=found->second;else row.reason=R::OriginalUnavailable;}
                    else if(!a->nameCensusComplete[dependencyNameFamily(nameFamily)])row.reason=R::NameSourceUnavailable;
                    else {const auto found=a->names.find({nameFamily,row.key});if(found==a->names.end())row.reason=R::MissingName;
                        else if(found->second.ambiguous)row.reason=R::AmbiguousName;
                        else {const auto candidate=a->nodes.find(found->second.handle);const auto* model=candidate==a->nodes.end()?nullptr:dependencyModel(candidate->second);
                            // The Unicode closure detects ambiguity; it is not
                            // evidence for native non-ASCII collation aliases.
                            if(!model||!dependencyNameSpelling(decoded,model->name))row.reason=R::NameSourceUnavailable;else row.target=found->second.handle;}}
                }
                const bool unresolvedName=row.name&&row.target==0u;
                if(!blocked&&row.reason==R::OriginalUnavailable&&!unresolvedName) {
                    if(!row.target) {
                        const bool required=ref.kind==Role::Owner||ref.kind==Role::FieldChild||ref.kind==Role::TableStyle||ref.kind==Role::TableGeometryBlock
                            ||ref.kind==Role::TableCellBlock||ref.kind==Role::TableCellField||ref.kind==Role::TableCellAttributeDefinition||ref.kind==Role::DictionaryDefault;
                        row.reason=required?R::NullRequired:R::NullOptional;
                        if(ref.kind==Role::Owner&&n.structural&&n.structural->input->kind==dxfRW::DxfStructuralSourceView::Kind::RootDictionary)row.reason=R::NullOptional;
                    } else {
                        const auto id=s.identityAudit->identities.find(row.target);
                        if(id==s.identityAudit->identities.end())row.reason=R::MissingTarget;
                        else {
                            const auto& p=identity.records[id->second];bool kind=true;
                            if(ref.kind==Role::Owner&&dependencyNameFamily(n.family)<5u) {
                                row.reason=p.kind==I::Kind::TableControl&&identity.records[n.physical].table==id->second?R::OriginalTableOwner:R::WrongOwner;
                            } else {
                                if(ref.kind==Role::ExtensionDictionary||ref.kind==Role::DictionarySoftOwner||ref.kind==Role::DictionaryHardOwner)
                                    kind=ref.kind!=Role::ExtensionDictionary||(p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"DICTIONARY"));
                                if(ref.kind==Role::TableStyle)kind=p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"TABLESTYLE");
                                if(ref.kind==Role::FieldChild||ref.kind==Role::TableCellField)kind=p.kind==I::Kind::Object&&resourceOriginName(dxfRW::DxfResourceSourceView::Kind::Field,p.name);
                                if(ref.kind==Role::TableGeometryBlock||ref.kind==Role::TableCellBlock||ref.kind==Role::BlockName)kind=p.kind==I::Kind::TableRecord&&asciiIdentifierEquals(p.name,"BLOCK_RECORD");
                                if(ref.kind==Role::TableCellAttributeDefinition)kind=p.kind==I::Kind::Entity&&asciiIdentifierEquals(p.name,"ATTDEF");
                                if(ref.kind==Role::LTypeShapeStyle||ref.kind==Role::LTypeTextStyle||ref.kind==Role::TextStyle)kind=p.kind==I::Kind::TableRecord&&asciiIdentifierEquals(p.name,"STYLE");
                                if(ref.kind==Role::LineType)kind=p.kind==I::Kind::TableRecord&&asciiIdentifierEquals(p.name,"LTYPE");
                                if(ref.kind==Role::BlockInsert)kind=p.kind==I::Kind::Entity&&(asciiIdentifierEquals(p.name,"INSERT")||asciiIdentifierEquals(p.name,"MINSERT"));
                                if(ref.kind==Role::BlockLayout||ref.kind==Role::LayoutName)kind=p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"LAYOUT");
                                if(ref.kind==Role::Material||ref.kind==Role::LayerMaterial)kind=p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"MATERIAL");
                                if(ref.kind==Role::PlotStyle||ref.kind==Role::LayerPlotStyle)kind=p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"ACDBPLACEHOLDER");
                                if(ref.kind==Role::VisualStyle)kind=p.kind==I::Kind::Object&&asciiIdentifierEquals(p.name,"VISUALSTYLE");
                                if(ref.kind==Role::DictionaryDefault)kind=p.kind==I::Kind::Object;
                                const auto proof=a->targets->targets.find(row.target);
                                row.reason=!kind?R::WrongKind:proof==a->targets->targets.end()||proof->second.ambiguous||proof->second.physical!=id->second?R::OriginalUnavailable:R::Matched;
                                if(row.reason==R::Matched&&(ref.kind==Role::PlotStyle||ref.kind==Role::LayerPlotStyle)
                                    &&(!proof->second.plot||!a->targets->plots||!a->targets->plots->plotTargets.count(row.target)))row.reason=R::OriginalUnavailable;
                                if(row.reason==R::Matched&&(ref.kind==Role::LTypeShapeStyle||ref.kind==Role::LTypeTextStyle||nameFamily==F::TextStyle)) {
                                    const auto target=a->nodes.find(row.target);const auto* style=target==a->nodes.end()?nullptr:dynamic_cast<const DRW_Textstyle*>(dependencyModel(target->second));
                                    if(!style||((style->flags&1)!=0)!=(ref.kind==Role::LTypeShapeStyle))row.reason=R::WrongKind;
                                }
                                if(row.reason==R::Matched&&row.ownership==O::Unqualified)row.reason=R::OwnershipUnknown;
                                if(row.reason==R::Matched&&dependencyEffectiveHardOwner(row.ownership)) {
                                    const auto target=a->nodes.find(row.target);const auto* model=target==a->nodes.end()?nullptr:dependencyModel(target->second);
                                    if(!model&&proof->second.layout&&proof->second.layout->input)model=proof->second.layout->input->model.get();
                                    const auto owner=model?model->parentHandle:target!=a->nodes.end()&&target->second.plot?target->second.plot->input->raw->parentHandle:DRW::NoHandle;
                                    if(owner!=n.handle)row.reason=R::WrongOwner;
                                }
                            }
                        }
                    }
                }
                a->rows.push_back(std::move(row));
            }
            if(n.table&&!n.tableRecord) {
                // AcDbBlockReference2 and AcDbTable343 describe the same
                // graphical BLOCK_RECORD, independently of entity owner330.
                // Join resolved original targets, never decoded/live defaults.
                A::Row* name=nullptr;A::Row* handle=nullptr;
                for(auto i=firstRow;i<a->rows.size();++i) {
                    auto& row=a->rows[i];if(row.role==Role::BlockName)name=&row;
                    if(row.role==Role::TableGeometryBlock)handle=&row;
                }
                const bool resolved=name&&handle&&name->reason==R::Matched&&handle->reason==R::Matched;
                const bool same=resolved&&name->target&&name->target==handle->target;
                const auto couple=[&](A::Row* row,const A::Row* other) {
                    if(!row)return;row->geometryCoupled=true;
                    if(other){row->coupledGroup=other->group;row->coupledTarget=other->target;}
                    // Keep an existing missing/wrong-kind/ambiguous refusal.
                    if(row->reason==R::Matched&&!same)row->reason=resolved?R::CoupledTargetMismatch:R::CoupledTargetUnavailable;
                };
                couple(name,handle);couple(handle,name);
            }
            if(n.table) {
                bool line=false,layer=false;for(const auto& ref:n.references){line=line||ref.kind==Role::LineTypeName;layer=layer||(ref.kind==Role::LayerName&&ref.code==8);}
                if(!line){A::Row row;row.source=n.handle;row.role=Role::LineTypeName;row.code=6;row.present=false;row.reason=R::Inherited;a->rows.push_back(std::move(row));}
                if(!layer){A::Row row;row.source=n.handle;row.role=Role::LayerName;row.code=8;row.present=false;row.reason=R::NullRequired;a->rows.push_back(std::move(row));}
            }else if(n.family==F::Layer) {
                bool line=false;for(const auto& ref:n.references)line=line||ref.kind==Role::LineTypeName;
                // LAYER6 has no documented omitted-field default. Retaining
                // the parser's CONTINUOUS default is not an original name use.
                if(!line){A::Row row;row.source=n.handle;row.role=Role::LineTypeName;row.code=6;row.present=false;row.reason=R::NullRequired;a->rows.push_back(std::move(row));}
            }
        }
        // Linear nonrecursive hard-ownership cycle check; soft pointer cycles
        // retain their original-target result and are not evaluated as geometry.
        std::map<std::uint32_t,std::size_t> vertices;for(const auto& p:a->nodes)vertices.emplace(p.first,vertices.size());
        std::vector<std::vector<std::size_t>> edges(vertices.size());std::vector<std::size_t> indegree(vertices.size(),0u),queue;
        for(const auto& row:a->rows)if(row.reason==R::Matched&&dependencyEffectiveHardOwner(row.ownership)) {
            const auto from=vertices.find(row.source),to=vertices.find(row.target);if(from!=vertices.end()&&to!=vertices.end()){edges[from->second].push_back(to->second);++indegree[to->second];}
        }
        queue.reserve(vertices.size());for(std::size_t i=0u;i<indegree.size();++i)if(!indegree[i])queue.push_back(i);
        for(std::size_t at=0u;at<queue.size();++at)for(const auto to:edges[queue[at]])if(--indegree[to]==0u)queue.push_back(to);
        for(auto& row:a->rows)if(row.reason==R::Matched&&dependencyEffectiveHardOwner(row.ownership)) {
            const auto to=vertices.find(row.target);if(to!=vertices.end()&&indegree[to->second])row.reason=R::OwnershipCycle;
        }
        bool complete=!a->unavailableSources;
        for(const auto value:a->nameCensusComplete)complete=complete&&value;
        for(const auto& row:a->rows)complete=complete&&dependencyDispositionMatched(row.reason);
        save(a->nodes.empty()&&!a->unavailableSources?S::NoSources:complete?S::OriginalTargetsMatched:S::Unqualified);
    }catch(...){s.dependencyCaptureFailed=true;}
}

dx_iface::DependencyReport dx_iface::dependencyReport(const dx_data& data) noexcept {
    using A=dx_data::TableDxfSourceCapture::DependencyAudit;using S=A::State;using R=A::Reason;
    const auto& s=data.m_tableDxfSource;DependencyReport r;r.bytes=s.dependencyBytes;r.work=s.dependencyWork;r.reservedRows=s.dependencyRowsReserved;
    if(!s.generation||data.sourceFormat!=dx_source_format::DXF)return r;
    if(!s.closed){r.state=S::Reading;return r;}if(!s.successful){r.state=S::ImportFailed;return r;}
    if(s.contextFailed||s.generation!=1u||s.generation!=data.importGeneration||s.reader||s.entitiesReadInput){r.state=S::ContextMismatch;return r;}
    if(s.dependencyCaptureFailed){r.state=S::CaptureFailed;return r;}const auto& a=s.dependencyAudit;if(!a)return r;
    const auto same=[](const auto& x,const auto& y){return x.get()==y.get()&&!x.owner_before(y)&&!y.owner_before(x);};
    if(!same(a,s.dependencyOwner)||a->container!=&data||a->generation!=s.generation||a->containerId!=data.m_entitiesContainerIdentity.value
        ||!same(a->targets,s.tableReferenceAudit)){r.state=S::ContextMismatch;return r;}
    const auto targets=tableReferenceReport(data);
    if(targets.state==TableReferenceState::ContextMismatch){r.state=S::ContextMismatch;return r;}
    if(targets.state==TableReferenceState::CaptureFailed||targets.state==TableReferenceState::SourceNotReady){r.state=S::SourceNotReady;return r;}
    const auto identity=identitySourceReport(data);const auto resources=resourceOriginReport(data);const auto structural=structuralSourceReport(data);
    if(identity.state==IdentitySourceState::ContextMismatch||resources.state==ResourceOriginState::ContextMismatch||structural.state==StructuralSourceState::ContextMismatch){r.state=S::ContextMismatch;return r;}
    if(identity.state!=IdentitySourceState::NamespaceReady||resources.state==ResourceOriginState::CaptureFailed||resources.state==ResourceOriginState::SourceNotReady
        ||structural.state==StructuralSourceState::CaptureFailed){r.state=S::SourceNotReady;return r;}
    for(const auto& p:a->nodes) {
        const auto& n=p.second;
        if(n.resource){const auto found=s.resourceOriginAudits.find(p.first);const auto body=s.resources.find(p.first);
            if(found==s.resourceOriginAudits.end()||!same(n.resource,found->second)||body==s.resources.end()||!dependencyReferencesEqual(n.references,body->second.references)){r.state=S::ContextMismatch;return r;}}
        if(n.structural){const auto found=s.structuralAudits.find(p.first);if(found==s.structuralAudits.end()||!same(n.structural,found->second)){r.state=S::ContextMismatch;return r;}}
        if(n.plot){if(!a->targets->plots){r.state=S::ContextMismatch;return r;}const auto found=a->targets->plots->records.find(p.first);
            if(found==a->targets->plots->records.end()||!same(n.plot,found->second)||!dependencyReferencesEqual(n.references,n.plot->references)){r.state=S::ContextMismatch;return r;}}
        if(n.table){const auto found=s.tableBodies.find(n.table->source->model->handle);if(found==s.tableBodies.end()||!same(n.table,found->second)){r.state=S::ContextMismatch;return r;}}
    }
    r.state=a->state;r.sources=a->nodes.size();r.names=a->names.size();r.references=a->rows.size();r.unavailableSources=a->unavailableSources;
    r.nameNamespacesComplete=true;
    for(unsigned i=0u;i<5u;++i){r.physicalNamedRows+=a->physicalNames[i];r.backedNamedRows+=a->backedNames[i];r.nameNamespacesComplete=r.nameNamespacesComplete&&a->nameCensusComplete[i];}
    for(const auto& row:a->rows) {
        if(row.geometryCoupled&&row.role==DependencyRole::BlockName) {
            ++r.geometryPairs;
            if(row.reason==R::Matched)++r.geometryMatched;
            else if(row.reason==R::CoupledTargetMismatch)++r.geometryMismatches;
            else ++r.geometryUnavailable;
        }
        if(dependencyEffectiveHardOwner(row.ownership))++r.hardOwnerReferences;
        if(row.ownership==A::Ownership::DictionaryFlagHard)++r.dictionaryFlagHardOwners;
        if(row.ownership==A::Ownership::ExplicitSoft)++r.explicitSoftOwners;
        if(dependencyDispositionMatched(row.reason))++r.matched;else if(!r.firstSource){r.firstSource=row.source;r.firstTarget=row.target;}
        switch(row.reason) {
        case R::Matched:break;case R::NullOptional:++r.nulls;break;case R::Inherited:++r.inherited;break;case R::Literal:++r.literals;break;case R::OriginalTableOwner:++r.tableOwners;break;
        case R::MissingTarget:case R::MissingName:++r.missing;break;case R::WrongKind:++r.wrongKind;break;
        case R::OriginalUnavailable:case R::NameSourceUnavailable:++r.unavailable;break;case R::AmbiguousName:++r.ambiguous;break;
        case R::InvalidName:++r.invalidNames;break;case R::UnknownRole:++r.unknownRoles;break;
        case R::NullRequired:case R::WrongOwner:++r.wrongOwner;break;case R::OwnershipCycle:++r.ownershipCycles;break;
        case R::OwnershipUnknown:++r.unknownOwnership;++r.unavailable;break;
        case R::CoupledTargetMismatch:break;
        case R::CoupledTargetUnavailable:++r.unavailable;break;
        }
    }
    r.originalTargetsMatched=r.state==S::OriginalTargetsMatched&&r.references&&r.references==r.matched;return r;
}
