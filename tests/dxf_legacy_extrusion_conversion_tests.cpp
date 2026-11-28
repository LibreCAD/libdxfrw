// SPDX-License-Identifier: GPL-2.0-or-later
#include "lc3_compat_check.cpp"
#include "drw_dxf_extrusion.h"
#include "libdwgr.h"

#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

namespace {
constexpr double tolerance = 1.e-9;
bool near(double a, double b) { return std::abs(a - b) < tolerance; }
bool same(const DRW_Coord& a, const DRW_Coord& b) {
    return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
}
bool same(const DRW_Circle& a, const DRW_Circle& b) {
    return same(a.basePoint, b.basePoint) && same(a.extPoint, b.extPoint)
        && near(a.radious, b.radious) && near(a.thickness, b.thickness)
        && a.haveExtrusion == b.haveExtrusion && a.layer == b.layer;
}
bool same(const DRW_Arc& a, const DRW_Arc& b) {
    return same(static_cast<const DRW_Circle&>(a), static_cast<const DRW_Circle&>(b))
        && near(a.staangle, b.staangle) && near(a.endangle, b.endangle)
        && a.isccw == b.isccw;
}
struct Adapter : LC3CompatCheck {
    std::vector<DRW_Circle> circles;
    std::vector<DRW_Arc> arcs;
    dxfRW* writer = nullptr;
    bool convert = false;
    bool failed = false;
    void addCircle(const DRW_Circle& value) override { circles.push_back(value); }
    void addArc(const DRW_Arc& value) override { arcs.push_back(value); }
    void writeEntities() override {
        failed = false;
        for (const auto& value : circles) {
            DRW_Circle output;
            if (convert && !DRW_DxfLegacyExtrusion::toNativeOcs(value, output)) {
                failed = true;
                return;
            }
            if (!convert) output = value;
            if (!writer || !writer->writeCircle(&output)) { failed = true; return; }
        }
        for (const auto& value : arcs) {
            DRW_Arc output;
            if (convert && !DRW_DxfLegacyExtrusion::toNativeOcs(value, output)) {
                failed = true;
                return;
            }
            if (!convert) output = value;
            if (!writer || !writer->writeArc(&output)) { failed = true; return; }
        }
    }
    bool finalizeDxfWrite() override { return !failed; }
};
Adapter authored() {
    Adapter result;
    const std::array<DRW_Coord, 6> normals{{
        {0., 0., 1.}, {0., 0., -1.}, {0.6, 0., 0.8},
        {3., 4., -5.}, {0.015625, 0., 1.}, {0.0158, 0., 1.}
    }};
    for (const auto& normal : normals) {
        DRW_Circle circle;
        circle.basePoint = {2., 3., 4.};
        circle.extPoint = normal;
        circle.radious = 2.;
        circle.thickness = 1.5;
        result.circles.push_back(circle);
        DRW_Arc arc;
        arc.basePoint = {2., 3., 4.};
        arc.extPoint = normal;
        arc.radious = 3.;
        arc.thickness = 0.5;
        arc.staangle = 0.25;
        arc.endangle = 1.5;
        result.arcs.push_back(arc);
    }
    return result;
}
bool writeDxf(const std::filesystem::path& path, Adapter& data, bool binary) {
    const std::string name = path.string();
    dxfRW writer(name.c_str());
    data.writer = &writer;
    return writer.write(&data, DRW::AC1027, binary) && !data.failed;
}
struct DwgAdapter final : Adapter, DRW_DwgDefaultMaterialProvider,
                          DRW_DwgDefaultPlotStyleProvider {
    dwgRW* dwgWriter = nullptr;
    bool wantsDwgDefaultMaterialGraph() const override { return true; }
    bool wantsDwgDefaultPlotStyleGraph() const override { return true; }
    bool dwgDefaultMaterialOpaqueReferencesAbsent(std::uint32_t a, std::uint32_t b,
            std::uint32_t c, std::uint32_t d) const override { return a && b && c && d; }
    void dwgDefaultMaterialGraphPlanned(std::uint32_t, std::uint32_t,
                                       std::uint32_t, std::uint32_t) override {}
    void writeHeader(DRW_Header& header) override {
        for (const char* key : {"$CMATERIAL", "CMATERIAL"}) {
            const auto found = header.vars.find(key);
            if (found != header.vars.end()) { delete found->second; header.vars.erase(found); }
        }
    }
    void writeEntities() override {
        failed = false;
        for (const auto& value : circles) {
            DRW_Circle output;
            if (convert && !DRW_DxfLegacyExtrusion::toNativeOcs(value, output)) { failed = true; return; }
            if (!convert) output = value;
            output.handle = 0u;
            if (!dwgWriter || !dwgWriter->writeCircle(&output)) { failed = true; return; }
        }
        for (const auto& value : arcs) {
            DRW_Arc output;
            if (convert && !DRW_DxfLegacyExtrusion::toNativeOcs(value, output)) { failed = true; return; }
            if (!convert) output = value;
            output.handle = 0u;
            if (!dwgWriter || !dwgWriter->writeArc(&output)) { failed = true; return; }
        }
    }
    bool finalizeDwgWrite() override { return !failed; }
};
bool writeDwg(dwgRW& writer, DwgAdapter& data) {
    data.dwgWriter = &writer;
    return writer.write(&data, DRW::AC1024, true) && !data.failed;
}
bool readDwg(const std::filesystem::path& path, Adapter& data, bool apply) {
    const std::string name = path.string();
    dwgR reader(name.c_str());
    return reader.read(&data, apply);
}
bool readDxf(const std::filesystem::path& path, Adapter& data, bool apply) {
    const std::string name = path.string();
    dxfRW reader(name.c_str());
    return reader.read(&data, apply);
}
bool sameGeometry(const Adapter& a, const Adapter& b, bool compareFlag = true) {
    if (a.circles.size() != b.circles.size() || a.arcs.size() != b.arcs.size())
        return false;
    for (std::size_t i = 0; i < a.circles.size(); ++i) {
        DRW_Circle left = a.circles[i], right = b.circles[i];
        if (!compareFlag) left.haveExtrusion = right.haveExtrusion;
        if (!same(left, right)) return false;
    }
    for (std::size_t i = 0; i < a.arcs.size(); ++i) {
        DRW_Arc left = a.arcs[i], right = b.arcs[i];
        if (!compareFlag) left.haveExtrusion = right.haveExtrusion;
        if (!same(left, right)) return false;
    }
    return true;
}
bool roundTrip(const std::filesystem::path& dir, bool binary) {
    const auto original = dir / (binary ? "binary-original.dxf" : "ascii-original.dxf");
    const auto converted = dir / (binary ? "binary-converted.dxf" : "ascii-converted.dxf");
    Adapter source = authored();
    if (!writeDxf(original, source, binary)) return false;
    Adapter raw, transformed;
    if (!readDxf(original, raw, false) || !readDxf(original, transformed, true)) return false;
    Adapter sourceSnapshot = transformed;
    transformed.convert = true;
    if (!writeDxf(converted, transformed, binary)
        || !sameGeometry(sourceSnapshot, transformed)) return false;
    Adapter gotRaw, gotTransformed;
    if (!readDxf(converted, gotRaw, false) || !readDxf(converted, gotTransformed, true))
        return false;
    return sameGeometry(raw, gotRaw) && sameGeometry(sourceSnapshot, gotTransformed);
}
bool dwgRoundTrip(const std::filesystem::path& dir) {
    const auto original = dir / "ac1024-original.dwg";
    const auto converted = dir / "ac1024-converted.dwg";
    DwgAdapter source;
    static_cast<Adapter&>(source) = authored();
    dwgRW first(original.string().c_str());
    if (!writeDwg(first, source)) return false;
    Adapter raw, applied;
    if (!readDwg(original, raw, false) || !readDwg(original, applied, true)) return false;
    const Adapter before = applied;
    DwgAdapter output;
    static_cast<Adapter&>(output) = applied;
    output.convert = true;
    dwgRW second(converted.string().c_str());
    if (!writeDwg(second, output) || !sameGeometry(before, output)) return false;
    Adapter resultRaw, resultApplied;
    if (!readDwg(converted, resultRaw, false) || !readDwg(converted, resultApplied, true)) return false;
    return sameGeometry(raw, resultRaw) && sameGeometry(before, resultApplied);
}
bool dwgRefusalAndRetry(const std::filesystem::path& path) {
    DwgAdapter input;
    static_cast<Adapter&>(input) = authored();
    input.circles[1].extPoint = {0., 0., 0.};
    input.circles[1].haveExtrusion = true;
    input.convert = true;
    { std::ofstream destination(path, std::ios::binary); destination << "unchanged"; }
    dwgRW writer(path.string().c_str());
    if (writeDwg(writer, input)) return false;
    std::ifstream previous(path, std::ios::binary);
    if (std::string((std::istreambuf_iterator<char>(previous)), {}) != "unchanged") return false;
    previous.close();
    input.circles[1].extPoint = {0., 0., -1.};
    return writeDwg(writer, input);
}
bool refusalAndRetry(const std::filesystem::path& path, bool binary) {
    Adapter input = authored();
    // Fail after one record has already been staged.
    input.circles[1].extPoint = {0., 0., 0.};
    input.circles[1].haveExtrusion = true;
    input.convert = true;
    { std::ofstream destination(path, std::ios::binary); destination << "unchanged"; }
    const std::string name = path.string();
    dxfRW writer(name.c_str());
    input.writer = &writer;
    if (writer.write(&input, DRW::AC1027, binary)) return false;
    { std::ifstream destination(path, std::ios::binary);
      const std::string actual((std::istreambuf_iterator<char>(destination)), {});
      if (actual != "unchanged") return false; }
    input.circles[1].extPoint = {0., 0., 1.};
    if (!writer.write(&input, DRW::AC1027, binary) || input.failed) return false;
    Adapter read;
    return readDxf(path, read, false) && sameGeometry(input, read, false);
}
bool directControls() {
    DRW_Circle source;
    source.basePoint = {2., 3., 4.};
    source.extPoint = {0.6, 0., 0.8};
    source.radious = 3.;
    // An authored object can have a nondefault normal and a false flag.
    // applyExtrusion then does nothing; the explicit inverse must also do so.
    source.applyExtrusion();
    DRW_Circle output;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(source, output) || !same(source, output))
        return false;
    source.haveExtrusion = true;
    const DRW_Circle native = source;
    source.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(source, source) || !same(source, native))
        return false;
    DRW_Circle unchanged;
    unchanged.basePoint = {91., 92., 93.};
    unchanged.radious = 42.;
    const DRW_Circle before = unchanged;
    // The unscaled z dot product overflows after its first two terms, even
    // though the third term cancels it to a finite native OCS coordinate.
    // Explicit recovery must scale before summation instead of refusing this
    // finite transform.
    const double maximum = std::numeric_limits<double>::max();
    DRW_Circle cancellation;
    cancellation.basePoint = {-0.9 * maximum, -0.9 * maximum,
                               0.32 * maximum};
    cancellation.extPoint = {1., 1., 1.};
    cancellation.haveExtrusion = true;
    DRW_Circle recovered;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellation, recovered)
        || !std::isfinite(recovered.basePoint.x)
        || !std::isfinite(recovered.basePoint.y)
        || !std::isfinite(recovered.basePoint.z)
        || std::fabs(recovered.basePoint.x) > 1.e-15 * maximum
        || !near(recovered.basePoint.y / maximum, 2.44 / std::sqrt(6.))
        || !near(recovered.basePoint.z / maximum, -1.48 / std::sqrt(3.)))
        return false;
    const auto matchesCancellation = [&](const DRW_Coord& point) {
        return std::isfinite(point.x) && std::isfinite(point.y)
            && std::isfinite(point.z)
            && std::fabs(point.x / maximum) < 1.e-15
            && near(point.y / maximum, 2.44 / std::sqrt(6.))
            && near(point.z / maximum, -1.48 / std::sqrt(3.));
    };
    DRW_Arc cancellationArc;
    cancellationArc.basePoint = cancellation.basePoint;
    cancellationArc.extPoint = cancellation.extPoint;
    cancellationArc.haveExtrusion = true;
    DRW_Arc recoveredArc;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellationArc, recoveredArc)
        || !matchesCancellation(recoveredArc.basePoint)) return false;
    DRW_Trace cancellationTrace;
    cancellationTrace.basePoint = cancellationTrace.secPoint
        = cancellationTrace.thirdPoint = cancellationTrace.fourPoint
        = cancellation.basePoint;
    cancellationTrace.extPoint = cancellation.extPoint;
    cancellationTrace.haveExtrusion = true;
    DRW_Trace recoveredTrace;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellationTrace, recoveredTrace)
        || !matchesCancellation(recoveredTrace.basePoint)
        || !matchesCancellation(recoveredTrace.secPoint)
        || !matchesCancellation(recoveredTrace.thirdPoint)
        || !matchesCancellation(recoveredTrace.fourPoint)) return false;
    DRW_Solid cancellationSolid;
    cancellationSolid.basePoint = cancellationSolid.secPoint
        = cancellationSolid.thirdPoint = cancellationSolid.fourPoint
        = cancellation.basePoint;
    cancellationSolid.extPoint = cancellation.extPoint;
    cancellationSolid.haveExtrusion = true;
    DRW_Solid recoveredSolid;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellationSolid, recoveredSolid)
        || !matchesCancellation(recoveredSolid.basePoint)
        || !matchesCancellation(recoveredSolid.secPoint)
        || !matchesCancellation(recoveredSolid.thirdPoint)
        || !matchesCancellation(recoveredSolid.fourPoint)) return false;
    DRW_Text cancellationText;
    cancellationText.basePoint = cancellationText.secPoint = cancellation.basePoint;
    cancellationText.extPoint = cancellation.extPoint;
    cancellationText.haveExtrusion = true;
    DRW_Text recoveredText;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellationText, recoveredText)
        || !matchesCancellation(recoveredText.basePoint)
        || !matchesCancellation(recoveredText.secPoint)) return false;
    DRW_Insert cancellationInsert;
    cancellationInsert.basePoint = cancellation.basePoint;
    cancellationInsert.extPoint = cancellation.extPoint;
    cancellationInsert.haveExtrusion = true;
    DRW_Insert recoveredInsert;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(cancellationInsert,
                                              recoveredInsert)
        || !matchesCancellation(recoveredInsert.basePoint)) return false;
    for (const DRW_Coord normal : {DRW_Coord{0.,0.,0.},
            DRW_Coord{std::numeric_limits<double>::infinity(),0.,1.},
            DRW_Coord{std::numeric_limits<double>::quiet_NaN(),0.,1.}}) {
        DRW_Circle invalid = native;
        invalid.extPoint = normal;
        if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged)
            || !same(unchanged, before)) return false;
    }
    // The inverse must use the same stable normalization as applyExtrusion,
    // including extreme finite and subnormal normals.
    const double tiny = std::numeric_limits<double>::denorm_min();
    for (const DRW_Coord normal : {DRW_Coord{maximum,maximum/4.,-maximum/2.},
                                   DRW_Coord{tiny,tiny,tiny},
                                   DRW_Coord{0.,0.,tiny}, DRW_Coord{0.,0.,-tiny}}) {
        DRW_Circle extreme = native;
        extreme.extPoint = normal;
        const DRW_Circle expected = extreme;
        extreme.applyExtrusion();
        if (!DRW_DxfLegacyExtrusion::toNativeOcs(extreme, extreme)
            || !same(extreme, expected)) return false;
        DRW_Arc extremeArc;
        extremeArc.basePoint = {2.,3.,4.}; extremeArc.extPoint = normal;
        extremeArc.radious = 2.; extremeArc.staangle = 0.25;
        extremeArc.endangle = 1.5; extremeArc.haveExtrusion = true;
        const DRW_Arc expectedArc = extremeArc;
        extremeArc.applyExtrusion();
        if (!DRW_DxfLegacyExtrusion::toNativeOcs(extremeArc, extremeArc)
            || !same(extremeArc, expectedArc)) return false;
    }
    DRW_Circle nonfinite = native;
    nonfinite.basePoint.x = std::numeric_limits<double>::quiet_NaN();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(nonfinite, unchanged)
        || !same(unchanged, before)) return false;
    // Finite inputs can overflow while being projected; refusal must be atomic.
    DRW_Circle overflow = native;
    overflow.haveExtrusion = true;
    overflow.extPoint = {1., 1., 0.};
    overflow.basePoint = {std::numeric_limits<double>::max(),
                          -std::numeric_limits<double>::max(), 0.};
    if (DRW_DxfLegacyExtrusion::toNativeOcs(overflow, unchanged)
        || !same(unchanged, before)) return false;
    struct DerivedCircle : DRW_Circle {} derived;
    const DRW_Circle& cast = derived;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(cast, unchanged)
        || !same(unchanged, before)) return false;
    DerivedCircle derivedOutput;
    derivedOutput.basePoint = {81., 82., 83.};
    const DRW_Circle derivedBefore = derivedOutput;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(native, derivedOutput)
        || !same(derivedOutput, derivedBefore)) return false;
    struct DerivedArc : DRW_Arc {} derivedArcOutput;
    derivedArcOutput.basePoint = {71.,72.,73.};
    const DRW_Arc derivedArcBefore = derivedArcOutput;
    DRW_Arc arcInput;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(arcInput, derivedArcOutput)
        || !same(derivedArcOutput, derivedArcBefore)) return false;
    DerivedArc derivedArcInput;
    DRW_Arc untouchedArc;
    untouchedArc.basePoint = {61.,62.,63.};
    const DRW_Arc untouchedArcBefore = untouchedArc;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(derivedArcInput, untouchedArc)
        || !same(untouchedArc, untouchedArcBefore)) return false;
    DRW_Circle wrong = native;
    wrong.eType = DRW::ARC;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(wrong, unchanged)
        || !same(unchanged, before)) return false;
    DRW_Arc arc;
    arc.basePoint = {2.,3.,4.}; arc.extPoint = {0.,0.,-1.};
    arc.radious = 2.; arc.staangle = 0.25; arc.endangle = 1.5;
    arc.haveExtrusion = true;
    const DRW_Arc nativeArc = arc;
    arc.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(arc, arc) || !same(arc, nativeArc))
        return false;
    DRW_Arc invalid = nativeArc;
    invalid.staangle = std::numeric_limits<double>::infinity();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, arc) || !same(arc, nativeArc))
        return false;
    const DRW_Circle& arcAsCircle = nativeArc;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(arcAsCircle, unchanged)
        || !same(unchanged, before)) return false;
    return true;
}

bool sameTrace(const DRW_Trace& a, const DRW_Trace& b) {
    return same(a.basePoint, b.basePoint) && same(a.secPoint, b.secPoint)
        && same(a.thirdPoint, b.thirdPoint) && same(a.fourPoint, b.fourPoint)
        && same(a.extPoint, b.extPoint) && near(a.thickness, b.thickness)
        && a.haveExtrusion == b.haveExtrusion && a.layer == b.layer;
}

struct TraceAdapter final : LC3CompatCheck {
    std::vector<DRW_Trace> traces;
    std::vector<DRW_Solid> solids;
    dxfRW* writer = nullptr;
    bool convert = false;
    bool failed = false;
    void addTrace(const DRW_Trace& e) override { traces.push_back(e); }
    void addSolid(const DRW_Solid& e) override { solids.push_back(e); }
    void writeEntities() override {
        failed = false;
        for (const auto& e : traces) {
            DRW_Trace out = e;
            if ((convert && !DRW_DxfLegacyExtrusion::toNativeOcs(e, out))
                || !writer->writeTrace(&out)) { failed = true; return; }
        }
        for (const auto& e : solids) {
            DRW_Solid out = e;
            if ((convert && !DRW_DxfLegacyExtrusion::toNativeOcs(e, out))
                || !writer->writeSolid(&out)) { failed = true; return; }
        }
    }
    bool finalizeDxfWrite() override { return !failed; }
};

template<class T> T authoredTrace(const DRW_Coord& normal) {
    T e;
    e.basePoint = {1., 2., 3.}; e.secPoint = {4., 2., 3.};
    e.thirdPoint = {1., 5., 3.}; e.fourPoint = {4., 5., 3.};
    e.extPoint = normal; e.haveExtrusion = true; e.thickness = 0.5;
    e.layer = "TRACE_TEST";
    return e;
}

bool traceControls(const std::filesystem::path& dir, bool binary) {
    TraceAdapter input;
    for (const DRW_Coord normal : {DRW_Coord{0.,1.,0.}, DRW_Coord{0.,0.,-1.},
                                   DRW_Coord{0.015625,0.,1.}, DRW_Coord{0.0158,0.,1.}}) {
        input.traces.push_back(authoredTrace<DRW_Trace>(normal));
        input.solids.push_back(authoredTrace<DRW_Solid>(normal));
    }
    const auto initial = dir / (binary ? "trace-original-binary.dxf" : "trace-original-ascii.dxf");
    const auto converted = dir / (binary ? "trace-converted-binary.dxf" : "trace-converted-ascii.dxf");
    dxfRW first(initial.string().c_str()); input.writer = &first;
    if (!first.write(&input, DRW::AC1027, binary)) return false;
    TraceAdapter raw, applied;
    dxfRW rawReader(initial.string().c_str());
    dxfRW appliedReader(initial.string().c_str());
    if (!rawReader.read(&raw, false) || !appliedReader.read(&applied, true)
        || raw.traces.size() != 4u || raw.solids.size() != 4u
        || applied.traces.size() != 4u || applied.solids.size() != 4u) return false;
    dxfRW second(converted.string().c_str()); applied.writer = &second; applied.convert = true;
    if (!second.write(&applied, DRW::AC1027, binary)) return false;
    TraceAdapter result; dxfRW finalReader(converted.string().c_str());
    if (!finalReader.read(&result, false) || result.traces.size() != 4u
        || result.solids.size() != 4u) return false;
    for (std::size_t i = 0; i < 4; ++i)
        if (!sameTrace(raw.traces[i], result.traces[i])
            || !sameTrace(raw.solids[i], result.solids[i])) return false;

    DRW_Trace trace = authoredTrace<DRW_Trace>({0.6, 0., 0.8});
    const DRW_Trace original = trace; trace.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(trace, trace) || !sameTrace(trace, original)) return false;
    DRW_Trace noExtrusion = authoredTrace<DRW_Trace>({3., 4., -5.});
    noExtrusion.haveExtrusion = false;
    const DRW_Trace noExtrusionBefore = noExtrusion;
    noExtrusion.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(noExtrusion, noExtrusion)
        || !sameTrace(noExtrusion, noExtrusionBefore)) return false;
    DRW_Trace scaled = authoredTrace<DRW_Trace>({3., 4., -5.});
    const DRW_Trace scaledBefore = scaled; scaled.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(scaled, scaled)
        || !sameTrace(scaled, scaledBefore)) return false;
    DRW_Solid solid = authoredTrace<DRW_Solid>({0., 0., -1.});
    const DRW_Solid originalSolid = solid; solid.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(solid, solid) || !sameTrace(solid, originalSolid)) return false;
    DRW_Trace unchanged = original;
    DRW_Trace invalid = original; invalid.extPoint = {0., 0., 0.};
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged) || !sameTrace(unchanged, original)) return false;
    const DRW_Trace invalidBefore = invalid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, invalid)
        || !sameTrace(invalid, invalidBefore)) return false;
    invalid = original; invalid.fourPoint.x = std::numeric_limits<double>::infinity();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged) || !sameTrace(unchanged, original)) return false;
    invalid = original; invalid.eType = DRW::SOLID;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged) || !sameTrace(unchanged, original)) return false;
    DRW_3Dface face; const DRW_Trace& faceAsTrace = face;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(faceAsTrace, unchanged) || !sameTrace(unchanged, original)) return false;
    DRW_Trace& outputFace = face;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(original, outputFace)) return false;
    struct DerivedTrace : DRW_Trace {} derivedTrace;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(derivedTrace, unchanged)
        || !sameTrace(unchanged, original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(original, derivedTrace)) return false;
    struct DerivedSolid : DRW_Solid {} derivedSolid;
    DRW_Solid unchangedSolid = originalSolid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(derivedSolid, unchangedSolid)
        || !sameTrace(unchangedSolid, originalSolid)
        || DRW_DxfLegacyExtrusion::toNativeOcs(originalSolid, derivedSolid)) return false;

    const auto retry = dir / (binary ? "trace-retry-binary.dxf" : "trace-retry-ascii.dxf");
    { std::ofstream out(retry, std::ios::binary); out << "unchanged"; }
    TraceAdapter retryInput; retryInput.traces = {original, invalid}; retryInput.convert = true;
    dxfRW retryWriter(retry.string().c_str()); retryInput.writer = &retryWriter;
    if (retryWriter.write(&retryInput, DRW::AC1027, binary)) return false;
    { std::ifstream in(retry, std::ios::binary); if (std::string((std::istreambuf_iterator<char>(in)), {}) != "unchanged") return false; }
    retryInput.traces[1] = original;
    return retryWriter.write(&retryInput, DRW::AC1027, binary);
}

bool sameText(const DRW_Text& a, const DRW_Text& b) {
    return same(a.basePoint, b.basePoint) && same(a.secPoint, b.secPoint)
        && same(a.extPoint, b.extPoint) && near(a.thickness, b.thickness)
        && near(a.height, b.height) && near(a.angle, b.angle)
        && near(a.widthscale, b.widthscale) && near(a.oblique, b.oblique)
        && a.text == b.text && a.style == b.style && a.layer == b.layer
        && a.textgen == b.textgen && a.alignH == b.alignH && a.alignV == b.alignV
        && a.haveExtrusion == b.haveExtrusion;
}
struct TextAdapter final : LC3CompatCheck {
    std::vector<DRW_Text> texts;
    dxfRW* writer = nullptr;
    bool convert = false, failed = false;
    void addText(const DRW_Text& e) override { texts.push_back(e); }
    void writeEntities() override {
        failed = false;
        for (const auto& e : texts) {
            DRW_Text out = e;
            if ((convert && !DRW_DxfLegacyExtrusion::toNativeOcs(e, out))
                || !writer->writeText(&out)) { failed = true; return; }
        }
    }
    bool finalizeDxfWrite() override { return !failed; }
};
DRW_Text authoredText(const DRW_Coord& normal) {
    DRW_Text e;
    e.basePoint = {1.,2.,3.}; e.secPoint = {3.,4.,3.};
    e.extPoint = normal; e.haveExtrusion = true; e.thickness = 0.3;
    e.height = 1.5; e.text = "aligned"; e.angle = 25.; e.oblique = 8.;
    e.widthscale = 1.2; e.textgen = 2; e.layer = "TEXT_TEST";
    e.alignH = DRW_Text::HCenter; e.alignV = DRW_Text::VMiddle;
    return e;
}
bool textControls(const std::filesystem::path& dir, bool binary) {
    TextAdapter input;
    for (const DRW_Coord normal : {DRW_Coord{0.,1.,0.}, DRW_Coord{0.,0.,-1.},
                                   DRW_Coord{0.015625,0.,1.}, DRW_Coord{0.0158,0.,1.}})
        input.texts.push_back(authoredText(normal));
    const auto initial = dir / (binary ? "text-original-binary.dxf" : "text-original-ascii.dxf");
    const auto converted = dir / (binary ? "text-converted-binary.dxf" : "text-converted-ascii.dxf");
    dxfRW first(initial.string().c_str()); input.writer = &first;
    if (!first.write(&input, DRW::AC1027, binary)) return false;
    TextAdapter raw, applied;
    dxfRW rawReader(initial.string().c_str()), appliedReader(initial.string().c_str());
    if (!rawReader.read(&raw, false) || !appliedReader.read(&applied, true)
        || raw.texts.size() != 4u || applied.texts.size() != 4u) return false;
    dxfRW second(converted.string().c_str()); applied.writer = &second; applied.convert = true;
    if (!second.write(&applied, DRW::AC1027, binary)) return false;
    TextAdapter result; dxfRW finalReader(converted.string().c_str());
    if (!finalReader.read(&result, false) || result.texts.size() != 4u) return false;
    for (std::size_t i = 0; i < 4; ++i)
        if (!sameText(raw.texts[i], result.texts[i])) return false;

    DRW_Text text = authoredText({3.,4.,-5.}); const DRW_Text original = text;
    text.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(text, text) || !sameText(text, original)) return false;
    text = original; text.haveExtrusion = false; const DRW_Text noOp = text;
    text.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(text, text) || !sameText(text, noOp)) return false;
    DRW_Text unchanged = original, invalid = original;
    invalid.extPoint = {0.,0.,0.}; const DRW_Text invalidBefore = invalid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged)
        || !sameText(unchanged, original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(invalid, invalid)
        || !sameText(invalid, invalidBefore)) return false;
    invalid = original; invalid.secPoint.x = std::numeric_limits<double>::infinity();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged) || !sameText(unchanged, original)) return false;
    invalid = original; invalid.extPoint = {1.,1.,0.};
    invalid.basePoint = {std::numeric_limits<double>::max(),
                         -std::numeric_limits<double>::max(), 0.};
    const DRW_Text overflowBefore = invalid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged)
        || !sameText(unchanged, original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(invalid, invalid)
        || !sameText(invalid, overflowBefore)) return false;
    invalid = original; invalid.eType = DRW::MTEXT;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid, unchanged) || !sameText(unchanged, original)) return false;
    DRW_Attrib attrib; DRW_Attdef attdef; DRW_MText mtext;
    for (const DRW_Text* derived : {static_cast<const DRW_Text*>(&attrib),
                                    static_cast<const DRW_Text*>(&attdef),
                                    static_cast<const DRW_Text*>(&mtext)})
        if (DRW_DxfLegacyExtrusion::toNativeOcs(*derived, unchanged)
            || !sameText(unchanged, original)) return false;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(original, attrib)
        || DRW_DxfLegacyExtrusion::toNativeOcs(original, attdef)
        || DRW_DxfLegacyExtrusion::toNativeOcs(original, mtext)) return false;

    const auto retry = dir / (binary ? "text-retry-binary.dxf" : "text-retry-ascii.dxf");
    { std::ofstream out(retry, std::ios::binary); out << "unchanged"; }
    TextAdapter retryInput; retryInput.texts = {original, invalid}; retryInput.convert = true;
    dxfRW retryWriter(retry.string().c_str()); retryInput.writer = &retryWriter;
    if (retryWriter.write(&retryInput, DRW::AC1027, binary)) return false;
    { std::ifstream in(retry, std::ios::binary); if (std::string((std::istreambuf_iterator<char>(in)), {}) != "unchanged") return false; }
    retryInput.texts[1] = original;
    return retryWriter.write(&retryInput, DRW::AC1027, binary);
}

bool sameInsert(const DRW_Insert& a, const DRW_Insert& b) {
    return same(a.basePoint,b.basePoint) && same(a.extPoint,b.extPoint)
        && near(a.xscale,b.xscale) && near(a.yscale,b.yscale)
        && near(a.zscale,b.zscale) && near(a.angle,b.angle)
        && near(a.colspace,b.colspace) && near(a.rowspace,b.rowspace)
        && a.colcount == b.colcount && a.rowcount == b.rowcount
        && a.haveExtrusion == b.haveExtrusion && a.name == b.name && a.layer == b.layer;
}
struct InsertAdapter final : LC3CompatCheck {
    std::vector<DRW_Insert> inserts;
    dxfRW* writer = nullptr;
    bool convert = false, failed = false;
    void addInsert(const DRW_Insert& e) override { inserts.push_back(e); }
    void writeEntities() override {
        failed = false;
        for (const auto& e : inserts) {
            DRW_Insert out = e;
            if ((convert && !DRW_DxfLegacyExtrusion::toNativeOcs(e,out))
                || !writer->writeInsert(&out)) { failed = true; return; }
        }
    }
    bool finalizeDxfWrite() override { return !failed; }
};
DRW_Insert authoredInsert(const DRW_Coord& normal) {
    DRW_Insert e; e.name = "UNIT"; e.basePoint = {3.,4.,2.};
    e.extPoint = normal; e.haveExtrusion = true; e.xscale = 2.;
    e.yscale = 3.; e.zscale = 4.; e.angle = 0.5; e.layer = "INSERT_TEST";
    e.colcount = 2; e.rowcount = 3; e.colspace = 5.; e.rowspace = 6.;
    return e;
}
bool insertControls(const std::filesystem::path& dir, bool binary) {
    InsertAdapter input;
    for (const DRW_Coord normal : {DRW_Coord{0.,1.,0.}, DRW_Coord{0.,0.,-1.},
                                   DRW_Coord{0.015625,0.,1.}, DRW_Coord{0.0158,0.,1.}})
        input.inserts.push_back(authoredInsert(normal));
    const auto initial = dir / (binary ? "insert-original-binary.dxf" : "insert-original-ascii.dxf");
    const auto converted = dir / (binary ? "insert-converted-binary.dxf" : "insert-converted-ascii.dxf");
    dxfRW first(initial.string().c_str()); input.writer = &first;
    if (!first.write(&input, DRW::AC1027, binary)) return false;
    InsertAdapter raw, applied;
    dxfRW rawReader(initial.string().c_str()), appliedReader(initial.string().c_str());
    if (!rawReader.read(&raw, false) || !appliedReader.read(&applied, true)
        || raw.inserts.size() != 4u || applied.inserts.size() != 4u) return false;
    dxfRW second(converted.string().c_str()); applied.writer = &second; applied.convert = true;
    if (!second.write(&applied, DRW::AC1027, binary)) return false;
    InsertAdapter result; dxfRW finalReader(converted.string().c_str());
    if (!finalReader.read(&result, false) || result.inserts.size() != 4u) return false;
    for (std::size_t i = 0; i < 4; ++i)
        if (!sameInsert(raw.inserts[i], result.inserts[i])) return false;

    DRW_Insert insert = authoredInsert({3.,4.,-5.});
    insert.blockRecH.code = 5; insert.blockRecH.size = 2;
    insert.blockRecH.ref = 42; insert.blockRecH.ref64 = 42;
    const DRW_Insert original = insert; insert.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(insert,insert) || !sameInsert(insert,original)
        || insert.blockRecH.code != 5 || insert.blockRecH.size != 2
        || insert.blockRecH.ref != 42 || insert.blockRecH.ref64 != 42) return false;
    insert = original; insert.haveExtrusion = false; const DRW_Insert noOp = insert;
    insert.applyExtrusion();
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(insert,insert) || !sameInsert(insert,noOp)) return false;
    DRW_Insert unchanged = original, invalid = original;
    invalid.extPoint = {0.,0.,0.}; const DRW_Insert invalidBefore = invalid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(invalid,invalid) || !sameInsert(invalid,invalidBefore)) return false;
    invalid = original; invalid.extPoint = {1.,1.,0.};
    invalid.basePoint = {std::numeric_limits<double>::max(),-std::numeric_limits<double>::max(),0.};
    const DRW_Insert overflowBefore = invalid;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(invalid,invalid) || !sameInsert(invalid,overflowBefore)) return false;
    invalid = original; invalid.eType = DRW::LINE;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    struct DerivedInsert : DRW_Insert {} derived;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(derived,unchanged) || !sameInsert(unchanged,original)
        || DRW_DxfLegacyExtrusion::toNativeOcs(original,derived)) return false;
    invalid = original; invalid.attlist.push_back(std::make_shared<DRW_Attrib>());
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    invalid = original; invalid.attlist.push_back(nullptr);
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    invalid = original; invalid.seqEndEntity = std::make_shared<DRW_SeqEnd>();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    invalid = original; invalid.attribHandles.emplace_back();
    if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    for (int part = 0; part < 4; ++part) {
        invalid = original;
        if (part == 0) invalid.seqendH.code = 3;
        if (part == 1) invalid.seqendH.size = 1;
        if (part == 2) invalid.seqendH.ref = 1;
        if (part == 3) invalid.seqendH.ref64 = 1;
        if (DRW_DxfLegacyExtrusion::toNativeOcs(invalid,unchanged) || !sameInsert(unchanged,original)) return false;
    }
    const auto retry = dir / (binary ? "insert-retry-binary.dxf" : "insert-retry-ascii.dxf");
    { std::ofstream out(retry, std::ios::binary); out << "unchanged"; }
    invalid = original; invalid.attlist.push_back(std::make_shared<DRW_Attrib>());
    InsertAdapter retryInput; retryInput.inserts = {original,invalid}; retryInput.convert = true;
    dxfRW retryWriter(retry.string().c_str()); retryInput.writer = &retryWriter;
    if (retryWriter.write(&retryInput,DRW::AC1027,binary)) return false;
    { std::ifstream in(retry, std::ios::binary); if (std::string((std::istreambuf_iterator<char>(in)), {}) != "unchanged") return false; }
    retryInput.inserts[1] = original;
    return retryWriter.write(&retryInput,DRW::AC1027,binary);
}

template<class T>
bool unstableBasisControl() {
    T source;
    source.basePoint = {0., 1e308, 0.};
    source.extPoint = {1e-170, 1e-170, 1.};
    source.haveExtrusion = true;
    source.layer = "SOURCE";
    source.applyExtrusion();
    const DRW_Coord before = source.basePoint;
    T output;
    output.basePoint = {7., 8., 9.};
    output.layer = "SENTINEL";
    if (DRW_DxfLegacyExtrusion::toNativeOcs(source, output)
        || output.basePoint.x != 7. || output.basePoint.y != 8.
        || output.basePoint.z != 9. || output.layer != "SENTINEL"
        || source.basePoint.x != before.x || source.basePoint.y != before.y
        || source.basePoint.z != before.z || source.layer != "SOURCE") return false;
    if (DRW_DxfLegacyExtrusion::toNativeOcs(source, source)
        || source.basePoint.x != before.x || source.basePoint.y != before.y
        || source.basePoint.z != before.z || source.layer != "SOURCE") return false;
    // The caller asserts no legacy transformation was applied: a nonzero
    // extreme normal remains a no-op and must not trigger the new check.
    source.haveExtrusion = false;
    if (!DRW_DxfLegacyExtrusion::toNativeOcs(source, output)
        || output.basePoint.x != before.x || output.basePoint.y != before.y
        || output.basePoint.z != before.z || output.layer != "SOURCE") return false;
    // Correcting the direction permits retry; uniform subnormal direction
    // components also normalize safely and retain the prior behavior.
    const double tiny = std::numeric_limits<double>::denorm_min();
    for (const DRW_Coord normal : {DRW_Coord{3., 4., -5.},
                                   DRW_Coord{tiny, tiny, tiny}}) {
        source.extPoint = normal;
        source.basePoint = {2., 3., 4.};
        source.haveExtrusion = true;
        source.applyExtrusion();
        if (!DRW_DxfLegacyExtrusion::toNativeOcs(source, output)
            || !same(output.basePoint, DRW_Coord{2., 3., 4.})) return false;
    }
    return true;
}

bool unstableBasisControls(const std::filesystem::path& dir, bool binary) {
    if (!unstableBasisControl<DRW_Circle>() || !unstableBasisControl<DRW_Arc>()
        || !unstableBasisControl<DRW_Trace>() || !unstableBasisControl<DRW_Solid>()
        || !unstableBasisControl<DRW_Text>() || !unstableBasisControl<DRW_Insert>())
        return false;
    const auto path = dir / (binary ? "basis-retry-binary.dxf" : "basis-retry-ascii.dxf");
    { std::ofstream f(path, std::ios::binary); f << "unchanged"; }
    Adapter input; input.convert = true;
    DRW_Circle circle; circle.radious = 1.; circle.basePoint = {0., 1e308, 0.};
    circle.extPoint = {1e-170, 1e-170, 1.}; circle.haveExtrusion = true;
    circle.applyExtrusion(); input.circles.push_back(circle);
    dxfRW writer(path.string().c_str()); input.writer = &writer;
    if (writer.write(&input, DRW::AC1027, binary)) return false;
    { std::ifstream in(path, std::ios::binary);
      if (std::string((std::istreambuf_iterator<char>(in)), {}) != "unchanged") return false; }
    input.circles[0].extPoint = {0., 0., 1.};
    input.circles[0].basePoint = {2., 3., 4.};
    return writer.write(&input, DRW::AC1027, binary);
}
} // namespace

int main(int argc, char** argv) {
    const bool keep = argc == 2 && std::string(argv[1]) == "--keep";
    if (argc > 2 || (argc == 2 && !keep)) return 2;
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto dir = std::filesystem::temp_directory_path()
        / ("libdxfrw-dxf-legacy-extrusion-" + std::to_string(nonce));
    if (!std::filesystem::create_directory(dir)) return 2;
    bool pass = directControls();
    for (bool binary : {false, true}) {
        pass = roundTrip(dir, binary) && pass;
        pass = refusalAndRetry(dir / (binary ? "retry-binary.dxf" : "retry-ascii.dxf"), binary) && pass;
        pass = traceControls(dir, binary) && pass;
        pass = textControls(dir, binary) && pass;
        pass = insertControls(dir, binary) && pass;
        pass = unstableBasisControls(dir, binary) && pass;
    }
    pass = dwgRoundTrip(dir) && pass;
    pass = dwgRefusalAndRetry(dir / "ac1024-retry.dwg") && pass;
    if (keep) std::cout << "OUTPUT " << dir.string() << '\n';
    else { std::error_code error; std::filesystem::remove_all(dir, error); }
    std::cout << "DXF explicit legacy extrusion conversion " << (pass ? "PASS" : "FAIL") << '\n';
    return pass ? 0 : 1;
}
