#include "dx_iface.h"
#include <iomanip>
#include <iostream>
#include <memory>

// Exact-source graph census for the AC1032 profile receipt. The selected
// handles are tied to that receipt and do not define a general TABLE schema.
class Probe final : public dx_iface {
public:
    void addNamedObjectDictionary(const DRW_Dictionary& value) override {
        dx_iface::addNamedObjectDictionary(value);
        dumpDictionary("NOD", value);
    }
    void addDictionary(const DRW_Dictionary& value) override {
        dx_iface::addDictionary(value);
        if (value.handle == 0x6au) dumpDictionary("DICTIONARY", value);
    }
    void addTableStyle(const DRW_TableStyle& value) override {
        dx_iface::addTableStyle(value);
        if (value.handle != 0x6bu) return;
        std::cout << "TABLESTYLE handle=0x" << std::hex << value.handle
                  << " owner=0x" << value.parentHandle
                  << " xdict=0x" << value.xDictHandle << std::dec
                  << " name=" << value.m_name << " reactors=";
        for (const auto handle : value.reactorHandles)
            std::cout << "0x" << std::hex << handle << ',';
        std::cout << std::dec << " rowStyles=" << value.m_rowStyles.size()
                  << " cellStyles=" << value.m_cellStyles.size() << '\n';
        for (const auto& cell : value.m_cellStyles) {
            std::cout << "STYLE_CELL id=" << cell.m_id << " name=" << cell.m_name
                      << " textStyle=0x" << std::hex
                      << cell.m_contentFormat.m_textStyleHandle << std::dec
                      << " borders=" << cell.m_borders.size() << '\n';
            for (const auto& border : cell.m_borders)
                std::cout << "  BORDER_LTYPE edge=" << border.m_edgeFlags
                          << " handle=0x" << std::hex
                          << border.m_lineTypeHandle << std::dec << '\n';
        }
    }
    void addTextStyle(const DRW_Textstyle& value) override {
        dx_iface::addTextStyle(value);
        if (value.handle == 0x11u)
            std::cout << "TEXTSTYLE handle=0x11 owner=0x" << std::hex
                      << value.parentHandle << std::dec << " name=" << value.name
                      << " font=" << value.font << '\n';
    }
    void addLType(const DRW_LType& value) override {
        dx_iface::addLType(value);
        if (value.handle == 0x14u)
            std::cout << "LTYPE handle=0x14 owner=0x" << std::hex
                      << value.parentHandle << std::dec << " name=" << value.name
                      << " description=" << value.desc << '\n';
    }
    void addBlockRecord(const DRW_Block_Record& value) override {
        dx_iface::addBlockRecord(value);
        if (value.handle != 0x10bu && value.handle != 0xd0u) return;
        std::cout << "BLOCK_RECORD handle=0x" << std::hex << value.handle
                  << " owner=0x" << value.parentHandle << std::dec
                  << " name=" << value.name
                  << " inserts=" << value.insertCount
                  << " entityMapCount=" << value.entityMapCount()
                  << " insertHandles=";
        for (const auto handle : value.insertHandles)
            std::cout << "0x" << std::hex << handle << ',';
        std::cout << std::dec << '\n';
    }
    void addBlock(const DRW_Block& value) override {
        dx_iface::addBlock(value);
        if (value.handle == 0x10fu || value.handle == 0xd1u)
            std::cout << "BLOCK handle=0x" << std::hex << value.handle
                      << " owner=0x" << value.parentHandle << std::dec
                      << " name=" << value.name << '\n';
    }
    void addInsert(const DRW_Insert& value) override {
        dx_iface::addInsert(value);
        if (value.handle == 0xdbu)
            std::cout << "INSERT handle=0xdb owner=0x" << std::hex
                      << value.parentHandle << std::dec << " blockName="
                      << value.name << '\n';
    }
    void addTable(const DRW_Table& value) override {
        dx_iface::addTable(value);
        if (value.handle != 0xcfu) return;
        std::cout << "TABLE handle=0xcf owner=0x" << std::hex
                  << value.parentHandle << " tableStyle=0x"
                  << value.m_tableStyleHandle << " contentStyle=0x"
                  << value.m_content.m_tableStyleHandle << " blockOwner=0x"
                  << value.m_ownerBlockRecordHandle << std::dec
                  << " insertBlockName=" << value.name
                  << " gridRows=" << value.m_content.m_rows.size()
                  << " gridColumns=" << value.m_content.m_columns.size()
                  << " semanticComplete=" << value.m_semanticContentComplete
                  << " hasSemantic=" << value.m_hasSemanticContent << '\n';
        for (std::size_t r = 0; r < value.m_content.m_rows.size(); ++r) {
            const auto& row = value.m_content.m_rows[r];
            for (std::size_t c = 0; c < row.m_cells.size(); ++c) {
                const auto& cell = row.m_cells[c];
                for (std::size_t i = 0; i < cell.m_contents.size(); ++i) {
                    const auto& content = cell.m_contents[i];
                    if (content.m_handle != 0u)
                        std::cout << "CELL_REF row=" << r << " col=" << c
                                  << " item=" << i << " type=" << content.m_type
                                  << " handle=0x" << std::hex << content.m_handle
                                  << std::dec << '\n';
                }
            }
        }
    }
    void addDwgFramePublication(
        const DRW_DwgFramePublication& value) override {
        dx_iface::addDwgFramePublication(value);
        if (value.m_handle != 0xcfu && value.m_handle != 0x6au
            && value.m_handle != 0x6bu && value.m_handle != 0x10bu
            && value.m_handle != 0x10fu && value.m_handle != 0xdbu
            && value.m_handle != 0x11u && value.m_handle != 0x14u
            && value.m_handle != 0xd0u && value.m_handle != 0xd1u)
            return;
        std::cout << "FRAME handle=0x" << std::hex << value.m_handle
                  << " offset=0x" << value.m_sourceOffset << std::dec
                  << " ordinal=" << value.m_sourceMapOrdinal
                  << " location=" << value.m_hasSourceLocation
                  << " entity=" << value.m_isEntity
                  << " encodedType=" << value.m_encodedType
                  << " resolvedType=" << value.m_resolvedType
                  << " record=" << value.m_recordName
                  << " class=" << value.m_className
                  << " carrier=" << static_cast<unsigned>(value.m_carrier)
                  << " commonLinks="
                  << static_cast<unsigned>(value.m_commonLinkEvidence)
                  << " offsetSpace="
                  << static_cast<unsigned>(value.m_sourceOffsetSpace)
                  << " owner=0x" << std::hex << value.m_parentHandle
                  << " xdict=0x" << value.m_xDictHandle << std::dec
                  << " reactors=";
        for (const auto handle : value.m_reactorHandles)
            std::cout << "0x" << std::hex << handle << ',';
        std::cout << std::dec << '\n';
    }
    void addDwgBlockReachability(
        const DRW_DwgBlockReachability& value) override {
        dx_iface::addDwgBlockReachability(value);
        if (value.m_blockRecord.m_handle != 0x10bu
            && value.m_blockRecord.m_handle != 0xd0u)
            return;
        const auto printFrame = [](const char* role,
                                   const DRW_DwgSourceFrame& frame) {
            std::cout << "  REACHABLE role=" << role << " handle=0x"
                      << std::hex << frame.m_handle << " offset=0x"
                      << frame.m_sourceOffset << std::dec << " ordinal="
                      << frame.m_sourceMapOrdinal << " location="
                      << frame.m_hasSourceLocation << '\n';
        };
        std::cout << "BLOCK_REACHABILITY handle=0x" << std::hex
                  << value.m_blockRecord.m_handle << std::dec << " complete="
                  << value.m_complete << " entities=" << value.m_entities.size()
                  << '\n';
        printFrame("record", value.m_blockRecord);
        printFrame("block", value.m_block);
        printFrame("endBlock", value.m_endBlock);
        for (const auto& frame : value.m_entities)
            printFrame("entity", frame);
    }
private:
    static void dumpDictionary(const char* kind, const DRW_Dictionary& value) {
        std::cout << kind << " handle=0x" << std::hex << value.handle
                  << " owner=0x" << value.parentHandle << std::dec
                  << " hardOwner=" << static_cast<unsigned>(value.hardOwner)
                  << " entries=" << value.m_entries.size() << " reactors=";
        for (const auto handle : value.reactorHandles)
            std::cout << "0x" << std::hex << handle << ',';
        std::cout << std::dec << '\n';
        for (const auto& entry : value.m_entries)
            std::cout << "  ENTRY key=" << entry.m_name << " handle=0x"
                      << std::hex << entry.m_handle << std::dec << '\n';
    }
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    dx_data data;
    Probe probe;
    const bool read = probe.fileImport(argv[1], &data, false);
    for (const auto* block : data.blocks) {
        if (!block || block->name != "B1") continue;
        std::cout << "BLOCK_MEMBERS name=B1 handle=0x" << std::hex
                  << block->handle << std::dec
                  << " entities=" << block->ent.size() << '\n';
        for (const auto* entity : block->ent) {
            if (!entity) continue;
            std::cout << "  MEMBER handle=0x" << std::hex << entity->handle
                      << " owner=0x" << entity->parentHandle << std::dec
                      << " type=" << static_cast<int>(entity->eType) << '\n';
        }
    }
    std::cout << "READ=" << read << '\n';
    return read ? 0 : 1;
}
