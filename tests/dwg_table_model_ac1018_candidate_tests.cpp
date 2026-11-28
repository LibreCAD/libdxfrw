#include "dx_iface.h"
#include "intern/dwg_table_model_ac1018_candidate.h"
#include <limits>
class DxIfaceStandardDwgCaptureTestAccess {
public:static bool same(const DRW_Table& a,const DRW_Table& b) noexcept{return dx_iface::sameDwgTableModel(a,b);}
};
namespace {
DRW_Table build(){DRW_Table t;t.handle=0x9cu;t.m_hasSemanticContent=true;t.m_semanticContentComplete=true;
    t.numProxyGraph=2104;t.proxyGraphics=std::string(2104,'p');t.m_content.m_columns.resize(3);t.m_content.m_rows.resize(3);
    for(auto& row:t.m_content.m_rows){row.m_cells.resize(3);for(auto& cell:row.m_cells){cell.m_contents.resize(1);
        cell.m_contents[0].m_value.m_value=DRW_Variant(1,UTF8STRING("x"));}}
    DRW_DwgSubrecordRange range;range.m_name="range";range.m_rawHandleCode=4u;t.m_content.m_subrecordRanges.push_back(range);return t;}
}
int main(){
    auto source=build();auto model=source;
    if(!DwgTableModelAc1018Candidate::bounded(source)||!DxIfaceStandardDwgCaptureTestAccess::same(source,model))return 1;
    auto reject=[&](){return !DxIfaceStandardDwgCaptureTestAccess::same(source,model);};
    model.proxyGraphics[0]^=1;if(!reject())return 2;model=source;
    model.m_tableRowHeightOverrides[0]=-0.0;if(!reject())return 3;model=source;
    model.m_tableRowAlignmentOverrides[0]=1;if(!reject())return 4;model=source;
    model.m_tableRowTextStyleHandles[1]=42u;if(!reject())return 5;model=source;
    model.m_tableRowTextStyleNames[2]="other";if(!reject())return 6;model=source;
    model.m_tableRowFillNone[0]=true;if(!reject())return 7;model=source;
    model.m_tableRowTextColors[0].method=1;if(!reject())return 8;model=source;
    model.m_tableRowFillColors[2].colorName="other";if(!reject())return 9;model=source;
    model.m_tableBorderColors[17].sourceVersion=DRW::AC1018;if(!reject())return 10;model=source;
    model.m_tableBorderLineweights[4]=1;if(!reject())return 11;model=source;
    model.m_tableBorderVisibilities[4]=1;if(!reject())return 12;model=source;
    model.m_hasTableBorderLineweightOverrides[4]=true;if(!reject())return 13;model=source;
    model.m_hasTableBorderVisibilityOverrides[4]=true;if(!reject())return 14;model=source;
    model.m_content.m_subrecordRanges[0].m_rawHandleCode=5u;if(!reject())return 15;model=source;
    model=source;model.m_content.m_subrecordRanges[0].m_name+="x";if(!reject())return 23;model=source;
    ++model.m_content.m_subrecordRanges[0].m_startBit;if(!reject())return 24;model=source;
    ++model.m_content.m_subrecordRanges[0].m_bitSize;if(!reject())return 25;model=source;
    model.m_content.m_subrecordRanges[0].m_version=DRW::AC1027;if(!reject())return 26;model=source;
    ++model.m_content.m_subrecordRanges[0].m_count;if(!reject())return 27;model=source;
    model.m_content.m_subrecordRanges[0].m_parseComplete=false;if(!reject())return 28;model=source;
    model.m_content.m_rows[0].m_cells[0].m_contents[0].m_text="changed";if(!reject())return 16;model=source;
    model.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.addString(1,"changed");
    if(!reject() || *source.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.content.s!="x")return 17;model=source;
    std::string alias="x";model.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.content.s=&alias;
    if(!reject())return 18;model=source;
    model.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value=DRW_Variant(1003,UTF8STRING("x"),true);
    model.m_content.m_rows[0].m_cells[0].m_contents[0].m_value.m_value.setDwgRawLayerReference(0u,DRW::AC1018);
    if(!reject())return 19;model=source;
    model.m_content.m_rows[0].m_cells.push_back({});if(!reject())return 20;model=source;
    model.extData.push_back(std::make_shared<DRW_Variant>(1,UTF8STRING("x")));if(!reject())return 21;model=source;
    model.m_content.m_rows[0].m_cells[0].m_contents[0].m_text=std::string(257,'x');if(!reject())return 22;
    return 0;
}
