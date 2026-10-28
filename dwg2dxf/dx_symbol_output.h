#ifndef DX_SYMBOL_OUTPUT_H
#define DX_SYMBOL_OUTPUT_H

// Internal, per-record observations of the exact staged TABLES body. This is
// name/identity/scope evidence only, never a full symbol-table semantic model.
#include "dx_standard_output.h"

namespace dx_symbol_output {

enum class Kind { Other, Layer, AppId, LineType };
inline Kind kind(std::string_view name) noexcept {
    return name == "LAYER" ? Kind::Layer : name == "APPID" ? Kind::AppId
        : name == "LTYPE" ? Kind::LineType : Kind::Other;
}

struct NameRecord {
    Kind type{Kind::Other};
    std::uint32_t handle{0}, owner{0}, tableHandle{0};
    std::string name;
    bool qualified{false};
};

struct Limits {
    std::size_t records{1000000u};
    std::size_t physicalRecords{4u * 1024u * 1024u};
    std::size_t nameWireBytes{1024u * 1024u};
    std::size_t nameBytes{512u * 1024u};
};

// The visitor runs before another row is decoded. It must copy/charge any
// values it retains; neither a section-sized copy nor a row inventory is made.
// A false result invalidates ALL tentative observations made by this call.
// All known kinds reach the visitor; select an explicit kind rather than
// interpreting every non-Layer row as AppId. LineType is name/scope evidence,
// not a qualification of its dash pattern or recursive dependencies.
template <class Visitor> bool inspect(
        const dxfRW::DxfTablesWriteView& view, Visitor&& visit,
        Limits limits = {}) {
    if (view.bytes.size() > static_cast<std::size_t>((std::numeric_limits<std::streamoff>::max)())) return false;
    dx_standard_output::InputBuffer buffer(view.bytes);
    std::istream input(&buffer);
    auto budget = std::make_shared<DxfReadRetainedBudget>(DRW::kMaxDxfBinaryPayloadBytes);
    auto reader = dx_standard_output::readerFor(input, view, budget);
    if (!reader) return false;
    reader->setRecordBudget(limits.physicalRecords);
    enum class Mode { None, Header, Row, End } mode{Mode::None};
    std::string tableName;
    NameRecord row;
    unsigned names = 0u, handles = 0u, owners = 0u, subclasses = 0u;
    unsigned capacity = 0u, flags = 0u, depth = 0u;
    std::size_t records = 0u;
    std::uint32_t tableHandle = 0u, handle = 0u, owner = 0u;
    bool active = false, valid = true, xdata = false, tableQualified = false;
    bool sawLayer = false, sawAppId = false, sawLineType = false;
    const auto finish = [&]() {
        if (depth != 0u) return false;
        if (mode == Mode::Header) {
            if (names != 1u || tableName.empty()) return false;
            const auto type = kind(tableName);
            if ((type == Kind::Layer && sawLayer) || (type == Kind::AppId && sawAppId)
                || (type == Kind::LineType && sawLineType)) return false;
            sawLayer = sawLayer || type == Kind::Layer;
            sawAppId = sawAppId || type == Kind::AppId;
            sawLineType = sawLineType || type == Kind::LineType;
            tableHandle = handle;
            const bool hasOwner = type == Kind::LineType ? view.version >= DRW::AC1014
                : view.version > DRW::AC1014;
            tableQualified = valid && handles == 1u && handle != 0u
                && subclasses == 1u && capacity == 1u
                && owners == (hasOwner ? 1u : 0u) && owner == 0u;
        } else if (mode == Mode::Row && row.type != Kind::Other) {
            row.handle = handle; row.owner = owner; row.tableHandle = tableHandle;
            const bool hasOwner = row.type == Kind::Layer || row.type == Kind::LineType
                || view.version > DRW::AC1014;
            row.qualified = tableQualified && valid && handles == 1u && handle != 0u
                && names == 1u && !row.name.empty() && subclasses == 2u && flags == 1u
                && owners == (hasOwner ? 1u : 0u)
                && (hasOwner ? owner == tableHandle : owner == 0u);
            if (!visit(static_cast<const NameRecord&>(row))) return false;
        }
        return true;
    };
    while (buffer.position() < view.bytes.size()) {
        int code = 0;
        if (!reader->readRec(&code)) return false;
        if (code == 0) {
            if (!finish() || records >= limits.records) return false;
            ++records;
            const auto name = reader->getStringView();
            if (name == "TABLE") {
                if (active) return false;
                active = true; mode = Mode::Header; tableName.clear(); tableQualified = false;
            } else if (name == "ENDTAB") {
                if (!active) return false;
                active = false; mode = Mode::End;
            } else {
                if (!active || tableName.empty() || name != tableName) return false;
                mode = Mode::Row;
            }
            row = {}; row.type = kind(tableName);
            names = handles = owners = subclasses = capacity = flags = depth = 0u;
            handle = owner = 0u; valid = true; xdata = false;
            continue;
        }
        if (mode != Mode::Header && mode != Mode::Row) return false;
        // Application metadata cannot spoof the primary name/identity/owner.
        if (code == 102) {
            const auto marker = reader->getStringView();
            if (!marker.empty() && marker.front() == '{') {
                if (depth >= 32u) return false;
                ++depth;
            } else if (marker == "}" && depth != 0u) --depth;
            else return false;
            continue;
        }
        if (depth != 0u) continue;
        if (code == 1001) xdata = true;
        if (xdata) {
            if (code < 1000) valid = false;
            continue;
        }
        if (code == 2) {
            if (++names != 1u) { valid = false; continue; }
            if (mode == Mode::Header) {
                const auto name = reader->getStringView();
                if (name.empty() || name.size() > 128u) return false;
                tableName.assign(name);
            } else if (row.type != Kind::Other) {
                if (subclasses != 2u) valid = false;
                if (reader->getStringView().size() > limits.nameWireBytes) return false;
                const auto before = reader->textReadObservations().issueDecodeCalls;
                row.name = reader->getUtf8String();
                const auto after = reader->textReadObservations();
                if (row.name.size() > limits.nameBytes) return false;
                if (after.counterSaturated || after.issueDecodeCalls != before) valid = false;
                const auto selection = reader->declaredCodePageSelection().kind;
                if (view.version < DRW::AC1021 && selection != DRW_CodePageKind::Canonical
                    && selection != DRW_CodePageKind::ExactAlias) valid = false;
            }
        } else if (code == 5) {
            if (++handles != 1u || !reader->isValidHandleString()) valid = false;
            else handle = reader->getHandleString();
        } else if (code == 330) {
            if (++owners != 1u || !reader->isValidHandleString()) valid = false;
            else owner = reader->getHandleString();
        } else if (code == 100) {
            ++subclasses;
            const auto marker = reader->getStringView();
            if (mode == Mode::Header) {
                if (subclasses != 1u || marker != "AcDbSymbolTable") valid = false;
            } else if (row.type != Kind::Other) {
                if ((subclasses == 1u && marker != "AcDbSymbolTableRecord")
                    || (subclasses == 2u && marker != (row.type == Kind::Layer
                        ? "AcDbLayerTableRecord" : row.type == Kind::AppId
                            ? "AcDbRegAppTableRecord" : "AcDbLinetypeTableRecord"))
                    || subclasses > 2u) valid = false;
            }
        } else if (code == 70) {
            if (mode == Mode::Header) {
                if (++capacity != 1u || reader->getInt32() < 0) valid = false;
            } else if (++flags != 1u) valid = false;
        }
    }
    return records != 0u && !active && mode == Mode::End && finish()
        && !budget->exceeded() && !reader->recordBudgetExceeded();
}

} // namespace dx_symbol_output
#endif
