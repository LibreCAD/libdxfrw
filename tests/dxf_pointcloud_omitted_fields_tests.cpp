// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdxfrw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
std::string contents(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
class Writer final : public LC3CompatCheck {
public:
    explicit Writer(dxfRW& w) : out(w) {}
    void writeEntities() override {
        ok = extended ? out.writePointCloudEx(&cloudEx) : out.writePointCloud(&cloud);
    }
    bool finalizeDxfWrite() override { return ok; }
    dxfRW& out;
    bool extended=false, ok=false;
    DRW_PointCloud cloud;
    DRW_PointCloudEx cloudEx;
};
class Reader final : public LC3CompatCheck {
public:
    void addPointCloud(const DRW_PointCloud* value) override { if (value) {cloud=*value; ++count;} }
    void addPointCloudEx(const DRW_PointCloudEx* value) override { if (value) {cloudEx=*value; ++count;} }
    int count=0;
    DRW_PointCloud cloud;
    DRW_PointCloudEx cloudEx;
};
using Change = std::function<void(DRW_PointCloud&)>;
std::vector<Change> classicChanges() {
    return {
        [](DRW_PointCloud& p){p.definitionHandle=0x301u;},
        [](DRW_PointCloud& p){p.reactorHandle=0x302u;},
        [](DRW_PointCloud& p){p.intensityScheme=1;},
        [](DRW_PointCloud& p){p.intensityStyle.minIntensity=1.;},
        [](DRW_PointCloud& p){p.intensityStyle.maxIntensity=1.;},
        [](DRW_PointCloud& p){p.intensityStyle.lowThreshold=1.;},
        [](DRW_PointCloud& p){p.intensityStyle.highThreshold=1.;}
    };
}
bool classic(const std::filesystem::path& path, DRW::Version version, bool binary) {
    for (const auto& change : classicChanges()) {
        { std::ofstream f(path,std::ios::binary|std::ios::trunc); f << "sentinel"; }
        dxfRW output(path.string().c_str()); Writer w(output);
        w.cloud.savedFilename="cloud.rcp"; change(w.cloud);
        const auto before=w.cloud;
        if (output.write(&w,version,binary) || w.ok || contents(path)!="sentinel"
            || w.cloud.handle!=before.handle || w.cloud.definitionHandle!=before.definitionHandle
            || w.cloud.reactorHandle!=before.reactorHandle || w.cloud.intensityScheme!=before.intensityScheme
            || w.cloud.intensityStyle.minIntensity!=before.intensityStyle.minIntensity
            || w.cloud.intensityStyle.maxIntensity!=before.intensityStyle.maxIntensity
            || w.cloud.intensityStyle.lowThreshold!=before.intensityStyle.lowThreshold
            || w.cloud.intensityStyle.highThreshold!=before.intensityStyle.highThreshold)
            return false;
        w.cloud=DRW_PointCloud{}; w.cloud.savedFilename="cloud.rcp";
        if (!output.write(&w,version,binary) || !w.ok) return false;
        dxfRW input(path.string().c_str()); Reader r;
        if (!input.read(&r,false) || r.count!=1 || r.cloud.savedFilename!="cloud.rcp") return false;
    }
    return true;
}
bool extended(const std::filesystem::path& path, DRW::Version version, bool binary) {
    for (int field=0; field<2; ++field) {
        { std::ofstream f(path,std::ios::binary|std::ios::trunc); f << "sentinel"; }
        dxfRW output(path.string().c_str()); Writer w(output); w.extended=true;
        w.cloudEx.definitionHandle=0x301u; w.cloudEx.reactorHandle=0x302u;
        w.cloudEx.name="cropped"; w.cloudEx.croppingCount=1;
        DRW_PointCloudExCropping crop; crop.type=1; crop.pointCount=1; crop.points={{1.,2.,3.}};
        w.cloudEx.croppings.push_back(crop);
        if (field==0) w.cloudEx.unknownInt0=7; else w.cloudEx.unknownInt1=9;
        if (output.write(&w,version,binary) || w.ok || contents(path)!="sentinel"
            || w.cloudEx.handle!=0u || w.cloudEx.croppings.size()!=1
            || w.cloudEx.unknownInt0!=(field==0?7:0) || w.cloudEx.unknownInt1!=(field==1?9:0)) return false;
        w.cloudEx.unknownInt0=0; w.cloudEx.unknownInt1=0;
        if (!output.write(&w,version,binary) || !w.ok) return false;
        dxfRW input(path.string().c_str()); Reader r;
        if (!input.read(&r,false) || r.count!=1 || r.cloudEx.name!="cropped"
            || r.cloudEx.croppings.size()!=1 || r.cloudEx.croppings[0].points.size()!=1
            || r.cloudEx.croppings[0].points[0].z!=3.) return false;
    }
    return true;
}
bool newerClassic(const std::filesystem::path& path, bool binary) {
    dxfRW output(path.string().c_str()); Writer w(output);
    w.cloud.definitionHandle=0x301; w.cloud.reactorHandle=0x302; w.cloud.intensityScheme=2;
    w.cloud.intensityStyle={1.,2.,3.,4.};
    if (!output.write(&w,DRW::AC1027,binary) || !w.ok) return false;
    dxfRW input(path.string().c_str()); Reader r;
    return input.read(&r,false) && r.count==1 && r.cloud.definitionHandle==0x301
        && r.cloud.reactorHandle==0x302 && r.cloud.intensityScheme==2
        && r.cloud.intensityStyle.minIntensity==1. && r.cloud.intensityStyle.maxIntensity==2.
        && r.cloud.intensityStyle.lowThreshold==3. && r.cloud.intensityStyle.highThreshold==4.;
}
bool uncroppedExtended(const std::filesystem::path& path, bool binary) {
    dxfRW output(path.string().c_str()); Writer w(output); w.extended=true;
    w.cloudEx.definitionHandle=0x301; w.cloudEx.reactorHandle=0x302;
    w.cloudEx.unknownInt0=7; w.cloudEx.unknownInt1=9;
    if (!output.write(&w,DRW::AC1027,binary) || !w.ok) return false;
    dxfRW input(path.string().c_str()); Reader r;
    return input.read(&r,false) && r.count==1 && r.cloudEx.croppingCount==0
        && r.cloudEx.unknownInt0==7 && r.cloudEx.unknownInt1==9;
}
}
int main() {
    auto path=std::filesystem::temp_directory_path()/
        ("dxf-pointcloud-fields-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dxf");
    struct Cleanup { std::filesystem::path p; ~Cleanup(){std::error_code e;std::filesystem::remove(p,e);} } cleanup{path};
    for (bool binary : {false,true}) {
        for (auto version : {DRW::AC1021,DRW::AC1024})
            if (!classic(path,version,binary)) { std::cerr << "classic refusal/retry failed\n";return 1; }
        for (auto version : {DRW::AC1027,DRW::AC1032})
            if (!extended(path,version,binary)) { std::cerr << "extended refusal/retry failed\n";return 2; }
        if (!newerClassic(path,binary)) { std::cerr << "newer classic readback failed\n";return 3; }
        if (!uncroppedExtended(path,binary)) { std::cerr << "uncropped extended readback failed\n";return 4; }
    }
    return 0;
}
