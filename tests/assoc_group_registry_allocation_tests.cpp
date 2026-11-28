// SPDX-License-Identifier: GPL-2.0-or-later
// Allocation failure/reset/retry for an authored AC1032 registry candidate.
#include "associative_constraint_group_test_helpers.h"
#include "drw_objects.h"
#include "intern/dwgbuffer.h"

#include <cstdlib>
#include <iostream>
#include <new>
#include <vector>

namespace {
long failAt = -1;
long allocationCount = -1;
bool faultReached = false;
struct ScopedFault {
    explicit ScopedFault(long point) { allocationCount = 0; failAt = point; faultReached = false; }
    ~ScopedFault() { failAt = -1; allocationCount = -1; }
};
class Probe final : public DRW_AssociativeObject {
public:
    Probe() : DRW_AssociativeObject("ACDBASSOC2DCONSTRAINTGROUP") {}
    using DRW_AssociativeObject::parseDwg;
};
using namespace associative_constraint_group_test;

bool makeFrame(Frame& frame) {
    dwgBufferW object, strings, handles;
    object.putObjType(DRW::AC1032, 0x301u);
    object.putHandle(reference(DRW::DwgNullReference, 0xA600u));
    object.putBitShort(0); object.putBitLong(0); // EED, reactors
    object.putBit(1u); object.putBit(0u); // no xdict or DataStorage
    object.putBitShort(2); object.putBitLong(0); // action class/status
    object.putBitLong(1); object.putBitLong(2); object.putBitLong(0);
    object.putBitShort(0); object.putBitLong(0);
    object.putBitShort(0); object.putBitLong(0);
    object.putBitLong(2); object.putBit(0u);
    object.put3BitDouble(DRW_Coord(0, 0, 0));
    object.put3BitDouble(DRW_Coord(1, 0, 0));
    object.put3BitDouble(DRW_Coord(0, 1, 0));
    object.putBitLong(2); // two actions
    object.putBitLong(2); object.putBitLong(0); object.putBitLong(0);
    object.putBit(0u); object.putBitLong(2); // root flag, class slots
    object.putBitLong(2); // registered entries
    object.putBit(0u); object.putBitLong(1); object.putBitLong(1);
    object.putBit(0u); object.putBitLong(2); object.putBitLong(2);
    strings.putVariableText(DRW::AC1032, "AcConstrainedBoundedLine");
    strings.putVariableText(DRW::AC1032, "AcPointCurveConstraint");
    if (!appendStringStream(object, strings)) return false;
    object.alignToByte(); frame.objectEndBit = object.bitCount();
    handles.putHandle(reference(DRW::DwgSoftPointer, 0xA601u));
    handles.putHandle(reference(DRW::DwgSoftPointer, 0xA602u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0xA603u));
    handles.putHandle(reference(DRW::DwgHardOwnership, 0xA604u));
    object.putBytes(handles.data().data(), handles.data().size());
    frame.handleBits = static_cast<std::uint32_t>(handles.bitCount());
    frame.payload = object.data();
    return object.isGood() && handles.isGood();
}

bool parsed(const Probe& probe) {
    return probe.m_constraintActionsParsed && probe.m_constraintActions.size() == 2u
        && probe.m_constraintNextNodeIdRead && probe.m_constraintNextNodeId == 2
        && !probe.m_constraintGroupBodyParsed && !probe.m_constraintNodesParsed
        && probe.m_constraintNodeCount == 0 && probe.m_constraintNodes.empty();
}
} // namespace

void* operator new(std::size_t size) {
    if (allocationCount >= 0) {
        if (allocationCount++ == failAt) { faultReached = true; throw std::bad_alloc(); }
    }
    if (void* p = std::malloc(size == 0 ? 1 : size)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }

int main() {
    Frame frame;
    if (!makeFrame(frame)) return 1;
    Probe calibration;
    dwgBuffer baseline(frame.payload.data(), frame.payload.size());
    long reached = 0;
    bool ok = false;
    {
        ScopedFault measure(-1);
        ok = calibration.parseDwg(DRW::AC1032, &baseline, frame.handleBits);
        reached = allocationCount;
    }
    if (!ok || !parsed(calibration) || reached <= 0) return 2;
    for (long index = 0; index < reached; ++index) {
        Probe source;
        dwgBuffer failing(frame.payload.data(), frame.payload.size());
        bool accepted = false, caught = false, triggered = false;
        {
            ScopedFault injection(index);
            try { accepted = source.parseDwg(DRW::AC1032, &failing, frame.handleBits); }
            catch (...) { caught = true; }
            triggered = faultReached;
        }
        if (!triggered || caught || accepted || failing.isGood()
            || source.m_constraintActionsParsed || source.m_constraintNextNodeIdRead
            || !source.m_constraintActions.empty() || !source.m_constraintNodes.empty()
            || !source.m_prefixStatuses.empty()) {
            std::cerr << "allocation failure did not roll back at offset " << index << '\n';
            return 3;
        }
        dwgBuffer retry(frame.payload.data(), frame.payload.size());
        if (!source.parseDwg(DRW::AC1032, &retry, frame.handleBits) || !parsed(source)) {
            std::cerr << "same-object retry failed at offset " << index << '\n';
            return 4;
        }
    }
    std::cout << "AC1032 registry parser allocation failures=" << reached
              << "; reset and same-object retry pass\n";
    return 0;
}
