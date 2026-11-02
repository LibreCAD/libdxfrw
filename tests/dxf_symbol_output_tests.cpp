#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>

#include "libdxfrw.h"
#include "dx_iface.h"
#include "dx_symbol_output.h"
#include "dx_symbol_name_key.h"
#include "intern/dxfwriter.h"
#include "intern/drw_textcodec.h"

namespace {
struct TestContext {
    int failures{0};
    void expect(bool condition, const char* label) {
        if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
    }
};
void testDxfTablesWriteInspection(TestContext& t) {
    class Interface final : public dx_iface {
    public:
        Interface() { cData = &data; currentBlock = data.mBlock; }
        void writeHeader(DRW_Header& header) override {
            header.vars.clear();
            header.vars["$DWGCODEPAGE"] = new DRW_Variant(3, std::string("ANSI_1252"));
        }
        void writeLayers() override {
            DRW_Layer layer; layer.handle = 0xA770u; layer.name = "caf\xC3\xA9";
            (void)dxfW->writeLayer(&layer);
        }
        void writeAppId() override {
            DRW_AppId app; app.name = "caf\xC3\xA9_APP";
            (void)dxfW->writeAppId(&app);
        }
        void writeObjects() override {
            ++objectCalls;
            if (lateObjectFailure) {
                (void)dxfW->writeXRecord(nullptr);
            }
        }
        bool finalizeDxfWrite() override {
            if (!dx_iface::finalizeDxfWrite() || rejectFinalization) return false;
            if (writeAtFinalization) { DRW_Line line; (void)dxfW->writeLine(&line); }
            reenter(nestedFinalization);
            return true;
        }
        void reenter(unsigned action) {
            if (action == 1u) nestedRejected = !dxfW->write(this, DRW::AC1032, false) && nestedRejected;
            if (action == 2u) nestedRejected = !dxfW->read(this, false) && nestedRejected;
            if (action == 3u) {
                std::string text;
                nestedRejected = !dxfW->readAscii(this, false, text) && nestedRejected;
            }
        }
        dx_data data;
        unsigned objectCalls{0};
        unsigned nestedFinalization{0};
        bool nestedRejected{true};
        bool lateObjectFailure{false}, rejectFinalization{false}, writeAtFinalization{false};
    } iface;
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-tables-inspection-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    dxfRW codec(path.string().c_str()); iface.dxfW = &codec;
    codec.setDxfWriteRecordInventoryEnabled(true);
    unsigned profiles = 0u;
    for (const auto profile : {dxfRW::DxfCompatibilityProfile::StandaloneSafe,
            dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy}) {
      codec.setDxfCompatibilityProfile(profile);
      for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018,
              DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
       for (const bool binary : {false, true}) {
        unsigned tablesCalls = 0u, objectsCalls = 0u;
        iface.objectCalls = 0u;
        std::string staged;
        t.expect(codec.setDxfTablesWriteInspector([&](const auto& view) {
            ++tablesCalls; staged.assign(view.bytes);
            unsigned layers = 0u, apps = 0u, named = 0u;
            const bool scanned = dx_symbol_output::inspect(view, [&](const auto& row) {
                layers += row.type == dx_symbol_output::Kind::Layer;
                apps += row.type == dx_symbol_output::Kind::AppId;
                named += row.qualified && (row.name == "caf\xC3\xA9" || row.name == "caf\xC3\xA9_APP");
                return true;
            });
            return scanned && view.version == version && view.binary == binary
                && view.compatibilityProfile == profile && !view.codePage.empty()
                && layers == 2u && apps == 2u && named == 2u && iface.objectCalls == 0u;
        }), "TABLES inspection installs outside the write");
        codec.setDxfObjectsWriteInspector([&](const auto&) {
            ++objectsCalls; return tablesCalls == 1u;
        });
        const bool wrote = codec.write(&iface, version, binary);
        std::ifstream input(path, std::ios::binary);
        const std::string published((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        t.expect(wrote && tablesCalls == 1u && objectsCalls == 1u && iface.objectCalls == 1u
                     && !staged.empty() && published.find(staged) != std::string::npos,
                 "Inspector sees exact backpatched TABLES bytes/profile before OBJECTS and eventual publication");
        ++profiles;
       }
      }
    }
    codec.setDxfCompatibilityProfile(dxfRW::DxfCompatibilityProfile::StandaloneSafe);
    for (const bool binary : {false, true}) for (unsigned fault = 0u; fault < 12u; ++fault) {
        const std::string sentinel = "TABLES_INSPECTOR_DESTINATION_UNCHANGED";
        { std::ofstream previous(path, std::ios::binary); previous << sentinel; }
        unsigned calls = 0u;
        iface.objectCalls = 0u;
        iface.lateObjectFailure = fault == 7u;
        iface.rejectFinalization = fault == 8u;
        iface.writeAtFinalization = fault == 9u;
        codec.setDxfObjectsWriteInspector({});
        codec.setDxfTablesWriteInspector([&](const auto&) {
            ++calls;
            if (fault == 0u) return false;
            if (fault == 1u) throw std::runtime_error("TABLES inspection failure control");
            if (fault == 2u) (void)codec.setDxfTablesWriteInspector({});
            if (fault == 3u) (void)codec.setDxfObjectsWriteInspector({});
            if (fault == 4u) { DRW_Line line; (void)codec.writeLine(&line); }
            if (fault == 5u) iface.reenter(1u);
            if (fault == 6u) codec.setHandleRemap({{0xA770u, 0xA771u}});
            if (fault == 10u) iface.reenter(2u);
            if (fault == 11u) iface.reenter(3u);
            return true;
        });
        const bool wrote = codec.write(&iface, DRW::AC1032, binary);
        std::ifstream input(path, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        const bool refused = !wrote && calls == 1u && retained == sentinel && iface.nestedRejected
                     && iface.objectCalls == (fault >= 7u && fault <= 9u ? 1u : 0u)
                     && codec.getWritingContext()->sourceHandleToWrittenTableMap.empty();
        if (!refused) std::cerr << "TABLES refusal fault=" << fault << " binary=" << binary
            << " wrote=" << wrote << " calls=" << calls << " objects=" << iface.objectCalls
            << " sentinel=" << (retained == sentinel) << " aliases="
            << codec.getWritingContext()->sourceHandleToWrittenTableMap.size() << '\n';
        t.expect(refused,
                 "TABLES veto/throw/mutation/reentrancy or later failure retracts bytes and aliases");
        iface.lateObjectFailure = iface.rejectFinalization = iface.writeAtFinalization = false;
        calls = 0u;
        t.expect(codec.setDxfTablesWriteInspector([&](const auto&) { ++calls; return true; })
                     && codec.write(&iface, DRW::AC1032, binary) && calls == 1u,
                 "Same-codec retry recovers after TABLES inspection or later publication failure");
    }
    codec.setDxfTablesWriteInspector({});
    // OBJECTS inspection has the same sticky contract. The consumer finalizer
    // retains its established non-mutating rejection of nested whole calls.
    for (const bool binary : {false, true}) for (const bool finalization : {false, true}) {
      for (unsigned action = 1u; action <= 3u; ++action) {
        const std::string sentinel = "READ_ONLY_WRITE_PHASE_NESTING_UNCHANGED";
        { std::ofstream previous(path, std::ios::binary); previous << sentinel; }
        iface.nestedFinalization = finalization ? action : 0u;
        iface.nestedRejected = true;
        codec.setDxfObjectsWriteInspector([&](const auto&) {
            if (!finalization) iface.reenter(action);
            return true;
        });
        const bool wrote = codec.write(&iface, DRW::AC1032, binary);
        std::ifstream input(path, std::ios::binary);
        const std::string retained((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
        input.close();
        if (finalization)
            t.expect(wrote && retained != sentinel && iface.nestedRejected,
                     "Finalizer rejects nested whole operations without changing the established outer-write contract");
        else
            t.expect(!wrote && retained == sentinel && iface.nestedRejected
                         && codec.getWritingContext()->sourceHandleToWrittenTableMap.empty(),
                     "Ignored nested write/read/readAscii invalidates OBJECTS inspection publication");
        iface.nestedFinalization = 0u;
        codec.setDxfObjectsWriteInspector({});
        t.expect(codec.write(&iface, DRW::AC1032, binary),
                 "Read-only phase nesting failure permits a corrected same-codec retry");
      }
    }
    t.expect(codec.write(&iface, DRW::AC1032, false), "Empty TABLES inspector retains ordinary authored write");
    unsigned r12Calls = 0u;
    dxfRW r12(path.string().c_str()); Interface r12Iface; r12Iface.dxfW = &r12;
    r12.setDxfTablesWriteInspector([&](const auto& view) {
        ++r12Calls; return view.version == DRW::AC1009 && !view.bytes.empty();
    });
    t.expect(r12.write(&r12Iface, DRW::AC1009, false) && r12Calls == 1u,
             "Legacy handle-free TABLES still has an honest raw inspection, not fabricated name qualification");
    std::cout << "TABLES inspector profile cells=" << profiles
              << " atomic refusal/retries=24 OBJECTS nesting/retries=6 finalizer non-mutating nesting=6 legacy raw=1\n";
}

void testSymbolUnicodeKeys(TestContext& t) {
    DRW_TextCodec codec; codec.setVersion("AC1021", false);
    const auto utf8 = [&](std::uint32_t scalar) {
        std::string bytes;
        const auto unit = [&](std::uint32_t value) {
            bytes.push_back(static_cast<char>(value & 0xFFu));
            bytes.push_back(static_cast<char>(value >> 8u));
        };
        if (scalar <= 0xFFFFu) unit(scalar);
        else { scalar -= 0x10000u; unit(0xD800u + (scalar >> 10u)); unit(0xDC00u + (scalar & 0x3FFu)); }
        return codec.toUtf8(bytes);
    };
    std::size_t checked = 0u;
    for (const auto& mapping : drw_symbol_casefold_data::mappings) {
        std::string expected;
        for (unsigned index = 0u; index < mapping.count; ++index)
            expected += utf8(mapping.target[index]);
        std::string actual, repeated;
        t.expect(dx_symbol_output::nameKey(utf8(mapping.source), actual)
            && actual == expected && dx_symbol_output::nameKey(actual, repeated) && repeated == actual,
            "Every pinned Unicode case-closure row decodes, expands and remains idempotent");
        ++checked;
    }
    const std::pair<std::string, std::string> pairs[] = {
        {"LAYER", "layer"}, {u8"Stra\u00DFe", "STRASSE"}, {u8"\u212A", "K"},
        {u8"\u00C6", u8"\u00E6"}, {u8"\u0131", "I"}, {u8"\u0130", u8"i\u0307"},
        {u8"\u03A3\u03C2\u03C3", u8"\u03C3\u03C3\u03C3"}, {u8"\uFB03", "FFI"},
        {u8"\u01C5", u8"\u01C4"}, {u8"\U00010400", u8"\U00010428"},
        {u8"\U0001E900", u8"\U0001E922"}};
    for (const auto& pair : pairs) {
        std::string a, b;
        t.expect(dx_symbol_output::nameKey(pair.first, a) && dx_symbol_output::nameKey(pair.second, b)
            && a == b, "Portable ambiguity keys cover ASCII, expansions, symbols and supplementary case pairs");
    }
    std::string a, b;
    t.expect(dx_symbol_output::nameKey(u8"\u00E9", a) && dx_symbol_output::nameKey(u8"e\u0301", b) && a != b,
             "Case ambiguity policy does not silently invent Unicode normalization equivalence");
    const std::string malformed[] = {"\x80", "\xC0\x80", "\xC2", "\xE0\x80\x80", "\xED\xA0\x80",
        "\xF4\x90\x80\x80", "\xF5\x80\x80\x80", "\xE2\x28\xA1", "\xE2\x82"};
    for (const auto& text : malformed) {
        a = "PREVIOUS_KEY";
        t.expect(!dx_symbol_output::nameKey(text, a) && a.empty(), "Malformed UTF8 refuses without retaining a prior/partial key");
    }
    std::string large;
    large.reserve(512u * 1024u);
    for (std::size_t index = 0u; index < 256u * 1024u; ++index) large += u8"\u0390";
    t.expect(dx_symbol_output::nameKey(large, a) && a.size() == 3u * large.size(),
             "Exact maximum input and3x full-fold expansion remain bounded");
    large.push_back('a');
    t.expect(!dx_symbol_output::nameKey(large, a) && a.empty(), "One-byte-over Unicode key input refuses and clears ownership");
    std::cout << "Unicode symbol-key rows=" << checked << " case-pair controls=11 malformed controls=9 owner limits=2\n";
}

void testDxfSourceTableIdentities(TestContext& t) {
    class Interface final : public dx_iface {
    public:
        Interface(TestContext& test) : test(test) { cData = &data; currentBlock = data.mBlock; }
        void writeHeader(DRW_Header& header) override {
            header.vars.clear();
            header.vars["$DWGCODEPAGE"] = new DRW_Variant(3, std::string("ANSI_1252"));
            for (const auto source : {0xA710u, 0xA711u, 0xA712u, 0xA713u, 0xA714u, 0xA715u, 0xA716u})
                test.expect(dxfW->dxfWriteEmittedSourceTableHandle(source) == 0u,
                            "No successful table encoder receipt exists before TABLES");
            if (plan) {
                for (const auto source : {0xA710u, 0xA711u, 0xA712u, 0xA713u, 0xA714u, 0xA715u, 0xA716u}) {
                    const auto allocated = dxfW->preallocateTableRecordHandle(source);
                    test.expect(allocated != 0u && dxfW->dxfWriteEmittedSourceTableHandle(source) == 0u,
                                "Forward preallocation cannot supply a successful-emission receipt");
                }
            }
            if (fault == 11u) {
                DRW_AppId app; app.handle = 0xA712u; app.name = "ACAD";
                (void)dxfW->writeAppId(&app);
            }
        }
        void writeVports() override {
            DRW_Vport row; row.handle = 0xA715u; row.name = "*ACTIVE";
            (void)dxfW->writeVport(&row);
        }
        void writeLayers() override {
            DRW_Layer zero; zero.handle = 0xA710u; zero.name = "0";
            DRW_Layer named; named.handle = fault == 9u ? 0xA715u : 0xA711u; named.name = "BOUND_LAYER";
            (void)dxfW->writeLayer(&zero); (void)dxfW->writeLayer(&named);
            if (fault == 3u) (void)dxfW->writeLayer(&named);
            test.expect(zero.handle == 0xA710u && zero.name == "0" && named.name == "BOUND_LAYER",
                        "Source table handles/names remain unchanged by serialization");
        }
        void writeTextstyles() override {
            DRW_Textstyle row; row.handle = 0xA714u; row.name = "BOUND_STYLE";
            (void)dxfW->writeTextstyle(&row);
        }
        void writeAppId() override {
            DRW_AppId acad; acad.handle = 0xA712u; acad.name = "ACAD";
            DRW_AppId named; named.handle = fault == 1u ? 0xA711u : 0xA713u; named.name = "BOUND_APP";
            (void)dxfW->writeAppId(&acad); (void)dxfW->writeAppId(&named);
            if (fault == 2u) { acad.handle = 0xA717u; (void)dxfW->writeAppId(&acad); }
            if (fault == 10u) { named.name = "SECOND_NAME"; (void)dxfW->writeAppId(&named); }
            test.expect(!dxfW->dxfWriteSourceTableHasIdentity(0xA712u, 0x12u, "APPID"),
                        "Completed-row receipts are not final inventory receipts during callbacks");
        }
        void writeObjects() override {
            if (fault == 7u) (void)dxfW->writeXRecord(nullptr);
            if (fault == 12u) {
                DRW_XRecord record; record.handle = 0xA713u; record.parentHandle = 0xCu;
                (void)dxfW->writeXRecord(&record);
            }
        }
        bool finalizeDxfWrite() override {
            if (!dx_iface::finalizeDxfWrite() || fault == 8u) return false;
            ++finalizations;
            const auto version = dxfW->getVersion();
            const std::pair<std::uint32_t, const char*> rows[] = {
                {0xA710u, "LAYER"}, {0xA711u, "LAYER"}, {0xA712u, "APPID"},
                {0xA713u, "APPID"}, {0xA714u, "STYLE"}, {0xA715u, "VPORT"}};
            for (const auto& row : rows) {
                const auto emitted = dxfW->dxfWriteEmittedSourceTableHandle(row.first);
                test.expect(emitted != 0u && (row.first != 0xA710u || emitted == 0x10u)
                    && (row.first != 0xA712u || emitted == 0x12u), "Actual staged table aliases include fixed built-ins");
                const auto owner = row.first <= 0xA711u ? 2u : row.first <= 0xA713u ? 9u : row.first == 0xA714u ? 3u : 8u;
                test.expect(dxfW->dxfWriteSourceTableHasIdentity(row.first, emitted, row.second) == inventory,
                            "Only unique actual TABLES identities with inventory enabled supply final receipts");
                test.expect(dxfW->dxfWriteSourceTableHasIdentity(row.first, emitted, row.second, owner)
                    == (inventory && (version > DRW::AC1014 || std::string(row.second) != "APPID")),
                    "Owner-aware table receipts check physically present versioned owners");
                test.expect(!dxfW->dxfWriteSourceTableHasIdentity(row.first, emitted, "XRECORD")
                    && !dxfW->dxfWriteSourceTableHasIdentity(row.first, emitted + 1u, row.second)
                    && !dxfW->dxfWriteSourceTableHasIdentity(row.first, emitted, row.second, owner + 1u),
                    "Wrong type, emitted handle or owner cannot borrow a registered table identity");
            }
            test.expect(dxfW->dxfWriteEmittedSourceTableHandle(0xA716u) == 0u
                && !dxfW->dxfWriteSourceTableHasIdentity(0xA716u,
                    dxfW->dxfWriteEmittedSourceTableHandle(0xA711u), "LAYER"),
                "An unwritten forward alias cannot borrow a real row, even at the same planned output handle");
            if (!plan) test.expect(dxfW->remapEntityHandle(0xA710u) == 0x10u,
                                  "Unplanned layer0 retains its established legacy structural alias");
            return true;
        }
        TestContext& test; dx_data data;
        bool inventory{true}, plan{false}; unsigned fault{0u}, finalizations{0u};
    } iface(t);
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-table-identity-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code ignored; std::filesystem::remove(path, ignored); }
    } cleanup{path};
    dxfRW codec(path.string().c_str()); iface.dxfW = &codec;
    const auto remaps = [](bool planned, unsigned fault = 0u) {
        std::map<std::uint32_t, std::uint32_t> values;
        if (planned) values = {{0xA710u, 0x10u}, {0xA711u, 0xA920u}, {0xA712u, 0x12u},
            {0xA713u, 0xA921u}, {0xA714u, 0xA922u}, {0xA715u, 0xA923u}, {0xA716u, 0xA920u}};
        if (fault == 4u) values[0xA712u] = 0xA999u;
        if (fault == 5u) values[0xA710u] = 0xA999u;
        return values;
    };
    unsigned cells = 0u, refusals = 0u;
    for (const auto version : {DRW::AC1014, DRW::AC1015, DRW::AC1018,
            DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      for (const bool binary : {false, true}) for (const bool planned : {false, true}) {
        iface.plan = planned; iface.finalizations = 0u;
        codec.setDxfWriteRecordInventoryEnabled(true); codec.setHandleRemap(remaps(planned));
        const bool wrote = codec.write(&iface, version, binary);
        t.expect(wrote && iface.finalizations == 1u
            && codec.dxfWriteEmittedSourceTableHandle(0xA710u) == 0u
            && !codec.dxfWriteSourceTableHasIdentity(0xA712u, 0x12u, "APPID"),
            "Source table identities require active/finalizing scope, including after successful publication");
        ++cells;
      }
    }
    iface.plan = false; codec.setHandleRemap({});
    for (const bool binary : {false, true}) {
        iface.inventory = false; codec.setDxfWriteRecordInventoryEnabled(false);
        t.expect(codec.write(&iface, DRW::AC1032, binary), "Ordinary no-inventory table output remains compatible");
        iface.inventory = true; codec.setDxfWriteRecordInventoryEnabled(true);
        for (unsigned fault = 1u; fault <= 12u; ++fault) {
            const std::string sentinel = "TABLE_IDENTITY_DESTINATION_UNCHANGED";
            { std::ofstream previous(path, std::ios::binary); previous << sentinel; }
            iface.fault = fault; codec.setHandleRemap(remaps(false, fault));
            codec.setDxfTablesWriteInspector([&](const auto&) { return fault != 6u; });
            const bool wrote = codec.write(&iface, DRW::AC1032, binary);
            std::ifstream input(path, std::ios::binary);
            const std::string retained((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            input.close();
            const bool refused = !wrote && retained == sentinel
                && codec.getWritingContext()->sourceHandleToWrittenTableMap.empty()
                && codec.dxfWriteEmittedSourceTableHandle(0xA712u) == 0u;
            if (!refused) std::cerr << "Table identity refusal fault=" << fault << " binary=" << binary << " wrote=" << wrote << '\n';
            t.expect(refused, "Table identity conflicts or later failures retract aliases and preserve old destination");
            iface.fault = 0u; codec.setHandleRemap({}); codec.setDxfTablesWriteInspector({});
            t.expect(codec.write(&iface, DRW::AC1032, binary), "Corrected same-codec table identity retry succeeds");
            ++refusals;
        }
    }
    std::cout << "Source table identity cells=" << cells << " atomic refusal/retries=" << refusals << " no-inventory controls=2\n";
}

void testDxfSourceLinetypeTargets(TestContext& t) {
    class Interface final : public dx_iface {
    public:
        Interface(TestContext& test) : test(test) { cData = &data; currentBlock = data.mBlock; }
        void writeHeader(DRW_Header& header) override {
            header.vars.clear();
            for (const auto source : {0xC410u, 0xC411u, 0xC412u, 0xC413u, 0xC414u})
                test.expect(dxfW->dxfWriteSourceLinetypeTargetHandle(source) == 0u,
                            "Source LTYPE target requires this attempt's writer registration");
            const auto forward = dxfW->preallocateEntityHandle(0xC414u);
            test.expect(forward != 0u && dxfW->dxfWriteSourceLinetypeTargetHandle(0xC414u) == 0u,
                        "A generic forward alias cannot invent a source LTYPE target");
        }
        void writeLTypes() override {
            const std::pair<std::uint32_t, const char*> canonical[] = {
                {0xC410u, "ByBlock"}, {0xC411u, "ByLayer"}, {0xC412u, "Continuous"}};
            for (const auto& source : canonical) {
                DRW_LType row; row.handle = source.first; row.name = source.second;
                test.expect(dxfW->writeLineType(&row)
                    && dxfW->dxfWriteSourceLinetypeTargetHandle(source.first) == 0x14u + source.first - 0xC410u,
                    "Registered canonical LTYPE targets use their fixed builtin identities");
            }
            DRW_LType custom; custom.handle = 0xC413u; custom.name = "CustomDash"; custom.path = {.5, -.25};
            test.expect(dxfW->writeLineType(&custom), "Custom source LTYPE writer succeeds");
            customTarget = dxfW->dxfWriteSourceLinetypeTargetHandle(custom.handle);
            test.expect(customTarget != 0u && customTarget == dxfW->remapEntityHandle(custom.handle)
                && dxfW->dxfWriteEmittedSourceTableHandle(custom.handle) == 0u
                && dxfW->dxfWriteSourceLinetypeTargetHandle(0xC414u) == 0u,
                "Source LTYPE registry target is distinct from a generic table-emission receipt");
            if (fault == 1u) { custom.name = "SecondDash"; (void)dxfW->writeLineType(&custom); }
            if (fault == 2u) { custom.name = "BYLAYER"; (void)dxfW->writeLineType(&custom); }
            if (fault == 3u) {
                custom.handle = 0xC415u; custom.path = {std::numeric_limits<double>::quiet_NaN()};
                (void)dxfW->writeLineType(&custom);
            }
            if (fault >= 1u && fault <= 3u)
                test.expect(dxfW->dxfWriteSourceLinetypeTargetHandle(0xC413u) == 0u,
                            "Sticky encoder failure cannot publish a source LTYPE target");
        }
        void writeObjects() override { if (fault == 5u) (void)dxfW->writeXRecord(nullptr); }
        bool finalizeDxfWrite() override {
            if (!dx_iface::finalizeDxfWrite() || fault == 6u) return false;
            for (const auto source : {0xC410u, 0xC411u, 0xC412u, 0xC413u}) {
                const auto target = dxfW->dxfWriteSourceLinetypeTargetHandle(source);
                test.expect(target != 0u && dxfW->dxfWriteRecordHasIdentity(target, "LTYPE", "TABLES") == inventory
                    && dxfW->dxfWriteRecordHasOwner(target, 5u) == (inventory && dxfW->getVersion() > DRW::AC1012),
                    "Tentative LTYPE target needs separate actual record/type/owner evidence");
            }
            return true;
        }
        TestContext& test; dx_data data; unsigned fault{0u};
        std::uint32_t customTarget{0u}; bool inventory{true};
    } iface(t);
    const auto path = std::filesystem::temp_directory_path() / ("libdxfrw-linetype-target-"
        + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); } } cleanup{path};
    dxfRW codec(path.string().c_str()); iface.dxfW = &codec;
    unsigned cells = 0u, refusals = 0u;
    for (const auto version : {DRW::AC1014, DRW::AC1027}) for (const bool binary : {false, true}) {
        codec.setDxfWriteRecordInventoryEnabled(true);
        unsigned lineTypes = 0u;
        codec.setDxfTablesWriteInspector([&](const auto& view) {
            return dx_symbol_output::inspect(view, [&](const auto& row) {
                if (row.type == dx_symbol_output::Kind::LineType) { ++lineTypes; return row.qualified; }
                return true;
            }) && lineTypes == 4u;
        });
        const auto before = codec.dxfWriteSourceLinetypeTargetHandle(0xC413u);
        const bool wrote = codec.write(&iface, version, binary);
        const auto after = codec.dxfWriteSourceLinetypeTargetHandle(0xC413u);
        if (before != 0u || !wrote || lineTypes != 4u || after != 0u)
            std::cerr << "Source LTYPE target version=" << static_cast<int>(version)
                << " binary=" << binary << " before=" << before << " wrote=" << wrote
                << " observed=" << lineTypes << " after=" << after << '\n';
        t.expect(before == 0u && wrote && lineTypes == 4u && after == 0u,
            "Source LTYPE target exists only within an admitted write attempt");
        ++cells;
    }
    for (const bool binary : {false, true}) {
        iface.inventory = false; codec.setDxfWriteRecordInventoryEnabled(false); codec.setDxfTablesWriteInspector({});
        t.expect(codec.write(&iface, DRW::AC1027, binary), "Source target registry does not fabricate disabled record inventory");
        iface.inventory = true; codec.setDxfWriteRecordInventoryEnabled(true);
        for (unsigned fault = 1u; fault <= 10u; ++fault) {
            const std::string sentinel = "SOURCE_LTYPE_TARGET_DESTINATION_UNCHANGED";
            { std::ofstream prior(path, std::ios::binary); prior << sentinel; }
            iface.fault = fault;
            codec.setDxfTablesWriteInspector([&](const auto&) {
                auto* context = codec.getWritingContext();
                if (fault == 7u) context->ambiguousSourceHandles.insert(0xC413u);
                if (fault == 8u) context->sourceHandleToMintedMap[0xC413u] = iface.customTarget + 1u;
                if (fault == 9u) context->sourceHandleToWrittenObjectMap[0xC413u] = iface.customTarget;
                if (fault == 10u) context->sourceHandleToWrittenTableMap[0xC413u] = iface.customTarget;
                if (fault >= 7u)
                    t.expect(codec.dxfWriteSourceLinetypeTargetHandle(0xC413u) == 0u,
                             "Ambiguous, conflicting or wrong-kind source aliases cannot qualify an LTYPE target");
                return fault != 4u && fault < 7u;
            });
            const bool wrote = codec.write(&iface, DRW::AC1027, binary);
            std::ifstream input(path, std::ios::binary);
            const std::string retained((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
            t.expect(!wrote && retained == sentinel && codec.dxfWriteSourceLinetypeTargetHandle(0xC413u) == 0u,
                     "LTYPE writer/inspection/later failure retracts query scope and preserves destination");
            iface.fault = 0u; codec.setDxfTablesWriteInspector({});
            t.expect(codec.write(&iface, DRW::AC1027, binary)
                && codec.dxfWriteSourceLinetypeTargetHandle(0xC413u) == 0u,
                "Corrected same-codec retry rebuilds source LTYPE target registry");
            ++refusals;
        }
    }
    std::cout << "Source LTYPE target cells=" << cells << " atomic refusal/retries=" << refusals << " no-inventory controls=2\n";
}

void testSymbolNameWireInspection(TestContext& t) {
    unsigned cells = 0u, limitsChecked = 0u;
    for (const auto version : {DRW::AC1014, DRW::AC1018, DRW::AC1032}) for (const bool binary : {false, true}) {
      for (const auto type : {dx_symbol_output::Kind::Layer, dx_symbol_output::Kind::AppId,
              dx_symbol_output::Kind::LineType}) {
       if (version == DRW::AC1014 && type != dx_symbol_output::Kind::LineType) continue;
       const std::string table = type == dx_symbol_output::Kind::Layer ? "LAYER"
            : type == dx_symbol_output::Kind::AppId ? "APPID" : "LTYPE";
       for (unsigned fault = 0u; fault < 45u; ++fault) {
        std::ostringstream stream(std::ios::out | std::ios::binary);
        std::unique_ptr<dxfWriter> writer;
        if (binary) writer = std::make_unique<dxfWriterBinary>(&stream);
        else writer = std::make_unique<dxfWriterAscii>(&stream);
        writer->setVersion(version == DRW::AC1014 ? "AC1014" : version == DRW::AC1018 ? "AC1018" : "AC1032", true);
        writer->setCodePage("ANSI_1252");
        const auto emitHeader = [&]() {
            writer->writeString(0, "TABLE");
            if (fault != 38u) writer->writeString(2, table);
            if (fault == 4u) writer->writeString(2, table);
            if (fault != 1u) writer->writeString(5, fault == 42u ? "10000A500" : "A500");
            writer->writeString(330, fault == 36u ? "BAD" : "0");
            if (fault == 37u) writer->writeString(330, "0");
            writer->writeString(100, fault == 2u ? "WrongTable" : "AcDbSymbolTable");
            if (fault != 3u) writer->writeInt16(70, 2);
        };
        if (fault == 32u) writer->writeString(0, table);
        emitHeader();
        const unsigned rows = fault == 33u || fault == 40u ? 2u : 1u;
        for (unsigned row = 0u; row < rows; ++row) {
            writer->writeString(0, fault == 5u ? "WRONG_TABLE_ROW" : table);
            if (fault != 6u) writer->writeString(5, fault == 41u ? "10000A510" : row == 0u ? "A510" : "A511");
            if (fault == 7u) writer->writeString(5, "A510");
            writer->writeString(102, "{ACAD_REACTORS");
            writer->writeString(330, "BEEF");
            writer->writeString(2, "IGNORED_METADATA_NAME");
            writer->writeString(5, "DEAD");
            if (fault == 34u) {
                writer->writeString(102, "{NESTED"); writer->writeString(2, "NESTED_METADATA_NAME");
                writer->writeString(102, "}");
            }
            if (fault != 20u) writer->writeString(102, "}");
            if (fault != 9u && fault != 22u)
                writer->writeString(330, fault == 43u ? "10000A500" : fault == 8u ? "A501" : "A500");
            if (fault == 10u) writer->writeString(330, "A500");
            if (fault == 17u) writer->writeUtf8String(2, "caf\xC3\xA9");
            if (fault != 11u) writer->writeString(100, "AcDbSymbolTableRecord");
            const std::string leaf = type == dx_symbol_output::Kind::Layer ? "AcDbLayerTableRecord"
                : type == dx_symbol_output::Kind::AppId ? "AcDbRegAppTableRecord" : "AcDbLinetypeTableRecord";
            writer->writeString(100, fault == 12u ? "WrongLeaf" : leaf);
            if (fault == 13u) writer->writeString(100, leaf);
            if (fault != 14u && fault != 21u && fault != 17u) {
                if (fault == 26u || fault == 33u || (fault == 40u && row == 0u)) writer->writeString(2, "\\U+ZZZZ");
                else writer->writeUtf8String(2, fault == 16u ? "" : fault == 35u ? "\xEF\xBF\xBD" : "caf\xC3\xA9");
            }
            if (fault == 15u) writer->writeUtf8String(2, "caf\xC3\xA9");
            if (fault != 18u) writer->writeInt16(70, 0);
            if (fault == 19u) writer->writeInt16(70, 0);
        }
        if (fault == 44u) emitHeader();
        if (fault != 24u) writer->writeString(0, "ENDTAB");
        if (fault == 23u) { emitHeader(); writer->writeString(0, "ENDTAB"); }
        if (fault == 31u) writer->writeString(2, "AFTER_ENDTAB");
        auto bytes = stream.str();
        if (fault == 25u) bytes.resize(bytes.size() - (binary ? 1u : 2u));
        if (fault == 39u && !binary) bytes.resize(bytes.size() - 1u);
        const std::string page = fault == 27u ? "NOT_A_CODEPAGE" : writer->getCodePage();
        const dxfRW::DxfTablesWriteView view{bytes, version, binary, page, dxfRW::DxfCompatibilityProfile::StandaloneSafe};
        dx_symbol_output::Limits limits;
        if (fault == 28u) limits.nameWireBytes = 0u;
        if (fault == 29u) limits.nameBytes = 0u;
        if (fault == 30u) limits.records = 2u;
        unsigned observed = 0u, qualified = 0u;
        const bool scanned = dx_symbol_output::inspect(view, [&](const auto& value) {
            ++observed; qualified += value.qualified;
            if (value.qualified) t.expect(value.type == type && value.handle == (fault == 40u ? 0xA511u : 0xA510u)
                && value.owner == 0xA500u && value.tableHandle == 0xA500u
                && value.name == (fault == 35u ? "\xEF\xBF\xBD" : "caf\xC3\xA9"),
                "Scoped name observations exclude application metadata and preserve exact semantic spelling");
            return true;
        }, limits);
        const bool positive = fault == 0u || fault == 34u || fault == 35u || fault == 39u || fault == 40u
            || (fault == 27u && version >= DRW::AC1021);
        // A malformed/truncated body invalidates all tentative callback rows.
        if ((scanned && qualified == 1u && observed == (fault == 40u ? 2u : 1u)) != positive) {
            std::cerr << "Symbol name kind=" << table << " version=" << static_cast<int>(version)
                      << " binary=" << binary << " fault=" << fault << " scanned=" << scanned
                      << " observed=" << observed << " qualified=" << qualified << '\n';
            t.expect(false, "Malformed/missing/duplicate/defaulted/scoped/profile-damaged name fields cannot qualify");
        }
        if (fault == 0u) {
            dx_symbol_output::Limits exact;
            exact.records = 3u; exact.nameBytes = 5u;
            exact.nameWireBytes = version < DRW::AC1021 ? 4u : 5u;
            t.expect(dx_symbol_output::inspect(view, [](const auto& value) { return value.qualified; }, exact),
                     "Exact observed row/name-wire/name-owner limits admit");
            for (unsigned below = 0u; below < 4u; ++below) {
                auto bounded = exact;
                if (below == 0u) --bounded.records;
                if (below == 1u) --bounded.nameBytes;
                if (below == 2u) --bounded.nameWireBytes;
                if (below == 3u) bounded.physicalRecords = 1u;
                t.expect(!dx_symbol_output::inspect(view, [](const auto&) { return true; }, bounded),
                         "One-below row/name-wire/name-owner and physical-token limits refuse");
                ++limitsChecked;
            }
            t.expect(!dx_symbol_output::inspect(view, [](const auto&) { return false; }), "Visitor veto invalidates scan");
        }
        ++cells;
       }
      }
    }
    std::cout << "Symbol-name wire cells=" << cells << " bounded refusals=" << limitsChecked << '\n';
}

} // namespace
int main(int argc, char** argv) {
    TestContext context;
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--dxf-tables-write-inspection"))
        testDxfTablesWriteInspection(context);
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--symbol-name-wire-inspection"))
        testSymbolNameWireInspection(context);
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--source-table-identities")) {
        testDxfSourceTableIdentities(context);
        testDxfSourceLinetypeTargets(context);
    }
    if (argc == 1 || (argc == 2 && std::string(argv[1]) == "--symbol-unicode-keys"))
        testSymbolUnicodeKeys(context);
    if (argc > 2 || (argc == 2 && std::string(argv[1]) != "--dxf-tables-write-inspection"
        && std::string(argv[1]) != "--symbol-name-wire-inspection"
        && std::string(argv[1]) != "--source-table-identities"
        && std::string(argv[1]) != "--symbol-unicode-keys")) return 2;
    return context.failures == 0 ? 0 : 1;
}
