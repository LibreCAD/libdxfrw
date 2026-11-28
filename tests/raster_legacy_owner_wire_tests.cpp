// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_raster_dwg_wire.h"
#include "intern/dwgbufferw.h"
#include <cstdint>
#include <iostream>
#include <vector>

namespace {
std::uint32_t cursor(const dwgBufferW& buffer) {
    return static_cast<std::uint32_t>(buffer.size() * 8u
        - (buffer.bitPos() ? 8u - buffer.bitPos() : 0u));
}

// Locally authored 20.4.81 control: no EED, one reactor, ASCII path. The
// role and its offset are parameters so negative controls alter only the
// owner token, not the following scalar or handle fields.
std::vector<std::uint8_t> definition(std::uint16_t type,
                                     std::uint8_t ownerRole,
                                     std::uint8_t ownerValue,
                                     std::uint8_t reactorRole = 4u,
                                     bool r2000 = false) {
    dwgBufferW out;
    out.putBitShort(type);
    auto sizeField = cursor(out);
    if (r2000) out.putRawLong32(0u);
    out.putFixedHandle(0u, 1u, 0x30u);
    out.putBitShort(0u); // EED terminator
    if (!r2000) {
        sizeField = cursor(out);
        out.putRawLong32(0u); // R13/R14 object-data end in bits
    }
    out.putBitLong(1); // reactor count
    out.putBitLong(0); // class version
    out.putRawDouble(100.0); out.putRawDouble(50.0);
    out.putVariableText(DRW::AC1014, "generated.img");
    out.putBit(1u); out.putRawChar8(5u);
    out.putRawDouble(0.5); out.putRawDouble(0.5);
    const auto dataEnd = cursor(out);
    out.putFixedHandle(ownerRole, 1u, ownerValue);
    out.putFixedHandle(reactorRole, 1u, 0x40u);
    out.putFixedHandle(3u, 0u, 0u);
    for (unsigned bit = 0; bit < 32u; ++bit) {
        const auto position = sizeField + bit;
        const auto mask = static_cast<std::uint8_t>(1u << (7u - position % 8u));
        const auto value = (dataEnd >> ((bit / 8u) * 8u + 7u - bit % 8u)) & 1u;
        if (value) out.data()[position / 8u] |= mask;
        else out.data()[position / 8u] &= static_cast<std::uint8_t>(~mask);
    }
    return out.data();
}
}

int main() {
    using namespace dx_raster_wire;
    int failures = 0;
    const auto expect = [&failures](bool ok, const char* message) {
        if (!ok) { std::cerr << message << '\n'; ++failures; }
    };
    const auto check = [](const std::vector<std::uint8_t>& bytes,
                          unsigned revision, std::uint32_t type = 528u) {
        return observe(bytes.data(), bytes.size(), revision,
                       Kind::Definition, 0u, detail::Ignore{},
                       detail::IgnoreEed{}, type);
    };
    const auto relative = definition(528u, 12u, 0x10u);
    const auto proof = check(relative, 14u);
    expect(proof.status == Status::KnownSoftPointerOwnerBackReference
               && proof.self == 0x30u && proof.ownerRole == 12u
               && proof.owner == 0x20u && proof.reactorCount == 1u
               && proof.at(Field::DataEnd).present
               && proof.at(Field::Owner).begin == proof.dataEnd
               && proof.at(Field::Owner).width() == 16u
               && proof.at(Field::Reactors).width() == 16u
               && proof.at(Field::XDictionary).width() == 8u,
           "R14 custom IMAGEDEF relative owner must be classified at its actual handle boundary");
    expect(!profile(14u, Kind::Definition, proof).knownBodyClass,
           "unqualified custom back-reference cannot become a known output class");
    const auto hard = definition(528u, 3u, 0x20u);
    expect(check(hard, 13u).status == Status::Proven
               && check(hard, 14u).status == Status::Proven,
           "R13/R14 documented hard-owner controls remain proven");
    expect(check(relative, 13u).status == Status::Malformed,
           "R13 relative-owner profile remains unqualified");
    const auto r2000Relative = definition(528u, 12u, 0x10u, 4u, true);
    const auto r2000Proof = check(r2000Relative, 2000u);
    expect(r2000Proof.status == Status::KnownSoftPointerOwnerBackReference
               && r2000Proof.self == 0x30u && r2000Proof.owner == 0x20u
               && r2000Proof.ownerRole == 12u
               && r2000Proof.at(Field::Owner).begin == r2000Proof.dataEnd
               && !profile(2000u, Kind::Definition, r2000Proof).knownBodyClass,
           "R2000 custom IMAGEDEF relative owner is observed without output qualification");
    expect(check(definition(528u, 3u, 0x20u, 4u, true), 2000u).status
               == Status::Proven,
           "R2000 documented hard-owner control remains proven");
    expect(check(definition(102u, 12u, 0x10u, 4u, true), 2000u, 102u).status
               == Status::Malformed,
           "R2000 fixed IMAGEDEF does not inherit custom-class evidence");
    expect(check(definition(528u, 12u, 0x40u, 4u, true), 2000u).status
               == Status::Malformed,
           "R2000 relative-owner subtraction must not underflow");
    expect(check(definition(528u, 5u, 0x20u), 14u).status == Status::Malformed,
           "wrong hard-pointer role must not become a back-reference");
    const auto reactorCheck = [](std::uint32_t ordinal, std::uint32_t value,
                                 Span span, std::uint8_t role) noexcept {
        return ordinal == 0u && value == 0x40u && span.width() == 16u
            && role == 4u;
    };
    const auto observeWithReactorCheck = [&reactorCheck](
            const std::vector<std::uint8_t>& bytes) {
        return observe(bytes.data(), bytes.size(), 14u, Kind::Definition, 0u,
                       reactorCheck, detail::IgnoreEed{}, 528u);
    };
    expect(observeWithReactorCheck(relative).status
               == Status::KnownSoftPointerOwnerBackReference,
           "relative owner still verifies the following reactor field");
    expect(observeWithReactorCheck(definition(528u, 12u, 0x10u, 5u)).status
               == Status::Malformed,
           "wrong reactor role must not pass the source observer");
    expect(check(definition(528u, 12u, 0x40u), 14u).status == Status::Malformed,
           "relative-owner subtraction must not underflow");
    expect(check(definition(102u, 12u, 0x10u), 14u, 102u).status == Status::Malformed,
           "fixed IMAGEDEF does not inherit the custom-class observation");
    auto truncated = relative;
    truncated.pop_back();
    expect(check(truncated, 14u).status == Status::Malformed,
           "truncated handle tail must refuse");
    expect(check(relative, 14u).status == Status::KnownSoftPointerOwnerBackReference,
           "complete source must pass after a malformed observation");
    return failures ? 1 : 0;
}
