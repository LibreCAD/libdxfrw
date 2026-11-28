// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_layout_associations.h"
#include <iostream>
#include <map>
#include <string>

namespace {
struct Layout {
    std::uint32_t handle;
    struct { std::uint32_t ref; } paperSpaceBlockRecordHandle;
};
struct Record {
    std::uint32_t handle;
    std::string name;
    std::uint32_t layoutHandle;
    std::uint32_t xDictHandle;
};
}

int main() {
    using namespace dx_layout_associations;
    int failures = 0;
    const auto expect = [&failures](bool condition, const char* label) {
        if (!condition) { std::cerr << label << '\n'; ++failures; }
    };
    for (const auto version : {DRW::AC1012, DRW::AC1014}) {
        expect(sourceMode(true, version) == Mode::LegacyDwgForwardOnly,
               "legacy DWG must use forward-only source relationships");
        expect(sourceMode(false, version) == Mode::Reciprocal,
               "legacy DXF must still require reciprocal relationships");
    }
    for (const auto version : {DRW::UNKNOWNV, DRW::AC1015, DRW::AC1018,
                               DRW::AC1024})
        expect(sourceMode(true, version) == Mode::Reciprocal,
               "nonlegacy DWG must require reciprocal relationships");

    Layout first{0x30u, {0x10u}}, second{0x31u, {0x11u}};
    Record a{0x10u, "*Model_Space", 0u, 0x40u};
    Record b{0x11u, "*Paper_Space", 0u, 0u};
    Record extra{0x12u, "Other", 0u, 0u};
    std::map<std::uint32_t, const Layout*> layouts{{first.handle, &first},
                                                   {second.handle, &second}};
    std::map<std::uint32_t, const Record*> records{{0x10u, &a}, {0x11u, &b},
                                                   {0x12u, &extra}};
    std::map<std::uint32_t, std::uint32_t> result{{0x99u, 0x98u}};
    std::string reason;
    const auto run = [&](Mode mode) { return resolve(layouts, records, mode, result, reason); };
    const std::map<std::uint32_t, std::uint32_t> expected{{0x10u, 0x30u}, {0x11u, 0x31u}};
    expect(run(Mode::LegacyDwgForwardOnly) && result == expected,
           "legacy forward associations must resolve uniquely");
    const auto refused = [&](const char* label) {
        result = {{0x99u, 0x98u}};
        expect(!run(Mode::LegacyDwgForwardOnly)
                   && result == std::map<std::uint32_t, std::uint32_t>{{0x99u, 0x98u}}, label);
    };
    second.paperSpaceBlockRecordHandle.ref = 0x10u;
    refused("duplicate block target must refuse without changing result");
    second.paperSpaceBlockRecordHandle.ref = 0u;
    refused("null block target must refuse without changing result");
    second.paperSpaceBlockRecordHandle.ref = 0x19u;
    refused("missing block target must refuse without changing result");
    second.paperSpaceBlockRecordHandle.ref = 0x11u;
    extra.layoutHandle = 0x30u;
    refused("legacy source cannot contain a synthesized backlink");
    extra.layoutHandle = 0u;
    expect(run(Mode::LegacyDwgForwardOnly) && result == expected,
           "corrected legacy source must succeed after refusal");
    // All users of the shared resolver must reject malformed identities before
    // dereferencing them, without replacing the caller's previous result.
    const auto identityRefused = [&](Mode mode, const char* label) {
        result = {{0x99u, 0x98u}};
        expect(!run(mode)
                   && result == std::map<std::uint32_t, std::uint32_t>{{0x99u, 0x98u}}, label);
    };
    for (const Mode mode : {Mode::LegacyDwgForwardOnly, Mode::Reciprocal}) {
        a.layoutHandle = mode == Mode::Reciprocal ? first.handle : 0u;
        b.layoutHandle = mode == Mode::Reciprocal ? second.handle : 0u;
        auto* savedRecord = records[0x11u];
        records[0x11u] = nullptr;
        identityRefused(mode, "null record must refuse without changing result");
        records[0x11u] = savedRecord;
        b.handle = 0x19u;
        identityRefused(mode, "record key mismatch must refuse without changing result");
        b.handle = 0x11u;
        auto* savedLayout = layouts[0x31u];
        layouts[0x31u] = nullptr;
        identityRefused(mode, "null layout must refuse without changing result");
        layouts[0x31u] = savedLayout;
        second.handle = 0x39u;
        identityRefused(mode, "layout key mismatch must refuse without changing result");
        second.handle = 0x31u;
        Record invalidRecord{DRW::NoHandle, "Invalid", 0u, 0u};
        records.emplace(invalidRecord.handle, &invalidRecord);
        identityRefused(mode, "sentinel record identity must refuse without changing result");
        records.erase(invalidRecord.handle);
        Layout invalidLayout{0u, {a.handle}};
        layouts.emplace(invalidLayout.handle, &invalidLayout);
        identityRefused(mode, "null layout identity must refuse without changing result");
        layouts.erase(invalidLayout.handle);
        second.paperSpaceBlockRecordHandle.ref = DRW::NoHandle;
        identityRefused(mode, "sentinel block target must refuse without changing result");
        second.paperSpaceBlockRecordHandle.ref = b.handle;
        expect(run(mode) && result == expected,
               "corrected identities must succeed after refusal");
    }
    a.layoutHandle = b.layoutHandle = 0u;
    std::vector<BlockRecordProjection> projection;
    const auto project = [&](DRW::Version target) {
        return projectLegacyToR2000(layouts, records, target, projection, reason);
    };
    expect(project(DRW::AC1015) && projection.size() == 3u
               && projection[0].handle == 0x10u
               && projection[0].name == "*Model_Space"
               && projection[0].layoutHandle == 0x30u
               && projection[0].xDictHandle == 0x40u
               && projection[1].handle == 0x11u
               && projection[1].layoutHandle == 0x31u
               && projection[2].layoutHandle == 0u
               && a.layoutHandle == 0u && b.layoutHandle == 0u,
           "R2000 projection preserves source and produces writer identities");
    const auto projectionRefused = [&](const char* label) {
        expect(!project(DRW::AC1015) && projection.size() == 3u
                   && projection[0].layoutHandle == 0x30u
                   && projection[1].layoutHandle == 0x31u
                   && a.layoutHandle == 0u && b.layoutHandle == 0u, label);
    };
    expect(!project(DRW::AC1014) && !project(DRW::AC1018)
               && projection.size() == 3u,
           "unqualified target versions leave the projection unchanged");
    second.paperSpaceBlockRecordHandle.ref = 0x10u;
    projectionRefused("duplicate target preserves projected and source state");
    second.paperSpaceBlockRecordHandle.ref = 0x19u;
    projectionRefused("missing target preserves projected and source state");
    second.paperSpaceBlockRecordHandle.ref = 0x11u;
    b.handle = 0x19u;
    projectionRefused("changed source identity preserves projected and source state");
    b.handle = 0x11u;
    auto* savedRecord = records[0x11u];
    records[0x11u] = nullptr;
    projectionRefused("missing source record preserves projected and source state");
    records[0x11u] = savedRecord;
    auto* savedLayout = layouts[0x31u];
    layouts[0x31u] = nullptr;
    projectionRefused("missing source layout preserves projected and source state");
    layouts[0x31u] = savedLayout;
    second.handle = 0x39u;
    projectionRefused("changed layout identity preserves projected and source state");
    second.handle = 0x31u;
    extra.layoutHandle = 0x30u;
    expect(!project(DRW::AC1015) && projection.size() == 3u
               && projection[0].layoutHandle == 0x30u,
           "unexpected source backlink preserves prior projection");
    extra.layoutHandle = 0u;
    expect(project(DRW::AC1015) && projection.size() == 3u
               && projection[1].layoutHandle == 0x31u,
           "corrected R2000 projection succeeds after failure");
    expect(!run(Mode::Reciprocal), "newer sources cannot omit backlinks");
    a.layoutHandle = first.handle; b.layoutHandle = second.handle;
    expect(run(Mode::Reciprocal) && result == expected,
           "reciprocal graph must resolve");
    extra.layoutHandle = first.handle;
    expect(!run(Mode::Reciprocal), "extra nonreciprocal backlink must refuse");
    extra.layoutHandle = 0u;
    expect(run(Mode::Reciprocal) && result == expected,
           "corrected reciprocal graph must succeed after refusal");
    return failures ? 1 : 0;
}
