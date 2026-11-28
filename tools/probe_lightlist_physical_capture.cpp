// SPDX-License-Identifier: GPL-2.0-or-later
// Read-only witness-specific diagnostic; the hard-coded spans do not
// establish a general LIGHTLIST DWG grammar or output eligibility.
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include <iomanip>
#include <iostream>

class Probe final : public dx_iface {
public:
    void addLightList(const DRW_LightList& value) override {
        ++typed;
        std::cout << "LIGHTLIST handle=" << value.handle << " owner=" << value.parentHandle
                  << " count=" << value.m_lights.size();
        for (const auto& light : value.m_lights)
            std::cout << " light=" << light.m_handle << ':' << light.m_name;
        std::cout << '\n';
        dx_iface::addLightList(value);
    }
    void addUnsupportedObject(const DRW_UnsupportedObject& value) override {
        if (value.m_recordName == "LIGHTLIST") {
            ++raw;
            std::cout << "raw handle=" << value.m_handle << " type=" << value.m_objectType
                      << " size=" << value.m_rawBytes.size() << " handle_bits=" << value.m_bodyBitSize;
            if (value.m_rawBytes.size() == 40u && value.m_bodyBitSize == 33u) {
                unsigned token = 0;
                for (unsigned bit = 68; bit != 84; ++bit)
                    token = (token << 1u) | ((value.m_rawBytes[bit / 8u] >> (7u - bit % 8u)) & 1u);
                std::cout << " bits_68_84=0x" << std::hex << std::setw(4) << std::setfill('0')
                          << token << std::dec;
            }
            std::cout << '\n';
        }
        dx_iface::addUnsupportedObject(value);
    }
    unsigned typed = 0, raw = 0;
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    dx_data data;
    Probe reader;
    const bool imported = reader.fileImport(argv[1], &data, false);
    std::cout << "imported=" << imported << " typed=" << reader.typed << " raw=" << reader.raw
              << " object_parse_failures=" << data.dwgReadOutcome.observations.objectParseFailures << '\n';
    return imported ? 0 : 1;
}
