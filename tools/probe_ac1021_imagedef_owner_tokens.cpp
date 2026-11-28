// Diagnostic only: inspect retained IMAGEDEF frames from a caller-supplied DWG.
// Page extraction is libdxfrw's; the bit walk below is separate from its typed
// IMAGEDEF parser and from LibreDWG. This does not qualify output semantics.
#include "dx_iface.h"
#include <cstdint>
#include <iostream>
#include <memory>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool dump(const dx_data& data) {
        std::size_t found = 0;
        for (const auto& entry : data.m_rasterDwgSources) {
            const auto& raw = entry.second.raw;
            if (raw.m_recordName != "IMAGEDEF") continue;
            if (raw.m_version != DRW::AC1021) return false;
            const auto& bytes = raw.m_rawBytes;
            const std::size_t total = bytes.size() * 8u;
            std::size_t position = 0;
            bool good = true;
            const auto take = [&](unsigned n) {
                std::uint32_t value = 0;
                if (!good || n > 32u || position > total || n > total - position) {
                    good = false;
                    return value;
                }
                for (unsigned i = 0; i < n; ++i, ++position)
                    value = (value << 1u) | ((bytes[position / 8u] >> (7u - position % 8u)) & 1u);
                return value;
            };
            const auto little = [&](unsigned n) {
                std::uint32_t value = 0;
                for (unsigned i = 0; i < n; ++i) value |= take(8u) << (8u * i);
                return value;
            };
            const auto tag = take(2u);
            const auto type = tag == 0u ? little(2u) : tag == 1u ? take(8u) : tag == 2u ? 0u : 256u;
            const auto dataEnd = little(4u);
            if (!good || type != static_cast<std::uint32_t>(raw.m_objectType) || dataEnd >= total) return false;
            position = dataEnd;
            const auto ownerStart = position;
            const auto ownerRole = take(4u), ownerLength = take(4u);
            if (!good || ownerLength > 4u) return false;
            std::uint32_t offset = 0;
            for (unsigned i = 0; i < ownerLength; ++i) offset = (offset << 8u) | take(8u);
            const auto ownerEnd = position;
            const auto reactorRole = take(4u), reactorLength = take(4u);
            if (!good || reactorLength > 4u) return false;
            std::uint32_t reactor = 0;
            for (unsigned i = 0; i < reactorLength; ++i) reactor = (reactor << 8u) | take(8u);
            if (!good || ownerRole != 12u || ownerLength != 1u || offset > raw.m_handle
                || reactorRole != 4u || reactorLength != 1u || raw.m_handle - offset != reactor) return false;
            std::cout << "self=" << raw.m_handle << " type=" << type << " bytes=" << bytes.size()
                      << " boundary=" << dataEnd << " owner_span=" << ownerStart << ':' << ownerEnd
                      << " owner_code=" << ownerRole << " offset=" << offset
                      << " owner=" << raw.m_handle - offset << " reactor_span=" << ownerEnd << ':' << position
                      << " reactor_code=" << reactorRole << " reactor=" << reactor << '\n';
            ++found;
        }
        return found != 0;
    }
};
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    auto data = std::make_unique<dx_data>();
    auto reader = std::make_unique<dx_iface>();
    return reader->fileImport(argv[1], data.get(), false)
        && DxIfaceStandardDwgCaptureTestAccess::dump(*data) ? 0 : 1;
}
