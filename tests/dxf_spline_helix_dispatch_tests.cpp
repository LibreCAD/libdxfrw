// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>

namespace {
void populate(DRW_Spline& spline) {
    spline.degree = 1;
    spline.fitlist.push_back(std::make_shared<DRW_Coord>(1., 0., 0.));
    spline.fitlist.push_back(std::make_shared<DRW_Coord>(0., 1., 2.));
}
void populate(DRW_Helix& helix) {
    populate(static_cast<DRW_Spline&>(helix));
    helix.m_majorVersion = 1;
    helix.m_maintVersion = 2;
    helix.axisBasePt = {3., 4., 5.};
    helix.startPt = {4., 4., 5.};
    helix.axisVector = {0., 0., 1.};
    helix.radius = 4.5;
    helix.turns = 3.25;
    helix.turnHeight = 2.75;
    helix.handedness = true;
    helix.constraintType = 2;
}
struct DerivedHelix : DRW_Helix { int extra = 17; };
class Writer : public LC3CompatCheck {
public:
    explicit Writer(dxfRW& writer) : out(writer) {}
    void writeEntities() override { ok = out.writeSpline(entity); }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out;
    DRW_Spline* entity = nullptr;
    bool ok = false;
};
class TypedReader : public LC3CompatCheck {
public:
    void addSpline(const DRW_Spline*) override { ++splines; }
    void addHelix(const DRW_Helix* value) override {
        ++helices;
        ok = value && value->m_majorVersion == 1 && value->m_maintVersion == 2
            && value->axisBasePt.x == 3. && value->axisBasePt.y == 4.
            && value->axisBasePt.z == 5. && value->axisVector.z == 1.
            && value->startPt.x == 4. && value->radius == 4.5
            && value->turns == 3.25 && value->turnHeight == 2.75
            && value->handedness && value->constraintType == 2
            && value->fitlist.size() == 2;
    }
    int splines = 0, helices = 0;
    bool ok = false;
};
class SplineOnlyReader : public LC3CompatCheck {
public:
    void addSpline(const DRW_Spline* value) override {
        ++calls;
        ok = value && value->fitlist.size() == 2
            && value->fitlist[1] && value->fitlist[1]->z == 2.;
    }
    int calls = 0;
    bool ok = false;
};
std::string contents(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
bool exercise(const std::filesystem::path& path, bool binary) {
    dxfRW output(path.string().c_str());
    Writer writer(output);
    DRW_Helix helix;
    populate(helix);
    writer.entity = &helix; // intentionally pass the derived object as a spline
    if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok)
        return false;
    dxfRW typedInput(path.string().c_str());
    TypedReader typed;
    if (!typedInput.read(&typed, false) || typed.helices != 1
        || typed.splines != 0 || !typed.ok)
        return false;
    dxfRW fallbackInput(path.string().c_str());
    SplineOnlyReader fallback;
    if (!fallbackInput.read(&fallback, false) || fallback.calls != 1 || !fallback.ok)
        return false;

    DRW_Spline spline;
    populate(spline);
    writer.entity = &spline;
    if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok)
        return false;
    dxfRW splineInput(path.string().c_str());
    TypedReader plain;
    if (!splineInput.read(&plain, false) || plain.splines != 1 || plain.helices != 0)
        return false;

    for (int mode : {0, 1}) {
        { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
        DerivedHelix derived;
        populate(derived);
        DRW_Helix wrongTag;
        populate(wrongTag);
        wrongTag.eType = DRW::SPLINE;
        writer.entity = mode == 0 ? static_cast<DRW_Spline*>(&derived)
                                  : static_cast<DRW_Spline*>(&wrongTag);
        if (output.write(&writer, DRW::AC1027, binary) || writer.ok
            || contents(path) != "sentinel")
            return false;
        writer.entity = &helix;
        if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok)
            return false;
        dxfRW retryInput(path.string().c_str());
        TypedReader retry;
        if (!retryInput.read(&retry, false) || retry.helices != 1 || !retry.ok)
            return false;
    }
    { std::ofstream existing(path, std::ios::binary | std::ios::trunc); existing << "sentinel"; }
    writer.entity = &helix;
    if (output.write(&writer, DRW::AC1009, binary) || writer.ok
        || contents(path) != "sentinel")
        return false;
    if (!output.write(&writer, DRW::AC1027, binary) || !writer.ok)
        return false;
    return true;
}
}
int main() {
    const auto path = std::filesystem::temp_directory_path()
        / ("dxf-spline-helix-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".dxf");
    struct Cleanup { std::filesystem::path path; ~Cleanup() {
        std::error_code ignored; std::filesystem::remove(path, ignored); } } cleanup{path};
    for (bool binary : {false, true}) {
        if (!exercise(path, binary)) {
            std::cerr << "spline/helix dispatch failed: binary=" << binary << '\n';
            return 1;
        }
    }
    return 0;
}
