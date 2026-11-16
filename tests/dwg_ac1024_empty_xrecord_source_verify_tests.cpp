// SPDX-License-Identifier: GPL-2.0-or-later
// Selected source correlation only; no output or whole-graph admission.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbuffer.h"
#include "intern/drw_textcodec.h"
#include <cstdlib>
#include <iostream>
#include <map>
#include <new>

namespace {
long remaining = -1;
std::size_t allocations = 0;
bool fired = false;
void arm(long count) { remaining = count; allocations = 0; fired = false; }
void disarm() { remaining = -1; }
}
void *operator new(std::size_t size) {
    if (remaining >= 0) {
        ++allocations;
        if (remaining-- == 0) { remaining = -1; fired = true; throw std::bad_alloc(); }
    }
    if (void *p = std::malloc(size ? size : 1u)) return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

namespace {
bool check(bool value, const char *reason) {
    if (!value) std::cerr << "empty AC1024 XRECORD: " << reason << '\n';
    return value;
}
bool verify(const DRW_XRecord &typed, const DRW_UnsupportedObject &raw,
            const DRW_DwgFramePublication &pub) {
    const auto valid = [&] { return typed.matchesAc1024EmptyDwgFrame(raw, pub); };
    if (!check(valid(), "matching source refused")) return false;
    arm(1000000); const bool calibrated = valid(); const auto count = allocations; disarm();
    if (!check(calibrated && count > 0u && count < 1000u, "allocation calibration")) return false;
    for (std::size_t i = 0; i < count; ++i) {
        arm(static_cast<long>(i)); const bool ok = valid(); const bool hit = fired; disarm();
        if (!check(hit && !ok && valid(), "allocation failure or retry")) return false;
    }
    auto altered = typed; altered.m_cloning = 1;
    if (!check(!altered.matchesAc1024EmptyDwgFrame(raw, pub), "changed cloning")) return false;
    altered = typed; altered.m_values.emplace_back(1, UTF8STRING("changed"));
    if (!check(!altered.matchesAc1024EmptyDwgFrame(raw, pub), "changed values")) return false;
    altered = typed; altered.m_handleValues.emplace_back(0, 0x10u);
    if (!check(!altered.matchesAc1024EmptyDwgFrame(raw, pub), "changed references")) return false;
    altered = typed; altered.parentHandle ^= 1u;
    if (!check(!altered.matchesAc1024EmptyDwgFrame(raw, pub), "changed owner")) return false;
    auto frame = raw; frame.m_rawBytes.front() ^= 0x80u;
    if (!check(!typed.matchesAc1024EmptyDwgFrame(frame, pub), "changed encoded type")) return false;
    frame = raw; frame.m_rawBytes.pop_back(); --frame.m_objectSize;
    if (!check(!typed.matchesAc1024EmptyDwgFrame(frame, pub), "truncated frame")) return false;
    frame = raw; frame.m_rawBytes.back() ^= 0x10u;
    if (!check(!typed.matchesAc1024EmptyDwgFrame(frame, pub), "changed owner bytes")) return false;
    auto publication = pub; ++publication.m_sourceOffset;
    if (!check(!typed.matchesAc1024EmptyDwgFrame(raw, publication), "changed location")) return false;
    publication = pub; publication.m_carrier = DRW_DwgFramePublication::Carrier::Typed;
    if (!check(!typed.matchesAc1024EmptyDwgFrame(raw, publication), "missing raw publication")) return false;
    std::cout << "empty AC1024 XRECORD source: PASS (" << count << " allocation failures/retries)\n";
    return valid();
}
bool generatedBody() {
    class Probe : public DRW_XRecord { public: using DRW_XRecord::parseDwg; };
    // Hand-authored DXF converted locally by ODA 27.1; no fixture is committed.
    DRW_UnsupportedObject raw;
    raw.m_version = DRW::AC1024; raw.m_objectType = 79; raw.m_handle = 0xB200u;
    raw.m_parentHandle = 0xB003u; raw.m_xDictFlag = 1u;
    raw.m_objectOffset = 10220u; raw.m_bodyBitSize = 28u; raw.m_objectSize = 9u;
    raw.m_rawBytes = {0x13u, 0xC0u, 0xACu, 0x80u, 0x2Bu, 0x44u, 0x2Bu, 0x00u, 0x30u};
    raw.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    DRW_DwgFramePublication pub;
    pub.m_version = raw.m_version; pub.m_handle = raw.m_handle;
    pub.m_sourceOffset = raw.m_objectOffset; pub.m_sourceMapOrdinal = 30u;
    pub.m_sourceOffsetSpace = DRW_DwgFrameOffsetSpace::DecodedBuffer;
    pub.m_hasSourceLocation = true; pub.m_encodedType = 79; pub.m_resolvedType = 79;
    pub.m_parentHandle = raw.m_parentHandle; pub.m_xDictFlag = 1u;
    pub.m_carrier = DRW_DwgFramePublication::Carrier::TypedAndRaw;
    pub.setCommonLinkEvidence(DRW_DwgCommonLinkEvidence::Validated);
    DRW_TextCodec codec; codec.setVersion(DRW::AC1024, false);
    dwgBuffer buffer(raw.m_rawBytes.data(), raw.m_rawBytes.size(), &codec);
    Probe typed;
    return check(typed.parseDwg(DRW::AC1024, &buffer, raw.m_bodyBitSize), "generated body parse")
        && verify(typed, raw, pub);
}
struct Capture final : LC3CompatCheck {
    std::map<std::uint32_t, DRW_XRecord> typed;
    std::map<std::uint32_t, DRW_UnsupportedObject> raw;
    std::map<std::uint32_t, DRW_DwgFramePublication> publications;
    bool duplicate = false;
    void addXRecord(const DRW_XRecord &v) override { if (v.handle == 0xB200u && !typed.emplace(v.handle,v).second) duplicate = true; }
    void addUnsupportedObject(const DRW_UnsupportedObject &v) override { if (v.m_handle == 0xB200u && !raw.emplace(v.m_handle,v).second) duplicate = true; }
    void addDwgFramePublication(const DRW_DwgFramePublication &v) override { if (v.m_handle == 0xB200u && !publications.emplace(v.m_handle,v).second) duplicate = true; }
};
bool generatedFile(const char *path) {
    Capture capture; dwgR reader(path);
    return check(reader.read(&capture, false) && reader.getVersion() == DRW::AC1024
                 && !capture.duplicate && capture.typed.size() == 1u
                 && capture.raw.size() == 1u && capture.publications.size() == 1u,
                 "file callbacks incomplete")
        && verify(capture.typed.at(0xB200u), capture.raw.at(0xB200u),
                  capture.publications.at(0xB200u));
}
}
int main(int argc, char **argv) {
    if (argc > 2 || !generatedBody()) return 1;
    if (argc == 2 && !generatedFile(argv[1])) return 1;
    return 0;
}
