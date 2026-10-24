/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2026 LibreCAD (librecad.org)                                **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

// Port of d:/data/dli/dwg-parser/src/dwg/acis/{sab,entities,geometry}.ts.
// Three layers: SAB byte decode -> record graph -> wireframe extraction.

#include "drw_acis.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <exception>
#include <limits>
#include <locale>
#include <map>
#include <new>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

// ── Layer 1: SAB byte reader (sab.ts) ──────────────────────────────────────

const char* const kSignatures[] = { "ACIS BinaryFile", "ASM BinaryFile4" };

bool isEndMarker(const std::string& t) {
    return t == "End-of-ACIS-data" || t == "End-of-ASM-data"
        || t == "End-of-ACIS-History-data";
}

// The raw-token vector retains the entity type at index 0. In the admitted
// Autodesk-modified 21800 profile, indexes 1 and 2 are the shared attribute
// pointer and entity ID; caller-built graph records without that prefix keep
// the historical offset zero. Derive this internally to preserve the layout
// of the installed DRW_SabRecord type.
std::size_t semanticTokenStart(const DRW_SabHeader& header,
                               const DRW_SabRecord& record) {
    if (header.signature == "ACIS BinaryFile" && header.version == 21800
        && !isEndMarker(record.type) && record.tokens.size() >= 3u
        && record.tokens[0].tag == DRW_SabTag::EntityType
        && record.tokens[0].sval == record.type
        && record.tokens[1].tag == DRW_SabTag::Pointer
        && record.tokens[2].tag == DRW_SabTag::Int)
        return 3u;
    return 0u;
}

// Thrown internally on any malformed/truncated read or exhausted parse cap.
struct SabError : std::exception {
    explicit SabError(bool resourceLimit = false)
        : resourceLimitExceeded(resourceLimit) {}

    bool resourceLimitExceeded;
};

class SabParseBudget {
public:
    explicit SabParseBudget(const DRW_SabParseLimits& limits)
        : m_limits(limits) {}

    void checkInput(std::size_t bytes) const {
        if (bytes > m_limits.maxInputBytes)
            throw SabError(true);
    }

    void consumeRecord() {
        if (recordsProcessed >= m_limits.maxRecords)
            throw SabError(true);
        ++recordsProcessed;
    }

    void consumeTag() {
        if (tagsProcessed >= m_limits.maxTags)
            throw SabError(true);
        ++tagsProcessed;
    }

    void accountString(std::size_t bytes) {
        if (bytes > m_limits.maxStringBytes
            || stringBytesAccounted > m_limits.maxCumulativeStringBytes
            || bytes > m_limits.maxCumulativeStringBytes
                            - stringBytesAccounted)
            throw SabError(true);
        stringBytesAccounted += bytes;
    }

    void accountGraphWork(std::size_t units) {
        if (graphWorkUnitsProcessed > m_limits.maxGraphWorkUnits
            || units > m_limits.maxGraphWorkUnits
                            - graphWorkUnitsProcessed)
            throw SabError(true);
        graphWorkUnitsProcessed += units;
    }

    void accountGraphItems(std::size_t items) {
        if (graphItemsAccounted > m_limits.maxGraphItems
            || items > m_limits.maxGraphItems - graphItemsAccounted)
            throw SabError(true);
        graphItemsAccounted += items;
    }

    void accountExtractionWork(std::size_t units = 1u) {
        if (extractionWorkUnitsProcessed
                > m_limits.maxExtractionWorkUnits
            || units > m_limits.maxExtractionWorkUnits
                            - extractionWorkUnitsProcessed)
            throw SabError(true);
        extractionWorkUnitsProcessed += units;
    }

    void accountOutputItems(std::size_t items = 1u) {
        if (outputItemsAccounted > m_limits.maxOutputItems
            || items > m_limits.maxOutputItems - outputItemsAccounted)
            throw SabError(true);
        outputItemsAccounted += items;
    }

    std::size_t tokenCapacityTarget(std::size_t oldCapacity,
                                    std::size_t needed) const {
        if (tokenCapacityItemsAccounted < oldCapacity)
            throw SabError();
        const std::size_t otherCapacity =
            tokenCapacityItemsAccounted - oldCapacity;
        if (otherCapacity > m_limits.maxTokenCapacityItems)
            throw SabError(true);
        const std::size_t available =
            m_limits.maxTokenCapacityItems - otherCapacity;
        if (needed > available)
            throw SabError(true);

        std::size_t target = needed;
        if (oldCapacity != 0u) {
            target = oldCapacity
                > std::numeric_limits<std::size_t>::max() - oldCapacity
                ? std::numeric_limits<std::size_t>::max()
                : oldCapacity * 2u;
            target = std::max(target, needed);
        }
        return std::min(target, available);
    }

    void accountTokenCapacity(std::size_t oldCapacity,
                             std::size_t newCapacity) {
        if (tokenCapacityItemsAccounted < oldCapacity)
            throw SabError();
        const std::size_t otherCapacity =
            tokenCapacityItemsAccounted - oldCapacity;
        if (otherCapacity > m_limits.maxTokenCapacityItems
            || newCapacity
                   > m_limits.maxTokenCapacityItems - otherCapacity)
            throw SabError(true);
        tokenCapacityItemsAccounted = otherCapacity + newCapacity;
    }

    std::size_t typePartCapacityTarget(std::size_t oldCapacity,
                                       std::size_t needed) const {
        if (oldCapacity > m_limits.maxTypePartCapacityItems
            || needed > m_limits.maxTypePartCapacityItems)
            throw SabError(true);
        std::size_t target = needed;
        if (oldCapacity != 0u) {
            target = oldCapacity
                > std::numeric_limits<std::size_t>::max() - oldCapacity
                ? std::numeric_limits<std::size_t>::max()
                : oldCapacity * 2u;
            target = std::max(target, needed);
        }
        return std::min(target, m_limits.maxTypePartCapacityItems);
    }

    void accountTypePartCapacity(std::size_t capacity) {
        if (capacity > m_limits.maxTypePartCapacityItems)
            throw SabError(true);
        typePartCapacityPeakItems = std::max(
            typePartCapacityPeakItems, capacity);
    }

    void checkSubtypeDepth(std::size_t depth) const {
        if (depth > m_limits.maxSubtypeDepth)
            throw SabError(true);
    }

    const DRW_SabParseLimits& limits() const { return m_limits; }

    std::size_t recordsProcessed = 0;
    std::size_t tagsProcessed = 0;
    std::size_t stringBytesAccounted = 0;
    std::size_t graphWorkUnitsProcessed = 0;
    std::size_t graphItemsAccounted = 0;
    std::size_t extractionWorkUnitsProcessed = 0;
    std::size_t outputItemsAccounted = 0;
    std::size_t tokenCapacityItemsAccounted = 0;
    std::size_t typePartCapacityPeakItems = 0;

private:
    const DRW_SabParseLimits& m_limits;
};

bool admitSetItem(std::set<int>& values, int value,
                  SabParseBudget& budget) {
    if (values.find(value) != values.end())
        return false;
    // Charge before the set node is allocated. If insertion itself fails, the
    // operation reports the admitted work together with allocation failure.
    budget.accountOutputItems();
    values.insert(value);
    return true;
}

enum class TransformCandidateProfile {
    Invalid,
    TwelveValues,
    ThirteenValues
};

TransformCandidateProfile parseTransformComponents(
    const std::string& literal, std::array<double, 12>& components,
    SabParseBudget& budget) {
    std::istringstream stream(literal);
    stream.imbue(std::locale::classic());
    for (double& component : components) {
        budget.accountExtractionWork();
        if (!(stream >> component) || !std::isfinite(component))
            return TransformCandidateProfile::Invalid;
    }
    stream >> std::ws;
    if (stream.eof())
        return TransformCandidateProfile::TwelveValues;

    double alternateComponent = 0.0;
    budget.accountExtractionWork();
    if (!(stream >> alternateComponent) || !std::isfinite(alternateComponent))
        return TransformCandidateProfile::Invalid;
    stream >> std::ws;
    return stream.eof() ? TransformCandidateProfile::ThirteenValues
                        : TransformCandidateProfile::Invalid;
}

std::uint8_t bitAlignedByte(const std::uint8_t* data, std::size_t index,
                            std::uint8_t shift) {
    if (shift == 0u)
        return data[index];
    const std::uint8_t next = data[index + 1u];
    return static_cast<std::uint8_t>((data[index] << shift)
                                     | (next >> (8u - shift)));
}

bool startsWithAscii(const std::uint8_t* data, std::size_t len,
                     const char* prefix, std::uint8_t shift) {
    std::size_t plen = std::strlen(prefix);
    if (len < plen) return false;
    for (std::size_t i = 0; i < plen; ++i) {
        if (bitAlignedByte(data, i, shift)
            != static_cast<std::uint8_t>(prefix[i]))
            return false;
    }
    return true;
}

class SabByteReader {
public:
    SabByteReader(const std::uint8_t* data, std::size_t len,
                  std::uint8_t shift = 0u)
        : m_data(data), m_len(len), m_shift(shift) {}

    std::size_t index = 0;

    bool hasData() const { return index < m_len; }

    std::uint8_t readByte() {
        require(1);
        return bitAlignedByte(m_data, index++, m_shift);
    }
    int readInt() {
        require(4);
        if (m_shift == 0u) {
            std::int32_t value;
            std::memcpy(&value, m_data + index, sizeof(value));
            index += 4u;
            return static_cast<int>(value);
        }
        std::uint8_t bytes[4];
        for (std::size_t i = 0; i < 4u; ++i)
            bytes[i] = readByte();
        std::int32_t v;
        std::memcpy(&v, bytes, sizeof(v));   // little-endian host (MSVC/x64)
        return static_cast<int>(v);
    }
    double readFloat() {
        require(8);
        if (m_shift == 0u) {
            double value;
            std::memcpy(&value, m_data + index, sizeof(value));
            index += 8u;
            return value;
        }
        std::uint8_t bytes[8];
        for (std::size_t i = 0; i < 8u; ++i)
            bytes[i] = readByte();
        double v;
        std::memcpy(&v, bytes, sizeof(v));
        return v;
    }
    std::string readStr(std::size_t length, SabParseBudget& budget) {
        require(length);
        budget.accountString(length);
        if (m_shift == 0u) {
            std::string s(reinterpret_cast<const char*>(m_data + index), length);
            index += length;
            return s;
        }
        std::string s;
        s.reserve(length);
        for (std::size_t i = 0; i < length; ++i)
            s.push_back(static_cast<char>(readByte()));
        return s;
    }
    DRW_Coord readVec() {
        double x = readFloat();
        double y = readFloat();
        double z = readFloat();
        return DRW_Coord(x, y, z);
    }

private:
    const std::uint8_t* m_data;
    std::size_t m_len;
    std::uint8_t m_shift;
    void require(std::size_t n) {
        if (index > m_len || n > m_len - index) throw SabError();
    }
};

std::string readStrTag(SabByteReader& r, SabParseBudget& budget) {
    budget.consumeTag();
    if (r.readByte() != DRW_SabTag::Str) throw SabError();
    return r.readStr(r.readByte(), budget);
}
double readDoubleTag(SabByteReader& r, SabParseBudget& budget) {
    budget.consumeTag();
    if (r.readByte() != DRW_SabTag::Double) throw SabError();
    return r.readFloat();
}

DRW_SabHeader readHeader(SabByteReader& r, const std::uint8_t* data,
                         std::size_t len, std::uint8_t shift,
                         SabParseBudget& budget) {
    DRW_SabHeader h;
    for (const char* sig : kSignatures) {
        if (startsWithAscii(data, len, sig, shift)) {
            budget.accountString(std::strlen(sig));
            h.signature = sig;
            r.index = std::strlen(sig);
            break;
        }
    }
    if (h.signature.empty()) throw SabError();
    h.version = r.readInt();
    h.numRecords = r.readInt();
    h.numEntities = r.readInt();
    h.flags = r.readInt();
    h.productId = readStrTag(r, budget);
    h.acisVersion = readStrTag(r, budget);
    h.creationDate = readStrTag(r, budget);
    h.unitsInMm = readDoubleTag(r, budget);
    h.resTol = readDoubleTag(r, budget);
    h.norTol = readDoubleTag(r, budget);
    return h;
}

DRW_SabToken tok(int tag) { DRW_SabToken t; t.tag = tag; return t; }

void appendSabToken(std::vector<DRW_SabToken>& tokens,
                    DRW_SabToken&& token, SabParseBudget& budget) {
    if (tokens.size() == tokens.capacity()) {
        if (tokens.size() == tokens.max_size())
            throw SabError(true);
        const std::size_t oldCapacity = tokens.capacity();
        const std::size_t target = budget.tokenCapacityTarget(
            oldCapacity, tokens.size() + 1u);
        tokens.reserve(target);
        budget.accountTokenCapacity(oldCapacity, tokens.capacity());
    }
    tokens.push_back(std::move(token));
}

void appendTypePart(std::vector<std::string>& parts, std::string&& part,
                    SabParseBudget& budget) {
    if (parts.size() == parts.capacity()) {
        if (parts.size() == parts.max_size())
            throw SabError(true);
        const std::size_t target = budget.typePartCapacityTarget(
            parts.capacity(), parts.size() + 1u);
        parts.reserve(target);
        budget.accountTypePartCapacity(parts.capacity());
    }
    parts.push_back(std::move(part));
}

// Read one record up to RECORD_END (or a data-end marker at stream end).
DRW_SabRecord readRecord(SabByteReader& r, SabParseBudget& budget) {
    DRW_SabRecord out;
    std::vector<std::string> typeParts;
    std::string type;
    std::size_t subtypeLevel = 0;

    for (;;) {
        if (!r.hasData()) {
            if (isEndMarker(type) && typeParts.empty() && subtypeLevel == 0) {
                out.type = std::move(type);
                return out;
            }
            throw SabError();
        }
        budget.consumeTag();
        int tag = r.readByte();
        switch (tag) {
        case DRW_SabTag::Int: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::Double: {
            DRW_SabToken t = tok(tag); t.dval = r.readFloat();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::Str: {
            DRW_SabToken t = tok(tag);
            t.sval = r.readStr(r.readByte(), budget);
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::Pointer: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::BoolTrue: {
            DRW_SabToken t = tok(tag); t.bval = true;
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::BoolFalse: {
            DRW_SabToken t = tok(tag); t.bval = false;
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::LiteralStr: {
            const int length = r.readInt();
            if (length < 0) throw SabError();
            DRW_SabToken t = tok(tag);
            t.sval = r.readStr(static_cast<std::size_t>(length), budget);
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::EntityTypeEx: {
            appendTypePart(typeParts, r.readStr(r.readByte(), budget), budget);
            break;
        }
        case DRW_SabTag::EntityType: {
            appendTypePart(typeParts, r.readStr(r.readByte(), budget), budget);
            std::size_t nameLength = typeParts.empty() ? 0u : typeParts.size() - 1u;
            for (const std::string& part : typeParts) {
                if (part.size() > std::numeric_limits<std::size_t>::max()
                                      - nameLength)
                    throw SabError();
                nameLength += part.size();
            }
            budget.accountString(nameLength);
            std::string name;
            name.reserve(nameLength);
            for (std::size_t i = 0; i < typeParts.size(); ++i) {
                if (i) name += '-';
                name += typeParts[i];
            }
            typeParts.clear();
            if (type.empty()) {
                budget.accountString(name.size());
                type = name;
            }
            DRW_SabToken t = tok(tag);
            t.sval = std::move(name);
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::LocationVec: {
            DRW_SabToken t = tok(tag); t.vec = r.readVec();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::DirectionVec: {
            DRW_SabToken t = tok(tag); t.vec = r.readVec();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::Enum: {
            DRW_SabToken t = tok(tag); t.ival = r.readInt();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::Unknown0x17: {
            DRW_SabToken t = tok(tag); t.dval = r.readFloat();
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::SubtypeStart: {
            if (subtypeLevel == std::numeric_limits<std::size_t>::max())
                throw SabError();
            ++subtypeLevel;
            budget.checkSubtypeDepth(subtypeLevel);
            DRW_SabToken t = tok(tag); t.ival = subtypeLevel;
            appendSabToken(out.tokens, std::move(t), budget);
            break;
        }
        case DRW_SabTag::SubtypeEnd: {
            if (subtypeLevel == 0) throw SabError();
            DRW_SabToken t = tok(tag); t.ival = subtypeLevel;
            appendSabToken(out.tokens, std::move(t), budget);
            --subtypeLevel;
            break;
        }
        case DRW_SabTag::RecordEnd:
            if (type.empty() || !typeParts.empty() || subtypeLevel != 0)
                throw SabError();
            out.type = std::move(type);
            return out;
        default:
            throw SabError();
        }
    }
}

// ── Layer 3 helpers: graph navigation (geometry.ts) ────────────────────────

bool endsWith(const std::string& s, const std::string& suffix) {
    return s.size() >= suffix.size()
        && s.compare(s.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const DRW_AcisNode* nodeAt(const DRW_AcisModel& m, int idx) {
    if (idx < 0 || idx >= static_cast<int>(m.nodes.size())) return nullptr;
    return &m.nodes[idx];
}

bool isFinite(const DRW_Coord& point);

// First non-null ref whose type == suffix or endsWith(suffix); -1 if none.
int refOfType(const DRW_AcisModel& m, const DRW_AcisNode& node,
              const std::string& suffix, SabParseBudget& budget) {
    for (int ref : node.refs) {
        budget.accountExtractionWork();
        const DRW_AcisNode* n = nodeAt(m, ref);
        if (n && (n->type == suffix || endsWith(n->type, suffix))) return ref;
    }
    return -1;
}

int exactRefOfType(const DRW_AcisModel& m, const DRW_AcisNode& node,
                   const std::string& type, SabParseBudget& budget) {
    for (int ref : node.refs) {
        budget.accountExtractionWork();
        const DRW_AcisNode* n = nodeAt(m, ref);
        if (n && n->type == type)
            return ref;
    }
    return -1;
}

bool isStrictAcis21800(const DRW_SabHeader& header) {
    return header.signature == "ACIS BinaryFile" && header.version == 21800;
}

int pointerRefOfType(const DRW_AcisModel& model, const DRW_AcisNode& node,
                     std::size_t ordinal, const char* type,
                     SabParseBudget& budget) {
    budget.accountExtractionWork();
    if (ordinal >= node.refs.size())
        return -1;
    const int index = node.refs[ordinal];
    const DRW_AcisNode* target = nodeAt(model, index);
    return target && target->type == type ? index : -1;
}

struct ParsedAcisUseIndexes {
    std::map<int, std::vector<int>> coedgeNodeIndicesByEdge;
    std::map<int, std::vector<int>> coedgeNodeIndicesByLoop;
    std::map<int, std::vector<int>> loopNodeIndicesByFace;
    std::map<int, std::vector<int>> faceNodeIndicesByShell;
    std::map<int, std::vector<int>> shellNodeIndicesByLump;
    std::size_t unclassifiedCoedgeCount = 0u;
    int firstUnclassifiedCoedgeNodeIndex = -1;
    std::size_t unclassifiedCoedgeOwnerCount = 0u;
    int firstUnclassifiedCoedgeOwnerNodeIndex = -1;
    std::size_t unclassifiedLoopCount = 0u;
    int firstUnclassifiedLoopNodeIndex = -1;
    std::size_t unclassifiedFaceCount = 0u;
    int firstUnclassifiedFaceNodeIndex = -1;
    std::size_t unclassifiedShellCount = 0u;
    int firstUnclassifiedShellNodeIndex = -1;

    bool coversParsedCoedges() const {
        return unclassifiedCoedgeCount == 0u;
    }

    bool coversParsedCoedgeOwners() const {
        return unclassifiedCoedgeOwnerCount == 0u;
    }

    bool coversParsedLoops() const {
        return unclassifiedLoopCount == 0u;
    }

    bool coversParsedFaces() const {
        return unclassifiedFaceCount == 0u;
    }

    bool coversParsedShells() const {
        return unclassifiedShellCount == 0u;
    }
};

ParsedAcisUseIndexes indexParsedAcisUses(
    const DRW_AcisModel& model, const std::set<int>& projectedEdgeIndices,
    const std::set<int>& projectedLoopIndices,
    const std::set<int>& projectedFaceIndices,
    const std::set<int>& projectedShellIndices,
    const std::set<int>& projectedLumpIndices, SabParseBudget& budget) {
    ParsedAcisUseIndexes parsedUses;
    if (!isStrictAcis21800(model.header)
        || (projectedEdgeIndices.empty() && projectedLoopIndices.empty()
            && projectedFaceIndices.empty()
            && projectedShellIndices.empty() && projectedLumpIndices.empty()))
        return parsedUses;

    // Build source-ordered indexes for the incidence families in one pass.
    // Rows retain explicit, well-typed coedge ordinal-4 -> edge, coedge
    // ordinal-5 -> loop, loop ordinal-3 -> face, face ordinal-3 -> shell,
    // and shell ordinal-5 -> lump references. Account for unclassified
    // records too, so a partial index is never reported as a complete
    // parsed-model census.
    for (std::size_t sourceIndex = 0u;
         sourceIndex < model.nodes.size(); ++sourceIndex) {
        budget.accountExtractionWork();
        const DRW_AcisNode& source = model.nodes[sourceIndex];
        if (!projectedEdgeIndices.empty() && source.type == "coedge") {
            const auto markUnclassified = [&parsedUses, sourceIndex]() {
                ++parsedUses.unclassifiedCoedgeCount;
                if (parsedUses.firstUnclassifiedCoedgeNodeIndex < 0)
                    parsedUses.firstUnclassifiedCoedgeNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.refs.size() <= 4u) {
                markUnclassified();
            } else {
                const int edgeIndex = source.refs[4u];
                const DRW_AcisNode* edge = nodeAt(model, edgeIndex);
                if (!edge || edge->type != "edge") {
                    markUnclassified();
                } else if (projectedEdgeIndices.find(edgeIndex)
                           != projectedEdgeIndices.end()) {
                    auto indexed =
                        parsedUses.coedgeNodeIndicesByEdge.find(edgeIndex);
                    if (indexed
                        == parsedUses.coedgeNodeIndicesByEdge.end()) {
                        budget.accountOutputItems(
                            2u); // map entry and vector object
                        indexed = parsedUses.coedgeNodeIndicesByEdge.emplace(
                            edgeIndex, std::vector<int>()).first;
                    }
                    budget.accountOutputItems();
                    indexed->second.push_back(
                        static_cast<int>(sourceIndex));
                }
            }
        }

        if (!projectedLoopIndices.empty() && source.type == "coedge") {
            const auto markUnclassified = [&parsedUses, sourceIndex]() {
                ++parsedUses.unclassifiedCoedgeOwnerCount;
                if (parsedUses.firstUnclassifiedCoedgeOwnerNodeIndex < 0)
                    parsedUses.firstUnclassifiedCoedgeOwnerNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.refs.size() <= 5u) {
                markUnclassified();
            } else {
                const int loopIndex = source.refs[5u];
                const DRW_AcisNode* loop = nodeAt(model, loopIndex);
                if (!loop || loop->type != "loop") {
                    markUnclassified();
                } else if (projectedLoopIndices.find(loopIndex)
                           != projectedLoopIndices.end()) {
                    auto indexed =
                        parsedUses.coedgeNodeIndicesByLoop.find(loopIndex);
                    if (indexed
                        == parsedUses.coedgeNodeIndicesByLoop.end()) {
                        budget.accountOutputItems(
                            2u); // map entry and vector object
                        indexed = parsedUses.coedgeNodeIndicesByLoop.emplace(
                            loopIndex, std::vector<int>()).first;
                    }
                    budget.accountOutputItems();
                    indexed->second.push_back(
                        static_cast<int>(sourceIndex));
                }
            }
        }

        if (!projectedFaceIndices.empty() && source.type == "loop") {
            const auto markUnclassified = [&parsedUses, sourceIndex]() {
                ++parsedUses.unclassifiedLoopCount;
                if (parsedUses.firstUnclassifiedLoopNodeIndex < 0)
                    parsedUses.firstUnclassifiedLoopNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.refs.size() <= 3u) {
                markUnclassified();
            } else {
                const int faceIndex = source.refs[3u];
                const DRW_AcisNode* face = nodeAt(model, faceIndex);
                if (!face || face->type != "face") {
                    markUnclassified();
                } else if (projectedFaceIndices.find(faceIndex)
                           != projectedFaceIndices.end()) {
                    auto indexed =
                        parsedUses.loopNodeIndicesByFace.find(faceIndex);
                    if (indexed
                        == parsedUses.loopNodeIndicesByFace.end()) {
                        budget.accountOutputItems(
                            2u); // map entry and vector object
                        indexed = parsedUses.loopNodeIndicesByFace.emplace(
                            faceIndex, std::vector<int>()).first;
                    }
                    budget.accountOutputItems();
                    indexed->second.push_back(
                        static_cast<int>(sourceIndex));
                }
            }
        }

        if (!projectedShellIndices.empty() && source.type == "face") {
            const auto markUnclassified = [&parsedUses, sourceIndex]() {
                ++parsedUses.unclassifiedFaceCount;
                if (parsedUses.firstUnclassifiedFaceNodeIndex < 0)
                    parsedUses.firstUnclassifiedFaceNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.refs.size() <= 3u) {
                markUnclassified();
            } else {
                const int shellIndex = source.refs[3u];
                const DRW_AcisNode* shell = nodeAt(model, shellIndex);
                if (!shell || shell->type != "shell") {
                    markUnclassified();
                } else if (projectedShellIndices.find(shellIndex)
                           != projectedShellIndices.end()) {
                    auto indexed =
                        parsedUses.faceNodeIndicesByShell.find(shellIndex);
                    if (indexed
                        == parsedUses.faceNodeIndicesByShell.end()) {
                        budget.accountOutputItems(
                            2u); // map entry and vector object
                        indexed = parsedUses.faceNodeIndicesByShell.emplace(
                            shellIndex, std::vector<int>()).first;
                    }
                    budget.accountOutputItems();
                    indexed->second.push_back(
                        static_cast<int>(sourceIndex));
                }
            }
        }

        if (!projectedLumpIndices.empty() && source.type == "shell") {
            const auto markUnclassified = [&parsedUses, sourceIndex]() {
                ++parsedUses.unclassifiedShellCount;
                if (parsedUses.firstUnclassifiedShellNodeIndex < 0)
                    parsedUses.firstUnclassifiedShellNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.refs.size() <= 5u) {
                markUnclassified();
            } else {
                const int lumpIndex = source.refs[5u];
                const DRW_AcisNode* lump = nodeAt(model, lumpIndex);
                if (!lump || lump->type != "lump") {
                    markUnclassified();
                } else if (projectedLumpIndices.find(lumpIndex)
                           != projectedLumpIndices.end()) {
                    auto indexed =
                        parsedUses.shellNodeIndicesByLump.find(lumpIndex);
                    if (indexed
                        == parsedUses.shellNodeIndicesByLump.end()) {
                        budget.accountOutputItems(
                            2u); // map entry and vector object
                        indexed = parsedUses.shellNodeIndicesByLump.emplace(
                            lumpIndex, std::vector<int>()).first;
                    }
                    budget.accountOutputItems();
                    indexed->second.push_back(
                        static_cast<int>(sourceIndex));
                }
            }
        }
    }
    return parsedUses;
}

void projectCoedgePartnerRing(
    const DRW_AcisModel& model, int startCoedgeIndex,
    SabParseBudget& budget, std::vector<int>& coedgeNodeIndices,
    std::vector<int>& nextCoedgeNodeIndices, bool& hasObservation,
    bool& ringClosed) {
    hasObservation = isStrictAcis21800(model.header);
    ringClosed = false;
    coedgeNodeIndices.clear();
    nextCoedgeNodeIndices.clear();
    if (!hasObservation || startCoedgeIndex < 0)
        return;

    // Partner links form a circular list in this reviewed profile. Retain the
    // exact unique traversal and each outgoing target so a cycle that enters a
    // different coedge is distinguishable from a ring returning to its root.
    std::set<int> seen;
    int currentCoedgeIndex = startCoedgeIndex;
    while (currentCoedgeIndex >= 0) {
        budget.accountExtractionWork();
        if (seen.find(currentCoedgeIndex) != seen.end()) {
            ringClosed = currentCoedgeIndex == startCoedgeIndex;
            break;
        }

        // Account for the cycle guard and both retained identity rows before
        // allocating any of their entries.
        budget.accountOutputItems(3u);
        seen.insert(currentCoedgeIndex);
        coedgeNodeIndices.push_back(currentCoedgeIndex);

        const DRW_AcisNode* coedge = nodeAt(model, currentCoedgeIndex);
        const int nextCoedgeIndex = coedge
            ? pointerRefOfType(model, *coedge, 3u, "coedge", budget) : -1;
        nextCoedgeNodeIndices.push_back(nextCoedgeIndex);
        if (nextCoedgeIndex < 0)
            break;
        currentCoedgeIndex = nextCoedgeIndex;
    }
}

bool coedgePartnerRingMatchesEdgeUseIndex(
    int startCoedgeIndex, const std::vector<int>& ringNodeIndices,
    const std::vector<int>& ringNextNodeIndices, bool ringClosed,
    const std::vector<int>& edgeUseNodeIndices,
    bool edgeUseIndexCoversParsedCoedges, SabParseBudget& budget) {
    if (!edgeUseIndexCoversParsedCoedges || !ringClosed
        || ringNodeIndices.empty()
        || ringNodeIndices.size() != ringNextNodeIndices.size()
        || ringNodeIndices.size() != edgeUseNodeIndices.size()
        || ringNodeIndices.front() != startCoedgeIndex)
        return false;

    for (std::size_t index = 0u; index < ringNodeIndices.size(); ++index) {
        budget.accountExtractionWork();
        const int expectedTarget = index + 1u < ringNodeIndices.size()
            ? ringNodeIndices[index + 1u] : ringNodeIndices.front();
        if (ringNextNodeIndices[index] != expectedTarget)
            return false;

        // The use index is assembled in model-node order, so binary search
        // proves set equality when combined with equal, duplicate-free sizes.
        std::size_t low = 0u;
        std::size_t high = edgeUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (edgeUseNodeIndices[middle] < ringNodeIndices[index])
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= edgeUseNodeIndices.size()
            || edgeUseNodeIndices[low] != ringNodeIndices[index])
            return false;
    }
    return true;
}

bool faceLoopChainMatchesUseIndex(
    const std::vector<DRW_AcisFaceLoop>& faceLoops,
    const std::vector<int>& loopUseNodeIndices,
    const std::vector<int>& loopNextNodeIndices,
    bool loopChainNullTerminated, bool loopChainCycleDetected,
    std::size_t loopChainUnclassifiedNextLinkCount,
    bool loopUseIndexCoversParsedLoops, SabParseBudget& budget) {
    if (!loopUseIndexCoversParsedLoops || faceLoops.empty()
        || faceLoops.size() != loopUseNodeIndices.size()
        || faceLoops.size() != loopNextNodeIndices.size()
        || !loopChainNullTerminated || loopChainCycleDetected
        || loopChainUnclassifiedNextLinkCount != 0u)
        return false;

    for (std::size_t index = 0u; index < faceLoops.size(); ++index) {
        const DRW_AcisFaceLoop& faceLoop = faceLoops[index];
        budget.accountExtractionWork();
        const int expectedNextLoop = index + 1u < faceLoops.size()
            ? faceLoops[index + 1u].nodeIndex : -1;
        if (loopNextNodeIndices[index] != expectedNextLoop)
            return false;
        std::size_t low = 0u;
        std::size_t high = loopUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (loopUseNodeIndices[middle] < faceLoop.nodeIndex)
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= loopUseNodeIndices.size()
            || loopUseNodeIndices[low] != faceLoop.nodeIndex)
            return false;
    }
    return true;
}

bool loopCoedgeChainMatchesUseIndex(
    int loopNodeIndex, const std::vector<int>& coedgeNodeIndices,
    const std::vector<int>& coedgeNextNodeIndices,
    const std::vector<int>& coedgeLoopNodeIndices,
    bool hasCoedgeRingClosure, bool coedgeRingClosed, bool coedgeRingOpen,
    bool coedgeLinksReciprocal,
    const std::vector<int>& coedgeUseNodeIndices,
    bool coedgeUseIndexCoversParsedCoedges, SabParseBudget& budget) {
    const std::size_t coedgeCount = coedgeNodeIndices.size();
    if (!coedgeUseIndexCoversParsedCoedges || coedgeCount == 0u
        || coedgeCount != coedgeUseNodeIndices.size()
        || coedgeCount != coedgeNextNodeIndices.size()
        || coedgeCount != coedgeLoopNodeIndices.size()
        || !hasCoedgeRingClosure
        || coedgeRingClosed == coedgeRingOpen
        || !coedgeLinksReciprocal)
        return false;

    for (std::size_t index = 0u; index < coedgeCount; ++index) {
        budget.accountExtractionWork();
        if (coedgeUseNodeIndices[index] < 0
            || (index > 0u
                && coedgeUseNodeIndices[index - 1u]
                    >= coedgeUseNodeIndices[index]))
            return false;

        const int expectedNext = index + 1u < coedgeCount
            ? coedgeNodeIndices[index + 1u]
            : (coedgeRingClosed ? coedgeNodeIndices.front() : -1);
        if (coedgeNextNodeIndices[index] != expectedNext
            || coedgeLoopNodeIndices[index] != loopNodeIndex)
            return false;

        // The parsed-model census is source ordered. Equal cardinality plus
        // unique chain nodes and exact membership proves identity-set equality
        // while allowing source order to differ from next-link traversal.
        std::size_t low = 0u;
        std::size_t high = coedgeUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (coedgeUseNodeIndices[middle]
                < coedgeNodeIndices[index])
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= coedgeUseNodeIndices.size()
            || coedgeUseNodeIndices[low]
                != coedgeNodeIndices[index])
            return false;
    }
    return true;
}

bool shellFaceChainMatchesUseIndex(
    const std::vector<int>& faceNodeIndices,
    const std::vector<int>& faceNextNodeIndices,
    const std::vector<int>& faceUseNodeIndices,
    bool faceChainNullTerminated, bool faceChainCycleDetected,
    std::size_t faceChainUnclassifiedLinkCount,
    bool faceUseIndexCoversParsedFaces, SabParseBudget& budget) {
    if (!faceUseIndexCoversParsedFaces || faceNodeIndices.empty()
        || faceNodeIndices.size() != faceNextNodeIndices.size()
        || faceNodeIndices.size() != faceUseNodeIndices.size()
        || !faceChainNullTerminated || faceChainCycleDetected
        || faceChainUnclassifiedLinkCount != 0u)
        return false;

    for (std::size_t index = 0u; index < faceNodeIndices.size(); ++index) {
        budget.accountExtractionWork();
        const int expectedNext = index + 1u < faceNodeIndices.size()
            ? faceNodeIndices[index + 1u] : -1;
        if (faceNextNodeIndices[index] != expectedNext)
            return false;

        std::size_t low = 0u;
        std::size_t high = faceUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (faceUseNodeIndices[middle] < faceNodeIndices[index])
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= faceUseNodeIndices.size()
            || faceUseNodeIndices[low] != faceNodeIndices[index])
            return false;
    }
    return true;
}

bool lumpShellChainMatchesUseIndex(
    const std::vector<int>& shellNodeIndices,
    const std::vector<int>& shellNextNodeIndices,
    const std::vector<int>& shellUseNodeIndices,
    bool shellChainNullTerminated, bool shellChainCycleDetected,
    std::size_t shellChainUnclassifiedLinkCount,
    bool shellUseIndexCoversParsedShells, SabParseBudget& budget) {
    if (!shellUseIndexCoversParsedShells || shellNodeIndices.empty()
        || shellNodeIndices.size() != shellNextNodeIndices.size()
        || shellNodeIndices.size() != shellUseNodeIndices.size()
        || !shellChainNullTerminated || shellChainCycleDetected
        || shellChainUnclassifiedLinkCount != 0u)
        return false;

    for (std::size_t index = 0u; index < shellNodeIndices.size(); ++index) {
        budget.accountExtractionWork();
        const int expectedNext = index + 1u < shellNodeIndices.size()
            ? shellNodeIndices[index + 1u] : -1;
        if (shellNextNodeIndices[index] != expectedNext)
            return false;

        std::size_t low = 0u;
        std::size_t high = shellUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (shellUseNodeIndices[middle] < shellNodeIndices[index])
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= shellUseNodeIndices.size()
            || shellUseNodeIndices[low] != shellNodeIndices[index])
            return false;
    }
    return true;
}

bool bodyLumpChainMatchesUseIndex(
    const std::vector<int>& lumpNodeIndices,
    const std::vector<int>& lumpNextNodeIndices,
    const std::vector<int>& lumpUseNodeIndices,
    bool lumpChainNullTerminated, bool lumpChainCycleDetected,
    std::size_t lumpChainUnclassifiedLinkCount,
    bool lumpUseIndexCoversParsedLumps, SabParseBudget& budget) {
    if (!lumpUseIndexCoversParsedLumps || lumpNodeIndices.empty()
        || lumpNodeIndices.size() != lumpNextNodeIndices.size()
        || lumpNodeIndices.size() != lumpUseNodeIndices.size()
        || !lumpChainNullTerminated || lumpChainCycleDetected
        || lumpChainUnclassifiedLinkCount != 0u)
        return false;

    for (std::size_t index = 0u; index < lumpNodeIndices.size(); ++index) {
        budget.accountExtractionWork();
        const int expectedNext = index + 1u < lumpNodeIndices.size()
            ? lumpNodeIndices[index + 1u] : -1;
        if (lumpNextNodeIndices[index] != expectedNext)
            return false;

        std::size_t low = 0u;
        std::size_t high = lumpUseNodeIndices.size();
        while (low < high) {
            budget.accountExtractionWork();
            const std::size_t middle = low + (high - low) / 2u;
            if (lumpUseNodeIndices[middle] < lumpNodeIndices[index])
                low = middle + 1u;
            else
                high = middle;
        }
        if (low >= lumpUseNodeIndices.size()
            || lumpUseNodeIndices[low] != lumpNodeIndices[index])
            return false;
    }
    return true;
}

int pointerRefWithSuffix(const DRW_AcisModel& model, const DRW_AcisNode& node,
                         std::size_t ordinal, const char* suffix,
                         SabParseBudget& budget) {
    budget.accountExtractionWork();
    if (ordinal >= node.refs.size())
        return -1;
    const int index = node.refs[ordinal];
    const DRW_AcisNode* target = nodeAt(model, index);
    return target && (target->type == suffix || endsWith(target->type, suffix))
        ? index : -1;
}

// All non-null refs whose type == `type` exactly.
bool vertexPoint(const DRW_AcisModel& m, int vertexIdx, DRW_Coord& out,
                 SabParseBudget& budget,
                 int* invalidPointNodeIndex = nullptr,
                 int* resolvedPointNodeIndex = nullptr,
                 bool exact21800PointReference = false) {
    const DRW_AcisNode* vertex = nodeAt(m, vertexIdx);
    if (!vertex) return false;
    const int pointIdx = exact21800PointReference
        ? pointerRefOfType(m, *vertex, 2u, "point", budget)
        : refOfType(m, *vertex, "point", budget);
    const DRW_AcisNode* point = nodeAt(m, pointIdx);
    if (!point) return false;
    if (resolvedPointNodeIndex)
        *resolvedPointNodeIndex = pointIdx;
    const std::size_t start = semanticTokenStart(m.header, point->record);
    for (std::size_t i = start; i < point->record.tokens.size(); ++i) {
        budget.accountExtractionWork();
        const DRW_SabToken& t = point->record.tokens[i];
        if (t.tag == DRW_SabTag::LocationVec) {
            if (!isFinite(t.vec) && invalidPointNodeIndex) {
                *invalidPointNodeIndex = pointIdx;
                return false;
            }
            out = t.vec;
            return true;
        }
    }
    return false;
}

// Walks a record's tokens in wire order, yielding the next token of a wanted tag.
class TokenCursor {
public:
    TokenCursor(const std::vector<DRW_SabToken>& tokens,
                std::size_t start, SabParseBudget& budget)
        : m_tokens(tokens), m_budget(budget),
          m_i(std::min(start, tokens.size())) {}

    bool nextVec(DRW_Coord& out) {
        const DRW_SabToken* t = next2(DRW_SabTag::LocationVec, DRW_SabTag::DirectionVec);
        if (!t) return false;
        out = t->vec;
        return true;
    }
    bool nextDouble(double& out) {
        const DRW_SabToken* t = next1(DRW_SabTag::Double);
        if (!t) return false;
        out = t->dval;
        return true;
    }

private:
    const std::vector<DRW_SabToken>& m_tokens;
    SabParseBudget& m_budget;
    std::size_t m_i = 0;
    const DRW_SabToken* next1(int tag) {
        while (m_i < m_tokens.size()) {
            m_budget.accountExtractionWork();
            const DRW_SabToken& t = m_tokens[m_i++];
            if (t.tag == tag) return &t;
        }
        return nullptr;
    }
    const DRW_SabToken* next2(int a, int b) {
        while (m_i < m_tokens.size()) {
            m_budget.accountExtractionWork();
            const DRW_SabToken& t = m_tokens[m_i++];
            if (t.tag == a || t.tag == b) return &t;
        }
        return nullptr;
    }
};

// Exact-field cursor for record layouts whose admitted field order is known.
// Pointer slots are skipped because graph references are validated separately;
// other token kinds stop the read instead of being silently scanned past.
class OrderedTokenCursor {
public:
    OrderedTokenCursor(const DRW_SabHeader& header,
                       const DRW_SabRecord& record,
                       SabParseBudget& budget)
        : m_tokens(record.tokens), m_budget(budget),
          m_i(std::min(semanticTokenStart(header, record),
                       record.tokens.size())) {
        // Hand-built records retain semantic offset zero, but may still carry
        // the public entity-identity marker before their fields.
        if (m_i == 0u && !m_tokens.empty()
            && m_tokens.front().tag == DRW_SabTag::EntityType
            && m_tokens.front().sval == record.type)
            m_i = 1u;
    }

    const DRW_SabToken* nextExact(int expectedTag) {
        skipPointers();
        if (m_i >= m_tokens.size())
            return nullptr;
        m_budget.accountExtractionWork();
        const DRW_SabToken& token = m_tokens[m_i++];
        return token.tag == expectedTag ? &token : nullptr;
    }

    const DRW_SabToken* nextVector() {
        skipPointers();
        if (m_i >= m_tokens.size())
            return nullptr;
        m_budget.accountExtractionWork();
        const DRW_SabToken& token = m_tokens[m_i++];
        return token.tag == DRW_SabTag::LocationVec
                || token.tag == DRW_SabTag::DirectionVec
            ? &token : nullptr;
    }

    const DRW_SabToken* nextOptionalVector() {
        skipPointers();
        if (m_i >= m_tokens.size()
            || (m_tokens[m_i].tag != DRW_SabTag::LocationVec
                && m_tokens[m_i].tag != DRW_SabTag::DirectionVec))
            return nullptr;
        m_budget.accountExtractionWork();
        return &m_tokens[m_i++];
    }

private:
    const std::vector<DRW_SabToken>& m_tokens;
    SabParseBudget& m_budget;
    std::size_t m_i = 0u;

    void skipPointers() {
        while (m_i < m_tokens.size()
               && m_tokens[m_i].tag == DRW_SabTag::Pointer) {
            m_budget.accountExtractionWork();
            ++m_i;
        }
    }
};

// Extract the nubs/nurbs B-spline control polygon from an intcurve's subtype
// tokens (see geometry.ts intcurveControlPoints).
std::vector<DRW_Coord> intcurveControlPoints(
    const std::vector<DRW_SabToken>& tokens, std::size_t start,
    SabParseBudget& budget) {
    std::vector<DRW_Coord> out;
    int nubs = -1;
    for (std::size_t k = std::min(start, tokens.size()); k < tokens.size(); ++k) {
        budget.accountExtractionWork();
        if (tokens[k].tag == DRW_SabTag::EntityType
            && (tokens[k].sval == "nubs" || tokens[k].sval == "nurbs")) {
            nubs = static_cast<int>(k);
            break;
        }
    }
    if (nubs < 0) return out;
    std::size_t i = static_cast<std::size_t>(nubs) + 1;
    long long numKnots = -1;
    for (; i < tokens.size(); ++i) {
        budget.accountExtractionWork();
        const DRW_SabToken& t = tokens[i];
        if (t.tag == DRW_SabTag::Double) break;
        if (t.tag == DRW_SabTag::Int) numKnots = t.ival;
        if (t.tag == DRW_SabTag::EntityType || t.tag == DRW_SabTag::SubtypeEnd) return out;
    }
    if (numKnots < 0) return out;
    for (long long k = 0; k < numKnots; ++k) {
        budget.accountExtractionWork();
        if (i >= tokens.size() || tokens[i].tag != DRW_SabTag::Double) return out;
        i += 1;
        if (i < tokens.size() && tokens[i].tag == DRW_SabTag::Int) {
            budget.accountExtractionWork();
            i += 1;
        }
    }
    DRW_Coord point;
    unsigned coordinate = 0;
    while (i < tokens.size() && tokens[i].tag == DRW_SabTag::Double) {
        budget.accountExtractionWork();
        if (coordinate == 0u) {
            point.x = tokens[i].dval;
            coordinate = 1u;
        } else if (coordinate == 1u) {
            point.y = tokens[i].dval;
            coordinate = 2u;
        } else {
            point.z = tokens[i].dval;
            budget.accountOutputItems();
            out.push_back(point);
            coordinate = 0u;
        }
        i += 1;
    }
    return out;
}

// Fill an edge's scalar analytic curve params from its curve node.
bool curveParams(const DRW_SabHeader& header, const DRW_AcisNode& curve,
                 DRW_AcisEdge& edge,
                 SabParseBudget& budget) {
    const bool strict21800 = header.signature == "ACIS BinaryFile"
        && header.version == 21800;
    if (strict21800
        && (curve.type == "straight-curve"
            || curve.type == "ellipse-curve")) {
        OrderedTokenCursor ordered(header, curve.record, budget);
        const DRW_SabToken* origin = ordered.nextVector();
        const DRW_SabToken* direction = ordered.nextVector();
        if (!origin || !direction)
            return false;

        if (curve.type == "straight-curve") {
            if (!isFinite(origin->vec) || !isFinite(direction->vec))
                return false;
            edge.curveType = DRW_AcisCurve::Straight;
            edge.p0 = origin->vec;
            edge.p1 = direction->vec;
            edge.hasCurve = true;
            return true;
        }

        const DRW_SabToken* majorAxis = ordered.nextVector();
        const DRW_SabToken* ratio =
            ordered.nextExact(DRW_SabTag::Double);
        if (!majorAxis || !ratio || !isFinite(origin->vec)
            || !isFinite(direction->vec) || !isFinite(majorAxis->vec)
            || !std::isfinite(ratio->dval))
            return false;
        edge.curveType = DRW_AcisCurve::Ellipse;
        edge.p0 = origin->vec;
        edge.p1 = direction->vec;
        edge.p2 = majorAxis->vec;
        edge.ratio = ratio->dval;
        edge.hasCurve = true;
        return true;
    }

    TokenCursor c(curve.record.tokens,
                  semanticTokenStart(header, curve.record), budget);
    if (curve.type == "straight-curve") {
        DRW_Coord origin, direction;
        if (c.nextVec(origin) && c.nextVec(direction)) {
            edge.curveType = DRW_AcisCurve::Straight;
            edge.p0 = origin;
            edge.p1 = direction;
            edge.hasCurve = true;
        }
    } else if (curve.type == "ellipse-curve") {
        DRW_Coord center, normal, majorAxis;
        double ratio;
        if (c.nextVec(center) && c.nextVec(normal) && c.nextVec(majorAxis) && c.nextDouble(ratio)) {
            edge.curveType = DRW_AcisCurve::Ellipse;
            edge.p0 = center;
            edge.p1 = normal;
            edge.p2 = majorAxis;
            edge.ratio = ratio;
            edge.hasCurve = true;
        }
    }
    return true;
}

bool sphereSurfaceParams(const DRW_SabHeader& header,
                         const DRW_AcisNode& surface, DRW_AcisFace& face,
                         SabParseBudget& budget);

// Fill a face's analytic surface params from its surface node.
bool surfaceParams(const DRW_SabHeader& header,
                   const DRW_AcisNode& surface, DRW_AcisFace& face,
                   SabParseBudget& budget) {
    const bool strict21800 = header.signature == "ACIS BinaryFile"
        && header.version == 21800;
    OrderedTokenCursor ordered(header, surface.record, budget);
    TokenCursor legacy(surface.record.tokens,
                       semanticTokenStart(header, surface.record), budget);
    if (surface.type == "plane-surface") {
        if (!strict21800) {
            DRW_Coord origin, normal, uDirection;
            if (legacy.nextVec(origin) && legacy.nextVec(normal)) {
                face.p0 = origin;
                face.p1 = normal;
                if (legacy.nextVec(uDirection)) {
                    face.p2 = uDirection;
                    face.hasUDir = true;
                }
                face.hasSurface = true;
            }
            return true;
        }
        const DRW_SabToken* origin =
            ordered.nextVector();
        const DRW_SabToken* normal =
            ordered.nextVector();
        if (!origin || !normal)
            return false;
        if (!isFinite(origin->vec) || !isFinite(normal->vec))
            return false;
        face.p0 = origin->vec;
        face.p1 = normal->vec;
        const DRW_SabToken* uDirection =
            ordered.nextOptionalVector();
        if (uDirection) {
            if (!isFinite(uDirection->vec))
                return false;
            face.p2 = uDirection->vec;
            face.hasUDir = true;
        }
        face.hasSurface = true;
        return true;
    } else if (surface.type == "cone-surface") {
        // The cone record includes version-specific continuation markers;
        // retain the established scan for this unresolved layout variant.
        DRW_Coord origin, axis, majorAxis;
        double ratio, sineAngle, cosineAngle;
        if (legacy.nextVec(origin) && legacy.nextVec(axis)
            && legacy.nextVec(majorAxis) && legacy.nextDouble(ratio)
            && legacy.nextDouble(sineAngle)
            && legacy.nextDouble(cosineAngle)) {
            if (strict21800
                && (!isFinite(origin) || !isFinite(axis)
                    || !isFinite(majorAxis) || !std::isfinite(ratio)
                    || !std::isfinite(sineAngle)
                    || !std::isfinite(cosineAngle)))
                return false;
            face.p0 = origin;
            face.p1 = axis;
            face.p2 = majorAxis;
            face.ratio = ratio;
            face.sineAngle = sineAngle;
            face.cosineAngle = cosineAngle;
            face.hasSurface = true;
        }
        return true;
    } else if (surface.type == "torus-surface") {
        if (!strict21800) {
            DRW_Coord center, axis;
            double majorRadius, minorRadius;
            if (legacy.nextVec(center) && legacy.nextVec(axis)
                && legacy.nextDouble(majorRadius)
                && legacy.nextDouble(minorRadius)) {
                face.p0 = center;
                face.p1 = axis;
                face.majorRadius = majorRadius;
                face.minorRadius = minorRadius;
                face.hasSurface = true;
            }
            return true;
        }
        const DRW_SabToken* center =
            ordered.nextVector();
        const DRW_SabToken* axis =
            ordered.nextVector();
        const DRW_SabToken* majorRadius =
            ordered.nextExact(DRW_SabTag::Double);
        const DRW_SabToken* minorRadius =
            ordered.nextExact(DRW_SabTag::Double);
        const DRW_SabToken* uDirection =
            ordered.nextVector();
        if (!center || !axis || !majorRadius || !minorRadius
            || !uDirection || !isFinite(center->vec) || !isFinite(axis->vec)
            || !std::isfinite(majorRadius->dval)
            || !std::isfinite(minorRadius->dval)
            || !isFinite(uDirection->vec))
            return false;
        face.p0 = center->vec;
        face.p1 = axis->vec;
        face.p2 = uDirection->vec;
        face.hasUDir = true;
        face.majorRadius = majorRadius->dval;
        face.minorRadius = minorRadius->dval;
        face.hasSurface = true;
        return true;
    } else if (surface.type == "sphere-surface") {
        if (strict21800)
            return sphereSurfaceParams(header, surface, face, budget);
        DRW_Coord center, uDirection, pole;
        double radius;
        if (legacy.nextVec(center) && legacy.nextDouble(radius)
            && legacy.nextVec(uDirection) && legacy.nextVec(pole)) {
            face.p0 = center;
            face.p1 = pole;
            face.p2 = uDirection;
            face.hasUDir = true;
            face.majorRadius = radius;
            face.hasSurface = true;
        }
        return true;
    }
    // Unknown surface records remain raw/unresolved in this diagnostic view.
    return true;
}

bool sphereSurfaceParams(const DRW_SabHeader& header,
                         const DRW_AcisNode& surface, DRW_AcisFace& face,
                         SabParseBudget& budget) {
    OrderedTokenCursor ordered(header, surface.record, budget);
    const DRW_SabToken* center =
        ordered.nextVector();
    const DRW_SabToken* radius = ordered.nextExact(DRW_SabTag::Double);
    const DRW_SabToken* uDirection =
        ordered.nextVector();
    const DRW_SabToken* pole =
        ordered.nextVector();
    if (!center || !radius || !uDirection || !pole
        || !isFinite(center->vec) || !std::isfinite(radius->dval)
        || !isFinite(uDirection->vec) || !isFinite(pole->vec))
        return false;

    face.p0 = center->vec;
    face.p1 = pole->vec;
    face.p2 = uDirection->vec;
    face.hasUDir = true;
    // DRW_AcisFace predates sphere support. Keep its signed radius in the
    // shared radius slot and document the sphere-specific meaning.
    face.majorRadius = radius->dval;
    face.hasSurface = true;
    return true;
}

DRW_AcisSurface surfaceKind(const std::string& type) {
    if (type == "plane-surface") return DRW_AcisSurface::Plane;
    if (type == "cone-surface") return DRW_AcisSurface::Cone;
    if (type == "torus-surface") return DRW_AcisSurface::Torus;
    if (type == "sphere-surface") return DRW_AcisSurface::Sphere;
    if (type == "spline-surface") return DRW_AcisSurface::Spline;
    return DRW_AcisSurface::Unknown;
}

DRW_AcisCurve curveKind(const std::string& type) {
    if (type == "straight-curve") return DRW_AcisCurve::Straight;
    if (type == "ellipse-curve") return DRW_AcisCurve::Ellipse;
    if (type == "intcurve-curve") return DRW_AcisCurve::Intcurve;
    return DRW_AcisCurve::Unknown;
}

// Capture coedge identities in a loop's next-link order, cycle-guarded. This
// preserves the source ring structure without deciding whether that ring is
// a geometrically valid B-rep boundary.
std::vector<int> loopCoedgeNodeIndices(const DRW_AcisModel& m,
                                      const DRW_AcisNode& loop,
                                      SabParseBudget& budget,
                                      bool& hasRingClosure,
                                      bool& ringClosed) {
    std::vector<int> indices;
    const bool strict21800 = isStrictAcis21800(m.header);
    hasRingClosure = strict21800;
    ringClosed = false;
    int start = strict21800
        ? pointerRefOfType(m, loop, 2u, "coedge", budget)
        : exactRefOfType(m, loop, "coedge", budget);
    if (start < 0) return indices;
    std::set<int> seen;
    int coedge = start;
    while (coedge >= 0) {
        budget.accountExtractionWork();
        if (seen.find(coedge) != seen.end()) {
            ringClosed = strict21800 && coedge == start;
            break;
        }
        // The cycle guard and retained ordered identity row occupy storage.
        budget.accountOutputItems(2u);
        seen.insert(coedge);
        indices.push_back(coedge);
        const DRW_AcisNode* c = nodeAt(m, coedge);
        if (!c) break;
        coedge = strict21800
            ? pointerRefOfType(m, *c, 1u, "coedge", budget)
            : exactRefOfType(m, *c, "coedge", budget);
    }
    return indices;
}

// Resolve an exact coedge pointer role for the admitted 21800 profile. Keep
// one slot per coedge and use -1 for null, absent, or untyped targets so
// consumers never infer a relation from entity-record order.
std::vector<int> loopCoedgePointerNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    std::size_t pointerOrdinal, const char* expectedType,
    SabParseBudget& budget) {
    std::vector<int> nodeIndices;
    const bool strict21800 = model.header.signature == "ACIS BinaryFile"
        && model.header.version == 21800;
    for (int coedgeIndex : coedgeNodeIndices) {
        budget.accountExtractionWork();
        int targetIndex = -1;
        const DRW_AcisNode* coedge = nodeAt(model, coedgeIndex);
        if (strict21800 && coedge
            && coedge->refs.size() > pointerOrdinal) {
            const int candidate = coedge->refs[pointerOrdinal];
            const DRW_AcisNode* target = nodeAt(model, candidate);
            if (target && target->type == expectedType)
                targetIndex = candidate;
        }
        budget.accountOutputItems();
        nodeIndices.push_back(targetIndex);
    }
    return nodeIndices;
}

std::vector<int> loopCoedgeEdgeNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 4u, "edge",
                                        budget);
}

std::vector<int> loopCoedgePreviousNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 2u,
                                        "coedge", budget);
}

std::vector<int> loopCoedgeNextNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 1u,
                                        "coedge", budget);
}

bool coedgeChainHasOpenEnds(const DRW_AcisModel& model,
                            const std::vector<int>& coedgeNodeIndices,
                            const std::vector<int>& nextNodeIndices,
                            const std::vector<int>& previousNodeIndices,
                            SabParseBudget& budget) {
    const std::size_t count = coedgeNodeIndices.size();
    if (count == 0u || nextNodeIndices.size() != count
        || previousNodeIndices.size() != count
        || previousNodeIndices.front() >= 0 || nextNodeIndices.back() >= 0)
        return false;

    const DRW_AcisNode* firstCoedge = nodeAt(model, coedgeNodeIndices.front());
    const DRW_AcisNode* lastCoedge = nodeAt(model, coedgeNodeIndices.back());
    if (!firstCoedge || !lastCoedge
        || firstCoedge->type != "coedge" || lastCoedge->type != "coedge"
        || firstCoedge->refs.size() != 7u || lastCoedge->refs.size() != 7u
        || firstCoedge->refs[2] != -1 || lastCoedge->refs[1] != -1)
        return false;

    for (std::size_t index = 0u; index < count; ++index) {
        budget.accountExtractionWork();
        const DRW_AcisNode* coedge =
            nodeAt(model, coedgeNodeIndices[index]);
        if (!coedge || coedge->type != "coedge"
            || coedge->refs.size() != 7u)
            return false;
        if (index + 1u == count)
            continue;
        if (nextNodeIndices[index] != coedgeNodeIndices[index + 1u]
            || previousNodeIndices[index + 1u]
                != coedgeNodeIndices[index])
            return false;
    }
    return true;
}

bool coedgeRingHasReciprocalLinks(
    const std::vector<int>& coedgeNodeIndices,
    const std::vector<int>& nextNodeIndices,
    const std::vector<int>& previousNodeIndices,
    bool ringClosed, bool ringOpen, SabParseBudget& budget) {
    const std::size_t count = coedgeNodeIndices.size();
    if (count == 0u || nextNodeIndices.size() != count
        || previousNodeIndices.size() != count
        || ringClosed == ringOpen)
        return false;
    // coedgeChainHasOpenEnds already verifies exact null endpoints and all
    // reciprocal interior links before setting ringOpen.
    if (ringOpen)
        return true;

    for (std::size_t index = 0u; index < count; ++index) {
        budget.accountExtractionWork();
        const int expectedNext = index + 1u < count
            ? coedgeNodeIndices[index + 1u] : coedgeNodeIndices.front();
        const int expectedPrevious = index == 0u
            ? coedgeNodeIndices.back() : coedgeNodeIndices[index - 1u];
        if (nextNodeIndices[index] != expectedNext
            || previousNodeIndices[index] != expectedPrevious)
            return false;
    }
    return true;
}

std::vector<int> loopCoedgePartnerNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 3u,
                                        "coedge", budget);
}

std::vector<int> loopEdgeEndpointVertexNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& edgeNodeIndices,
    std::size_t endpointPointerOrdinal, SabParseBudget& budget) {
    std::vector<int> nodeIndices;
    const bool strict21800 = isStrictAcis21800(model.header);
    for (int edgeIndex : edgeNodeIndices) {
        int vertexIndex = -1;
        const DRW_AcisNode* edge = nodeAt(model, edgeIndex);
        if (strict21800 && edge) {
            vertexIndex = pointerRefOfType(
                model, *edge, endpointPointerOrdinal, "vertex", budget);
        } else {
            budget.accountExtractionWork();
        }
        budget.accountOutputItems();
        nodeIndices.push_back(vertexIndex);
    }
    return nodeIndices;
}

std::vector<int> loopCoedgeLoopNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 5u, "loop",
                                        budget);
}

std::vector<int> loopCoedgePCurveNodeIndices(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    return loopCoedgePointerNodeIndices(model, coedgeNodeIndices, 6u, "pcurve",
                                        budget);
}

struct AcisBooleanFields {
    std::array<std::int8_t, 3> values {{-1, -1, -1}};
    std::size_t count = 0u;
};

// Read only consecutive SAB boolean fields that follow an exact pointer prefix.
// This keeps face flags and the coedge sense from drifting across intervening
// pointer slots in the 21800 record layout.
AcisBooleanFields booleanFieldsAfterPointerCount(
    const DRW_SabHeader& header, const DRW_AcisNode& node,
    std::size_t requiredPointerCount, SabParseBudget& budget) {
    AcisBooleanFields fields;
    const std::vector<DRW_SabToken>& tokens = node.record.tokens;
    std::size_t tokenIndex = semanticTokenStart(header, node.record);
    if (tokenIndex == 0u && !tokens.empty()
        && tokens.front().tag == DRW_SabTag::EntityType
        && tokens.front().sval == node.type)
        tokenIndex = 1u;

    std::size_t pointerCount = 0u;
    for (; tokenIndex < tokens.size(); ++tokenIndex) {
        budget.accountExtractionWork();
        const DRW_SabToken& token = tokens[tokenIndex];
        if (token.tag == DRW_SabTag::Pointer) {
            if (pointerCount == requiredPointerCount)
                break;
            ++pointerCount;
            continue;
        }
        if (pointerCount != requiredPointerCount)
            break;
        if (token.tag == DRW_SabTag::BoolTrue
            || token.tag == DRW_SabTag::BoolFalse) {
            if (fields.count == fields.values.size())
                break;
            fields.values[fields.count++] = static_cast<std::int8_t>(
                token.tag == DRW_SabTag::BoolTrue ? 1 : 0);
            continue;
        }
        break;
    }
    return fields;
}

std::vector<std::int8_t> loopCoedgeSenseFlags(
    const DRW_AcisModel& model, const std::vector<int>& coedgeNodeIndices,
    SabParseBudget& budget) {
    std::vector<std::int8_t> senseFlags;
    const bool strict21800 = model.header.signature == "ACIS BinaryFile"
        && model.header.version == 21800;
    for (int coedgeIndex : coedgeNodeIndices) {
        std::int8_t sense = -1;
        if (strict21800) {
            const DRW_AcisNode* coedge = nodeAt(model, coedgeIndex);
            if (coedge) {
                const AcisBooleanFields fields = booleanFieldsAfterPointerCount(
                    model.header, *coedge, 5u, budget);
                if (fields.count != 0u)
                    sense = fields.values[0];
            }
        } else {
            budget.accountExtractionWork();
        }
        budget.accountOutputItems();
        senseFlags.push_back(sense);
    }
    return senseFlags;
}

struct CoedgeOrientedEndpointNodeIndices {
    std::vector<int> start;
    std::vector<int> end;
};

CoedgeOrientedEndpointNodeIndices loopCoedgeOrientedEndpointNodeIndices(
    const std::vector<int>& edgeStartVertexNodeIndices,
    const std::vector<int>& edgeEndVertexNodeIndices,
    const std::vector<std::int8_t>& coedgeSenseFlags,
    SabParseBudget& budget) {
    CoedgeOrientedEndpointNodeIndices endpoints;
    const std::size_t count = coedgeSenseFlags.size();
    if (edgeStartVertexNodeIndices.size() != count
        || edgeEndVertexNodeIndices.size() != count) {
        return endpoints;
    }
    endpoints.start.reserve(count);
    endpoints.end.reserve(count);
    for (std::size_t index = 0u; index < count; ++index) {
        budget.accountExtractionWork();
        budget.accountOutputItems(2u);
        if (coedgeSenseFlags[index] == 0) {
            endpoints.start.push_back(
                edgeStartVertexNodeIndices[index]);
            endpoints.end.push_back(edgeEndVertexNodeIndices[index]);
        } else if (coedgeSenseFlags[index] == 1) {
            endpoints.start.push_back(
                edgeEndVertexNodeIndices[index]);
            endpoints.end.push_back(
                edgeStartVertexNodeIndices[index]);
        } else {
            endpoints.start.push_back(-1);
            endpoints.end.push_back(-1);
        }
    }
    return endpoints;
}

struct CoedgeEndpointChainObservation {
    bool hasObservation = false;
    bool continuous = false;
    std::size_t discontinuityCount = 0u;
    int firstDiscontinuityNodeIndex = -1;
};

CoedgeEndpointChainObservation observeCoedgeEndpointChain(
    const std::vector<int>& coedgeNodeIndices,
    const std::vector<int>& orientedStartVertexNodeIndices,
    const std::vector<int>& orientedEndVertexNodeIndices,
    bool ringClosed, bool ringOpen, bool linksReciprocal,
    SabParseBudget& budget) {
    CoedgeEndpointChainObservation observation;
    const std::size_t count = coedgeNodeIndices.size();
    if ((!ringClosed && !ringOpen) || !linksReciprocal || count == 0u
        || orientedStartVertexNodeIndices.size() != count
        || orientedEndVertexNodeIndices.size() != count) {
        return observation;
    }

    const std::size_t joinCount = ringClosed ? count : count - 1u;
    for (std::size_t index = 0u; index < joinCount; ++index) {
        budget.accountExtractionWork();
        const std::size_t nextIndex = (index + 1u) % count;
        const int endVertex = orientedEndVertexNodeIndices[index];
        const int nextStartVertex =
            orientedStartVertexNodeIndices[nextIndex];
        if (endVertex < 0 || nextStartVertex < 0) {
            observation.discontinuityCount = 0u;
            observation.firstDiscontinuityNodeIndex = -1;
            return observation;
        }
        if (endVertex != nextStartVertex) {
            ++observation.discontinuityCount;
            if (observation.firstDiscontinuityNodeIndex < 0)
                observation.firstDiscontinuityNodeIndex =
                    coedgeNodeIndices[index];
        }
    }
    observation.hasObservation = true;
    observation.continuous = observation.discontinuityCount == 0u;
    return observation;
}

std::size_t decimalLength(int value) {
    std::size_t length = value < 0 ? 1u : 0u;
    unsigned long long magnitude = value < 0
        ? static_cast<unsigned long long>(-(static_cast<long long>(value)))
        : static_cast<unsigned long long>(value);
    do {
        ++length;
        magnitude /= 10u;
    } while (magnitude != 0u);
    return length;
}

void appendDiagnostic(std::vector<std::string>& diagnostics,
                      SabParseBudget& budget,
                      const char* prefix, int index, const char* suffix) {
    const std::size_t prefixLength = std::strlen(prefix);
    const std::size_t suffixLength = std::strlen(suffix);
    const std::size_t numberLength = decimalLength(index);
    if (prefixLength > std::numeric_limits<std::size_t>::max()
                           - numberLength
        || prefixLength + numberLength
               > std::numeric_limits<std::size_t>::max() - suffixLength)
        throw SabError(true);
    const std::size_t length = prefixLength + numberLength + suffixLength;
    budget.accountString(length);
    budget.accountOutputItems();
    std::string message;
    message.reserve(length);
    message.append(prefix, prefixLength);
    char digits[sizeof(int) * 3u + 2u];
    std::size_t position = sizeof(digits);
    const bool negative = index < 0;
    unsigned long long magnitude = negative
        ? static_cast<unsigned long long>(-(static_cast<long long>(index)))
        : static_cast<unsigned long long>(index);
    do {
        digits[--position] = static_cast<char>('0' + magnitude % 10u);
        magnitude /= 10u;
    } while (magnitude != 0u);
    if (negative)
        digits[--position] = '-';
    message.append(digits + position, sizeof(digits) - position);
    message.append(suffix, suffixLength);
    diagnostics.push_back(std::move(message));
}

bool isFinite(const DRW_Coord& point) {
    return std::isfinite(point.x) && std::isfinite(point.y)
        && std::isfinite(point.z);
}

// Evaluate a product of three factors divided by two positive, finite factors
// without overflowing an intermediate when the final result is representable.
// frexp/ldexp keep exponent arithmetic bounded even for subnormal vectors.
bool finiteProductQuotient3(double first, double second, double third,
                            double firstDenominator,
                            double secondDenominator, double& result) {
    if (!std::isfinite(first) || !std::isfinite(second)
        || !std::isfinite(third) || !std::isfinite(firstDenominator)
        || !std::isfinite(secondDenominator)
        || firstDenominator <= 0.0 || secondDenominator <= 0.0)
        return false;
    if (first == 0.0 || second == 0.0 || third == 0.0) {
        result = 0.0;
        return true;
    }

    int firstExponent = 0;
    int secondExponent = 0;
    int thirdExponent = 0;
    int firstDenominatorExponent = 0;
    int secondDenominatorExponent = 0;
    const double firstMantissa = std::frexp(first, &firstExponent);
    const double secondMantissa = std::frexp(second, &secondExponent);
    const double thirdMantissa = std::frexp(third, &thirdExponent);
    const double firstDenominatorMantissa =
        std::frexp(firstDenominator, &firstDenominatorExponent);
    const double secondDenominatorMantissa =
        std::frexp(secondDenominator, &secondDenominatorExponent);
    const int resultExponent = firstExponent + secondExponent + thirdExponent
        - firstDenominatorExponent - secondDenominatorExponent;
    const double resultMantissa = firstMantissa * secondMantissa
        * thirdMantissa / firstDenominatorMantissa
        / secondDenominatorMantissa;
    const double candidate = std::ldexp(resultMantissa, resultExponent);
    if (!std::isfinite(candidate))
        return false;
    result = candidate;
    return true;
}

struct ScaledAcisProduct {
    double mantissa = 0.0;
    int exponent = 0;
};

bool scaledAcisProduct5(double first, double second, double third,
                        double fourth, double fifth,
                        ScaledAcisProduct& result) {
    const double factors[5] = {first, second, third, fourth, fifth};
    double mantissa = 1.0;
    int exponent = 0;
    for (double factor : factors) {
        if (!std::isfinite(factor))
            return false;
        if (factor == 0.0) {
            result = ScaledAcisProduct();
            return true;
        }
        int factorExponent = 0;
        mantissa *= std::frexp(factor, &factorExponent);
        exponent += factorExponent;
    }

    int adjustment = 0;
    mantissa = std::frexp(mantissa, &adjustment);
    result.mantissa = mantissa;
    result.exponent = exponent + adjustment;
    return true;
}

bool finiteScaledAcisSum6(const std::array<ScaledAcisProduct, 6>& terms,
                          double& result) {
    int largestExponent = std::numeric_limits<int>::min();
    for (const ScaledAcisProduct& term : terms) {
        if (term.mantissa != 0.0)
            largestExponent = std::max(largestExponent, term.exponent);
    }
    if (largestExponent == std::numeric_limits<int>::min()) {
        result = 0.0;
        return true;
    }

    double sum = 0.0;
    double correction = 0.0;
    for (const ScaledAcisProduct& term : terms) {
        if (term.mantissa == 0.0)
            continue;
        const double scaledTerm = std::ldexp(
            term.mantissa, term.exponent - largestExponent);
        const double adjustedTerm = scaledTerm - correction;
        const double nextSum = sum + adjustedTerm;
        correction = (nextSum - sum) - adjustedTerm;
        sum = nextSum;
    }
    const double candidate = std::ldexp(sum, largestExponent);
    if (!std::isfinite(candidate) || (sum != 0.0 && candidate == 0.0))
        return false;
    result = candidate;
    return true;
}

bool isFiniteProjection(const DRW_AcisBrep& brep, SabParseBudget& budget) {
    if (brep.hasBBox
        && (!isFinite(brep.bboxMin) || !isFinite(brep.bboxMax)))
        return false;

    for (const DRW_AcisVertex& vertex : brep.vertices) {
        budget.accountExtractionWork();
        if (vertex.valid && !isFinite(vertex.point))
            return false;
    }
    for (const DRW_AcisEdge& edge : brep.edges) {
        budget.accountExtractionWork();
        if ((edge.hasStart && !isFinite(edge.start))
            || (edge.hasEnd && !isFinite(edge.end)))
            return false;
        if (!edge.hasCurve)
            continue;
        switch (edge.curveType) {
        case DRW_AcisCurve::Straight:
            if (!isFinite(edge.p0) || !isFinite(edge.p1))
                return false;
            break;
        case DRW_AcisCurve::Ellipse:
            if (!isFinite(edge.p0) || !isFinite(edge.p1)
                || !isFinite(edge.p2) || !std::isfinite(edge.ratio))
                return false;
            break;
        case DRW_AcisCurve::Intcurve:
            for (const DRW_Coord& point : edge.controlPoints) {
                budget.accountExtractionWork();
                if (!isFinite(point))
                    return false;
            }
            break;
        case DRW_AcisCurve::Unknown:
            break;
        }
    }
    for (const DRW_AcisFace& face : brep.faces) {
        budget.accountExtractionWork();
        if (!face.hasSurface)
            continue;
        if (!isFinite(face.p0) || !isFinite(face.p1))
            return false;
        switch (face.surfaceType) {
        case DRW_AcisSurface::Plane:
            if (face.hasUDir && !isFinite(face.p2))
                return false;
            break;
        case DRW_AcisSurface::Cone:
            if (!isFinite(face.p2) || !std::isfinite(face.ratio)
                || !std::isfinite(face.sineAngle)
                || !std::isfinite(face.cosineAngle))
                return false;
            break;
        case DRW_AcisSurface::Torus:
            if ((face.hasUDir && !isFinite(face.p2))
                || !std::isfinite(face.majorRadius)
                || !std::isfinite(face.minorRadius))
                return false;
            break;
        case DRW_AcisSurface::Sphere:
            if ((face.hasUDir && !isFinite(face.p2))
                || !std::isfinite(face.majorRadius))
                return false;
            break;
        case DRW_AcisSurface::Spline:
        case DRW_AcisSurface::Unknown:
            break;
        }
    }
    return true;
}

void captureBudget(DRW_SabParseOutcome& outcome,
                   const SabParseBudget& budget) {
    outcome.recordsProcessed = budget.recordsProcessed;
    outcome.tagsProcessed = budget.tagsProcessed;
    outcome.stringBytesAccounted = budget.stringBytesAccounted;
    outcome.graphWorkUnitsProcessed = budget.graphWorkUnitsProcessed;
    outcome.graphItemsAccounted = budget.graphItemsAccounted;
    outcome.extractionWorkUnitsProcessed =
        budget.extractionWorkUnitsProcessed;
    outcome.outputItemsAccounted = budget.outputItemsAccounted;
    outcome.tokenCapacityItemsAccounted =
        budget.tokenCapacityItemsAccounted;
    outcome.typePartCapacityPeakItems = budget.typePartCapacityPeakItems;
}

bool parseSabPrefix(const std::uint8_t* data, std::size_t length,
                    DRW_SabData& out, DRW_SabParseOutcome& outcome,
                    SabParseBudget& budget, std::uint8_t shift = 0u) {
    out = DRW_SabData();
    outcome = DRW_SabParseOutcome();
    outcome.sourceByteCount = length;
    if (!data)
        return false;
    try {
        budget.checkInput(length);
        SabByteReader reader(data, length, shift);
        DRW_SabData parsed;
        parsed.header = readHeader(reader, data, length, shift, budget);
        if (parsed.header.numRecords < 0)
            throw SabError();

        // The pinned ezdxf 1.4.4 SAB 21800 witness uses zero for this
        // header count while carrying records. Only this profile has an
        // unspecified count; do not generalize it to other SABs.
        const bool unknownAcis21800Count =
            parsed.header.signature == "ACIS BinaryFile"
            && parsed.header.version == 21800
            && parsed.header.numRecords == 0;

        // A normal record needs at least EntityType + a non-empty name +
        // RecordEnd (4 bytes); the terminal marker may omit RecordEnd (3).
        // Use that lower bound to keep a corrupt positive count from causing
        // an input-independent reserve, while avoiding geometric growth for
        // ordinary known-count streams.
        const std::size_t remainingBytes = length - reader.index;
        const std::size_t remainder = remainingBytes % 4u;
        const std::size_t byteBound = remainingBytes / 4u
            + (remainder == 3u ? 1u : 0u);
        std::size_t reserveRecords = std::min(
            byteBound, budget.limits().maxRecords);
        if (!unknownAcis21800Count && parsed.header.numRecords > 0) {
            reserveRecords = std::min(
                reserveRecords,
                static_cast<std::size_t>(parsed.header.numRecords));
        }
        parsed.records.reserve(reserveRecords);

        while (reader.hasData()) {
            budget.consumeRecord();
            DRW_SabRecord record = readRecord(reader, budget);
            const bool marker = isEndMarker(record.type);
            const bool autodesk21800 =
                parsed.header.signature == "ACIS BinaryFile"
                && parsed.header.version == 21800;
            if (autodesk21800 && !marker) {
                if (record.tokens.size() < 3u
                    || record.tokens[0].tag != DRW_SabTag::EntityType
                    || record.tokens[0].sval != record.type
                    || record.tokens[1].tag != DRW_SabTag::Pointer
                    || record.tokens[2].tag != DRW_SabTag::Int)
                    throw SabError();
            }
            parsed.records.push_back(std::move(record));
            if (marker)
                break;
        }

        const bool countMatched = unknownAcis21800Count
            || parsed.records.size()
                == static_cast<std::size_t>(parsed.header.numRecords);
        if (!countMatched)
            throw SabError();

        outcome.consumedByteCount = reader.index;
        outcome.trailingByteCount = length - reader.index;
        outcome.terminalRecordSeen = !parsed.records.empty()
            && isEndMarker(parsed.records.back().type);
        outcome.recordCountKnown = !unknownAcis21800Count;
        outcome.recordCountMatched = outcome.recordCountKnown;
        outcome.complete = outcome.trailingByteCount == 0
            && (!unknownAcis21800Count || outcome.terminalRecordSeen);
        captureBudget(outcome, budget);
        out = std::move(parsed);
        return true;
    } catch (const SabError& error) {
        out = DRW_SabData();
        outcome = DRW_SabParseOutcome();
        outcome.sourceByteCount = length;
        captureBudget(outcome, budget);
        outcome.resourceLimitExceeded = error.resourceLimitExceeded;
        return false;
    } catch (const std::bad_alloc&) {
        out = DRW_SabData();
        outcome = DRW_SabParseOutcome();
        outcome.sourceByteCount = length;
        captureBudget(outcome, budget);
        outcome.allocationFailure = true;
        return false;
    } catch (...) {
        out = DRW_SabData();
        outcome = DRW_SabParseOutcome();
        outcome.sourceByteCount = length;
        captureBudget(outcome, budget);
        return false;
    }
}

} // namespace

// ── Layer 1 entry: drw_parseSab ────────────────────────────────────────────

bool drw_parseSabWithOutcome(const std::uint8_t* data, std::size_t length,
                             DRW_SabData& out,
                             DRW_SabParseOutcome& outcome) {
    const DRW_SabParseLimits limits;
    SabParseBudget budget(limits);
    return parseSabPrefix(data, length, out, outcome, budget);
}

bool drw_parseSabWithLimits(const std::uint8_t* data, std::size_t length,
                            DRW_SabData& out,
                            DRW_SabParseOutcome& outcome,
                            const DRW_SabParseLimits& limits) {
    SabParseBudget budget(limits);
    return parseSabPrefix(data, length, out, outcome, budget);
}

bool drw_parseSab(const std::uint8_t* data, std::size_t length,
                  DRW_SabData& out) {
    DRW_SabParseOutcome outcome;
    if (!drw_parseSabWithOutcome(data, length, out, outcome)
        || !outcome.complete) {
        out = DRW_SabData();
        return false;
    }
    return true;
}

// ── Layer 2 entry: drw_buildAcisModel ──────────────────────────────────────

std::vector<int> DRW_AcisModel::nodesOfType(const std::string& type) const {
    std::vector<int> out;
    for (const DRW_AcisNode& n : nodes) {
        if (isEndMarker(n.type)) continue;
        if (n.type == type) out.push_back(n.index);
    }
    return out;
}

namespace {

bool buildAcisModelImpl(const DRW_SabData& sab, DRW_AcisModel& out,
                        SabParseBudget& budget) {
    if (sab.records.size()
        > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;

    budget.accountGraphItems(sab.records.size());
    budget.accountGraphWork(sab.records.size());
    budget.accountString(sab.header.signature.size());
    budget.accountString(sab.header.productId.size());
    budget.accountString(sab.header.acisVersion.size());
    budget.accountString(sab.header.creationDate.size());

    // The temporary planner vector retains one slot per record while the
    // reference vectors are sized. Include those slots in the aggregate graph
    // item budget before reserving storage.
    budget.accountGraphItems(sab.records.size());
    std::vector<std::size_t> referenceCounts;
    referenceCounts.reserve(sab.records.size());
    std::size_t totalTokenCount = 0;
    for (const DRW_SabRecord& record : sab.records) {
        const std::size_t dataTokenStart =
            semanticTokenStart(sab.header, record);
        budget.accountString(record.type.size());
        budget.accountString(record.type.size());
        budget.accountGraphItems(record.tokens.size());
        budget.accountGraphWork(record.tokens.size());
        if (record.tokens.size()
            > std::numeric_limits<std::size_t>::max() - totalTokenCount)
            throw SabError(true);
        totalTokenCount += record.tokens.size();

        std::size_t referenceCount = 0;
        for (std::size_t tokenIndex = 0;
             tokenIndex < record.tokens.size(); ++tokenIndex) {
            const DRW_SabToken& token = record.tokens[tokenIndex];
            if (tokenIndex >= dataTokenStart
                && token.tag == DRW_SabTag::Pointer) {
                if (referenceCount == std::numeric_limits<std::size_t>::max())
                    throw SabError(true);
                ++referenceCount;
            }
            budget.accountString(token.sval.size());
        }
        budget.accountGraphItems(referenceCount);
        referenceCounts.push_back(referenceCount);
    }

    // Account the copy pass and pointer-resolution pass before allocating the
    // graph. The earlier pass counts strings and pointer slots; these two
    // passes bound the work performed by vector/string copies and ref builds.
    budget.accountGraphWork(totalTokenCount);
    budget.accountGraphWork(totalTokenCount);

    DRW_AcisModel model;
    model.header = sab.header;
    model.nodes.resize(sab.records.size());
    const int nodeCount = static_cast<int>(model.nodes.size());
    for (std::size_t i = 0; i < sab.records.size(); ++i) {
        DRW_AcisNode& node = model.nodes[i];
        const DRW_SabRecord& source = sab.records[i];
        node.index = static_cast<int>(i);
        node.type = source.type;
        node.record = source;
        node.refs.reserve(referenceCounts[i]);
        for (std::size_t tokenIndex =
                 semanticTokenStart(sab.header, node.record);
             tokenIndex < node.record.tokens.size(); ++tokenIndex) {
            const DRW_SabToken& token = node.record.tokens[tokenIndex];
            if (token.tag != DRW_SabTag::Pointer)
                continue;
            const bool targetInRange = token.ival >= 0
                && static_cast<unsigned long long>(token.ival)
                    < static_cast<unsigned long long>(nodeCount);
            node.refs.push_back(targetInRange
                                    ? static_cast<int>(token.ival) : -1);
        }
    }
    out = std::move(model);
    return true;
}

} // namespace

DRW_AcisModel drw_buildAcisModel(const DRW_SabData& sab) {
    const DRW_SabParseLimits limits;
    DRW_AcisModel model;
    DRW_SabParseOutcome outcome;
    drw_buildAcisModelWithLimits(sab, model, outcome, limits);
    return model;
}

bool drw_buildAcisModelWithLimits(const DRW_SabData& sab,
                                  DRW_AcisModel& out,
                                  DRW_SabParseOutcome& outcome,
                                  const DRW_SabParseLimits& limits) {
    out = DRW_AcisModel();
    outcome = DRW_SabParseOutcome();
    SabParseBudget budget(limits);
    try {
        if (!buildAcisModelImpl(sab, out, budget)) {
            captureBudget(outcome, budget);
            out = DRW_AcisModel();
            return false;
        }
        captureBudget(outcome, budget);
        return true;
    } catch (const SabError& error) {
        captureBudget(outcome, budget);
        outcome.resourceLimitExceeded = error.resourceLimitExceeded;
    } catch (const std::bad_alloc&) {
        captureBudget(outcome, budget);
        outcome.allocationFailure = true;
    } catch (...) {
        captureBudget(outcome, budget);
    }
    out = DRW_AcisModel();
    return false;
}

// ── Layer 3 entry: drw_extractAcisWireframe ────────────────────────────────

static bool extractAcisWireframeImpl(const DRW_AcisModel& model,
                                     DRW_AcisBrep& out,
                                     SabParseBudget& budget,
                                     const std::vector<int>* selectedNodes = nullptr,
                                     int* invalidSurfaceNodeIndex = nullptr,
                                     int* invalidCurveNodeIndex = nullptr,
                                     int* invalidPointNodeIndex = nullptr) {
    out = DRW_AcisBrep();
    if (model.nodes.size()
        > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        return false;
    const std::size_t candidateCount = selectedNodes
        ? selectedNodes->size() : model.nodes.size();
    const auto nodeIndexAt = [&](std::size_t ordinal, int& index) {
        if (selectedNodes) {
            if (ordinal >= selectedNodes->size())
                return false;
            index = (*selectedNodes)[ordinal];
        } else {
            if (ordinal >= model.nodes.size())
                return false;
            index = static_cast<int>(ordinal);
        }
        return index >= 0
            && static_cast<std::size_t>(index) < model.nodes.size();
    };

    double minX = std::numeric_limits<double>::infinity();
    double minY = minX, minZ = minX;
    double maxX = -minX, maxY = -minX, maxZ = -minX;
    bool grown = false;
    auto grow = [&](const DRW_Coord& p) {
        grown = true;
        if (p.x < minX)
            minX = p.x;
        if (p.x > maxX)
            maxX = p.x;
        if (p.y < minY)
            minY = p.y;
        if (p.y > maxY)
            maxY = p.y;
        if (p.z < minZ)
            minZ = p.z;
        if (p.z > maxZ)
            maxZ = p.z;
    };

    std::set<int> projectedEdgeIndices;
    std::set<int> projectedLoopIndices;
    std::set<int> projectedFaceIndices;
    std::set<int> projectedShellIndices;
    std::set<int> projectedLumpIndices;
    if (isStrictAcis21800(model.header)) {
        for (std::size_t ordinal = 0u; ordinal < candidateCount; ++ordinal) {
            budget.accountExtractionWork();
            int nodeIndex = -1;
            if (!nodeIndexAt(ordinal, nodeIndex))
                return false;
            const std::string& type =
                model.nodes[static_cast<std::size_t>(nodeIndex)].type;
            std::set<int>* projectedIndices = nullptr;
            if (type == "edge")
                projectedIndices = &projectedEdgeIndices;
            else if (type == "loop")
                projectedIndices = &projectedLoopIndices;
            else if (type == "face")
                projectedIndices = &projectedFaceIndices;
            else if (type == "shell")
                projectedIndices = &projectedShellIndices;
            else if (type == "lump")
                projectedIndices = &projectedLumpIndices;
            if (projectedIndices
                && projectedIndices->find(nodeIndex)
                    == projectedIndices->end()) {
                budget.accountOutputItems();
                projectedIndices->insert(nodeIndex);
            }
        }
    }
    const ParsedAcisUseIndexes parsedAcisUseIndexes = indexParsedAcisUses(
        model, projectedEdgeIndices, projectedLoopIndices,
        projectedFaceIndices,
        projectedShellIndices, projectedLumpIndices, budget);

    // A source loop may be referenced by several faces. Its coedge-ring
    // cardinality and next-loop reference are invariant for this immutable
    // model, so resolve them once while still emitting one DRW_AcisFaceLoop
    // per face reference below. Cache nodes are charged before traversal;
    // projected rows and each face's cycle guard retain separate accounting.
    struct CachedLoopProjection {
        std::vector<int> coedgeNodeIndices;
        std::vector<int> coedgeNextNodeIndices;
        std::vector<int> coedgePreviousNodeIndices;
        std::vector<int> coedgePartnerNodeIndices;
        std::vector<int> coedgeEdgeNodeIndices;
        std::vector<int> coedgeEdgeStartVertexNodeIndices;
        std::vector<int> coedgeEdgeEndVertexNodeIndices;
        std::vector<int> coedgeOrientedStartVertexNodeIndices;
        std::vector<int> coedgeOrientedEndVertexNodeIndices;
        std::vector<int> coedgeLoopNodeIndices;
        std::vector<int> coedgePCurveNodeIndices;
        std::vector<std::int8_t> coedgeSenseFlags;
        bool hasCoedgeEndpointChainObservation = false;
        bool coedgeEndpointChainContinuous = false;
        std::size_t coedgeEndpointDiscontinuityCount = 0u;
        int firstCoedgeEndpointDiscontinuityNodeIndex = -1;
        bool hasCoedgeUseIndexObservation = false;
        bool coedgeUseIndexCoversParsedCoedges = false;
        std::size_t coedgeUseIndexUnclassifiedCoedgeCount = 0u;
        int firstUnclassifiedCoedgeOwnerNodeIndex = -1;
        bool coedgeChainMatchesUseIndex = false;
        std::vector<int> coedgeUseNodeIndices;
        int nextLoopIndex = -1;
        int nextLoopTargetNodeIndex = -1;
        bool nextLoopTargetClassified = false;
        bool nextLoopIsNull = false;
        bool hasCoedgeRingClosure = false;
        bool coedgeRingClosed = false;
        bool coedgeRingOpen = false;
        bool coedgeLinksReciprocal = false;
    };
    std::map<int, CachedLoopProjection> loopProjections;

    // A surface may be shared by several faces. Its scalar parameter projection
    // is invariant for this immutable model, so parse its token sequence once
    // and copy the result into each face. Cache nodes are charged before parse
    // or insertion; face rows remain separately charged and ordered.
    std::map<int, DRW_AcisFace> surfaceProjections;
    const auto projectSurface = [&](int surfaceIndex,
                                    const DRW_AcisNode& surface,
                                    DRW_AcisFace& face) -> bool {
        auto cached = surfaceProjections.find(surfaceIndex);
        if (cached == surfaceProjections.end()) {
            budget.accountOutputItems();
            DRW_AcisFace projected;
            projected.surfaceType = surfaceKind(surface.type);
            if (!surfaceParams(model.header, surface, projected, budget))
                return false;
            cached = surfaceProjections.emplace(
                surfaceIndex, std::move(projected)).first;
        }

        const DRW_AcisFace& projected = cached->second;
        face.surfaceType = projected.surfaceType;
        face.hasSurface = projected.hasSurface;
        face.p0 = projected.p0;
        face.p1 = projected.p1;
        face.p2 = projected.p2;
        face.hasUDir = projected.hasUDir;
        face.ratio = projected.ratio;
        face.sineAngle = projected.sineAngle;
        face.cosineAngle = projected.cosineAngle;
        face.majorRadius = projected.majorRadius;
        face.minorRadius = projected.minorRadius;
        return true;
    };

    // Scalar straight/ellipse curve parameters are immutable and can also be
    // shared by edges. Cache their token scan, while leaving vector-valued
    // intcurve output separately owned by each edge.
    std::map<int, DRW_AcisEdge> scalarCurveProjections;
    std::map<int, std::vector<DRW_Coord>> intcurveControlPointProjections;
    const auto projectCurve = [&](int curveIndex,
                                  const DRW_AcisNode& curve,
                                  DRW_AcisEdge& edge) -> bool {
        if (curve.type == "intcurve-curve") {
            auto cached = intcurveControlPointProjections.find(curveIndex);
            if (cached == intcurveControlPointProjections.end()) {
                budget.accountOutputItems();
                std::vector<DRW_Coord> projected = intcurveControlPoints(
                    curve.record.tokens,
                    semanticTokenStart(model.header, curve.record), budget);
                for (const DRW_Coord& point : projected) {
                    budget.accountExtractionWork();
                    if (!isFinite(point)) {
                        if (invalidCurveNodeIndex)
                            *invalidCurveNodeIndex = curveIndex;
                        return false;
                    }
                }
                cached = intcurveControlPointProjections.emplace(
                    curveIndex, std::move(projected)).first;
            }

            const std::vector<DRW_Coord>& controlPoints = cached->second;
            budget.accountExtractionWork(controlPoints.size());
            budget.accountOutputItems(controlPoints.size());
            edge.curveType = DRW_AcisCurve::Intcurve;
            edge.hasCurve = true;
            edge.controlPoints = controlPoints;
            return true;
        }

        if (curve.type != "straight-curve"
            && curve.type != "ellipse-curve") {
            edge.curveType = curveKind(curve.type);
            return curveParams(model.header, curve, edge, budget);
        }

        auto cached = scalarCurveProjections.find(curveIndex);
        if (cached == scalarCurveProjections.end()) {
            budget.accountOutputItems();
            DRW_AcisEdge projected;
            projected.curveType = curveKind(curve.type);
            if (!curveParams(model.header, curve, projected, budget)) {
                if (invalidCurveNodeIndex)
                    *invalidCurveNodeIndex = curveIndex;
                return false;
            }
            cached = scalarCurveProjections.emplace(
                curveIndex, std::move(projected)).first;
        }

        const DRW_AcisEdge& projected = cached->second;
        edge.curveType = projected.curveType;
        edge.hasCurve = projected.hasCurve;
        edge.p0 = projected.p0;
        edge.p1 = projected.p1;
        edge.p2 = projected.p2;
        edge.ratio = projected.ratio;
        return true;
    };

    struct CachedVertexPoint {
        DRW_Coord point;
        int pointNodeIndex = -1;
        bool resolved = false;
    };
    std::map<int, CachedVertexPoint> vertexPoints;
    const auto resolveVertexPoint = [&](int vertexIndex, DRW_Coord& point,
                                        int& pointNodeIndex) {
        const auto cached = vertexPoints.find(vertexIndex);
        if (cached != vertexPoints.end()) {
            if (cached->second.resolved)
                point = cached->second.point;
            pointNodeIndex = cached->second.pointNodeIndex;
            return cached->second.resolved;
        }

        budget.accountOutputItems();
        CachedVertexPoint value;
        value.resolved = vertexPoint(model, vertexIndex, point, budget,
                                     invalidPointNodeIndex,
                                     &value.pointNodeIndex,
                                     selectedNodes
                                         && isStrictAcis21800(model.header));
        if (value.resolved)
            value.point = point;
        pointNodeIndex = value.pointNodeIndex;
        vertexPoints.emplace(vertexIndex, value);
        return value.resolved;
    };

    std::vector<std::string> vertexDiagnostics;
    std::vector<std::string> edgeDiagnostics;
    std::vector<std::string> faceDiagnostics;
    const auto projectVertex = [&](int vertexIndex,
                                   const DRW_AcisNode* vertexNode) -> bool {
        DRW_AcisVertex vertex;
        vertex.nodeIndex = vertexIndex;
        DRW_Coord point;
        if (vertexNode
            && resolveVertexPoint(vertexIndex, point,
                                  vertex.pointNodeIndex)) {
            vertex.point = point;
            vertex.valid = true;
            grow(point);
        } else {
            if (invalidPointNodeIndex && *invalidPointNodeIndex >= 0)
                return false;
            appendDiagnostic(vertexDiagnostics, budget, "vertex#",
                             vertexIndex, ": no resolvable point");
        }
        budget.accountOutputItems();
        out.vertices.push_back(vertex);
        return true;
    };

    const auto projectEdge = [&](int edgeIndex,
                                 const DRW_AcisNode* edgeNode) -> bool {
        DRW_AcisEdge edge;
        edge.nodeIndex = edgeIndex;
        if (edgeNode) {
            if (isStrictAcis21800(model.header)) {
                edge.coedgeNodeIndex = pointerRefOfType(
                    model, *edgeNode, 3u, "coedge", budget);
                const DRW_AcisNode* coedgeNode =
                    nodeAt(model, edge.coedgeNodeIndex);
                edge.hasReciprocalCoedgeLink = coedgeNode
                    && pointerRefOfType(model, *coedgeNode, 4u, "edge",
                                        budget) == edgeIndex;
                projectCoedgePartnerRing(
                    model, edge.coedgeNodeIndex, budget,
                    edge.coedgePartnerRingNodeIndices,
                    edge.coedgePartnerRingNextNodeIndices,
                    edge.hasCoedgePartnerRingObservation,
                    edge.coedgePartnerRingClosed);
                edge.hasCoedgeEdgeUseIndexObservation = true;
                edge.coedgeEdgeUseIndexCoversParsedCoedges =
                    parsedAcisUseIndexes.coversParsedCoedges();
                edge.coedgeEdgeUseIndexUnclassifiedCoedgeCount =
                    parsedAcisUseIndexes.unclassifiedCoedgeCount;
                edge.firstUnclassifiedCoedgeEdgeUseNodeIndex =
                    parsedAcisUseIndexes.firstUnclassifiedCoedgeNodeIndex;
                const auto edgeUses =
                    parsedAcisUseIndexes.coedgeNodeIndicesByEdge.find(
                        edgeIndex);
                if (edgeUses
                    != parsedAcisUseIndexes.coedgeNodeIndicesByEdge.end()) {
                    budget.accountOutputItems(edgeUses->second.size());
                    edge.coedgeEdgeUseNodeIndices = edgeUses->second;
                }
                edge.coedgePartnerRingMatchesEdgeUseIndex =
                    coedgePartnerRingMatchesEdgeUseIndex(
                        edge.coedgeNodeIndex,
                        edge.coedgePartnerRingNodeIndices,
                        edge.coedgePartnerRingNextNodeIndices,
                        edge.coedgePartnerRingClosed,
                        edge.coedgeEdgeUseNodeIndices,
                        edge.coedgeEdgeUseIndexCoversParsedCoedges, budget);
            }
            std::size_t endpointCount = 0u;
            const bool exactRootedEdgeSlots = selectedNodes
                && isStrictAcis21800(model.header);
            int curveIndex = exactRootedEdgeSlots
                ? pointerRefWithSuffix(model, *edgeNode, 4u, "-curve", budget)
                : -1;
            const auto projectEndpoint = [&](int vertexIndex,
                                             bool startEndpoint) -> bool {
                if (vertexIndex < 0)
                    return true;
                DRW_Coord point;
                int pointNodeIndex = -1;
                const bool valid = resolveVertexPoint(
                    vertexIndex, point, pointNodeIndex);
                if (!valid && invalidPointNodeIndex
                    && *invalidPointNodeIndex >= 0)
                    return false;
                if (startEndpoint) {
                    edge.startVertexNodeIndex = vertexIndex;
                    if (valid) {
                        edge.start = point;
                        edge.hasStart = true;
                    }
                } else {
                    edge.endVertexNodeIndex = vertexIndex;
                    if (valid) {
                        edge.end = point;
                        edge.hasEnd = true;
                    }
                }
                return true;
            };
            if (exactRootedEdgeSlots) {
                const int startVertex = pointerRefOfType(
                    model, *edgeNode, 1u, "vertex", budget);
                const int endVertex = pointerRefOfType(
                    model, *edgeNode, 2u, "vertex", budget);
                if (!projectEndpoint(startVertex, true)
                    || !projectEndpoint(endVertex, false))
                    return false;
            } else {
                for (int ref : edgeNode->refs) {
                    budget.accountExtractionWork();
                    const DRW_AcisNode* referencedNode = nodeAt(model, ref);
                    if (!referencedNode)
                        continue;

                    if (endpointCount < 2u
                        && referencedNode->type == "vertex") {
                        if (endpointCount == 0u) {
                            if (!projectEndpoint(ref, true))
                                return false;
                        } else if (!projectEndpoint(ref, false)) {
                            return false;
                        }
                        ++endpointCount;
                    }

                    if (curveIndex < 0
                        && endsWith(referencedNode->type, "-curve"))
                        curveIndex = ref;

                    // Preserve the legacy flexible first-typed-reference
                    // behavior while avoiding a second scan after both
                    // endpoints and the first curve have been found.
                    if (endpointCount == 2u && curveIndex >= 0)
                        break;
                }
            }
            edge.curveNodeIndex = curveIndex;
            const DRW_AcisNode* curveNode = nodeAt(model, curveIndex);
            if (curveNode)
                if (!projectCurve(curveIndex, *curveNode, edge))
                    return false;
        }
        if (edge.hasStart && edge.hasEnd
            && edge.start.x == edge.end.x && edge.start.y == edge.end.y
            && edge.start.z == edge.end.z) {
            appendDiagnostic(edgeDiagnostics, budget, "edge#", edgeIndex,
                             ": degenerate (coincident endpoints)");
        }
        budget.accountOutputItems();
        out.edges.push_back(std::move(edge));
        return true;
    };

    const auto projectFace = [&](int faceIndex,
                                 const DRW_AcisNode* faceNode) -> bool {
        DRW_AcisFace face;
        face.nodeIndex = faceIndex;
        if (faceNode) {
            face.shellNodeIndex = isStrictAcis21800(model.header)
                ? pointerRefOfType(model, *faceNode, 3u, "shell", budget)
                : refOfType(model, *faceNode, "shell", budget);
            if (model.header.signature == "ACIS BinaryFile"
                && model.header.version == 21800) {
                const AcisBooleanFields flags = booleanFieldsAfterPointerCount(
                    model.header, *faceNode, 6u, budget);
                if (flags.count >= 1u) {
                    face.hasSense = true;
                    face.senseReversed = flags.values[0] != 0;
                }
                if (flags.count >= 2u) {
                    face.hasDoubleSided = true;
                    face.doubleSided = flags.values[1] != 0;
                }
                if (face.doubleSided && flags.count >= 3u) {
                    face.hasContainment = true;
                    face.containmentIn = flags.values[2] != 0;
                }
            }
            const bool strict21800 = isStrictAcis21800(model.header);
            face.hasLoopChainObservation = strict21800;
            int surfaceIndex = strict21800
                ? pointerRefWithSuffix(model, *faceNode, 5u, "-surface", budget)
                : -1;
            int loopIndex = strict21800
                ? pointerRefOfType(model, *faceNode, 2u, "loop", budget)
                : -1;
            for (int ref : faceNode->refs) {
                budget.accountExtractionWork();
                const DRW_AcisNode* referencedNode = nodeAt(model, ref);
                if (!referencedNode)
                    continue;

                if (!strict21800 && surfaceIndex < 0
                    && (referencedNode->type == "-surface"
                        || endsWith(referencedNode->type, "-surface")))
                    surfaceIndex = ref;
                if (!strict21800 && loopIndex < 0
                    && (referencedNode->type == "loop"
                        || endsWith(referencedNode->type, "loop")))
                    loopIndex = ref;

                // Preserve the first source-order match for both links while
                // avoiding the former second scan of this same ref list.
                if (surfaceIndex >= 0 && loopIndex >= 0)
                    break;
            }
            face.surfaceNodeIndex = surfaceIndex;
            const DRW_AcisNode* surfaceNode = nodeAt(model, surfaceIndex);
            if (surfaceNode
                && !projectSurface(surfaceIndex, *surfaceNode, face)) {
                if (invalidSurfaceNodeIndex)
                    *invalidSurfaceNodeIndex = surfaceIndex;
                return false;
            }

            // Walk the next-loop chain (loop-typed ref), cycle-guarded.
            std::set<int> seen;
            while (loopIndex >= 0) {
                budget.accountExtractionWork();
                if (seen.find(loopIndex) != seen.end()) {
                    face.loopChainCycleDetected = true;
                    face.firstRepeatedLoopChainNodeIndex = loopIndex;
                    break;
                }
                budget.accountOutputItems();
                seen.insert(loopIndex);
                const DRW_AcisNode* loop = nodeAt(model, loopIndex);
                if (!loop)
                    break;
                DRW_AcisFaceLoop faceLoop;
                faceLoop.nodeIndex = loopIndex;
                auto cachedLoop = loopProjections.find(loopIndex);
                if (cachedLoop == loopProjections.end()) {
                    budget.accountOutputItems();
                    CachedLoopProjection projected;
                    projected.coedgeNodeIndices = loopCoedgeNodeIndices(
                        model, *loop, budget,
                        projected.hasCoedgeRingClosure,
                        projected.coedgeRingClosed);
                    projected.coedgePreviousNodeIndices =
                        loopCoedgePreviousNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgeNextNodeIndices =
                        loopCoedgeNextNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgePartnerNodeIndices =
                        loopCoedgePartnerNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgeRingOpen =
                        projected.hasCoedgeRingClosure
                        && !projected.coedgeRingClosed
                        && coedgeChainHasOpenEnds(
                            model,
                            projected.coedgeNodeIndices,
                            projected.coedgeNextNodeIndices,
                            projected.coedgePreviousNodeIndices, budget);
                    projected.coedgeLinksReciprocal =
                        projected.hasCoedgeRingClosure
                        && coedgeRingHasReciprocalLinks(
                            projected.coedgeNodeIndices,
                            projected.coedgeNextNodeIndices,
                            projected.coedgePreviousNodeIndices,
                            projected.coedgeRingClosed,
                            projected.coedgeRingOpen, budget);
                    projected.coedgeEdgeNodeIndices =
                        loopCoedgeEdgeNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgeEdgeStartVertexNodeIndices =
                        loopEdgeEndpointVertexNodeIndices(
                            model, projected.coedgeEdgeNodeIndices, 1u, budget);
                    projected.coedgeEdgeEndVertexNodeIndices =
                        loopEdgeEndpointVertexNodeIndices(
                            model, projected.coedgeEdgeNodeIndices, 2u, budget);
                    projected.coedgeLoopNodeIndices =
                        loopCoedgeLoopNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgePCurveNodeIndices =
                        loopCoedgePCurveNodeIndices(
                            model, projected.coedgeNodeIndices, budget);
                    projected.coedgeSenseFlags = loopCoedgeSenseFlags(
                        model, projected.coedgeNodeIndices, budget);
                    const CoedgeOrientedEndpointNodeIndices orientedEndpoints =
                        loopCoedgeOrientedEndpointNodeIndices(
                            projected.coedgeEdgeStartVertexNodeIndices,
                            projected.coedgeEdgeEndVertexNodeIndices,
                            projected.coedgeSenseFlags, budget);
                    projected.coedgeOrientedStartVertexNodeIndices =
                        orientedEndpoints.start;
                    projected.coedgeOrientedEndVertexNodeIndices =
                        orientedEndpoints.end;
                    const CoedgeEndpointChainObservation endpointChain =
                        observeCoedgeEndpointChain(
                            projected.coedgeNodeIndices,
                            projected.coedgeOrientedStartVertexNodeIndices,
                            projected.coedgeOrientedEndVertexNodeIndices,
                            projected.coedgeRingClosed,
                            projected.coedgeRingOpen,
                            projected.coedgeLinksReciprocal, budget);
                    projected.hasCoedgeEndpointChainObservation =
                        endpointChain.hasObservation;
                    projected.coedgeEndpointChainContinuous =
                        endpointChain.continuous;
                    projected.coedgeEndpointDiscontinuityCount =
                        endpointChain.discontinuityCount;
                    projected.firstCoedgeEndpointDiscontinuityNodeIndex =
                        endpointChain.firstDiscontinuityNodeIndex;
                    if (isStrictAcis21800(model.header)) {
                        projected.hasCoedgeUseIndexObservation = true;
                        projected.coedgeUseIndexCoversParsedCoedges =
                            parsedAcisUseIndexes.coversParsedCoedgeOwners();
                        projected.coedgeUseIndexUnclassifiedCoedgeCount =
                            parsedAcisUseIndexes.unclassifiedCoedgeOwnerCount;
                        projected.firstUnclassifiedCoedgeOwnerNodeIndex =
                            parsedAcisUseIndexes
                                .firstUnclassifiedCoedgeOwnerNodeIndex;
                        const auto coedgeUses =
                            parsedAcisUseIndexes.coedgeNodeIndicesByLoop.find(
                                loopIndex);
                        if (coedgeUses
                            != parsedAcisUseIndexes.coedgeNodeIndicesByLoop
                                   .end()) {
                            budget.accountOutputItems(
                                coedgeUses->second.size());
                            projected.coedgeUseNodeIndices =
                                coedgeUses->second;
                        }
                        projected.coedgeChainMatchesUseIndex =
                            loopCoedgeChainMatchesUseIndex(
                                loopIndex,
                                projected.coedgeNodeIndices,
                                projected.coedgeNextNodeIndices,
                                projected.coedgeLoopNodeIndices,
                                projected.hasCoedgeRingClosure,
                                projected.coedgeRingClosed,
                                projected.coedgeRingOpen,
                                projected.coedgeLinksReciprocal,
                                projected.coedgeUseNodeIndices,
                                projected
                                    .coedgeUseIndexCoversParsedCoedges,
                                budget);
                        budget.accountExtractionWork();
                        if (loop->refs.size() > 1u) {
                            projected.nextLoopTargetNodeIndex = loop->refs[1u];
                            if (projected.nextLoopTargetNodeIndex < 0) {
                                projected.nextLoopTargetClassified = true;
                                projected.nextLoopIsNull = true;
                            } else {
                                const DRW_AcisNode* nextLoop = nodeAt(
                                    model,
                                    projected.nextLoopTargetNodeIndex);
                                if (nextLoop && nextLoop->type == "loop") {
                                    projected.nextLoopTargetClassified = true;
                                    projected.nextLoopIndex =
                                        projected.nextLoopTargetNodeIndex;
                                }
                            }
                        }
                    } else {
                        projected.nextLoopIndex = refOfType(
                            model, *loop, "loop", budget);
                    }
                    cachedLoop = loopProjections.emplace(
                        loopIndex, std::move(projected)).first;
                }
                if (cachedLoop->second.coedgeNodeIndices.size()
                    > static_cast<std::size_t>(
                        std::numeric_limits<int>::max()))
                    throw SabError(true);
                faceLoop.coedgeCount = static_cast<int>(
                    cachedLoop->second.coedgeNodeIndices.size());
                faceLoop.hasCoedgeRingClosure =
                    cachedLoop->second.hasCoedgeRingClosure;
                faceLoop.coedgeRingClosed =
                    cachedLoop->second.coedgeRingClosed;
                faceLoop.coedgeRingOpen =
                    cachedLoop->second.coedgeRingOpen;
                faceLoop.coedgeLinksReciprocal =
                    cachedLoop->second.coedgeLinksReciprocal;
                if (strict21800) {
                    budget.accountOutputItems();
                    face.loopNextNodeIndices.push_back(
                        cachedLoop->second.nextLoopTargetNodeIndex);
                    if (!cachedLoop->second.nextLoopTargetClassified) {
                        ++face.loopChainUnclassifiedNextLinkCount;
                        if (face.firstUnclassifiedLoopNextNodeIndex < 0)
                            face.firstUnclassifiedLoopNextNodeIndex =
                                loopIndex;
                    } else if (cachedLoop->second.nextLoopIsNull) {
                        face.loopChainNullTerminated = true;
                    }
                }
                budget.accountOutputItems(
                    cachedLoop->second.coedgeNodeIndices.size());
                faceLoop.coedgeNodeIndices =
                    cachedLoop->second.coedgeNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeNextNodeIndices.size());
                faceLoop.coedgeNextNodeIndices =
                    cachedLoop->second.coedgeNextNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgePreviousNodeIndices.size());
                faceLoop.coedgePreviousNodeIndices =
                    cachedLoop->second.coedgePreviousNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgePartnerNodeIndices.size());
                faceLoop.coedgePartnerNodeIndices =
                    cachedLoop->second.coedgePartnerNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeEdgeNodeIndices.size());
                faceLoop.coedgeEdgeNodeIndices =
                    cachedLoop->second.coedgeEdgeNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeEdgeStartVertexNodeIndices.size());
                faceLoop.coedgeEdgeStartVertexNodeIndices =
                    cachedLoop->second.coedgeEdgeStartVertexNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeEdgeEndVertexNodeIndices.size());
                faceLoop.coedgeEdgeEndVertexNodeIndices =
                    cachedLoop->second.coedgeEdgeEndVertexNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second
                        .coedgeOrientedStartVertexNodeIndices.size());
                faceLoop.coedgeOrientedStartVertexNodeIndices =
                    cachedLoop->second
                        .coedgeOrientedStartVertexNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second
                        .coedgeOrientedEndVertexNodeIndices.size());
                faceLoop.coedgeOrientedEndVertexNodeIndices =
                    cachedLoop->second
                        .coedgeOrientedEndVertexNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeLoopNodeIndices.size());
                faceLoop.coedgeLoopNodeIndices =
                    cachedLoop->second.coedgeLoopNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgePCurveNodeIndices.size());
                faceLoop.coedgePCurveNodeIndices =
                    cachedLoop->second.coedgePCurveNodeIndices;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeSenseFlags.size());
                faceLoop.coedgeSenseFlags =
                    cachedLoop->second.coedgeSenseFlags;
                faceLoop.hasCoedgeEndpointChainObservation =
                    cachedLoop->second
                        .hasCoedgeEndpointChainObservation;
                faceLoop.coedgeEndpointChainContinuous =
                    cachedLoop->second.coedgeEndpointChainContinuous;
                faceLoop.coedgeEndpointDiscontinuityCount =
                    cachedLoop->second.coedgeEndpointDiscontinuityCount;
                faceLoop.firstCoedgeEndpointDiscontinuityNodeIndex =
                    cachedLoop->second
                        .firstCoedgeEndpointDiscontinuityNodeIndex;
                faceLoop.hasCoedgeUseIndexObservation =
                    cachedLoop->second.hasCoedgeUseIndexObservation;
                faceLoop.coedgeUseIndexCoversParsedCoedges =
                    cachedLoop->second.coedgeUseIndexCoversParsedCoedges;
                faceLoop.coedgeUseIndexUnclassifiedCoedgeCount =
                    cachedLoop->second
                        .coedgeUseIndexUnclassifiedCoedgeCount;
                faceLoop.firstUnclassifiedCoedgeOwnerNodeIndex =
                    cachedLoop->second
                        .firstUnclassifiedCoedgeOwnerNodeIndex;
                faceLoop.coedgeChainMatchesUseIndex =
                    cachedLoop->second.coedgeChainMatchesUseIndex;
                budget.accountOutputItems(
                    cachedLoop->second.coedgeUseNodeIndices.size());
                faceLoop.coedgeUseNodeIndices =
                    cachedLoop->second.coedgeUseNodeIndices;
                budget.accountOutputItems();
                face.loops.push_back(faceLoop);
                loopIndex = cachedLoop->second.nextLoopIndex;
            }
            if (strict21800) {
                face.hasLoopUseIndexObservation = true;
                face.loopUseIndexCoversParsedLoops =
                    parsedAcisUseIndexes.coversParsedLoops();
                face.loopUseIndexUnclassifiedLoopCount =
                    parsedAcisUseIndexes.unclassifiedLoopCount;
                face.firstUnclassifiedLoopOwnerNodeIndex =
                    parsedAcisUseIndexes.firstUnclassifiedLoopNodeIndex;
                const auto loopUses =
                    parsedAcisUseIndexes.loopNodeIndicesByFace.find(
                        faceIndex);
                if (loopUses
                    != parsedAcisUseIndexes.loopNodeIndicesByFace.end()) {
                    budget.accountOutputItems(loopUses->second.size());
                    face.loopUseNodeIndices = loopUses->second;
                }
                face.faceLoopChainMatchesUseIndex =
                    faceLoopChainMatchesUseIndex(
                        face.loops, face.loopUseNodeIndices,
                        face.loopNextNodeIndices,
                        face.loopChainNullTerminated,
                        face.loopChainCycleDetected,
                        face.loopChainUnclassifiedNextLinkCount,
                        face.loopUseIndexCoversParsedLoops, budget);
            }
            if (!surfaceNode) {
                appendDiagnostic(faceDiagnostics, budget, "face#",
                                 faceIndex, ": no surface");
            }
        }
        budget.accountOutputItems();
        out.faces.push_back(std::move(face));
        return true;
    };

    // Classify and project every candidate in one pass. Category vectors
    // retain source order; diagnostic buffers preserve the established
    // vertex/edge/face ordering independently of mixed source ordering.
    for (std::size_t i = 0; i < candidateCount; ++i) {
        budget.accountExtractionWork();
        int nodeIndex = -1;
        if (!nodeIndexAt(i, nodeIndex))
            return false;

        const DRW_AcisNode& candidate =
            model.nodes[static_cast<std::size_t>(nodeIndex)];
        if (candidate.type == "vertex") {
            if (!projectVertex(nodeIndex, nodeAt(model, nodeIndex)))
                return false;
        }
        else if (candidate.type == "edge"
                 && !projectEdge(nodeIndex, nodeAt(model, nodeIndex)))
            return false;
        else if (candidate.type == "face"
                 && !projectFace(nodeIndex, nodeAt(model, nodeIndex)))
            return false;
    }

    // Preserve each rooted lump's first-shell/next-shell chain and compare it
    // with the source-ordered parsed-model index of exact shell->lump links.
    // This is structural incidence only; it does not validate a lump or B-rep.
    if (selectedNodes && isStrictAcis21800(model.header)) {
        for (int lumpIndex : projectedLumpIndices) {
            budget.accountExtractionWork();
            const DRW_AcisNode* lumpNode = nodeAt(model, lumpIndex);
            if (!lumpNode)
                return false;

            budget.accountOutputItems();
            DRW_AcisLump lump;
            lump.nodeIndex = lumpIndex;
            lump.bodyNodeIndex = pointerRefOfType(
                model, *lumpNode, 3u, "body", budget);
            lump.hasShellChainObservation = true;
            lump.hasShellUseIndexObservation = true;
            lump.shellUseIndexCoversParsedShells =
                parsedAcisUseIndexes.coversParsedShells();
            lump.shellUseIndexUnclassifiedShellCount =
                parsedAcisUseIndexes.unclassifiedShellCount;
            lump.firstUnclassifiedShellOwnerNodeIndex =
                parsedAcisUseIndexes.firstUnclassifiedShellNodeIndex;

            const auto shellUses =
                parsedAcisUseIndexes.shellNodeIndicesByLump.find(lumpIndex);
            if (shellUses
                != parsedAcisUseIndexes.shellNodeIndicesByLump.end()) {
                budget.accountOutputItems(shellUses->second.size());
                lump.shellUseNodeIndices = shellUses->second;
            }

            int shellIndex = -1;
            if (lumpNode->refs.size() <= 2u) {
                ++lump.shellChainUnclassifiedLinkCount;
                lump.firstUnclassifiedShellLinkSourceNodeIndex = lumpIndex;
            } else {
                const int firstShellTarget = lumpNode->refs[2u];
                if (firstShellTarget < 0) {
                    lump.shellChainNullTerminated = true;
                } else {
                    const DRW_AcisNode* firstShell =
                        nodeAt(model, firstShellTarget);
                    if (firstShell && firstShell->type == "shell")
                        shellIndex = firstShellTarget;
                    else {
                        ++lump.shellChainUnclassifiedLinkCount;
                        lump.firstUnclassifiedShellLinkSourceNodeIndex =
                            lumpIndex;
                    }
                }
            }

            std::set<int> seenShells;
            while (shellIndex >= 0) {
                budget.accountExtractionWork();
                if (seenShells.find(shellIndex) != seenShells.end()) {
                    lump.shellChainCycleDetected = true;
                    lump.firstRepeatedShellChainNodeIndex = shellIndex;
                    break;
                }
                budget.accountOutputItems(3u);
                seenShells.insert(shellIndex);
                lump.shellNodeIndices.push_back(shellIndex);

                const DRW_AcisNode* shellNode = nodeAt(model, shellIndex);
                if (!shellNode || shellNode->type != "shell") {
                    ++lump.shellChainUnclassifiedLinkCount;
                    if (lump.firstUnclassifiedShellLinkSourceNodeIndex < 0)
                        lump.firstUnclassifiedShellLinkSourceNodeIndex =
                            shellIndex;
                    lump.shellNextNodeIndices.push_back(-1);
                    break;
                }
                if (shellNode->refs.size() <= 1u) {
                    ++lump.shellChainUnclassifiedLinkCount;
                    if (lump.firstUnclassifiedShellLinkSourceNodeIndex < 0)
                        lump.firstUnclassifiedShellLinkSourceNodeIndex =
                            shellIndex;
                    lump.shellNextNodeIndices.push_back(-1);
                    break;
                }

                const int nextShellTarget = shellNode->refs[1u];
                lump.shellNextNodeIndices.push_back(nextShellTarget);
                if (nextShellTarget < 0) {
                    lump.shellChainNullTerminated = true;
                    break;
                }
                const DRW_AcisNode* nextShell =
                    nodeAt(model, nextShellTarget);
                if (!nextShell || nextShell->type != "shell") {
                    ++lump.shellChainUnclassifiedLinkCount;
                    if (lump.firstUnclassifiedShellLinkSourceNodeIndex < 0)
                        lump.firstUnclassifiedShellLinkSourceNodeIndex =
                            shellIndex;
                    break;
                }
                shellIndex = nextShellTarget;
            }

            lump.shellChainMatchesUseIndex =
                lumpShellChainMatchesUseIndex(
                    lump.shellNodeIndices, lump.shellNextNodeIndices,
                    lump.shellUseNodeIndices,
                    lump.shellChainNullTerminated,
                    lump.shellChainCycleDetected,
                    lump.shellChainUnclassifiedLinkCount,
                    lump.shellUseIndexCoversParsedShells, budget);
            out.lumps.push_back(std::move(lump));
        }
    }

    // Preserve each rooted shell's observed first-face/next-face chain and
    // compare it with a single parsed-model index of exact face->shell links.
    // This is structural incidence only; a matching list is not shell or B-rep
    // validation. The generic/unrooted wireframe API leaves this extension
    // empty because it has no selected body graph to qualify.
    if (selectedNodes && isStrictAcis21800(model.header)) {
        for (int shellIndex : projectedShellIndices) {
            budget.accountExtractionWork();
            const DRW_AcisNode* shellNode = nodeAt(model, shellIndex);
            if (!shellNode)
                return false;

            budget.accountOutputItems();
            DRW_AcisShell shell;
            shell.nodeIndex = shellIndex;
            shell.lumpNodeIndex = pointerRefOfType(
                model, *shellNode, 5u, "lump", budget);
            shell.hasFaceChainObservation = true;
            shell.hasFaceUseIndexObservation = true;
            shell.faceUseIndexCoversParsedFaces =
                parsedAcisUseIndexes.coversParsedFaces();
            shell.faceUseIndexUnclassifiedFaceCount =
                parsedAcisUseIndexes.unclassifiedFaceCount;
            shell.firstUnclassifiedFaceOwnerNodeIndex =
                parsedAcisUseIndexes.firstUnclassifiedFaceNodeIndex;

            const auto faceUses =
                parsedAcisUseIndexes.faceNodeIndicesByShell.find(shellIndex);
            if (faceUses
                != parsedAcisUseIndexes.faceNodeIndicesByShell.end()) {
                budget.accountOutputItems(faceUses->second.size());
                shell.faceUseNodeIndices = faceUses->second;
            }

            int faceIndex = -1;
            if (shellNode->refs.size() <= 3u) {
                ++shell.faceChainUnclassifiedLinkCount;
                shell.firstUnclassifiedFaceLinkSourceNodeIndex = shellIndex;
            } else {
                const int firstFaceTarget = shellNode->refs[3u];
                if (firstFaceTarget < 0) {
                    shell.faceChainNullTerminated = true;
                } else {
                    const DRW_AcisNode* firstFace =
                        nodeAt(model, firstFaceTarget);
                    if (firstFace && firstFace->type == "face")
                        faceIndex = firstFaceTarget;
                    else {
                        ++shell.faceChainUnclassifiedLinkCount;
                        shell.firstUnclassifiedFaceLinkSourceNodeIndex =
                            shellIndex;
                    }
                }
            }

            std::set<int> seenFaces;
            while (faceIndex >= 0) {
                budget.accountExtractionWork();
                if (seenFaces.find(faceIndex) != seenFaces.end()) {
                    shell.faceChainCycleDetected = true;
                    shell.firstRepeatedFaceChainNodeIndex = faceIndex;
                    break;
                }
                budget.accountOutputItems(3u);
                seenFaces.insert(faceIndex);
                shell.faceNodeIndices.push_back(faceIndex);

                const DRW_AcisNode* faceNode = nodeAt(model, faceIndex);
                if (!faceNode || faceNode->type != "face") {
                    ++shell.faceChainUnclassifiedLinkCount;
                    if (shell.firstUnclassifiedFaceLinkSourceNodeIndex < 0)
                        shell.firstUnclassifiedFaceLinkSourceNodeIndex =
                            faceIndex;
                    shell.faceNextNodeIndices.push_back(-1);
                    break;
                }
                if (faceNode->refs.size() <= 1u) {
                    ++shell.faceChainUnclassifiedLinkCount;
                    if (shell.firstUnclassifiedFaceLinkSourceNodeIndex < 0)
                        shell.firstUnclassifiedFaceLinkSourceNodeIndex =
                            faceIndex;
                    shell.faceNextNodeIndices.push_back(-1);
                    break;
                }

                const int nextFaceTarget = faceNode->refs[1u];
                shell.faceNextNodeIndices.push_back(nextFaceTarget);
                if (nextFaceTarget < 0) {
                    shell.faceChainNullTerminated = true;
                    break;
                }
                const DRW_AcisNode* nextFace =
                    nodeAt(model, nextFaceTarget);
                if (!nextFace || nextFace->type != "face") {
                    ++shell.faceChainUnclassifiedLinkCount;
                    if (shell.firstUnclassifiedFaceLinkSourceNodeIndex < 0)
                        shell.firstUnclassifiedFaceLinkSourceNodeIndex =
                            faceIndex;
                    break;
                }
                faceIndex = nextFaceTarget;
            }

            shell.faceChainMatchesUseIndex =
                shellFaceChainMatchesUseIndex(
                    shell.faceNodeIndices, shell.faceNextNodeIndices,
                    shell.faceUseNodeIndices,
                    shell.faceChainNullTerminated,
                    shell.faceChainCycleDetected,
                    shell.faceChainUnclassifiedLinkCount,
                    shell.faceUseIndexCoversParsedFaces, budget);
            out.shells.push_back(std::move(shell));
        }
    }

    const auto appendDiagnostics = [&](std::vector<std::string>& diagnostics) {
        for (std::string& diagnostic : diagnostics) {
            budget.accountExtractionWork();
            // The diagnostic has already been counted in its category-local
            // staging vector; this separately retained slot belongs to the
            // public result vector.
            budget.accountOutputItems();
            out.diagnostics.push_back(std::move(diagnostic));
        }
    };
    appendDiagnostics(vertexDiagnostics);
    appendDiagnostics(edgeDiagnostics);
    appendDiagnostics(faceDiagnostics);

    if (grown) {
        out.hasBBox = true;
        out.bboxMin = DRW_Coord(minX, minY, minZ);
        out.bboxMax = DRW_Coord(maxX, maxY, maxZ);
    }
    return true;
}

bool drw_extractAcisWireframe(const DRW_AcisModel& model, DRW_AcisBrep& out) {
    const DRW_SabParseLimits limits;
    SabParseBudget budget(limits);
    try {
        DRW_AcisBrep extracted;
        if (!extractAcisWireframeImpl(model, extracted, budget)
            || !isFiniteProjection(extracted, budget)) {
            out = DRW_AcisBrep();
            return false;
        }
        out = std::move(extracted);
        return true;
    } catch (...) {
        out = DRW_AcisBrep();
        return false;
    }
}

namespace {

bool acisStraightEdgeParameterEndpoints(
    const DRW_AcisEdge& edge, double endpointCurveTolerance,
    double& startParameter, double& endParameter) noexcept {
    if (!std::isfinite(endpointCurveTolerance)
        || endpointCurveTolerance < 0.0 || !edge.hasStart || !edge.hasEnd
        || !isFinite(edge.start) || !isFinite(edge.end))
        return false;

    double candidateStartParameter = 0.0;
    double candidateEndParameter = 0.0;
    if (!drw_pointWithinToleranceOfAcisStraightCurve(
            edge, edge.start, endpointCurveTolerance,
            candidateStartParameter)
        || !drw_pointWithinToleranceOfAcisStraightCurve(
            edge, edge.end, endpointCurveTolerance,
            candidateEndParameter))
        return false;

    startParameter = candidateStartParameter;
    endParameter = candidateEndParameter;
    return true;
}

bool acisStraightEdgeFractionForParameter(
    double startParameter, double endParameter, double parameter,
    double& endpointFraction) noexcept {
    if (!std::isfinite(startParameter) || !std::isfinite(endParameter)
        || !std::isfinite(parameter))
        return false;
    if (startParameter == endParameter)
        return false;
    if (parameter == startParameter) {
        endpointFraction = 0.0;
        return true;
    }
    if (parameter == endParameter) {
        endpointFraction = 1.0;
        return true;
    }

    double candidateFraction = 0.0;
    const double parameterDelta = endParameter - startParameter;
    const double offset = parameter - startParameter;
    if (std::isfinite(parameterDelta) && std::isfinite(offset)) {
        if (parameterDelta == 0.0)
            return false;
        candidateFraction = offset / parameterDelta;
    } else {
        const double scale = std::max(
            std::fabs(startParameter),
            std::max(std::fabs(endParameter), std::fabs(parameter)));
        if (scale == 0.0)
            return false;
        const double scaledDelta = endParameter / scale
            - startParameter / scale;
        if (scaledDelta == 0.0)
            return false;
        const double scaledOffset = parameter / scale
            - startParameter / scale;
        candidateFraction = scaledOffset / scaledDelta;
    }

    if (!std::isfinite(candidateFraction) || candidateFraction < 0.0
        || candidateFraction > 1.0)
        return false;
    endpointFraction = candidateFraction;
    return true;
}

} // namespace

bool drw_orientAcisFaceNormal(const DRW_AcisFace& face,
                              const DRW_Coord& surfaceNormal,
                              DRW_Coord& faceNormal) noexcept {
    if (!face.hasSense || !isFinite(surfaceNormal))
        return false;

    const double scale = std::max(
        std::fabs(surfaceNormal.x),
        std::max(std::fabs(surfaceNormal.y), std::fabs(surfaceNormal.z)));
    if (scale == 0.0)
        return false;

    const DRW_Coord scaled(surfaceNormal.x / scale,
                           surfaceNormal.y / scale,
                           surfaceNormal.z / scale);
    const double length = std::hypot(std::hypot(scaled.x, scaled.y),
                                     scaled.z);
    if (!std::isfinite(length) || length == 0.0)
        return false;

    const double direction = face.senseReversed ? -1.0 : 1.0;
    const DRW_Coord candidate(direction * (scaled.x / length),
                              direction * (scaled.y / length),
                              direction * (scaled.z / length));
    if (!isFinite(candidate))
        return false;

    faceNormal = candidate;
    return true;
}

bool drw_evaluateAcisStraightCurve(const DRW_AcisEdge& edge,
                                   double parameter,
                                   DRW_Coord& point) noexcept {
    if (!edge.hasCurve || edge.curveType != DRW_AcisCurve::Straight
        || !std::isfinite(parameter) || !isFinite(edge.p0)
        || !isFinite(edge.p1)
        || (edge.p1.x == 0.0 && edge.p1.y == 0.0 && edge.p1.z == 0.0)) {
        return false;
    }

    const DRW_Coord evaluated(
        edge.p0.x + parameter * edge.p1.x,
        edge.p0.y + parameter * edge.p1.y,
        edge.p0.z + parameter * edge.p1.z);
    if (!isFinite(evaluated))
        return false;

    point = evaluated;
    return true;
}

bool drw_evaluateAcisEllipseCurve(const DRW_AcisEdge& edge,
                                  double parameter,
                                  double frameTolerance,
                                  DRW_Coord& point) noexcept {
    if (!edge.hasCurve || edge.curveType != DRW_AcisCurve::Ellipse
        || !std::isfinite(parameter) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || !isFinite(edge.p0)
        || !isFinite(edge.p1) || !isFinite(edge.p2)
        || !std::isfinite(edge.ratio) || edge.ratio <= 0.0)
        return false;

    const double normalScale = std::max(
        std::fabs(edge.p1.x),
        std::max(std::fabs(edge.p1.y), std::fabs(edge.p1.z)));
    const double majorScale = std::max(
        std::fabs(edge.p2.x),
        std::max(std::fabs(edge.p2.y), std::fabs(edge.p2.z)));
    if (normalScale == 0.0 || majorScale == 0.0)
        return false;

    const DRW_Coord scaledNormal(edge.p1.x / normalScale,
                                 edge.p1.y / normalScale,
                                 edge.p1.z / normalScale);
    const double normalLength = std::sqrt(
        scaledNormal.x * scaledNormal.x
        + scaledNormal.y * scaledNormal.y
        + scaledNormal.z * scaledNormal.z);
    if (!std::isfinite(normalLength) || normalLength == 0.0)
        return false;
    const DRW_Coord unitNormal(scaledNormal.x / normalLength,
                               scaledNormal.y / normalLength,
                               scaledNormal.z / normalLength);

    const DRW_Coord scaledMajor(edge.p2.x / majorScale,
                                edge.p2.y / majorScale,
                                edge.p2.z / majorScale);
    const double majorLength = std::sqrt(
        scaledMajor.x * scaledMajor.x
        + scaledMajor.y * scaledMajor.y
        + scaledMajor.z * scaledMajor.z);
    if (!std::isfinite(majorLength) || majorLength == 0.0)
        return false;
    const double normalComponent = scaledMajor.x * unitNormal.x
        + scaledMajor.y * unitNormal.y + scaledMajor.z * unitNormal.z;
    if (!std::isfinite(normalComponent)
        || std::fabs(normalComponent) / majorLength > frameTolerance)
        return false;

    // Remove only a caller-tolerated frame discrepancy. This gives the
    // evaluated minor axis an exactly in-plane, right-handed direction.
    const DRW_Coord projectedMajor(
        scaledMajor.x - normalComponent * unitNormal.x,
        scaledMajor.y - normalComponent * unitNormal.y,
        scaledMajor.z - normalComponent * unitNormal.z);
    const double projectedMajorLength = std::sqrt(
        projectedMajor.x * projectedMajor.x
        + projectedMajor.y * projectedMajor.y
        + projectedMajor.z * projectedMajor.z);
    if (!std::isfinite(projectedMajorLength)
        || projectedMajorLength == 0.0)
        return false;
    // Keep the projected major vector and its right-handed cross basis in the
    // same scaled coordinates. Renormalizing either can introduce a tiny
    // frame mismatch that becomes a huge spurious offset for large ellipses.
    const DRW_Coord minorDirection(
        unitNormal.y * projectedMajor.z
            - unitNormal.z * projectedMajor.y,
        unitNormal.z * projectedMajor.x
            - unitNormal.x * projectedMajor.z,
        unitNormal.x * projectedMajor.y
            - unitNormal.y * projectedMajor.x);
    if (!isFinite(minorDirection)
        || (minorDirection.x == 0.0 && minorDirection.y == 0.0
            && minorDirection.z == 0.0))
        return false;

    const double cosine = std::cos(parameter);
    const double sine = std::sin(parameter);
    if (!std::isfinite(cosine) || !std::isfinite(sine))
        return false;

    const double center[3] = {edge.p0.x, edge.p0.y, edge.p0.z};
    const double majorDirection[3] = {
        projectedMajor.x, projectedMajor.y, projectedMajor.z};
    const double minorDirectionComponents[3] = {
        minorDirection.x, minorDirection.y, minorDirection.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t axis = 0u; axis < 3u; ++axis) {
        double majorAxisComponent = 0.0;
        double minorAxisComponent = 0.0;
        if (!finiteProductQuotient3(majorScale, majorDirection[axis], 1.0,
                                    1.0, 1.0, majorAxisComponent)
            || !finiteProductQuotient3(
                majorScale, edge.ratio,
                minorDirectionComponents[axis],
                1.0, 1.0, minorAxisComponent))
            return false;

        const double majorTerm = majorAxisComponent * cosine;
        const double minorTerm = minorAxisComponent * sine;
        if (!std::isfinite(majorTerm) || !std::isfinite(minorTerm))
            return false;
        const double scale = std::max(
            std::fabs(center[axis]),
            std::max(std::fabs(majorTerm), std::fabs(minorTerm)));
        if (scale == 0.0) {
            coordinates[axis] = 0.0;
            continue;
        }
        // Preserve a small center when the two large in-plane offsets cancel.
        const double normalized = majorTerm / scale + minorTerm / scale
            + center[axis] / scale;
        if (!finiteProductQuotient3(scale, normalized, 1.0, 1.0, 1.0,
                                    coordinates[axis]))
            return false;
    }

    const DRW_Coord evaluated(coordinates[0], coordinates[1], coordinates[2]);
    if (!isFinite(evaluated))
        return false;
    point = evaluated;
    return true;
}

namespace {

bool projectPointToAcisEllipseCurveImpl(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double frameTolerance, bool allowNonUnique,
    DRW_Coord& projectedPoint) noexcept {
    if (!edge.hasCurve || edge.curveType != DRW_AcisCurve::Ellipse
        || !isFinite(point) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || !isFinite(edge.p0)
        || !isFinite(edge.p1) || !isFinite(edge.p2)
        || !std::isfinite(edge.ratio) || edge.ratio <= 0.0)
        return false;

    const double normalScale = std::max(
        std::fabs(edge.p1.x),
        std::max(std::fabs(edge.p1.y), std::fabs(edge.p1.z)));
    const double majorScale = std::max(
        std::fabs(edge.p2.x),
        std::max(std::fabs(edge.p2.y), std::fabs(edge.p2.z)));
    if (normalScale == 0.0 || majorScale == 0.0)
        return false;

    const DRW_Coord scaledNormal(edge.p1.x / normalScale,
                                 edge.p1.y / normalScale,
                                 edge.p1.z / normalScale);
    const double normalLength = std::sqrt(
        scaledNormal.x * scaledNormal.x
        + scaledNormal.y * scaledNormal.y
        + scaledNormal.z * scaledNormal.z);
    if (!std::isfinite(normalLength) || normalLength == 0.0)
        return false;
    const DRW_Coord unitNormal(scaledNormal.x / normalLength,
                               scaledNormal.y / normalLength,
                               scaledNormal.z / normalLength);

    const DRW_Coord scaledMajor(edge.p2.x / majorScale,
                                edge.p2.y / majorScale,
                                edge.p2.z / majorScale);
    const double majorLength = std::sqrt(
        scaledMajor.x * scaledMajor.x
        + scaledMajor.y * scaledMajor.y
        + scaledMajor.z * scaledMajor.z);
    if (!std::isfinite(majorLength) || majorLength == 0.0)
        return false;
    const double normalComponent = scaledMajor.x * unitNormal.x
        + scaledMajor.y * unitNormal.y + scaledMajor.z * unitNormal.z;
    if (!std::isfinite(normalComponent)
        || std::fabs(normalComponent) / majorLength > frameTolerance)
        return false;

    const DRW_Coord projectedMajor(
        scaledMajor.x - normalComponent * unitNormal.x,
        scaledMajor.y - normalComponent * unitNormal.y,
        scaledMajor.z - normalComponent * unitNormal.z);
    const double projectedMajorLength = std::sqrt(
        projectedMajor.x * projectedMajor.x
        + projectedMajor.y * projectedMajor.y
        + projectedMajor.z * projectedMajor.z);
    if (!std::isfinite(projectedMajorLength)
        || projectedMajorLength == 0.0)
        return false;
    const DRW_Coord unitMajor(projectedMajor.x / projectedMajorLength,
                              projectedMajor.y / projectedMajorLength,
                              projectedMajor.z / projectedMajorLength);
    const DRW_Coord crossMinor(
        unitNormal.y * unitMajor.z - unitNormal.z * unitMajor.y,
        unitNormal.z * unitMajor.x - unitNormal.x * unitMajor.z,
        unitNormal.x * unitMajor.y - unitNormal.y * unitMajor.x);
    const double minorDirectionLength = std::sqrt(
        crossMinor.x * crossMinor.x + crossMinor.y * crossMinor.y
        + crossMinor.z * crossMinor.z);
    if (!std::isfinite(minorDirectionLength)
        || minorDirectionLength == 0.0)
        return false;
    const DRW_Coord unitMinor(crossMinor.x / minorDirectionLength,
                              crossMinor.y / minorDirectionLength,
                              crossMinor.z / minorDirectionLength);

    double majorRadius = 0.0;
    double minorRadius = 0.0;
    if (!finiteProductQuotient3(majorScale, projectedMajorLength, 1.0,
                                1.0, 1.0, majorRadius)
        || !finiteProductQuotient3(majorRadius, edge.ratio, 1.0,
                                   1.0, 1.0, minorRadius)
        || majorRadius <= 0.0 || minorRadius <= 0.0)
        return false;

    const DRW_Coord delta(point.x - edge.p0.x,
                          point.y - edge.p0.y,
                          point.z - edge.p0.z);
    if (!isFinite(delta))
        return false;
    const double deltaScale = std::max(
        std::fabs(delta.x), std::max(std::fabs(delta.y), std::fabs(delta.z)));
    double projectedMajorCoordinate = 0.0;
    double projectedMinorCoordinate = 0.0;
    if (deltaScale != 0.0) {
        const DRW_Coord scaledDelta(delta.x / deltaScale,
                                    delta.y / deltaScale,
                                    delta.z / deltaScale);
        const double majorDot = scaledDelta.x * unitMajor.x
            + scaledDelta.y * unitMajor.y + scaledDelta.z * unitMajor.z;
        const double minorDot = scaledDelta.x * unitMinor.x
            + scaledDelta.y * unitMinor.y + scaledDelta.z * unitMinor.z;
        if (!finiteProductQuotient3(deltaScale, majorDot, 1.0,
                                    1.0, 1.0, projectedMajorCoordinate)
            || !finiteProductQuotient3(deltaScale, minorDot, 1.0,
                                      1.0, 1.0, projectedMinorCoordinate))
            return false;
    }

    // The closest-point equation is symmetric in the two axes. Sort them so
    // the minor radius is always the smaller one, including unusual ratios
    // above one, while keeping the returned point in the original 3D frame.
    const bool swapAxes = minorRadius > majorRadius;
    const double a = swapAxes ? minorRadius : majorRadius;
    const double b = swapAxes ? majorRadius : minorRadius;
    const DRW_Coord& longAxis = swapAxes ? unitMinor : unitMajor;
    const DRW_Coord& shortAxis = swapAxes ? unitMajor : unitMinor;
    const double x = swapAxes ? projectedMinorCoordinate
                              : projectedMajorCoordinate;
    const double y = swapAxes ? projectedMajorCoordinate
                              : projectedMinorCoordinate;

    const double solveScale = std::max(
        a, std::max(b, std::max(std::fabs(x), std::fabs(y))));
    if (!std::isfinite(solveScale) || solveScale == 0.0)
        return false;
    const double scaledA = a / solveScale;
    const double scaledB = b / solveScale;
    const double scaledX = std::fabs(x) / solveScale;
    const double scaledY = std::fabs(y) / solveScale;
    if (scaledA == 0.0 || scaledB == 0.0)
        return false;
    const double aSquared = scaledA * scaledA;
    const double bSquared = scaledB * scaledB;
    if (aSquared == 0.0 || bSquared == 0.0)
        return false;

    const auto constraint = [=](double lambda) noexcept {
        const double xNumerator = scaledA * scaledX;
        const double yNumerator = scaledB * scaledY;
        const double xDenominator = aSquared + lambda;
        const double yDenominator = bSquared + lambda;
        if (xDenominator <= 0.0 || yDenominator <= 0.0)
            return 2.0;
        if ((xNumerator != 0.0 && xNumerator > xDenominator)
            || (yNumerator != 0.0 && yNumerator > yDenominator))
            return 2.0;
        const double xTerm = xNumerator == 0.0
            ? 0.0 : xNumerator / xDenominator;
        const double yTerm = yNumerator == 0.0
            ? 0.0 : yNumerator / yDenominator;
        return xTerm * xTerm + yTerm * yTerm;
    };

    double xOnEllipse = 0.0;
    double yOnEllipse = 0.0;
    const double atZero = constraint(0.0);
    if (atZero == 1.0) {
        xOnEllipse = std::copysign(scaledX, x);
        yOnEllipse = std::copysign(scaledY, y);
    } else if (atZero < 1.0 && y == 0.0) {
        if (a == b) {
            if (x == 0.0) {
                if (!allowNonUnique)
                    return false;
                xOnEllipse = scaledA;
            } else {
                xOnEllipse = std::copysign(scaledA, x);
            }
        } else {
            const double cuspDenominator = aSquared - bSquared;
            if (cuspDenominator <= 0.0)
                return false;
            const double cuspCoordinate =
                scaledA * scaledX / cuspDenominator;
            if (cuspCoordinate < 1.0) {
                if (!allowNonUnique)
                    return false;
                // The two closest points are mirror images across the major
                // axis. Either has the same distance; choose +shortAxis only
                // for this distance-only caller.
                const double closestMajor =
                    aSquared * scaledX / cuspDenominator;
                const double normalizedMajor = closestMajor / scaledA;
                const double minorSquared =
                    1.0 - normalizedMajor * normalizedMajor;
                if (minorSquared < 0.0)
                    return false;
                xOnEllipse = std::copysign(closestMajor, x);
                yOnEllipse = scaledB * std::sqrt(minorSquared);
            } else if (cuspCoordinate == 1.0) {
                xOnEllipse = std::copysign(scaledA, x);
            } else {
                // The axis case has a root strictly above -b^2; the same
                // monotone solve below handles it without dividing by zero.
                double low = -bSquared;
                double high = 0.0;
                for (unsigned int iteration = 0u; iteration < 128u;
                     ++iteration) {
                    const double middle = low + (high - low) * 0.5;
                    if (middle == low || middle == high)
                        break;
                    if (constraint(middle) > 1.0)
                        low = middle;
                    else
                        high = middle;
                }
                const double denominator = aSquared + high;
                if (denominator <= 0.0)
                    return false;
                xOnEllipse = std::copysign(
                    aSquared * scaledX / denominator, x);
            }
        }
    } else {
        const bool outside = atZero > 1.0;
        double low = outside ? 0.0 : -bSquared;
        double high = outside
            ? 2.0 * (scaledA * scaledX + scaledB * scaledY) : 0.0;
        if (outside && (!std::isfinite(high) || high <= 0.0))
            return false;
        for (unsigned int iteration = 0u; iteration < 128u; ++iteration) {
            const double middle = low + (high - low) * 0.5;
            if (middle == low || middle == high)
                break;
            if (constraint(middle) > 1.0)
                low = middle;
            else
                high = middle;
        }
        const double xDenominator = aSquared + high;
        const double yDenominator = bSquared + high;
        if (xDenominator <= 0.0 || yDenominator <= 0.0)
            return false;
        xOnEllipse = std::copysign(
            aSquared * scaledX / xDenominator, x);
        yOnEllipse = std::copysign(
            bSquared * scaledY / yDenominator, y);
    }

    const double axisRoundoff = 64.0
        * std::numeric_limits<double>::epsilon();
    if (!std::isfinite(xOnEllipse) || !std::isfinite(yOnEllipse)
        || std::fabs(xOnEllipse) - scaledA > axisRoundoff * scaledA
        || std::fabs(yOnEllipse) - scaledB > axisRoundoff * scaledB)
        return false;
    if (std::fabs(xOnEllipse) > scaledA)
        xOnEllipse = std::copysign(scaledA, xOnEllipse);
    if (std::fabs(yOnEllipse) > scaledB)
        yOnEllipse = std::copysign(scaledB, yOnEllipse);
    const double ellipseRadius = std::hypot(
        xOnEllipse / scaledA, yOnEllipse / scaledB);
    if (!std::isfinite(ellipseRadius)
        || std::fabs(ellipseRadius - 1.0) > 64.0
            * std::numeric_limits<double>::epsilon())
        return false;

    const double centerScale = std::max(
        std::fabs(edge.p0.x),
        std::max(std::fabs(edge.p0.y), std::fabs(edge.p0.z)));
    const double outputScale = std::max(centerScale, solveScale);
    if (outputScale == 0.0 || !std::isfinite(outputScale))
        return false;
    const double offsetScale = solveScale / outputScale;
    const double center[3] = {edge.p0.x, edge.p0.y, edge.p0.z};
    const double longComponents[3] = {
        longAxis.x, longAxis.y, longAxis.z};
    const double shortComponents[3] = {
        shortAxis.x, shortAxis.y, shortAxis.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t axis = 0u; axis < 3u; ++axis) {
        const double normalized = center[axis] / outputScale
            + offsetScale * (xOnEllipse * longComponents[axis]
                + yOnEllipse * shortComponents[axis]);
        if (!finiteProductQuotient3(outputScale, normalized, 1.0,
                                    1.0, 1.0, coordinates[axis]))
            return false;
    }
    const DRW_Coord candidate(coordinates[0], coordinates[1], coordinates[2]);
    if (!isFinite(candidate))
        return false;
    projectedPoint = candidate;
    return true;
}

} // namespace

bool drw_projectPointToAcisEllipseCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double frameTolerance, DRW_Coord& projectedPoint) noexcept {
    return projectPointToAcisEllipseCurveImpl(
        edge, point, frameTolerance, false, projectedPoint);
}

bool drw_pointWithinToleranceOfAcisEllipseCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double frameTolerance, double linearTolerance) noexcept {
    if (!std::isfinite(linearTolerance) || linearTolerance < 0.0)
        return false;
    DRW_Coord projectedPoint;
    if (!projectPointToAcisEllipseCurveImpl(
            edge, point, frameTolerance, true, projectedPoint))
        return false;
    const DRW_Coord delta(point.x - projectedPoint.x,
                          point.y - projectedPoint.y,
                          point.z - projectedPoint.z);
    if (!isFinite(delta))
        return false;
    const double distance = std::hypot(std::hypot(delta.x, delta.y), delta.z);
    return std::isfinite(distance) && distance <= linearTolerance;
}

bool drw_acisEllipseEdgeEndpointsWithinTolerance(
    const DRW_AcisEdge& edge, double frameTolerance,
    double endpointCurveTolerance) noexcept {
    if (!edge.hasCurve || edge.curveType != DRW_AcisCurve::Ellipse
        || !edge.hasStart || !edge.hasEnd || !isFinite(edge.start)
        || !isFinite(edge.end) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || !std::isfinite(endpointCurveTolerance)
        || endpointCurveTolerance < 0.0)
        return false;

    return drw_pointWithinToleranceOfAcisEllipseCurve(
               edge, edge.start, frameTolerance, endpointCurveTolerance)
        && drw_pointWithinToleranceOfAcisEllipseCurve(
               edge, edge.end, frameTolerance, endpointCurveTolerance);
}

bool drw_evaluateAcisStraightEdge(
    const DRW_AcisEdge& edge, double endpointFraction,
    double endpointCurveTolerance, DRW_Coord& point) noexcept {
    if (!std::isfinite(endpointFraction)
        || endpointFraction < 0.0 || endpointFraction > 1.0)
        return false;

    double startParameter = 0.0;
    double endParameter = 0.0;
    if (!acisStraightEdgeParameterEndpoints(
            edge, endpointCurveTolerance, startParameter, endParameter))
        return false;

    double curveParameter = startParameter;
    if (endpointFraction == 1.0) {
        curveParameter = endParameter;
    } else if (endpointFraction != 0.0) {
        const double parameterDelta = endParameter - startParameter;
        if (std::isfinite(parameterDelta)) {
            curveParameter = std::fma(
                endpointFraction, parameterDelta, startParameter);
        } else {
            curveParameter = (1.0 - endpointFraction) * startParameter
                + endpointFraction * endParameter;
        }
    }

    if (!std::isfinite(curveParameter))
        return false;

    DRW_Coord candidatePoint;
    if (!drw_evaluateAcisStraightCurve(
            edge, curveParameter, candidatePoint))
        return false;

    point = candidatePoint;
    return true;
}

bool drw_evaluateAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    double traversalFraction, double endpointCurveTolerance,
    DRW_Coord& point) noexcept {
    if ((coedgeSense != 0 && coedgeSense != 1)
        || !std::isfinite(traversalFraction)
        || traversalFraction < 0.0 || traversalFraction > 1.0)
        return false;

    const double edgeFraction = coedgeSense == 0
        ? traversalFraction : 1.0 - traversalFraction;
    return drw_evaluateAcisStraightEdge(
        edge, edgeFraction, endpointCurveTolerance, point);
}

bool drw_projectPointToAcisStraightCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double& parameter, DRW_Coord& projectedPoint) noexcept {
    if (!edge.hasCurve || edge.curveType != DRW_AcisCurve::Straight
        || !isFinite(point) || !isFinite(edge.p0) || !isFinite(edge.p1))
        return false;

    const double directionScale = std::max(
        std::fabs(edge.p1.x),
        std::max(std::fabs(edge.p1.y), std::fabs(edge.p1.z)));
    if (directionScale == 0.0)
        return false;

    const DRW_Coord scaledDirection(
        edge.p1.x / directionScale, edge.p1.y / directionScale,
        edge.p1.z / directionScale);
    const double directionNormSquared =
        scaledDirection.x * scaledDirection.x
        + scaledDirection.y * scaledDirection.y
        + scaledDirection.z * scaledDirection.z;
    if (!std::isfinite(directionNormSquared)
        || directionNormSquared <= 0.0)
        return false;

    const DRW_Coord delta(point.x - edge.p0.x, point.y - edge.p0.y,
                          point.z - edge.p0.z);
    if (!isFinite(delta))
        return false;
    const double deltaScale = std::max(
        std::fabs(delta.x), std::max(std::fabs(delta.y), std::fabs(delta.z)));

    double candidateParameter = 0.0;
    DRW_Coord candidatePoint = edge.p0;
    if (deltaScale != 0.0) {
        const DRW_Coord scaledDelta(delta.x / deltaScale,
                                    delta.y / deltaScale,
                                    delta.z / deltaScale);
        const double dot = scaledDelta.x * scaledDirection.x
            + scaledDelta.y * scaledDirection.y
            + scaledDelta.z * scaledDirection.z;
        if (!finiteProductQuotient3(deltaScale, dot, 1.0,
                                   directionNormSquared, directionScale,
                                   candidateParameter))
            return false;

        double projectedDeltaX = 0.0;
        double projectedDeltaY = 0.0;
        double projectedDeltaZ = 0.0;
        if (!finiteProductQuotient3(deltaScale, dot, scaledDirection.x,
                                    directionNormSquared, 1.0,
                                    projectedDeltaX)
            || !finiteProductQuotient3(deltaScale, dot, scaledDirection.y,
                                       directionNormSquared, 1.0,
                                       projectedDeltaY)
            || !finiteProductQuotient3(deltaScale, dot, scaledDirection.z,
                                       directionNormSquared, 1.0,
                                       projectedDeltaZ))
            return false;

        candidatePoint = DRW_Coord(
            edge.p0.x + projectedDeltaX,
            edge.p0.y + projectedDeltaY,
            edge.p0.z + projectedDeltaZ);
        if (!isFinite(candidatePoint))
            return false;
    }

    parameter = candidateParameter;
    projectedPoint = candidatePoint;
    return true;
}

bool drw_pointWithinToleranceOfAcisStraightCurve(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double linearTolerance, double& parameter) noexcept {
    if (!std::isfinite(linearTolerance) || linearTolerance < 0.0
        || !isFinite(point))
        return false;

    double candidateParameter = 0.0;
    DRW_Coord projectedPoint;
    if (!drw_projectPointToAcisStraightCurve(
            edge, point, candidateParameter, projectedPoint))
        return false;

    const DRW_Coord residual(point.x - projectedPoint.x,
                             point.y - projectedPoint.y,
                             point.z - projectedPoint.z);
    if (!isFinite(residual))
        return false;

    const double maxResidual = std::max(
        std::fabs(residual.x),
        std::max(std::fabs(residual.y), std::fabs(residual.z)));
    if (maxResidual > linearTolerance)
        return false;
    if (maxResidual != 0.0) {
        const double normalizedX = residual.x / maxResidual;
        const double normalizedY = residual.y / maxResidual;
        const double normalizedZ = residual.z / maxResidual;
        const double normalizedDistanceSquared =
            normalizedX * normalizedX + normalizedY * normalizedY
            + normalizedZ * normalizedZ;
        const double normalizedTolerance = linearTolerance / maxResidual;
        if (normalizedTolerance < std::sqrt(3.0)
            && normalizedDistanceSquared
                > normalizedTolerance * normalizedTolerance)
            return false;
    }

    parameter = candidateParameter;
    return true;
}

bool drw_projectPointToAcisStraightEdge(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double endpointCurveTolerance, double& parameter,
    DRW_Coord& projectedPoint) noexcept {
    if (!isFinite(point))
        return false;

    double startParameter = 0.0;
    double endParameter = 0.0;
    if (!acisStraightEdgeParameterEndpoints(
            edge, endpointCurveTolerance, startParameter, endParameter))
        return false;

    double unboundedParameter = 0.0;
    DRW_Coord unboundedPoint;
    if (!drw_projectPointToAcisStraightCurve(
            edge, point, unboundedParameter, unboundedPoint))
        return false;

    const double lowerParameter = std::min(startParameter, endParameter);
    const double upperParameter = std::max(startParameter, endParameter);
    const double candidateParameter = std::max(
        lowerParameter, std::min(unboundedParameter, upperParameter));
    DRW_Coord candidatePoint = unboundedPoint;
    if (candidateParameter != unboundedParameter
        && !drw_evaluateAcisStraightCurve(
            edge, candidateParameter, candidatePoint))
        return false;

    parameter = candidateParameter;
    projectedPoint = candidatePoint;
    return true;
}

bool drw_pointWithinToleranceOfAcisStraightEdge(
    const DRW_AcisEdge& edge, const DRW_Coord& point,
    double endpointCurveTolerance, double linearTolerance,
    double& parameter) noexcept {
    if (!std::isfinite(linearTolerance) || linearTolerance < 0.0
        || !isFinite(point))
        return false;

    double candidateParameter = 0.0;
    DRW_Coord projectedPoint;
    if (!drw_projectPointToAcisStraightEdge(
            edge, point, endpointCurveTolerance, candidateParameter,
            projectedPoint))
        return false;

    const DRW_Coord residual(point.x - projectedPoint.x,
                             point.y - projectedPoint.y,
                             point.z - projectedPoint.z);
    if (!isFinite(residual))
        return false;

    const double maxResidual = std::max(
        std::fabs(residual.x),
        std::max(std::fabs(residual.y), std::fabs(residual.z)));
    if (maxResidual > linearTolerance)
        return false;
    if (maxResidual != 0.0) {
        const double normalizedX = residual.x / maxResidual;
        const double normalizedY = residual.y / maxResidual;
        const double normalizedZ = residual.z / maxResidual;
        const double normalizedDistanceSquared =
            normalizedX * normalizedX + normalizedY * normalizedY
            + normalizedZ * normalizedZ;
        const double normalizedTolerance = linearTolerance / maxResidual;
        if (normalizedTolerance < std::sqrt(3.0)
            && normalizedDistanceSquared
                > normalizedTolerance * normalizedTolerance)
            return false;
    }

    parameter = candidateParameter;
    return true;
}

bool drw_projectPointToAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    const DRW_Coord& point, double endpointCurveTolerance,
    double& traversalFraction, DRW_Coord& projectedPoint) noexcept {
    if (coedgeSense != 0 && coedgeSense != 1)
        return false;

    double candidateParameter = 0.0;
    DRW_Coord candidatePoint;
    if (!drw_projectPointToAcisStraightEdge(
            edge, point, endpointCurveTolerance, candidateParameter,
            candidatePoint))
        return false;

    double startParameter = 0.0;
    double endParameter = 0.0;
    if (!acisStraightEdgeParameterEndpoints(
            edge, endpointCurveTolerance, startParameter, endParameter))
        return false;
    double candidateFraction = 0.0;
    if (!acisStraightEdgeFractionForParameter(
            startParameter, endParameter, candidateParameter,
            candidateFraction))
        return false;
    if (coedgeSense == 1)
        candidateFraction = 1.0 - candidateFraction;

    traversalFraction = candidateFraction;
    projectedPoint = candidatePoint;
    return true;
}

bool drw_pointWithinToleranceOfAcisStraightCoedge(
    const DRW_AcisEdge& edge, std::int8_t coedgeSense,
    const DRW_Coord& point, double endpointCurveTolerance,
    double linearTolerance, double& traversalFraction) noexcept {
    if (coedgeSense != 0 && coedgeSense != 1)
        return false;

    double candidateParameter = 0.0;
    if (!drw_pointWithinToleranceOfAcisStraightEdge(
            edge, point, endpointCurveTolerance, linearTolerance,
            candidateParameter))
        return false;

    double startParameter = 0.0;
    double endParameter = 0.0;
    if (!acisStraightEdgeParameterEndpoints(
            edge, endpointCurveTolerance, startParameter, endParameter))
        return false;
    double candidateFraction = 0.0;
    if (!acisStraightEdgeFractionForParameter(
            startParameter, endParameter, candidateParameter,
            candidateFraction))
        return false;
    if (coedgeSense == 1)
        candidateFraction = 1.0 - candidateFraction;

    traversalFraction = candidateFraction;
    return true;
}

namespace {

bool acisPlaneUnitNormal(const DRW_AcisFace& face,
                         DRW_Coord& unitNormal) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Plane
        || !isFinite(face.p0) || !isFinite(face.p1))
        return false;
    const double normalScale = std::max(
        std::fabs(face.p1.x),
        std::max(std::fabs(face.p1.y), std::fabs(face.p1.z)));
    if (normalScale == 0.0)
        return false;

    const DRW_Coord scaledNormal(face.p1.x / normalScale,
                                 face.p1.y / normalScale,
                                 face.p1.z / normalScale);
    const double normalLength = std::sqrt(
        scaledNormal.x * scaledNormal.x
        + scaledNormal.y * scaledNormal.y
        + scaledNormal.z * scaledNormal.z);
    if (!std::isfinite(normalLength) || normalLength == 0.0)
        return false;
    const DRW_Coord candidateNormal(scaledNormal.x / normalLength,
                                    scaledNormal.y / normalLength,
                                    scaledNormal.z / normalLength);
    unitNormal = candidateNormal;
    return true;
}

bool acisTorusUnitAxis(const DRW_AcisFace& face,
                       DRW_Coord& unitAxis) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Torus
        || !isFinite(face.p0) || !isFinite(face.p1)
        || !std::isfinite(face.majorRadius)
        || !std::isfinite(face.minorRadius)
        || face.majorRadius <= 0.0
        || face.minorRadius <= 0.0)
        return false;

    const double axisScale = std::max(
        std::fabs(face.p1.x),
        std::max(std::fabs(face.p1.y), std::fabs(face.p1.z)));
    if (axisScale == 0.0)
        return false;

    const DRW_Coord scaledAxis(face.p1.x / axisScale,
                               face.p1.y / axisScale,
                               face.p1.z / axisScale);
    const double axisLength = std::sqrt(
        scaledAxis.x * scaledAxis.x + scaledAxis.y * scaledAxis.y
        + scaledAxis.z * scaledAxis.z);
    if (!std::isfinite(axisLength) || axisLength == 0.0)
        return false;

    unitAxis = DRW_Coord(scaledAxis.x / axisLength,
                         scaledAxis.y / axisLength,
                         scaledAxis.z / axisLength);
    return true;
}

bool acisCylinderSurfaceFrame(const DRW_AcisFace& face,
                              DRW_Coord& unitAxis,
                              double& radius) noexcept {
    // The ACIS cone-surface representation also covers the circular-cylinder
    // special case. Keep this helper on the exact profile emitted by the
    // pinned peer writer; other ratios/angles remain unsupported here.
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Cone
        || !isFinite(face.p0) || !isFinite(face.p1) || !isFinite(face.p2)
        || !std::isfinite(face.ratio) || face.ratio != 1.0
        || !std::isfinite(face.sineAngle) || face.sineAngle != 0.0
        || !std::isfinite(face.cosineAngle) || face.cosineAngle != 1.0)
        return false;

    const double axisScale = std::max(
        std::fabs(face.p1.x),
        std::max(std::fabs(face.p1.y), std::fabs(face.p1.z)));
    if (axisScale == 0.0)
        return false;
    const DRW_Coord scaledAxis(face.p1.x / axisScale,
                               face.p1.y / axisScale,
                               face.p1.z / axisScale);
    const double axisLength = std::sqrt(
        scaledAxis.x * scaledAxis.x + scaledAxis.y * scaledAxis.y
        + scaledAxis.z * scaledAxis.z);
    if (!std::isfinite(axisLength) || axisLength == 0.0)
        return false;

    const double radiusScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (radiusScale == 0.0)
        return false;
    const DRW_Coord scaledMajorAxis(face.p2.x / radiusScale,
                                    face.p2.y / radiusScale,
                                    face.p2.z / radiusScale);
    const double scaledRadius = std::sqrt(
        scaledMajorAxis.x * scaledMajorAxis.x
        + scaledMajorAxis.y * scaledMajorAxis.y
        + scaledMajorAxis.z * scaledMajorAxis.z);
    double candidateRadius = 0.0;
    if (!std::isfinite(scaledRadius) || scaledRadius == 0.0
        || !finiteProductQuotient3(radiusScale, scaledRadius, 1.0,
                                   1.0, 1.0, candidateRadius)
        || candidateRadius <= 0.0)
        return false;

    unitAxis = DRW_Coord(scaledAxis.x / axisLength,
                         scaledAxis.y / axisLength,
                         scaledAxis.z / axisLength);
    radius = candidateRadius;
    return true;
}

bool acisConeSurfaceFrame(const DRW_AcisFace& face, double frameTolerance,
                          DRW_Coord& unitAxis, DRW_Coord& unitMajor,
                          DRW_Coord& unitMinor,
                          DRW_Coord& scaledProjectedMajor,
                          double& majorScale,
                          double& projectedMajorLength) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Cone
        || !isFinite(face.p0) || !isFinite(face.p1) || !isFinite(face.p2)
        || !std::isfinite(face.ratio) || face.ratio <= 0.0
        || !std::isfinite(face.sineAngle)
        || !std::isfinite(face.cosineAngle)
        || face.cosineAngle <= 0.0
        || !std::isfinite(frameTolerance) || frameTolerance < 0.0)
        return false;

    const double angleLength = std::hypot(face.sineAngle,
                                          face.cosineAngle);
    if (!std::isfinite(angleLength)
        || std::fabs(angleLength - 1.0) > frameTolerance)
        return false;

    const double axisScale = std::max(
        std::fabs(face.p1.x),
        std::max(std::fabs(face.p1.y), std::fabs(face.p1.z)));
    majorScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (axisScale == 0.0 || majorScale == 0.0)
        return false;

    const DRW_Coord scaledAxis(face.p1.x / axisScale,
                               face.p1.y / axisScale,
                               face.p1.z / axisScale);
    const double axisLength = std::sqrt(
        scaledAxis.x * scaledAxis.x + scaledAxis.y * scaledAxis.y
        + scaledAxis.z * scaledAxis.z);
    if (!std::isfinite(axisLength) || axisLength == 0.0)
        return false;
    const DRW_Coord candidateAxis(scaledAxis.x / axisLength,
                                  scaledAxis.y / axisLength,
                                  scaledAxis.z / axisLength);

    const DRW_Coord scaledMajor(face.p2.x / majorScale,
                                face.p2.y / majorScale,
                                face.p2.z / majorScale);
    const double majorLength = std::sqrt(
        scaledMajor.x * scaledMajor.x + scaledMajor.y * scaledMajor.y
        + scaledMajor.z * scaledMajor.z);
    if (!std::isfinite(majorLength) || majorLength == 0.0)
        return false;
    const double axialMajorComponent = scaledMajor.x * candidateAxis.x
        + scaledMajor.y * candidateAxis.y
        + scaledMajor.z * candidateAxis.z;
    if (!std::isfinite(axialMajorComponent)
        || std::fabs(axialMajorComponent) / majorLength > frameTolerance)
        return false;

    const DRW_Coord projectedMajor(
        scaledMajor.x - axialMajorComponent * candidateAxis.x,
        scaledMajor.y - axialMajorComponent * candidateAxis.y,
        scaledMajor.z - axialMajorComponent * candidateAxis.z);
    projectedMajorLength = std::sqrt(
        projectedMajor.x * projectedMajor.x
        + projectedMajor.y * projectedMajor.y
        + projectedMajor.z * projectedMajor.z);
    if (!std::isfinite(projectedMajorLength)
        || projectedMajorLength == 0.0)
        return false;

    const DRW_Coord candidateMajor(
        projectedMajor.x / projectedMajorLength,
        projectedMajor.y / projectedMajorLength,
        projectedMajor.z / projectedMajorLength);
    DRW_Coord candidateMinor(
        candidateAxis.y * candidateMajor.z
            - candidateAxis.z * candidateMajor.y,
        candidateAxis.z * candidateMajor.x
            - candidateAxis.x * candidateMajor.z,
        candidateAxis.x * candidateMajor.y
            - candidateAxis.y * candidateMajor.x);
    const double minorLength = std::sqrt(
        candidateMinor.x * candidateMinor.x
        + candidateMinor.y * candidateMinor.y
        + candidateMinor.z * candidateMinor.z);
    if (!std::isfinite(minorLength) || minorLength == 0.0)
        return false;
    candidateMinor.x /= minorLength;
    candidateMinor.y /= minorLength;
    candidateMinor.z /= minorLength;

    unitAxis = candidateAxis;
    unitMajor = candidateMajor;
    unitMinor = candidateMinor;
    scaledProjectedMajor = projectedMajor;
    return true;
}

bool acisCylinderAxisAlignedCoordinate(const DRW_Coord& unitAxis,
                                       unsigned int& axisCoordinate) noexcept {
    if (std::fabs(unitAxis.x) == 1.0 && unitAxis.y == 0.0
        && unitAxis.z == 0.0) {
        axisCoordinate = 0u;
        return true;
    }
    if (unitAxis.x == 0.0 && std::fabs(unitAxis.y) == 1.0
        && unitAxis.z == 0.0) {
        axisCoordinate = 1u;
        return true;
    }
    if (unitAxis.x == 0.0 && unitAxis.y == 0.0
        && std::fabs(unitAxis.z) == 1.0) {
        axisCoordinate = 2u;
        return true;
    }
    return false;
}

bool acisCylinderAxisAlignedRadial(const DRW_Coord& point,
                                   const DRW_Coord& origin,
                                   unsigned int axisCoordinate,
                                   DRW_Coord& radialDirection,
                                   double* radialDistance) noexcept {
    if (axisCoordinate > 2u || !isFinite(point) || !isFinite(origin))
        return false;

    // Isolate perpendicular components before normalization. Scaling the full
    // delta by a much larger axial coordinate can underflow a valid radius.
    const DRW_Coord radial = axisCoordinate == 0u
        ? DRW_Coord(0.0, point.y - origin.y, point.z - origin.z)
        : axisCoordinate == 1u
            ? DRW_Coord(point.x - origin.x, 0.0, point.z - origin.z)
            : DRW_Coord(point.x - origin.x, point.y - origin.y, 0.0);
    if (!isFinite(radial))
        return false;
    const double radialScale = std::max(
        std::fabs(radial.x),
        std::max(std::fabs(radial.y), std::fabs(radial.z)));
    if (radialScale == 0.0) {
        radialDirection = DRW_Coord(0.0, 0.0, 0.0);
        if (radialDistance)
            *radialDistance = 0.0;
        return true;
    }

    const DRW_Coord scaledRadial(radial.x / radialScale,
                                 radial.y / radialScale,
                                 radial.z / radialScale);
    const double scaledLength = std::sqrt(
        scaledRadial.x * scaledRadial.x
        + scaledRadial.y * scaledRadial.y
        + scaledRadial.z * scaledRadial.z);
    if (!std::isfinite(scaledLength) || scaledLength == 0.0)
        return false;
    if (radialDistance
        && !finiteProductQuotient3(radialScale, scaledLength, 1.0,
                                   1.0, 1.0, *radialDistance))
        return false;

    radialDirection = DRW_Coord(scaledRadial.x / scaledLength,
                                scaledRadial.y / scaledLength,
                                scaledRadial.z / scaledLength);
    return isFinite(radialDirection);
}

bool acisCylinderSignedDistance(const DRW_AcisFace& face,
                                const DRW_Coord& point,
                                const DRW_Coord& unitAxis, double radius,
                                double& signedDistance) noexcept {
    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    unsigned int axisCoordinate = 0u;
    if (acisCylinderAxisAlignedCoordinate(unitAxis, axisCoordinate)) {
        DRW_Coord radialDirection;
        double radialDistance = 0.0;
        if (acisCylinderAxisAlignedRadial(point, face.p0, axisCoordinate,
                                          radialDirection,
                                          &radialDistance)) {
            const double candidateDistance = radialDistance - radius;
            if (std::isfinite(candidateDistance)) {
                signedDistance = candidateDistance;
                return true;
            }
        }
    }

    if (isFinite(delta)) {
        const double deltaScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (deltaScale == 0.0) {
            signedDistance = -radius;
            return true;
        }
        const DRW_Coord scaledDelta(delta.x / deltaScale,
                                    delta.y / deltaScale,
                                    delta.z / deltaScale);
        const double axial = scaledDelta.x * unitAxis.x
            + scaledDelta.y * unitAxis.y + scaledDelta.z * unitAxis.z;
        const DRW_Coord radial(scaledDelta.x - axial * unitAxis.x,
                               scaledDelta.y - axial * unitAxis.y,
                               scaledDelta.z - axial * unitAxis.z);
        const double radialFractionLength = std::sqrt(
            radial.x * radial.x + radial.y * radial.y
            + radial.z * radial.z);
        double radialDistance = 0.0;
        if (std::isfinite(radialFractionLength)
            && finiteProductQuotient3(deltaScale, radialFractionLength, 1.0,
                                      1.0, 1.0, radialDistance)) {
            const double candidateDistance = radialDistance - radius;
            if (std::isfinite(candidateDistance)) {
                signedDistance = candidateDistance;
                return true;
            }
        }
    }

    const double coordinateScale = std::max(
        std::max(std::fabs(point.x),
                 std::max(std::fabs(point.y), std::fabs(point.z))),
        std::max(
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y), std::fabs(face.p0.z))),
            radius));
    if (coordinateScale == 0.0 || !std::isfinite(coordinateScale))
        return false;

    const DRW_Coord scaledDelta = isFinite(delta)
        ? DRW_Coord(delta.x / coordinateScale, delta.y / coordinateScale,
                    delta.z / coordinateScale)
        : DRW_Coord(point.x / coordinateScale - face.p0.x / coordinateScale,
                    point.y / coordinateScale - face.p0.y / coordinateScale,
                    point.z / coordinateScale - face.p0.z / coordinateScale);
    const double deltaScale = std::max(
        std::fabs(scaledDelta.x),
        std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
    if (!isFinite(scaledDelta) || deltaScale == 0.0
        || !std::isfinite(deltaScale))
        return false;

    const DRW_Coord normalizedDelta(scaledDelta.x / deltaScale,
                                    scaledDelta.y / deltaScale,
                                    scaledDelta.z / deltaScale);
    const double axial = normalizedDelta.x * unitAxis.x
        + normalizedDelta.y * unitAxis.y
        + normalizedDelta.z * unitAxis.z;
    const DRW_Coord radial(normalizedDelta.x - axial * unitAxis.x,
                           normalizedDelta.y - axial * unitAxis.y,
                           normalizedDelta.z - axial * unitAxis.z);
    const double radialFractionLength = std::sqrt(
        radial.x * radial.x + radial.y * radial.y
        + radial.z * radial.z);
    const double scaledRadialDistance = deltaScale * radialFractionLength;
    const double scaledSignedDistance = scaledRadialDistance
        - radius / coordinateScale;
    if (!std::isfinite(scaledSignedDistance))
        return false;

    double candidateDistance = 0.0;
    if (!finiteProductQuotient3(coordinateScale, scaledSignedDistance, 1.0,
                                1.0, 1.0, candidateDistance))
        return false;
    signedDistance = candidateDistance;
    return true;
}

bool acisProjectPointToCylinder(const DRW_AcisFace& face,
                                const DRW_Coord& point,
                                const DRW_Coord& unitAxis, double radius,
                                DRW_Coord& projectedPoint) noexcept {
    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    const double pointScale = std::max(
        std::fabs(point.x), std::max(std::fabs(point.y), std::fabs(point.z)));
    const double originScale = std::max(
        std::fabs(face.p0.x),
        std::max(std::fabs(face.p0.y), std::fabs(face.p0.z)));
    double workScale = std::max(std::max(pointScale, originScale), radius);
    if (isFinite(delta)) {
        workScale = std::max(
            workScale,
            std::max(std::fabs(delta.x),
                     std::max(std::fabs(delta.y), std::fabs(delta.z))));
    }
    if (workScale == 0.0 || !std::isfinite(workScale))
        return false;

    const DRW_Coord scaledDelta = isFinite(delta)
        ? DRW_Coord(delta.x / workScale, delta.y / workScale,
                    delta.z / workScale)
        : DRW_Coord(point.x / workScale - face.p0.x / workScale,
                    point.y / workScale - face.p0.y / workScale,
                    point.z / workScale - face.p0.z / workScale);
    if (!isFinite(scaledDelta))
        return false;

    const double axial = scaledDelta.x * unitAxis.x
        + scaledDelta.y * unitAxis.y + scaledDelta.z * unitAxis.z;
    unsigned int axisCoordinate = 0u;
    DRW_Coord radialDirection;
    if (acisCylinderAxisAlignedCoordinate(unitAxis, axisCoordinate)
        && acisCylinderAxisAlignedRadial(point, face.p0, axisCoordinate,
                                         radialDirection, nullptr)) {
        if (radialDirection.x == 0.0 && radialDirection.y == 0.0
            && radialDirection.z == 0.0)
            return false;
    } else {
        const DRW_Coord radial(scaledDelta.x - axial * unitAxis.x,
                               scaledDelta.y - axial * unitAxis.y,
                               scaledDelta.z - axial * unitAxis.z);
        const double radialLength = std::sqrt(
            radial.x * radial.x + radial.y * radial.y
            + radial.z * radial.z);
        const double deltaLength = std::sqrt(
            scaledDelta.x * scaledDelta.x
            + scaledDelta.y * scaledDelta.y
            + scaledDelta.z * scaledDelta.z);
        const double ambiguityFloor =
            8.0 * std::numeric_limits<double>::epsilon() * deltaLength;
        if (!std::isfinite(radialLength) || !std::isfinite(deltaLength)
            || radialLength <= ambiguityFloor)
            return false;

        radialDirection = DRW_Coord(radial.x / radialLength,
                                    radial.y / radialLength,
                                    radial.z / radialLength);
    }

    const double radiusFraction = radius / workScale;
    const DRW_Coord base(
        face.p0.x / workScale + axial * unitAxis.x,
        face.p0.y / workScale + axial * unitAxis.y,
        face.p0.z / workScale + axial * unitAxis.z);
    DRW_Coord candidate;
    if (radiusFraction == 0.0) {
        // Preserve a tiny but representable radius when it is far below the
        // scale of a large origin/axial coordinate and normalized addition
        // would underflow it to zero.
        DRW_Coord basePoint;
        const auto scaleBase = [workScale](double value,
                                           double& result) noexcept {
            return finiteProductQuotient3(workScale, value, 1.0,
                                          1.0, 1.0, result);
        };
        if (!scaleBase(base.x, basePoint.x)
            || !scaleBase(base.y, basePoint.y)
            || !scaleBase(base.z, basePoint.z))
            return false;
        candidate = DRW_Coord(
            basePoint.x + radius * radialDirection.x,
            basePoint.y + radius * radialDirection.y,
            basePoint.z + radius * radialDirection.z);
        if (!isFinite(candidate))
            return false;
    } else {
        const DRW_Coord normalizedCandidate(
            base.x + radiusFraction * radialDirection.x,
            base.y + radiusFraction * radialDirection.y,
            base.z + radiusFraction * radialDirection.z);
        if (!finiteProductQuotient3(workScale, normalizedCandidate.x, 1.0,
                                    1.0, 1.0, candidate.x)
            || !finiteProductQuotient3(workScale, normalizedCandidate.y, 1.0,
                                       1.0, 1.0, candidate.y)
            || !finiteProductQuotient3(workScale, normalizedCandidate.z, 1.0,
                                       1.0, 1.0, candidate.z)
            || !isFinite(candidate))
            return false;
    }

    projectedPoint = candidate;
    return true;
}

bool acisTorusSignedDistance(const DRW_AcisFace& face,
                             const DRW_Coord& point,
                             const DRW_Coord& unitAxis,
                             double& signedDistance) noexcept {
    if (face.majorRadius <= face.minorRadius)
        return false;

    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    if (isFinite(delta)) {
        const double deltaScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (deltaScale == 0.0) {
            signedDistance = face.majorRadius - face.minorRadius;
            return true;
        }

        const DRW_Coord scaledDelta(delta.x / deltaScale,
                                    delta.y / deltaScale,
                                    delta.z / deltaScale);
        const double axialFraction = scaledDelta.x * unitAxis.x
            + scaledDelta.y * unitAxis.y
            + scaledDelta.z * unitAxis.z;
        const DRW_Coord radialFraction(
            scaledDelta.x - axialFraction * unitAxis.x,
            scaledDelta.y - axialFraction * unitAxis.y,
            scaledDelta.z - axialFraction * unitAxis.z);
        const double radialFractionLength = std::sqrt(
            radialFraction.x * radialFraction.x
            + radialFraction.y * radialFraction.y
            + radialFraction.z * radialFraction.z);
        const double radialDistance = deltaScale * radialFractionLength;
        const double axialDistance = deltaScale * axialFraction;
        if (std::isfinite(radialDistance) && std::isfinite(axialDistance)) {
            const double crossSectionDistance = std::hypot(
                radialDistance - face.majorRadius, axialDistance);
            if (std::isfinite(crossSectionDistance)) {
                const double candidateDistance =
                    crossSectionDistance - face.minorRadius;
                if (std::isfinite(candidateDistance)) {
                    signedDistance = candidateDistance;
                    return true;
                }
            }
        }
    }

    // Scale the overflowing coordinate difference and radii together. This
    // path is used only when direct differences or derived coordinates cannot
    // be represented as doubles.
    const double coordinateScale = std::max(
        std::max(std::fabs(point.x),
                 std::max(std::fabs(point.y), std::fabs(point.z))),
        std::max(
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y), std::fabs(face.p0.z))),
            std::max(face.majorRadius, face.minorRadius)));
    if (coordinateScale == 0.0 || !std::isfinite(coordinateScale))
        return false;

    const DRW_Coord scaledDelta(
        point.x / coordinateScale - face.p0.x / coordinateScale,
        point.y / coordinateScale - face.p0.y / coordinateScale,
        point.z / coordinateScale - face.p0.z / coordinateScale);
    const double deltaScale = std::max(
        std::fabs(scaledDelta.x),
        std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
    if (deltaScale == 0.0 || !std::isfinite(deltaScale))
        return false;

    const DRW_Coord normalizedDelta(scaledDelta.x / deltaScale,
                                   scaledDelta.y / deltaScale,
                                   scaledDelta.z / deltaScale);
    const double axialFraction = normalizedDelta.x * unitAxis.x
        + normalizedDelta.y * unitAxis.y
        + normalizedDelta.z * unitAxis.z;
    const DRW_Coord radialFraction(
        normalizedDelta.x - axialFraction * unitAxis.x,
        normalizedDelta.y - axialFraction * unitAxis.y,
        normalizedDelta.z - axialFraction * unitAxis.z);
    const double radialFractionLength = std::sqrt(
        radialFraction.x * radialFraction.x
        + radialFraction.y * radialFraction.y
        + radialFraction.z * radialFraction.z);
    const double scaledMajorRadius = face.majorRadius / coordinateScale;
    const double scaledMinorRadius = face.minorRadius / coordinateScale;
    const double scaledRadialDistance = deltaScale * radialFractionLength;
    const double scaledAxialDistance = deltaScale * axialFraction;
    const double scaledCrossSectionDistance = std::hypot(
        scaledRadialDistance - scaledMajorRadius, scaledAxialDistance);
    const double scaledSignedDistance =
        scaledCrossSectionDistance - scaledMinorRadius;
    if (!std::isfinite(scaledSignedDistance))
        return false;

    double candidateDistance = 0.0;
    if (!finiteProductQuotient3(coordinateScale, scaledSignedDistance,
                                1.0, 1.0, 1.0, candidateDistance))
        return false;
    signedDistance = candidateDistance;
    return true;
}

bool acisProjectPointToTorus(const DRW_AcisFace& face,
                             const DRW_Coord& point,
                             const DRW_Coord& unitAxis,
                             DRW_Coord& projectedPoint) noexcept {
    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    const double pointScale = std::max(
        std::fabs(point.x),
        std::max(std::fabs(point.y), std::fabs(point.z)));
    const double centerScale = std::max(
        std::fabs(face.p0.x),
        std::max(std::fabs(face.p0.y), std::fabs(face.p0.z)));
    double workScale = std::max(
        std::max(pointScale, centerScale),
        std::max(face.majorRadius, face.minorRadius));

    DRW_Coord unitDelta;
    double relativeDeltaMagnitude = 0.0;
    if (isFinite(delta)) {
        const double deltaScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (deltaScale == 0.0) {
            if (face.majorRadius == face.minorRadius) {
                projectedPoint = face.p0;
                return true;
            }
            return false;
        }
        unitDelta = DRW_Coord(delta.x / deltaScale,
                              delta.y / deltaScale,
                              delta.z / deltaScale);
        workScale = std::max(workScale, deltaScale);
        relativeDeltaMagnitude = deltaScale / workScale;
    } else {
        if (workScale == 0.0 || !std::isfinite(workScale))
            return false;
        const DRW_Coord scaledDelta(
            point.x / workScale - face.p0.x / workScale,
            point.y / workScale - face.p0.y / workScale,
            point.z / workScale - face.p0.z / workScale);
        const double deltaScale = std::max(
            std::fabs(scaledDelta.x),
            std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
        if (deltaScale == 0.0 || !std::isfinite(deltaScale))
            return false;
        unitDelta = DRW_Coord(scaledDelta.x / deltaScale,
                              scaledDelta.y / deltaScale,
                              scaledDelta.z / deltaScale);
        relativeDeltaMagnitude = deltaScale;
    }

    if (workScale == 0.0 || !std::isfinite(workScale)
        || !std::isfinite(relativeDeltaMagnitude))
        return false;

    const double axialFraction = unitDelta.x * unitAxis.x
        + unitDelta.y * unitAxis.y + unitDelta.z * unitAxis.z;
    const DRW_Coord radialFraction(
        unitDelta.x - axialFraction * unitAxis.x,
        unitDelta.y - axialFraction * unitAxis.y,
        unitDelta.z - axialFraction * unitAxis.z);
    const double radialFractionLength = std::sqrt(
        radialFraction.x * radialFraction.x
        + radialFraction.y * radialFraction.y
        + radialFraction.z * radialFraction.z);
    if (!std::isfinite(radialFractionLength))
        return false;
    const bool hasRadialDirection = radialFractionLength != 0.0;
    const DRW_Coord radialDirection = hasRadialDirection
        ? DRW_Coord(radialFraction.x / radialFractionLength,
                    radialFraction.y / radialFractionLength,
                    radialFraction.z / radialFractionLength)
        : DRW_Coord();

    const double scaledMajorRadius = face.majorRadius / workScale;
    const double scaledMinorRadius = face.minorRadius / workScale;
    const double geometryScale = std::max(
        relativeDeltaMagnitude,
        std::max(scaledMajorRadius, scaledMinorRadius));
    if (geometryScale == 0.0 || !std::isfinite(geometryScale))
        return false;

    const double normalizedDeltaMagnitude =
        relativeDeltaMagnitude / geometryScale;
    const double majorRadius = scaledMajorRadius / geometryScale;
    const double minorRadius = scaledMinorRadius / geometryScale;
    if (!std::isfinite(normalizedDeltaMagnitude)
        || !std::isfinite(majorRadius) || !std::isfinite(minorRadius)
        || minorRadius == 0.0)
        return false;

    const double radialCoordinate =
        normalizedDeltaMagnitude * radialFractionLength;
    const double axialCoordinate = normalizedDeltaMagnitude * axialFraction;
    const double crossSectionRadial = radialCoordinate - majorRadius;
    const double crossSectionDistance =
        std::hypot(crossSectionRadial, axialCoordinate);
    if (!std::isfinite(crossSectionDistance)
        || crossSectionDistance == 0.0)
        return false;

    const double projectedRadial = majorRadius
        + minorRadius * (crossSectionRadial / crossSectionDistance);
    const double projectedAxial =
        minorRadius * (axialCoordinate / crossSectionDistance);
    if (!hasRadialDirection && projectedRadial != 0.0)
        return false;
    const double offsetX = geometryScale
        * (radialDirection.x * projectedRadial
           + unitAxis.x * projectedAxial);
    const double offsetY = geometryScale
        * (radialDirection.y * projectedRadial
           + unitAxis.y * projectedAxial);
    const double offsetZ = geometryScale
        * (radialDirection.z * projectedRadial
           + unitAxis.z * projectedAxial);
    const double centerX = face.p0.x / workScale;
    const double centerY = face.p0.y / workScale;
    const double centerZ = face.p0.z / workScale;
    const double normalizedX = centerX + offsetX;
    const double normalizedY = centerY + offsetY;
    const double normalizedZ = centerZ + offsetZ;
    DRW_Coord candidatePoint;
    if (!finiteProductQuotient3(workScale, normalizedX, 1.0, 1.0, 1.0,
                                candidatePoint.x)
        || !finiteProductQuotient3(workScale, normalizedY, 1.0, 1.0, 1.0,
                                   candidatePoint.y)
        || !finiteProductQuotient3(workScale, normalizedZ, 1.0, 1.0, 1.0,
                                   candidatePoint.z)
        || !isFinite(candidatePoint))
        return false;

    projectedPoint = candidatePoint;
    return true;
}

} // namespace

bool drw_pointWithinToleranceOfAcisPlaneSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(linearTolerance)
        || linearTolerance < 0.0)
        return false;
    DRW_Coord unitNormal;
    if (!acisPlaneUnitNormal(face, unitNormal))
        return false;

    const DRW_Coord delta(point.x - face.p0.x, point.y - face.p0.y,
                          point.z - face.p0.z);
    double candidateDistance = 0.0;
    double normalizedDot = 0.0;
    double distanceScale = 0.0;
    bool useScaledCoordinates = !isFinite(delta);
    DRW_Coord scaledDelta;
    if (!useScaledCoordinates) {
        distanceScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (distanceScale != 0.0) {
            scaledDelta = DRW_Coord(delta.x / distanceScale,
                                    delta.y / distanceScale,
                                    delta.z / distanceScale);
        }
    } else {
        const double coordinateScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))));
        if (coordinateScale == 0.0)
            return false;
        scaledDelta = DRW_Coord(
            point.x / coordinateScale - face.p0.x / coordinateScale,
            point.y / coordinateScale - face.p0.y / coordinateScale,
            point.z / coordinateScale - face.p0.z / coordinateScale);
        distanceScale = std::max(
            std::fabs(scaledDelta.x),
            std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
        if (distanceScale == 0.0)
            return false;
        scaledDelta.x /= distanceScale;
        scaledDelta.y /= distanceScale;
        scaledDelta.z /= distanceScale;
        normalizedDot = scaledDelta.x * unitNormal.x
            + scaledDelta.y * unitNormal.y
            + scaledDelta.z * unitNormal.z;
        if (!finiteProductQuotient3(coordinateScale, distanceScale,
                                    normalizedDot, 1.0, 1.0,
                                    candidateDistance))
            return false;
    }

    if (!useScaledCoordinates && distanceScale != 0.0) {
        normalizedDot = scaledDelta.x * unitNormal.x
            + scaledDelta.y * unitNormal.y
            + scaledDelta.z * unitNormal.z;
        if (!finiteProductQuotient3(distanceScale, normalizedDot, 1.0,
                                    1.0, 1.0, candidateDistance))
            return false;
    }
    if (candidateDistance == 0.0 && normalizedDot != 0.0
        && linearTolerance == 0.0)
        return false;
    if (std::fabs(candidateDistance) > linearTolerance)
        return false;

    signedDistance = candidateDistance;
    return true;
}

bool drw_projectPointToAcisPlaneSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept {
    if (!isFinite(point))
        return false;
    DRW_Coord unitNormal;
    if (!acisPlaneUnitNormal(face, unitNormal))
        return false;

    const auto projectComponent = [](
        double value, double correctionScale, double signedScale,
        double normalComponent, double& result) noexcept {
        double correction = 0.0;
        if (finiteProductQuotient3(correctionScale, signedScale,
                                   normalComponent, 1.0, 1.0,
                                   correction)) {
            const double candidate = value - correction;
            if (!std::isfinite(candidate))
                return false;
            result = candidate;
            return true;
        }

        const double scale = std::max(std::fabs(value), correctionScale);
        if (scale == 0.0) {
            result = 0.0;
            return true;
        }
        const double normalizedProjection = value / scale
            - (correctionScale / scale) * signedScale * normalComponent;
        return finiteProductQuotient3(scale, normalizedProjection, 1.0,
                                      1.0, 1.0, result);
    };

    const DRW_Coord delta(point.x - face.p0.x, point.y - face.p0.y,
                          point.z - face.p0.z);
    DRW_Coord candidatePoint = point;
    if (isFinite(delta)) {
        const double deltaScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (deltaScale != 0.0) {
            const DRW_Coord normalizedDelta(delta.x / deltaScale,
                                            delta.y / deltaScale,
                                            delta.z / deltaScale);
            const double signedScale = normalizedDelta.x * unitNormal.x
                + normalizedDelta.y * unitNormal.y
                + normalizedDelta.z * unitNormal.z;
            if (!projectComponent(point.x, deltaScale, signedScale,
                                  unitNormal.x, candidatePoint.x)
                || !projectComponent(point.y, deltaScale, signedScale,
                                     unitNormal.y, candidatePoint.y)
                || !projectComponent(point.z, deltaScale, signedScale,
                                     unitNormal.z, candidatePoint.z))
                return false;
        }
    } else {
        const double coordinateScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))));
        if (coordinateScale == 0.0)
            return false;
        const DRW_Coord scaledDelta(
            point.x / coordinateScale - face.p0.x / coordinateScale,
            point.y / coordinateScale - face.p0.y / coordinateScale,
            point.z / coordinateScale - face.p0.z / coordinateScale);
        const double deltaScale = std::max(
            std::fabs(scaledDelta.x),
            std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
        if (deltaScale == 0.0)
            return false;
        const DRW_Coord normalizedDelta(scaledDelta.x / deltaScale,
                                        scaledDelta.y / deltaScale,
                                        scaledDelta.z / deltaScale);
        const double signedScale = deltaScale
            * (normalizedDelta.x * unitNormal.x
               + normalizedDelta.y * unitNormal.y
               + normalizedDelta.z * unitNormal.z);
        if (!projectComponent(point.x, coordinateScale, signedScale,
                              unitNormal.x, candidatePoint.x)
            || !projectComponent(point.y, coordinateScale, signedScale,
                                 unitNormal.y, candidatePoint.y)
            || !projectComponent(point.z, coordinateScale, signedScale,
                                 unitNormal.z, candidatePoint.z))
            return false;
    }

    projectedPoint = candidatePoint;
    return true;
}

bool drw_evaluateAcisPlaneFaceNormal(
    const DRW_AcisFace& face, DRW_Coord& faceNormal) noexcept {
    if (!face.hasSense)
        return false;

    DRW_Coord surfaceNormal;
    if (!acisPlaneUnitNormal(face, surfaceNormal))
        return false;
    return drw_orientAcisFaceNormal(face, surfaceNormal, faceNormal);
}

bool drw_evaluateAcisConeSurface(const DRW_AcisFace& face,
                                 double uParameter, double vParameter,
                                 double frameTolerance,
                                 DRW_Coord& point) noexcept {
    if (!std::isfinite(uParameter) || !std::isfinite(vParameter))
        return false;

    DRW_Coord unitAxis;
    DRW_Coord unitMajor;
    DRW_Coord unitMinor;
    DRW_Coord scaledProjectedMajor;
    double majorScale = 0.0;
    double projectedMajorLength = 0.0;
    if (!acisConeSurfaceFrame(face, frameTolerance, unitAxis, unitMajor,
                              unitMinor, scaledProjectedMajor, majorScale,
                              projectedMajorLength))
        return false;

    const double cosineU = std::cos(uParameter);
    const double sineU = std::sin(uParameter);
    if (!std::isfinite(cosineU) || !std::isfinite(sineU))
        return false;

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double major[3] = {scaledProjectedMajor.x,
                             scaledProjectedMajor.y,
                             scaledProjectedMajor.z};
    const double unitMajorComponents[3] = {
        unitMajor.x, unitMajor.y, unitMajor.z};
    const double unitMinorComponents[3] = {
        unitMinor.x, unitMinor.y, unitMinor.z};
    const double unitAxisComponents[3] = {
        unitAxis.x, unitAxis.y, unitAxis.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0, 1.0,
                                terms[0])
            || !scaledAcisProduct5(majorScale, major[coordinate], cosineU,
                                   1.0, 1.0, terms[1])
            || !scaledAcisProduct5(majorScale, projectedMajorLength,
                                   face.ratio, sineU,
                                   unitMinorComponents[coordinate],
                                   terms[2])
            || !scaledAcisProduct5(vParameter, face.sineAngle, cosineU,
                                   unitMajorComponents[coordinate], 1.0,
                                   terms[3])
            || !scaledAcisProduct5(vParameter, face.sineAngle, face.ratio,
                                   sineU, unitMinorComponents[coordinate],
                                   terms[4])
            || !scaledAcisProduct5(vParameter, face.cosineAngle,
                                   unitAxisComponents[coordinate], 1.0, 1.0,
                                   terms[5])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }

    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    point = candidatePoint;
    return true;
}

bool drw_evaluateAcisConeFaceNormal(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& faceNormal) noexcept {
    if (!face.hasSense || !std::isfinite(uParameter)
        || !std::isfinite(vParameter))
        return false;

    DRW_Coord unitAxis;
    DRW_Coord unitMajor;
    DRW_Coord unitMinor;
    DRW_Coord scaledProjectedMajor;
    double majorScale = 0.0;
    double projectedMajorLength = 0.0;
    if (!acisConeSurfaceFrame(face, frameTolerance, unitAxis, unitMajor,
                              unitMinor, scaledProjectedMajor, majorScale,
                              projectedMajorLength))
        return false;

    std::array<ScaledAcisProduct, 6> radialTerms;
    double radialScale = 0.0;
    if (!scaledAcisProduct5(majorScale, projectedMajorLength, 1.0, 1.0,
                            1.0, radialTerms[0])
        || !scaledAcisProduct5(vParameter, face.sineAngle, 1.0, 1.0,
                               1.0, radialTerms[1])
        || !finiteScaledAcisSum6(radialTerms, radialScale)
        || radialScale == 0.0)
        return false;

    const double cosineU = std::cos(uParameter);
    const double sineU = std::sin(uParameter);
    if (!std::isfinite(cosineU) || !std::isfinite(sineU))
        return false;

    const double majorCoefficient =
        face.ratio * face.cosineAngle * cosineU;
    const double minorCoefficient = face.cosineAngle * sineU;
    const double axisCoefficient = -face.ratio * face.sineAngle;
    if (!std::isfinite(majorCoefficient)
        || !std::isfinite(minorCoefficient)
        || !std::isfinite(axisCoefficient))
        return false;
    const double coefficientScale = std::max(
        std::fabs(majorCoefficient),
        std::max(std::fabs(minorCoefficient),
                 std::fabs(axisCoefficient)));
    if (coefficientScale == 0.0)
        return false;

    const double nappeDirection = radialScale < 0.0 ? -1.0 : 1.0;
    const double scaledMajorCoefficient =
        nappeDirection * majorCoefficient / coefficientScale;
    const double scaledMinorCoefficient =
        nappeDirection * minorCoefficient / coefficientScale;
    const double scaledAxisCoefficient =
        nappeDirection * axisCoefficient / coefficientScale;
    const DRW_Coord surfaceNormal(
        scaledMajorCoefficient * unitMajor.x
            + scaledMinorCoefficient * unitMinor.x
            + scaledAxisCoefficient * unitAxis.x,
        scaledMajorCoefficient * unitMajor.y
            + scaledMinorCoefficient * unitMinor.y
            + scaledAxisCoefficient * unitAxis.y,
        scaledMajorCoefficient * unitMajor.z
            + scaledMinorCoefficient * unitMinor.z
            + scaledAxisCoefficient * unitAxis.z);
    if (!isFinite(surfaceNormal))
        return false;

    return drw_orientAcisFaceNormal(face, surfaceNormal, faceNormal);
}

namespace {

struct AcisConeMeridianProjection {
    double radial = 0.0;
    double axial = 0.0;
    double distance = 0.0;
    bool valid = false;
};

bool acisCircularConeMeridianCandidate(
    double branch, double radialQuery, double axialQuery,
    double scaledRadius, double sineAngle, double cosineAngle,
    AcisConeMeridianProjection& candidate) noexcept {
    const double branchSine = branch * sineAngle;
    const double baseRadial = branch * scaledRadius;
    const double denominator = branchSine * branchSine
        + cosineAngle * cosineAngle;
    if (!std::isfinite(denominator) || denominator == 0.0)
        return false;
    const double numerator = (radialQuery - baseRadial) * branchSine
        + axialQuery * cosineAngle;
    if (!std::isfinite(numerator))
        return false;
    double parameter = numerator / denominator;
    if (!std::isfinite(parameter))
        return false;

    double radial = branch * (scaledRadius + sineAngle * parameter);
    if (!std::isfinite(radial))
        return false;
    if (radial < 0.0) {
        if (sineAngle == 0.0)
            return false;
        parameter = -scaledRadius / sineAngle;
        if (!std::isfinite(parameter))
            return false;
        radial = 0.0;
    }
    const double axial = cosineAngle * parameter;
    if (!std::isfinite(axial))
        return false;
    const double radialResidual = radialQuery - radial;
    const double axialResidual = axialQuery - axial;
    if (!std::isfinite(radialResidual) || !std::isfinite(axialResidual))
        return false;
    const double distance = std::hypot(radialResidual, axialResidual);
    if (!std::isfinite(distance))
        return false;

    candidate.radial = radial;
    candidate.axial = axial;
    candidate.distance = distance;
    candidate.valid = true;
    return true;
}

bool acisCircularConeClosestPoint(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, DRW_Coord* projectedPoint,
    bool& projectionUnique, double& distanceScale,
    double& normalizedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || face.ratio != 1.0)
        return false;

    DRW_Coord unitAxis;
    DRW_Coord unitMajor;
    DRW_Coord unitMinor;
    DRW_Coord scaledProjectedMajor;
    double majorScale = 0.0;
    double projectedMajorLength = 0.0;
    if (!acisConeSurfaceFrame(face, frameTolerance, unitAxis, unitMajor,
                              unitMinor, scaledProjectedMajor, majorScale,
                              projectedMajorLength))
        return false;

    double radius = 0.0;
    if (!finiteProductQuotient3(majorScale, projectedMajorLength, 1.0,
                                1.0, 1.0, radius)
        || radius <= 0.0)
        return false;
    distanceScale = std::max(
        radius,
        std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z)))));
    if (distanceScale == 0.0 || !std::isfinite(distanceScale))
        return false;
    const double scaledRadius = radius / distanceScale;
    if (!std::isfinite(scaledRadius) || scaledRadius == 0.0)
        return false;

    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    const DRW_Coord scaledDelta = isFinite(delta)
        ? DRW_Coord(delta.x / distanceScale, delta.y / distanceScale,
                    delta.z / distanceScale)
        : DRW_Coord(point.x / distanceScale - face.p0.x / distanceScale,
                    point.y / distanceScale - face.p0.y / distanceScale,
                    point.z / distanceScale - face.p0.z / distanceScale);
    if (!isFinite(scaledDelta))
        return false;

    const double localMajor = scaledDelta.x * unitMajor.x
        + scaledDelta.y * unitMajor.y + scaledDelta.z * unitMajor.z;
    const double localMinor = scaledDelta.x * unitMinor.x
        + scaledDelta.y * unitMinor.y + scaledDelta.z * unitMinor.z;
    const double localAxial = scaledDelta.x * unitAxis.x
        + scaledDelta.y * unitAxis.y + scaledDelta.z * unitAxis.z;
    if (!std::isfinite(localMajor) || !std::isfinite(localMinor)
        || !std::isfinite(localAxial))
        return false;
    const double radialQuery = std::hypot(localMajor, localMinor);
    if (!std::isfinite(radialQuery))
        return false;

    AcisConeMeridianProjection candidates[2];
    acisCircularConeMeridianCandidate(
        1.0, radialQuery, localAxial, scaledRadius, face.sineAngle,
        face.cosineAngle, candidates[0]);
    acisCircularConeMeridianCandidate(
        -1.0, radialQuery, localAxial, scaledRadius, face.sineAngle,
        face.cosineAngle, candidates[1]);
    if (!candidates[0].valid && !candidates[1].valid)
        return false;

    if (radialQuery == 0.0 && face.sineAngle != 0.0) {
        const double apexParameter = -scaledRadius / face.sineAngle;
        const double apexAxial = face.cosineAngle * apexParameter;
        const double apexResidual = std::fabs(localAxial - apexAxial);
        const double apexScale = std::max(
            1.0, std::max(std::fabs(localAxial), std::fabs(apexAxial)));
        const double apexRoundoff = 16.0
            * std::numeric_limits<double>::epsilon() * apexScale;
        if (std::isfinite(apexParameter) && std::isfinite(apexAxial)
            && apexResidual <= apexRoundoff) {
            candidates[0].radial = 0.0;
            candidates[0].axial = apexAxial;
            candidates[0].distance = apexResidual;
            candidates[0].valid = true;
            candidates[1] = candidates[0];
        }
    }

    std::size_t selected = candidates[0].valid ? 0u : 1u;
    projectionUnique = true;
    if (candidates[0].valid && candidates[1].valid) {
        const double distanceDifference = std::fabs(
            candidates[0].distance - candidates[1].distance);
        const double comparisonScale = std::max(
            1.0, std::max(candidates[0].distance,
                          candidates[1].distance));
        const double tieBound = 128.0
            * std::numeric_limits<double>::epsilon() * comparisonScale;
        if (distanceDifference <= tieBound) {
            const bool sameMeridianPoint =
                candidates[0].radial == candidates[1].radial
                && candidates[0].axial == candidates[1].axial;
            projectionUnique = sameMeridianPoint;
        } else {
            selected = candidates[1].distance < candidates[0].distance
                ? 1u : 0u;
        }
    }

    AcisConeMeridianProjection closest = candidates[selected];
    if (radialQuery == 0.0 && closest.radial != 0.0) {
        projectionUnique = false;
        if (face.sineAngle != 0.0) {
            const double apexParameter = -scaledRadius / face.sineAngle;
            const double apexAxial = face.cosineAngle * apexParameter;
            const double apexResidual = std::fabs(localAxial - apexAxial);
            const double apexScale = std::max(
                1.0, std::max(std::fabs(localAxial),
                               std::fabs(apexAxial)));
            const double apexRoundoff = 16.0
                * std::numeric_limits<double>::epsilon() * apexScale;
            if (std::isfinite(apexParameter) && std::isfinite(apexAxial)
                && apexResidual <= apexRoundoff) {
                closest.radial = 0.0;
                closest.axial = apexAxial;
                closest.distance = apexResidual;
                projectionUnique = true;
            }
        }
    }
    normalizedDistance = closest.distance;
    if (!projectedPoint || !projectionUnique)
        return true;

    DRW_Coord radialDirection;
    if (closest.radial != 0.0) {
        if (radialQuery == 0.0)
            return false;
        const double majorFraction = localMajor / radialQuery;
        const double minorFraction = localMinor / radialQuery;
        radialDirection = DRW_Coord(
            majorFraction * unitMajor.x + minorFraction * unitMinor.x,
            majorFraction * unitMajor.y + minorFraction * unitMinor.y,
            majorFraction * unitMajor.z + minorFraction * unitMinor.z);
        if (!isFinite(radialDirection))
            return false;
    }

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double axis[3] = {unitAxis.x, unitAxis.y, unitAxis.z};
    const double radial[3] = {
        radialDirection.x, radialDirection.y, radialDirection.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0, 1.0,
                                terms[0])
            || !scaledAcisProduct5(distanceScale, closest.axial,
                                   axis[coordinate], 1.0, 1.0, terms[1])
            || !scaledAcisProduct5(distanceScale, closest.radial,
                                   radial[coordinate], 1.0, 1.0, terms[2])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }

    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    *projectedPoint = candidatePoint;
    return true;
}

struct AcisPolynomialRootSet {
    double values[4] = {0.0, 0.0, 0.0, 0.0};
    std::size_t count = 0u;
    bool identicallyZero = false;
};

double acisPolynomialValue(const double* coefficients, std::size_t degree,
                           double x) noexcept {
    double value = coefficients[degree];
    while (degree != 0u)
        value = value * x + coefficients[--degree];
    return value;
}

double acisPolynomialMagnitudeBound(const double* coefficients,
                                    std::size_t degree,
                                    double x) noexcept {
    const double absoluteX = std::fabs(x);
    double value = std::fabs(coefficients[degree]);
    while (degree != 0u)
        value = value * absoluteX + std::fabs(coefficients[--degree]);
    return value;
}

void acisAppendPolynomialRoot(AcisPolynomialRootSet& roots,
                              double value) noexcept {
    if (!std::isfinite(value) || value < -1.0 || value > 1.0)
        return;
    const double duplicateBound = 64.0
        * std::numeric_limits<double>::epsilon()
        * std::max(1.0, std::fabs(value));
    for (std::size_t index = 0u; index < roots.count; ++index) {
        if (std::fabs(roots.values[index] - value) <= duplicateBound)
            return;
    }
    if (roots.count < 4u)
        roots.values[roots.count++] = value;
}

bool acisPolynomialRootsInUnitInterval(
    const std::array<double, 5>& input, std::size_t inputDegree,
    AcisPolynomialRootSet& roots) noexcept {
    roots = AcisPolynomialRootSet();
    if (inputDegree > 4u)
        return false;

    double coefficients[5] = {0.0, 0.0, 0.0, 0.0, 0.0};
    double coefficientScale = 0.0;
    for (std::size_t index = 0u; index <= inputDegree; ++index) {
        if (!std::isfinite(input[index]))
            return false;
        coefficientScale = std::max(coefficientScale,
                                    std::fabs(input[index]));
    }
    if (coefficientScale == 0.0) {
        roots.identicallyZero = true;
        return true;
    }
    for (std::size_t index = 0u; index <= inputDegree; ++index)
        coefficients[index] = input[index] / coefficientScale;

    std::size_t degree = inputDegree;
    while (degree != 0u && coefficients[degree] == 0.0)
        --degree;
    if (degree == 0u)
        return true;

    if (degree == 1u) {
        const double root = -coefficients[0] / coefficients[1];
        if (root >= -1.0 && root <= 1.0)
            acisAppendPolynomialRoot(roots, root);
        return true;
    }

    std::array<double, 5> derivative = {{0.0, 0.0, 0.0, 0.0, 0.0}};
    for (std::size_t index = 1u; index <= degree; ++index)
        derivative[index - 1u] = static_cast<double>(index)
            * coefficients[index];
    AcisPolynomialRootSet criticalPoints;
    if (!acisPolynomialRootsInUnitInterval(derivative, degree - 1u,
                                           criticalPoints))
        return false;

    double boundaries[6] = {-1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    std::size_t boundaryCount = 1u;
    for (std::size_t index = 0u; index < criticalPoints.count; ++index) {
        const double critical = criticalPoints.values[index];
        if (critical > -1.0 && critical < 1.0)
            boundaries[boundaryCount++] = critical;
    }
    boundaries[boundaryCount++] = 1.0;
    std::sort(boundaries, boundaries + boundaryCount);

    const double zeroTolerance = 128.0
        * std::numeric_limits<double>::epsilon();
    for (std::size_t index = 0u; index < boundaryCount; ++index) {
        const double value = acisPolynomialValue(
            coefficients, degree, boundaries[index]);
        const double bound = acisPolynomialMagnitudeBound(
            coefficients, degree, boundaries[index]);
        if (!std::isfinite(value) || !std::isfinite(bound))
            return false;
        if (std::fabs(value) <= zeroTolerance * bound)
            acisAppendPolynomialRoot(roots, boundaries[index]);
    }

    for (std::size_t interval = 1u; interval < boundaryCount; ++interval) {
        double low = boundaries[interval - 1u];
        double high = boundaries[interval];
        double lowValue = acisPolynomialValue(coefficients, degree, low);
        double highValue = acisPolynomialValue(coefficients, degree, high);
        if (lowValue == 0.0 || highValue == 0.0
            || std::signbit(lowValue) == std::signbit(highValue))
            continue;
        for (unsigned int iteration = 0u; iteration < 128u; ++iteration) {
            const double middle = low + (high - low) * 0.5;
            if (middle == low || middle == high)
                break;
            const double middleValue = acisPolynomialValue(
                coefficients, degree, middle);
            if (middleValue == 0.0) {
                low = middle;
                high = middle;
                break;
            }
            if (std::signbit(lowValue) != std::signbit(middleValue)) {
                high = middle;
                highValue = middleValue;
            } else {
                low = middle;
                lowValue = middleValue;
            }
        }
        (void)highValue;
        acisAppendPolynomialRoot(roots, low + (high - low) * 0.5);
    }

    std::sort(roots.values, roots.values + roots.count);
    return true;
}

struct AcisEllipticConeCandidate {
    double localMajor = 0.0;
    double localMinor = 0.0;
    double localAxial = 0.0;
    double distance = 0.0;
    bool valid = false;
};

bool acisEllipticConeClosestPoint(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, DRW_Coord* projectedPoint,
    bool& projectionUnique, double& distanceScale,
    double& normalizedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || face.ratio == 1.0
        || face.sineAngle == 0.0)
        return false;

    DRW_Coord unitAxis;
    DRW_Coord unitMajor;
    DRW_Coord unitMinor;
    DRW_Coord scaledProjectedMajor;
    double majorScale = 0.0;
    double projectedMajorLength = 0.0;
    if (!acisConeSurfaceFrame(face, frameTolerance, unitAxis, unitMajor,
                              unitMinor, scaledProjectedMajor, majorScale,
                              projectedMajorLength))
        return false;

    double radius = 0.0;
    if (!finiteProductQuotient3(majorScale, projectedMajorLength, 1.0,
                                1.0, 1.0, radius)
        || radius <= 0.0)
        return false;

    const double coordinateScale = std::max(
        std::max(std::fabs(point.x),
                 std::max(std::fabs(point.y), std::fabs(point.z))),
        std::max(
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))), radius));
    if (coordinateScale == 0.0 || !std::isfinite(coordinateScale))
        return false;

    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    const DRW_Coord scaledDelta = isFinite(delta)
        ? DRW_Coord(delta.x / coordinateScale,
                    delta.y / coordinateScale,
                    delta.z / coordinateScale)
        : DRW_Coord(point.x / coordinateScale - face.p0.x / coordinateScale,
                    point.y / coordinateScale - face.p0.y / coordinateScale,
                    point.z / coordinateScale - face.p0.z / coordinateScale);
    if (!isFinite(scaledDelta))
        return false;

    const double localMajor = scaledDelta.x * unitMajor.x
        + scaledDelta.y * unitMajor.y + scaledDelta.z * unitMajor.z;
    const double localMinor = scaledDelta.x * unitMinor.x
        + scaledDelta.y * unitMinor.y + scaledDelta.z * unitMinor.z;
    const double localAxial = scaledDelta.x * unitAxis.x
        + scaledDelta.y * unitAxis.y + scaledDelta.z * unitAxis.z;
    const double scaledRadius = radius / coordinateScale;
    const double coneSlope = face.cosineAngle / face.sineAngle;
    if (!std::isfinite(localMajor) || !std::isfinite(localMinor)
        || !std::isfinite(localAxial) || scaledRadius == 0.0
        || !std::isfinite(coneSlope))
        return false;

    const double apexAxial = scaledRadius * coneSlope;
    if (!std::isfinite(apexAxial))
        return false;
    const double relativeMajor = localMajor;
    const double relativeMinor = localMinor;
    const double relativeAxial = localAxial + apexAxial;
    const double geometryScale = std::max(
        scaledRadius,
        std::max(std::fabs(relativeMajor),
                 std::max(std::fabs(relativeMinor),
                          std::fabs(relativeAxial))));
    if (geometryScale == 0.0 || !std::isfinite(geometryScale))
        return false;

    const double queryMajor = relativeMajor / geometryScale;
    const double queryMinor = relativeMinor / geometryScale;
    const double queryAxial = relativeAxial / geometryScale;
    const double normalizedRadius = scaledRadius / geometryScale;
    const double directionScale = std::max(
        1.0, std::max(face.ratio, std::fabs(coneSlope)));
    const double majorDirection = 1.0 / directionScale;
    const double minorDirection = face.ratio / directionScale;
    const double axialDirection = coneSlope / directionScale;
    if (!std::isfinite(queryMajor) || !std::isfinite(queryMinor)
        || !std::isfinite(queryAxial) || normalizedRadius == 0.0
        || majorDirection == 0.0 || minorDirection == 0.0
        || axialDirection == 0.0)
        return false;

    if (queryMajor == 0.0 && queryMinor == 0.0 && queryAxial == 0.0) {
        projectionUnique = true;
        distanceScale = coordinateScale;
        normalizedDistance = 0.0;
        if (projectedPoint) {
            const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
            const double axis[3] = {unitAxis.x, unitAxis.y, unitAxis.z};
            double coordinates[3] = {0.0, 0.0, 0.0};
            for (std::size_t coordinate = 0u; coordinate < 3u;
                 ++coordinate) {
                std::array<ScaledAcisProduct, 6> terms;
                if (!scaledAcisProduct5(
                        coordinateScale, geometryScale, normalizedRadius,
                        coneSlope, -axis[coordinate], terms[0])
                    || !scaledAcisProduct5(origin[coordinate], 1.0, 1.0,
                                           1.0, 1.0, terms[1])
                    || !finiteScaledAcisSum6(terms,
                                             coordinates[coordinate]))
                    return false;
            }
            const DRW_Coord candidate(coordinates[0], coordinates[1],
                                      coordinates[2]);
            if (!isFinite(candidate))
                return false;
            *projectedPoint = candidate;
        }
        return true;
    }

    const double majorSquared = majorDirection * majorDirection;
    const double minorSquared = minorDirection * minorDirection;
    const double axialSquared = axialDirection * axialDirection;
    const double denominatorConstant = majorSquared + axialSquared;
    const double denominatorQuadratic = 2.0
        * (axialSquared + 2.0 * minorSquared - majorSquared);
    if (majorSquared == 0.0 || minorSquared == 0.0 || axialSquared == 0.0
        || denominatorConstant == 0.0
        || !std::isfinite(denominatorConstant)
        || !std::isfinite(denominatorQuadratic))
        return false;

    const double linearMajor = majorDirection * queryMajor;
    const double linearMinor = minorDirection * queryMinor;
    const double linearAxial = axialDirection * queryAxial;
    double bestDistance = std::numeric_limits<double>::infinity();
    AcisEllipticConeCandidate best;
    projectionUnique = true;
    bool sawCandidate = false;
    bool stationaryPolynomialFlat = false;

    const auto considerAngle = [&](double angle) noexcept {
        const double cosine = std::cos(angle);
        const double sine = std::sin(angle);
        const double directionMajor = majorDirection * cosine;
        const double directionMinor = minorDirection * sine;
        const double denominator = directionMajor * directionMajor
            + directionMinor * directionMinor + axialSquared;
        const double numerator = linearMajor * cosine
            + linearMinor * sine + linearAxial;
        if (!std::isfinite(denominator) || denominator == 0.0
            || !std::isfinite(numerator))
            return false;
        const double parameter = numerator / denominator;
        if (!std::isfinite(parameter))
            return false;
        const double closestMajor = parameter * directionMajor;
        const double closestMinor = parameter * directionMinor;
        const double closestAxial = parameter * axialDirection;
        const double residualMajor = queryMajor - closestMajor;
        const double residualMinor = queryMinor - closestMinor;
        const double residualAxial = queryAxial - closestAxial;
        const double distance = std::hypot(
            std::hypot(residualMajor, residualMinor), residualAxial);
        if (!std::isfinite(closestMajor) || !std::isfinite(closestMinor)
            || !std::isfinite(closestAxial) || !std::isfinite(distance))
            return false;

        AcisEllipticConeCandidate candidate;
        candidate.localMajor = closestMajor;
        candidate.localMinor = closestMinor;
        candidate.localAxial = closestAxial
            - normalizedRadius * coneSlope;
        candidate.distance = distance;
        candidate.valid = true;
        if (!sawCandidate) {
            best = candidate;
            bestDistance = distance;
            sawCandidate = true;
            return true;
        }

        const double comparisonScale = std::max(
            1.0, std::max(bestDistance, distance));
        const double tieBound = 256.0
            * std::numeric_limits<double>::epsilon() * comparisonScale;
        if (distance < bestDistance - tieBound) {
            best = candidate;
            bestDistance = distance;
            projectionUnique = true;
            return true;
        }
        if (std::fabs(distance - bestDistance) <= tieBound) {
            const double pointDifference = std::hypot(
                std::hypot(candidate.localMajor - best.localMajor,
                           candidate.localMinor - best.localMinor),
                candidate.localAxial - best.localAxial);
            const double pointScale = std::max(
                1.0, std::max(
                    std::hypot(std::hypot(best.localMajor, best.localMinor),
                               best.localAxial),
                    std::hypot(std::hypot(candidate.localMajor,
                                          candidate.localMinor),
                               candidate.localAxial)));
            if (pointDifference > 512.0
                    * std::numeric_limits<double>::epsilon() * pointScale)
                projectionUnique = false;
        }
        return true;
    };

    for (unsigned int chart = 0u; chart < 2u; ++chart) {
        const double sign = chart == 0u ? 1.0 : -1.0;
        const double chartMajor = sign * linearMajor;
        const double chartMinor = sign * linearMinor;
        const double n0 = chartMajor + linearAxial;
        const double n1 = 2.0 * chartMinor;
        const double n2 = linearAxial - chartMajor;
        const double p0 = 2.0 * n1 * denominatorConstant;
        const double p1 = 4.0 * n2 * denominatorConstant
            - 2.0 * n0 * denominatorQuadratic;
        const double p3 = 2.0 * n2 * denominatorQuadratic
            - 4.0 * n0 * denominatorConstant;
        const double p4 = -2.0 * n1 * denominatorConstant;
        const std::array<double, 5> stationary = {{p0, p1, 0.0, p3, p4}};
        AcisPolynomialRootSet roots;
        if (!acisPolynomialRootsInUnitInterval(stationary, 4u, roots))
            return false;
        stationaryPolynomialFlat = stationaryPolynomialFlat
            || roots.identicallyZero;
        for (std::size_t root = 0u; root < roots.count; ++root) {
            const double angle = (chart == 0u ? 0.0 : 3.14159265358979323846)
                + 2.0 * std::atan(roots.values[root]);
            if (!considerAngle(angle))
                return false;
        }
        const double chartCenter = chart == 0u ? 0.0
            : 3.14159265358979323846;
        if (!considerAngle(chartCenter - 1.57079632679489661923)
            || !considerAngle(chartCenter + 1.57079632679489661923))
            return false;
    }

    if (!sawCandidate)
        return false;
    if (stationaryPolynomialFlat
        && (queryMajor != 0.0 || queryMinor != 0.0 || queryAxial != 0.0))
        projectionUnique = false;

    const double scaledDistance = geometryScale * bestDistance;
    if (!std::isfinite(scaledDistance)
        || (bestDistance != 0.0 && scaledDistance == 0.0))
        return false;
    distanceScale = coordinateScale;
    normalizedDistance = scaledDistance;
    if (!projectedPoint || !projectionUnique)
        return true;

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double major[3] = {unitMajor.x, unitMajor.y, unitMajor.z};
    const double minor[3] = {unitMinor.x, unitMinor.y, unitMinor.z};
    const double axis[3] = {unitAxis.x, unitAxis.y, unitAxis.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0, 1.0,
                                terms[0])
            || !scaledAcisProduct5(coordinateScale, geometryScale,
                                   best.localMajor, major[coordinate], 1.0,
                                   terms[1])
            || !scaledAcisProduct5(coordinateScale, geometryScale,
                                   best.localMinor, minor[coordinate], 1.0,
                                   terms[2])
            || !scaledAcisProduct5(coordinateScale, geometryScale,
                                   best.localAxial, axis[coordinate], 1.0,
                                   terms[3])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }
    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    *projectedPoint = candidatePoint;
    return true;
}

bool acisEllipticCylinderClosestPoint(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, DRW_Coord* projectedPoint,
    bool& projectionUnique, double& distanceScale,
    double& normalizedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(frameTolerance)
        || frameTolerance < 0.0 || face.ratio == 1.0
        || face.sineAngle != 0.0 || face.cosineAngle != 1.0)
        return false;

    DRW_Coord unitAxis;
    DRW_Coord unitMajor;
    DRW_Coord unitMinor;
    DRW_Coord scaledProjectedMajor;
    double majorScale = 0.0;
    double projectedMajorLength = 0.0;
    if (!acisConeSurfaceFrame(face, frameTolerance, unitAxis, unitMajor,
                              unitMinor, scaledProjectedMajor, majorScale,
                              projectedMajorLength))
        return false;

    double radius = 0.0;
    if (!finiteProductQuotient3(majorScale, projectedMajorLength, 1.0,
                                1.0, 1.0, radius)
        || radius <= 0.0)
        return false;
    const double coordinateScale = std::max(
        std::max(std::fabs(point.x),
                 std::max(std::fabs(point.y), std::fabs(point.z))),
        std::max(
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))), radius));
    if (coordinateScale == 0.0 || !std::isfinite(coordinateScale))
        return false;

    const DRW_Coord delta(point.x - face.p0.x,
                          point.y - face.p0.y,
                          point.z - face.p0.z);
    const DRW_Coord scaledDelta = isFinite(delta)
        ? DRW_Coord(delta.x / coordinateScale,
                    delta.y / coordinateScale,
                    delta.z / coordinateScale)
        : DRW_Coord(point.x / coordinateScale - face.p0.x / coordinateScale,
                    point.y / coordinateScale - face.p0.y / coordinateScale,
                    point.z / coordinateScale - face.p0.z / coordinateScale);
    if (!isFinite(scaledDelta))
        return false;
    const double axialFraction = scaledDelta.x * unitAxis.x
        + scaledDelta.y * unitAxis.y + scaledDelta.z * unitAxis.z;
    if (!std::isfinite(axialFraction))
        return false;

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double axis[3] = {unitAxis.x, unitAxis.y, unitAxis.z};
    DRW_AcisEdge ellipse;
    ellipse.hasCurve = true;
    ellipse.curveType = DRW_AcisCurve::Ellipse;
    ellipse.p1 = face.p1;
    ellipse.p2 = face.p2;
    ellipse.ratio = face.ratio;
    double centerCoordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(coordinateScale, axialFraction,
                                axis[coordinate], 1.0, 1.0, terms[0])
            || !scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0,
                                   1.0, terms[1])
            || !finiteScaledAcisSum6(terms,
                                     centerCoordinates[coordinate]))
            return false;
    }
    ellipse.p0 = DRW_Coord(centerCoordinates[0], centerCoordinates[1],
                           centerCoordinates[2]);
    if (!isFinite(ellipse.p0))
        return false;

    DRW_Coord candidatePoint;
    if (!projectPointToAcisEllipseCurveImpl(
            ellipse, point, frameTolerance, projectedPoint == nullptr,
            candidatePoint))
        return false;
    const DRW_Coord pointDelta(point.x - candidatePoint.x,
                               point.y - candidatePoint.y,
                               point.z - candidatePoint.z);
    if (isFinite(pointDelta)) {
        distanceScale = std::max(
            std::fabs(pointDelta.x),
            std::max(std::fabs(pointDelta.y), std::fabs(pointDelta.z)));
        if (distanceScale == 0.0) {
            distanceScale = 1.0;
            normalizedDistance = 0.0;
        } else {
            normalizedDistance = std::hypot(
                std::hypot(pointDelta.x / distanceScale,
                           pointDelta.y / distanceScale),
                pointDelta.z / distanceScale);
        }
    } else {
        distanceScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(
                std::max(std::fabs(candidatePoint.x),
                         std::max(std::fabs(candidatePoint.y),
                                  std::fabs(candidatePoint.z))), 1.0));
        const DRW_Coord scaledDifference(
            point.x / distanceScale - candidatePoint.x / distanceScale,
            point.y / distanceScale - candidatePoint.y / distanceScale,
            point.z / distanceScale - candidatePoint.z / distanceScale);
        normalizedDistance = std::hypot(
            std::hypot(scaledDifference.x, scaledDifference.y),
            scaledDifference.z);
    }
    if (!std::isfinite(distanceScale) || !std::isfinite(normalizedDistance)
        || (normalizedDistance != 0.0 && distanceScale == 0.0))
        return false;

    projectionUnique = true;
    if (projectedPoint)
        *projectedPoint = candidatePoint;
    return true;
}

} // namespace

bool drw_projectPointToAcisConeSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, DRW_Coord& projectedPoint) noexcept {
    DRW_Coord candidatePoint;
    bool projectionUnique = false;
    double distanceScale = 0.0;
    double normalizedDistance = 0.0;
    const bool accepted = face.ratio == 1.0
        ? acisCircularConeClosestPoint(
              face, point, frameTolerance, &candidatePoint,
              projectionUnique, distanceScale, normalizedDistance)
        : face.sineAngle == 0.0
        ? acisEllipticCylinderClosestPoint(
              face, point, frameTolerance, &candidatePoint,
              projectionUnique, distanceScale, normalizedDistance)
        : acisEllipticConeClosestPoint(
              face, point, frameTolerance, &candidatePoint,
              projectionUnique, distanceScale, normalizedDistance);
    if (!accepted || !projectionUnique)
        return false;
    projectedPoint = candidatePoint;
    return true;
}

bool drw_pointWithinToleranceOfAcisConeSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double frameTolerance, double linearTolerance) noexcept {
    if (!std::isfinite(linearTolerance) || linearTolerance < 0.0)
        return false;
    bool projectionUnique = false;
    double distanceScale = 0.0;
    double normalizedDistance = 0.0;
    const bool accepted = face.ratio == 1.0
        ? acisCircularConeClosestPoint(
              face, point, frameTolerance, nullptr, projectionUnique,
              distanceScale, normalizedDistance)
        : face.sineAngle == 0.0
        ? acisEllipticCylinderClosestPoint(
              face, point, frameTolerance, nullptr, projectionUnique,
              distanceScale, normalizedDistance)
        : acisEllipticConeClosestPoint(
              face, point, frameTolerance, nullptr, projectionUnique,
              distanceScale, normalizedDistance);
    if (!accepted)
        return false;
    double distance = 0.0;
    if (!finiteProductQuotient3(distanceScale, normalizedDistance, 1.0,
                                1.0, 1.0, distance))
        return false;
    if (distance == 0.0 && normalizedDistance != 0.0
        && linearTolerance == 0.0)
        return false;
    return distance <= linearTolerance;
}

bool drw_projectPointToAcisCylinderSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept {
    if (!isFinite(point))
        return false;

    DRW_Coord unitAxis;
    double radius = 0.0;
    if (!acisCylinderSurfaceFrame(face, unitAxis, radius))
        return false;

    DRW_Coord candidatePoint;
    if (!acisProjectPointToCylinder(face, point, unitAxis, radius,
                                    candidatePoint))
        return false;

    projectedPoint = candidatePoint;
    return true;
}

bool drw_pointWithinToleranceOfAcisCylinderSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(linearTolerance)
        || linearTolerance < 0.0)
        return false;

    DRW_Coord unitAxis;
    double radius = 0.0;
    if (!acisCylinderSurfaceFrame(face, unitAxis, radius))
        return false;

    double candidateDistance = 0.0;
    if (!acisCylinderSignedDistance(face, point, unitAxis, radius,
                                    candidateDistance)
        || std::fabs(candidateDistance) > linearTolerance)
        return false;

    signedDistance = candidateDistance;
    return true;
}

bool drw_pointWithinToleranceOfAcisTorusSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept {
    if (!isFinite(point) || !std::isfinite(linearTolerance)
        || linearTolerance < 0.0)
        return false;

    DRW_Coord unitAxis;
    if (!acisTorusUnitAxis(face, unitAxis))
        return false;

    double candidateDistance = 0.0;
    if (!acisTorusSignedDistance(face, point, unitAxis, candidateDistance)
        || std::fabs(candidateDistance) > linearTolerance)
        return false;

    signedDistance = candidateDistance;
    return true;
}

bool drw_projectPointToAcisTorusSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept {
    if (!isFinite(point))
        return false;

    DRW_Coord unitAxis;
    if (!acisTorusUnitAxis(face, unitAxis))
        return false;

    DRW_Coord candidatePoint;
    if (!acisProjectPointToTorus(face, point, unitAxis, candidatePoint))
        return false;

    projectedPoint = candidatePoint;
    return true;
}

bool drw_evaluateAcisTorusSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept {
    if (!std::isfinite(uParameter) || !std::isfinite(vParameter)
        || !std::isfinite(frameTolerance) || frameTolerance < 0.0
        || !face.hasUDir || !isFinite(face.p2))
        return false;

    DRW_Coord unitAxis;
    if (!acisTorusUnitAxis(face, unitAxis))
        return false;

    const double majorDirectionScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (majorDirectionScale == 0.0)
        return false;
    const DRW_Coord scaledMajorDirection(
        face.p2.x / majorDirectionScale,
        face.p2.y / majorDirectionScale,
        face.p2.z / majorDirectionScale);
    const double majorDirectionLength = std::sqrt(
        scaledMajorDirection.x * scaledMajorDirection.x
        + scaledMajorDirection.y * scaledMajorDirection.y
        + scaledMajorDirection.z * scaledMajorDirection.z);
    if (!std::isfinite(majorDirectionLength)
        || majorDirectionLength == 0.0)
        return false;
    const double axialComponent = scaledMajorDirection.x * unitAxis.x
        + scaledMajorDirection.y * unitAxis.y
        + scaledMajorDirection.z * unitAxis.z;
    if (!std::isfinite(axialComponent)
        || std::fabs(axialComponent) / majorDirectionLength
            > frameTolerance)
        return false;

    const DRW_Coord projectedMajorDirection(
        scaledMajorDirection.x - axialComponent * unitAxis.x,
        scaledMajorDirection.y - axialComponent * unitAxis.y,
        scaledMajorDirection.z - axialComponent * unitAxis.z);
    const double projectedMajorLength = std::sqrt(
        projectedMajorDirection.x * projectedMajorDirection.x
        + projectedMajorDirection.y * projectedMajorDirection.y
        + projectedMajorDirection.z * projectedMajorDirection.z);
    if (!std::isfinite(projectedMajorLength)
        || projectedMajorLength == 0.0)
        return false;
    const DRW_Coord unitMajor(
        projectedMajorDirection.x / projectedMajorLength,
        projectedMajorDirection.y / projectedMajorLength,
        projectedMajorDirection.z / projectedMajorLength);
    DRW_Coord crossMinor(
        unitAxis.y * unitMajor.z - unitAxis.z * unitMajor.y,
        unitAxis.z * unitMajor.x - unitAxis.x * unitMajor.z,
        unitAxis.x * unitMajor.y - unitAxis.y * unitMajor.x);
    if (!isFinite(crossMinor)
        || (crossMinor.x == 0.0 && crossMinor.y == 0.0
            && crossMinor.z == 0.0))
        return false;
    const DRW_Coord unitMinor(crossMinor.x, crossMinor.y, crossMinor.z);

    const double cosineU = std::cos(uParameter);
    const double sineU = std::sin(uParameter);
    const double cosineV = std::cos(vParameter);
    const double sineV = std::sin(vParameter);
    if (!std::isfinite(cosineU) || !std::isfinite(sineU)
        || !std::isfinite(cosineV) || !std::isfinite(sineV))
        return false;

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double majorComponents[3] = {
        unitMajor.x, unitMajor.y, unitMajor.z};
    const double minorComponents[3] = {
        unitMinor.x, unitMinor.y, unitMinor.z};
    const double axisComponents[3] = {
        unitAxis.x, unitAxis.y, unitAxis.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(face.majorRadius, cosineU,
                                majorComponents[coordinate], 1.0, 1.0,
                                terms[0])
            || !scaledAcisProduct5(face.majorRadius, sineU,
                                   minorComponents[coordinate], 1.0, 1.0,
                                   terms[1])
            || !scaledAcisProduct5(face.minorRadius, cosineV, cosineU,
                                   majorComponents[coordinate], 1.0,
                                   terms[2])
            || !scaledAcisProduct5(face.minorRadius, cosineV, sineU,
                                   minorComponents[coordinate], 1.0,
                                   terms[3])
            || !scaledAcisProduct5(face.minorRadius, sineV,
                                   axisComponents[coordinate], 1.0, 1.0,
                                   terms[4])
            || !scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0,
                                   1.0, terms[5])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }

    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    point = candidatePoint;
    return true;
}

bool drw_evaluateAcisTorusFaceNormal(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& faceNormal) noexcept {
    if (!face.hasSense || !face.hasUDir
        || !std::isfinite(uParameter) || !std::isfinite(vParameter)
        || !std::isfinite(frameTolerance) || frameTolerance < 0.0
        || !isFinite(face.p2))
        return false;

    DRW_Coord unitAxis;
    if (!acisTorusUnitAxis(face, unitAxis))
        return false;

    const double majorDirectionScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (majorDirectionScale == 0.0)
        return false;
    const DRW_Coord scaledMajorDirection(
        face.p2.x / majorDirectionScale,
        face.p2.y / majorDirectionScale,
        face.p2.z / majorDirectionScale);
    const double majorDirectionLength = std::sqrt(
        scaledMajorDirection.x * scaledMajorDirection.x
        + scaledMajorDirection.y * scaledMajorDirection.y
        + scaledMajorDirection.z * scaledMajorDirection.z);
    if (!std::isfinite(majorDirectionLength)
        || majorDirectionLength == 0.0)
        return false;
    const double axialComponent = scaledMajorDirection.x * unitAxis.x
        + scaledMajorDirection.y * unitAxis.y
        + scaledMajorDirection.z * unitAxis.z;
    if (!std::isfinite(axialComponent)
        || std::fabs(axialComponent) / majorDirectionLength
            > frameTolerance)
        return false;

    const DRW_Coord projectedMajorDirection(
        scaledMajorDirection.x - axialComponent * unitAxis.x,
        scaledMajorDirection.y - axialComponent * unitAxis.y,
        scaledMajorDirection.z - axialComponent * unitAxis.z);
    const double projectedMajorLength = std::sqrt(
        projectedMajorDirection.x * projectedMajorDirection.x
        + projectedMajorDirection.y * projectedMajorDirection.y
        + projectedMajorDirection.z * projectedMajorDirection.z);
    if (!std::isfinite(projectedMajorLength)
        || projectedMajorLength == 0.0)
        return false;
    const DRW_Coord unitMajor(
        projectedMajorDirection.x / projectedMajorLength,
        projectedMajorDirection.y / projectedMajorLength,
        projectedMajorDirection.z / projectedMajorLength);
    const DRW_Coord unitMinor(
        unitAxis.y * unitMajor.z - unitAxis.z * unitMajor.y,
        unitAxis.z * unitMajor.x - unitAxis.x * unitMajor.z,
        unitAxis.x * unitMajor.y - unitAxis.y * unitMajor.x);
    if (!isFinite(unitMinor)
        || (unitMinor.x == 0.0 && unitMinor.y == 0.0
            && unitMinor.z == 0.0))
        return false;

    const double cosineU = std::cos(uParameter);
    const double sineU = std::sin(uParameter);
    const double cosineV = std::cos(vParameter);
    const double sineV = std::sin(vParameter);
    if (!std::isfinite(cosineU) || !std::isfinite(sineU)
        || !std::isfinite(cosineV) || !std::isfinite(sineV))
        return false;

    std::array<ScaledAcisProduct, 6> radialTerms;
    double radialScale = 0.0;
    if (!scaledAcisProduct5(face.majorRadius, 1.0, 1.0, 1.0, 1.0,
                            radialTerms[0])
        || !scaledAcisProduct5(face.minorRadius, cosineV, 1.0, 1.0, 1.0,
                               radialTerms[1])
        || !finiteScaledAcisSum6(radialTerms, radialScale)
        || radialScale == 0.0)
        return false;

    const double parameterDirection = radialScale < 0.0 ? -1.0 : 1.0;
    const double radialMajor = parameterDirection * cosineV * cosineU;
    const double radialMinor = parameterDirection * cosineV * sineU;
    const double axial = parameterDirection * sineV;
    const DRW_Coord surfaceNormal(
        radialMajor * unitMajor.x + radialMinor * unitMinor.x
            + axial * unitAxis.x,
        radialMajor * unitMajor.y + radialMinor * unitMinor.y
            + axial * unitAxis.y,
        radialMajor * unitMajor.z + radialMinor * unitMinor.z
            + axial * unitAxis.z);
    if (!isFinite(surfaceNormal))
        return false;

    return drw_orientAcisFaceNormal(face, surfaceNormal, faceNormal);
}

bool drw_pointWithinToleranceOfAcisSphereSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, double& signedDistance) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Sphere
        || !isFinite(face.p0) || !isFinite(point)
        || !std::isfinite(face.majorRadius)
        || !std::isfinite(linearTolerance) || linearTolerance < 0.0)
        return false;
    const double radius = std::fabs(face.majorRadius);

    const DRW_Coord delta(point.x - face.p0.x, point.y - face.p0.y,
                          point.z - face.p0.z);
    double distanceScale = 0.0;
    double normalizedDistance = 0.0;
    if (isFinite(delta)) {
        distanceScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (distanceScale != 0.0) {
            const double x = delta.x / distanceScale;
            const double y = delta.y / distanceScale;
            const double z = delta.z / distanceScale;
            normalizedDistance = std::sqrt(x * x + y * y + z * z);
        }
    } else {
        distanceScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))));
        if (distanceScale == 0.0)
            return false;
        const DRW_Coord scaledDelta(
            point.x / distanceScale - face.p0.x / distanceScale,
            point.y / distanceScale - face.p0.y / distanceScale,
            point.z / distanceScale - face.p0.z / distanceScale);
        const double deltaScale = std::max(
            std::fabs(scaledDelta.x),
            std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
        if (deltaScale == 0.0)
            return false;
        const double x = scaledDelta.x / deltaScale;
        const double y = scaledDelta.y / deltaScale;
        const double z = scaledDelta.z / deltaScale;
        normalizedDistance = deltaScale * std::sqrt(x * x + y * y + z * z);
    }

    double candidateDistance = 0.0;
    double radialDistance = 0.0;
    if (finiteProductQuotient3(distanceScale, normalizedDistance, 1.0,
                               1.0, 1.0, radialDistance)) {
        candidateDistance = radialDistance - radius;
    } else {
        const double referenceScale = std::max(distanceScale, radius);
        const double normalizedSignedDistance =
            (distanceScale / referenceScale) * normalizedDistance
            - radius / referenceScale;
        if (!finiteProductQuotient3(referenceScale,
                                    normalizedSignedDistance, 1.0,
                                    1.0, 1.0, candidateDistance))
            return false;
        if (candidateDistance == 0.0
            && normalizedSignedDistance != 0.0 && linearTolerance == 0.0)
            return false;
    }
    if (!std::isfinite(candidateDistance))
        return false;
    if (std::fabs(candidateDistance) > linearTolerance)
        return false;

    signedDistance = candidateDistance;
    return true;
}

bool drw_projectPointToAcisSphereSurface(
    const DRW_AcisFace& face, const DRW_Coord& point,
    DRW_Coord& projectedPoint) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Sphere
        || !isFinite(face.p0) || !isFinite(point)
        || !std::isfinite(face.majorRadius))
        return false;
    const double radius = std::fabs(face.majorRadius);
    if (radius == 0.0) {
        projectedPoint = face.p0;
        return true;
    }

    const DRW_Coord delta(point.x - face.p0.x, point.y - face.p0.y,
                          point.z - face.p0.z);
    DRW_Coord normalizedDelta;
    if (isFinite(delta)) {
        const double deltaScale = std::max(
            std::fabs(delta.x),
            std::max(std::fabs(delta.y), std::fabs(delta.z)));
        if (deltaScale == 0.0)
            return false;
        normalizedDelta = DRW_Coord(delta.x / deltaScale,
                                     delta.y / deltaScale,
                                     delta.z / deltaScale);
    } else {
        const double coordinateScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))));
        if (coordinateScale == 0.0)
            return false;
        const DRW_Coord scaledDelta(
            point.x / coordinateScale - face.p0.x / coordinateScale,
            point.y / coordinateScale - face.p0.y / coordinateScale,
            point.z / coordinateScale - face.p0.z / coordinateScale);
        const double deltaScale = std::max(
            std::fabs(scaledDelta.x),
            std::max(std::fabs(scaledDelta.y), std::fabs(scaledDelta.z)));
        if (deltaScale == 0.0)
            return false;
        normalizedDelta = DRW_Coord(scaledDelta.x / deltaScale,
                                    scaledDelta.y / deltaScale,
                                    scaledDelta.z / deltaScale);
    }

    const double directionLength = std::sqrt(
        normalizedDelta.x * normalizedDelta.x
        + normalizedDelta.y * normalizedDelta.y
        + normalizedDelta.z * normalizedDelta.z);
    if (!std::isfinite(directionLength) || directionLength == 0.0)
        return false;
    const DRW_Coord unitDirection(normalizedDelta.x / directionLength,
                                  normalizedDelta.y / directionLength,
                                  normalizedDelta.z / directionLength);
    const DRW_Coord candidatePoint(
        face.p0.x + radius * unitDirection.x,
        face.p0.y + radius * unitDirection.y,
        face.p0.z + radius * unitDirection.z);
    if (!isFinite(candidatePoint))
        return false;

    projectedPoint = candidatePoint;
    return true;
}

bool drw_evaluateAcisSphereFaceNormal(
    const DRW_AcisFace& face, const DRW_Coord& point,
    double linearTolerance, DRW_Coord& faceNormal) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Sphere
        || !face.hasSense || !isFinite(face.p0) || !isFinite(point)
        || !std::isfinite(face.majorRadius) || face.majorRadius == 0.0
        || !std::isfinite(linearTolerance) || linearTolerance < 0.0)
        return false;

    double signedDistance = 0.0;
    if (!drw_pointWithinToleranceOfAcisSphereSurface(
            face, point, linearTolerance, signedDistance))
        return false;

    DRW_Coord radial(point.x - face.p0.x, point.y - face.p0.y,
                     point.z - face.p0.z);
    if (!isFinite(radial)) {
        const double coordinateScale = std::max(
            std::max(std::fabs(point.x),
                     std::max(std::fabs(point.y), std::fabs(point.z))),
            std::max(std::fabs(face.p0.x),
                     std::max(std::fabs(face.p0.y),
                              std::fabs(face.p0.z))));
        if (coordinateScale == 0.0)
            return false;
        radial = DRW_Coord(
            point.x / coordinateScale - face.p0.x / coordinateScale,
            point.y / coordinateScale - face.p0.y / coordinateScale,
            point.z / coordinateScale - face.p0.z / coordinateScale);
    }

    return drw_orientAcisFaceNormal(face, radial, faceNormal);
}

bool drw_evaluateAcisPlaneSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept {
    if (!std::isfinite(uParameter) || !std::isfinite(vParameter)
        || !std::isfinite(frameTolerance) || frameTolerance < 0.0
        || !face.hasUDir || !isFinite(face.p2))
        return false;

    DRW_Coord unitNormal;
    if (!acisPlaneUnitNormal(face, unitNormal))
        return false;

    const double uDirectionScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (uDirectionScale == 0.0)
        return false;
    const DRW_Coord scaledUDirection(face.p2.x / uDirectionScale,
                                     face.p2.y / uDirectionScale,
                                     face.p2.z / uDirectionScale);
    const double uDirectionLength = std::sqrt(
        scaledUDirection.x * scaledUDirection.x
        + scaledUDirection.y * scaledUDirection.y
        + scaledUDirection.z * scaledUDirection.z);
    if (!std::isfinite(uDirectionLength) || uDirectionLength == 0.0)
        return false;

    const double axialComponent = scaledUDirection.x * unitNormal.x
        + scaledUDirection.y * unitNormal.y
        + scaledUDirection.z * unitNormal.z;
    if (!std::isfinite(axialComponent)
        || std::fabs(axialComponent) / uDirectionLength > frameTolerance)
        return false;

    const DRW_Coord projectedUDirection(
        scaledUDirection.x - axialComponent * unitNormal.x,
        scaledUDirection.y - axialComponent * unitNormal.y,
        scaledUDirection.z - axialComponent * unitNormal.z);
    const double projectedUDirectionLength = std::sqrt(
        projectedUDirection.x * projectedUDirection.x
        + projectedUDirection.y * projectedUDirection.y
        + projectedUDirection.z * projectedUDirection.z);
    if (!std::isfinite(projectedUDirectionLength)
        || projectedUDirectionLength == 0.0)
        return false;
    const DRW_Coord unitU(
        projectedUDirection.x / projectedUDirectionLength,
        projectedUDirection.y / projectedUDirectionLength,
        projectedUDirection.z / projectedUDirectionLength);

    const DRW_Coord crossV(
        unitNormal.y * unitU.z - unitNormal.z * unitU.y,
        unitNormal.z * unitU.x - unitNormal.x * unitU.z,
        unitNormal.x * unitU.y - unitNormal.y * unitU.x);
    if (!isFinite(crossV)
        || (crossV.x == 0.0 && crossV.y == 0.0 && crossV.z == 0.0))
        return false;
    const DRW_Coord unitV(crossV.x, crossV.y, crossV.z);

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double uComponents[3] = {unitU.x, unitU.y, unitU.z};
    const double vComponents[3] = {unitV.x, unitV.y, unitV.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(uParameter, uComponents[coordinate],
                                   1.0, 1.0, 1.0, terms[1])
            || !scaledAcisProduct5(vParameter, vComponents[coordinate],
                                   1.0, 1.0, 1.0, terms[0])
            || !scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0,
                                   1.0, terms[2])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }

    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    point = candidatePoint;
    return true;
}

bool drw_evaluateAcisSphereSurface(
    const DRW_AcisFace& face, double uParameter, double vParameter,
    double frameTolerance, DRW_Coord& point) noexcept {
    if (!face.hasSurface || face.surfaceType != DRW_AcisSurface::Sphere
        || !isFinite(face.p0) || !isFinite(face.p1) || !isFinite(face.p2)
        || !std::isfinite(face.majorRadius)
        || !std::isfinite(uParameter) || !std::isfinite(vParameter)
        || !std::isfinite(frameTolerance) || frameTolerance < 0.0
        || !face.hasUDir)
        return false;

    const double axisScale = std::max(
        std::fabs(face.p1.x),
        std::max(std::fabs(face.p1.y), std::fabs(face.p1.z)));
    if (axisScale == 0.0)
        return false;
    const DRW_Coord scaledAxis(face.p1.x / axisScale,
                               face.p1.y / axisScale,
                               face.p1.z / axisScale);
    const double axisLength = std::sqrt(
        scaledAxis.x * scaledAxis.x + scaledAxis.y * scaledAxis.y
        + scaledAxis.z * scaledAxis.z);
    if (!std::isfinite(axisLength) || axisLength == 0.0)
        return false;
    const DRW_Coord unitAxis(scaledAxis.x / axisLength,
                             scaledAxis.y / axisLength,
                             scaledAxis.z / axisLength);

    const double uDirectionScale = std::max(
        std::fabs(face.p2.x),
        std::max(std::fabs(face.p2.y), std::fabs(face.p2.z)));
    if (uDirectionScale == 0.0)
        return false;
    const DRW_Coord scaledUDirection(face.p2.x / uDirectionScale,
                                     face.p2.y / uDirectionScale,
                                     face.p2.z / uDirectionScale);
    const double uDirectionLength = std::sqrt(
        scaledUDirection.x * scaledUDirection.x
        + scaledUDirection.y * scaledUDirection.y
        + scaledUDirection.z * scaledUDirection.z);
    if (!std::isfinite(uDirectionLength) || uDirectionLength == 0.0)
        return false;
    const double axialComponent = scaledUDirection.x * unitAxis.x
        + scaledUDirection.y * unitAxis.y
        + scaledUDirection.z * unitAxis.z;
    if (!std::isfinite(axialComponent)
        || std::fabs(axialComponent) / uDirectionLength > frameTolerance)
        return false;

    const DRW_Coord projectedUDirection(
        scaledUDirection.x - axialComponent * unitAxis.x,
        scaledUDirection.y - axialComponent * unitAxis.y,
        scaledUDirection.z - axialComponent * unitAxis.z);
    const double projectedUDirectionLength = std::sqrt(
        projectedUDirection.x * projectedUDirection.x
        + projectedUDirection.y * projectedUDirection.y
        + projectedUDirection.z * projectedUDirection.z);
    if (!std::isfinite(projectedUDirectionLength)
        || projectedUDirectionLength == 0.0)
        return false;
    const DRW_Coord unitU(
        projectedUDirection.x / projectedUDirectionLength,
        projectedUDirection.y / projectedUDirectionLength,
        projectedUDirection.z / projectedUDirectionLength);
    const DRW_Coord crossV(
        unitAxis.y * unitU.z - unitAxis.z * unitU.y,
        unitAxis.z * unitU.x - unitAxis.x * unitU.z,
        unitAxis.x * unitU.y - unitAxis.y * unitU.x);
    if (!isFinite(crossV)
        || (crossV.x == 0.0 && crossV.y == 0.0 && crossV.z == 0.0))
        return false;
    const DRW_Coord unitV(crossV.x, crossV.y, crossV.z);

    const double cosineU = std::cos(uParameter);
    const double sineU = std::sin(uParameter);
    const double cosineV = std::cos(vParameter);
    const double sineV = std::sin(vParameter);
    if (!std::isfinite(cosineU) || !std::isfinite(sineU)
        || !std::isfinite(cosineV) || !std::isfinite(sineV))
        return false;

    const double origin[3] = {face.p0.x, face.p0.y, face.p0.z};
    const double uComponents[3] = {unitU.x, unitU.y, unitU.z};
    const double vComponents[3] = {unitV.x, unitV.y, unitV.z};
    const double axisComponents[3] = {
        unitAxis.x, unitAxis.y, unitAxis.z};
    double coordinates[3] = {0.0, 0.0, 0.0};
    for (std::size_t coordinate = 0u; coordinate < 3u; ++coordinate) {
        std::array<ScaledAcisProduct, 6> terms;
        if (!scaledAcisProduct5(face.majorRadius, cosineV, cosineU,
                                uComponents[coordinate], 1.0, terms[0])
            || !scaledAcisProduct5(face.majorRadius, cosineV, sineU,
                                   vComponents[coordinate], 1.0, terms[1])
            || !scaledAcisProduct5(face.majorRadius, sineV,
                                   axisComponents[coordinate], 1.0, 1.0,
                                   terms[2])
            || !scaledAcisProduct5(origin[coordinate], 1.0, 1.0, 1.0,
                                   1.0, terms[3])
            || !finiteScaledAcisSum6(terms, coordinates[coordinate]))
            return false;
    }

    const DRW_Coord candidatePoint(coordinates[0], coordinates[1],
                                   coordinates[2]);
    if (!isFinite(candidatePoint))
        return false;
    point = candidatePoint;
    return true;
}

bool drw_extractAcisWireframeWithReport(
    const DRW_AcisModel& model, DRW_AcisBrep& out,
    DRW_AcisProjectionReport& report) {
    out = DRW_AcisBrep();
    report = DRW_AcisProjectionReport();
    const DRW_SabParseLimits limits;
    SabParseBudget budget(limits);
    try {
        DRW_AcisBrep extracted;
        if (!extractAcisWireframeImpl(model, extracted, budget)
            || !isFiniteProjection(extracted, budget))
            return false;

        DRW_AcisProjectionReport observed;
        observed.modelNodeCount = model.nodes.size();
        observed.vertexCount = extracted.vertices.size();
        observed.edgeCount = extracted.edges.size();
        observed.faceCount = extracted.faces.size();
        for (const DRW_AcisVertex& vertex : extracted.vertices) {
            budget.accountExtractionWork();
            if (!vertex.valid)
                ++observed.verticesWithoutResolvedPoint;
        }
        for (const DRW_AcisEdge& edge : extracted.edges) {
            budget.accountExtractionWork();
            if (!edge.hasStart || !edge.hasEnd)
                ++observed.edgesWithoutResolvedEndpointPair;
            if (!edge.hasCurve)
                ++observed.edgesWithoutCurveFlag;
        }
        for (const DRW_AcisFace& face : extracted.faces) {
            budget.accountExtractionWork();
            if (!face.hasSurface)
                ++observed.facesWithoutSurfaceFlag;
        }
        out = std::move(extracted);
        report = observed;
        return true;
    } catch (...) {
        out = DRW_AcisBrep();
        report = DRW_AcisProjectionReport();
        return false;
    }
}

bool drw_extractAcisBodyWireframes(
    const DRW_AcisModel& model, std::vector<DRW_AcisBodyProjection>& out,
    DRW_AcisRootedProjectionReport& report,
    const DRW_SabParseLimits& limits) {
    out.clear();
    report = DRW_AcisRootedProjectionReport();
    report.failure = DRW_AcisProjectionFailure::InvalidModel;
    SabParseBudget budget(limits);
    const auto fail = [&](DRW_AcisProjectionFailure failure, int nodeIndex,
                          std::size_t pointerOrdinal =
                              std::numeric_limits<std::size_t>::max()) {
        report.failure = failure;
        report.failedNodeIndex = nodeIndex;
        report.failedPointerOrdinal = pointerOrdinal;
    };
    try {
        if (model.header.signature != "ACIS BinaryFile"
            || model.header.version != 21800) {
            fail(DRW_AcisProjectionFailure::UnsupportedProfile, -1);
            return false;
        }
        if (model.nodes.size()
            > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            fail(DRW_AcisProjectionFailure::ResourceLimit, -1);
            return false;
        }

        // The resolved model already owns the ordered pointer vector for
        // every node. Validate its correspondence with the source tokens only
        // once, then reuse that immutable vector throughout the rooted walk.
        // The prior helper rebuilt a temporary vector on every owner check,
        // cycle pass, and body traversal, multiplying both scans and item
        // budget on shared graphs.
        struct PointerRowView {
            const std::vector<int>* refs = nullptr;

            explicit operator bool() const { return refs != nullptr; }
            std::size_t size() const { return refs ? refs->size() : 0u; }
            int operator[](std::size_t ordinal) const {
                return (*refs)[ordinal];
            }
        };
        std::set<int> validatedPointerRows;
        const auto readPointers = [&](int sourceIndex) -> PointerRowView {
            if (sourceIndex < 0
                || static_cast<std::size_t>(sourceIndex) >= model.nodes.size()) {
                fail(DRW_AcisProjectionFailure::InvalidReference, sourceIndex);
                return PointerRowView();
            }
            const DRW_AcisNode& source =
                model.nodes[static_cast<std::size_t>(sourceIndex)];
            if (source.index != sourceIndex
                || source.type != source.record.type) {
                fail(DRW_AcisProjectionFailure::InvalidModel, sourceIndex);
                return PointerRowView();
            }
            if (validatedPointerRows.find(sourceIndex)
                == validatedPointerRows.end()) {
                std::size_t ordinal = 0;
                const std::size_t dataTokenStart =
                    semanticTokenStart(model.header, source.record);
                for (std::size_t tokenIndex =
                         dataTokenStart;
                     tokenIndex < source.record.tokens.size(); ++tokenIndex) {
                    const DRW_SabToken& token =
                        source.record.tokens[tokenIndex];
                    budget.accountExtractionWork();
                    if (token.tag != DRW_SabTag::Pointer)
                        continue;
                    if (ordinal >= source.refs.size()) {
                        fail(DRW_AcisProjectionFailure::InvalidReference,
                             sourceIndex, ordinal);
                        return PointerRowView();
                    }
                    const int resolved = source.refs[ordinal];
                    if (token.ival == -1) {
                        if (resolved != -1) {
                            fail(DRW_AcisProjectionFailure::InvalidReference,
                                 sourceIndex, ordinal);
                            return PointerRowView();
                        }
                    } else if (token.ival < 0
                               || static_cast<unsigned long long>(token.ival)
                                      >= model.nodes.size()
                               || resolved != static_cast<int>(token.ival)) {
                        fail(DRW_AcisProjectionFailure::InvalidReference,
                             sourceIndex, ordinal);
                        return PointerRowView();
                    }
                    ++ordinal;
                }
                if (ordinal != source.refs.size()) {
                    fail(DRW_AcisProjectionFailure::InvalidReference,
                         sourceIndex, ordinal);
                    return PointerRowView();
                }
                if (!admitSetItem(validatedPointerRows, sourceIndex, budget)) {
                    fail(DRW_AcisProjectionFailure::InvalidModel, sourceIndex);
                    return PointerRowView();
                }
            }
            return PointerRowView{&source.refs};
        };

        const auto expectedPointerCount = [](const std::string& type)
            -> std::size_t {
            if (type == "body" || type == "lump") return 4u;
            if (type == "shell" || type == "face") return 6u;
            if (type == "loop") return 4u;
            if (type == "coedge") return 7u;
            if (type == "edge") return 5u;
            if (type == "vertex") return 3u;
            if (type == "point") return 1u;
            if (type == "transform") return 0u;
            if (type == "plane-surface" || type == "cone-surface"
                || type == "torus-surface" || type == "sphere-surface"
                || type == "straight-curve"
                || type == "ellipse-curve" || type == "intcurve-curve")
                return 1u;
            return std::numeric_limits<std::size_t>::max();
        };
        const auto typeMatches = [&](int index, const std::string& expected) {
            if (index < 0 || static_cast<std::size_t>(index) >= model.nodes.size())
                return false;
            const std::string& actual =
                model.nodes[static_cast<std::size_t>(index)].type;
            if (expected == "curve") return endsWith(actual, "curve");
            if (expected == "surface") return endsWith(actual, "surface");
            return actual == expected;
        };
        const auto supportedCurve = [](const std::string& type) {
            return type == "straight-curve" || type == "ellipse-curve"
                || type == "intcurve-curve";
        };
        const auto supportedSurface = [](const std::string& type) {
            return type == "plane-surface" || type == "cone-surface"
                || type == "torus-surface" || type == "sphere-surface";
        };
        const auto ownerMatches = [&](int childIndex, std::size_t ordinal,
                                      int expectedOwner) {
            const PointerRowView childPointers = readPointers(childIndex);
            if (!childPointers)
                return false;
            if (ordinal >= childPointers.size()) {
                fail(DRW_AcisProjectionFailure::InvalidReference,
                     childIndex, ordinal);
                return false;
            }
            const std::size_t expectedCount = expectedPointerCount(
                model.nodes[static_cast<std::size_t>(childIndex)].type);
            if (expectedCount != std::numeric_limits<std::size_t>::max()
                && childPointers.size() != expectedCount) {
                fail(DRW_AcisProjectionFailure::InvalidModel, childIndex);
                return false;
            }
            const int owner = childPointers[ordinal];
            if (owner >= 0 && expectedOwner >= 0
                && owner != expectedOwner) {
                fail(DRW_AcisProjectionFailure::OwnerMismatch, childIndex,
                     ordinal);
                return false;
            }
            return true;
        };

        // Index every parsed lump's exact ordinal-3 body owner once. The
        // source-ordered index is shared by all rooted body projections so a
        // partial model-wide census cannot accidentally match a body chain.
        std::map<int, std::vector<int>> lumpNodeIndicesByBody;
        std::size_t unclassifiedLumpOwnerCount = 0u;
        int firstUnclassifiedLumpOwnerNodeIndex = -1;
        for (std::size_t sourceIndex = 0u;
             sourceIndex < model.nodes.size(); ++sourceIndex) {
            budget.accountExtractionWork();
            const DRW_AcisNode& source = model.nodes[sourceIndex];
            if (source.type != "lump")
                continue;
            const auto markUnclassified = [&]() {
                ++unclassifiedLumpOwnerCount;
                if (firstUnclassifiedLumpOwnerNodeIndex < 0)
                    firstUnclassifiedLumpOwnerNodeIndex =
                        static_cast<int>(sourceIndex);
            };
            if (source.index != static_cast<int>(sourceIndex)
                || source.type != source.record.type
                || source.refs.size() != 4u) {
                markUnclassified();
                continue;
            }
            const int bodyIndex = source.refs[3u];
            const DRW_AcisNode* body = nodeAt(model, bodyIndex);
            if (!body || body->type != "body") {
                markUnclassified();
                continue;
            }
            auto indexed = lumpNodeIndicesByBody.find(bodyIndex);
            if (indexed == lumpNodeIndicesByBody.end()) {
                budget.accountOutputItems(2u); // map entry + vector object
                indexed = lumpNodeIndicesByBody.emplace(
                    bodyIndex, std::vector<int>()).first;
            }
            budget.accountOutputItems();
            indexed->second.push_back(static_cast<int>(sourceIndex));
        }

        DRW_AcisRootedProjectionReport observed;
        std::vector<DRW_AcisBodyProjection> projections;
        std::set<int> reachedByAnyBody;
        int activeNodeIndex = -1;
        for (std::size_t rootOrdinal = 0; rootOrdinal < model.nodes.size();
             ++rootOrdinal) {
            budget.accountExtractionWork();
            const DRW_AcisNode& root = model.nodes[rootOrdinal];
            if (root.type != "body")
                continue;
            const int bodyIndex = static_cast<int>(rootOrdinal);
            activeNodeIndex = bodyIndex;
            if (root.index != bodyIndex || root.type != root.record.type) {
                fail(DRW_AcisProjectionFailure::InvalidModel, bodyIndex);
                return false;
            }

            ++observed.bodyCount;
            DRW_AcisBodyProjection projection;
            projection.bodyNodeIndex = bodyIndex;
            projection.report.bodyRootSelected = true;
            std::set<int> discovered;
            std::set<int> queued;
            std::set<int> unsupportedPatterns;
            std::set<int> unsupportedWires;
            std::set<int> unsupportedSubshells;
            std::set<int> unsupportedPCurves;
            std::set<int> unsupportedCurves;
            std::set<int> unsupportedSurfaces;
            std::set<int> unsupportedLinkRows;
            std::vector<int> pending;
            std::size_t pendingIndex = 0;
            const auto admit = [&](int index, bool traverse) {
                if (index < 0)
                    return true;
                if (static_cast<std::size_t>(index) >= model.nodes.size()) {
                    fail(DRW_AcisProjectionFailure::InvalidReference,
                         activeNodeIndex);
                    return false;
                }
                if (admitSetItem(discovered, index, budget)) {
                    budget.accountOutputItems();
                    projection.reachableNodeIndices.push_back(index);
                    if (!admitSetItem(reachedByAnyBody, index, budget))
                        ++observed.multiplyRootedNodeCount;
                }
                if (traverse && admitSetItem(queued, index, budget)) {
                    budget.accountOutputItems();
                    pending.push_back(index);
                }
                return true;
            };
            const auto addTarget = [&](PointerRowView pointers,
                                       std::size_t ordinal,
                                       const std::string& expected,
                                       bool traverse) {
                if (ordinal >= pointers.size()) {
                    fail(DRW_AcisProjectionFailure::InvalidModel,
                         activeNodeIndex, ordinal);
                    return false;
                }
                const int target = pointers[ordinal];
                if (target < 0)
                    return true;
                if (!typeMatches(target, expected)) {
                    fail(DRW_AcisProjectionFailure::InvalidReference,
                         activeNodeIndex, ordinal);
                    return false;
                }
                return admit(target, traverse);
            };
            const auto mark = [&](int index, std::set<int>& values) {
                if (index >= 0) {
                    admitSetItem(values, index, budget);
                    admitSetItem(unsupportedLinkRows, index, budget);
                }
            };
            const auto checkOwner = [&](int child, std::size_t ordinal,
                                        int owner) {
                return ownerMatches(child, ordinal, owner);
            };

            PointerRowView bodyPointers;
            activeNodeIndex = bodyIndex;
            bodyPointers = readPointers(bodyIndex);
            if (!bodyPointers
                || bodyPointers.size() != 4u || !admit(bodyIndex, true)) {
                if (report.failure == DRW_AcisProjectionFailure::InvalidModel)
                    fail(DRW_AcisProjectionFailure::InvalidModel, bodyIndex);
                return false;
            }
            if (bodyPointers[1] < 0)
                ++observed.bodiesWithoutSupportedLump;
            if (bodyPointers[3] >= 0) {
                if (!typeMatches(bodyPointers[3], "transform")) {
                    fail(DRW_AcisProjectionFailure::InvalidReference,
                         bodyIndex, 3u);
                    return false;
                }
                projection.bodyTransformNodeIndex = bodyPointers[3];
                projection.report.bodyTransformPresent = true;
                const DRW_AcisNode& transformNode = model.nodes[
                    static_cast<std::size_t>(bodyPointers[3])];
                projection.transform.sourceNodeIndex = bodyPointers[3];
                projection.transform.state = DRW_AcisTransformState::NotParsed;
                projection.transform.pointerResolved = true;
                const std::size_t tokenCapacity =
                    budget.tokenCapacityTarget(
                        0u, transformNode.record.tokens.size());
                budget.accountTokenCapacity(0u, tokenCapacity);
                budget.accountOutputItems(
                    transformNode.record.tokens.size());
                projection.transform.rawTokens.reserve(
                    tokenCapacity);
                std::size_t tokenIndex = 0u;
                const std::size_t semanticTokenIndex =
                    semanticTokenStart(model.header, transformNode.record);
                std::size_t coordinateVectorCount = 0u;
                for (const DRW_SabToken& token : transformNode.record.tokens) {
                    budget.accountExtractionWork();
                    budget.accountString(token.sval.size());
                    projection.transform.rawTokens.push_back(token);
                    const bool semanticToken =
                        tokenIndex >= semanticTokenIndex;
                    if (semanticToken
                        && (token.tag == DRW_SabTag::LocationVec
                        || token.tag == DRW_SabTag::DirectionVec)
                        )
                        ++coordinateVectorCount;
                    if (semanticToken
                        && !projection.transform.rawLiteralPresent
                        && token.tag == DRW_SabTag::LiteralStr) {
                        budget.accountString(token.sval.size());
                        projection.transform.rawLiteral = token.sval;
                        projection.transform.rawLiteralPresent = true;
                        projection.transform.rawLiteralTokenIndex =
                            tokenIndex;
                    }
                    ++tokenIndex;
                }
                std::array<double, 12> candidateComponents {};
                const TransformCandidateProfile candidateProfile =
                    projection.transform.rawLiteralPresent
                        ? parseTransformComponents(
                              projection.transform.rawLiteral,
                              candidateComponents, budget)
                        : TransformCandidateProfile::Invalid;
                if (candidateProfile
                    == TransformCandidateProfile::TwelveValues) {
                    projection.transform.candidateComponents =
                        candidateComponents;
                    projection.transform.candidateComponentsPresent = true;
                    projection.transform.state =
                        DRW_AcisTransformState::ParsedUnapplied;
                } else if (coordinateVectorCount >= 3u) {
                    // A structured vector payload can also contain literal
                    // hint strings. Do not mistake its first string for the
                    // numeric long-string profile.
                    projection.transform.state =
                        DRW_AcisTransformState::UnsupportedVariant;
                } else if (candidateProfile
                           == TransformCandidateProfile::ThirteenValues) {
                    projection.transform.state =
                        DRW_AcisTransformState::UnsupportedVariant;
                } else if (projection.transform.rawLiteralPresent) {
                    projection.transform.state = DRW_AcisTransformState::Invalid;
                } else if (!projection.transform.rawTokens.empty()) {
                    // SAB implementations also expose transform data as
                    // coordinate triplets plus scalar/hint tokens. Keep this
                    // bounded decoded shape available without guessing how it
                    // maps to the twelve-value profile.
                    projection.transform.state =
                        DRW_AcisTransformState::UnsupportedVariant;
                } else {
                    projection.transform.state = DRW_AcisTransformState::Invalid;
                }
            }

            while (pendingIndex < pending.size()) {
                budget.accountExtractionWork();
                const int nodeIndex = pending[pendingIndex++];
                activeNodeIndex = nodeIndex;
                // queued admits each source index once before it enters this
                // worklist, so this traversal cannot dequeue duplicates.
                const DRW_AcisNode& node =
                    model.nodes[static_cast<std::size_t>(nodeIndex)];
                if (node.index != nodeIndex || node.type != node.record.type) {
                    fail(DRW_AcisProjectionFailure::InvalidModel, nodeIndex);
                    return false;
                }
                const PointerRowView pointers = readPointers(nodeIndex);
                if (!pointers)
                    return false;
                const std::size_t count = expectedPointerCount(node.type);
                if (count != std::numeric_limits<std::size_t>::max()
                    && pointers.size() != count) {
                    fail(DRW_AcisProjectionFailure::InvalidModel, nodeIndex);
                    return false;
                }
                for (std::size_t ordinal = 0; ordinal < pointers.size(); ++ordinal) {
                    budget.accountOutputItems();
                    projection.graphLinks.push_back(
                        {nodeIndex, ordinal, pointers[ordinal]});
                }

                // ACIS 21800 includes the leading pattern pointer on these
                // supported record families. Pattern semantics stay partial.
                if (node.type != "transform") {
                    if (!addTarget(pointers, 0u, "pattern", false))
                        return false;
                    mark(pointers[0], unsupportedPatterns);
                }

                if (node.type == "body") {
                    if (!addTarget(pointers, 1u, "lump", true)
                        || !addTarget(pointers, 3u, "transform", true))
                        return false;
                    if (pointers[2] >= 0) {
                        if (!addTarget(pointers, 2u, "wire", false))
                            return false;
                        mark(pointers[2], unsupportedWires);
                    }
                } else if (node.type == "lump") {
                    if (pointers[3] >= 0 && pointers[3] != bodyIndex) {
                        fail(DRW_AcisProjectionFailure::OwnerMismatch,
                             nodeIndex, 3u);
                        return false;
                    }
                    if (!addTarget(pointers, 1u, "lump", true)
                        || !addTarget(pointers, 2u, "shell", true))
                        return false;
                    if (pointers[1] >= 0
                        && !checkOwner(pointers[1], 3u, bodyIndex))
                        return false;
                    if (pointers[2] >= 0
                        && !checkOwner(pointers[2], 5u, nodeIndex))
                        return false;
                } else if (node.type == "shell") {
                    if (pointers[5] >= 0) {
                        if (!addTarget(pointers, 5u, "lump", false))
                            return false;
                        const PointerRowView lumpPointers =
                            readPointers(pointers[5]);
                        if (!lumpPointers
                            || lumpPointers.size() != 4u) {
                            if (report.failure
                                == DRW_AcisProjectionFailure::InvalidModel)
                                fail(DRW_AcisProjectionFailure::InvalidModel,
                                     pointers[5]);
                            return false;
                        }
                        if (lumpPointers[3] >= 0
                            && lumpPointers[3] != bodyIndex) {
                            fail(DRW_AcisProjectionFailure::OwnerMismatch,
                                 pointers[5], 3u);
                            return false;
                        }
                    }
                    if (!addTarget(pointers, 1u, "shell", true)
                        || !addTarget(pointers, 3u, "face", true))
                        return false;
                    if (pointers[1] >= 0
                        && !checkOwner(pointers[1], 5u, pointers[5]))
                        return false;
                    if (pointers[3] >= 0
                        && !checkOwner(pointers[3], 3u, nodeIndex))
                        return false;
                    if (pointers[2] >= 0) {
                        if (!addTarget(pointers, 2u, "subshell", false))
                            return false;
                        mark(pointers[2], unsupportedSubshells);
                    }
                    if (pointers[4] >= 0) {
                        if (!addTarget(pointers, 4u, "wire", false))
                            return false;
                        mark(pointers[4], unsupportedWires);
                    }
                } else if (node.type == "face") {
                    if (pointers[3] >= 0) {
                        if (!addTarget(pointers, 3u, "shell", false))
                            return false;
                        const PointerRowView shellPointers =
                            readPointers(pointers[3]);
                        if (!shellPointers
                            || shellPointers.size() != 6u) {
                            if (report.failure
                                == DRW_AcisProjectionFailure::InvalidModel)
                                fail(DRW_AcisProjectionFailure::InvalidModel,
                                     pointers[3]);
                            return false;
                        }
                        if (shellPointers[5] >= 0
                            && !checkOwner(shellPointers[5], 3u, bodyIndex))
                            return false;
                    }
                    if (!addTarget(pointers, 1u, "face", true)
                        || !addTarget(pointers, 2u, "loop", true))
                        return false;
                    if (pointers[1] >= 0
                        && !checkOwner(pointers[1], 3u, pointers[3]))
                        return false;
                    if (pointers[2] >= 0
                        && !checkOwner(pointers[2], 3u, nodeIndex))
                        return false;
                    if (pointers[4] >= 0) {
                        if (!addTarget(pointers, 4u, "subshell", false))
                            return false;
                        mark(pointers[4], unsupportedSubshells);
                    }
                    if (pointers[5] >= 0) {
                        if (!addTarget(pointers, 5u, "surface",
                                       supportedSurface(model.nodes[
                                           static_cast<std::size_t>(
                                               pointers[5])].type)))
                            return false;
                        const std::string& type = model.nodes[
                            static_cast<std::size_t>(pointers[5])].type;
                        if (!supportedSurface(type))
                            mark(pointers[5], unsupportedSurfaces);
                    }
                } else if (node.type == "loop") {
                    if (pointers[3] >= 0
                        && !addTarget(pointers, 3u, "face", false))
                        return false;
                    if (!addTarget(pointers, 1u, "loop", true)
                        || !addTarget(pointers, 2u, "coedge", true))
                        return false;
                    if (pointers[1] >= 0
                        && !checkOwner(pointers[1], 3u, pointers[3]))
                        return false;
                    if (pointers[2] >= 0
                        && !checkOwner(pointers[2], 5u, nodeIndex))
                        return false;
                } else if (node.type == "coedge") {
                    if (pointers[5] >= 0
                        && !addTarget(pointers, 5u, "loop", false))
                        return false;
                    for (std::size_t ordinal : {1u, 2u, 3u}) {
                        if (!addTarget(pointers, ordinal, "coedge", true))
                            return false;
                        if (ordinal != 3u && pointers[ordinal] >= 0
                            && !checkOwner(pointers[ordinal], 5u,
                                           pointers[5]))
                            return false;
                    }
                    if (!addTarget(pointers, 4u, "edge", true))
                        return false;
                    if (pointers[6] >= 0) {
                        if (!addTarget(pointers, 6u, "pcurve", false))
                            return false;
                        mark(pointers[6], unsupportedPCurves);
                    }
                } else if (node.type == "edge") {
                    if (!addTarget(pointers, 1u, "vertex", true)
                        || !addTarget(pointers, 2u, "vertex", true)
                        || !addTarget(pointers, 3u, "coedge", true))
                        return false;
                    if (pointers[3] >= 0
                        && !checkOwner(pointers[3], 4u, nodeIndex))
                        return false;
                    if (pointers[4] >= 0) {
                        if (!addTarget(pointers, 4u, "curve",
                                       supportedCurve(model.nodes[
                                           static_cast<std::size_t>(
                                               pointers[4])].type)))
                            return false;
                        const std::string& type = model.nodes[
                            static_cast<std::size_t>(pointers[4])].type;
                        if (!supportedCurve(type))
                            mark(pointers[4], unsupportedCurves);
                    }
                } else if (node.type == "vertex") {
                    if (!addTarget(pointers, 1u, "edge", true)
                        || !addTarget(pointers, 2u, "point", true))
                        return false;
                } else if (node.type == "point"
                           || node.type == "transform"
                           || node.type == "straight-curve"
                           || node.type == "ellipse-curve"
                           || node.type == "intcurve-curve"
                           || node.type == "plane-surface"
                           || node.type == "cone-surface"
                           || node.type == "torus-surface"
                           || node.type == "sphere-surface") {
                    // Scalar/vector payloads remain source-indexed in model;
                    // this projection applies no transforms or units.
                } else {
                    fail(DRW_AcisProjectionFailure::InvalidModel, nodeIndex);
                    return false;
                }
            }

            // Unsupported nodes stay uninterpreted and are not traversed, but
            // their raw outgoing pointer rows are still part of the retained
            // body subgraph. Preserve each row once, including null slots.
            for (int nodeIndex : unsupportedLinkRows) {
                budget.accountExtractionWork();
                activeNodeIndex = nodeIndex;
                const PointerRowView pointers = readPointers(nodeIndex);
                if (!pointers)
                    return false;
                for (std::size_t ordinal = 0; ordinal < pointers.size();
                     ++ordinal) {
                    budget.accountOutputItems();
                    projection.graphLinks.push_back(
                        {nodeIndex, ordinal, pointers[ordinal]});
                }
            }
            activeNodeIndex = bodyIndex;

            // The ordered worklist and its membership set are traversal-only;
            // release their retained storage before cycle and geometry passes.
            std::set<int>().swap(queued);
            std::vector<int>().swap(pending);

            const auto countFunctionalCycles = [&](const std::string& type,
                                                   std::size_t pointerOrdinal,
                                                   std::size_t& cycleCount) {
                std::set<int> completed;
                for (int start : projection.reachableNodeIndices) {
                    budget.accountExtractionWork();
                    if (model.nodes[static_cast<std::size_t>(start)].type != type
                        || completed.find(start) != completed.end())
                        continue;
                    std::set<int> currentPath;
                    int current = start;
                    while (current >= 0
                           && discovered.find(current) != discovered.end()
                           && model.nodes[static_cast<std::size_t>(current)].type
                                  == type) {
                        budget.accountExtractionWork();
                        if (currentPath.find(current) != currentPath.end()) {
                            ++cycleCount;
                            budget.accountOutputItems();
                            break;
                        }
                        if (completed.find(current) != completed.end())
                            break;
                        budget.accountOutputItems();
                        currentPath.insert(current);
                        const PointerRowView pointers = readPointers(current);
                        if (!pointers
                            || pointerOrdinal >= pointers.size()) {
                            if (report.failure
                                == DRW_AcisProjectionFailure::InvalidModel)
                                fail(DRW_AcisProjectionFailure::InvalidReference,
                                     current, pointerOrdinal);
                            return false;
                        }
                        current = pointers[pointerOrdinal];
                    }
                    // currentPath already contains the unique visited nodes;
                    // replaying a parallel path vector only duplicated their
                    // retained indices before adding them to completed.
                    for (int visited : currentPath) {
                        budget.accountOutputItems();
                        completed.insert(visited);
                    }
                }
                return true;
            };
            for (const std::string& type : {"lump", "shell", "face", "loop"}) {
                if (!countFunctionalCycles(type, 1u,
                                           projection.nextListCycleCount))
                    return false;
            }
            if (!countFunctionalCycles("coedge", 1u,
                                       projection.coedgeNextCycleCount)
                || !countFunctionalCycles("coedge", 3u,
                                          projection.coedgePartnerCycleCount))
                return false;

            DRW_AcisBrep wireframe;
            int invalidSurfaceNodeIndex = -1;
            int invalidCurveNodeIndex = -1;
            int invalidPointNodeIndex = -1;
            if (!extractAcisWireframeImpl(model, wireframe, budget,
                                          &projection.reachableNodeIndices,
                                          &invalidSurfaceNodeIndex,
                                          &invalidCurveNodeIndex,
                                          &invalidPointNodeIndex)
                || !isFiniteProjection(wireframe, budget)) {
                fail(DRW_AcisProjectionFailure::InvalidModel,
                     invalidPointNodeIndex >= 0
                         ? invalidPointNodeIndex
                         : invalidSurfaceNodeIndex >= 0
                             ? invalidSurfaceNodeIndex
                             : invalidCurveNodeIndex >= 0
                                 ? invalidCurveNodeIndex : bodyIndex);
                return false;
            }

            projection.hasLumpChainObservation = true;
            projection.hasLumpUseIndexObservation = true;
            projection.lumpUseIndexCoversParsedLumps =
                unclassifiedLumpOwnerCount == 0u;
            projection.lumpUseIndexUnclassifiedLumpCount =
                unclassifiedLumpOwnerCount;
            projection.firstUnclassifiedLumpOwnerNodeIndex =
                firstUnclassifiedLumpOwnerNodeIndex;
            const auto lumpUses = lumpNodeIndicesByBody.find(bodyIndex);
            if (lumpUses != lumpNodeIndicesByBody.end()) {
                budget.accountOutputItems(lumpUses->second.size());
                projection.lumpUseNodeIndices = lumpUses->second;
            }

            if (!bodyPointers || bodyPointers.size() != 4u) {
                if (report.failure
                    == DRW_AcisProjectionFailure::InvalidModel)
                    fail(DRW_AcisProjectionFailure::InvalidModel, bodyIndex);
                return false;
            }
            int lumpIndex = bodyPointers[1u];
            if (lumpIndex < 0) {
                projection.lumpChainNullTerminated = true;
            } else {
                std::set<int> seenLumps;
                while (lumpIndex >= 0) {
                    budget.accountExtractionWork();
                    if (seenLumps.find(lumpIndex) != seenLumps.end()) {
                        projection.lumpChainCycleDetected = true;
                        projection.firstRepeatedLumpChainNodeIndex =
                            lumpIndex;
                        break;
                    }
                    budget.accountOutputItems(3u);
                    seenLumps.insert(lumpIndex);
                    projection.lumpNodeIndices.push_back(lumpIndex);

                    const PointerRowView lumpPointers =
                        readPointers(lumpIndex);
                    if (!lumpPointers || lumpPointers.size() != 4u) {
                        if (report.failure
                            == DRW_AcisProjectionFailure::InvalidModel)
                            fail(DRW_AcisProjectionFailure::InvalidModel,
                                 lumpIndex);
                        return false;
                    }
                    const int nextLumpIndex = lumpPointers[1u];
                    projection.lumpNextNodeIndices.push_back(nextLumpIndex);
                    if (nextLumpIndex < 0) {
                        projection.lumpChainNullTerminated = true;
                        break;
                    }
                    lumpIndex = nextLumpIndex;
                }
            }
            projection.lumpChainMatchesUseIndex =
                bodyLumpChainMatchesUseIndex(
                    projection.lumpNodeIndices,
                    projection.lumpNextNodeIndices,
                    projection.lumpUseNodeIndices,
                    projection.lumpChainNullTerminated,
                    projection.lumpChainCycleDetected,
                    projection.lumpChainUnclassifiedLinkCount,
                    projection.lumpUseIndexCoversParsedLumps, budget);

            projection.wireframe = std::move(wireframe);
            projection.report.modelNodeCount =
                projection.reachableNodeIndices.size();
            projection.report.vertexCount = projection.wireframe.vertices.size();
            projection.report.edgeCount = projection.wireframe.edges.size();
            projection.report.faceCount = projection.wireframe.faces.size();
            projection.report.unsupportedPatternNodeCount =
                unsupportedPatterns.size();
            projection.report.unsupportedWireNodeCount = unsupportedWires.size();
            projection.report.unsupportedSubshellNodeCount =
                unsupportedSubshells.size();
            projection.report.unsupportedPCurveNodeCount =
                unsupportedPCurves.size();
            projection.report.unsupportedCurveNodeCount =
                unsupportedCurves.size();
            projection.report.unsupportedSurfaceNodeCount =
                unsupportedSurfaces.size();
            for (const DRW_AcisVertex& vertex : projection.wireframe.vertices) {
                budget.accountExtractionWork();
                if (!vertex.valid)
                    ++projection.report.verticesWithoutResolvedPoint;
            }
            for (const DRW_AcisEdge& edge : projection.wireframe.edges) {
                budget.accountExtractionWork();
                if (!edge.hasStart || !edge.hasEnd)
                    ++projection.report.edgesWithoutResolvedEndpointPair;
                if (!edge.hasCurve)
                    ++projection.report.edgesWithoutCurveFlag;
            }
            for (const DRW_AcisFace& face : projection.wireframe.faces) {
                budget.accountExtractionWork();
                if (!face.hasSurface)
                    ++projection.report.facesWithoutSurfaceFlag;
                if (face.loops.empty())
                    ++projection.report.facesWithoutLoop;
            }
            projection.report.bodyTransformsApplied = false;
            projection.report.sourceUnitScaleApplied = false;
            projection.report.brepValidityChecked = false;
            projection.bodyGraphProjected = true;
            budget.accountOutputItems();
            projections.push_back(std::move(projection));
        }

        for (std::size_t index = 0; index < model.nodes.size(); ++index) {
            budget.accountExtractionWork();
            const int nodeIndex = static_cast<int>(index);
            if (reachedByAnyBody.find(nodeIndex) != reachedByAnyBody.end())
                continue;
            const std::string& type = model.nodes[index].type;
            if (type == "vertex")
                ++observed.verticesOutsideBodyRoots;
            else if (type == "edge")
                ++observed.edgesOutsideBodyRoots;
            else if (type == "face")
                ++observed.facesOutsideBodyRoots;
        }
        out = std::move(projections);
        report = observed;
        return true;
    } catch (const SabError& error) {
        out.clear();
        report = DRW_AcisRootedProjectionReport();
        report.failure = error.resourceLimitExceeded
            ? DRW_AcisProjectionFailure::ResourceLimit
            : DRW_AcisProjectionFailure::InvalidModel;
        return false;
    } catch (const std::bad_alloc&) {
        out.clear();
        report = DRW_AcisRootedProjectionReport();
        report.failure = DRW_AcisProjectionFailure::AllocationFailure;
        return false;
    } catch (...) {
        out.clear();
        report = DRW_AcisRootedProjectionReport();
        report.failure = DRW_AcisProjectionFailure::UnexpectedFailure;
        return false;
    }
}

// ── Convenience entry: drw_decodeAcisWireframe ─────────────────────────────

namespace {

const char* const kSignaturesLocal[] = { "ACIS BinaryFile", "ASM BinaryFile4" };

std::uint8_t peekBitAlignedByte(const std::vector<unsigned char>& data,
                                std::size_t index, std::uint8_t bit) {
    if (index >= data.size()) return 0;
    if (bit == 0) return data[index];
    const std::uint8_t next = index + 1 < data.size() ? data[index + 1] : 0;
    return static_cast<std::uint8_t>((data[index] << bit) | (next >> (8 - bit)));
}

bool findSignatureAtBit(const std::vector<unsigned char>& data,
                        std::uint8_t bit, const char* signature,
                        std::size_t& offset) {
    const std::size_t length = std::strlen(signature);
    if (length == 0 || length > data.size())
        return false;

    // KMP keeps signature discovery linear even when untrusted carrier bytes
    // contain long near-matches. The pattern is tiny; this allocation is
    // bounded by the signature length, independent of input size.
    std::vector<std::size_t> prefix(length, 0u);
    for (std::size_t i = 1, matched = 0; i < length; ++i) {
        while (matched > 0 && signature[i] != signature[matched])
            matched = prefix[matched - 1];
        if (signature[i] == signature[matched])
            ++matched;
        prefix[i] = matched;
    }

    std::size_t matched = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        const std::uint8_t byte = peekBitAlignedByte(data, i, bit);
        while (matched > 0
               && byte != static_cast<std::uint8_t>(signature[matched]))
            matched = prefix[matched - 1];
        if (byte == static_cast<std::uint8_t>(signature[matched]))
            ++matched;
        if (matched == length) {
            // i is always within the source buffer, so this also preserves
            // signature matches that end at the final source byte.
            offset = i + 1 - length;
            return true;
        }
    }
    return false;
}

bool findSignature(const std::vector<unsigned char>& data,
                   std::size_t& offset, std::uint8_t& bit) {
    for (const char* sig : kSignaturesLocal) {
        for (std::uint8_t candidateBit = 0; candidateBit < 8; ++candidateBit) {
            if (findSignatureAtBit(data, candidateBit, sig, offset)) {
                bit = candidateBit;
                return true;
            }
        }
    }
    return false;
}

} // namespace

bool drw_decodeAcisWireframeWithOutcome(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabParseOutcome& outcome) {
    const DRW_SabParseLimits limits;
    return drw_decodeAcisWireframeWithLimits(raw, out, outcome, limits);
}

static bool decodeAcisWireframeWithLimitsImpl(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabParseOutcome& outcome, const DRW_SabParseLimits& limits,
    DRW_SabHeader* sourceHeader) {
    out = DRW_AcisBrep();
    if (sourceHeader)
        *sourceHeader = DRW_SabHeader();
    outcome = DRW_SabParseOutcome();
    outcome.sourceByteCount = raw.size();
    if (raw.size() > limits.maxInputBytes) {
        outcome.resourceLimitExceeded = true;
        return false;
    }
    SabParseBudget budget(limits);
    try {
        budget.checkInput(raw.size());
        std::size_t offset = 0;
        std::uint8_t bit = 0;
        if (!findSignature(raw, offset, bit)) return false;
        const std::size_t candidateLength = raw.size() - offset
            - (bit == 0u ? 0u : 1u);
        DRW_SabData sab;
        const bool parsed = parseSabPrefix(
            raw.data() + offset, candidateLength, sab, outcome, budget, bit);
        if (!parsed) {
            return false;
        }
        DRW_AcisModel model;
        if (!buildAcisModelImpl(sab, model, budget)) {
            captureBudget(outcome, budget);
            return false;
        }
        if (!extractAcisWireframeImpl(model, out, budget)
            || !isFiniteProjection(out, budget)) {
            out = DRW_AcisBrep();
            captureBudget(outcome, budget);
            return false;
        }
        captureBudget(outcome, budget);
        if (sourceHeader)
            *sourceHeader = sab.header;
        return true;
    } catch (const SabError& error) {
        out = DRW_AcisBrep();
        captureBudget(outcome, budget);
        outcome.resourceLimitExceeded = error.resourceLimitExceeded;
        return false;
    } catch (const std::bad_alloc&) {
        out = DRW_AcisBrep();
        captureBudget(outcome, budget);
        outcome.allocationFailure = true;
        return false;
    } catch (...) {
        out = DRW_AcisBrep();
        captureBudget(outcome, budget);
        return false;
    }
}

bool drw_decodeAcisWireframeWithLimits(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabParseOutcome& outcome, const DRW_SabParseLimits& limits) {
    return decodeAcisWireframeWithLimitsImpl(raw, out, outcome, limits,
                                             nullptr);
}

bool drw_decodeAcisWireframeWithHeader(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out,
    DRW_SabHeader& header, DRW_SabParseOutcome& outcome,
    const DRW_SabParseLimits& limits) {
    return decodeAcisWireframeWithLimitsImpl(raw, out, outcome, limits,
                                             &header);
}

LIBDXFRW_EXPORT bool drw_decodeAcisWireframe(
    const std::vector<unsigned char>& raw, DRW_AcisBrep& out) {
    DRW_SabParseOutcome outcome;
    if (!drw_decodeAcisWireframeWithOutcome(raw, out, outcome)
        || !outcome.complete) {
        out = DRW_AcisBrep();
        return false;
    }
    return true;
}
