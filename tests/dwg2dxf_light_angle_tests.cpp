// SPDX-License-Identifier: GPL-2.0-or-later
#include "../dwg2dxf/dx_iface.h"
#include "lc3_compat_check.cpp"
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <new>
#include <type_traits>

class DxIfaceLightAngleTestAccess {
public:
    static std::shared_ptr<dx_dwgLightImport> make(std::uint64_t g,
                                                   DRW::Version v) {
        auto p = std::make_shared<dx_dwgLightImport>();
        p->generation = g; p->version = p->sealedVersion = v; p->accepted = true;
        return p;
    }
    static void bind(dx_ifaceDwgLight& light,
                     const std::shared_ptr<dx_dwgLightImport>& p) {
        light.source = p; light.capturedSource = p;
        light.sourceGeneration = p->generation;
    }
    static bool captured(const dx_ifaceDwgLight& light, DRW::Version v) {
        return light.source && light.source == light.capturedSource.lock()
            && light.source->accepted && light.source->version == v
            && light.source->sealedVersion == v
            && light.sourceGeneration == light.source->generation;
    }
    static std::shared_ptr<dx_dwgLightImport> receipt(const dx_ifaceDwgLight& light) {
        return light.source;
    }
    static void accepted(const std::shared_ptr<dx_dwgLightImport>& p, bool v) {
        p->accepted = v;
    }
    static void version(const std::shared_ptr<dx_dwgLightImport>& p,
                        DRW::Version v) { p->version = v; }
    static void bothVersions(const std::shared_ptr<dx_dwgLightImport>& p,
                             DRW::Version v) { p->version = p->sealedVersion = v; }
    static void replace(dx_ifaceDwgLight& light,
                        const std::shared_ptr<dx_dwgLightImport>& p) {
        light.source = p;
    }
};

namespace {
std::string readBytes(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}
bool near(double a, double b) { return std::fabs(a - b) < 1e-10; }
struct DwgWriter : LC3CompatCheck {
    dwgRW& output;
    bool registered{false}, written{false};
    explicit DwgWriter(dwgRW& output_) : output(output_) {}
    void writeDwgClasses() override {
        registered = output.registerDwgEntityClassInstance(DRW_Light::kDwgClassNum, 0xF600u);
    }
    void writeEntities() override {
        DRW_Light light; light.handle = 0xF600u; light.m_name = "LIGHT";
        light.m_type = 2; light.m_status = true; light.m_color = 7;
        light.m_hotspotAngle = 0.25; light.m_falloffAngle = 0.75;
        written = output.writeLight(&light);
    }
    bool finalizeDwgWrite() override { return true; }
};
struct DwgReader : LC3CompatCheck {
    double hotspot{-1}, falloff{-1};
    void addLight(const DRW_Light& light) override {
        hotspot = light.m_hotspotAngle; falloff = light.m_falloffAngle;
    }
};
}
int main() {
    const auto dir = std::filesystem::temp_directory_path() /
        ("light-angle-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(dir);
    struct Cleanup { std::filesystem::path p; ~Cleanup() { std::error_code e; std::filesystem::remove_all(p,e); } } cleanup{dir};
    const auto dwgPath = dir / "source.dwg";
    dwgRW dwg(dwgPath.string().c_str()); DwgWriter source(dwg);
    if (!dwg.write(&source, DRW::AC1027, true) || !source.registered || !source.written) return 1;
    dwgR dwgReader(dwgPath.string().c_str()); DwgReader callback;
    if (!dwgReader.read(&callback, false) || callback.hotspot != 0.25 || callback.falloff != 0.75) return 2;
    dx_data imported; dx_iface importer;
    if (!importer.fileImport(dwgPath.string(), &imported, false)) return 3;
    const dx_ifaceDwgLight* captured = nullptr;
    for (const auto* entity : imported.mBlock->ent)
        if (const auto* light = dynamic_cast<const dx_ifaceDwgLight*>(entity)) captured = light;
    if (!captured || !DxIfaceLightAngleTestAccess::captured(*captured, DRW::AC1027)
        || captured->m_hotspotAngle != 0.25 || captured->m_falloffAngle != 0.75) return 4;
    // A replacement at the exact same address does not inherit provenance.
    using Slot = std::aligned_storage_t<sizeof(dx_ifaceDwgLight), alignof(dx_ifaceDwgLight)>;
    Slot slot;
    auto* old = new (&slot) dx_ifaceDwgLight(*captured);
    if (!DxIfaceLightAngleTestAccess::captured(*old, DRW::AC1027)) return 16;
    old->~dx_ifaceDwgLight();
    auto* replacement = new (&slot) DRW_Light();
    DRW_Entity* replacementEntity = replacement;
    if (dynamic_cast<dx_ifaceDwgLight*>(replacementEntity) != nullptr) return 17;
    replacement->~DRW_Light();

    bool laterImportChecked = false;
    for (bool binary : {false, true}) {
        const auto path = dir / (binary ? "out-binary.dxf" : "out-ascii.dxf");
        dx_data data; data.importGeneration = 11; dx_iface adapter;
        auto origin = DxIfaceLightAngleTestAccess::receipt(*captured);
        DRW_Light base; base.m_name = "DWG_LIGHT"; base.m_type = 2;
        base.m_status = true; base.m_color = 7; base.m_hotspotAngle = 0.25; base.m_falloffAngle = 0.75;
        auto* light = new dx_ifaceDwgLight(*captured); light->handle = 0xA100u;
        data.mBlock->ent.push_back(light);
        const std::string sentinel = "LIGHT_SENTINEL\n";
        { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << sentinel; }
        if (!light->m_colorDwgMetadataPresent
            || adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel || !light->m_colorDwgMetadataPresent) return 21;
        // The source's DWG CMC has no qualified DXF projection. Model an
        // explicit caller edit to a known DXF ACI rather than discard it in
        // production; the angle carrier survives that edit.
        light->m_color = 7u; light->m_colorRgb = -1;
        light->m_colorMethod = 0u; light->m_colorDwgFlags = 0u;
        light->m_colorDwgIndex = 0u; light->m_colorDwgMetadataPresent = false;
        light->m_colorName.clear(); light->m_colorBookName.clear();
        auto* dxfLight = new DRW_Light(base); dxfLight->handle = 0xA101u;
        dxfLight->m_name = "DXF_LIGHT"; dxfLight->m_hotspotAngle = 25; dxfLight->m_falloffAngle = 45;
        data.mBlock->ent.push_back(dxfLight);
        for (int attempt = 0; attempt < 2; ++attempt) {
            // Changing the document generation does not alter individual origins.
            data.importGeneration = static_cast<std::uint64_t>(11 + attempt);
            if (!adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)) return 5;
            if (!laterImportChecked) {
                const auto generation = imported.importGeneration;
                if (!importer.fileImport(path.string(), &imported, false)
                    || imported.importGeneration == generation
                    || !DxIfaceLightAngleTestAccess::captured(*captured, DRW::AC1027)
                    || captured->m_hotspotAngle != 0.25) return 14;
                laterImportChecked = true;
            }
            dx_data result; dx_iface reader;
            if (!reader.fileImport(path.string(), &result, false) || result.mBlock->ent.size() != 2u) return 6;
            auto it = result.mBlock->ent.begin();
            const auto* a = dynamic_cast<const DRW_Light*>(*it++);
            const auto* b = dynamic_cast<const DRW_Light*>(*it);
            if (!a || !b || !near(a->m_hotspotAngle, 0.25 * ARAD)
                || !near(a->m_falloffAngle, 0.75 * ARAD)
                || b->m_hotspotAngle != 25 || b->m_falloffAngle != 45
                || light->m_hotspotAngle != 0.25 || light->m_falloffAngle != 0.75) return 7;
        }
        { std::ofstream f(path, std::ios::binary | std::ios::trunc); f << sentinel; }
        light->m_hotspotAngle = std::numeric_limits<double>::max();
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 8;
        light->m_hotspotAngle = std::numeric_limits<double>::quiet_NaN();
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 9;
        light->m_hotspotAngle = 0.25;
        DxIfaceLightAngleTestAccess::accepted(origin, false);
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 10;
        DxIfaceLightAngleTestAccess::accepted(origin, true);
        DxIfaceLightAngleTestAccess::version(origin, DRW::AC1024);
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 12;
        DxIfaceLightAngleTestAccess::version(origin, DRW::AC1027);
        DxIfaceLightAngleTestAccess::bothVersions(origin, DRW::AC1024);
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 15;
        DxIfaceLightAngleTestAccess::bothVersions(origin, DRW::AC1027);
        auto forged = DxIfaceLightAngleTestAccess::make(11, DRW::AC1027);
        DxIfaceLightAngleTestAccess::replace(*light, forged);
        if (adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)
            || readBytes(path) != sentinel) return 13;
        DxIfaceLightAngleTestAccess::replace(*light, origin);
        if (!adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)) return 11;
        light->m_hotspotAngle = 0.5; light->m_falloffAngle = 1.0;
        if (!adapter.fileExport(path.string(), DRW::AC1027, binary, &data, false, true)) return 18;
        dx_data edited; dx_iface editedReader;
        if (!editedReader.fileImport(path.string(), &edited, false)
            || edited.mBlock->ent.empty()) return 19;
        const auto* editedLight = dynamic_cast<const DRW_Light*>(edited.mBlock->ent.front());
        if (!editedLight || !near(editedLight->m_hotspotAngle, 0.5 * ARAD)
            || !near(editedLight->m_falloffAngle, ARAD)
            || light->m_hotspotAngle != 0.5 || light->m_falloffAngle != 1.0) return 20;
    }
    return 0;
}
