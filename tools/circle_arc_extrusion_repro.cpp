// SPDX-License-Identifier: GPL-2.0-or-later
// Reproduce the legacy applyExtrusion -> write frame ambiguity without a
// checked-in drawing. Build from the repository root:
//   c++ -std=c++17 -Isrc tools/circle_arc_extrusion_repro.cpp build-gcc/libdxfrw.a -o /tmp/circle_arc_extrusion_repro
// Run: /tmp/circle_arc_extrusion_repro
// PASS means the old behavior, including its known loss, was reproduced.
#include "../tests/lc3_compat_check.cpp"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {
struct Probe : LC3CompatCheck {
    dxfRW* dxfWriter = nullptr;
    DRW_Circle circle;
    DRW_Arc arc;
    bool gotCircle = false;
    bool gotArc = false;
    bool wrote = false;
    void addCircle(const DRW_Circle& item) override { circle = item; gotCircle = true; }
    void addArc(const DRW_Arc& item) override { arc = item; gotArc = true; }
    void writeEntities() override {
        const bool a = dxfWriter && dxfWriter->writeCircle(&circle);
        const bool b = dxfWriter && dxfWriter->writeArc(&arc);
        wrote = a && b;
    }
};
bool near(double a, double b) { return std::abs(a - b) < 1.e-9; }
bool same(const DRW_Coord& a, const DRW_Coord& b) {
    return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
}
bool same(const DRW_Arc& a, const DRW_Arc& b) {
    return same(a.basePoint, b.basePoint) && near(a.staangle, b.staangle)
        && near(a.endangle, b.endangle);
}
bool saveDxf(const std::filesystem::path& p, Probe& data, bool binary) {
    const std::string filename = p.string();
    dxfRW writer(filename.c_str());
    data.dxfWriter = &writer;
    return writer.write(&data, DRW::AC1027, binary) && data.wrote;
}
bool loadDxf(const std::filesystem::path& p, bool ext, Probe& data) {
    const std::string filename = p.string();
    dxfRW reader(filename.c_str());
    return reader.read(&data, ext) && data.gotCircle && data.gotArc;
}
void show(const char* route, bool binary, bool ext, const Probe& first,
          const Probe& second) {
    std::cout << route << " input=" << (binary ? "binary" : "ascii")
              << " applyExt=" << ext << " circle (" << first.circle.basePoint.x
              << ',' << first.circle.basePoint.y << ',' << first.circle.basePoint.z
              << ") -> (" << second.circle.basePoint.x << ',' << second.circle.basePoint.y
              << ',' << second.circle.basePoint.z << "); arc (" << first.arc.basePoint.x
              << ',' << first.arc.basePoint.y << ',' << first.arc.basePoint.z << ';'
              << first.arc.staangle << ',' << first.arc.endangle << ") -> ("
              << second.arc.basePoint.x << ',' << second.arc.basePoint.y << ','
              << second.arc.basePoint.z << ';' << second.arc.staangle << ','
              << second.arc.endangle << ")\n";
}
} // namespace

int main() {
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-circle-arc-frame-" + std::to_string(nonce));
    if (!std::filesystem::create_directory(dir)) return 2;
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code ec; std::filesystem::remove_all(path, ec);
    }} cleanup{dir};

    Probe authored;
    authored.circle.basePoint = {2, 3, 4};
    authored.circle.extPoint = {0.6, 0, 0.8};
    authored.circle.radious = 2;
    authored.arc.basePoint = {2, 3, 4};
    authored.arc.extPoint = {0, 0, -1};
    authored.arc.radious = 2;
    authored.arc.staangle = 0.25;
    authored.arc.endangle = 1.5;
    if (authored.circle.haveExtrusion || authored.arc.haveExtrusion) return 3;
    for (const bool binary : {false, true}) {
        const auto original = dir / "input.dxf";
        const auto dxfOutput = dir / "output.dxf";
        if (!saveDxf(original, authored, binary)) return 4;
        for (const bool apply : {false, true}) {
            Probe first, second;
            if (!loadDxf(original, apply, first)) { std::cerr << "input DXF read failed\n"; return 5; }
            if (!saveDxf(dxfOutput, first, binary)) { std::cerr << "output DXF write failed\n"; return 5; }
            if (!loadDxf(dxfOutput, apply, second)) { std::cerr << "output DXF read failed\n"; return 5; }
            show("DXF->DXF", binary, apply, first, second);
            const bool circleSame = same(first.circle.basePoint, second.circle.basePoint);
            const bool arcSame = same(first.arc, second.arc);
            if (circleSame != !apply || arcSame != !apply) return 6;
            if (apply
                && (!same(first.circle.basePoint, {0., 2., 5.})
                    || !same(second.circle.basePoint, {1.4, 0., 5.2})
                    || !same(first.arc.basePoint, {-2., 3., -4.})
                    || !same(second.arc.basePoint, {2., 3., 4.})
                    || !near(first.arc.staangle, 1.6415926535897932)
                    || !near(first.arc.endangle, 2.8915926535897932)
                    || !near(second.arc.staangle, 0.25)
                    || !near(second.arc.endangle, 1.5))) return 7;
        }
    }
    std::cout << "Reproduced legacy CIRCLE/ARC frame loss; no drawing retained.\n";
    return 0;
}
