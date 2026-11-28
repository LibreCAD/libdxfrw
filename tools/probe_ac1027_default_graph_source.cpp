// Selected-record source probe; supply a separately hash-bound physical DWG.
#include "dx_iface.h"
#include "dx_data.h"
#include "dx_ac1027_layer_wire.h"
#include <algorithm>
#include <iostream>

class DxIfaceStandardDwgCaptureTestAccess {
public:
    static bool inspect(dx_data& data) {
        const auto& source = data.m_ac1027DefaultLayerSource;
        const auto& publication = data.m_ac1027LayerPublication;
        const auto& header = data.m_ac1027HeaderMaterialSource;
        if (!source || !header || data.m_ac1027DefaultLayerDuplicate
            || data.m_ac1027HeaderMaterialDuplicate
            || data.m_ac1027DefaultLayerGeneration != data.importGeneration
            || data.m_ac1027HeaderMaterialGeneration != data.importGeneration
            || !publication.valid || publication.handle != source->m_raw.m_handle
            || publication.offset != source->m_sourceOffset
            || publication.ordinal != source->m_sourceMapOrdinal
            || publication.space != source->m_sourceOffsetSpace
            || header->code != 5u || header->ref64 != header->ref) return false;
        const auto layer = std::find_if(data.layers.begin(), data.layers.end(),
            [&](const DRW_Layer& item) { return item.handle == source->m_raw.m_handle; });
        const auto global = std::find_if(data.materials.begin(), data.materials.end(),
            [](const DRW_Material& item) { return item.m_name == "Global"; });
        const auto byLayer = std::find_if(data.materials.begin(), data.materials.end(),
            [](const DRW_Material& item) { return item.m_name == "ByLayer"; });
        if (layer == data.layers.end() || global == data.materials.end()
            || byLayer == data.materials.end() || data.placeholders.size() != 1u
            || header->ref != byLayer->handle
            || !dx_ac1027_layer_wire::matches(source->m_raw, *layer,
                data.placeholders.front().handle, global->handle)) return false;
        std::cout << "source-layer=" << source->m_raw.m_handle
                  << " offset=" << source->m_sourceOffset
                  << " ordinal=" << source->m_sourceMapOrdinal
                  << " layer-count=" << data.layers.size()
                  << " publication-duplicate=" << publication.duplicate
                  << " header-material=" << header->ref
                  << " global=" << global->handle
                  << " placeholder=" << data.placeholders.front().handle << '\n';
        return true;
    }
};
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(argv[1], &data, false)
        || data.sourceVersion != DRW::AC1027
        || !DxIfaceStandardDwgCaptureTestAccess::inspect(data)) return 1;
    return 0;
}
