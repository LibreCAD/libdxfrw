#ifndef DXFTABLECONTENT_H
#define DXFTABLECONTENT_H

#include "drw_entities.h"
#include "dxfreader.h"

#include <charconv>
#include <cmath>
#include <limits>

// Ordered, non-recursive DXF TABLECONTENT source decoder. Schema facts come
// from ODA 5.4.1 20.4.97-101 and witnessed DXF marker scopes. No native bit
// layout, rendering semantics or converter replay admission is inferred here.
namespace dxfTableContent {
class Decoder {
    struct Unmatched {};
    const DRW_RawDxfObject& raw;
    dxfReader& reader;
    std::size_t position = 0u, customItems = 0u, contents = 0u, attributes = 0u, geometries = 0u;
    DRW_TableContentObject object;
    std::shared_ptr<DRW_TableContentDxfMetadata> metadata;

    const DRW_Variant& take(int code, DRW_Variant::TYPE type) {
        if (position == raw.groups.size() || raw.groups[position].code() != code
            || raw.groups[position].type() != type) throw Unmatched{};
        return raw.groups[position++];
    }
    int peek() const { return position == raw.groups.size() ? -1 : raw.groups[position].code(); }
    std::int32_t integer(int code) { return take(code, DRW_Variant::INTEGER).i_val(); }
    std::uint32_t word(int code) { return static_cast<std::uint32_t>(integer(code)); }
    double real(int code) {
        const double value = take(code, DRW_Variant::DOUBLE).d_val();
        if (!std::isfinite(value)) throw Unmatched{};
        return value;
    }
    std::string_view literal(int code) {
        const auto& value = take(code, DRW_Variant::STRING);
        if (value.content.s == nullptr) throw Unmatched{};
        return *value.content.s;
    }
    void marker(int code, std::string_view expected) {
        if (literal(code) != expected) throw Unmatched{};
    }
    std::uint32_t handle(int code) {
        const auto text = literal(code);
        std::uint64_t value = 0u;
        const auto parsed = std::from_chars(text.data(), text.data()+text.size(), value, 16);
        if (text.empty() || text.size()>16u || parsed.ec != std::errc{}
            || parsed.ptr != text.data()+text.size() || value>std::numeric_limits<std::uint32_t>::max())
            throw Unmatched{};
        return static_cast<std::uint32_t>(value);
    }
    void retain(std::size_t bytes) {
        if (!retainDxfReadBytes(metadata->m_retainedTextLease, reader.retainedByteBudget(), bytes))
            throw Unmatched{};
    }
    std::string text(int code) {
        const auto source = literal(code);
        if (source.size()>16u*1024u*1024u) throw Unmatched{};
        // Precharge the by-value input and worst-case UTF8 scratch expansion.
        DxfReadRetainedReservation scratch(reader.retainedByteBudget(), source.size()*5u);
        if (reader.retainedByteBudget() && !scratch.reserved()) throw Unmatched{};
        auto result = reader.toUtf8String(std::string(source), code);
        retain(result.size());
        return result;
    }
    std::size_t count(int code, std::size_t max, std::size_t minimumGroups) {
        const auto value = integer(code);
        if (value<0 || static_cast<std::size_t>(value)>max
            || (minimumGroups && static_cast<std::size_t>(value)>(raw.groups.size()-position)/minimumGroups))
            throw Unmatched{};
        return static_cast<std::size_t>(value);
    }
    void chargeNode() { if (!reader.chargeTypedVariantNode()) throw Unmatched{}; }
    bool flag(int code) {
        const int value = integer(code);
        if (value != 0 && value != 1) throw Unmatched{};
        return value != 0;
    }
    DRW_Coord point(int code, bool three = true) {
        const double x=real(code), y=real(code+10);
        return DRW_Coord(x,y,three?real(code+20):0.0);
    }
    static bool dataType(int value) {
        return value==0 || value==1 || value==2 || value==4 || value==8 || value==16
            || value==32 || value==64 || value==512;
    }
    static bool unitType(int value) {
        return value==0 || value==1 || value==2 || value==4 || value==8;
    }
    int styleId() {
        const int id=integer(90);
        if (id<0 || (id>3 && id<101 && (id!=4 || raw.m_version<DRW::AC1024))) throw Unmatched{};
        return id;
    }
    DRW_CadValue value() {
        DRW_CadValue result;
        result.m_formatFlags=integer(93);
        result.m_dataType=integer(90);
        if (!dataType(result.m_dataType)) throw Unmatched{};
        // The ODA first flag bit suppresses the payload. Bit 2 alone does
        // not: witnessed checksum values use flags=2 and strings flags=6.
        if ((result.m_formatFlags & 1)==0) {
            if (result.m_dataType==0 || result.m_dataType==1) {
                const int number=integer(91); chargeNode(); result.m_value=DRW_Variant(91,number);
            } else if (result.m_dataType==2) {
                const double number=real(140); chargeNode(); result.m_value=DRW_Variant(140,number);
            } else if (result.m_dataType==4) {
                auto decoded=text(1); chargeNode(); result.m_value.addString(1,std::move(decoded));
            } else if (result.m_dataType==64) result.m_handle=handle(330);
            else if (result.m_dataType==16 || result.m_dataType==32) {
                const auto size=count(92,24u,0u);
                if (size!=(result.m_dataType==16?16u:24u)) throw Unmatched{};
                result.m_dataSize=static_cast<std::uint32_t>(size);
                const auto coordinate=point(11,result.m_dataType==32); chargeNode();
                result.m_value=DRW_Variant(11,coordinate);
            } else if (result.m_dataType==8) {
                if (count(92,8u,0u)!=8u) throw Unmatched{};
                result.m_dataSize=8u; retain(8u);
                while (peek()==310) {
                    const auto hex=literal(310);
                    if ((hex.size()&1u)!=0u || hex.size()/2u>8u-result.m_rawData.size()) throw Unmatched{};
                    for (std::size_t i=0; i<hex.size(); i+=2u) {
                        unsigned byte=0u;
                        const auto parsed=std::from_chars(hex.data()+i,hex.data()+i+2u,byte,16);
                        if (parsed.ec!=std::errc{} || parsed.ptr!=hex.data()+i+2u) throw Unmatched{};
                        result.m_rawData.push_back(static_cast<std::uint8_t>(byte));
                    }
                }
                if (result.m_rawData.size()!=8u) throw Unmatched{};
            } else throw Unmatched{}; // Unproved general/buffer payload form.
        }
        result.m_unitType=integer(94);
        if (!unitType(result.m_unitType)) throw Unmatched{};
        result.m_formatString=text(300); result.m_valueString=text(302);
        marker(304,"ACVALUE_END");
        return result;
    }
    void custom(DRW_TableContentDxfMetadata::Scope& scope) {
        scope.m_customData=integer(91);
        marker(301,"CUSTOMDATA"); marker(1,"DATAMAP_BEGIN");
        const auto size=count(90,10000u-customItems,7u); customItems+=size;
        scope.m_customItems.reserve(size);
        for (std::size_t i=0; i<size; ++i) {
            DRW_TableContentDxfMetadata::CustomItem item;
            item.m_name=text(300); marker(301,"DATAMAP_VALUE"); item.m_value=value();
            scope.m_customItems.push_back(std::move(item));
        }
        marker(309,"DATAMAP_END");
    }
    void color(int& scalar, DRW_TableStyleCmcColor& metadataColor) {
        scalar=integer(62);
        if (peek()==420) {
            metadataColor.m_rgb=integer(420);
            if (metadataColor.m_rgb<0 || metadataColor.m_rgb>0xFFFFFF) throw Unmatched{};
        }
        // Named-color presence needs its own witnessed schema, not guessed
        // private TABLESTYLE parser flags. Such profiles remain raw.
    }
    DRW_TableStyleContentFormat format() {
        DRW_TableStyleContentFormat result;
        marker(300,"CONTENTFORMAT"); marker(1,"CONTENTFORMAT_BEGIN");
        result.m_propertyOverrideFlags=word(90); result.m_propertyFlags=word(91);
        result.m_valueDataType=integer(92); result.m_valueUnitType=integer(93);
        if (!dataType(result.m_valueDataType) || !unitType(result.m_valueUnitType)
            || (result.m_propertyFlags & ~std::uint32_t{0x100u})) throw Unmatched{};
        result.m_valueFormatString=text(300); result.m_rotation=real(40); result.m_blockScale=real(140);
        result.m_cellAlignment=integer(94);
        if (result.m_cellAlignment<0 || result.m_cellAlignment>9) throw Unmatched{};
        color(result.m_contentColor,result.m_contentColorCmc);
        result.m_textStyleHandle=handle(340); result.m_textHeight=real(144);
        marker(309,"CONTENTFORMAT_END");
        return result;
    }
    DRW_TableStyleCellStyle style(int expectedType) {
        DRW_TableStyleCellStyle result;
        marker(1,"TABLEFORMAT_BEGIN"); result.m_type=integer(90);
        if (result.m_type!=expectedType) throw Unmatched{};
        result.m_hasData=flag(170);
        if (result.m_hasData) {
            result.m_propertyOverrideFlags=word(91); result.m_mergeFlags=word(92);
            color(result.m_backgroundColor,result.m_backgroundColorCmc);
            result.m_contentLayout=word(93); result.m_contentFormat=format();
            result.m_marginOverrideFlags=flag(171)?1:0;
            if (result.m_marginOverrideFlags) {
                marker(301,"MARGIN"); marker(1,"CELLMARGIN_BEGIN");
                result.m_verticalMargin=real(40); result.m_horizontalMargin=real(40);
                result.m_bottomMargin=real(40); result.m_rightMargin=real(40);
                result.m_marginHorizontalSpacing=real(40); result.m_marginVerticalSpacing=real(40);
                marker(309,"CELLMARGIN_END");
            }
            const auto borders=count(94,6u,1u); result.m_borders.reserve(borders);
            for (std::size_t i=0; i<borders; ++i) {
                DRW_TableStyleBorder border;
                border.m_edgeFlags=integer(95);
                if (border.m_edgeFlags==0 || (border.m_edgeFlags & ~0x3F)) throw Unmatched{};
                marker(302,"GRIDFORMAT"); marker(1,"GRIDFORMAT_BEGIN");
                border.m_propertyOverrideFlags=integer(90); border.m_borderType=integer(91);
                color(border.m_color,border.m_colorCmc); border.m_lineWeight=integer(92);
                border.m_lineTypeHandle=handle(340); border.m_visible=integer(93);
                if (border.m_visible!=0 && border.m_visible!=1) throw Unmatched{};
                border.m_doubleLineSpacing=real(40); marker(309,"GRIDFORMAT_END");
                result.m_borders.push_back(std::move(border));
            }
        }
        marker(309,"TABLEFORMAT_END");
        return result;
    }
    void cell(DRW_TableCell& cell, DRW_TableContentDxfMetadata::Cell& detail) {
        marker(300,"CELL"); marker(1,"LINKEDTABLEDATACELL_BEGIN");
        cell.m_flags=integer(90); cell.m_toolTip=text(300); custom(detail.m_scope);
        cell.m_hasLinkedData=flag(92);
        if (cell.m_hasLinkedData) {
            cell.m_dataLinkHandle=handle(340); cell.m_linkedDataRowCount=integer(93);
            cell.m_linkedDataColumnCount=integer(94); cell.m_linkedDataUnknown=integer(96);
        }
        const auto size=count(95,10000u-contents,8u); contents+=size;
        cell.m_contents.reserve(size); detail.m_contentFormats.reserve(size);
        for (std::size_t i=0; i<size; ++i) {
            marker(302,"CONTENT"); marker(1,"CELLCONTENT_BEGIN");
            DRW_TableCellContent content; content.m_type=integer(90);
            if (content.m_type==1) {
                marker(300,"VALUE"); content.m_value=value();
                if (content.m_value.m_value.type()==DRW_Variant::STRING) {
                    retain(content.m_value.m_value.content.s->size());
                    content.m_text=*content.m_value.m_value.content.s;
                }
            } else if (content.m_type==2 || content.m_type==4) content.m_handle=handle(340);
            else if (content.m_type!=0) throw Unmatched{};
            const auto attrs=count(91,10000u-attributes,3u); attributes+=attrs;
            for (std::size_t j=0; j<attrs; ++j) {
                DRW_TableCellAttribute attribute;
                attribute.m_attdefHandle=handle(330); attribute.m_text=text(301);
                attribute.m_index=integer(92); attribute.m_contentIndex=static_cast<std::uint32_t>(i);
                if (attribute.m_index<1) throw Unmatched{};
                cell.m_attributes.push_back(std::move(attribute));
            }
            marker(309,"CELLCONTENT_END"); marker(1,"FORMATTEDCELLCONTENT_BEGIN");
            std::shared_ptr<const DRW_TableStyleContentFormat> contentFormat;
            if (flag(170)) contentFormat=std::make_shared<DRW_TableStyleContentFormat>(format());
            marker(309,"FORMATTEDCELLCONTENT_END");
            cell.m_contents.push_back(std::move(content)); detail.m_contentFormats.push_back(std::move(contentFormat));
        }
        marker(309,"LINKEDTABLEDATACELL_END"); marker(1,"FORMATTEDTABLEDATACELL_BEGIN");
        marker(300,"CELLTABLEFORMAT"); detail.m_scope.m_style=style(1);
        marker(309,"FORMATTEDTABLEDATACELL_END"); marker(1,"TABLECELL_BEGIN");
        cell.m_styleId=styleId(); cell.m_geometryFlags=word(91);
        if (cell.m_geometryFlags) {
            cell.m_geometryRecordFlags=word(91); cell.m_width=real(40); cell.m_height=real(41);
            const auto sizeGeometry=count(94,10000u-geometries,11u); geometries+=sizeGeometry;
            cell.m_geometryHandle=handle(330); detail.m_geometry.reserve(sizeGeometry);
            for (std::size_t i=0; i<sizeGeometry; ++i) {
                DRW_TableGeometryContent geometry;
                geometry.m_topLeft=point(10); geometry.m_center=point(11);
                geometry.m_contentWidth=real(43); geometry.m_contentHeight=real(44);
                geometry.m_width=real(45); geometry.m_height=real(46); geometry.m_unknown=integer(95);
                detail.m_geometry.push_back(geometry);
            }
        }
        marker(309,"TABLECELL_END");
    }
    void parse() {
        object.handle=handle(5); object.parentHandle=handle(330);
        if (object.handle==0u) throw Unmatched{};
        marker(100,"AcDbLinkedData"); object.m_content.m_name=text(1); object.m_content.m_description=text(300);
        marker(100,"AcDbLinkedTableData"); const auto columns=count(90,1000u,14u);
        object.m_content.m_columns.reserve(columns); metadata->m_columns.reserve(columns);
        for (std::size_t i=0; i<columns; ++i) {
            DRW_TableColumn column; DRW_TableContentDxfMetadata::Scope detail;
            marker(300,"COLUMN"); marker(1,"LINKEDTABLEDATACOLUMN_BEGIN");
            column.m_name=text(300); custom(detail); marker(309,"LINKEDTABLEDATACOLUMN_END");
            marker(1,"FORMATTEDTABLEDATACOLUMN_BEGIN"); marker(300,"COLUMNTABLEFORMAT");
            detail.m_style=style(3); marker(309,"FORMATTEDTABLEDATACOLUMN_END");
            marker(1,"TABLECOLUMN_BEGIN"); column.m_styleId=styleId(); column.m_width=real(40);
            marker(309,"TABLECOLUMN_END");
            object.m_content.m_columns.push_back(std::move(column)); metadata->m_columns.push_back(std::move(detail));
        }
        const auto rows=count(91,10000u,14u);
        if (columns && rows>200000u/columns) throw Unmatched{};
        object.m_content.m_rows.reserve(rows); metadata->m_rows.reserve(rows);
        for (std::size_t i=0; i<rows; ++i) {
            DRW_TableRow row; DRW_TableContentDxfMetadata::Row detail;
            marker(301,"ROW"); marker(1,"LINKEDTABLEDATAROW_BEGIN");
            const auto cells=count(90,columns,15u);
            if (cells!=columns) throw Unmatched{};
            row.m_cells.reserve(cells); detail.m_cells.reserve(cells);
            for (std::size_t j=0; j<cells; ++j) {
                DRW_TableCell parsed; DRW_TableContentDxfMetadata::Cell companion;
                cell(parsed,companion); row.m_cells.push_back(std::move(parsed)); detail.m_cells.push_back(std::move(companion));
            }
            custom(detail.m_scope); marker(309,"LINKEDTABLEDATAROW_END");
            marker(1,"FORMATTEDTABLEDATAROW_BEGIN"); marker(300,"ROWTABLEFORMAT");
            detail.m_scope.m_style=style(2); marker(309,"FORMATTEDTABLEDATAROW_END");
            marker(1,"TABLEROW_BEGIN"); row.m_styleId=styleId(); row.m_height=real(40); marker(309,"TABLEROW_END");
            object.m_content.m_rows.push_back(std::move(row)); metadata->m_rows.push_back(std::move(detail));
        }
        metadata->m_linkedDataUnknown92=integer(92);
        marker(100,"AcDbFormattedTableData"); marker(300,"TABLEFORMAT"); metadata->m_tableStyle=style(4);
        const auto ranges=count(90,10000u,4u); object.m_content.m_mergedRanges.reserve(ranges);
        for (std::size_t i=0; i<ranges; ++i) {
            DRW_TableMergedRange range;
            range.m_topRow=integer(91); range.m_leftColumn=integer(92);
            range.m_bottomRow=integer(93); range.m_rightColumn=integer(94);
            if (range.m_topRow<0 || range.m_leftColumn<0 || range.m_bottomRow<range.m_topRow
                || range.m_rightColumn<range.m_leftColumn || static_cast<std::size_t>(range.m_bottomRow)>=rows
                || static_cast<std::size_t>(range.m_rightColumn)>=columns) throw Unmatched{};
            for (const auto& prior : object.m_content.m_mergedRanges)
                if (range.m_topRow<=prior.m_bottomRow && prior.m_topRow<=range.m_bottomRow
                    && range.m_leftColumn<=prior.m_rightColumn && prior.m_leftColumn<=range.m_rightColumn) throw Unmatched{};
            object.m_content.m_mergedRanges.push_back(range);
        }
        marker(100,"AcDbTableContent"); object.m_content.m_tableStyleHandle=handle(340);
        if (position!=raw.groups.size()) throw Unmatched{};
        object.m_parseComplete=true;
        // This flag protects the old minimal native encoder. Rich companions
        // must not be discarded by treating m_content alone as lossless.
        object.m_semanticContentComplete=false;
        object.m_dxfMetadata=std::move(metadata);
    }
public:
    Decoder(const DRW_RawDxfObject& source,dxfReader& input) : raw(source),reader(input),
        metadata(std::make_shared<DRW_TableContentDxfMetadata>()) {}
    bool decode(DRW_TableContentObject& output) {
        try { parse(); output=std::move(object); return true; }
        catch (...) { return false; }
    }
};
} // namespace dxfTableContent
#endif
