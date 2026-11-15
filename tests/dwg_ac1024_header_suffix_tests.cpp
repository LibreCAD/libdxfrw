// SPDX-License-Identifier: GPL-2.0-or-later
#include "intern/dwgreader.h"
#include "intern/dwgheadersection.h"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

namespace {
std::uint16_t crc16(const std::vector<std::uint8_t>& bytes, std::size_t end) {
    std::uint16_t value = 0xc0c1u;
    for (std::size_t i = 16; i < end; ++i) {
        value ^= bytes[i];
        for (int bit = 0; bit < 8; ++bit)
            value = static_cast<std::uint16_t>((value >> 1)
                ^ ((value & 1u) ? 0xa001u : 0u));
    }
    return value;
}
std::vector<std::uint8_t> makeSection(std::size_t suffixSize) {
    std::vector<std::uint8_t> bytes(dwgSentinels::HEADER_BEGIN,
                                     dwgSentinels::HEADER_BEGIN + 16);
    // Size excludes the high word, as in the physical AC1024 source.
    const std::uint8_t data[] = {4,0,0,0, 0,0,0,0, 1,2,3,4};
    bytes.insert(bytes.end(), data, data + sizeof(data));
    const auto crc = crc16(bytes, bytes.size());
    bytes.push_back(static_cast<std::uint8_t>(crc));
    bytes.push_back(static_cast<std::uint8_t>(crc >> 8));
    bytes.insert(bytes.end(), dwgSentinels::HEADER_END,
                 dwgSentinels::HEADER_END + 16);
    for (std::size_t i = 0; i < suffixSize; ++i)
        bytes.push_back(static_cast<std::uint8_t>(i * 73u + 19u));
    return bytes;
}
DwgHeaderEnvelope inspect(std::vector<std::uint8_t>& bytes, std::uint8_t maintenance) {
    dwgBuffer buffer(bytes.data(), bytes.size());
    if (!buffer.setPosition(16)) return {};
    return inspectDwgHeaderEnvelope(buffer, DRW::AC1024, maintenance);
}
bool accepted(std::vector<std::uint8_t>& bytes, std::uint8_t maintenance,
              std::uint64_t suffix) {
    const auto r = inspect(bytes, maintenance);
    return r.error == DwgHeaderEnvelopeError::None && r.excludesHighSizeWord
        && r.hasHighSizeWord && r.highSizeWord == 0 && r.unclaimedBytes == suffix
        && r.trailerEnd + suffix == bytes.size();
}
bool ac1021Accepted(std::vector<std::uint8_t>& bytes, std::uint8_t maintenance,
                    std::uint64_t suffix) {
    dwgBuffer buffer(bytes.data(), bytes.size());
    if (!buffer.setPosition(16)) return false;
    const auto result = inspectDwgHeaderEnvelope(buffer, DRW::AC1021, maintenance);
    return result.error == DwgHeaderEnvelopeError::None
        && result.unclaimedBytes == suffix && result.trailerEnd + suffix == bytes.size();
}
}
int main(int argc, char** argv) {
    if (argc == 2) {
        std::ifstream f(argv[1], std::ios::binary);
        std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(f)), {});
        if (!f || !accepted(bytes, 55, 739)) return 1;
        const auto result = inspect(bytes, 55);
        if (result.payloadEnd != 825 || result.trailerEnd != 843
            || result.storedCrc != 0xe6acu) return 1;
        std::cout << "physical AC1024 HEADER envelope and suffix PASS\n";
        return 0;
    }
    if (argc != 1) return 2;
    auto good = makeSection(739);
    if (!accepted(good, 55, 739)) return 1;
    if (inspect(good, 54).error == DwgHeaderEnvelopeError::None
        || inspect(good, 56).error == DwgHeaderEnvelopeError::None) return 1;
    for (std::size_t offset : {std::size_t(0), std::size_t(24), std::size_t(28), std::size_t(30)}) {
        auto corrupt = good; corrupt[offset] ^= 1u;
        if (inspect(corrupt, 55).error == DwgHeaderEnvelopeError::None
            || !accepted(good, 55, 739)) return 1;
    }
    auto wrongHigh = good; wrongHigh[20] = 1;
    const auto resealed = crc16(wrongHigh, 28);
    wrongHigh[28] = static_cast<std::uint8_t>(resealed);
    wrongHigh[29] = static_cast<std::uint8_t>(resealed >> 8);
    if (inspect(wrongHigh, 55).error == DwgHeaderEnvelopeError::None) return 1;
    auto truncated = good; truncated.resize(30);
    if (inspect(truncated, 55).error == DwgHeaderEnvelopeError::None) return 1;
    auto exact = makeSection(0);
    if (!accepted(exact, 55, 0) || !accepted(exact, 54, 0)) return 1;
    auto canonical = exact;
    canonical[16] = 8; // This count includes the high word.
    const auto canonicalCrc = crc16(canonical, 28);
    canonical[28] = static_cast<std::uint8_t>(canonicalCrc);
    canonical[29] = static_cast<std::uint8_t>(canonicalCrc >> 8);
    const auto canonicalResult = inspect(canonical, 55);
    if (canonicalResult.error != DwgHeaderEnvelopeError::None
        || canonicalResult.excludesHighSizeWord || canonicalResult.unclaimedBytes != 0) return 1;
    // Independent ACadRust AC1021 maintenance-25 producer appends eight zeros.
    auto ac1021 = makeSection(0);
    ac1021[16] = 8;
    const auto crc = crc16(ac1021, 28);
    ac1021[28] = static_cast<std::uint8_t>(crc);
    ac1021[29] = static_cast<std::uint8_t>(crc >> 8);
    ac1021.resize(ac1021.size() + 8, 0);
    if (!ac1021Accepted(ac1021, 25, 8) || ac1021Accepted(ac1021, 24, 8)) return 1;
    for (std::size_t offset : {std::size_t(0), std::size_t(24), std::size_t(28),
                               std::size_t(30), ac1021.size() - 1}) {
        auto corrupt = ac1021; corrupt[offset] ^= 1u;
        if (ac1021Accepted(corrupt, 25, 8) || !ac1021Accepted(ac1021, 25, 8)) return 1;
    }
    auto shortSuffix = ac1021; shortSuffix.pop_back();
    auto longSuffix = ac1021; longSuffix.push_back(0);
    if (ac1021Accepted(shortSuffix, 25, 7) || ac1021Accepted(longSuffix, 25, 9)) return 1;
    ac1021.resize(ac1021.size() - 8);
    if (!ac1021Accepted(ac1021, 25, 0) || !ac1021Accepted(ac1021, 24, 0)) return 1;
    std::cout << "AC1024 bounded HEADER suffix and corruption controls PASS\n";
    return 0;
}
