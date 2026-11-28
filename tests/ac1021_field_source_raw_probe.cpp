// Opt-in physical source probe; no DWG/DXF fixture is bundled by this tool.
#include "dx_iface.h"

#include <cstdint>
#include <iomanip>
#include <iostream>

namespace {
class FieldProbe final : public dx_iface {
public:
    void addUnsupportedObject(const DRW_UnsupportedObject& object) override {
        if (object.m_recordName == "FIELD") {
            if (++fields > 4096u || object.m_rawBytes.size() > 1024u * 1024u) {
                bounded = false;
            } else {
                std::cout << "RAW " << std::hex << object.m_handle << ' '
                          << std::dec << object.m_rawBytes.size() << ' ';
                for (const std::uint8_t value : object.m_rawBytes)
                    std::cout << std::hex << std::setfill('0') << std::setw(2)
                              << static_cast<unsigned>(value);
                std::cout << std::dec << '\n';
            }
        }
        dx_iface::addUnsupportedObject(object);
    }
    std::size_t fields = 0;
    bool bounded = true;
};
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: ac1021_field_source_raw_probe input.dwg\n";
        return 2;
    }
    dx_data data;
    FieldProbe probe;
    if (!probe.fileImport(argv[1], &data, false) || !probe.bounded
        || data.sourceFormat != dx_source_format::DWG
        || data.sourceVersion != DRW::AC1021 || !std::cout.good()) {
        std::cerr << "AC1021 FIELD frame capture failed\n";
        return 1;
    }
    return 0;
}
