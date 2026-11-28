// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <vector>

namespace {
struct SplineProbe : DRW_Spline {
    using DRW_Spline::encodeDwgSplineBody;
    bool parseBody(DRW::Version version, dwgBuffer& input, std::uint32_t end) {
        objSize = end; dwgDataEndBit = end;
        return parseDwgSplineBody(version, &input);
    }
};
void populate(DRW_Spline& spline, const std::vector<double>& weights, int flags) {
    spline.degree = 2;
    spline.flags = flags;
    spline.knotslist = {0., 0., 0., 1., 1., 1.};
    spline.controllist = {std::make_shared<DRW_Coord>(0., 0., 0.),
                          std::make_shared<DRW_Coord>(1., 2., 0.),
                          std::make_shared<DRW_Coord>(2., 0., 1.)};
    spline.weightlist = weights;
}
class Writer : public LC3CompatCheck {
public:
    explicit Writer(dxfRW& output) : output_(output) {}
    void writeEntities() override { accepted = output_.writeSpline(entity); }
    bool finalizeDxfWrite() override { return accepted; }
    dxfRW& output_; DRW_Spline* entity = nullptr; bool accepted = false;
};
class Reader : public LC3CompatCheck {
public:
    void addSpline(const DRW_Spline* s) override { capture(s, false); }
    void addHelix(const DRW_Helix* s) override { capture(s, true); }
    void capture(const DRW_Spline* s, bool h) {
        if (s == nullptr) return;
        ++count; helix = h; flags = s->flags; weights = s->weightlist;
    }
    int count = 0, flags = 0; bool helix = false; std::vector<double> weights;
};
std::string contents(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
bool dxfCheck(const std::filesystem::path& path, bool binary, bool isHelix,
              const std::vector<double>& weights, int flags, bool rational) {
    dxfRW file(path.string().c_str()); Writer writer(file);
    DRW_Spline spline; DRW_Helix helix;
    DRW_Spline* entity = isHelix ? static_cast<DRW_Spline*>(&helix) : &spline;
    populate(*entity, weights, flags); writer.entity = entity;
    if (!file.write(&writer, DRW::AC1027, binary) || !writer.accepted
        || entity->weightlist != weights || entity->flags != flags) return false;
    dxfRW input(path.string().c_str()); Reader reader;
    if (!input.read(&reader, false) || reader.count != 1 || reader.helix != isHelix
        || ((reader.flags & 4) != 0) != rational || reader.weights != weights) return false;
    return (reader.flags & flags) == flags;
}
bool dwgCheck(DRW::Version version, const std::vector<double>& weights,
              int flags, bool rational) {
    SplineProbe source; populate(source, weights, flags);
    dwgBufferW out; source.encodeDwgSplineBody(version, &out);
    if (!out.isGood() || source.weightlist != weights || source.flags != flags) return false;
    const auto bits = out.size() * 8u - (out.bitPos() ? 8u - out.bitPos() : 0u);
    if (bits == 0 || bits > std::numeric_limits<std::uint32_t>::max()) return false;
    out.putBitLong(777);
    dwgBuffer wire(out.data().data(), out.size());
    if (wire.getBitLong() != 1) return false;
    if (version > DRW::AC1024) { (void)wire.getBitLong(); (void)wire.getBitLong(); }
    if (wire.getBitLong() != 2 || (wire.getBit() != 0) != rational) return false;
    if ((wire.getBit() != 0) != ((flags & 1) != 0)
        || (wire.getBit() != 0) != ((flags & 2) != 0)) return false;
    (void)wire.getBitDouble(); (void)wire.getBitDouble();
    if (wire.getBitLong() != 6 || wire.getBitLong() != 3
        || (wire.getBit() != 0) != !weights.empty()) return false;
    for (double expected : source.knotslist) if (wire.getBitDouble() != expected) return false;
    for (std::size_t i = 0; i < 3; ++i) {
        const auto point = wire.get3BitDouble(); const auto& expected = *source.controllist[i];
        if (point.x != expected.x || point.y != expected.y || point.z != expected.z) return false;
        if (!weights.empty() && wire.getBitDouble() != weights[i]) return false;
    }
    if (!wire.isGood() || wire.getBitLong() != 777) return false;
    dwgBuffer reinput(out.data().data(), out.size()); SplineProbe result;
    if (!result.parseBody(version, reinput, static_cast<std::uint32_t>(bits))
        || result.weightlist != weights || reinput.getBitLong() != 777) return false;
    // The DWG reader intentionally maps a present weight array to rational,
    // even if the raw Rational bit was clear for an all-unit array.
    return (result.flags & flags) == flags;
}
bool invalidWeight(const std::filesystem::path& path, bool binary) {
    dxfRW file(path.string().c_str()); Writer writer(file); DRW_Spline spline;
    populate(spline, {1., std::numeric_limits<double>::infinity(), 1.}, 0);
    writer.entity = &spline;
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    if (file.write(&writer, DRW::AC1027, binary) || writer.accepted || contents(path) != "sentinel") return false;
    spline.weightlist[1] = std::nextafter(1., 2.);
    if (!file.write(&writer, DRW::AC1027, binary) || !writer.accepted) return false;
    dxfRW input(path.string().c_str()); Reader reader;
    return input.read(&reader, false) && reader.count == 1 && (reader.flags & 4)
        && reader.weights == spline.weightlist;
}
}
int main() {
    const auto path = std::filesystem::temp_directory_path() /
        ("spline-precision-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::error_code e; std::filesystem::remove(path,e); } } cleanup{path};
    const double up = std::nextafter(1., 2.);
    const double down = std::nextafter(1., 0.);
    struct Case { std::vector<double> weights; int flags; bool rational; };
    const std::vector<Case> cases = {
        {{}, 0, false}, {{1.,1.,1.}, 0, false}, {{1.,up,1.}, 0, true},
        {{1.,down,1.}, 0, true}, {{2.,2.,2.}, 0, true},
        {{1.,1.,1.}, 4, true}, {{}, 4, true}, {{1.,up,1.}, 3, true}
    };
    for (const auto& value : cases) {
        for (bool binary : {false, true}) for (bool helix : {false, true})
            if (!dxfCheck(path, binary, helix, value.weights, value.flags, value.rational)) {
                std::cerr << "DXF weight check failed binary=" << binary << " helix=" << helix << '\n'; return 1;
            }
        for (auto v : {DRW::AC1015, DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032})
            if (!dwgCheck(v, value.weights, value.flags, value.rational)) {
                std::cerr << "DWG weight check failed version=" << v << '\n'; return 1;
            }
    }
    for (bool binary : {false,true}) if (!invalidWeight(path,binary)) return 1;
    return 0;
}
