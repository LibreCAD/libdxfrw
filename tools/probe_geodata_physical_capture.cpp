// SPDX-License-Identifier: GPL-2.0-or-later
// Read-only scratch witness probe; link with the test adapter and libdxfrw.
#include "dwg2dxf/dx_data.h"
#include "dwg2dxf/dx_iface.h"
#include <iostream>

class Probe final : public dx_iface {
public:
    void addGeoData(const DRW_GeoData& value) override {
        ++callbacks;
        std::cout << "GEODATA handle=" << value.handle << " owner=" << value.parentHandle
                  << " host=" << value.m_hostBlockHandle << " version=" << value.m_version
                  << " mesh_points=" << value.m_points.size() << " mesh_faces=" << value.m_faces.size() << '\n';
        dx_iface::addGeoData(value);
    }
    unsigned callbacks = 0;
};

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    dx_data data;
    Probe reader;
    const bool imported = reader.fileImport(argv[1], &data, false);
    std::cout << "imported=" << imported << " callbacks=" << reader.callbacks
              << " captured=" << data.geoDataObjects.size()
              << " object_parse_failures=" << data.dwgReadOutcome.observations.objectParseFailures << '\n';
    return imported ? 0 : 1;
}
