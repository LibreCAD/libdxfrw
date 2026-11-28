// SPDX-License-Identifier: GPL-2.0-or-later
#include "dx_iface.h"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "usage: ac1021_group_polyline_dxf_readback <2d.dxf> <3d.dxf>\n";
        return 2;
    }
    const auto verify = [](const char* path, bool threeD) {
        dx_data data;
        dx_iface reader;
        if (!reader.fileImport(path, &data, false)
            || data.sourceFormat != dx_source_format::DXF
            || data.mBlock == nullptr || data.groupCaptureFailed
            || data.groupNameResolutionFailed)
            return false;
        const DRW_Polyline* polyline = nullptr;
        for (const DRW_Entity* entity : data.mBlock->ent) {
            const auto* candidate = dynamic_cast<const DRW_Polyline*>(entity);
            if (candidate == nullptr) continue;
            if (polyline != nullptr) return false;
            polyline = candidate;
        }
        if (polyline == nullptr || polyline->flags != (threeD ? 8 : 0)
            || polyline->vertlist.size() != 2u || !polyline->vertlist[0]
            || !polyline->vertlist[1])
            return false;
        const DRW_Vertex& first = *polyline->vertlist[0];
        const DRW_Vertex& second = *polyline->vertlist[1];
        if (first.basePoint.x != 1. || first.basePoint.y != 2.
            || first.basePoint.z != 0. || second.basePoint.x != 3.
            || second.basePoint.y != 4.
            || second.basePoint.z != (threeD ? 1. : 0.)
            || (threeD && (first.flags != 32 || second.flags != 32)))
            return false;
        const DRW_Group* group = nullptr;
        for (const DRW_Group& candidate : data.groups) {
            if (candidate.name != "GROUP_VERTEX") continue;
            if (group != nullptr) return false;
            group = &candidate;
        }
        return group != nullptr && group->m_entityHandles.size() == 1u
            && group->m_entityHandles.front() == first.handle;
    };
    if (!verify(argv[1], false) || !verify(argv[2], true)) return 1;
    std::cout << "AC1021 strict DXF 2D/3D polyline coordinates and GROUP membership read back\n";
    return 0;
}
