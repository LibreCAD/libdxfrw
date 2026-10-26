// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DXFTABLE_H
#define DXFTABLE_H
#include "dxfcompound.h"
#include <tuple>
#include <type_traits>

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
};
}
#endif
