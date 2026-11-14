// SPDX-License-Identifier: GPL-2.0-or-later
// Generated TABLECONTENT OBJECTS allocation-failure rollback and retry.
#include "lc3_compat_check.cpp"
#include "dx_iface.h"
#include "dwgbufferw.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
std::size_t failedAllocations = 0;

struct AllocationFailure {
    explicit AllocationFailure(long count) { allocationsBeforeFailure = count; }
    ~AllocationFailure() { allocationsBeforeFailure = -1; }
};

class TableContentEncoderProbe final : public DRW_TableContentObject {
public:
    using DRW_TableContentObject::encodeDwg;
};

bool encodeDirect(const TableContentEncoderProbe& source,
                  dwgBufferW& body, dwgBufferW& strings,
                  dwgBufferW& handles) {
    return source.encodeDwg(DRW::AC1021, &body, &strings, &handles);
}

bool sameBuffer(const dwgBufferW& actual, const dwgBufferW& expected) {
    if (actual.data() != expected.data()
        || actual.bitPos() != expected.bitPos()
        || actual.bitCount() != expected.bitCount()
        || actual.handleOccurrences().size()
            != expected.handleOccurrences().size())
        return false;
    for (std::size_t i = 0; i < actual.handleOccurrences().size(); ++i) {
        const auto& a = actual.handleOccurrences()[i];
        const auto& e = expected.handleOccurrences()[i];
        if (a.code != e.code || a.reference != e.reference
            || a.startBit != e.startBit || a.endBit != e.endBit)
            return false;
    }
    return true;
}

bool runDirectEncoderAllocationSweep() {
    TableContentEncoderProbe source;
    source.m_semanticContentComplete = true;
    source.m_content.m_name = "direct allocation table";
    source.m_content.m_description = "direct rollback";
    DRW_TableColumn column;
    column.m_name = "Value";
    source.m_content.m_columns.push_back(column);
    DRW_TableCell cell;
    DRW_TableRow row;
    row.m_cells.push_back(cell);
    source.m_content.m_rows.push_back(row);

    dwgBufferW body, strings, handles;
    body.putBit(1);
    strings.putBit(0);
    handles.putBit(1);
    dwgHandle seedHandle;
    seedHandle.code = DRW::DwgHardPointer;
    seedHandle.ref = 0x7711u;
    handles.putHandle(seedHandle);
    const dwgBufferW cleanBody = body;
    const dwgBufferW cleanStrings = strings;
    const dwgBufferW cleanHandles = handles;

    long allocationCount = 0;
    const std::size_t failedBeforeCalibration = failedAllocations;
    {
        AllocationFailure failure(1000000);
        if (!encodeDirect(source, body, strings, handles))
            return false;
        allocationCount = 1000000 - allocationsBeforeFailure;
        if (failedAllocations != failedBeforeCalibration)
            return false;
    }
    if (allocationCount <= 0)
        return false;
    const dwgBufferW expectedBody = body;
    const dwgBufferW expectedStrings = strings;
    const dwgBufferW expectedHandles = handles;

    for (long offset = 0; offset < allocationCount; ++offset) {
        body = cleanBody;
        strings = cleanStrings;
        handles = cleanHandles;
        const std::size_t failedBefore = failedAllocations;
        bool encoded = false;
        {
            AllocationFailure failure(offset);
            encoded = encodeDirect(source, body, strings, handles);
        }
        if (encoded || failedAllocations != failedBefore + 1u
            || !sameBuffer(body, cleanBody)
            || !sameBuffer(strings, cleanStrings)
            || !sameBuffer(handles, cleanHandles))
            return false;
        if (!encodeDirect(source, body, strings, handles)
            || !sameBuffer(body, expectedBody)
            || !sameBuffer(strings, expectedStrings)
            || !sameBuffer(handles, expectedHandles))
            return false;
    }
    std::cout << "TABLECONTENT direct encoder: allocation offsets="
              << allocationCount << ", rollback/retry=PASS\n";
    return true;
}

class TableContentExport final : public LC3CompatCheck {
public:
    explicit TableContentExport(dxfRW& output) : output(output) {
        content.handle = 0x7100u;
        content.m_parseComplete = true;
        content.m_semanticContentComplete = true;
        content.m_content.m_name = "generated allocation table";
        content.m_content.m_description = "single-cell rollback control";
        DRW_TableColumn column;
        column.m_name = "Value";
        content.m_content.m_columns.push_back(column);
        DRW_TableCellContent value;
        value.m_type = 1;
        value.m_text = "allocation retry";
        value.m_value.m_dataType = 4;
        value.m_value.m_value.addString(1, value.m_text);
        DRW_TableCell cell;
        cell.m_contents.push_back(value);
        DRW_TableRow row;
        row.m_cells.push_back(cell);
        content.m_content.m_rows.push_back(row);
    }

    dxfRW& output;
    DRW_TableContentObject content;
    long failAt = -1;
    long reached = 0;
    bool emitted = false;

    void writeObjects() override {
        const auto emit = [this]() {
            emitted = output.writeTableContent(&content);
        };
        if (failAt < 0) {
            emit();
            return;
        }
        const auto before = failedAllocations;
        try {
            AllocationFailure failure(failAt);
            emit();
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
        } catch (const std::bad_alloc&) {
            reached = failedAllocations != before
                ? static_cast<long>(failedAllocations - before)
                : failAt - allocationsBeforeFailure;
            throw;
        }
    }

    bool finalizeDxfWrite() override { return emitted; }
};

bool readAll(const std::filesystem::path& path, std::string& value) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    value.assign(std::istreambuf_iterator<char>(input),
                 std::istreambuf_iterator<char>());
    return !input.bad();
}

bool write(TableContentExport& exporter, dxfRW& codec,
           DRW::Version version, bool binary) {
    return codec.write(&exporter, version, binary);
}

bool verify(const std::filesystem::path& path) {
    dx_data data;
    dx_iface reader;
    const bool imported = reader.fileImport(path.string(), &data, false);
    if (!imported || data.tableContents.size() != 1u)
        return false;
    const DRW_TableContentObject& actual = data.tableContents.front();
    if (actual.handle != 0x7100u || !actual.m_parseComplete
        || actual.m_content.m_name != "generated allocation table"
        || actual.m_content.m_description != "single-cell rollback control"
        || actual.m_content.m_columns.size() != 1u
        || actual.m_content.m_columns.front().m_name != "Value"
        || actual.m_content.m_rows.size() != 1u
        || actual.m_content.m_rows.front().m_cells.size() != 1u)
        return false;
    const auto& contents = actual.m_content.m_rows.front().m_cells.front().m_contents;
    return contents.size() == 1u && contents.front().m_type == 1
        && contents.front().m_text == "allocation retry"
        && contents.front().m_value.m_dataType == 4
        && contents.front().m_value.m_value.type() == DRW_Variant::STRING
        && contents.front().m_value.m_value.c_str()
            == std::string("allocation retry");
}

bool sameValue(const DRW_CadValue& a, const DRW_CadValue& b) {
    return a.m_formatFlags == b.m_formatFlags
        && a.m_dataType == b.m_dataType && a.m_dataSize == b.m_dataSize
        && a.m_unitType == b.m_unitType && a.m_value.type() == b.m_value.type()
        && a.m_value.code() == b.m_value.code()
        && (a.m_value.type() != DRW_Variant::STRING
            || a.m_value.c_str() == std::string(b.m_value.c_str()))
        && a.m_formatString == b.m_formatString
        && a.m_valueString == b.m_valueString && a.m_handle == b.m_handle
        && a.m_rawData == b.m_rawData;
}

bool sameCell(const DRW_TableCell& a, const DRW_TableCell& b) {
    if (a.m_flags != b.m_flags || a.m_type != b.m_type
        || a.m_edgeFlags != b.m_edgeFlags || a.m_isMerged != b.m_isMerged
        || a.m_autoFit != b.m_autoFit || a.m_mergedWidth != b.m_mergedWidth
        || a.m_mergedHeight != b.m_mergedHeight || a.m_rotation != b.m_rotation
        || a.m_toolTip != b.m_toolTip || a.m_textStyleName != b.m_textStyleName
        || a.m_styleId != b.m_styleId
        || a.m_overrideStyleType != b.m_overrideStyleType
        || a.m_virtualEdgeFlags != b.m_virtualEdgeFlags
        || a.m_overrideFlags != b.m_overrideFlags
        || a.m_valueHandle != b.m_valueHandle
        || a.m_textStyleHandle != b.m_textStyleHandle
        || a.m_textStyleOverrideHandle != b.m_textStyleOverrideHandle
        || a.m_blockHandle != b.m_blockHandle
        || a.m_hasLinkedData != b.m_hasLinkedData
        || a.m_dataLinkHandle != b.m_dataLinkHandle
        || a.m_linkedDataRowCount != b.m_linkedDataRowCount
        || a.m_linkedDataColumnCount != b.m_linkedDataColumnCount
        || a.m_linkedDataUnknown != b.m_linkedDataUnknown
        || a.m_blockScale != b.m_blockScale || a.m_width != b.m_width
        || a.m_height != b.m_height || a.m_geometryFlags != b.m_geometryFlags
        || a.m_geometryHandle != b.m_geometryHandle
        || a.m_geometryRecordFlags != b.m_geometryRecordFlags
        || a.m_geometryTopLeft.x != b.m_geometryTopLeft.x
        || a.m_geometryTopLeft.y != b.m_geometryTopLeft.y
        || a.m_geometryTopLeft.z != b.m_geometryTopLeft.z
        || a.m_geometryCenter.x != b.m_geometryCenter.x
        || a.m_geometryCenter.y != b.m_geometryCenter.y
        || a.m_geometryCenter.z != b.m_geometryCenter.z
        || a.m_contentWidth != b.m_contentWidth
        || a.m_contentHeight != b.m_contentHeight
        || a.m_geometryWidth != b.m_geometryWidth
        || a.m_geometryHeight != b.m_geometryHeight
        || a.m_textColorRgb != b.m_textColorRgb
        || a.m_textColorMethod != b.m_textColorMethod
        || a.m_fillColorRgb != b.m_fillColorRgb
        || a.m_fillColorMethod != b.m_fillColorMethod
        || a.m_backgroundEnabled != b.m_backgroundEnabled
        || a.m_textAlignment != b.m_textAlignment
        || a.m_textHeight != b.m_textHeight
        || a.m_attributes.size() != b.m_attributes.size()
        || a.m_contents.size() != b.m_contents.size())
        return false;
    for (std::size_t i = 0; i < a.m_borders.size(); ++i) {
        const auto& left = a.m_borders[i];
        const auto& right = b.m_borders[i];
        if (left.m_lineWeight != right.m_lineWeight
            || left.m_visible != right.m_visible
            || left.m_colorRgb != right.m_colorRgb
            || left.m_colorMethod != right.m_colorMethod
            || left.m_hasDxfVisibility != right.m_hasDxfVisibility)
            return false;
    }
    for (std::size_t i = 0; i < a.m_attributes.size(); ++i) {
        const auto& left = a.m_attributes[i];
        const auto& right = b.m_attributes[i];
        if (left.m_attdefHandle != right.m_attdefHandle
            || left.m_index != right.m_index || left.m_text != right.m_text
            || left.m_contentIndex != right.m_contentIndex)
            return false;
    }
    for (std::size_t i = 0; i < a.m_contents.size(); ++i) {
        const auto& left = a.m_contents[i];
        const auto& right = b.m_contents[i];
        if (left.m_type != right.m_type || left.m_text != right.m_text
            || !sameValue(left.m_value, right.m_value)
            || left.m_handle != right.m_handle)
            return false;
    }
    return true;
}

bool sourceUnchanged(const TableContentExport& actual,
                     const TableContentExport& expected) {
    const auto& a = actual.content;
    const auto& b = expected.content;
    if (a.tType != b.tType || a.handle != b.handle
        || a.parentHandle != b.parentHandle || a.name != b.name
        || a.flags != b.flags || a.reactorHandles != b.reactorHandles
        || a.xDictHandle != b.xDictHandle
        || !a.pendingAppIdResolutions.empty()
        || !b.pendingAppIdResolutions.empty()
        || !a.pendingLayerRefResolutions.empty()
        || !b.pendingLayerRefResolutions.empty()
        || a.extensionDictionaryFlag() != b.extensionDictionaryFlag()
        || a.reactorCount() != b.reactorCount()
        || a.hasDataStorageBinaryData() != b.hasDataStorageBinaryData()
        || a.hasDwgCommonLinkTail() != b.hasDwgCommonLinkTail()
        || !a.extData.empty()
        || !b.extData.empty() || !a.appData.empty() || !b.appData.empty()
        || a.m_parseComplete != b.m_parseComplete
        || a.m_semanticContentComplete != b.m_semanticContentComplete
        || a.m_dxfMetadata != b.m_dxfMetadata
        || a.m_content.m_name != b.m_content.m_name
        || a.m_content.m_description != b.m_content.m_description
        || a.m_content.m_tableStyleHandle != b.m_content.m_tableStyleHandle
        || a.m_content.m_overrideStyleType != b.m_content.m_overrideStyleType
        || a.m_content.m_fieldHandles != b.m_content.m_fieldHandles
        || a.m_content.m_columns.size() != b.m_content.m_columns.size()
        || a.m_content.m_rows.size() != b.m_content.m_rows.size()
        || a.m_content.m_mergedRanges.size() != b.m_content.m_mergedRanges.size()
        || a.m_content.m_subrecordRanges.size() != b.m_content.m_subrecordRanges.size())
        return false;
    for (std::size_t i = 0; i < a.m_content.m_columns.size(); ++i) {
        const auto& left = a.m_content.m_columns[i];
        const auto& right = b.m_content.m_columns[i];
        if (left.m_name != right.m_name || left.m_width != right.m_width
            || left.m_styleId != right.m_styleId
            || left.m_overrideStyleType != right.m_overrideStyleType)
            return false;
    }
    for (std::size_t i = 0; i < a.m_content.m_rows.size(); ++i) {
        const auto& left = a.m_content.m_rows[i];
        const auto& right = b.m_content.m_rows[i];
        if (left.m_height != right.m_height || left.m_styleId != right.m_styleId
            || left.m_overrideStyleType != right.m_overrideStyleType
            || left.m_cells.size() != right.m_cells.size())
            return false;
        for (std::size_t j = 0; j < left.m_cells.size(); ++j) {
            if (!sameCell(left.m_cells[j], right.m_cells[j]))
                return false;
        }
    }
    for (std::size_t i = 0; i < a.m_content.m_mergedRanges.size(); ++i) {
        const auto& left = a.m_content.m_mergedRanges[i];
        const auto& right = b.m_content.m_mergedRanges[i];
        if (left.m_topRow != right.m_topRow
            || left.m_leftColumn != right.m_leftColumn
            || left.m_bottomRow != right.m_bottomRow
            || left.m_rightColumn != right.m_rightColumn)
            return false;
    }
    for (std::size_t i = 0; i < a.m_content.m_subrecordRanges.size(); ++i) {
        const auto& left = a.m_content.m_subrecordRanges[i];
        const auto& right = b.m_content.m_subrecordRanges[i];
        if (left.m_name != right.m_name || left.m_startBit != right.m_startBit
            || left.m_bitSize != right.m_bitSize || left.m_version != right.m_version
            || left.m_count != right.m_count
            || left.m_rawHandleCode != right.m_rawHandleCode
            || left.m_parseComplete != right.m_parseComplete)
            return false;
    }
    return true;
}

bool run(const std::filesystem::path& directory) {
    const auto fail = [](const char* message) {
        std::cerr << "TABLECONTENT OBJECTS allocation retry: " << message << '\n';
        return false;
    };
    if (!std::filesystem::create_directories(directory)
        && !std::filesystem::is_directory(directory))
        return fail("could not create scratch directory");
    if (!runDirectEncoderAllocationSweep())
        return fail("direct encoder allocation rollback/retry failed");
    const std::string sentinel = "prior TABLECONTENT destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      const auto output = directory
          / ("tablecontent-objects-"
             + std::to_string(static_cast<int>(version)) + ".dxf");
      dxfRW codec(output.string().c_str());
      TableContentExport exporter(codec);
      TableContentExport expected(codec);

      for (const bool binary : {false, true}) {
        if (!write(exporter, codec, version, binary))
            return fail("warm-up export was refused");
        exporter.failAt = 1000000;
        if (!write(exporter, codec, version, binary))
            return fail("allocation calibration export was refused");
        if (!verify(output))
            return fail("calibration output did not read back typed content");
        const long allocationCount = exporter.reached;
        if (allocationCount <= 0)
            return fail("calibration observed no callback allocations");
        for (long offset = 0; offset < allocationCount; ++offset) {
            {
                std::ofstream prior(output, std::ios::binary | std::ios::trunc);
                prior << sentinel;
                if (!prior)
                    return fail("could not initialize destination sentinel");
            }
            exporter.failAt = offset;
            exporter.reached = 0;
            exporter.emitted = false;
            const bool wrote = write(exporter, codec, version, binary);
            exporter.failAt = -1;
            std::string current;
            if (wrote || exporter.reached < 1 || !readAll(output, current)
                || current != sentinel)
                return fail("injected failure changed destination or was missed");
            if (!sourceUnchanged(exporter, expected))
                return fail("injected failure changed the caller object");
            if (!write(exporter, codec, version, binary) || !verify(output))
                return fail("same-writer retry failed typed readback");
            if (!sourceUnchanged(exporter, expected))
                return fail("same-writer retry changed the caller object");
        }
        std::cout << "TABLECONTENT OBJECTS "
                  << static_cast<int>(version) << ' '
                  << (binary ? "Binary" : "ASCII")
                  << ": allocation offsets=" << allocationCount
                  << ", rollback/retry/readback=PASS\n";
      }
      std::error_code ignored;
      std::filesystem::remove(output, ignored);
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        ++failedAllocations;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* value = std::malloc(size == 0 ? 1 : size))
        return value;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: tablecontent_objects_allocation_tests <scratch-dir>\n";
        return 2;
    }
    return run(argv[1]) ? 0 : 1;
}
