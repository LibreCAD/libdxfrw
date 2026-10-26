// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFTABLE_H
#define DXFTABLE_H
#include "dxfcompound.h"
#include <tuple>
#include <type_traits>
#include <typeinfo>

// Shared production field entry/finalization and original nonallocating TABLE
// parser-state observation. Not a scalar profile or semantic/output certificate.
namespace dxfTable {
struct SourceState {
    dxfCompound::SourceState common;
    unsigned subclass{0};int rows{-1},columns{-1},currentCell{-1};
    std::size_t rowHeights{0},columnWidths{0},nextCell{0};
    bool inValue{false},dataVersion{false},properties{false},unsupported{false};
    int attributesExpected{-1},attributesRead{0},attributesContent{-1};
    bool operator==(const SourceState& b) const noexcept {
        return common==b.common&&std::tie(subclass,rows,columns,currentCell,rowHeights,columnWidths,nextCell,inValue,dataVersion,properties,unsupported,attributesExpected,attributesRead,attributesContent)
            ==std::tie(b.subclass,b.rows,b.columns,b.currentCell,b.rowHeights,b.columnWidths,b.nextCell,b.inValue,b.dataVersion,b.properties,b.unsupported,b.attributesExpected,b.attributesRead,b.attributesContent);
    }
    bool hasNoPendingXdata() const noexcept{return common.hasNoPendingXdata();}
};
static_assert(std::is_trivially_copyable<SourceState>::value,"TABLE state observation must not allocate");
class Decoder {
    DRW_Table& value;
public:
    explicit Decoder(DRW_Table& v):value(v){}
    bool parseCode(int code,const std::unique_ptr<dxfReader>& reader){return value.parseCode(code,reader);}
    void finish() noexcept{value.finalizeDxfParse();}
    static SourceState observe(const DRW_Table& v) noexcept {
        SourceState s;s.common=dxfCompound::Decoder::observe(v);s.subclass=static_cast<unsigned>(v.m_dxfSubclass);
        s.rows=v.m_dxfRowsExpected;s.columns=v.m_dxfColumnsExpected;s.currentCell=v.m_dxfCurrentCell;
        s.rowHeights=v.m_dxfRowHeightsRead;s.columnWidths=v.m_dxfColumnWidthsRead;s.nextCell=v.m_dxfNextCell;
        s.inValue=v.m_dxfInCellValue;s.dataVersion=v.m_dxfDataVersionRead;s.properties=v.m_dxfTablePropertiesStarted;s.unsupported=v.m_dxfUnsupportedData;
        s.attributesExpected=v.m_dxfAttributesExpected;s.attributesRead=v.m_dxfAttributesRead;s.attributesContent=v.m_dxfAttributesContentIndex;return s;
    }
    // Exact modeled equality only: no scalar inventory, target resolution or
    // output admission. Private parser state is compared separately, pre-copy.
    template<class Equal> static bool sameModel(const DRW_Table& a,const DRW_Table& b,Equal equal) noexcept {
        if(!concrete(a)||!concrete(b)||!dxfCompound::Decoder::sameInsertModel(a,b,equal)
            ||std::tie(a.m_valueFlag,a.m_hasSemanticContent,a.m_semanticContentComplete,a.m_dataVersion,a.m_ownerBlockRecordHandle,
                a.m_tableOverrideFlags,a.m_borderColorOverrideFlags,a.m_borderLineweightOverrideFlags,a.m_borderVisibilityOverrideFlags,
                a.m_flowDirection,a.m_titleSuppressed,a.m_headerSuppressed,a.m_tableStyleHandle)
            !=std::tie(b.m_valueFlag,b.m_hasSemanticContent,b.m_semanticContentComplete,b.m_dataVersion,b.m_ownerBlockRecordHandle,
                b.m_tableOverrideFlags,b.m_borderColorOverrideFlags,b.m_borderLineweightOverrideFlags,b.m_borderVisibilityOverrideFlags,
                b.m_flowDirection,b.m_titleSuppressed,b.m_headerSuppressed,b.m_tableStyleHandle)
            ||!point(a.m_horizontalDirection,b.m_horizontalDirection)||!real(a.m_horizontalCellMargin,b.m_horizontalCellMargin)
            ||!real(a.m_verticalCellMargin,b.m_verticalCellMargin))return false;
        const auto& x=a.m_content;const auto& y=b.m_content;
        if(std::tie(x.m_name,x.m_description,x.m_tableStyleHandle,x.m_overrideStyleType,x.m_fieldHandles)
            !=std::tie(y.m_name,y.m_description,y.m_tableStyleHandle,y.m_overrideStyleType,y.m_fieldHandles)
            ||x.m_columns.size()!=y.m_columns.size()||x.m_rows.size()!=y.m_rows.size()
            ||x.m_mergedRanges.size()!=y.m_mergedRanges.size()||x.m_subrecordRanges.size()!=y.m_subrecordRanges.size())return false;
        for(std::size_t i=0;i<x.m_columns.size();++i) {
            const auto& p=x.m_columns[i];const auto& q=y.m_columns[i];
            if(std::tie(p.m_name,p.m_styleId,p.m_overrideStyleType)!=std::tie(q.m_name,q.m_styleId,q.m_overrideStyleType)||!real(p.m_width,q.m_width))return false;
        }
        for(std::size_t i=0;i<x.m_rows.size();++i) {
            const auto& p=x.m_rows[i];const auto& q=y.m_rows[i];
            if(p.m_styleId!=q.m_styleId||p.m_overrideStyleType!=q.m_overrideStyleType||!real(p.m_height,q.m_height)||p.m_cells.size()!=q.m_cells.size())return false;
            for(std::size_t j=0;j<p.m_cells.size();++j)if(!cell(p.m_cells[j],q.m_cells[j],equal))return false;
        }
        for(std::size_t i=0;i<x.m_mergedRanges.size();++i) {
            const auto& p=x.m_mergedRanges[i];const auto& q=y.m_mergedRanges[i];
            if(std::tie(p.m_topRow,p.m_leftColumn,p.m_bottomRow,p.m_rightColumn)!=std::tie(q.m_topRow,q.m_leftColumn,q.m_bottomRow,q.m_rightColumn))return false;
        }
        for(std::size_t i=0;i<x.m_subrecordRanges.size();++i) {
            const auto& p=x.m_subrecordRanges[i];const auto& q=y.m_subrecordRanges[i];
            if(std::tie(p.m_name,p.m_startBit,p.m_bitSize,p.m_version,p.m_count,p.m_parseComplete)
                !=std::tie(q.m_name,q.m_startBit,q.m_bitSize,q.m_version,q.m_count,q.m_parseComplete))return false;
        }
        return true;
    }
private:
    static bool concrete(const DRW_Table& t) noexcept {
        if(typeid(t)!=typeid(DRW_Table)||t.eType!=DRW::TABLE)return false;
        for(const auto& a:t.attlist) {
            const auto* attribute=a.get();if(!attribute||typeid(*attribute)!=typeid(DRW_Attrib))return false;
            const auto* nested=attribute->mtext.get();if(nested&&typeid(*nested)!=typeid(DRW_MText))return false;
        }
        const auto* footer=t.seqEndEntity.get();return !footer||typeid(*footer)==typeid(DRW_SeqEnd);
    }
    static bool real(double a,double b) noexcept{return std::memcmp(&a,&b,sizeof(a))==0;}
    static bool point(const DRW_Coord& a,const DRW_Coord& b) noexcept{return real(a.x,b.x)&&real(a.y,b.y)&&real(a.z,b.z);}
    template<class Equal> static bool cell(const DRW_TableCell& a,const DRW_TableCell& b,Equal equal) noexcept {
        const auto fields=[](const auto& c){return std::tie(c.m_flags,c.m_type,c.m_edgeFlags,c.m_isMerged,c.m_autoFit,c.m_mergedWidth,c.m_mergedHeight,
            c.m_toolTip,c.m_textStyleName,c.m_styleId,c.m_overrideStyleType,c.m_virtualEdgeFlags,c.m_overrideFlags,c.m_valueHandle,c.m_textStyleHandle,
            c.m_textStyleOverrideHandle,c.m_blockHandle,c.m_hasLinkedData,c.m_dataLinkHandle,c.m_linkedDataRowCount,c.m_linkedDataColumnCount,
            c.m_linkedDataUnknown,c.m_geometryFlags,c.m_geometryHandle,c.m_geometryRecordFlags,c.m_textColorRgb,c.m_textColorMethod,
            c.m_fillColorRgb,c.m_fillColorMethod,c.m_backgroundEnabled,c.m_textAlignment);};
        if(fields(a)!=fields(b)||!point(a.m_geometryTopLeft,b.m_geometryTopLeft)||!point(a.m_geometryCenter,b.m_geometryCenter)
            ||a.m_attributes.size()!=b.m_attributes.size()||a.m_contents.size()!=b.m_contents.size())return false;
        const double x[]={a.m_rotation,a.m_blockScale,a.m_width,a.m_height,a.m_contentWidth,a.m_contentHeight,a.m_geometryWidth,a.m_geometryHeight,a.m_textHeight};
        const double y[]={b.m_rotation,b.m_blockScale,b.m_width,b.m_height,b.m_contentWidth,b.m_contentHeight,b.m_geometryWidth,b.m_geometryHeight,b.m_textHeight};
        for(std::size_t i=0;i<sizeof(x)/sizeof(x[0]);++i)if(!real(x[i],y[i]))return false;
        for(std::size_t i=0;i<a.m_borders.size();++i) {
            const auto& p=a.m_borders[i];const auto& q=b.m_borders[i];
            if(std::tie(p.m_lineWeight,p.m_visible,p.m_colorRgb,p.m_colorMethod)!=std::tie(q.m_lineWeight,q.m_visible,q.m_colorRgb,q.m_colorMethod))return false;
        }
        for(std::size_t i=0;i<a.m_attributes.size();++i) {
            const auto& p=a.m_attributes[i];const auto& q=b.m_attributes[i];
            if(std::tie(p.m_attdefHandle,p.m_index,p.m_text,p.m_contentIndex)!=std::tie(q.m_attdefHandle,q.m_index,q.m_text,q.m_contentIndex))return false;
        }
        for(std::size_t i=0;i<a.m_contents.size();++i) {
            const auto& p=a.m_contents[i];const auto& q=b.m_contents[i];const auto& v=p.m_value;const auto& w=q.m_value;
            if(std::tie(p.m_type,p.m_text,p.m_handle)!=std::tie(q.m_type,q.m_text,q.m_handle)
                ||std::tie(v.m_formatFlags,v.m_dataType,v.m_dataSize,v.m_unitType,v.m_formatString,v.m_valueString,v.m_handle,v.m_rawData)
                    !=std::tie(w.m_formatFlags,w.m_dataType,w.m_dataSize,w.m_unitType,w.m_formatString,w.m_valueString,w.m_handle,w.m_rawData)
                ||!equal(v.m_value,w.m_value))return false;
        }
        return true;
    }
};
}
#endif
