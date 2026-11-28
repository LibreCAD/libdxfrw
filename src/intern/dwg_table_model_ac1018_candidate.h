/* Bounded deep-snapshot eligibility for one parsed AC1018 TABLE profile.
 * This admits source observation only; it cannot authorize DWG/DXF output.
 */
#ifndef DWG_TABLE_MODEL_AC1018_CANDIDATE_H
#define DWG_TABLE_MODEL_AC1018_CANDIDATE_H
#include "drw_entities.h"
#include "dxfattdef.h"
#include "dxfentitiesauth.h"
#include "dxftable.h"
#include <cstddef>
#include <cstring>
#include <tuple>
#include <typeinfo>
namespace DwgTableModelAc1018Candidate {
// A conservative fixed charge covers container/control-block overhead and all
// individually bounded allocations below. It is charged before any deep copy.
constexpr std::size_t ModelReservation=256u*1024u;
inline bool bounded(const DRW_Table& t) noexcept {
    const auto small=[](const std::string& s){return s.size()<=256u;};
    const auto color=[&](const DRW_DwgCmColor& c){return small(c.colorName)&&small(c.bookName);};
    if(typeid(t)!=typeid(DRW_Table) || t.eType!=DRW::TABLE || !t.handle
        || !t.m_hasSemanticContent || !t.m_semanticContentComplete
        || t.proxyGraphics.size()!=2104u || t.numProxyGraph!=2104
        || !small(t.name) || !small(t.layer) || !small(t.lineType)
        || !small(t.colorName) || !small(t.proxyGraphicsProvenance().sourceCodePage) || !t.extData.empty() || !t.appData.empty()
        || !t.reactorHandles.empty() || t.hasDataStorageRecord || t.hasDataStorageBinaryData()
        || !t.dataStorageHandleKey.empty() || !t.dataStorageData.empty() || !t.dataStoragePayloadMarkerSection.empty()
        || !t.pendingAppIdResolutions.empty() || !t.pendingLayerRefResolutions.empty()
        || !dxfAttdef::Decoder::hasNoDxfAccountingLease(t) || !dxfAttdef::Decoder::hasNoEntityNativeWriteBindings(t)
        || !t.attlist.empty() || t.seqEndEntity || !t.attribHandles.empty()
        || t.m_content.m_columns.size()!=3u || t.m_content.m_rows.size()!=3u
        || t.m_content.m_mergedRanges.size()>9u || t.m_content.m_subrecordRanges.size()>32u
        || !t.m_content.m_fieldHandles.empty() || !small(t.m_content.m_name) || !small(t.m_content.m_description))return false;
    for(const auto& name:t.m_tableRowTextStyleNames)if(!small(name))return false;
    for(const auto& value:t.m_tableRowTextColors)if(!color(value))return false;
    for(const auto& value:t.m_tableRowFillColors)if(!color(value))return false;
    for(const auto& value:t.m_tableBorderColors)if(!color(value))return false;
    for(const auto& column:t.m_content.m_columns)if(!small(column.m_name))return false;
    for(const auto& range:t.m_content.m_subrecordRanges)if(!small(range.m_name))return false;
    for(const auto& row:t.m_content.m_rows) {
        if(row.m_cells.size()!=3u)return false;
        for(const auto& cell:row.m_cells) {
            if(!small(cell.m_toolTip) || !small(cell.m_textStyleName) || !cell.m_attributes.empty()
                || cell.m_contents.size()!=1u)return false;
            const auto& content=cell.m_contents.front();const auto& value=content.m_value;
            std::size_t variantBytes=0u;
            if(!small(content.m_text) || !small(value.m_formatString) || !small(value.m_valueString)
                || !value.m_rawData.empty() || value.m_value.type()!=DRW_Variant::STRING
                || !dxfEntitiesAuth::VariantAccess::canonical(value.m_value)
                || !small(*value.m_value.content.s)
                || !value.m_value.estimateRetainedPayloadBytes(1024u,variantBytes)
                || value.m_value.dwgCmColorValue()!=nullptr || value.m_value.isLayerRef()
                || value.m_value.rawDwgLayerReference()!=0u)return false;
        }
    }
    return true;
}

// Exact modeled equality for an immutable source snapshot. This is deliberately
// separate from source identity, dependency closure, and output eligibility.
// Check paired vector sizes before indexing: callback copies are mutable and a
// stale or adversarial copy must fail closed rather than read past a vector.
inline bool sameModel(const DRW_Table& a,const DRW_Table& b) noexcept {
    if(!bounded(a)||!bounded(b)
        ||!dxfTable::Decoder::sameModel(a,b,dxfEntitiesAuth::VariantAccess::equal)
        ||a.proxyGraphicsProvenance().resourceIndicesRemappedForDxf!=b.proxyGraphicsProvenance().resourceIndicesRemappedForDxf
        ||a.m_tableRowAlignmentOverrides.size()!=b.m_tableRowAlignmentOverrides.size()
        ||a.m_tableRowTextStyleHandles.size()!=b.m_tableRowTextStyleHandles.size()
        ||a.m_tableRowTextStyleNames.size()!=b.m_tableRowTextStyleNames.size()
        ||a.m_tableRowFillNone.size()!=b.m_tableRowFillNone.size()
        ||a.m_tableRowHeightOverrides.size()!=b.m_tableRowHeightOverrides.size()
        ||a.m_tableRowTextColors.size()!=b.m_tableRowTextColors.size()
        ||a.m_tableRowFillColors.size()!=b.m_tableRowFillColors.size()
        ||a.m_tableBorderColors.size()!=b.m_tableBorderColors.size()
        ||a.m_tableBorderLineweights.size()!=b.m_tableBorderLineweights.size()
        ||a.m_tableBorderVisibilities.size()!=b.m_tableBorderVisibilities.size()
        ||a.m_hasTableBorderLineweightOverrides.size()!=b.m_hasTableBorderLineweightOverrides.size()
        ||a.m_hasTableBorderVisibilityOverrides.size()!=b.m_hasTableBorderVisibilityOverrides.size())return false;
    if(a.m_tableRowAlignmentOverrides!=b.m_tableRowAlignmentOverrides
        ||a.m_tableRowTextStyleHandles!=b.m_tableRowTextStyleHandles
        ||a.m_tableRowTextStyleNames!=b.m_tableRowTextStyleNames
        ||a.m_tableRowFillNone!=b.m_tableRowFillNone
        ||a.m_tableBorderLineweights!=b.m_tableBorderLineweights
        ||a.m_tableBorderVisibilities!=b.m_tableBorderVisibilities
        ||a.m_hasTableBorderLineweightOverrides!=b.m_hasTableBorderLineweightOverrides
        ||a.m_hasTableBorderVisibilityOverrides!=b.m_hasTableBorderVisibilityOverrides)return false;
    const auto sameReal=[](double x,double y) noexcept{return std::memcmp(&x,&y,sizeof(x))==0;};
    const auto sameColor=[](const DRW_DwgCmColor& x,const DRW_DwgCmColor& y) noexcept {
        return std::tie(x.method,x.flags,x.index,x.rgb24,x.hasRgbColor,x.colorName,x.bookName,x.sourceVersion)
            ==std::tie(y.method,y.flags,y.index,y.rgb24,y.hasRgbColor,y.colorName,y.bookName,y.sourceVersion);
    };
    for(std::size_t i=0;i<a.m_tableRowHeightOverrides.size();++i)
        if(!sameReal(a.m_tableRowHeightOverrides[i],b.m_tableRowHeightOverrides[i])
            ||!sameColor(a.m_tableRowTextColors[i],b.m_tableRowTextColors[i])
            ||!sameColor(a.m_tableRowFillColors[i],b.m_tableRowFillColors[i]))return false;
    for(std::size_t i=0;i<a.m_tableBorderColors.size();++i)
        if(!sameColor(a.m_tableBorderColors[i],b.m_tableBorderColors[i]))return false;
    for(std::size_t i=0;i<a.m_content.m_subrecordRanges.size();++i)
        if(a.m_content.m_subrecordRanges[i].m_rawHandleCode!=b.m_content.m_subrecordRanges[i].m_rawHandleCode)return false;
    return true;
}
}
#endif
