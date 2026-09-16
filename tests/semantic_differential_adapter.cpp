/******************************************************************************
** Deterministic semantic-evidence adapter for libdxfrw.                     **
**                                                                           **
** This source is deliberately limited to the public libdxfrw API and C++17 **
** so the same translation unit can be compiled against the standalone      **
** package and LibreCAD's pinned bundled package.                            **
******************************************************************************/

#include "drw_interface.h"
#include "libdwgr.h"
#include "libdxfrw.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifndef LIBDXFRW_SEMANTIC_ADAPTER_NAME
#define LIBDXFRW_SEMANTIC_ADAPTER_NAME "libdxfrw-semantic-adapter"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE
#define LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE "libdxfrw"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_COMMIT
#define LIBDXFRW_SEMANTIC_ADAPTER_COMMIT "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST "unknown"
#endif
#ifndef LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST
#define LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST "unknown"
#endif

namespace {

class Sha256 {
public:
    Sha256() { reset(); }

    void update(const void* data, std::size_t size) {
        const std::uint8_t* bytes = static_cast<const std::uint8_t*>(data);
        total_ += static_cast<std::uint64_t>(size);
        while (size != 0) {
            const std::size_t take = std::min(size, block_.size() - used_);
            std::memcpy(block_.data() + used_, bytes, take);
            used_ += take;
            bytes += take;
            size -= take;
            if (used_ == block_.size()) {
                transform(block_.data());
                used_ = 0;
            }
        }
    }

    void update(const std::string& value) { update(value.data(), value.size()); }

    std::string finish() {
        const std::uint64_t bitCount = total_ * 8u;
        const std::uint8_t marker = 0x80;
        update(&marker, 1);
        const std::uint8_t zero = 0;
        while (used_ != 56)
            update(&zero, 1);
        std::array<std::uint8_t, 8> length{};
        for (std::size_t i = 0; i < length.size(); ++i)
            length[7 - i] = static_cast<std::uint8_t>(bitCount >> (i * 8));
        update(length.data(), length.size());

        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << std::hex << std::setfill('0');
        for (std::uint32_t word : state_)
            out << std::setw(8) << word;
        const std::string result = out.str();
        reset();
        return result;
    }

private:
    static std::uint32_t rotate(std::uint32_t value, unsigned count) {
        return (value >> count) | (value << (32u - count));
    }

    void reset() {
        state_ = {{0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u}};
        block_.fill(0);
        used_ = 0;
        total_ = 0;
    }

    void transform(const std::uint8_t* input) {
        static const std::uint32_t constants[64] = {
            0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
            0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
            0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
            0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
            0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
            0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
            0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
            0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
        };
        std::uint32_t words[64];
        for (std::size_t i = 0; i < 16; ++i) {
            words[i] = (static_cast<std::uint32_t>(input[i * 4]) << 24) |
                       (static_cast<std::uint32_t>(input[i * 4 + 1]) << 16) |
                       (static_cast<std::uint32_t>(input[i * 4 + 2]) << 8) |
                       static_cast<std::uint32_t>(input[i * 4 + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            const std::uint32_t s0 = rotate(words[i - 15], 7) ^
                                     rotate(words[i - 15], 18) ^
                                     (words[i - 15] >> 3);
            const std::uint32_t s1 = rotate(words[i - 2], 17) ^
                                     rotate(words[i - 2], 19) ^
                                     (words[i - 2] >> 10);
            words[i] = words[i - 16] + s0 + words[i - 7] + s1;
        }
        std::uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        std::uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];
        for (std::size_t i = 0; i < 64; ++i) {
            const std::uint32_t s1 = rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25);
            const std::uint32_t choose = (e & f) ^ (~e & g);
            const std::uint32_t temp1 = h + s1 + choose + constants[i] + words[i];
            const std::uint32_t s0 = rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22);
            const std::uint32_t majority = (a & b) ^ (a & c) ^ (b & c);
            const std::uint32_t temp2 = s0 + majority;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    std::array<std::uint32_t, 8> state_{};
    std::array<std::uint8_t, 64> block_{};
    std::size_t used_ = 0;
    std::uint64_t total_ = 0;
};

std::string sha256(const void* data, std::size_t size) {
    Sha256 hash;
    hash.update(data, size);
    return hash.finish();
}

std::string sha256(const std::string& value) {
    return sha256(value.data(), value.size());
}

std::string jsonString(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (unsigned char ch : value) {
        switch (ch) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\b': out << "\\b"; break;
        case '\f': out << "\\f"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (ch < 0x20)
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<unsigned>(ch) << std::dec;
            else
                out << static_cast<char>(ch);
        }
    }
    out << '"';
    return out.str();
}

std::string jsonNullable(const std::string& value) {
    return value.empty() ? "null" : jsonString(value);
}

std::string number(double value) {
    if (!std::isfinite(value))
        throw std::domain_error("non-finite semantic number");
    if (value == 0.0)
        value = 0.0; // normalize negative zero
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return out.str();
}

void appendLengthFramed(std::string& destination, const std::string& value) {
    destination += std::to_string(value.size());
    destination.push_back(':');
    destination += value;
}

std::string decimal(std::int64_t value) {
    return std::to_string(value);
}

std::string unsignedDecimal(std::uint64_t value) {
    return std::to_string(value);
}

std::string hexHandle(std::uint32_t value) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::hex << std::nouppercase << value;
    return out.str();
}

std::string indexedId(const char* prefix, std::size_t index) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << prefix << ':' << std::setw(8) << std::setfill('0') << index;
    return out.str();
}

struct Field {
    std::string name;
    std::string type;
    std::string value;
};

Field stringField(const std::string& name, const std::string& value) {
    return Field{name, "string", jsonString(value)};
}
Field handleField(const std::string& name, std::uint32_t value) {
    return Field{name, "handle", jsonString(hexHandle(value))};
}
void appendIdentityHandleFields(std::vector<Field>& fields, std::uint32_t handle,
                                std::uint32_t ownerHandle) {
    if (handle != DRW::NoHandle)
        fields.push_back(handleField("handle", handle));
    if (ownerHandle != DRW::NoHandle)
        fields.push_back(handleField("ownerHandle", ownerHandle));
}
Field intField(const std::string& name, std::int64_t value) {
    return Field{name, "int64", decimal(value)};
}
Field uintField(const std::string& name, std::uint64_t value) {
    return Field{name, "uint64", unsignedDecimal(value)};
}
Field doubleField(const std::string& name, double value) {
    return Field{name, "double", number(value)};
}
Field boolField(const std::string& name, bool value) {
    return Field{name, "bool", value ? "true" : "false"};
}
Field pointField(const std::string& name, const DRW_Coord& value) {
    return Field{name, "point3d", std::string("{\"x\":") + number(value.x) +
                 ",\"y\":" + number(value.y) + ",\"z\":" +
                 number(value.z) + "}"};
}

struct CallbackRow {
    std::string id;
    std::size_t ordinal = 0;
    std::string kind;
    std::string recordId;
    std::string blockContextId;
    std::vector<std::string> carrierIds;
};
struct RecordRow {
    std::string id;
    std::size_t callbackOrdinal = 0;
    std::string kind;
    std::string recordClass;
    std::string entity;
    std::string sourceHandle;
    std::string ownerHandle;
    std::string blockId;
    std::vector<Field> fields;
};
struct NodeRow {
    std::string id;
    std::string kind;
    std::string recordId;
    std::string handle;
};
struct EdgeRow {
    std::string id;
    std::string kind;
    std::string from;
    std::string to;
    std::string disposition;
};
struct CarrierRow {
    std::string id;
    std::string source;
    std::size_t size = 0;
    std::string digest;
    std::string disposition;
    std::string recordId;
    std::string sourceHandle;
};
struct UnsupportedRow {
    std::string id;
    std::string kind;
    std::string source;
    std::string recordId;
    std::string carrierId;
    std::string diagnosticId;
    std::string disposition;
    std::string reason;
};
struct DiagnosticRow {
    std::string id;
    std::size_t index = 0;
    std::string severity;
    std::string stage;
    std::string code;
    std::string path;
    std::string messageDigest;
};

class SemanticSink final : public DRW_Interface {
public:
    std::vector<CallbackRow> callbacks;
    std::vector<RecordRow> records;
    std::vector<NodeRow> nodes;
    std::vector<EdgeRow> edges;
    std::vector<CarrierRow> carriers;
    std::vector<UnsupportedRow> unsupported;
    std::vector<DiagnosticRow> diagnostics;

    void configureWrite(dxfRW* writer) {
        dxfWriter_ = writer;
        dwgWriter_ = nullptr;
        writeRecipeSucceeded_ = true;
    }
    void configureWrite(dwgRW* writer) {
        dxfWriter_ = nullptr;
        dwgWriter_ = writer;
        writeRecipeSucceeded_ = true;
    }
    bool writeRecipeSucceeded() const { return writeRecipeSucceeded_; }

    void addHeader(const DRW_Header*) override {
        addCallback("addHeader");
        addUnsupported("headerPayload", "header", std::string(), std::string(),
                       std::string(), "excluded");
    }
    void addLType(const DRW_LType& d) override { addTable("addLType", "LTYPE", d); }
    void addLayer(const DRW_Layer& d) override { addTable("addLayer", "LAYER", d); }
    void addDimStyle(const DRW_Dimstyle& d) override { addTable("addDimStyle", "DIMSTYLE", d); }
    void addVport(const DRW_Vport& d) override { addTable("addVport", "VPORT", d); }
    void addTextStyle(const DRW_Textstyle& d) override { addTable("addTextStyle", "STYLE", d); }
    void addAppId(const DRW_AppId& d) override { addTable("addAppId", "APPID", d); }

#define SEMANTIC_OPAQUE_VALUE(method, type, token) \
    void method(const type& value) override { \
        (void)value; \
        addOpaqueTyped(#method, token); \
    }
#define SEMANTIC_OPAQUE_POINTER(method, type, token) \
    void method(const type* value) override { \
        if (value == nullptr) addNullDiagnostic(#method); \
        else addOpaqueTyped(#method, token); \
    }

    SEMANTIC_OPAQUE_VALUE(addViewportEntityHeader, DRW_ViewportEntityHeader, "VPORT_ENTITY_HEADER")
    SEMANTIC_OPAQUE_VALUE(addVxControl, DRW_VxControl, "VX_CONTROL")
    SEMANTIC_OPAQUE_VALUE(addVxTableRecord, DRW_VxTableRecord, "VX_TABLE_RECORD")
    SEMANTIC_OPAQUE_VALUE(addTvDeviceProperties, DRW_TvDeviceProperties, "TVDEVICEPROPERTIES")
    SEMANTIC_OPAQUE_VALUE(addCsacDocumentOptions, DRW_CsacDocumentOptions, "CSACDOCUMENTOPTIONS")
    SEMANTIC_OPAQUE_VALUE(addContextDataManager, DRW_ContextDataManager, "CONTEXTDATAMANAGER")
    SEMANTIC_OPAQUE_VALUE(addUCS, DRW_UCS, "UCS")
    SEMANTIC_OPAQUE_VALUE(addView, DRW_View, "VIEW")
    SEMANTIC_OPAQUE_VALUE(addTolerance, DRW_Tolerance, "TOLERANCE")
    SEMANTIC_OPAQUE_VALUE(addDictionary, DRW_Dictionary, "DICTIONARY")
    SEMANTIC_OPAQUE_VALUE(addDictionaryWithDefault, DRW_DictionaryWithDefault, "DICTIONARYWDFLT")
    SEMANTIC_OPAQUE_VALUE(addDictionaryVar, DRW_DictionaryVar, "DICTIONARYVAR")
    SEMANTIC_OPAQUE_VALUE(addXRecord, DRW_XRecord, "XRECORD")
    SEMANTIC_OPAQUE_VALUE(addField, DRW_Field, "FIELD")
    SEMANTIC_OPAQUE_VALUE(addFieldList, DRW_FieldList, "FIELDLIST")
    SEMANTIC_OPAQUE_VALUE(addDataTable, DRW_DataTable, "DATATABLE")
    SEMANTIC_OPAQUE_VALUE(addDynamicBlockObject, DRW_DynamicBlockObject, "DYNAMIC_BLOCK_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addRasterVariables, DRW_RasterVariables, "RASTERVARIABLES")
    SEMANTIC_OPAQUE_VALUE(addWipeoutVariables, DRW_WipeoutVariables, "WIPEOUTVARIABLES")
    SEMANTIC_OPAQUE_VALUE(addSortEntsTable, DRW_SortEntsTable, "SORTENTSTABLE")
    SEMANTIC_OPAQUE_VALUE(addMaterial, DRW_Material, "MATERIAL")
    SEMANTIC_OPAQUE_VALUE(addTableStyle, DRW_TableStyle, "TABLESTYLE")
    SEMANTIC_OPAQUE_VALUE(addTableContent, DRW_TableContentObject, "TABLECONTENT")
    SEMANTIC_OPAQUE_VALUE(addObjectContextData, DRW_ObjectContextData, "OBJECTCONTEXTDATA")
    SEMANTIC_OPAQUE_VALUE(addCellStyleMap, DRW_CellStyleMap, "CELLSTYLEMAP")
    SEMANTIC_OPAQUE_VALUE(addLayout, DRW_Layout, "LAYOUT")
    SEMANTIC_OPAQUE_VALUE(addMLineStyle, DRW_MLineStyle, "MLINESTYLE")
    SEMANTIC_OPAQUE_VALUE(addDwgFramePublication, DRW_DwgFramePublication, "DWG_FRAME_PUBLICATION")
    SEMANTIC_OPAQUE_VALUE(addDwgDictionaryMembership, DRW_DwgDictionaryMembership, "DWG_DICTIONARY_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgDictionaryWithDefaultMembership, DRW_DwgDictionaryWithDefaultMembership, "DWG_DICTIONARY_DEFAULT_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgGroupMembership, DRW_DwgGroupMembership, "DWG_GROUP_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgSortEntsMembership, DRW_DwgSortEntsMembership, "DWG_SORTENTS_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgFieldListMembership, DRW_DwgFieldListMembership, "DWG_FIELDLIST_MEMBERSHIP")
    SEMANTIC_OPAQUE_VALUE(addDwgFieldPayloadReceipt, DRW_DwgFieldPayloadReceipt, "DWG_FIELD_PAYLOAD_RECEIPT")
    SEMANTIC_OPAQUE_VALUE(addDwgTypedReference, DRW_DwgTypedReference, "DWG_TYPED_REFERENCE")
    SEMANTIC_OPAQUE_VALUE(addDwgBlockReachability, DRW_DwgBlockReachability, "DWG_BLOCK_REACHABILITY")
    SEMANTIC_OPAQUE_VALUE(addDwgFrameCoverageReport, DRW_DwgFrameCoverageReport, "DWG_FRAME_COVERAGE")
    SEMANTIC_OPAQUE_VALUE(addDwgClassCoverageReport, DRW_DwgClassCoverageReport, "DWG_CLASS_COVERAGE")
    SEMANTIC_OPAQUE_VALUE(addDataStorage, DRW_DataStorageSection, "DATASTORAGE")
    SEMANTIC_OPAQUE_VALUE(addDxfClass, DRW_Class, "DXF_CLASS")

    void addBlock(const DRW_Block& d) override {
        std::vector<Field> fields = entityFields(d, "BLOCK", "BLOCK");
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("flags", d.flags));
        const std::size_t recordIndex = addRecord("addBlock", "block", "BLOCK", fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        currentBlockId_ = "node:" + records[recordIndex].id;
        for (NodeRow& node : nodes) {
            if (node.id == currentBlockId_) {
                node.kind = "block";
                break;
            }
        }
        addExcluded(records[recordIndex].id, "unmodeledBlockPayload");
    }
    void setBlock(int handle) override {
        const std::string handleNode = ensureHandleNode(static_cast<std::uint32_t>(handle));
        for (NodeRow& node : nodes) {
            if (node.id == handleNode) {
                node.kind = "block";
                break;
            }
        }
        addCallback("setBlock", std::string(), handleNode);
        currentBlockId_ = handleNode;
    }
    void endBlock() override {
        addCallback("endBlock", std::string(), currentBlockId_);
        currentBlockId_.clear();
    }

    void addPoint(const DRW_Point& d) override {
        std::vector<Field> fields = entityFields(d, "POINT", "POINT");
        fields.push_back(pointField("basePoint", d.basePoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        addEntityRecord("addPoint", "POINT", fields, d, false);
    }
    void addLine(const DRW_Line& d) override {
        std::vector<Field> fields = entityFields(d, "LINE", "LINE");
        fields.push_back(pointField("start", d.basePoint));
        fields.push_back(pointField("end", d.secPoint));
        fields.push_back(pointField("extrusion", d.extPoint));
        fields.push_back(doubleField("thickness", d.thickness));
        addEntityRecord("addLine", "LINE", fields, d, false);
    }
    SEMANTIC_OPAQUE_VALUE(add3DLine, DRW_3DLine, "3DLINE")
    void addRay(const DRW_Ray& d) override { addLineLike("addRay", "RAY", d); }
    void addXline(const DRW_Xline& d) override { addLineLike("addXline", "XLINE", d); }
    void addArc(const DRW_Arc& d) override { addBasicEntity("addArc", "ARC", d); }
    void addCircle(const DRW_Circle& d) override { addBasicEntity("addCircle", "CIRCLE", d); }
    void addEllipse(const DRW_Ellipse& d) override { addBasicEntity("addEllipse", "ELLIPSE", d); }
    void addLWPolyline(const DRW_LWPolyline& d) override { addBasicEntity("addLWPolyline", "LWPOLYLINE", d); }
    SEMANTIC_OPAQUE_POINTER(addMLine, DRW_MLine, "MLINE")
    SEMANTIC_OPAQUE_POINTER(addUnderlay, DRW_Underlay, "UNDERLAY")
    SEMANTIC_OPAQUE_VALUE(addShape, DRW_Shape, "SHAPE")
    SEMANTIC_OPAQUE_VALUE(addOle2Frame, DRW_Ole2Frame, "OLE2FRAME")
    SEMANTIC_OPAQUE_VALUE(addOleFrame, DRW_OleFrame, "OLEFRAME")
    SEMANTIC_OPAQUE_VALUE(addProxyEntity, DRW_ProxyEntity, "PROXY_ENTITY")
    SEMANTIC_OPAQUE_POINTER(linkUnderlay, DRW_UnderlayDefinition, "UNDERLAYDEFINITION")
    void addPolyline(const DRW_Polyline& d) override { addBasicEntity("addPolyline", "POLYLINE", d); }
    void addSpline(const DRW_Spline* d) override { addPointerEntity("addSpline", "SPLINE", d); }
    SEMANTIC_OPAQUE_POINTER(addHelix, DRW_Helix, "HELIX")
    SEMANTIC_OPAQUE_VALUE(addMesh, DRW_Mesh, "MESH")
    void addKnot(const DRW_Entity& d) override { addBasicEntity("addKnot", "KNOT", d); }
    void addInsert(const DRW_Insert& d) override { addBasicEntity("addInsert", "INSERT", d); }
    SEMANTIC_OPAQUE_VALUE(addTable, DRW_Table, "TABLE")
    void addTrace(const DRW_Trace& d) override { addBasicEntity("addTrace", "TRACE", d); }
    void add3dFace(const DRW_3Dface& d) override { addBasicEntity("add3dFace", "3DFACE", d); }
    SEMANTIC_OPAQUE_VALUE(addModelerGeometry, DRW_ModelerGeometry, "MODELER_GEOMETRY")
    SEMANTIC_OPAQUE_VALUE(addLight, DRW_Light, "LIGHT")
    SEMANTIC_OPAQUE_VALUE(addCamera, DRW_Camera, "CAMERA")
    SEMANTIC_OPAQUE_VALUE(addGeoPositionMarker, DRW_GeoPositionMarker, "GEOPOSITIONMARKER")
    SEMANTIC_OPAQUE_VALUE(addSectionObject, DRW_SectionObject, "SECTIONOBJECT")
    void addSolid(const DRW_Solid& d) override { addBasicEntity("addSolid", "SOLID", d); }
    void addMText(const DRW_MText& d) override {
        std::vector<Field> fields = entityFields(d, "MTEXT", "MTEXT");
        fields.push_back(stringField("text", d.text));
        fields.push_back(pointField("insertion", d.basePoint));
        fields.push_back(doubleField("height", d.height));
        fields.push_back(stringField("style", d.style));
        addEntityRecord("addMText", "MTEXT", fields, d, true);
    }
    void addText(const DRW_Text& d) override {
        if (const auto* value = dynamic_cast<const DRW_RText*>(&d)) {
            std::vector<Field> fields = entityFields(*value, "RTEXT", "RTEXT");
            fields.push_back(stringField("text", value->text));
            fields.push_back(intField("flags", value->m_rTextFlags));
            fields.push_back(pointField("insertion", value->basePoint));
            fields.push_back(pointField("extrusion", value->extPoint));
            fields.push_back(doubleField("rotation", value->angle));
            fields.push_back(doubleField("height", value->height));
            fields.push_back(stringField("style", value->style));
            addEntityRecord("addText", "RTEXT", fields, *value, false);
            return;
        }
        if (const auto* value = dynamic_cast<const DRW_ArcAlignedText*>(&d)) {
            std::vector<Field> fields = entityFields(*value, "ARCALIGNEDTEXT", "ARCALIGNEDTEXT");
            fields.push_back(stringField("text", value->text));
            fields.push_back(pointField("center", value->m_center));
            fields.push_back(doubleField("radius", value->m_radius));
            fields.push_back(doubleField("startAngle", value->m_startAngle));
            fields.push_back(doubleField("endAngle", value->m_endAngle));
            fields.push_back(stringField("textSize", value->m_textSize));
            fields.push_back(stringField("xScale", value->m_xScale));
            fields.push_back(stringField("characterSpacing", value->m_charSpacing));
            fields.push_back(stringField("offsetFromArc", value->m_offsetFromArc));
            fields.push_back(stringField("rightOffset", value->m_rightOffset));
            fields.push_back(stringField("leftOffset", value->m_leftOffset));
            fields.push_back(stringField("fontName", value->m_fontName));
            fields.push_back(stringField("bigFontName", value->m_bigFontName));
            fields.push_back(intField("rawColor", value->m_rawColor));
            fields.push_back(handleField("arcHandle", value->m_arcHandle));
            addEntityRecord("addText", "ARCALIGNEDTEXT", fields, *value, true);
            return;
        }
        std::vector<Field> fields = entityFields(d, "TEXT", "TEXT");
        fields.push_back(stringField("text", d.text));
        fields.push_back(pointField("insertion", d.basePoint));
        fields.push_back(doubleField("height", d.height));
        fields.push_back(doubleField("rotation", d.angle));
        fields.push_back(stringField("style", d.style));
        addEntityRecord("addText", "TEXT", fields, d, false);
    }
    SEMANTIC_OPAQUE_VALUE(addAttDef, DRW_Attdef, "ATTDEF")
    void addDimAlign(const DRW_DimAligned* d) override { addPointerEntity("addDimAlign", "DIMALIGNED", d); }
    void addDimLinear(const DRW_DimLinear* d) override { addPointerEntity("addDimLinear", "DIMLINEAR", d); }
    void addDimRadial(const DRW_DimRadial* d) override {
        if (d == nullptr) { addNullDiagnostic("addDimRadial"); return; }
        if (const auto* value = dynamic_cast<const DRW_DimLargeRadial*>(d)) {
            std::vector<Field> fields = entityFields(*value, "LARGE_RADIAL", "LARGE_RADIAL");
            fields.push_back(pointField("center", value->getCenterPoint()));
            fields.push_back(pointField("chord", value->getChordPoint()));
            fields.push_back(pointField("overrideCenter", value->overrideCenterPoint));
            fields.push_back(pointField("jogPoint", value->jogPoint));
            fields.push_back(doubleField("jogAngle", value->jogAngle));
            fields.push_back(doubleField("leaderLength", value->getLeaderLength()));
            addEntityRecord("addDimRadial", "LARGE_RADIAL", fields, *value, false);
            return;
        }
        addPointerEntity("addDimRadial", "DIMRADIAL", d);
    }
    void addDimDiametric(const DRW_DimDiametric* d) override { addPointerEntity("addDimDiametric", "DIMDIAMETRIC", d); }
    void addDimAngular(const DRW_DimAngular* d) override { addPointerEntity("addDimAngular", "DIMANGULAR", d); }
    void addDimAngular3P(const DRW_DimAngular3p* d) override { addPointerEntity("addDimAngular3P", "DIMANGULAR3P", d); }
    void addDimOrdinate(const DRW_DimOrdinate* d) override { addPointerEntity("addDimOrdinate", "DIMORDINATE", d); }
    void addDimArc(const DRW_DimArc* d) override { addPointerEntity("addDimArc", "DIMARC", d); }
    void addLeader(const DRW_Leader* d) override { addPointerEntity("addLeader", "LEADER", d); }
    void addHatch(const DRW_Hatch* d) override { addPointerEntity("addHatch", "HATCH", d); }
    void addMPolygon(const DRW_MPolygon* d) override {
        if (d == nullptr) { addNullDiagnostic("addMPolygon"); return; }
        std::vector<Field> fields = entityFields(*d, "MPOLYGON", "MPOLYGON");
        fields.push_back(boolField("solid", d->solid != 0));
        fields.push_back(boolField("associative", d->associative != 0));
        fields.push_back(stringField("name", d->name));
        fields.push_back(intField("hpattern", d->hpattern));
        fields.push_back(doubleField("basePoint.z", d->basePoint.z));
        fields.push_back(doubleField("extPoint.x", d->extPoint.x));
        fields.push_back(doubleField("extPoint.y", d->extPoint.y));
        fields.push_back(doubleField("extPoint.z", d->extPoint.z));
        fields.push_back(intField("fillColorAci", d->fillColorAci));
        fields.push_back(intField("fillColorRgb", d->fillColorRgb));
        fields.push_back(stringField("fillColorName", d->fillColorName));
        fields.push_back(intField("loopCount", d->loopsnum));
        addEntityRecord("addMPolygon", "MPOLYGON", fields, *d, false);
    }
    void addViewport(const DRW_Viewport& d) override { addBasicEntity("addViewport", "VIEWPORT", d); }
    void addImage(const DRW_Image* d) override { addPointerEntity("addImage", "IMAGE", d); }
    void linkImage(const DRW_ImageDef* d) override {
        if (d == nullptr) { addNullDiagnostic("linkImage"); return; }
        addTable("linkImage", "IMAGEDEF", *d);
    }
    SEMANTIC_OPAQUE_POINTER(addWipeout, DRW_Wipeout, "WIPEOUT")
    SEMANTIC_OPAQUE_POINTER(addPointCloud, DRW_PointCloud, "POINTCLOUD")
    SEMANTIC_OPAQUE_POINTER(addPointCloudEx, DRW_PointCloudEx, "POINTCLOUDEX")
    SEMANTIC_OPAQUE_POINTER(addNavisworksModel, DRW_NavisworksModel, "NAVISWORKSMODEL")
    SEMANTIC_OPAQUE_POINTER(addSurface, DRW_Surface, "SURFACE")
    SEMANTIC_OPAQUE_POINTER(addMLeader, DRW_MLeader, "MULTILEADER")
    SEMANTIC_OPAQUE_POINTER(addMLeaderStyle, DRW_MLeaderStyle, "MLEADERSTYLE")
    SEMANTIC_OPAQUE_VALUE(addDbColor, DRW_DbColor, "DBCOLOR")
    SEMANTIC_OPAQUE_VALUE(addVisualStyle, DRW_VisualStyle, "VISUALSTYLE")
    SEMANTIC_OPAQUE_VALUE(addScale, DRW_Scale, "SCALE")
    SEMANTIC_OPAQUE_VALUE(addDimensionAssociation, DRW_DimensionAssociation, "DIMASSOC")
    SEMANTIC_OPAQUE_VALUE(addEvaluationGraph, DRW_EvaluationGraph, "EVALUATION_GRAPH")
    SEMANTIC_OPAQUE_VALUE(addBlockRepresentationData, DRW_BlockRepresentationData, "BLOCKREPRESENTATIONDATA")
    SEMANTIC_OPAQUE_VALUE(addDetailViewStyle, DRW_DetailViewStyle, "DETAILVIEWSTYLE")
    SEMANTIC_OPAQUE_VALUE(addSectionViewStyle, DRW_SectionViewStyle, "SECTIONVIEWSTYLE")
    SEMANTIC_OPAQUE_VALUE(addBreakData, DRW_BreakData, "BREAKDATA")
    SEMANTIC_OPAQUE_VALUE(addBreakPointRef, DRW_BreakPointRef, "BREAKPOINTREF")
    SEMANTIC_OPAQUE_VALUE(addGroup, DRW_Group, "GROUP")
    SEMANTIC_OPAQUE_VALUE(addImageDefinitionReactor, DRW_ImageDefinitionReactor, "IMAGEDEF_REACTOR")
    SEMANTIC_OPAQUE_VALUE(addSpatialFilter, DRW_SpatialFilter, "SPATIAL_FILTER")
    SEMANTIC_OPAQUE_VALUE(addGeoData, DRW_GeoData, "GEODATA")
    SEMANTIC_OPAQUE_VALUE(addTableGeometry, DRW_TableGeometry, "TABLEGEOMETRY")
    SEMANTIC_OPAQUE_VALUE(addAcDbPlaceholder, DRW_AcDbPlaceholder, "ACDBPLACEHOLDER")
    SEMANTIC_OPAQUE_VALUE(addVbaProject, DRW_VbaProject, "VBA_PROJECT")
    SEMANTIC_OPAQUE_VALUE(addProxyObject, DRW_ProxyObject, "PROXY_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addSun, DRW_Sun, "SUN")
    SEMANTIC_OPAQUE_VALUE(addLightList, DRW_LightList, "LIGHTLIST")
    SEMANTIC_OPAQUE_VALUE(addDataLink, DRW_DataLink, "DATALINK")
    SEMANTIC_OPAQUE_VALUE(addGeoMapImage, DRW_GeoMapImage, "GEOMAPIMAGE")
    SEMANTIC_OPAQUE_VALUE(addBackground, DRW_Background, "BACKGROUND")
    SEMANTIC_OPAQUE_VALUE(addPointCloudDef, DRW_PointCloudDef, "POINTCLOUDDEF")
    SEMANTIC_OPAQUE_VALUE(addNavisworksModelDef, DRW_NavisworksModelDef, "NAVISWORKSMODELDEF")
    SEMANTIC_OPAQUE_VALUE(addPointCloudColorMap, DRW_PointCloudColorMap, "POINTCLOUDCOLORMAP")
    SEMANTIC_OPAQUE_VALUE(addSunStudy, DRW_SunStudy, "SUNSTUDY")
    SEMANTIC_OPAQUE_VALUE(addLayerFilter, DRW_LayerFilter, "LAYERFILTER")
    SEMANTIC_OPAQUE_VALUE(addMotionPath, DRW_MotionPath, "MOTIONPATH")
    SEMANTIC_OPAQUE_VALUE(addCurvePath, DRW_CurvePath, "CURVEPATH")
    SEMANTIC_OPAQUE_VALUE(addPointPath, DRW_PointPath, "POINTPATH")
    SEMANTIC_OPAQUE_VALUE(addObjectPtr, DRW_ObjectPtr, "OBJECT_PTR")
    SEMANTIC_OPAQUE_VALUE(addPartialViewingIndex, DRW_PartialViewingIndex, "PARTIAL_VIEWING_INDEX")
    SEMANTIC_OPAQUE_VALUE(addRenderSettings, DRW_RenderSettings, "RENDER_SETTINGS")
    SEMANTIC_OPAQUE_VALUE(addSection, DRW_Section, "SECTION")
    SEMANTIC_OPAQUE_VALUE(addAssociativeObject, DRW_AssociativeObject, "ASSOCIATIVE_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addAcShHistoryObject, DRW_AcShHistoryObject, "ACSH_HISTORY_OBJECT")
    SEMANTIC_OPAQUE_VALUE(addIDBuffer, DRW_IDBuffer, "IDBUFFER")
    SEMANTIC_OPAQUE_VALUE(addIndex, DRW_Index, "INDEX")
    SEMANTIC_OPAQUE_VALUE(addLayerIndex, DRW_LayerIndex, "LAYER_INDEX")
    SEMANTIC_OPAQUE_VALUE(addSpatialIndex, DRW_SpatialIndex, "SPATIAL_INDEX")

#undef SEMANTIC_OPAQUE_POINTER
#undef SEMANTIC_OPAQUE_VALUE

    void addUnsupportedObject(const DRW_UnsupportedObject& d) override {
        const std::string recordClass = !d.m_recordName.empty()
            ? d.m_recordName
            : (!d.m_className.empty() ? d.m_className
                                      : "DWG_TYPE_" + std::to_string(d.m_objectType));
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        fields.push_back(intField("objectType", d.m_objectType));
        appendIdentityHandleFields(fields, d.m_handle, d.m_parentHandle);
        fields.push_back(boolField("isEntity", d.m_isEntity));
        fields.push_back(uintField("rawByteCount", d.m_rawBytes.size()));
        const std::size_t recordIndex = addRecord("addUnsupportedObject", "unsupported", recordClass, fields);
        addOwnerEdges(records[recordIndex], d.m_handle, d.m_parentHandle);
        const std::size_t callbackIndex = records[recordIndex].callbackOrdinal;
        const std::string carrierId = addCarrier("dwgUnsupportedObject", d.m_rawBytes.data(),
                                                 d.m_rawBytes.size(), "preserved",
                                                 records[recordIndex].id);
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueObject", "dwg", records[recordIndex].id,
                       carrierId, std::string(), "preserved");
    }
    void addRawDwgSection(const DRW_RawDwgSection& d) override {
        const std::size_t callbackIndex = addCallback("addRawDwgSection");
        const std::string carrierId = addCarrier("rawDwgSection:" + d.m_name,
                                                 d.m_data.data(), d.m_data.size(),
                                                 "preserved", std::string());
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueSection", "dwg", std::string(), carrierId,
                       std::string(), "preserved");
    }
    void addRawDxfObject(const DRW_RawDxfObject& d) override { addRawDxf("addRawDxfObject", "rawDxfObject", d); }
    void addRawDxfEntity(const DRW_RawDxfObject& d) override { addRawDxf("addRawDxfEntity", "rawDxfEntity", d); }
    void addRawDxfSection(const DRW_RawDxfSection& d) override {
        const std::string bytes = serializeVariants(d.m_groups);
        const std::size_t callbackIndex = addCallback("addRawDxfSection");
        const std::string carrierId = addCarrier("rawDxfSection:" + d.m_name,
                                                 bytes.data(), bytes.size(),
                                                 "preservedCanonical", std::string());
        callbacks[callbackIndex].carrierIds.push_back(carrierId);
        addUnsupported("opaqueSection", "dxf", std::string(), carrierId,
                       std::string(), "preservedCanonical");
    }

    void addComment(const char* comment) override {
        const std::size_t callbackIndex = addCallback("addComment");
        if (comment != nullptr) {
            const std::string value(comment);
            callbacks[callbackIndex].carrierIds.push_back(
                addCarrier("dxfComment", value.data(), value.size(),
                           "observed", std::string()));
        }
    }
    void addPlotSettings(const DRW_PlotSettings* d) override {
        if (d == nullptr) { addNullDiagnostic("addPlotSettings"); return; }
        addTable("addPlotSettings", "PLOTSETTINGS", *d);
    }

    void writeHeader(DRW_Header&) override { addCallback("writeHeader"); }
    void writeDwgClasses() override { addCallback("writeDwgClasses"); }
    void collectDwgAppIds() override { addCallback("collectDwgAppIds"); }
    void writeBlocks() override { addCallback("writeBlocks"); }
    void writeBlockRecords() override { addCallback("writeBlockRecords"); }
    void writeEntities() override {
        addCallback("writeEntities");
        DRW_Line line;
        line.layer = "0";
        line.basePoint = DRW_Coord{1.0, 2.0, 3.0};
        line.secPoint = DRW_Coord{4.0, 5.0, 6.0};
        line.extPoint = DRW_Coord{0.0, 0.0, 1.0};
        line.thickness = 0.25;
        if (dxfWriter_ != nullptr)
            writeRecipeSucceeded_ = dxfWriter_->writeLine(&line) && writeRecipeSucceeded_;
        else if (dwgWriter_ != nullptr)
            writeRecipeSucceeded_ = dwgWriter_->writeLine(&line) && writeRecipeSucceeded_;
        else
            writeRecipeSucceeded_ = false;
    }
    void writeLTypes() override { addCallback("writeLTypes"); }
    void writeLayers() override { addCallback("writeLayers"); }
    void writeTextstyles() override { addCallback("writeTextstyles"); }
    void writeVports() override { addCallback("writeVports"); }
    void writeDimstyles() override { addCallback("writeDimstyles"); }
    void writeObjects() override { addCallback("writeObjects"); }
    bool finalizeDwgWrite() override {
        addCallback("finalizeDwgWrite");
        return true;
    }
    bool finalizeDwgWriteStructure() override {
        addCallback("finalizeDwgWriteStructure");
        return true;
    }
    void writeAppId() override { addCallback("writeAppId"); }
    void writeViews() override { addCallback("writeViews"); }
    void writeUCSs() override { addCallback("writeUCSs"); }

    void addOperationFailure(int code, const std::string& stage,
                             const std::string& path,
                             const std::string& operation) {
        addDiagnostic("error", stage,
                      operation + "-error-" + std::to_string(code), path,
                      stage + ": libdxfrw " + operation + " failed");
    }

    bool addWriteResult(const std::string& path, const std::string& digest,
                        std::uint64_t size, const std::string& format,
                        const std::string& version) {
        std::ifstream input(path.c_str(), std::ios::binary);
        if (!input)
            return false;
        std::vector<std::uint8_t> bytes;
        std::array<char, 65536> buffer{};
        while (input) {
            input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            const std::streamsize count = input.gcount();
            if (count > 0)
                bytes.insert(bytes.end(), buffer.data(), buffer.data() + count);
        }
        if (input.bad())
            return false;
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", "WRITE_RESULT"));
        fields.push_back(stringField("entity", "WRITE_RESULT"));
        fields.push_back(stringField("sha256", digest));
        fields.push_back(uintField("byteSize", size));
        fields.push_back(stringField("format", format));
        fields.push_back(stringField("version", version));
        const std::size_t recordIndex = addRecord("writeResult", "artifact",
                                                  "WRITE_RESULT", fields);
        const std::string carrierId = addCarrier("generatedOutput", bytes.data(),
            bytes.size(), "observed", records[recordIndex].id);
        callbacks[records[recordIndex].callbackOrdinal].carrierIds.push_back(carrierId);
        return true;
    }

    void addIntegrityDiagnostic(const DwgIntegrityDiagnostic& d) {
        std::ostringstream message;
        message.imbue(std::locale::classic());
        message << "schema=" << d.schemaVersion << ";phase=" << static_cast<int>(d.phase)
                << ";kind=" << static_cast<int>(d.kind)
                << ";section=" << d.sectionName
                << ";logicalSection=" << d.logicalSectionId
                << ";descriptor=" << d.sectionDescriptorId;
        if (d.hasPageId) message << ";page=" << d.pageId;
        if (d.hasFileOffset) message << ";offset=" << d.fileOffset;
        if (d.hasLogicalHandle) message << ";handle=" << d.logicalHandle;
        if (d.hasExpected) message << ";expected=" << d.expected;
        if (d.hasObserved) message << ";observed=" << d.observed;
        addDiagnostic(d.severity == DwgIntegritySeverity::Error ? "error" : "warning",
                      "integrity",
                      "dwg-integrity-" + std::to_string(static_cast<int>(d.kind)),
                      "/diagnostics/integrity", message.str());
    }

    std::size_t fieldCount() const {
        std::size_t total = 0;
        for (const RecordRow& row : records)
            total += row.fields.size();
        return total;
    }

private:
    void addOpaqueTyped(const std::string& callback,
                        const std::string& recordClass) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        const std::size_t recordIndex = addRecord(callback, "unsupported",
                                                  recordClass, fields);
        addUnsupported("semanticFields", callback, records[recordIndex].id,
                       std::string(), std::string(), "excluded");
    }

    std::size_t addCallback(const std::string& kind,
                            const std::string& recordId = std::string(),
                            const std::string& explicitBlock = std::string()) {
        const std::size_t ordinal = callbacks.size();
        callbacks.push_back(CallbackRow{indexedId("callback", ordinal), ordinal,
            kind, recordId, explicitBlock.empty() ? currentBlockId_ : explicitBlock, {}});
        return ordinal;
    }

    std::size_t addRecord(const std::string& callbackKind, const std::string& kind,
                          const std::string& recordClass, std::vector<Field> fields) {
        const std::string recordId = indexedId("record", records.size());
        const std::size_t callbackIndex = addCallback(callbackKind, recordId);
        records.push_back(RecordRow{recordId, callbackIndex, kind, recordClass,
                                    recordClass, std::string(), std::string(),
                                    currentBlockId_, std::move(fields)});
        nodes.push_back(NodeRow{"node:" + recordId, "record", recordId, std::string()});
        if (!currentBlockId_.empty())
            addEdge("blockMembership", "node:" + recordId, nodeId(currentBlockId_));
        return records.size() - 1;
    }

    template <typename T>
    void addTable(const std::string& callback, const std::string& recordClass,
                  const T& d) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", recordClass));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("name", d.name));
        fields.push_back(intField("flags", d.flags));
        const std::size_t recordIndex = addRecord(callback, "table", recordClass, fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        addExcluded(records[recordIndex].id, "tablePayload");
    }

    std::vector<Field> entityFields(const DRW_Entity& d,
                                    const std::string& recordClass,
                                    const std::string& entity) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", recordClass));
        fields.push_back(stringField("entity", entity));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        fields.push_back(stringField("layer", d.layer));
        fields.push_back(stringField("lineType", d.lineType));
        fields.push_back(intField("color", d.color));
        fields.push_back(intField("color24", d.color24));
        fields.push_back(boolField("visible", d.visible));
        fields.push_back(intField("space", static_cast<int>(d.space)));
        return fields;
    }

    void addEntityRecord(const std::string& callback, const std::string& recordClass,
                         std::vector<Field> fields, const DRW_Entity& d,
                         bool hasExcludedFields) {
        (void)hasExcludedFields;
        const std::size_t recordIndex = addRecord(callback, "entity", recordClass,
                                                  std::move(fields));
        RecordRow& record = records[recordIndex];
        addOwnerEdges(record, d.handle, d.parentHandle);
        const std::size_t callbackIndex = record.callbackOrdinal;
        if (!d.proxyGraphics.empty()) {
            const std::string carrierId = addCarrier("entityProxyGraphics",
                d.proxyGraphics.data(), d.proxyGraphics.size(), "preserved", record.id);
            callbacks[callbackIndex].carrierIds.push_back(carrierId);
        }
        if (!d.dataStorageData.empty()) {
            const std::string carrierId = addCarrier("entityDataStorage",
                d.dataStorageData.data(), d.dataStorageData.size(), "preserved", record.id);
            callbacks[callbackIndex].carrierIds.push_back(carrierId);
        }
        addExcluded(record.id, "unmodeledCommonEntityOrSubtypeFields");
    }

    template <typename T>
    void addBasicEntity(const std::string& callback, const std::string& recordClass,
                        const T& d) {
        addEntityRecord(callback, recordClass,
                        entityFields(d, recordClass, recordClass), d, true);
    }

    template <typename T>
    void addPointerEntity(const std::string& callback, const std::string& recordClass,
                          const T* d) {
        if (d == nullptr) { addNullDiagnostic(callback); return; }
        addBasicEntity(callback, recordClass, *d);
    }

    template <typename T>
    void addLineLike(const std::string& callback, const std::string& recordClass,
                     const T& d) {
        std::vector<Field> fields = entityFields(d, recordClass, recordClass);
        fields.push_back(pointField("start", d.basePoint));
        fields.push_back(pointField("direction", d.secPoint));
        addEntityRecord(callback, recordClass, fields, d, false);
    }

    void addOwnerEdges(RecordRow& record, std::uint32_t handle,
                       std::uint32_t ownerHandle) {
        if (handle != DRW::NoHandle) {
            record.sourceHandle = hexHandle(handle);
            for (NodeRow& node : nodes) {
                if (node.recordId == record.id) {
                    node.handle = hexHandle(handle);
                    break;
                }
            }
            addEdge("handleReference", "node:" + record.id,
                    ensureHandleNode(handle));
        }
        if (ownerHandle != DRW::NoHandle) {
            record.ownerHandle = hexHandle(ownerHandle);
            const std::string owner = ensureHandleNode(ownerHandle);
            addEdge("owner", "node:" + record.id, owner);
        }
    }

    std::string ensureHandleNode(std::uint32_t handle) {
        const std::string id = "node:handle:" + hexHandle(handle);
        if (nodeIds_.insert(std::make_pair(id, true)).second)
            nodes.push_back(NodeRow{id, "handle", std::string(), hexHandle(handle)});
        return id;
    }

    std::string nodeId(const std::string& recordOrNode) const {
        return recordOrNode.compare(0, 5, "node:") == 0
            ? recordOrNode : "node:" + recordOrNode;
    }

    void addEdge(const std::string& kind, const std::string& from,
                 const std::string& to) {
        edges.push_back(EdgeRow{indexedId("edge", edges.size()), kind, from, to,
                                "observed"});
    }

    std::string addCarrier(const std::string& source, const void* bytes,
                           std::size_t size, const std::string& disposition,
                           const std::string& recordId) {
        const std::string id = indexedId("carrier", carriers.size());
        static const std::uint8_t empty = 0;
        std::string sourceHandle;
        for (const RecordRow& record : records) {
            if (record.id == recordId) {
                sourceHandle = record.sourceHandle;
                break;
            }
        }
        const std::string sourcePath = recordId.empty()
            ? "/opaqueCarriers/" + pointerSegment(source)
            : "/records/" + pointerSegment(recordId) + "/" + pointerSegment(source);
        carriers.push_back(CarrierRow{id, sourcePath, size,
            sha256(size == 0 ? static_cast<const void*>(&empty) : bytes, size),
            disposition, recordId, sourceHandle});
        return id;
    }

    void addUnsupported(const std::string& kind, const std::string& source,
                        const std::string& recordId, const std::string& carrierId,
                        const std::string& diagnosticId,
                        const std::string& disposition) {
        const std::string reason = disposition == "excluded"
            ? "adapterFieldCoverageExcluded" : "opaqueBytesPreserved";
        const std::string sourcePath = !recordId.empty()
            ? "/records/" + pointerSegment(recordId) + "/" + pointerSegment(source)
            : (!carrierId.empty()
                ? "/opaqueCarriers/" + pointerSegment(carrierId) + "/" +
                    pointerSegment(source)
                : "/unsupportedContent/" + pointerSegment(source));
        unsupported.push_back(UnsupportedRow{indexedId("unsupported", unsupported.size()),
            kind, sourcePath, recordId, carrierId, diagnosticId, disposition, reason});
    }

    void addExcluded(const std::string& recordId, const std::string& source) {
        addUnsupported("semanticFields", source, recordId, std::string(),
                       std::string(), "excluded");
    }

    void addDiagnostic(const std::string& severity, const std::string& stage,
                       const std::string& code,
                       const std::string& path, const std::string& message) {
        const std::size_t index = diagnostics.size();
        diagnostics.push_back(DiagnosticRow{indexedId("diagnostic", index), index,
            severity, stage, code, path, sha256(message)});
    }

    void addNullDiagnostic(const std::string& callback) {
        addCallback(callback);
        addDiagnostic("error", "callback", "null-callback-payload-1", "/callbacks",
                      callback + " received a null payload");
    }

    static std::string variantValue(const DRW_Variant& value) {
        std::ostringstream out;
        out.imbue(std::locale::classic());
        out << value.code() << ':' << static_cast<int>(value.type()) << ':';
        switch (value.type()) {
        case DRW_Variant::STRING: out << value.c_str(); break;
        case DRW_Variant::INTEGER: out << value.i_val(); break;
        case DRW_Variant::INTEGER64: out << value.i64_val(); break;
        case DRW_Variant::DOUBLE: out << number(value.d_val()); break;
        case DRW_Variant::COORD:
            if (value.coord() != nullptr)
                out << number(value.coord()->x) << ',' << number(value.coord()->y)
                    << ',' << number(value.coord()->z);
            break;
        case DRW_Variant::BINARY:
            if (value.binary() != nullptr) {
                out << value.binary()->size() << ':';
                for (std::uint8_t byte : *value.binary())
                    out << std::hex << std::setw(2) << std::setfill('0')
                        << static_cast<unsigned>(byte);
            }
            break;
        case DRW_Variant::INVALID: out << "invalid"; break;
        }
        return out.str();
    }

    static std::string pointerSegment(const std::string& value) {
        std::string result;
        for (char ch : value) {
            if (ch == '~') result += "~0";
            else if (ch == '/') result += "~1";
            else result.push_back(ch);
        }
        return result;
    }

    static std::string serializeVariants(const std::vector<DRW_Variant>& values) {
        std::string result;
        for (const DRW_Variant& value : values) {
            const std::string encoded = variantValue(value);
            appendLengthFramed(result, encoded);
        }
        return result;
    }

    void addRawDxf(const std::string& callback, const std::string& source,
                   const DRW_RawDxfObject& d) {
        std::vector<Field> fields;
        fields.push_back(stringField("recordClass", d.name));
        fields.push_back(stringField("entity", d.name));
        appendIdentityHandleFields(fields, d.handle, d.parentHandle);
        const std::size_t recordIndex = addRecord(callback, "unsupported", d.name, fields);
        addOwnerEdges(records[recordIndex], d.handle, d.parentHandle);
        const std::string bytes = serializeVariants(d.groups);
        const std::string carrierId = addCarrier(source, bytes.data(), bytes.size(),
                                                 "preservedCanonical",
                                                 records[recordIndex].id);
        callbacks[records[recordIndex].callbackOrdinal].carrierIds.push_back(carrierId);
        addUnsupported("opaqueRecord", "dxf", records[recordIndex].id,
                       carrierId, std::string(), "preservedCanonical");
    }

    std::string currentBlockId_;
    std::map<std::string, bool> nodeIds_;
    dxfRW* dxfWriter_ = nullptr;
    dwgRW* dwgWriter_ = nullptr;
    bool writeRecipeSucceeded_ = true;
};

struct Options {
    std::string input;
    std::string inputPathHint;
    std::string output;
    std::string recipe;
    std::string facade;
    std::string direction = "read";
    std::string inputId;
    std::string originKind = "localFile";
    std::string repository;
    std::string inputCommit;
    std::string sourcePath;
    std::string sourceBlob;
    std::string registryDigest;
    std::string generatorDigest;
    std::string adapterName = LIBDXFRW_SEMANTIC_ADAPTER_NAME;
#ifdef LIBDXFRW_SEMANTIC_SIDE_TARGET
    std::string adapterSide = "target";
#else
    std::string adapterSide = "standalone";
#endif
    std::string package = LIBDXFRW_SEMANTIC_ADAPTER_PACKAGE;
    std::string adapterCommit = LIBDXFRW_SEMANTIC_ADAPTER_COMMIT;
    std::string sourceDigest = LIBDXFRW_SEMANTIC_ADAPTER_SOURCE_DIGEST;
    std::string configDigest = LIBDXFRW_SEMANTIC_ADAPTER_CONFIG_DIGEST;
    std::string staticLibraryDigest = LIBDXFRW_SEMANTIC_ADAPTER_STATIC_LIBRARY_DIGEST;
    std::string binaryDigest = LIBDXFRW_SEMANTIC_ADAPTER_BINARY_DIGEST;
    std::string linkedClosureDigest = LIBDXFRW_SEMANTIC_ADAPTER_LINKED_CLOSURE_DIGEST;
    bool applyExtrusion = false;
    std::string compatibilityProfile = "default";
    bool selfTest = false;
};

std::string nextArgument(int& index, int argc, char** argv, const std::string& name) {
    if (index + 1 >= argc)
        throw std::runtime_error("missing value for " + name);
    return argv[++index];
}

Options parseOptions(int argc, char** argv) {
    Options options;
    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "--input") options.input = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-path-hint") options.inputPathHint = nextArgument(i, argc, argv, arg);
        else if (arg == "--output") options.output = nextArgument(i, argc, argv, arg);
        else if (arg == "--recipe") options.recipe = nextArgument(i, argc, argv, arg);
        else if (arg == "--facade") options.facade = nextArgument(i, argc, argv, arg);
        else if (arg == "--direction") options.direction = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-id") options.inputId = nextArgument(i, argc, argv, arg);
        else if (arg == "--origin-kind") options.originKind = nextArgument(i, argc, argv, arg);
        else if (arg == "--repository") options.repository = nextArgument(i, argc, argv, arg);
        else if (arg == "--input-commit") options.inputCommit = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-path") options.sourcePath = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-blob") options.sourceBlob = nextArgument(i, argc, argv, arg);
        else if (arg == "--registry-digest") options.registryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--generator-digest" || arg == "--generator-sha256") options.generatorDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-name") options.adapterName = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-side") options.adapterSide = nextArgument(i, argc, argv, arg);
        else if (arg == "--package") options.package = nextArgument(i, argc, argv, arg);
        else if (arg == "--adapter-commit") options.adapterCommit = nextArgument(i, argc, argv, arg);
        else if (arg == "--source-digest" || arg == "--source-sha256") options.sourceDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--config-digest" || arg == "--config-sha256") options.configDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--static-library-digest" || arg == "--static-library-sha256") options.staticLibraryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--binary-digest" || arg == "--binary-sha256") options.binaryDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--linked-closure-digest" || arg == "--linked-library-closure-sha256") options.linkedClosureDigest = nextArgument(i, argc, argv, arg);
        else if (arg == "--apply-extrusion") options.applyExtrusion = true;
        else if (arg == "--compatibility-profile") options.compatibilityProfile = nextArgument(i, argc, argv, arg);
        else if (arg == "--json") {}
        else if (arg == "--self-test") options.selfTest = true;
        else throw std::runtime_error("unknown argument: " + arg);
    }
    return options;
}

struct InputInfo {
    std::string digest;
    std::uint64_t size = 0;
    std::string format = "unknown";
    std::string version = "unknown";
};

struct DrawingIdentity {
    std::string format = "unknown";
    std::string version = "unknown";
};

bool validAcVersion(const std::string& value) {
    return value.size() == 6 && value.compare(0, 2, "AC") == 0 &&
        std::all_of(value.begin() + 2, value.end(), [](char ch) {
            return std::isdigit(static_cast<unsigned char>(ch)) != 0;
        });
}

std::string trimAscii(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() &&
           std::isspace(static_cast<unsigned char>(value[begin])) != 0)
        ++begin;
    std::size_t end = value.size();
    while (end > begin &&
           std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
        --end;
    return value.substr(begin, end - begin);
}

bool parseDxfGroupCode(const std::string& text, int& code) {
    const std::string normalized = trimAscii(text);
    if (normalized.empty())
        return false;
    char* end = nullptr;
    errno = 0;
    const long value = std::strtol(normalized.c_str(), &end, 10);
    if (errno != 0 || end == normalized.c_str() || *end != '\0' ||
        value < 0 || value > 1071)
        return false;
    code = static_cast<int>(value);
    return true;
}

std::string asciiDxfVersion(const std::string& prefix) {
    if (prefix.find('\0') != std::string::npos)
        return std::string();
    std::vector<std::string> lines;
    std::size_t begin = 0;
    while (begin < prefix.size()) {
        const std::size_t newline = prefix.find('\n', begin);
        std::string line = prefix.substr(begin, newline == std::string::npos
            ? std::string::npos : newline - begin);
        if (!line.empty() && line.back() == '\r')
            line.pop_back();
        lines.push_back(line);
        if (newline == std::string::npos)
            break;
        begin = newline + 1;
    }
    bool expectSectionName = false;
    bool inHeader = false;
    bool expectVersion = false;
    for (std::size_t index = 0; index + 1 < lines.size(); index += 2) {
        int code = 0;
        if (!parseDxfGroupCode(lines[index], code))
            return std::string();
        const std::string value = trimAscii(lines[index + 1]);
        if (expectSectionName) {
            inHeader = code == 2 && value == "HEADER";
            expectSectionName = false;
        }
        if (expectVersion) {
            return code == 1 && validAcVersion(value) ? value : std::string();
        }
        if (inHeader && code == 9 && value == "$ACADVER") {
            expectVersion = true;
        } else if (code == 0 && value == "SECTION") {
            expectSectionName = true;
            inHeader = false;
        } else if (code == 0 && value == "ENDSEC") {
            inHeader = false;
        }
    }
    return std::string();
}

std::string binaryDxfVersion(const std::string& prefix) {
    std::string sentinel("AutoCAD Binary DXF\r\n");
    sentinel.push_back(static_cast<char>(0x1a));
    sentinel.push_back('\0');
    if (prefix.size() < sentinel.size() || prefix.compare(0, sentinel.size(), sentinel) != 0)
        return std::string();

    std::vector<std::string> markers;
    std::string modern;
    modern.push_back(static_cast<char>(9));
    modern.push_back('\0');
    modern.append("$ACADVER");
    modern.push_back('\0');
    modern.push_back(static_cast<char>(1));
    modern.push_back('\0');
    markers.push_back(modern);
    std::string legacy;
    legacy.push_back(static_cast<char>(9));
    legacy.append("$ACADVER");
    legacy.push_back('\0');
    legacy.push_back(static_cast<char>(1));
    markers.push_back(legacy);
    for (const std::string& marker : markers) {
        const std::size_t found = prefix.find(marker, sentinel.size());
        if (found == std::string::npos)
            continue;
        const std::size_t versionOffset = found + marker.size();
        if (versionOffset + 6 < prefix.size()) {
            const std::string version = prefix.substr(versionOffset, 6);
            if (validAcVersion(version) && prefix[versionOffset + 6] == '\0')
                return version;
        }
    }
    return std::string();
}

DrawingIdentity classifyDrawingPrefix(const std::string& prefix) {
    if (prefix.size() >= 6) {
        const std::string version = prefix.substr(0, 6);
        if (validAcVersion(version))
            return DrawingIdentity{"dwg", version};
    }
    const std::string binaryVersion = binaryDxfVersion(prefix);
    if (!binaryVersion.empty())
        return DrawingIdentity{"dxf-binary", binaryVersion};
    const std::string asciiVersion = asciiDxfVersion(prefix);
    if (!asciiVersion.empty())
        return DrawingIdentity{"dxf-ascii", asciiVersion};
    return DrawingIdentity{};
}

InputInfo inspectInput(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
        throw std::runtime_error("unable to open input");
    Sha256 hash;
    std::array<char, 65536> buffer{};
    std::string prefix;
    InputInfo info;
    while (input) {
        input.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize count = input.gcount();
        if (count <= 0)
            continue;
        if (prefix.size() < 65536) {
            const std::size_t remaining = 65536 - prefix.size();
            prefix.append(buffer.data(), std::min<std::size_t>(remaining,
                static_cast<std::size_t>(count)));
        }
        hash.update(buffer.data(), static_cast<std::size_t>(count));
        info.size += static_cast<std::uint64_t>(count);
    }
    if (input.bad())
        throw std::runtime_error("error reading input");
    info.digest = hash.finish();
    const DrawingIdentity identity = classifyDrawingPrefix(prefix);
    info.format = identity.format;
    info.version = identity.version;
    return info;
}

std::string logicalBasename(const std::string& path) {
    const std::size_t slash = path.find_last_of("/\\");
    const std::string value = slash == std::string::npos ? path : path.substr(slash + 1);
    return value.empty() ? "input" : value;
}

bool validPathHint(const std::string& path) {
    if (path.empty() || path.front() == '/' || path.find('\\') != std::string::npos)
        return false;
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const std::size_t end = path.find('/', begin);
        const std::string part = path.substr(begin, end == std::string::npos
            ? std::string::npos : end - begin);
        if (part.empty() || part == "." || part == "..")
            return false;
        if (end == std::string::npos)
            break;
        begin = end + 1;
    }
    return true;
}

std::string minimalLineRecipe() {
    return "{\"end\":[4,5,6],\"entity\":\"LINE\",\"extrusion\":[0,0,1],"
           "\"id\":\"minimal-line-v1\",\"start\":[1,2,3],\"thickness\":0.25,"
           "\"version\":\"AC1024\"}\n";
}

void verifyMinimalLineRecipe(const std::string& path) {
    std::ifstream input(path.c_str(), std::ios::binary);
    if (!input)
        throw std::runtime_error("unable to open local recipe: " + path);
    std::ostringstream bytes;
    bytes << input.rdbuf();
    if (input.bad())
        throw std::runtime_error("error reading local recipe: " + path);
    if (bytes.str() != minimalLineRecipe())
        throw std::runtime_error("local recipe does not match minimal-line-v1");
}

InputInfo recipeInputInfo(const std::string& facade) {
    const std::string recipe = minimalLineRecipe();
    InputInfo info;
    info.digest = sha256(recipe);
    info.size = recipe.size();
    info.format = facade == "dwgRW" ? "generated-dwg-recipe" : "generated-dxf-recipe";
    info.version = "AC1024";
    return info;
}

std::string failureStage(int error) {
    switch (error) {
    case static_cast<int>(DRW::BAD_NONE): return "none";
    case static_cast<int>(DRW::BAD_UNKNOWN): return "unknown";
    case static_cast<int>(DRW::BAD_OPEN): return "open";
    case static_cast<int>(DRW::BAD_VERSION): return "version";
    case static_cast<int>(DRW::BAD_READ_METADATA): return "metadata";
    case static_cast<int>(DRW::BAD_READ_FILE_HEADER): return "fileHeader";
    case static_cast<int>(DRW::BAD_READ_HEADER): return "header";
    case static_cast<int>(DRW::BAD_READ_HANDLES): return "handles";
    case static_cast<int>(DRW::BAD_READ_CLASSES): return "classes";
    case static_cast<int>(DRW::BAD_READ_TABLES): return "tables";
    case static_cast<int>(DRW::BAD_READ_BLOCKS): return "blocks";
    case static_cast<int>(DRW::BAD_READ_ENTITIES): return "entities";
    case static_cast<int>(DRW::BAD_READ_OBJECTS): return "objects";
    case static_cast<int>(DRW::BAD_READ_SECTION): return "section";
    case static_cast<int>(DRW::BAD_CODE_PARSED): return "parseCode";
    default: return "unknown";
    }
}

template <typename T, typename Writer>
void writeArray(std::ostream& out, const std::vector<T>& values, Writer writer) {
    out << '[';
    for (std::size_t i = 0; i < values.size(); ++i) {
        if (i != 0) out << ',';
        writer(out, values[i]);
    }
    out << ']';
}

void writeFields(std::ostream& out, const std::vector<Field>& fields) {
    writeArray(out, fields, [](std::ostream& stream, const Field& field) {
        stream << "{\"name\":" << jsonString(field.name)
               << ",\"type\":" << jsonString(field.type)
               << ",\"value\":" << field.value << '}';
    });
}

void emitResult(std::ostream& out, const Options& options, const InputInfo& input,
                const SemanticSink& sink, bool succeeded, int error,
                const std::string& stage) {
    const std::string optionsCanonical =
        std::string("{\"applyExtrusion\":") +
        (options.applyExtrusion ? "true" : "false") +
        ",\"compatibilityProfile\":" + jsonString(options.compatibilityProfile) + "}";
    std::string firstDiagnostic;
    std::string firstStage = stage;
    std::string firstPath = "/input";
    int firstCode = error;
    if (!succeeded) {
        for (const DiagnosticRow& diagnostic : sink.diagnostics) {
            if (diagnostic.severity == "error") {
                firstDiagnostic = diagnostic.id;
                firstStage = diagnostic.stage;
                firstPath = diagnostic.path;
                const std::string::size_type separator = diagnostic.code.rfind('-');
                if (separator != std::string::npos && separator + 1 < diagnostic.code.size()) {
                    const char* number = diagnostic.code.c_str() + separator + 1;
                    char* end = nullptr;
                    errno = 0;
                    const long parsed = std::strtol(number, &end, 10);
                    if (errno == 0 && end != number && *end == '\0' &&
                        parsed >= std::numeric_limits<int>::min() &&
                        parsed <= std::numeric_limits<int>::max())
                        firstCode = static_cast<int>(parsed);
                }
                break;
            }
        }
    }
    out.imbue(std::locale::classic());
    out << "{\"schema\":2,\"kind\":\"libdxfrw-semantic-adapter-result\"";
    out << ",\"input\":{\"id\":" << jsonString(options.inputId)
        << ",\"pathHint\":" << jsonString(options.inputPathHint)
        << ",\"originKind\":" << jsonString(options.originKind)
        << ",\"repository\":" << jsonNullable(options.repository)
        << ",\"commit\":" << jsonNullable(options.inputCommit)
        << ",\"sourcePath\":" << jsonNullable(options.sourcePath)
        << ",\"sourceBlob\":" << jsonNullable(options.sourceBlob)
        << ",\"registryDigest\":" << jsonNullable(options.registryDigest)
        << ",\"generatorDigest\":" << jsonNullable(options.generatorDigest)
        << ",\"sha256\":" << jsonString(input.digest)
        << ",\"byteSize\":" << input.size
        << ",\"detectedFormat\":" << jsonString(input.format)
        << ",\"detectedVersion\":" << jsonString(input.version) << '}';
    out << ",\"adapter\":{\"name\":" << jsonString(options.adapterName)
        << ",\"side\":" << jsonString(options.adapterSide)
        << ",\"package\":" << jsonString(options.package)
        << ",\"commit\":" << jsonString(options.adapterCommit)
        << ",\"sourceDigest\":" << jsonString(options.sourceDigest)
        << ",\"configDigest\":" << jsonString(options.configDigest)
        << ",\"staticLibraryDigest\":" << jsonString(options.staticLibraryDigest)
        << ",\"binaryDigest\":" << jsonString(options.binaryDigest)
        << ",\"linkedClosureDigest\":" << jsonString(options.linkedClosureDigest) << '}';
    out << ",\"invocation\":{\"facade\":" << jsonString(options.facade)
        << ",\"direction\":" << jsonString(options.direction)
        << ",\"options\":{\"applyExtrusion\":" << (options.applyExtrusion ? "true" : "false")
        << ",\"compatibilityProfile\":" << jsonString(options.compatibilityProfile) << '}'
        << ",\"optionsDigest\":" << jsonString(sha256(optionsCanonical)) << '}';
    out << ",\"status\":{\"outcome\":" << jsonString(succeeded ? "success" : "failure")
        << ",\"operationSucceeded\":" << (succeeded ? "true" : "false")
        << ",\"exitCode\":" << (succeeded ? 0 : 1)
        << ",\"firstFailure\":{\"stage\":" << (succeeded ? "null" : jsonString(firstStage))
        << ",\"code\":" << (succeeded ? "null" : std::to_string(firstCode))
        << ",\"path\":" << (succeeded ? "null" : jsonString(firstPath))
        << ",\"diagnosticId\":" << jsonNullable(firstDiagnostic) << "}}";

    out << ",\"callbacks\":";
    writeArray(out, sink.callbacks, [](std::ostream& stream, const CallbackRow& row) {
        stream << "{\"id\":" << jsonString(row.id) << ",\"ordinal\":" << row.ordinal
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"blockContextId\":" << jsonNullable(row.blockContextId)
               << ",\"carrierIds\":";
        writeArray(stream, row.carrierIds, [](std::ostream& s, const std::string& id) { s << jsonString(id); });
        stream << '}';
    });
    out << ",\"records\":";
    writeArray(out, sink.records, [](std::ostream& stream, const RecordRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"callbackOrdinal\":" << row.callbackOrdinal
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordClass\":" << jsonString(row.recordClass)
               << ",\"entity\":" << jsonString(row.entity)
               << ",\"sourceHandle\":" << jsonNullable(row.sourceHandle)
               << ",\"ownerHandle\":" << jsonNullable(row.ownerHandle)
               << ",\"blockId\":" << jsonNullable(row.blockId)
               << ",\"fields\":";
        writeFields(stream, row.fields);
        stream << '}';
    });
    out << ",\"graphNodes\":";
    writeArray(out, sink.nodes, [](std::ostream& stream, const NodeRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"handle\":" << jsonNullable(row.handle) << '}';
    });
    out << ",\"graphEdges\":";
    writeArray(out, sink.edges, [](std::ostream& stream, const EdgeRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"from\":" << jsonString(row.from)
               << ",\"to\":" << jsonString(row.to)
               << ",\"disposition\":" << jsonString(row.disposition) << '}';
    });
    out << ",\"opaqueCarriers\":";
    writeArray(out, sink.carriers, [](std::ostream& stream, const CarrierRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"source\":" << jsonString(row.source)
               << ",\"byteSize\":" << row.size
               << ",\"sha256\":" << jsonString(row.digest)
               << ",\"disposition\":" << jsonString(row.disposition)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"sourceHandle\":" << jsonNullable(row.sourceHandle) << '}';
    });
    out << ",\"unsupportedContent\":";
    writeArray(out, sink.unsupported, [](std::ostream& stream, const UnsupportedRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"kind\":" << jsonString(row.kind)
               << ",\"source\":" << jsonString(row.source)
               << ",\"recordId\":" << jsonNullable(row.recordId)
               << ",\"carrierId\":" << jsonNullable(row.carrierId)
               << ",\"diagnosticId\":" << jsonNullable(row.diagnosticId)
               << ",\"disposition\":" << jsonString(row.disposition)
               << ",\"reason\":" << jsonString(row.reason) << '}';
    });
    out << ",\"diagnostics\":";
    writeArray(out, sink.diagnostics, [](std::ostream& stream, const DiagnosticRow& row) {
        stream << "{\"id\":" << jsonString(row.id)
               << ",\"ordinal\":" << row.index
               << ",\"severity\":" << jsonString(row.severity)
               << ",\"stage\":" << jsonString(row.stage)
               << ",\"code\":" << jsonString(row.code)
               << ",\"path\":" << jsonString(row.path)
               << ",\"messageDigest\":" << jsonString(row.messageDigest) << '}';
    });
    out << ",\"cardinality\":{\"callbackCount\":" << sink.callbacks.size()
        << ",\"recordCount\":" << sink.records.size()
        << ",\"fieldCount\":" << sink.fieldCount()
        << ",\"graphNodeCount\":" << sink.nodes.size()
        << ",\"graphEdgeCount\":" << sink.edges.size()
        << ",\"opaqueCarrierCount\":" << sink.carriers.size()
        << ",\"unsupportedContentCount\":" << sink.unsupported.size()
        << ",\"diagnosticCount\":" << sink.diagnostics.size() << "}}\n";
}

int selfTest() {
    if (sha256("abc") != "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")
        return 1;
    if (jsonString("a\n\"b") != "\"a\\n\\\"b\"")
        return 1;
    std::string oneRecord;
    appendLengthFramed(oneRecord, "1:0:a\n2:0:b");
    std::string twoRecords;
    appendLengthFramed(twoRecords, "1:0:a");
    appendLengthFramed(twoRecords, "2:0:b");
    if (oneRecord == twoRecords)
        return 1;
    try {
        (void)number(std::numeric_limits<double>::infinity());
        return 1;
    } catch (const std::domain_error&) {
    }
    const DrawingIdentity dwgIdentity = classifyDrawingPrefix("AC1024payload");
    if (dwgIdentity.format != "dwg" || dwgIdentity.version != "AC1024")
        return 1;
    const DrawingIdentity asciiIdentity = classifyDrawingPrefix(
        "  0\nSECTION\n  2\nHEADER\n  9\n$ACADVER\n  1\nAC1021\n  0\nENDSEC\n");
    if (asciiIdentity.format != "dxf-ascii" || asciiIdentity.version != "AC1021")
        return 1;
    std::string binary("AutoCAD Binary DXF\r\n");
    binary.push_back(static_cast<char>(0x1a));
    binary.push_back('\0');
    binary.push_back(static_cast<char>(9));
    binary.push_back('\0');
    binary.append("$ACADVER");
    binary.push_back('\0');
    binary.push_back(static_cast<char>(1));
    binary.push_back('\0');
    binary.append("AC1027");
    binary.push_back('\0');
    const DrawingIdentity binaryIdentity = classifyDrawingPrefix(binary);
    if (binaryIdentity.format != "dxf-binary" || binaryIdentity.version != "AC1027")
        return 1;
    if (classifyDrawingPrefix("{\"version\":\"AC1024\"}\n").format != "unknown")
        return 1;
    if (classifyDrawingPrefix("AutoCAD Binary DXFAC1024").format != "unknown")
        return 1;
    if (failureStage(static_cast<int>(DRW::BAD_READ_SECTION)) != "section" ||
        failureStage(static_cast<int>(DRW::BAD_CODE_PARSED)) != "parseCode")
        return 1;
    SemanticSink failureSink;
    failureSink.addOperationFailure(2, "write", "/output", "write");
    if (failureSink.diagnostics.size() != 1 ||
        failureSink.diagnostics[0].code != "write-error-2" ||
        failureSink.diagnostics[0].path != "/output")
        return 1;
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        Options options = parseOptions(argc, argv);
        if (options.selfTest)
            return selfTest();
        if (options.input.empty() && options.recipe.empty())
            throw std::runtime_error("--input is required");
        if (options.facade != "dxfRW" && options.facade != "dwgRW")
            throw std::runtime_error("--facade must be dxfRW or dwgRW");
        if (options.direction != "read" && options.direction != "write")
            throw std::runtime_error("--direction must be read or write");
        if (options.compatibilityProfile != "default")
            throw std::runtime_error(
                "only the cross-package default compatibility profile is supported");

        const bool writeMode = options.direction == "write";
        if (writeMode) {
            if (options.recipe.empty())
                options.recipe = options.input == "minimal-line-v1"
                    ? "minimal-line-v1" : "minimal-line-v1";
            if (options.recipe != "minimal-line-v1")
                throw std::runtime_error("only the minimal-line-v1 recipe is supported");
            if (options.output.empty())
                throw std::runtime_error("--output is required for write direction");
            if (options.originKind == "localFile")
                options.originKind = "localFromScratch";
            if (options.inputId.empty())
                options.inputId = "minimal-line-v1";
            if (options.inputPathHint.empty())
                options.inputPathHint = "local/minimal-line-v1";
            if (options.generatorDigest.empty())
                options.generatorDigest = sha256(minimalLineRecipe());
        } else {
            if (options.inputId.empty())
                options.inputId = logicalBasename(options.input);
            if (options.inputPathHint.empty())
                options.inputPathHint = logicalBasename(options.input);
        }
        if (!validPathHint(options.inputPathHint))
            throw std::runtime_error("--input-path-hint must be a normalized relative POSIX path");

        InputInfo input;
        if (writeMode) {
            if (options.input != "minimal-line-v1" && !options.input.empty()) {
                verifyMinimalLineRecipe(options.input);
                input = inspectInput(options.input);
                input.format = "recipe-json";
                input.version = "AC1024";
            } else {
                input = recipeInputInfo(options.facade);
                input.format = "recipe-json";
            }
        } else {
            input = inspectInput(options.input);
        }
        SemanticSink sink;
        bool succeeded = false;
        int error = 1;
        std::string stage = "invocation";
        if (writeMode) {
            bool wrote = false;
            if (options.facade == "dxfRW") {
                dxfRW writer(options.output.c_str());
                sink.configureWrite(&writer);
                wrote = writer.write(&sink, DRW::AC1024, false);
                error = static_cast<int>(writer.getError());
            } else {
                dwgRW writer(options.output.c_str());
                sink.configureWrite(&writer);
                wrote = writer.write(&sink, DRW::AC1024, false);
                error = static_cast<int>(writer.getError());
            }
            wrote = wrote && sink.writeRecipeSucceeded();
            if (!wrote) {
                stage = "write";
                sink.addOperationFailure(error, stage, "/output", "write");
            } else {
                const InputInfo generated = inspectInput(options.output);
                if (!sink.addWriteResult(options.output, generated.digest,
                                         generated.size, generated.format,
                                         generated.version)) {
                    error = 1;
                    stage = "writeResult";
                    sink.addOperationFailure(error, stage, "/output", "write-result");
                } else if (options.facade == "dxfRW") {
                    dxfRW reader(options.output.c_str());
                    succeeded = reader.read(&sink, options.applyExtrusion);
                    error = static_cast<int>(reader.getError());
                    stage = succeeded ? "none" : "readback";
                    if (!succeeded)
                        sink.addOperationFailure(error, stage, "/output", "readback");
                } else {
                    dwgRW reader(options.output.c_str());
                    succeeded = reader.read(&sink, options.applyExtrusion);
                    error = static_cast<int>(reader.getError());
                    stage = succeeded ? "none" : "readback";
                    if (!succeeded)
                        sink.addOperationFailure(error, stage, "/output", "readback");
                    for (const DwgIntegrityDiagnostic& diagnostic :
                         reader.getIntegrityDiagnostics())
                        sink.addIntegrityDiagnostic(diagnostic);
                }
            }
        } else if (options.facade == "dxfRW") {
            dxfRW reader(options.input.c_str());
            succeeded = reader.read(&sink, options.applyExtrusion);
            error = static_cast<int>(reader.getError());
            stage = failureStage(error);
            if (!succeeded)
                sink.addOperationFailure(error, stage, "/input", "read");
        } else {
            dwgRW reader(options.input.c_str());
            succeeded = reader.read(&sink, options.applyExtrusion);
            error = static_cast<int>(reader.getError());
            stage = failureStage(error);
            if (!succeeded)
                sink.addOperationFailure(error, stage, "/input", "read");
            const std::vector<DwgIntegrityDiagnostic> diagnostics =
                reader.getIntegrityDiagnostics();
            for (const DwgIntegrityDiagnostic& diagnostic : diagnostics)
                sink.addIntegrityDiagnostic(diagnostic);
        }
        emitResult(std::cout, options, input, sink, succeeded, error, stage);
        return succeeded ? 0 : 1;
    } catch (const std::exception& error) {
        std::cerr << "semantic adapter: " << error.what() << '\n';
        return 2;
    }
}
