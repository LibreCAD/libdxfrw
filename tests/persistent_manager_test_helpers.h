#ifndef LIBDXFRW_TESTS_PERSISTENT_MANAGER_TEST_HELPERS_H
#define LIBDXFRW_TESTS_PERSISTENT_MANAGER_TEST_HELPERS_H

#include <cstdint>
#include <limits>
#include <vector>

#include "drw_base.h"
#include "intern/dwgbufferw.h"

namespace persistent_manager_test {

struct Options {
    DRW::Version version = DRW::AC1021;
    std::uint16_t classNumber = 601u; // Synthetic file-local ordinal, not a fixed type.
    std::uint32_t handle = 0xD63u;
    std::int32_t classVersion = 2;
    std::int32_t unknown0 = 0;
    std::int32_t unknown2 = 2;
    std::int32_t associationSubentities = 0;
    std::int32_t stepCount = 2;
    std::vector<std::int32_t> steps{3, 4};
    bool hasSubentityCount = false;
    std::int32_t subentityCount = 0;
    std::vector<std::int32_t> subentities;
    bool singleCountBit = false;
};

struct Frame {
    std::vector<std::uint8_t> payload;
    std::uint32_t handleBits = 0;
    std::uint64_t bodyStartBit = 0;
    std::uint64_t scalarEndBit = 0;
    std::uint64_t objectEndBit = 0;
};

inline dwgHandle reference(std::uint8_t code, std::uint32_t ref) {
    dwgHandle result;
    result.code = code;
    result.size = ref <= 0xFFu ? 1u : 2u;
    result.ref = ref;
    result.ref64 = ref;
    return result;
}

// Runtime-generated common OBJECT framing follows ODA section 20.1. The eight
// scalar prefix is a literal reproduction of the paired AC1021 D63 source
// record, not an inferred general persistent-manager body specification.
inline bool makeFrame(const Options& options, Frame& frame) {
    if (options.version < DRW::AC1021)
        return false;
    dwgBufferW object;
    object.putObjType(options.version, options.classNumber);
    const std::size_t sizeBit = object.bitCount();
    if (options.version < DRW::AC1024)
        object.putRawLong32(0u);
    object.putHandle(reference(DRW::DwgNullReference, options.handle));
    object.putBitShort(0u); // no EED
    object.putBitLong(1); // one reactor
    object.putBit(1u); // no extension dictionary
    if (options.version > DRW::AC1024)
        object.putBit(0u); // no data-store association
    frame.bodyStartBit = object.bitCount();
    object.putBitLong(options.classVersion);
    object.putBitLong(options.unknown0);
    object.putBitLong(options.unknown2);
    object.putBitLong(4); // source numassocsteps scalar
    object.putBitLong(options.associationSubentities);
    object.putBitLong(options.stepCount);
    for (const auto step : options.steps)
        object.putBitLong(step);
    if (options.hasSubentityCount) {
        object.putBitLong(options.subentityCount);
        for (const auto subentity : options.subentities)
            object.putBitLong(subentity);
    }
    if (options.singleCountBit)
        object.putBit(1u); // Cannot borrow the following zero footer as BL0's second bit.
    frame.scalarEndBit = object.bitCount();
    object.putBit(0u); // ODA common OBJECT string-presence bit
    frame.objectEndBit = object.bitCount();
    if (options.version < DRW::AC1024)
        object.patchRawLong32AtBit(sizeBit,
            static_cast<std::uint32_t>(frame.objectEndBit));
    object.putHandle(reference(DRW::DwgSoftPointer, 0xCu)); // owner
    object.putHandle(reference(DRW::DwgSoftPointer, 0xCu)); // reactor
    object.alignToByte();
    if (!object.isGood() || object.bitCount() < frame.objectEndBit
        || object.bitCount() - frame.objectEndBit
               > std::numeric_limits<std::uint32_t>::max())
        return false;
    frame.handleBits = static_cast<std::uint32_t>(
        object.bitCount() - frame.objectEndBit);
    frame.payload = object.data();
    return true;
}

inline bool wrapFrame(const Options& options, const Frame& object,
                      std::vector<std::uint8_t>& bytes) {
    dwgBufferW frame;
    frame.putModularShort(static_cast<std::int32_t>(object.payload.size()));
    if (options.version >= DRW::AC1024)
        frame.putUModularChar(object.handleBits);
    frame.putBytes(object.payload.data(), object.payload.size());
    frame.putRawShort16(frame.crc16(0xC0C1u, 0u, frame.size()));
    if (!frame.isGood())
        return false;
    bytes = frame.data();
    return true;
}

} // namespace persistent_manager_test

#endif
