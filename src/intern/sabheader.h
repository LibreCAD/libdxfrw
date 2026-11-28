/* Minimal SAB framing shared by the DWG-associated modeler output gates.
 * This checks only the signature and availability of four 32-bit header
 * fields; it does not validate their values or the SAB body. */
#ifndef LIBDXFRW_SABHEADER_H
#define LIBDXFRW_SABHEADER_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

inline bool drw_hasSabMinimumHeader(const std::vector<std::uint8_t>& payload) {
    static constexpr char signature[] = "ACIS BinaryFile";
    constexpr std::size_t signatureSize = sizeof(signature) - 1u;
    constexpr std::size_t fixedHeaderSize = signatureSize + 4u * 4u;
    return payload.size() >= fixedHeaderSize
        && std::equal(signature, signature + signatureSize, payload.begin(),
                      [](char a, std::uint8_t b) {
                          return static_cast<std::uint8_t>(a) == b;
                      });
}

#endif
