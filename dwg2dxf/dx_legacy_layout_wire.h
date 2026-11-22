// SPDX-License-Identifier: GPL-2.0-or-later
#ifndef DX_LEGACY_LAYOUT_WIRE_H
#define DX_LEGACY_LAYOUT_WIRE_H

#include "dx_legacy_dwg_wire.h"

// Independent, bounded R13/R14 LAYOUT body/tail observer (ODA 20.4.84).
// This checks the modeled physical fields only: callers must still bind the
// class definition, captured frame identity, dictionary graph and output.
namespace dx_legacy_layout_wire {
using dx_legacy_dwg_wire::Bits;
enum class Field : std::size_t { Owner, Reactors, XDictionary, BlockRecord,
                                 Viewport, BaseUcs, NamedUcs, Count };
struct Proof {
    std::uint32_t sourceClass = 0u;
    std::uint32_t bodyEnd = 0u;
    std::array<dx_raster_wire::Span, static_cast<std::size_t>(Field::Count)> handles{};
};

inline bool sameDouble(double value, double expected) noexcept {
    return std::isfinite(value) && std::isfinite(expected)
        && std::memcmp(&value, &expected, sizeof value) == 0;
}
inline bool rawDouble(Bits& bits, double expected) noexcept {
    const auto raw = bits.little(8u);
    double value = 0.0;
    std::memcpy(&value, &raw, sizeof value);
    return bits.good && sameDouble(value, expected);
}
inline bool bitDouble(Bits& bits, double expected) noexcept {
    const auto tag = bits.take(2u);
    double value = 0.0;
    if (tag == 0u) {
        const auto raw = bits.little(8u);
        std::memcpy(&value, &raw, sizeof value);
    } else if (tag == 1u) value = 1.0;
    else if (tag != 2u) return false;
    return bits.good && sameDouble(value, expected);
}
inline bool point(Bits& bits, const DRW_Coord& value) noexcept {
    return bitDouble(bits, value.x) && bitDouble(bits, value.y)
        && bitDouble(bits, value.z);
}
inline bool absolute(Bits& bits, std::uint8_t role,
                     std::uint32_t expected) noexcept {
    const auto token = dx_raster_wire::detail::handle(bits, role, 0u);
    return bits.good && token.value == expected
        && (token.role == role
            || (token.role == 0u && token.length == 0u && expected == 0u));
}
inline bool ascii(const std::string& text) noexcept {
    return std::all_of(text.begin(), text.end(),
                       [](unsigned char c) { return c > 0u && c < 128u; });
}

inline bool layout(const DRW_UnsupportedObject& raw, const DRW_Layout& value,
                   Proof* proof = nullptr) noexcept {
    using namespace dx_legacy_dwg_wire;
    const auto size = raw.m_rawBytes.size();
    if (!supported(raw.m_version) || raw.m_isEntity
        || raw.m_objectType != 82 || raw.m_handle != value.handle
        || raw.m_objectSize != size || size > UINT32_MAX / 8u
        || raw.m_bodyBitSize != 0u || raw.m_hasDataStorage
        || value.handle == 0u || value.parentHandle == 0u
        || value.reactorCount() < 0
        || value.reactorHandles.size() != static_cast<std::size_t>(value.reactorCount())
        || value.hasDataStorageBinaryData() || value.hasDwgCommonLinkTail()
        || !value.appData.empty() || !value.extData.empty()
        || !value.pendingAppIdResolutions.empty() || !value.pendingLayerRefResolutions.empty()
        || value.viewportCount != 0 || !value.viewportHandles.empty()
        || value.plotViewHandle.ref != 0u || value.shadePlotHandle.ref != 0u
        || value.paperSpaceBlockRecordHandle.ref == 0u || value.name.empty()
        || value.tabOrder < 0 || value.plotLayoutFlags < 0 || value.paperUnits < 0
        || value.plotRotation < 0 || value.plotType < 0 || value.scaleType < 0
        || value.layoutFlags < 0 || value.orthoViewType < 0
        || !ascii(value.pageSetupName) || !ascii(value.printerConfig)
        || !ascii(value.paperSize) || !ascii(value.plotViewName)
        || !ascii(value.currentStyleSheet) || !ascii(value.name)) return false;
    Bits b{raw.m_rawBytes.data(), 0u, static_cast<std::uint32_t>(size * 8u)};
    Bits typeProbe = b;
    const auto encodedType = typeProbe.shortValue();
    // Dispatch normalizes the callback to fixed type 82; R13/R14 bytes
    // still contain a custom-class number. The caller must bind that number
    // to the original class table before this can qualify source identity.
    if (!typeProbe.good || encodedType < 500u) return false;
    std::uint32_t bodyEnd = 0u;
    if (!frameStart(b, raw.m_version, encodedType,
                    value.handle, value.reactorHandles.size(),
                    [](Bits& source) noexcept { return source.shortValue() == 0u && source.good; },
                    bodyEnd)) return false;
    const auto d = [&b](double v) noexcept {
        return bitDouble(b, v);
    };
    const auto bs = [&b](std::int32_t v) noexcept {
        return v >= 0 && b.shortValue() == static_cast<std::uint32_t>(v) && b.good;
    };
    if (!text(b, value.pageSetupName) || !text(b, value.printerConfig)
        || !bs(value.plotLayoutFlags) || !d(value.marginLeft) || !d(value.marginBottom)
        || !d(value.marginRight) || !d(value.marginTop) || !d(value.paperWidth)
        || !d(value.paperHeight) || !text(b, value.paperSize)
        || !d(value.plotOriginX) || !d(value.plotOriginY) || !bs(value.paperUnits)
        || !bs(value.plotRotation) || !bs(value.plotType)
        || !d(value.windowMinX) || !d(value.windowMinY)
        || !d(value.windowMaxX) || !d(value.windowMaxY) || !text(b, value.plotViewName)
        || !d(value.realWorldUnits) || !d(value.drawingUnits)
        || !text(b, value.currentStyleSheet) || !bs(value.scaleType)
        || !d(value.scaleFactor) || !d(value.paperImageOriginX) || !d(value.paperImageOriginY)
        || !text(b, value.name) || b.longValue() != static_cast<std::uint32_t>(value.tabOrder)
        || !bs(value.layoutFlags) || !point(b, value.ucsOrigin)
        || !rawDouble(b, value.limMinX) || !rawDouble(b, value.limMinY)
        || !rawDouble(b, value.limMaxX) || !rawDouble(b, value.limMaxY)
        || !point(b, value.insPoint) || !point(b, value.ucsXAxis) || !point(b, value.ucsYAxis)
        || !d(value.elevation) || !bs(value.orthoViewType)
        || !point(b, value.extMin) || !point(b, value.extMax)
        || !b.good || b.pos != bodyEnd) return false;
    Proof staged;
    staged.sourceClass = encodedType; staged.bodyEnd = bodyEnd;
    const auto mark = [&staged](Field field, std::uint32_t begin, std::uint32_t end) {
        staged.handles[static_cast<std::size_t>(field)] = {begin, end, true};
    };
    auto start = b.pos;
    if (!reference(b, 4u, value.handle, value.parentHandle)) return false;
    mark(Field::Owner, start, b.pos);
    start = b.pos;
    for (const auto reactor : value.reactorHandles)
        if (!absolute(b, 4u, reactor)) return false;
    mark(Field::Reactors, start, b.pos);
    const auto observe = [&b, &mark](Field field, std::uint8_t role,
                                     std::uint32_t expected) {
        const auto begin = b.pos;
        if (!absolute(b, role, expected)) return false;
        mark(field, begin, b.pos);
        return true;
    };
    const auto observeTyped = [&b, &mark](Field field, std::uint8_t role,
                                          const dwgHandle& expected) {
        const auto begin = b.pos;
        const auto token = dx_raster_wire::detail::handle(b, role, 0u);
        if (!b.good || token.value != expected.ref || expected.ref64 != expected.ref
            || token.role != expected.rawCode || token.role != expected.code
            || token.length != expected.size
            || (token.role != role && !(token.role == 0u && token.length == 0u)))
            return false;
        mark(field, begin, b.pos);
        return true;
    };
    if (!observe(Field::XDictionary, 3u, value.xDictHandle)
        || !observeTyped(Field::BlockRecord, 4u, value.paperSpaceBlockRecordHandle)
        || !observeTyped(Field::Viewport, 4u, value.lastActiveViewportHandle)
        || !observeTyped(Field::BaseUcs, 5u, value.baseUcsHandle)
        || !observeTyped(Field::NamedUcs, 5u, value.namedUcsHandle)
        || !padding(b)) return false;
    if (proof != nullptr) *proof = staged;
    return true;
}
} // namespace dx_legacy_layout_wire
#endif
