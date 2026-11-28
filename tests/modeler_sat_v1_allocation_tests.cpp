#include "drw_entities.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace allocationFault {
bool armed = false;
bool fired = false;
bool observing = false;
std::size_t requestedSize = 0u;
std::size_t matchingAllocations = 0u;
}

void* operator new(std::size_t size) {
    if (allocationFault::observing
        && size == allocationFault::requestedSize)
        ++allocationFault::matchingAllocations;
    if (allocationFault::armed && size == allocationFault::requestedSize) {
        allocationFault::armed = false;
        allocationFault::fired = true;
        throw std::bad_alloc();
    }
    if (void* result = std::malloc(size == 0u ? 1u : size))
        return result;
    throw std::bad_alloc();
}

void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* value) noexcept { std::free(value); }
void operator delete[](void* value) noexcept { std::free(value); }
void operator delete(void* value, std::size_t) noexcept { std::free(value); }
void operator delete[](void* value, std::size_t) noexcept { std::free(value); }

namespace {
class TestModeler final : public DRW_ModelerGeometry {
public:
    using DRW_ModelerGeometry::DRW_ModelerGeometry;
    using DRW_ModelerGeometry::parseDwg;
    using DRW_Entity::encodeDwgCommon;
    using DRW_Entity::encodeDwgEntHandle;

    void setObjectType(std::uint16_t value) { oType = value; }
};

bool patchObjectSize(std::vector<std::uint8_t>& bytes,
                     std::uint64_t sizeFieldBit,
                     std::uint32_t objectSizeBits) {
    const std::uint64_t totalBits =
        static_cast<std::uint64_t>(bytes.size()) * 8u;
    if (sizeFieldBit > totalBits || totalBits - sizeFieldBit < 32u)
        return false;
    for (std::uint32_t byteIndex = 0u; byteIndex < 4u; ++byteIndex) {
        const std::uint8_t raw = static_cast<std::uint8_t>(
            objectSizeBits >> (byteIndex * 8u));
        for (std::uint32_t bitIndex = 0u; bitIndex < 8u; ++bitIndex) {
            const std::uint64_t destinationBit =
                sizeFieldBit + byteIndex * 8u + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(destinationBit & 7u)));
            const std::uint8_t value = static_cast<std::uint8_t>(
                (raw >> (7u - bitIndex)) & 1u);
            auto& destination = bytes[static_cast<std::size_t>(
                destinationBit >> 3u)];
            destination = static_cast<std::uint8_t>(
                (destination & static_cast<std::uint8_t>(~mask))
                | (value ? mask : 0u));
        }
    }
    return true;
}

bool buildFrame(std::vector<std::uint8_t>& bytes,
                const std::vector<std::uint8_t>& payload) {
    TestModeler source(DRW::E3DSOLID);
    source.setObjectType(38u); // ODA §20.4.41: 3DSOLID
    source.handle = 0x120u;

    dwgBufferW frame;
    if (!source.encodeDwgCommon(DRW::AC1018, &frame))
        return false;
    frame.putBit(0); // modeler body is non-empty
    frame.putBit(1); // opaque modeler flag
    frame.putBitShort(1); // SAT v1
    frame.putBitLong(static_cast<std::int32_t>(payload.size()));
    for (const std::uint8_t value : payload) {
        const std::uint8_t encoded = value > 0x20u && value <= 0x7Eu
            ? static_cast<std::uint8_t>(0x9Fu - value) : value;
        frame.putRawChar8(encoded);
    }
    frame.putBitLong(0); // SAT block terminator
    frame.putBit(0); // no DWG wireframe block
    frame.putBit(1); // empty wireframe-side body
    if (!frame.isGood() || frame.bitCount() > UINT32_MAX)
        return false;
    const std::uint32_t objectSizeBits =
        static_cast<std::uint32_t>(frame.bitCount());

    dwgBuffer typeReader(frame.data().data(), frame.data().size());
    if (typeReader.getObjType(DRW::AC1018) != 38u)
        return false;
    const std::uint64_t sizeFieldBit = typeReader.getPosition() * 8u
        + typeReader.getBitPos();
    if (!source.encodeDwgEntHandle(DRW::AC1018, &frame) || !frame.isGood())
        return false;
    bytes = frame.data();
    return patchObjectSize(bytes, sizeFieldBit, objectSizeBits);
}

bool parsedPayload(const std::vector<std::uint8_t>& frame,
                   std::vector<std::uint8_t>& payload,
                   std::size_t& rangeCount) {
    dwgBuffer input(frame.data(), frame.size());
    TestModeler parsed(DRW::E3DSOLID);
    if (!parsed.parseDwg(DRW::AC1018, &input) || !input.isGood()
        || parsed.handle != 0x120u || parsed.m_modelerVersion != 1u
        || parsed.m_dwgSourceVersion != DRW::AC1018
        || parsed.m_objectSize != frame.size())
        return false;
    payload = parsed.m_dwgAcisPayload;
    rangeCount = parsed.m_payloadRanges.size();
    return true;
}
} // namespace

int main() {
    // A distinctive size targets satPayload.reserve(), the optional payload
    // extraction allocation, without faulting construction or later parsing.
    constexpr std::size_t payloadSize = 173u;
    std::vector<std::uint8_t> authored(payloadSize, static_cast<std::uint8_t>('Z'));
    std::vector<std::uint8_t> frame;
    if (!buildFrame(frame, authored)) {
        std::cerr << "could not build generated AC1018 SAT-v1 frame\n";
        return 1;
    }

    allocationFault::requestedSize = payloadSize;
    allocationFault::matchingAllocations = 0u;
    allocationFault::observing = true;
    dwgBuffer baselineInput(frame.data(), frame.size());
    TestModeler baselineParse(DRW::E3DSOLID);
    const bool baselineOk = baselineParse.parseDwg(DRW::AC1018, &baselineInput)
        && baselineInput.isGood();
    allocationFault::observing = false;
    if (!baselineOk || baselineParse.m_dwgAcisPayload.size() != payloadSize
        || baselineParse.m_payloadRanges.size() != 1u
        || allocationFault::matchingAllocations != 1u) {
        std::cerr << "baseline SAT-v1 extraction failed\n";
        return 1;
    }
    const std::vector<std::uint8_t> baseline = baselineParse.m_dwgAcisPayload;
    const std::size_t baselineRanges = baselineParse.m_payloadRanges.size();

    allocationFault::fired = false;
    allocationFault::armed = true;
    dwgBuffer failedInput(frame.data(), frame.size());
    TestModeler failedParse(DRW::E3DSOLID);
    const bool imported = failedParse.parseDwg(DRW::AC1018, &failedInput)
        && failedInput.isGood();
    allocationFault::armed = false;
    if (!imported || !allocationFault::fired
        || !failedParse.m_dwgAcisPayload.empty()
        || !failedParse.m_payloadRanges.empty()
        || failedParse.m_objectSize != frame.size()) {
        std::cerr << "SAT payload allocation failure escaped optional extraction\n";
        return 1;
    }

    dwgBuffer retryInput(frame.data(), frame.size());
    if (!failedParse.parseDwg(DRW::AC1018, &retryInput)
        || !retryInput.isGood()
        || failedParse.m_dwgAcisPayload != baseline
        || failedParse.m_payloadRanges.size() != baselineRanges
        || failedParse.m_objectSize != frame.size()) {
        std::cerr << "same-input SAT-v1 extraction retry failed\n";
        return 1;
    }
    std::cout << "AC1018 SAT-v1 optional extraction allocation refusal and retry passed\n";
    return 0;
}
