#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
#include <utility>

#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwg_fixed_handles.h"
#include "intern/dwgreader.h"

namespace {
void require(bool ok, const char* text) {
    if (!ok) { std::cerr << text << '\n'; std::exit(1); }
}
void put(dwgBufferW& w, unsigned code, std::uint64_t value) {
    dwgHandle h;
    h.code = static_cast<std::uint8_t>(code);
    h.ref = static_cast<std::uint32_t>(value);
    h.ref64 = value;
    w.putHandle(h);
}
struct Frame {
    std::vector<std::uint8_t> bytes;
    std::size_t reactorBit;
};
struct DetachedFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits;
};
Frame frame(bool dictionary, bool aligned, unsigned count, unsigned reactorCode = DRW::DwgSoftPointer,
            std::uint64_t reactorRef = 0x40,
            unsigned declaredCount = std::numeric_limits<unsigned>::max()) {
    dwgBufferW w;
    w.putObjType(DRW::AC1021, DRW::DwgBlockControlObjectType);
    w.putRawLong32(0);
    put(w, 0, 0xf0);
    w.putBitShort(0);
    w.putBitLong(declaredCount == std::numeric_limits<unsigned>::max()
                     ? count : declaredCount);
    w.putBit(dictionary ? 0 : 1);
    w.putBitLong(1);
    w.putBit(0);
    if (aligned) w.alignToByte();
    auto boundary = w.bitCount();
    put(w, DRW::DwgSoftPointer, 0);
    auto reactorBit = w.bitCount();
    for (unsigned i = 0; i < count; ++i) put(w, reactorCode, reactorRef + i);
    if (dictionary) put(w, DRW::DwgHardOwnership, 0x42);
    put(w, DRW::DwgSoftOwnership, 0x43);
    put(w, DRW::DwgHardOwnership, DRW::DwgModelSpaceBlockRecordHandle);
    put(w, DRW::DwgHardOwnership, DRW::DwgPaperSpaceBlockRecordHandle);
    w.alignToByte();
    w.patchRawLong32AtBit(10u, static_cast<std::uint32_t>(boundary));
    return {w.data(), static_cast<std::size_t>(reactorBit)};
}
bool parse(DRW_ObjControl& control, const Frame& f) {
    dwgBuffer b(f.bytes.data(), f.bytes.size());
    return control.parseDwg(DRW::AC1021, &b, 0);
}
DetachedFrame ac1032VportControl(bool includeReactor = true) {
    dwgBufferW body;
    body.putObjType(DRW::AC1032, DRW::DwgVPortControlObjectType);
    put(body, 0, DRW::DwgVportControlHandle);
    body.putBitShort(0); // Empty EED.
    body.putBitLong(includeReactor ? 1 : 0);
    body.putBit(1); // XDictionary is absent.
    body.putBit(0); // No R2013+ binary data.
    body.putBitShort(1); // One VPORT row.
    body.putBit(0); // No detached strings.
    body.alignToByte();
    const std::uint64_t handleStartBit = body.bitCount();

    dwgBufferW handles;
    put(handles, DRW::DwgSoftPointer, 0); // Control owner.
    if (includeReactor)
        put(handles, DRW::DwgSoftPointer, 0x3d126); // Reactor, not a VPORT.
    put(handles, DRW::DwgSoftOwnership, 0x57); // *Active VPORT row.
    handles.alignToByte();
    if (handleStartBit > std::numeric_limits<std::uint32_t>::max()
        || handles.size() > std::numeric_limits<std::uint32_t>::max() / 8u)
        return {};

    DetachedFrame result;
    result.bytes = body.data();
    result.bytes.insert(result.bytes.end(), handles.data().begin(),
                        handles.data().end());
    result.handleBits = static_cast<std::uint32_t>(handles.size() * 8u);
    return result;
}
bool parse(DRW_ObjControl& control, const DetachedFrame& f) {
    if (f.bytes.empty())
        return false;
    dwgBuffer b(f.bytes.data(), f.bytes.size());
    return control.parseDwg(DRW::AC1032, &b, f.handleBits);
}
struct HistoricalAppIdFrame {
    std::vector<std::uint8_t> bytes;
    std::uint32_t handleBits{0};
    std::uint32_t expectedUnclaimedBits{0};
};
HistoricalAppIdFrame historicalAppIdControl(
    DRW::Version version, std::uint32_t unclaimedObjectDataBits = 0u) {
    dwgBufferW body;
    body.putObjType(version, DRW::DwgAppIdControlObjectType);
    std::uint64_t objectSizeBit = 0;
    if (version == DRW::AC1021) {
        objectSizeBit = body.bitCount();
        body.putRawLong32(0u); // Patched to the complete historical inline frame.
    }
    put(body, 0u, DRW::DwgAppIdControlHandle);
    body.putBitShort(0u); // No EED.
    body.putBitLong(0); // No reactors.
    body.putBit(0u); // No extension dictionary.
    if (version > DRW::AC1024)
        body.putBit(0u); // No R2013+ data-storage payload.
    body.putBitShort(1u); // Historical APPID_CONTROL count is BS.
    body.putBit(0u); // No detached strings.

    if (version > DRW::AC1021) {
        for (std::uint32_t bit = 0; bit < unclaimedObjectDataBits; ++bit)
            body.putBit((bit & 1u) != 0u);
    } else if (unclaimedObjectDataBits != 0u) {
        return {};
    }
    const std::uint64_t objectDataEndBit = body.bitCount();
    put(body, DRW::DwgSoftPointer, 0u); // Control owner.
    put(body, DRW::DwgHardOwnership, 0u); // Null XDictionary.
    put(body, DRW::DwgSoftOwnership, 0x12u); // One APPID table record.
    const std::uint64_t handlesEndBit = body.bitCount();
    body.alignToByte();
    if (!body.isGood())
        return {};

    HistoricalAppIdFrame result;
    result.bytes = body.data();
    const std::uint64_t totalBits = result.bytes.size() * 8u;
    if (version == DRW::AC1021) {
        if (totalBits > std::numeric_limits<std::uint32_t>::max())
            return {};
        body.patchRawLong32AtBit(
            objectSizeBit, static_cast<std::uint32_t>(totalBits));
        result.bytes = body.data();
        result.expectedUnclaimedBits = static_cast<std::uint32_t>(
            totalBits - handlesEndBit);
    } else {
        if (totalBits < objectDataEndBit
            || totalBits - objectDataEndBit
                > std::numeric_limits<std::uint32_t>::max())
            return {};
        result.handleBits = static_cast<std::uint32_t>(
            totalBits - objectDataEndBit);
        result.expectedUnclaimedBits = unclaimedObjectDataBits;
    }
    return result;
}
bool parse(DRW_ObjControl& control, DRW::Version version,
           const HistoricalAppIdFrame& f) {
    if (f.bytes.empty())
        return false;
    dwgBuffer body(f.bytes.data(), f.bytes.size());
    return control.parseDwg(version, &body, f.handleBits);
}
void assertReset(const DRW_ObjControl& c) {
    require(c.reactorCount() == 0 && c.reactorHandles.empty() && c.orderedHandleSlots.empty()
            && c.handlesList.empty() && c.phantomHandles.empty() && c.xDictHandle == 0
            && !c.historicalAppIdControlRead
            && c.unclaimedObjectDataBits == 0u,
            "failure did not reset control");
}
void replaceByte(Frame& f, std::size_t bit, std::uint8_t value) {
    for (unsigned i = 0; i < 8; ++i) {
        const auto at = bit + i;
        const auto mask = static_cast<std::uint8_t>(1u << (7u - (at % 8u)));
        if ((value & (0x80u >> i)) != 0) f.bytes.at(at / 8u) |= mask;
        else f.bytes.at(at / 8u) &= static_cast<std::uint8_t>(~mask);
    }
}
}
int main() {
    DRW_ObjControl c;
    for (bool dictionary : {false, true}) for (bool aligned : {false, true}) {
        auto good = frame(dictionary, aligned, 2);
        require(parse(c, good), "valid reactor frame rejected");
        require(c.reactorCount() == 2 && c.reactorHandles == std::vector<std::uint32_t>({0x40, 0x41}), "reactor order lost");
        require(c.xDictHandle == (dictionary ? 0x42u : 0u), "dictionary role lost");
        require(c.orderedHandleSlots == std::vector<std::uint32_t>({0x43, DRW::DwgModelSpaceBlockRecordHandle, DRW::DwgPaperSpaceBlockRecordHandle}), "entry order lost");
        require(!parse(c, frame(dictionary, aligned, 0, DRW::DwgSoftPointer,
                                0x40, 1000000)),
                "oversized reactor count accepted");
        assertReset(c);
        require(parse(c, good), "retry after oversized count failed");
        auto bad = good;
        replaceByte(bad, bad.reactorBit, 0x4f);
        require(!parse(c, bad), "oversized reactor handle accepted");
        assertReset(c);
        bad = good;
        bad.bytes.pop_back();
        require(!parse(c, bad), "truncated frame accepted");
        assertReset(c);
        require(!parse(c, frame(dictionary, aligned, 2, DRW::DwgHardOwnership)), "wrong reactor role accepted");
        assertReset(c);
        require(!parse(c, frame(dictionary, aligned, 2, DRW::DwgSoftPointer, UINT64_C(0x100000040))), "wide reactor reference truncated");
        assertReset(c);
        require(parse(c, good), "same-control retry failed");
    }
    for (const auto relative : std::vector<std::pair<unsigned, std::uint64_t>>{
             {6, 0}, {8, 0}, {10, 0x10}, {12, 0x10}}) {
        require(parse(c, frame(false, false, 1, relative.first, relative.second)),
                "documented relative soft-pointer reactor rejected");
        const std::uint32_t expected = relative.first == 6 ? 0xf1u
            : relative.first == 8 ? 0xefu : relative.first == 10 ? 0x100u : 0xe0u;
        require(c.reactorHandles == std::vector<std::uint32_t>({expected}),
                "relative reactor resolved incorrectly");
    }
    require(parse(c, frame(false, false, 0)), "zero-reactor frame rejected");
    require(c.reactorHandles.empty() && c.reactorCount() == 0, "zero-reactor state dirty");

    const auto vport = ac1032VportControl();
    require(parse(c, vport), "AC1032 VPORT control with reactor rejected");
    require(c.reactorHandles == std::vector<std::uint32_t>({0x3d126})
                && c.orderedHandleSlots == std::vector<std::uint32_t>({0x57})
                && c.handlesList == std::list<std::uint32_t>({0x57}),
            "AC1032 VPORT reactor was confused with its table entry");

    auto truncated = vport;
    truncated.bytes.pop_back();
    require(!parse(c, truncated), "truncated AC1032 VPORT control accepted");
    assertReset(c);
    require(parse(c, vport), "AC1032 VPORT control retry after truncation failed");
    require(c.reactorHandles == std::vector<std::uint32_t>({0x3d126})
                && c.orderedHandleSlots == std::vector<std::uint32_t>({0x57}),
            "AC1032 VPORT control retry changed handle roles");

    auto noReactor = ac1032VportControl(false);
    require(parse(c, noReactor), "AC1032 VPORT control without reactor rejected");
    require(c.reactorHandles.empty()
                && c.orderedHandleSlots == std::vector<std::uint32_t>({0x57}),
            "AC1032 zero-reactor control altered its table entry");

    for (const DRW::Version version : {DRW::AC1021, DRW::AC1027}) {
        const auto historical = historicalAppIdControl(version);
        const bool expectsHistoricalFallback = version == DRW::AC1021;
        require(!historical.bytes.empty(),
                "failed to build generated historical APPID_CONTROL frame");
        require(parse(c, version, historical),
                "bounded historical APPID_CONTROL frame rejected");
        const bool historicalFieldsMatch =
            c.historicalAppIdControlRead == expectsHistoricalFallback
            && c.orderedHandleSlots.size() == 1u
            && c.unclaimedObjectDataBits == historical.expectedUnclaimedBits;
        if (!historicalFieldsMatch) {
            std::cerr << "historical APPID_CONTROL mismatch: version="
                      << static_cast<int>(version)
                      << " retained=" << c.historicalAppIdControlRead
                      << " handles=" << c.orderedHandleSlots.size()
                      << " unclaimed=" << c.unclaimedObjectDataBits
                      << " expected=" << historical.expectedUnclaimedBits
                      << '\n';
        }
        require(historicalFieldsMatch,
                "historical APPID_CONTROL fields or suffix disposition changed");

        auto truncated = historical;
        truncated.bytes.pop_back();
        require(!parse(c, version, truncated),
                "truncated historical APPID_CONTROL frame accepted");
        assertReset(c);
        require(parse(c, version, historical),
                "historical APPID_CONTROL retry after truncation failed");
        require(c.historicalAppIdControlRead == expectsHistoricalFallback
                    && c.orderedHandleSlots.size() == 1u,
                "historical APPID_CONTROL retry changed parsed state");
    }
    const auto suffixedHistoricalAppId =
        historicalAppIdControl(DRW::AC1027, 5u);
    require(!suffixedHistoricalAppId.bytes.empty()
                && parse(c, DRW::AC1027, suffixedHistoricalAppId)
                && c.historicalAppIdControlRead
                && c.unclaimedObjectDataBits == 5u,
            "historical AC1027 APPID_CONTROL suffix was not retained as unclaimed");
    return 0;
}
