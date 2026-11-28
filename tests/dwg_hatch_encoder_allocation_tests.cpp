// SPDX-License-Identifier: GPL-2.0-or-later
// Generated HATCH DWG encoder allocation rollback and retry controls.
#include "drw_entities.h"
#include "intern/dwgbufferw.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <type_traits>
#include <vector>

namespace {
std::atomic<long> allocationsBeforeFailure{-1};
std::atomic<unsigned long> failedAllocations{0};

bool failAllocation() noexcept {
    const long remaining = allocationsBeforeFailure.load(
        std::memory_order_relaxed);
    if (remaining < 0)
        return false;
    if (remaining == 0) {
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);
        failedAllocations.fetch_add(1u, std::memory_order_relaxed);
        return true;
    }
    allocationsBeforeFailure.fetch_sub(1, std::memory_order_relaxed);
    return false;
}

class HatchProbe final : public DRW_Hatch {
public:
    using DRW_Hatch::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class MPolygonProbe final : public DRW_MPolygon {
public:
    using DRW_MPolygon::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class MLeaderProbe final : public DRW_MLeader {
public:
    using DRW_MLeader::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class ArcAlignedTextProbe final : public DRW_ArcAlignedText {
public:
    using DRW_ArcAlignedText::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class LightProbe final : public DRW_Light {
public:
    using DRW_Light::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class SectionObjectProbe final : public DRW_SectionObject {
public:
    using DRW_SectionObject::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class GeoPositionMarkerProbe final : public DRW_GeoPositionMarker {
public:
    using DRW_GeoPositionMarker::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class CameraProbe final : public DRW_Camera {
public:
    using DRW_Camera::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class PlaneSurfaceProbe final : public DRW_PlaneSurface {
public:
    using DRW_Surface::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

class RawPlaneSurfaceProbe final : public DRW_PlaneSurface {
public:
    using DRW_Surface::encodeDwg;
    std::int16_t rawType() const { return oType; }
};

template <class SurfaceType>
class SurfaceSubtypeProbe final : public SurfaceType {
public:
    using DRW_Surface::encodeDwg;
    std::int16_t rawType() const { return this->oType; }
};

using ExtrudedSurfaceProbe = SurfaceSubtypeProbe<DRW_ExtrudedSurface>;
using RevolvedSurfaceProbe = SurfaceSubtypeProbe<DRW_RevolvedSurface>;
using SweptSurfaceProbe = SurfaceSubtypeProbe<DRW_SweptSurface>;
using LoftedSurfaceProbe = SurfaceSubtypeProbe<DRW_LoftedSurface>;
using NurbsSurfaceProbe = SurfaceSubtypeProbe<DRW_NurbsSurface>;

struct Buffers {
    dwgBufferW body;
    dwgBufferW strings;
    dwgBufferW handles;
};

enum class AliasLayout {
    Separate,
    SharedStringAndHandle,
    BodyAndString,
    BodyAndHandles,
    AllBody
};

void seed(Buffers& buffers) {
    buffers.body.putRawChar8(0xa5u);
    buffers.body.putBit(1u);
    buffers.strings.putRawChar8(0x5au);
    buffers.strings.putBit(0u);
    buffers.handles.putRawChar8(0x3cu);
    buffers.handles.putBit(1u);
}

void route(AliasLayout layout, Buffers& buffers,
           dwgBufferW*& strings, dwgBufferW*& handles) {
    strings = &buffers.strings;
    handles = &buffers.handles;
    switch (layout) {
    case AliasLayout::Separate:
        break;
    case AliasLayout::SharedStringAndHandle:
        handles = strings;
        break;
    case AliasLayout::BodyAndString:
        strings = &buffers.body;
        break;
    case AliasLayout::BodyAndHandles:
        handles = &buffers.body;
        break;
    case AliasLayout::AllBody:
        strings = &buffers.body;
        handles = &buffers.body;
        break;
    }
}

template <class Shape>
Shape makeShape(DRW::Version version) {
    Shape shape;
    shape.handle = 0x40u;
    if constexpr (std::is_same_v<Shape, MLeaderProbe>) {
        shape.styleHandle.ref = 0x50u;
        shape.leaderLineTypeHandle.ref = 0x51u;
        shape.arrowHeadHandle.ref = 0x52u;
        shape.styleTextStyleHandle.ref = 0x53u;
        DRW_MLeaderRoot root;
        root.connectionPoint = {1.0, 2.0, 3.0};
        root.direction = {0.0, 1.0, 0.0};
        root.leaderIndex = 1;
        DRW_MLeaderLeaderLine line;
        line.points.emplace_back(4.0, 5.0, 6.0);
        line.leaderLineIndex = 2;
        line.lineTypeHandle.ref = 0x55u;
        line.arrowHandle.ref = 0x56u;
        root.leaderLines.push_back(line);
        shape.context.roots.push_back(root);
    } else if constexpr (std::is_same_v<Shape, CameraProbe>) {
        shape.m_viewHandle = 0x50u;
    } else if constexpr (std::is_same_v<Shape, PlaneSurfaceProbe>) {
        shape.setDwgClassNum(DRW_PlaneSurface::kDwgClassNum);
        shape.uIsolines = 4;
        shape.vIsolines = 6;
    } else if constexpr (std::is_same_v<Shape, RawPlaneSurfaceProbe>) {
        shape.setDwgClassNum(DRW_PlaneSurface::kDwgClassNum);
        shape.hasRawDwgBody = true;
        shape.rawDwgBodyVersion = version;
        shape.rawDwgBodyBitSize = 13u;
        shape.rawAcisData = {0xA5u, 0x1Fu};
    } else if constexpr (std::is_same_v<Shape, ExtrudedSurfaceProbe>) {
        shape.setDwgClassNum(DRW_ExtrudedSurface::kDwgClassNum);
        shape.modelerFormatVersion = 1;
        shape.uIsolines = 4;
        shape.vIsolines = 5;
        shape.sweepVector = {0.0, 0.0, 1.0};
        shape.referenceVector = {1.0, 0.0, 0.0};
        shape.scaleFactor = 1.25;
    } else if constexpr (std::is_same_v<Shape, RevolvedSurfaceProbe>) {
        shape.setDwgClassNum(DRW_RevolvedSurface::kDwgClassNum);
        shape.modelerFormatVersion = 1;
        shape.axisPoint = {1.0, 2.0, 3.0};
        shape.axisVector = {0.0, 0.0, 1.0};
        shape.revolveAngle = 1.5;
    } else if constexpr (std::is_same_v<Shape, SweptSurfaceProbe>) {
        shape.setDwgClassNum(DRW_SweptSurface::kDwgClassNum);
        shape.modelerFormatVersion = 1;
        shape.sweepEntityId = 4u;
        shape.sweepData = {0x01u, 0xA5u};
        shape.pathEntityId = 5u;
        shape.pathData = {0xFEu, 0x7Fu};
        shape.referenceVector = {1.0, 2.0, 3.0};
        shape.scaleFactor = 1.1;
    } else if constexpr (std::is_same_v<Shape, LoftedSurfaceProbe>) {
        shape.setDwgClassNum(DRW_LoftedSurface::kDwgClassNum);
        shape.modelerFormatVersion = 1;
        shape.numCrossSections = 1;
        shape.numGuideCurves = 1;
        shape.crossSectionHandles = {0x61u};
        shape.guideCurveHandles = {0x62u};
        shape.pathCurveHandle = 0x63u;
        shape.dxfReferenceData.emplace_back(90, std::int32_t{2});
    } else if constexpr (std::is_same_v<Shape, NurbsSurfaceProbe>) {
        shape.setDwgClassNum(DRW_NurbsSurface::kDwgClassNum);
        shape.uIsolines = 12;
        shape.vIsolines = 13;
        shape.short170 = 14;
        shape.cvHullDisplay = true;
        shape.uvec1 = {1.0, 0.0, 0.0};
        shape.vvec1 = {0.0, 1.0, 0.0};
        shape.uvec2 = {2.0, 0.0, 0.0};
        shape.vvec2 = {0.0, 2.0, 0.0};
    } else if constexpr (std::is_same_v<Shape, GeoPositionMarkerProbe>) {
        shape.m_classVersion = 2u;
        shape.m_position = {1.0, 2.0, 3.0};
        shape.m_radius = 2.5;
        shape.m_notes = "AllocationRetryGeoPositionMarker";
        shape.m_landingGap = 0.5;
        shape.m_mtextVisible = true;
        shape.m_textAlignment = 3u;
        shape.m_enableFrameText = version >= DRW::AC1032;
        shape.mtext = std::make_unique<DRW_MText>();
        shape.mtext->text = "embedded allocation retry marker";
        shape.mtext->height = 0.75;
    } else if constexpr (std::is_same_v<Shape, ArcAlignedTextProbe>) {
        shape.m_center = {1.0, 2.0, 0.0};
        shape.m_radius = 12.5;
        shape.m_startAngle = 0.25;
        shape.m_endAngle = 2.5;
        shape.m_textSize = "2.5";
        shape.m_xScale = "0.9";
        shape.m_charSpacing = "1.1";
        shape.m_fontName = "txt";
        shape.m_bigFontName = "bigfont";
        shape.text = "ARC ALIGNED TRANSACTION";
        shape.m_arcHandle = 0x57u;
    } else if constexpr (std::is_same_v<Shape, LightProbe>) {
        shape.m_classVersion = 2u;
        shape.m_name = "AllocationRetryLight";
        shape.m_type = 1u;
        shape.m_status = true;
        shape.m_color = 256u;
        shape.m_colorMethod = 0xC0u;
        shape.m_colorDwgMetadataPresent = true;
        shape.m_plotGlyph = true;
        shape.m_intensity = 1.75;
        shape.m_position = {1.0, 2.0, 3.0};
        shape.m_target = {4.0, 5.0, 6.0};
    } else if constexpr (std::is_same_v<Shape, SectionObjectProbe>) {
        shape.m_state = 3u;
        shape.m_flags = 5u;
        shape.m_name = "AllocationRetrySection";
        shape.m_vertDir = {0.0, 0.0, 1.0};
        shape.m_topHeight = 8.0;
        shape.m_bottomHeight = -2.0;
        shape.m_indicatorColor = 7u;
        shape.m_verts = {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}};
        shape.m_blVerts = {{-1.0, -2.0, -3.0}};
        shape.m_sectionSettingsHandle = 0x61u;
    } else {
        shape.name = "ANSI31";
        shape.solid = false;
        shape.associative = true;
        shape.angle = 17.0 / ARAD;
        shape.scale = 0.75;

        if constexpr (std::is_same_v<Shape, HatchProbe>) {
            shape.pixelSize = 2.5;
            shape.seedPoints = {{1.0, 2.0, 0.0}, {3.0, 4.0, 0.0}};
        } else {
            shape.fillColorAci = 42;
            shape.xDirX = 1.0;
            shape.xDirY = 0.0;
            shape.degenerateLoops = 1;
        }

        DRW_Hatch::PatternLine pattern;
        pattern.angle = 30.0 / ARAD;
        pattern.baseX = 0.25;
        pattern.baseY = 0.5;
        pattern.offsetX = 0.75;
        pattern.offsetY = 1.0;
        pattern.dashList = {1.0, -0.25, 0.5};
        shape.patternLines.push_back(pattern);

        auto loop = std::make_shared<DRW_HatchLoop>(4);
        auto line = std::make_shared<DRW_Line>();
        line->basePoint = {0.0, 0.0, 0.0};
        line->secPoint = {10.0, 0.0, 0.0};
        loop->objlist.push_back(line);
        loop->m_boundaryHandles = {0x60u};
        loop->update();
        shape.appendLoop(loop);
    }
    return shape;
}

bool sameBuffers(const Buffers& left, const Buffers& right) {
    return left.body.data() == right.body.data()
        && left.body.bitPos() == right.body.bitPos()
        && left.strings.data() == right.strings.data()
        && left.strings.bitPos() == right.strings.bitPos()
        && left.handles.data() == right.handles.data()
        && left.handles.bitPos() == right.handles.bitPos();
}

template <class Shape>
bool encode(DRW::Version version, Shape& shape, AliasLayout layout,
            Buffers& buffers) {
    dwgBufferW* strings = nullptr;
    dwgBufferW* handles = nullptr;
    route(layout, buffers, strings, handles);
    return shape.encodeDwg(version, &buffers.body, 0u, strings, handles);
}

template <class Shape>
bool sweep(DRW::Version version, AliasLayout layout,
           std::int16_t expectedType, unsigned long& reachedFailures) {
    Shape clean = makeShape<Shape>(version);
    Buffers cleanBuffers;
    seed(cleanBuffers);
    if (!encode(version, clean, layout, cleanBuffers)
        || clean.rawType() != expectedType) {
        std::cerr << "clean entity encoding failed for version "
                  << static_cast<int>(version) << '\n';
        return false;
    }

    bool reachedSuccess = false;
    for (long failIndex = 0; failIndex < 128; ++failIndex) {
        Shape shape = makeShape<Shape>(version);
        Buffers buffers;
        seed(buffers);
        const Buffers before = buffers;
        const auto oldType = shape.rawType();
        const auto oldHandle = shape.handle;
        const auto failuresBefore = failedAllocations.load(
            std::memory_order_relaxed);

        allocationsBeforeFailure.store(failIndex,
                                       std::memory_order_relaxed);
        const bool encoded = encode(version, shape, layout, buffers);
        allocationsBeforeFailure.store(-1, std::memory_order_relaxed);

        if (encoded) {
            if (!sameBuffers(buffers, cleanBuffers)
                || shape.rawType() != clean.rawType()
                || shape.handle != clean.handle) {
                std::cerr << "entity output differs after allocation sweep\n";
                return false;
            }
            reachedFailures = static_cast<unsigned long>(failIndex);
            reachedSuccess = true;
            break;
        }

        if (failedAllocations.load(std::memory_order_relaxed)
                == failuresBefore
            || !sameBuffers(buffers, before)
            || shape.rawType() != oldType || shape.handle != oldHandle) {
            std::cerr << "entity allocation failure did not roll back\n";
            return false;
        }

        // Same object, same buffers, corrected retry must reproduce clean bytes.
        if (!encode(version, shape, layout, buffers)
            || !sameBuffers(buffers, cleanBuffers)
            || shape.rawType() != clean.rawType()
            || shape.handle != clean.handle) {
            std::cerr << "entity same-object retry failed\n";
            return false;
        }
    }
    if (!reachedSuccess) {
        std::cerr << "HATCH allocation sweep did not reach clean success\n";
        return false;
    }
    return true;
}
} // namespace

void* operator new(std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* memory = std::malloc(size == 0u ? 1u : size))
        return memory;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) {
    if (failAllocation())
        throw std::bad_alloc();
    if (void* memory = std::malloc(size == 0u ? 1u : size))
        return memory;
    throw std::bad_alloc();
}

void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept { std::free(memory); }

int main() {
    const DRW::Version versions[] = {DRW::AC1015, DRW::AC1018,
        DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032};
    const AliasLayout layouts[] = {AliasLayout::Separate,
        AliasLayout::SharedStringAndHandle, AliasLayout::BodyAndString,
        AliasLayout::BodyAndHandles, AliasLayout::AllBody};
    unsigned long totalReachedFailures = 0u;
    for (const auto version : versions) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<HatchProbe>(version, layout, 78, reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
            if (!sweep<MPolygonProbe>(version, layout,
                                     DRW_MPolygon::kDwgClassNum,
                                     reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<PlaneSurfaceProbe>(version, layout, 535,
                                          reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<RawPlaneSurfaceProbe>(version, layout, 535,
                                             reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<ExtrudedSurfaceProbe>(version, layout, 521,
                                             reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
            if (!sweep<RevolvedSurfaceProbe>(version, layout, 531,
                                             reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
            if (!sweep<SweptSurfaceProbe>(version, layout, 533,
                                          reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
            if (!sweep<LoftedSurfaceProbe>(version, layout, 529,
                                           reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
            if (!sweep<NurbsSurfaceProbe>(version, layout, 537,
                                          reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<LightProbe>(version, layout, 502, reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<SectionObjectProbe>(version, layout, 564,
                                           reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version : {DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<GeoPositionMarkerProbe>(version, layout, 1164,
                                               reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const auto version : versions) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<CameraProbe>(version, layout, 542,
                                   reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const auto version : versions) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<ArcAlignedTextProbe>(version, layout, 522,
                                           reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    for (const DRW::Version version :
         {DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const auto layout : layouts) {
            unsigned long reachedFailures = 0u;
            if (!sweep<MLeaderProbe>(version, layout, 501, reachedFailures))
                return 1;
            totalReachedFailures += reachedFailures;
        }
    }
    if (failedAllocations.load(std::memory_order_relaxed) == 0u
        || totalReachedFailures == 0u) {
        std::cerr << "HATCH allocation injector did not fire\n";
        return 1;
    }
    std::cout << "HATCH/MPOLYGON/MLEADER/ARCALIGNEDTEXT/LIGHT/SECTIONOBJECT/"
              << "GEOPOSITIONMARKER/CAMERA/ALL-SURFACE-SUBTYPES/RAW-BODY "
              << "DWG encode allocation "
              << "rollback/retry "
              << "passed across " << sizeof(versions) / sizeof(versions[0])
              << " HATCH-family/ARCALIGNEDTEXT versions, 3 MLEADER versions "
              << "and 4 LIGHT/SECTIONOBJECT versions, "
              << sizeof(layouts) / sizeof(layouts[0])
                     * (sizeof(versions) / sizeof(versions[0]) * 4u + 3u
                        + 8u + 2u + 4u + 20u + 4u)
              << " entity/version/layout profiles; " << totalReachedFailures
              << " allocation offsets exercised\n";
    return 0;
}
