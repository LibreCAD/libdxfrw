// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_dwg_class_evidence.h"
#include <iostream>

namespace wire = dx_class_wire;
namespace evidence = dx_class_evidence;
namespace {
unsigned checks = 0u;
bool expect(bool value, int line) {
    ++checks;
    if (!value) std::cerr << "class join check failed at line " << line << '\n';
    return value;
}
#define CHECK(value) do { if (!expect((value), __LINE__)) return false; } while (false)
struct Pack {
    std::vector<std::uint8_t> bytes;
    std::uint32_t bits{0};
    void put(std::uint32_t value, unsigned width) {
        for (unsigned n = width; n; --n, ++bits) {
            if (bits / 8u == bytes.size()) bytes.push_back(0u);
            bytes[bits / 8u] |= static_cast<std::uint8_t>(((value >> (n - 1u)) & 1u) << (7u - bits % 8u));
        }
    }
    void little(std::uint32_t value, unsigned width) {
        for (unsigned n = 0; n < width; ++n) put((value >> (n * 8u)) & 255u, 8u);
    }
    void bs(std::uint32_t value) {
        if (!value) put(2u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 2u); }
    }
    void bl(std::uint32_t value) {
        if (!value) put(2u, 2u);
        else if (value < 256u) { put(1u, 2u); put(value, 8u); }
        else { put(0u, 2u); little(value, 4u); }
    }
};
std::uint16_t authorCrc(const std::uint8_t* data, std::size_t size) {
    constexpr std::uint16_t table[] = {0x0000,0xCC01,0xD801,0x1400,0xF001,0x3C00,0x2800,0xE401,
                                      0xA001,0x6C00,0x7800,0xB401,0x5000,0x9C01,0x8801,0x4400};
    std::uint16_t crc = 0xC0C1u;
    for (std::size_t n = 0; n < size; ++n) {
        crc ^= data[n];
        for (unsigned k = 0; k < 2u; ++k) crc = static_cast<std::uint16_t>((crc >> 4u) ^ table[crc & 15u]);
    }
    return crc;
}
DRW_DwgClassSourceReport packet(unsigned kind) {
    Pack body;
    body.bs(500u); body.put(0u, 16u); body.put(1u, 1u); // section header
    body.bs(500u); body.bs(0u);
    for (auto text : {'A','C','R'}) { body.bs(1u); body.put(text, 8u); }
    body.put(0u, 1u); body.bs(0x1F3u); body.bl(1u);
    body.bs(kind == 1u ? 1u : 257u);
    body.bs(kind == 0u ? 1024u : kind == 1u ? 2u : 329u);
    // Literal words make the version/maintenance/unknown portion consume the
    // same104 bits under both interpretations, with DIFFERENT values(kind0).
    // This is not
    // a native producer/schema qualification and uses no production writer.
    body.bl(kind == 0u ? 0x04030201u : 0u);
    body.bl(kind == 0u ? 0x06030201u : 0u);
    while (body.bits % 8u) body.put(0u, 1u);
    Pack section;
    for (auto byte : {0x8D,0xA1,0xC4,0xB8,0xC4,0xA9,0xF8,0xC5,0xC0,0xDC,0xF4,0x5F,0xE7,0xCF,0xB6,0x8A})
        section.put(byte, 8u);
    section.little(static_cast<std::uint32_t>(body.bytes.size()), 4u);
    for (auto byte : body.bytes) section.put(byte, 8u);
    section.little(authorCrc(section.bytes.data() + 16u, section.bytes.size() - 16u), 2u);
    for (auto byte : {0x72,0x5E,0x3B,0x47,0x3B,0x56,0x07,0x3A,0x3F,0x23,0x0B,0xA0,0x18,0x30,0x49,0x75})
        section.put(byte, 8u);
    DRW_DwgClassSourceReport source;
    source.m_status = DRW_DwgClassSourceStatus::Captured; source.m_version = DRW::AC1018;
    source.m_sectionDescriptorId = 3; source.m_offsetSpace = DRW_DwgFrameOffsetSpace::DecodedBuffer;
    source.m_classReadCompleted = source.m_parserCoverageComplete = true;
    source.m_byteCount = source.m_declaredSectionSize = section.bytes.size();
    source.m_bytes = std::make_shared<const std::vector<std::uint8_t>>(std::move(section.bytes));
    return source;
}
evidence::Rows metadata(const DRW_DwgClassSourceReport& source, wire::Trailer trailer) {
    wire::SectionProfile profile; profile.revision = 2004u; profile.trailer = trailer;
    const auto section = wire::observeSection(source.m_bytes->data(), source.m_bytes->size(), profile);
    const auto record = wire::scan(section.data, section.strings, 2004u, trailer);
    evidence::Rows rows;
    rows.complete = true; rows.status = DRW_DwgClassCoverageStatus::FinalizedComplete;
    rows.text = {'A','C','R'};
    evidence::Row row;
    row.number = record.number; row.itemClass = record.itemClass;
    row.proxy = record.proxy; row.wasProxy = record.zombie; row.entity = 0;
    row.instances = record.instances; row.version = record.dwgVersion; row.maintenance = record.maintenance;
    row.unknown1 = record.unknown1; row.unknown2 = record.unknown2; row.descriptor = 3;
    row.state = DRW_DwgClassCoverageState::Published;
    row.data = {section.data.begin, record.dataEnd, true, true, DRW_DwgFrameOffsetSpace::DecodedBuffer};
    row.strings = row.data;
    row.names = {{{0u,1u},{1u,1u},{2u,1u}}};
    rows.entries.push_back(row);
    return rows;
}
bool ambiguity() {
    const auto source = packet(0u);
    const auto shorts = metadata(source, wire::Trailer::OdaBitShort);
    const auto longs = metadata(source, wire::Trailer::ProducerBitLong);
    CHECK(shorts.entries[0].version != longs.entries[0].version
        || shorts.entries[0].maintenance != longs.entries[0].maintenance);
    const auto shortMatch = evidence::observe(source, shorts), longMatch = evidence::observe(source, longs);
    CHECK(shortMatch.framedCandidates == 2u && longMatch.framedCandidates == 2u);
    CHECK(shortMatch.matchedCandidates == 1u && longMatch.matchedCandidates == 1u);
    CHECK(shortMatch.matchedMask != longMatch.matchedMask);
    CHECK(shortMatch.state == evidence::State::RowMismatch && longMatch.state == evidence::State::RowMismatch);
    CHECK((shortMatch.profileIssues & evidence::UnselectedSchema) && (longMatch.profileIssues & evidence::UnselectedSchema));
    std::cout << "scratch dual interpretation: BS=" << shorts.entries[0].version << ',' << shorts.entries[0].maintenance
              << ',' << shorts.entries[0].unknown1 << ',' << shorts.entries[0].unknown2
              << " BL=" << longs.entries[0].version << ',' << longs.entries[0].maintenance
              << ',' << longs.entries[0].unknown1 << ',' << longs.entries[0].unknown2 << '\n';
    const auto common = packet(1u);
    const auto agreed = evidence::observe(common, metadata(common, wire::Trailer::OdaBitShort));
    CHECK(agreed.state == evidence::State::Joined && agreed.framedCandidates == 2u
        && agreed.candidateMask == agreed.matchedMask && (agreed.profileIssues & evidence::UnselectedSchema));
    const auto distinct = packet(2u);
    auto rows = metadata(distinct, wire::Trailer::OdaBitShort);
    const auto unique = evidence::observe(distinct, rows);
    CHECK(unique.state == evidence::State::Joined && unique.framedCandidates == 1u);
    rows.entries[0].names[0].offset = rows.text.size() + 1u;
    CHECK(evidence::observe(distinct, rows).state == evidence::State::RowMismatch);
    return true;
}
bool textConventions() {
    const auto compare = [](std::vector<std::uint8_t> bytes, bool unicode, unsigned issues,
                            std::string_view expected) {
        wire::View view{bytes.data(), bytes.size(), 0u, static_cast<std::uint32_t>(bytes.size() * 8u)};
        wire::Text text; text.count = static_cast<std::uint16_t>(bytes.size() / (unicode ? 2u : 1u));
        text.units = {0u, view.end}; text.issues = issues;
        return evidence::nameMatches(view, text, unicode, expected);
    };
    CHECK(compare({'A'}, false, 0u, "A"));
    CHECK(compare({'A',0}, false, wire::EmbeddedNull, "A"));
    CHECK(!compare({'A',0,'B',0}, false, wire::EmbeddedNull, "AB"));
    CHECK(!compare({'A',0,'B',0}, false, 0u, "AB"));
    CHECK(!compare({'A',0,0}, false, wire::EmbeddedNull, "A"));
    CHECK(!compare({0x80u}, false, 0u, "\xC2\x80"));
    CHECK(!compare({0u,0xD8u}, true, 0u, "A"));
    CHECK(compare({0x3Du,0xD8u,0u,0xDEu,0u,0u}, true, wire::EmbeddedNull, "\xF0\x9F\x98\x80"));
    return true;
}
bool rasterRows() {
    for (const bool definition : {false, true}) {
        evidence::Rows rows;
        rows.complete = true; rows.status = DRW_DwgClassCoverageStatus::FinalizedComplete;
        evidence::Row row;
        row.number = 532u; row.itemClass = 0x1F3u; row.proxy = 7; row.wasProxy = 1;
        row.instances = 4; row.version = 257; row.maintenance = 329;
        row.unknown1 = 41; row.unknown2 = 73; row.descriptor = 3;
        row.state = DRW_DwgClassCoverageState::Published;
        row.data = row.strings = {160u, 240u, true, true, DRW_DwgFrameOffsetSpace::DecodedBuffer};
        const std::array<std::string_view, 3> names{{"SCRATCH", evidence::rasterClass(definition), evidence::rasterRecord(definition)}};
        for (unsigned index = 0u; index < names.size(); ++index) {
            const auto name = names[index];
            row.names[index] = {rows.text.size(), name.size()};
            rows.text.insert(rows.text.end(), name.begin(), name.end());
        }
        rows.entries.push_back(row); evidence::indexRasterRoles(rows);
        DRW_UnsupportedObject raw;
        raw.m_objectType = row.number; raw.m_isCustomClass = raw.m_hasClassDefinition = true;
        raw.m_recordName = evidence::rasterRecord(definition); raw.m_className = evidence::rasterClass(definition);
        raw.m_classAppName = "SCRATCH"; raw.m_classProxyFlag = row.proxy; raw.m_classWasProxy = true;
        raw.m_classEntityFlagRaw = row.itemClass; raw.m_classDwgVersion = row.version;
        raw.m_classMaintenanceVersion = row.maintenance; raw.m_classUnknown1 = row.unknown1; raw.m_classUnknown2 = row.unknown2;
        DRW_DwgFramePublication publication;
        publication.m_isCustomClass = true; publication.m_encodedType = row.number; publication.m_classStreamOrdinal = 0u;
        publication.m_recordName = raw.m_recordName; publication.m_className = raw.m_className;
        const auto match = [&](const auto& table, const auto& object, const auto& receipt) {
            return evidence::matchRasterRow(table, object, receipt, definition);
        };
        CHECK(match(rows, raw, publication) == evidence::RasterState::Joined);
        for (unsigned mode = 0; mode != 27u; ++mode) {
            auto badRows = rows; auto badRaw = raw; auto badReceipt = publication;
            auto& bad = badRows.entries.front();
            switch (mode) {
            case 0: badRaw.m_isCustomClass = false; break;
            case 1: badRaw.m_hasClassDefinition = false; break;
            case 2: badRaw.m_isEntity = true; break;
            case 3: badReceipt.m_isEntity = true; break;
            case 4: badReceipt.m_isCustomClass = false; break;
            case 5: badReceipt.m_classStreamOrdinal.reset(); break;
            case 6: badReceipt.m_classStreamOrdinal = 1u; break;
            case 7: ++badReceipt.m_encodedType; break;
            case 8: badRaw.m_objectType = badReceipt.m_encodedType = 499u; break;
            case 9: badReceipt.m_recordName += "_EDIT"; break;
            case 10: badReceipt.m_className += "_EDIT"; break;
            case 11: ++bad.ordinal; break;
            case 12: ++bad.number; break;
            case 13: bad.state = DRW_DwgClassCoverageState::Parsed; break;
            case 14: bad.descriptor = -1; break;
            case 15: bad.data.m_present = false; break;
            case 16: bad.strings.m_present = false; break;
            case 17: ++badRaw.m_classProxyFlag; break;
            case 18: badRaw.m_classWasProxy = false; break;
            case 19: ++badRaw.m_classEntityFlagRaw; break;
            case 20: ++bad.entity; break;
            case 21: ++badRaw.m_classDwgVersion; break;
            case 22: ++badRaw.m_classMaintenanceVersion; break;
            case 23: ++badRaw.m_classUnknown1; break;
            case 24: ++badRaw.m_classUnknown2; break;
            case 25: bad.names[0].offset = badRows.text.size() + 1u; break;
            case 26: bad.itemClass = badRaw.m_classEntityFlagRaw = 0x1F2u; bad.entity = 1; break;
            }
            CHECK(match(badRows, badRaw, badReceipt) == evidence::RasterState::Contradiction);
        }
        for (unsigned name = 0; name < 3u; ++name) {
            auto bad = raw;
            (name == 0u ? bad.m_classAppName : name == 1u ? bad.m_className : bad.m_recordName) += "_EDIT";
            CHECK(match(rows, bad, publication) == evidence::RasterState::Contradiction);
        }
        auto count = rows; ++count.entries.front().instances;
        CHECK(match(count, raw, publication) == evidence::RasterState::Joined); // Not carried by raw object.
        for (unsigned alias = 0; alias < 3u; ++alias) {
            auto ambiguous = rows;
            auto other = row; other.ordinal = 1u; ++other.number;
            if (alias < 2u) other.names[alias == 0u ? 1u : 2u] = other.names[0];
            ambiguous.entries.push_back(other); evidence::indexRasterRoles(ambiguous);
            CHECK(match(ambiguous, raw, publication) == evidence::RasterState::AliasedIdentity);
        }
    }
    evidence::RasterBinding a, b;
    a.state = b.state = evidence::RasterState::Joined;
    CHECK(evidence::sameRasterBinding(a, b));
    ++b.attempt; CHECK(!evidence::sameRasterBinding(a, b)); b = a;
    ++b.ordinal; CHECK(!evidence::sameRasterBinding(a, b)); b = a;
    ++b.number; CHECK(!evidence::sameRasterBinding(a, b)); b = a;
    ++b.profileIssues; CHECK(!evidence::sameRasterBinding(a, b)); b = a;
    b.rows = std::make_shared<const evidence::Rows>(); CHECK(!evidence::sameRasterBinding(a, b));
    return true;
}
}
int main() {
    if (!ambiguity() || !textConventions() || !rasterRows()) return 1;
    std::cout << "class source joins: " << checks << " controls PASS; schema/support not promoted\n";
}
