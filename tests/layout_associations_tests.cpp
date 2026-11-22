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
struct Record { std::uint32_t layoutHandle; };
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
    Record a{0u}, b{0u}, extra{0u};
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
