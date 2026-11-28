// SPDX-License-Identifier: GPL-2.0-or-later
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <functional>
#include <iostream>
#include <limits>
#include <vector>

namespace {
class Probe final : public DRW_PointCloudEx {
public:
    using DRW_PointCloudEx::encodeDwg;
    void setType(std::uint16_t value) { oType = value; }
    std::uint16_t type() const { return oType; }
};
Probe cloud() {
    Probe p;
    p.setType(0x777u);
    p.name = "cropped cloud";
    p.classVersion = 1;
    p.croppingCount = 1;
    DRW_PointCloudExCropping c;
    c.type = 1;
    c.isInside = true;
    c.cropXDirection = {1., 0., 0.};
    c.cropYDirection = {0., 1., 0.};
    c.pointCount = 2;
    c.points = {{1., 2., 3.}, {4., 5., 6.}};
    p.croppings.push_back(c);
    return p;
}
using Change = std::function<void(Probe&)>;
bool sameOmittedFields(const Probe& a, const Probe& b) {
    const auto sameDouble = [](double x, double y) {
        return x == y || (std::isnan(x) && std::isnan(y));
    };
    return a.unknownInt0 == b.unknownInt0 && a.unknownInt1 == b.unknownInt1
        && a.stylizationType == b.stylizationType
        && a.intensityColorScheme == b.intensityColorScheme
        && a.currentColorScheme == b.currentColorScheme
        && a.classificationColorScheme == b.classificationColorScheme
        && sameDouble(a.elevationMin,b.elevationMin)
        && sameDouble(a.elevationMax,b.elevationMax)
        && sameDouble(a.intensityMin,b.intensityMin)
        && sameDouble(a.intensityMax,b.intensityMax)
        && a.intensityOutOfRangeBehavior == b.intensityOutOfRangeBehavior
        && a.elevationOutOfRangeBehavior == b.elevationOutOfRangeBehavior
        && a.elevationApplyToFixedRange == b.elevationApplyToFixedRange
        && a.intensityAsGradient == b.intensityAsGradient
        && a.elevationAsGradient == b.elevationAsGradient;
}
std::vector<Change> changes() {
    return {
        [](Probe& p){p.unknownInt0=1;}, [](Probe& p){p.unknownInt1=1;},
        [](Probe& p){p.stylizationType=1;},
        [](Probe& p){p.intensityColorScheme="intensity";},
        [](Probe& p){p.currentColorScheme="current";},
        [](Probe& p){p.classificationColorScheme="classification";},
        [](Probe& p){p.elevationMin=1.;}, [](Probe& p){p.elevationMax=1.;},
        [](Probe& p){p.intensityMin=1.;}, [](Probe& p){p.intensityMax=1.;},
        [](Probe& p){p.intensityMin=std::numeric_limits<double>::quiet_NaN();},
        [](Probe& p){p.intensityOutOfRangeBehavior=1;},
        [](Probe& p){p.elevationOutOfRangeBehavior=1;},
        [](Probe& p){p.elevationApplyToFixedRange=true;},
        [](Probe& p){p.intensityAsGradient=true;},
        [](Probe& p){p.elevationAsGradient=true;}
    };
}
bool direct(DRW::Version version) {
    for (const auto& change : changes()) {
        auto p=cloud(); change(p);
        const auto before=p;
        dwgBufferW body, text, handles;
        body.putRawChar8(0xa5); body.putBit(1);
        text.putRawChar8(0x5a); text.putBit(0);
        handles.putRawChar8(0x3c); handles.putBit(1);
        const auto a=body.data(), b=text.data(), c=handles.data();
        const auto ab=body.bitPos(), bb=text.bitPos(), cb=handles.bitPos();
        if (p.encodeDwg(version, &body, 0, &text, &handles)
            || body.data()!=a || text.data()!=b || handles.data()!=c
            || body.bitPos()!=ab || text.bitPos()!=bb || handles.bitPos()!=cb
            || p.type()!=0x777u || p.handle!=0u || !sameOmittedFields(p,before))
            return false;
        p=cloud();
        if (!p.encodeDwg(version, &body, 0, &text, &handles))
            return false;
    }
    // A zero-crop record still permits the display fields carried by that layout.
    auto p=cloud(); p.croppingCount=0; p.croppings.clear();
    p.stylizationType=1; p.currentColorScheme="current"; p.intensityMin=12.;
    dwgBufferW body, text, handles;
    return p.encodeDwg(version,&body,0,&text,&handles);
}
class Writer final : public LC3CompatCheck {
public:
    explicit Writer(dwgRW& writer) : out(writer) {}
    void writeEntities() override { auto p=cloud(); ok=out.writePointCloudEx(&p); }
    bool finalizeDwgWrite() override { return ok; }
    dwgRW& out; bool ok=false;
};
class Reader final : public LC3CompatCheck {
public:
    void addPointCloudEx(const DRW_PointCloudEx* p) override {
        ok=p && p->name=="cropped cloud" && p->croppingCount==1
            && p->croppings.size()==1 && p->croppings[0].pointCount==2
            && p->croppings[0].points.size()==2
            && p->croppings[0].points[1].z==6.
            && p->stylizationType==0 && p->currentColorScheme.empty();
    }
    bool ok=false;
};
}
int main() {
    const auto path=std::filesystem::temp_directory_path()/
        ("pointcloudex-crop-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".dwg");
    struct Cleanup { std::filesystem::path path; ~Cleanup(){std::error_code e; std::filesystem::remove(path,e);} } cleanup{path};
    for (const auto version : {DRW::AC1027, DRW::AC1032}) {
        if (!direct(version)) { std::cerr << "direct refusal/retry failed\n"; return 1; }
        dwgRW output(path.string().c_str()); Writer writer(output);
        if (!output.write(&writer,version,true) || !writer.ok) { std::cerr << "file write failed\n"; return 2; }
        dwgRW input(path.string().c_str()); Reader reader;
        if (!input.read(&reader,false) || !reader.ok) { std::cerr << "file readback failed\n"; return 3; }
    }
    return 0;
}
