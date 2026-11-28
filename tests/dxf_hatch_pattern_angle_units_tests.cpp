// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"
#include "intern/dxfreader.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

#if !defined(DRW_HATCH_ANGLE_RADIANS) || DRW_HATCH_ANGLE_RADIANS != 1
#error "HATCH angle API must be declared radians"
#endif

namespace {
constexpr double kDegreesToRadians = 3.14159265358979323846 / 180.0;

struct HatchWriter final : LC3CompatCheck {
    HatchWriter(dxfRW& codec, bool mpolygon) : codec(codec), mpolygon(mpolygon) {
        DRW_Hatch& hatch = mpolygon ? static_cast<DRW_Hatch&>(polygon) : plain;
        hatch.name = "ANSI31";
        hatch.solid = 0;
        hatch.angle = 30.0 * kDegreesToRadians;
        DRW_Hatch::PatternLine line;
        line.angle = 75.0 * kDegreesToRadians;
        hatch.patternLines.push_back(line);
        if (mpolygon) {
            polygon.dxfVersion = 1;
            polygon.solid = 0;
        }
    }

    void writeEntities() override {
        accepted = mpolygon ? codec.writeMPolygon(&polygon)
                            : codec.writeHatch(&plain);
    }

    dxfRW& codec;
    bool mpolygon;
    bool accepted{false};
    DRW_Hatch plain;
    DRW_MPolygon polygon;
};

struct HatchReader final : LC3CompatCheck {
    void addHatch(const DRW_Hatch* hatch) override { capture(hatch); }
    void addMPolygon(const DRW_MPolygon* hatch) override { capture(hatch); }

    void capture(const DRW_Hatch* hatch) {
        if (!hatch) return;
        ++count;
        angle = hatch->angle;
        if (!hatch->patternLines.empty())
            lineAngle = hatch->patternLines.front().angle;
    }

    int count{0};
    double angle{0.0};
    double lineAngle{0.0};
};

bool rawDxfAngles(const std::filesystem::path& path, bool binary,
                  const char* target, std::vector<double>& angles) {
    std::ifstream input(path, std::ios::binary);
    if (!input) return false;
    if (binary) input.ignore(22);
    std::unique_ptr<dxfReader> reader = binary
        ? std::unique_ptr<dxfReader>(new dxfReaderBinary(&input))
        : std::unique_ptr<dxfReader>(new dxfReaderAscii(&input));
    int code = 0;
    std::string entity;
    bool foundEntity = false;
    while (reader->readRec(&code)) {
        if (code == 0) {
            entity = reader->getUtf8String();
            foundEntity = foundEntity || entity == target;
        } else if (entity == target && (code == 52 || code == 53)) {
            angles.push_back(reader->getDouble());
        }
    }
    if (!foundEntity || angles.size() != 2u
        || std::fabs(angles[0] - 30.0) >= 1e-10
        || std::fabs(angles[1] - 75.0) >= 1e-10) {
        std::cerr << "raw angles target=" << target << " binary=" << binary
                  << " found=" << foundEntity << " values=";
        for (double angle : angles) std::cerr << ' ' << angle;
        std::cerr << '\n';
        return false;
    }
    return true;
}
} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path()
        / ("libdxfrw-hatch-angle-units-"
           + std::to_string(std::chrono::steady_clock::now()
                                .time_since_epoch().count())
           + ".dxf");
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() { std::error_code error; std::filesystem::remove(path, error); }
    } cleanup{path};

    for (bool binary : {false, true}) {
        for (bool mpolygon : {false, true}) {
            dxfRW writer(path.string().c_str());
            HatchWriter source(writer, mpolygon);
            if (!writer.write(&source, DRW::AC1027, binary) || !source.accepted) {
                std::cerr << "failed to write HATCH angle control\n";
                return 1;
            }
            std::vector<double> wireAngles;
            if (!rawDxfAngles(path, binary, mpolygon ? "MPOLYGON" : "HATCH",
                              wireAngles)) {
                std::cerr << "DXF wire angles were not emitted in degrees\n";
                return 2;
            }
            dxfRW reader(path.string().c_str());
            HatchReader result;
            if (!reader.read(&result, false) || result.count != 1
                || std::fabs(result.angle - 30.0 * kDegreesToRadians) > 1e-12
                || std::fabs(result.lineAngle - 75.0 * kDegreesToRadians) > 1e-12) {
                std::cerr << "DXF HATCH radian API readback mismatch\n";
                return 3;
            }
        }
    }
    std::cout << "DXF HATCH/MPOLYGON degree wire and radian API PASS\n";
    return 0;
}
