#ifndef LIBDXFRW_DXF_ENTITY_IDENTITY_TEST_HELPERS_H
#define LIBDXFRW_DXF_ENTITY_IDENTITY_TEST_HELPERS_H

#include <cstdint>
#include <map>
#include "dwg2dxf/dx_iface.h"

namespace dxf_identity_test {

// Capture only source keys marked written at the successful finalization
// checkpoint. Tests still compare these bindings to independently parsed
// output records; the map alone is not geometry or complete-graph evidence.
class Exporter final : public dx_iface {
public:
    bool finalizeDxfWrite() override {
        if (!dx_iface::finalizeDxfWrite() || dxfW == nullptr)
            return false;
        const auto* context = dxfW->getWritingContext();
        if (context == nullptr)
            return false;
        std::map<std::uint32_t, std::uint32_t> staged;
        for (const auto source : context->writtenSourceEntityHandles) {
            const auto binding = context->sourceHandleToMintedMap.find(source);
            if (source == 0u || binding == context->sourceHandleToMintedMap.end()
                || binding->second == 0u)
                return false;
            staged.emplace(source, binding->second);
        }
        sourceToEmitted.swap(staged);
        return true;
    }
    std::uint32_t emittedHandle(std::uint32_t source) const noexcept {
        const auto found = sourceToEmitted.find(source);
        return found == sourceToEmitted.end() ? 0u : found->second;
    }
    std::map<std::uint32_t, std::uint32_t> sourceToEmitted;
};

} // namespace dxf_identity_test

#endif
