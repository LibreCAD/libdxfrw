#ifndef DX_STANDARD_OUTPUT_H
#define DX_STANDARD_OUTPUT_H

// Internal, synchronous inspection of already staged DXF bytes. No source
// grammar is invented here: the second pass uses the production parseCode
// bodies. The first pass records physical presence/scope so tolerant defaults
// and ignored unknown groups cannot become preservation evidence.
#include <algorithm>
#include <charconv>
#include <cstring>
#include <istream>
#include <iterator>
#include <limits>
#include <memory>
#include <streambuf>
#include <string_view>
#include <utility>
#include <vector>
#include "libdxfrw.h"
#include "intern/dxfreader.h"

namespace dx_standard_output {

class InputBuffer final : public std::streambuf {
public:
    explicit InputBuffer(std::string_view bytes = {}) { reset(bytes); }
    void reset(std::string_view bytes) {
        char* begin = const_cast<char*>(bytes.data());
        setg(begin, begin, begin == nullptr ? nullptr : begin + bytes.size());
    }
    std::size_t position() const noexcept {
        return eback() == nullptr ? 0u : static_cast<std::size_t>(gptr() - eback());
    }
protected:
    pos_type seekoff(off_type offset, std::ios_base::seekdir direction,
                     std::ios_base::openmode mode) override {
        if ((mode & std::ios_base::out) != 0 || eback() == nullptr)
            return pos_type(off_type(-1));
        const auto length = static_cast<off_type>(egptr() - eback());
        const auto origin = direction == std::ios_base::beg ? off_type(0)
            : direction == std::ios_base::cur ? static_cast<off_type>(position())
            : direction == std::ios_base::end ? length : off_type(-1);
        if (origin < 0 || offset < -origin || offset > length - origin)
            return pos_type(off_type(-1));
        const auto target = origin + offset;
        setg(eback(), eback() + target, egptr());
        return pos_type(target);
    }
    pos_type seekpos(pos_type position, std::ios_base::openmode mode) override {
        return seekoff(static_cast<off_type>(position), std::ios_base::beg, mode);
    }
};

inline std::unique_ptr<dxfReader> readerFor(
        std::istream& input, const dxfRW::DxfObjectsWriteView& view,
        std::shared_ptr<DxfReadRetainedBudget> budget) {
    const char* version = nullptr;
    for (const auto& row : DRW::dwgVersionStrings)
        if (row.second == view.version) version = row.first;
    if (version == nullptr || view.version < DRW::AC1014
        || view.codePage.size() > 128u
        || (view.compatibilityProfile != dxfRW::DxfCompatibilityProfile::StandaloneSafe
            && view.compatibilityProfile != dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy))
        return nullptr;
    std::unique_ptr<dxfReader> reader;
    if (view.binary) reader = std::make_unique<dxfReaderBinary>(&input);
    else reader = std::make_unique<dxfReaderAscii>(&input);
    reader->setVersion(version, true);
    reader->setCodePage(std::string(view.codePage));
    reader->setClassifierProfile(view.compatibilityProfile
        == dxfRW::DxfCompatibilityProfile::StandaloneSafe
        ? DxfClassifierProfile::StandaloneSafe : DxfClassifierProfile::LibreCadMasterLegacy);
    reader->setIgnoreComments(false);
    reader->setRecordBudget(4u * 1024u * 1024u);
    reader->setTokenByteBudget(DRW::kMaxDxfBinaryPayloadBytes);
    reader->setInputByteBudget(std::min(view.bytes.size(),
        4u * DRW::kMaxDxfBinaryPayloadBytes));
    reader->setDecodeWorkBudget(4u * DRW::kMaxDxfBinaryPayloadBytes);
    reader->setTypedVariantNodeBudget(DRW::kMaxDxfBinaryPayloadBytes / sizeof(DRW_Variant));
    reader->setRetainedByteBudget(std::move(budget));
    return reader;
}

enum class Family { Other, Dictionary, Group, Xrecord };
enum class RawHandleTransport { Translate, ObjectScopes };
// Exact ordered primitive transport for a separately admitted raw profile.
// The caller owns field-role admission; no unknown widths or record defaults
// are inferred. Only the current token is decoded, never a record inventory.
template<class Mapper> bool rawScalarTransportEqual(
        const std::vector<DRW_Variant>& source, std::string_view body,
        const dxfRW::DxfObjectsWriteView& view, Mapper&& map,
        bool materialColors = false,
        std::size_t retainedByteBudget = DRW::kMaxDxfBinaryPayloadBytes,
        bool literalSourceHandles = false,
        RawHandleTransport handleTransport = RawHandleTransport::Translate) {
    InputBuffer buffer(body); std::istream input(&buffer);
    auto reader = readerFor(input, view,
        std::make_shared<DxfReadRetainedBudget>(retainedByteBudget));
    if (!reader) return false;
    reader->setMaterialUnsignedColorValues(materialColors);
    if(literalSourceHandles)reader->setAllowWideHandleLexemes(true);
    std::size_t applicationDepth = 0u;
    for (const auto& value : source) {
        int code = 0;
        if (!reader->readRec(&code) || code != value.code()) return false;
        if (handleTransport == RawHandleTransport::ObjectScopes && code == 102) {
            if (value.type() != DRW_Variant::STRING || !value.content.s) return false;
            const auto& text = *value.content.s;
            if (!text.empty() && text.front() == '{') {
                if (++applicationDepth > DRW::kMaxDxfApplicationGroupNesting) return false;
            } else if (text == "}" && applicationDepth) --applicationDepth;
            else return false;
        }
        if (isDxfHandleCode(code)) {
            if (value.type() != DRW_Variant::STRING || !value.content.s) return false;
            // Source reconstruction must retain exact lexical values, including
            // opaque/arbitrary wide IDs. This opt-in is NOT output remap proof.
            // Raw-object replay keeps arbitrary IDs and application-local5
            // literal. Their spelling is not a translated identity/reference.
            const bool literalRole = handleTransport == RawHandleTransport::ObjectScopes
                && (!isDxfTranslatedHandleCode(code) || (code == 5 && applicationDepth));
            if(literalSourceHandles || literalRole) {
                if(reader->getStringView()!=*value.content.s)return false;
                continue;
            }
            std::uint32_t key = 0u, expected = 0u;
            const auto& token = *value.content.s;
            const auto parsed = std::from_chars(token.data(), token.data() + token.size(), key, 16);
            if (token.empty() || parsed.ec != std::errc{} || parsed.ptr != token.data() + token.size()
                || !map(key, expected) || reader->getHandleString() != expected) return false;
        } else if (value.type() == DRW_Variant::STRING) {
            if (!value.content.s || reader->getStringView() != *value.content.s) return false;
        } else if (value.type() == DRW_Variant::INTEGER) {
            if (reader->getInt32() != value.i_val()) return false;
        } else if (value.type() == DRW_Variant::INTEGER64) {
            if (materialColors && (code == 90 || code == 91 || code == 92)) {
                if (value.i64_val() < 0 || value.i64_val() > 4294967295LL
                    || reader->getUInt32() != static_cast<std::uint32_t>(value.i64_val())) return false;
            } else if (code >= 160 && code <= 169) {
                if (reader->getInt64() != value.i64_val()) return false;
            } else return false;
        } else if (value.type() == DRW_Variant::DOUBLE) {
            const double actual = reader->getDouble(), expected = value.d_val();
            if (std::memcmp(&actual, &expected, sizeof(double)) != 0) return false;
        } else return false;
    }
    return buffer.position() == body.size() && applicationDepth == 0u;
}

inline Family family(std::string_view name) noexcept {
    return name == "DICTIONARY" ? Family::Dictionary
        : name == "GROUP" ? Family::Group
        : name == "XRECORD" ? Family::Xrecord : Family::Other;
}

// Raw framing uses the active DXF classifier only for binary field widths.
// It deliberately does not coerce unrelated numeric values into signed model
// fields. No decoded strings/payload owners or object models are allocated.
class RecordCursor {
public:
    struct Token { int code{-1}; std::string_view text; };
    explicit RecordCursor(std::string_view bytes, const dxfRW::DxfObjectsWriteView& view)
        : bytes(bytes), binary(view.binary), profile(view.compatibilityProfile == dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy
            ? DxfClassifierProfile::LibreCadMasterLegacy : DxfClassifierProfile::StandaloneSafe) {}
    std::size_t position() const noexcept { return at; }
    bool done() const noexcept { return at == bytes.size(); }
    bool next(Token& token) {
        token = {};
        if (++tokens > 4u * 1024u * 1024u) return false;
        if (!binary) {
            std::string_view code;
            if (!line(code)) return false;
            while (!code.empty() && (code.front() == ' ' || code.front() == '\t')) code.remove_prefix(1);
            while (!code.empty() && (code.back() == ' ' || code.back() == '\t')) code.remove_suffix(1);
            if (!code.empty() && code.front() == '+') code.remove_prefix(1);
            const auto parsed = std::from_chars(code.data(), code.data() + code.size(), token.code);
            return !code.empty() && parsed.ec == std::errc{} && parsed.ptr == code.data() + code.size()
                && token.code >= 0 && token.code <= 1071 && line(token.text);
        }
        if (bytes.size() - at < 2u) return false;
        token.code = static_cast<unsigned char>(bytes[at]) | (static_cast<unsigned char>(bytes[at + 1u]) << 8u);
        at += 2u;
        const auto kind = dxfValueKindForCode(token.code, profile);
        if (kind == DxfValueKind::Str) {
            const auto end = bytes.find('\0', at);
            if (end == std::string_view::npos || end - at > DRW::kMaxDxfBinaryPayloadBytes) return false;
            token.text = bytes.substr(at, end - at); at = end + 1u; return true;
        }
        std::size_t width = 0u;
        switch (kind) {
        case DxfValueKind::Dbl: case DxfValueKind::I64: width = 8u; break;
        case DxfValueKind::I32: width = 4u; break;
        case DxfValueKind::I16: width = 2u; break;
        case DxfValueKind::Bln: width = 1u; break;
        case DxfValueKind::Bin:
            if (at == bytes.size()) return false;
            width = static_cast<unsigned char>(bytes[at++]); break;
        default: return false; // Unknown binary widths cannot be framed safely.
        }
        if (width > bytes.size() - at) return false;
        token.text = bytes.substr(at, width); at += width; return true;
    }
private:
    bool line(std::string_view& result) {
        const auto end = bytes.find('\n', at);
        if (end == std::string_view::npos || end - at > DRW::kMaxDxfBinaryPayloadBytes) return false;
        result = bytes.substr(at, end - at); at = end + 1u;
        if (!result.empty() && result.back() == '\r') result.remove_suffix(1);
        return true;
    }
    std::string_view bytes;
    bool binary;
    DxfClassifierProfile profile;
    std::size_t at{0u}, tokens{0u};
};

struct Shape {
    Family kind{Family::Other};
    std::uint32_t handle{0};
    unsigned self{0}, owner{0}, subclass{0}, hardOwner{0}, cloning{0};
    unsigned description{0}, unnamed{0}, selectable{0}, depth{0};
    int pointNext{0};
    bool body{false}, xdata{false}, entryPending{false}, valid{true};
    bool xrecordHeaderPending{false};
    DRW::Version version{DRW::UNKNOWNV};

    void observe(int code, dxfReader& reader) {
        if (kind == Family::Other) return;
        // Normal post-subclass groups (including later 102/280/330) belong
        // to XRECORD application data. A trailing 1001 starts common XDATA.
        if (kind == Family::Xrecord && body) {
            if (xrecordHeaderPending) {
                xrecordHeaderPending = false;
                if (version > DRW::AC1014) {
                    if (code != 280) valid = false;
                    else { ++cloning; return; }
                }
            }
            if (!xdata && code < 1000) {
                if (!isDxfXRecordApplicationCode(code)) valid = false;
                return;
            }
            if (!xdata && code != 1001) { valid = false; return; }
        }
        if (code == 102) {
            const auto marker = reader.getStringView();
            if (body || xdata) valid = false;
            if (marker.size() > 1u && marker.front() == '{') {
                if (++depth > static_cast<unsigned>(DRW::kMaxDxfApplicationGroupNesting)) valid = false;
            } else if (marker == "}" && depth != 0u) --depth;
            else valid = false;
            return;
        }
        if (depth != 0u) return;
        if (pointNext != 0) {
            if (code != pointNext) valid = false;
            pointNext = code >= 1020 && code <= 1023 ? code + 10 : 0;
        } else if (code >= 1010 && code <= 1013) pointNext = code + 10;
        else if ((code >= 1020 && code <= 1023) || (code >= 1030 && code <= 1033)) valid = false;
        if (code >= 1000) {
            if (!xdata && code != 1001) valid = false;
            xdata = true;
            const bool supported = (code >= 1000 && code <= 1005)
                || (code >= 1010 && code <= 1013) || (code >= 1020 && code <= 1023)
                || (code >= 1030 && code <= 1033) || (code >= 1040 && code <= 1042)
                || code == 1070 || code == 1071;
            if (!body || !supported) valid = false;
            return;
        }
        if (xdata) valid = false;
        if (code == 5 && !body) {
            if (++self != 1u || !reader.isValidHandleString()) valid = false;
            else handle = reader.getHandleString();
            return;
        }
        if (code == 330 && !body) { if (++owner != 1u) valid = false; return; }
        if (code == 100) {
            const auto expected = kind == Family::Dictionary ? "AcDbDictionary"
                : kind == Family::Group ? "AcDbGroup" : "AcDbXrecord";
            if (++subclass != 1u || reader.getStringView() != expected) valid = false;
            body = true;
            xrecordHeaderPending = kind == Family::Xrecord;
            return;
        }
        if (!body) { valid = false; return; }
        if (kind == Family::Dictionary) {
            if (entryPending && code != 350 && code != 360) valid = false;
            if (code == 280) { if (++hardOwner != 1u) valid = false; }
            else if (code == 281) { if (++cloning != 1u) valid = false; }
            else if (code == 3) { if (entryPending) valid = false; entryPending = true; }
            else if (code == 350 || code == 360) {
                if (!entryPending) valid = false;
                entryPending = false;
            } else valid = false;
        } else if (kind == Family::Group) {
            if (code == 300) { if (++description != 1u) valid = false; }
            else if (code == 70) {
                if (++unnamed != 1u || (reader.getInt32() != 0 && reader.getInt32() != 1)) valid = false;
            } else if (code == 71) {
                if (++selectable != 1u || (reader.getInt32() != 0 && reader.getInt32() != 1)) valid = false;
            } else if (code != 340) valid = false;
        }
    }
    bool complete() const noexcept {
        if (!valid || self != 1u || owner != 1u || subclass != 1u
            || depth != 0u || pointNext != 0 || entryPending || handle == 0u)
            return false;
        if (kind == Family::Dictionary) return hardOwner == 1u && cloning == 1u;
        if (kind == Family::Group) return description == 1u && unnamed == 1u && selectable == 1u;
        return kind == Family::Xrecord && (version <= DRW::AC1014 || cloning == 1u);
    }
};

template <class Record> class Probe final : public Record {
public:
    using Record::parseCode;
};

template <> class Probe<DRW_MLineStyle> final : public DRW_MLineStyle {
public:
    Probe() { resetDxf(); }
    using DRW_MLineStyle::parseCode;
};

// The input/reader/scratch owners are bounded and lexical. Application-group
// parsers may consume several tokens; hence the separate physical shape pass.
class Decoder final {
public:
    explicit Decoder(const dxfRW::DxfObjectsWriteView& profile, bool materialColors=false,
                     DRW::DxfTextEncoding encoding=DRW::DxfTextEncoding::Declared,
                     DRW::DxfCodePagePolicy policy=DRW::DxfCodePagePolicy::Compatible)
        : input(&buffer), budget(std::make_shared<DxfReadRetainedBudget>(DRW::kMaxDxfBinaryPayloadBytes)),
          reader(readerFor(input, profile, budget)) {
        if(reader) {
            reader->setMaterialUnsignedColorValues(materialColors);
            reader->setDxfTextEncoding(encoding);
            reader->setDxfCodePagePolicy(policy);
        }
    }
    bool good() const noexcept { return reader != nullptr; }
    template <class Record, class Visitor> bool decodeRecord(
            std::string_view bytes, Visitor&& visit) {
        if (!reader) return false;
        buffer.reset(bytes);
        input.clear();
        Probe<Record> record;
        int code = 0;
        while (buffer.position() < bytes.size()) {
            if (!reader->readRec(&code) || code == 0 || !record.parseCode(code, reader)) return false;
        }
        if (budget->exceeded() || reader->typedVariantNodeBudgetExceeded()) return false;
        return visit(static_cast<const Record&>(record));
    }
private:
    InputBuffer buffer;
    std::istream input;
    std::shared_ptr<DxfReadRetainedBudget> budget;
    std::unique_ptr<dxfReader> reader;
};

// Frame borrowed records without decoding unrelated object profiles through
// the generic modeled reader (e.g. MATERIAL's unsigned color fields). Identify
// only the common header for selection: a later XRECORD application102/330 is
// not common metadata. Selected shapes still use the original strict observer.
template <class Selector, class Visitor> bool inspectSelected(
        const dxfRW::DxfObjectsWriteView& view, Selector&& select, Visitor&& visit) {
    if (view.bytes.size() > 4u * DRW::kMaxDxfBinaryPayloadBytes
        || view.bytes.size() > static_cast<std::size_t>((std::numeric_limits<std::streamoff>::max)())) return false;
    InputBuffer buffer;
    std::istream input(&buffer);
    auto budget = std::make_shared<DxfReadRetainedBudget>(DRW::kMaxDxfBinaryPayloadBytes);
    auto reader = readerFor(input, view, budget);
    if (!reader) return false;
    std::string_view name;
    std::size_t bodyStart = 0u, records = 0u;
    const auto deliver = [&](std::string_view body) {
        RecordCursor header(body, view);
        RecordCursor::Token token;
        std::uint32_t handle = 0u;
        unsigned depth = 0u, selves = 0u;
        while (!header.done()) {
            if (!header.next(token)) return false;
            if (depth == 0u && token.code == 100) break;
            if (token.code == 102) {
                if (!token.text.empty() && token.text.front() == '{') ++depth;
                else if (token.text == "}" && depth != 0u) --depth;
                else break; // A selected malformed header cannot earn a shape.
            } else if (depth == 0u && token.code == 5 && ++selves == 1u) {
                std::uint32_t candidate = 0u;
                const auto parsed = std::from_chars(token.text.data(), token.text.data() + token.text.size(), candidate, 16);
                if (!token.text.empty() && parsed.ec == std::errc{} && parsed.ptr == token.text.data() + token.text.size()) handle = candidate;
            }
        }
        if (!select(name, handle)) return true;
        Shape shape;
        shape.kind = family(name);
        shape.version = view.version;
        if (shape.kind == Family::Other) { shape.handle = handle; return visit(shape, body); }
        buffer.reset(body);
        input.clear();
        while (buffer.position() < body.size()) {
            int code = 0;
            if (!reader->readRec(&code) || code == 0) return false;
            shape.observe(code, *reader);
        }
        return !budget->exceeded() && visit(shape, body);
    };
    RecordCursor cursor(view.bytes, view);
    RecordCursor::Token token;
    while (!cursor.done()) {
        const auto start = cursor.position();
        if (!cursor.next(token) || token.text.size() > DRW::kMaxDxfBinaryPayloadBytes) return false;
        if (token.code == 0) {
            if (token.text.empty() || token.text == "SECTION" || token.text == "ENDSEC" || token.text == "EOF") return false;
            if (!name.empty() && !deliver(view.bytes.substr(bodyStart, start - bodyStart))) return false;
            if (++records > 1000000u) return false;
            name = token.text;
            bodyStart = cursor.position();
        } else if (name.empty()) return false;
    }
    return !name.empty() && deliver(view.bytes.substr(bodyStart));
}

// Default inspection visits all shapes. No section-sized copy, record/model
// inventory or borrowed view is retained after its synchronous callback.
template <class Visitor> bool inspect(
        const dxfRW::DxfObjectsWriteView& view, Visitor&& visit) {
    return inspectSelected(view, [](std::string_view, std::uint32_t) { return true; }, std::forward<Visitor>(visit));
}

inline bool handleValue(const DRW_Variant& value, std::uint64_t& result) noexcept {
    if (value.type() == DRW_Variant::INTEGER) {
        result = static_cast<std::uint32_t>(value.i_val()); return true;
    }
    if (value.type() == DRW_Variant::INTEGER64) {
        result = static_cast<std::uint64_t>(value.i64_val()); return true;
    }
    if (value.type() != DRW_Variant::STRING || value.content.s == nullptr) return false;
    const auto& text = *value.content.s;
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, 16);
    return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}

template <class Mapper> bool referenceEqual(std::uint32_t source, std::uint32_t actual,
        Mapper& map, std::vector<std::uint32_t>& references) {
    std::uint32_t expected = 0;
    if (!map(source, expected) || expected != actual) return false;
    if (expected != 0u) {
        if (references.size() >= 1000000u) return false;
        references.push_back(expected);
    }
    return true;
}

template <class Mapper> bool valueEqual(const DRW_Variant& source, const DRW_Variant& actual,
        Mapper& map, std::vector<std::uint32_t>& references) {
    if (source.code() != actual.code() || source.dwgCmColorValue() != nullptr
        || source.isLayerRef() || source.rawDwgLayerReference() != 0u) return false;
    // DWG common EED stores binary bytes; the shared DXF XDATA parser keeps
    // the physical hexadecimal token. Compare one exact chunk without a
    // decoded copy, concatenation or a change to item boundaries/order.
    if (source.code() == 1004 && source.type() == DRW_Variant::BINARY
        && actual.type() == DRW_Variant::STRING) {
        if (!source.binary() || !actual.content.s) return false;
        const auto& bytes = *source.binary();
        const auto& hex = *actual.content.s;
        if ((hex.size() & 1u) != 0u || hex.size() / 2u != bytes.size()) return false;
        const auto nibble = [](unsigned char digit) -> int {
            if (digit >= '0' && digit <= '9') return digit - '0';
            if (digit >= 'A' && digit <= 'F') return digit - 'A' + 10;
            if (digit >= 'a' && digit <= 'f') return digit - 'a' + 10;
            return -1;
        };
        for (std::size_t index = 0; index < bytes.size(); ++index) {
            const int high = nibble(static_cast<unsigned char>(hex[2u * index]));
            const int low = nibble(static_cast<unsigned char>(hex[2u * index + 1u]));
            if (high < 0 || low < 0 || static_cast<unsigned>(high * 16 + low) != bytes[index]) return false;
        }
        return true;
    }
    if (isDxfHandleCode(source.code())) {
        std::uint64_t a = 0, b = 0;
        if (!handleValue(source, a) || !handleValue(actual, b)) return false;
        if (dxfHandleRoleForCode(source.code()) == DxfHandleRole::Arbitrary) return a == b;
        if (a > (std::numeric_limits<std::uint32_t>::max)()
            || b > (std::numeric_limits<std::uint32_t>::max)()) return false;
        return referenceEqual(static_cast<std::uint32_t>(a), static_cast<std::uint32_t>(b), map, references);
    }
    // The writer accepts a wide source1071 only within the signed32-bit wire
    // range. The shared reader returns INTEGER; do not narrow before comparing
    // or generalize this representation bridge to other codes/types.
    if (source.code() == 1071 && source.type() == DRW_Variant::INTEGER64) {
        const auto value = source.i64_val();
        return value >= (std::numeric_limits<std::int32_t>::min)()
            && value <= (std::numeric_limits<std::int32_t>::max)()
            && actual.type() == DRW_Variant::INTEGER && actual.dwgCmColorValue() == nullptr
            && !actual.isLayerRef() && actual.rawDwgLayerReference() == 0u
            && value == actual.i_val();
    }
    if (source.type() != actual.type()) return false;
    switch (source.type()) {
    case DRW_Variant::STRING:
        return source.content.s && actual.content.s && *source.content.s == *actual.content.s;
    case DRW_Variant::INTEGER: return source.i_val() == actual.i_val();
    case DRW_Variant::INTEGER64: return source.i64_val() == actual.i64_val();
    case DRW_Variant::DOUBLE: {
        const double a = source.d_val(), b = actual.d_val();
        return std::memcmp(&a, &b, sizeof(a)) == 0;
    }
    case DRW_Variant::COORD:
        return source.coord() && actual.coord()
            && std::memcmp(&source.coord()->x, &actual.coord()->x, sizeof(double)) == 0
            && std::memcmp(&source.coord()->y, &actual.coord()->y, sizeof(double)) == 0
            && std::memcmp(&source.coord()->z, &actual.coord()->z, sizeof(double)) == 0;
    case DRW_Variant::BINARY: return source.binary() && actual.binary() && *source.binary() == *actual.binary();
    case DRW_Variant::INVALID: return false;
    }
    return false;
}

inline std::string_view appName(const std::list<DRW_Variant>& group) noexcept {
    if (group.empty() || group.front().code() != 102
        || group.front().type() != DRW_Variant::STRING || !group.front().content.s) return {};
    std::string_view name(*group.front().content.s);
    return !name.empty() && name.front() == '{' ? name.substr(1) : name;
}

inline bool reservedGroupExact(const std::list<DRW_Variant>& group,
        const DRW_TableEntry& actual, bool reactors) noexcept {
    const auto count = reactors ? actual.reactorHandles.size() : (actual.xDictHandle != 0u ? 1u : 0u);
    if (count == 0u || group.size() != count + 2u) return false;
    auto value = std::next(group.begin());
    for (std::size_t index = 0; index < count; ++index, ++value) {
        std::uint64_t handle = 0;
        if (value->code() != (reactors ? 330 : 360) || !handleValue(*value, handle)
            || handle != (reactors ? actual.reactorHandles[index] : actual.xDictHandle)) return false;
    }
    return value->code() == 102 && value->type() == DRW_Variant::STRING
        && value->content.s && *value->content.s == "}";
}

struct NoLayerReferences {
    bool operator()(const DRW_Variant&, const DRW_Variant&,
                    std::vector<std::uint32_t>&) const noexcept { return false; }
};

// Compare each source value with its prescribed physical transport, rather
// than joining adjacent values or requiring source/decoded cardinality parity.
// This is shared by generated roots and admitted standard-object frames.
template<class Mapper, class LayerVerifier> bool extendedDataTransportEqual(
        const DRW_TableEntry& source, const DRW_TableEntry& actual, Mapper& map,
        std::vector<std::uint32_t>& references, LayerVerifier& verifyLayer) {
    if (source.extData.size() > DRW_TableEntry::kMaxExtendedDataItems
        || actual.extData.size() > DRW_TableEntry::kMaxExtendedDataItems) return false;
    const auto nibble = [](unsigned char c) -> int {
        return c >= '0' && c <= '9' ? c - '0' : c >= 'A' && c <= 'F' ? c - 'A' + 10 : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
    };
    std::size_t next = 0u;
    for (const auto* value : source.extData) {
        if (!value || next >= actual.extData.size()) return false;
        if (value->code() != 1004) {
            const auto* output = actual.extData[next++];
            if (!output) return false;
            if (value->code() == 1003 && (value->isLayerRef() || value->rawDwgLayerReference())) {
                if (!verifyLayer(*value,*output,references)) return false;
            } else if (!valueEqual(*value,*output,map,references)) return false;
            continue;
        }
        if (value->dwgCmColorValue() || value->isLayerRef() || value->rawDwgLayerReference()) return false;
        const auto* bytes = value->type() == DRW_Variant::BINARY ? value->binary() : nullptr;
        const auto* hex = value->type() == DRW_Variant::STRING ? value->content.s : nullptr;
        if ((!bytes && !hex) || (hex && (hex->size() & 1u))) return false;
        const auto size = bytes ? bytes->size() : hex->size() / 2u;
        if (size > DRW::kMaxDxfBinaryPayloadBytes) return false;
        const auto chunks = size ? (size + 126u) / 127u : 1u;
        if (chunks > actual.extData.size() - next) return false;
        for (std::size_t offset = 0u; offset < size || (offset == 0u && !size); offset += 127u) {
            const auto count = std::min<std::size_t>(127u,size - offset);
            const auto* output = actual.extData[next++];
            if (!output || output->code() != 1004 || output->type() != DRW_Variant::STRING || !output->content.s
                || output->content.s->size() != 2u * count) return false;
            for (std::size_t n = 0u; n < count; ++n) {
                const int high = nibble((*output->content.s)[2u * n]), low = nibble((*output->content.s)[2u * n + 1u]);
                if (high < 0 || low < 0) return false;
                unsigned expected = 0u;
                if (bytes) expected = (*bytes)[offset + n];
                else {
                    const int a = nibble((*hex)[2u * (offset + n)]), b = nibble((*hex)[2u * (offset + n) + 1u]);
                    if (a < 0 || b < 0) return false;
                    expected = static_cast<unsigned>(16 * a + b);
                }
                if (static_cast<unsigned>(16 * high + low) != expected) return false;
            }
        }
    }
    return next == actual.extData.size();
}

template <class Mapper, class LayerVerifier> bool commonEqual(const DRW_TableEntry& source,
        const DRW_TableEntry& actual, Family kind, Mapper& map,
        std::vector<std::uint32_t>& references, LayerVerifier& verifyLayer) {
    std::uint32_t self = 0;
    if (!map(source.handle, self) || self != actual.handle
        || source.flags != 0 || actual.flags != 0 || source.hasDataStorageBinaryData()
        || (kind != Family::Group && !source.name.empty())
        || !referenceEqual(source.parentHandle, actual.parentHandle, map, references)
        || source.reactorHandles.size() != actual.reactorHandles.size()
        || !referenceEqual(source.xDictHandle, actual.xDictHandle, map, references)) return false;
    // The caller must supply a frame whose bounded common-link receipt was
    // correlated to this model. The model's legacy marker is not a wire field.
    // A proven null H (even with a DWG present-null flag) maps to no DXF link;
    // an Unknown tail must never reach this admitted comparison.
    for (std::size_t index = 0; index < source.reactorHandles.size(); ++index)
        if (!referenceEqual(source.reactorHandles[index], actual.reactorHandles[index], map, references)) return false;
    if (!extendedDataTransportEqual(source,actual,map,references,verifyLayer)) return false;
    auto expected = source.appData.begin();
    unsigned reactorGroups = 0, dictionaryGroups = 0;
    for (const auto& group : actual.appData) {
        const auto name = appName(group);
        if (name == "ACAD_REACTORS") {
            if (++reactorGroups != 1u || !reservedGroupExact(group, actual, true)) return false;
            continue;
        }
        if (name == "ACAD_XDICTIONARY") {
            if (++dictionaryGroups != 1u || !reservedGroupExact(group, actual, false)) return false;
            continue;
        }
        if (expected == source.appData.end() || name != appName(*expected)
            || expected->size() != group.size() || group.empty()) return false;
        auto a = std::next(expected->begin()), b = std::next(group.begin());
        for (; a != expected->end(); ++a, ++b)
            if (!valueEqual(*a, *b, map, references)) return false;
        ++expected;
    }
    // Reserved source appData and unresolved layer/CMC carriers remain outside
    // the admitted projection rather than silently discounting their metadata.
    return expected == source.appData.end()
        && reactorGroups == (source.reactorHandles.empty() ? 0u : 1u)
        && dictionaryGroups == (source.xDictHandle == 0u ? 0u : 1u);
}

template <class Mapper, class LayerVerifier = NoLayerReferences> bool equal(const DRW_Dictionary& source,
        const DRW_Dictionary& actual, Mapper& map, std::vector<std::uint32_t>& references,
        LayerVerifier verifyLayer = {}) {
    if (!commonEqual(source, actual, Family::Dictionary, map, references, verifyLayer)
        || !actual.hasCompleteDxfEntries() || source.cloning != actual.cloning
        || source.hardOwner != actual.hardOwner || source.m_entries.size() != actual.m_entries.size()) return false;
    for (std::size_t index = 0; index < source.m_entries.size(); ++index) {
        const auto& a = source.m_entries[index]; const auto& b = actual.m_entries[index];
        const auto role = a.m_dxfHandleCode == DRW_Dictionary::Entry::DxfHandleCode::Unspecified
            ? (source.hardOwner == 1 ? DRW_Dictionary::Entry::DxfHandleCode::HardOwner
                                    : DRW_Dictionary::Entry::DxfHandleCode::SoftOwner) : a.m_dxfHandleCode;
        if (a.m_name != b.m_name || role != b.m_dxfHandleCode
            || !referenceEqual(a.m_handle, b.m_handle, map, references)) return false;
    }
    return true;
}

template <class Mapper, class LayerVerifier = NoLayerReferences> bool equal(const DRW_Group& source,
        const DRW_Group& actual, Mapper& map, std::vector<std::uint32_t>& references,
        LayerVerifier verifyLayer = {}) {
    if (!commonEqual(source, actual, Family::Group, map, references, verifyLayer)
        || source.m_description != actual.m_description || source.m_isUnnamed != actual.m_isUnnamed
        || source.m_selectable != actual.m_selectable || source.m_entityHandles.size() != actual.m_entityHandles.size()) return false;
    for (std::size_t index = 0; index < source.m_entityHandles.size(); ++index)
        if (!referenceEqual(source.m_entityHandles[index], actual.m_entityHandles[index], map, references)) return false;
    return true;
}

template<class Mapper> bool xrecordApplicationTransportEqual(const std::vector<DRW_Variant>& source,
        const std::vector<DRW_Variant>& actual, Mapper& map, std::vector<std::uint32_t>& references) {
    if (source.size() > 1000000u || actual.size() > 1000000u) return false;
    std::size_t next = 0u;
    for (const auto& value : source) {
        if (!isDxfXRecordApplicationCode(value.code()) || next >= actual.size()) return false;
        if (value.code() < 310 || value.code() > 319) {
            if (value.type() == DRW_Variant::BINARY) return false;
            if (!valueEqual(value,actual[next++],map,references)) return false;
            continue;
        }
        if (value.type() != DRW_Variant::BINARY || !value.binary() || value.dwgCmColorValue()
            || value.isLayerRef() || value.rawDwgLayerReference()) return false;
        const auto& bytes = *value.binary();
        if (bytes.size() > DRW::kMaxDxfBinaryPayloadBytes) return false;
        const auto chunks = bytes.empty() ? 1u : (bytes.size() + 126u) / 127u;
        if (chunks > actual.size() - next) return false;
        // Match each source value's exact writer chunks independently. Never
        // concatenate values, repartition boundaries or allocate joined bytes.
        for (std::size_t n = 0u; n < chunks; ++n) {
            const auto& output = actual[next++];
            const auto offset = 127u * n, count = std::min<std::size_t>(127u,bytes.size() - offset);
            if (output.code() != value.code() || output.type() != DRW_Variant::BINARY || !output.binary()
                || output.dwgCmColorValue() || output.isLayerRef() || output.rawDwgLayerReference()
                || output.binary()->size() != count
                || !std::equal(bytes.begin() + offset,bytes.begin() + offset + count,output.binary()->begin())) return false;
        }
    }
    return next == actual.size();
}

template <class Mapper, class LayerVerifier = NoLayerReferences> bool equal(const DRW_XRecord& source,
        const DRW_XRecord& actual, Mapper& map, std::vector<std::uint32_t>& references,
        LayerVerifier verifyLayer = {}) {
    if (!commonEqual(source, actual, Family::Xrecord, map, references, verifyLayer)
        || source.m_rawDataValid || source.hasDwgRawDataSourceRecord()
        || !source.hasExactDxfCoordinateTuples() || !actual.hasExactDxfCoordinateTuples()
        || source.m_cloning != actual.m_cloning) return false;
    return xrecordApplicationTransportEqual(source.m_dataEntries,actual.m_dataEntries,map,references);
}

} // namespace dx_standard_output
#endif
