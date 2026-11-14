// SPDX-License-Identifier: GPL-2.0-or-later
// Generated IMAGE/IMAGEDEF/IMAGEDEF_REACTOR revision-matrix transaction test.
#include "dx_iface.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <new>
#include <string>

namespace {
long allocationsBeforeFailure = -1;
bool allocationFailed = false;

void* allocate(std::size_t size) {
    if (allocationsBeforeFailure == 0) {
        allocationsBeforeFailure = -1;
        allocationFailed = true;
        throw std::bad_alloc();
    }
    if (allocationsBeforeFailure > 0)
        --allocationsBeforeFailure;
    if (void* memory = std::malloc(size == 0 ? 1 : size))
        return memory;
    throw std::bad_alloc();
}

class ImageExport final : public dx_iface {
public:
    explicit ImageExport(bool plannedDefinition = false)
        : plannedDefinition(plannedDefinition) {
        image.m_classVersion = 2;
        if (plannedDefinition) {
            image.handle = 0xA71Fu;
            image.ref = 0xA720u;
            image.m_imageDefReactorHandle = 0xA721u;
        }
        image.basePoint = {1.0, 2.0, 3.0};
        image.secPoint = {1.0, 0.0, 0.0};
        image.vVector = {0.0, 1.0, 0.0};
        image.sizeu = 640.0;
        image.sizev = 480.0;
        image.clip = 1;
        image.brightness = 61;
        image.contrast = 72;
        image.fade = 13;
        image.m_clipBoundaryType = 2;
        image.clipPath = {{0.0, 0.0, 0.0}, {640.0, 0.0, 0.0},
                          {640.0, 480.0, 0.0}};
        image.clipMode = true;
    }

    void writeEntities() override {
        const auto* definition = plannedDefinition
            ? dxfW->writeImage(&image, "version-matrix-image.png", 9u,
                               image.ref)
            : dxfW->writeImage(&image, "version-matrix-image.png");
        written = definition != nullptr;
    }

    void writeObjects() override { dx_iface::writeObjects(); }

    DRW_Image image;
    bool plannedDefinition{false};
    bool written{false};
};

constexpr std::uint32_t kSourceImageHandle = 0xA71Fu;
constexpr std::uint32_t kSourceDefinitionHandle = 0xA720u;
constexpr std::uint32_t kSourceReactorHandle = 0xA721u;

bool installDefinitionPlan(dxfRW& writer, bool plannedDefinition) {
    if (!plannedDefinition)
        return true;
    dxfRW::ImageDefinitionPlanEntry plan;
    plan.importGeneration = 9u;
    plan.sourceHandle = kSourceDefinitionHandle;
    plan.definition.handle = kSourceDefinitionHandle;
    plan.definition.name = "version-matrix-image.png";
    plan.definition.imgVersion = 2;
    plan.definition.u = 640.0;
    plan.definition.v = 480.0;
    plan.definition.up = 0.25;
    plan.definition.vp = 0.5;
    plan.definition.loaded = 1;
    plan.definition.resolution = 5;
    plan.definition.reactorHandles = {kSourceReactorHandle};
    DRW_ImageDefinitionReactor reactor;
    reactor.handle = kSourceReactorHandle;
    reactor.parentHandle = kSourceImageHandle;
    reactor.m_classVersion = 2;
    plan.selectedReactors.push_back(reactor);
    return writer.setImageDefinitionPlan({plan});
}

bool readbackMatches(const std::filesystem::path& path,
                     DRW::Version targetVersion, bool plannedDefinition) {
    dx_data data;
    dx_iface reader;
    if (!reader.fileImport(path.string(), &data, false)) {
        std::cerr << "  read failed\n";
        return false;
    }
    if (data.sourceVersion != targetVersion || data.mBlock == nullptr
        || data.mBlock->ent.size() != 1u
        || data.imageDefinitions.size() != 1u
        || data.imageDefinitionReactors.size() != 1u) {
        std::cerr << "  inventory/version mismatch: version="
                  << static_cast<int>(data.sourceVersion) << " entities="
                  << (data.mBlock == nullptr ? 0u : data.mBlock->ent.size())
                  << " definitions=" << data.imageDefinitions.size()
                  << " reactors=" << data.imageDefinitionReactors.size()
                  << '\n';
        return false;
    }
    const auto* image = dynamic_cast<const DRW_Image*>(data.mBlock->ent.front());
    if (image == nullptr || image->ref == 0u
        || image->m_imageDefReactorHandle == 0u) {
        std::cerr << "  image handle/type mismatch\n";
        return false;
    }
    if (plannedDefinition
        && (image->handle == kSourceImageHandle
            || image->ref == kSourceDefinitionHandle
            || image->m_imageDefReactorHandle == kSourceReactorHandle)) {
        std::cerr << "  planned source identities were not remapped\n";
        return false;
    }
    const DRW_ImageDef* definition =
        data.findImageDefinition(data.importGeneration, image->ref);
    if (definition == nullptr
        || (plannedDefinition
            && definition->handle == kSourceDefinitionHandle)
        || definition->name != "version-matrix-image.png"
        || definition->u != (plannedDefinition ? 640.0 : 0.0)
        || definition->v != (plannedDefinition ? 480.0 : 0.0)
        || definition->up != (plannedDefinition ? 0.25 : 0.0)
        || definition->vp != (plannedDefinition ? 0.5 : 0.0)
        || definition->loaded != (plannedDefinition ? 1 : 0)
        || definition->resolution != (plannedDefinition ? 5 : 0)
        || definition->reactorHandles.size() != 1u
        || definition->reactorHandles.front()
            != image->m_imageDefReactorHandle) {
        std::cerr << "  definition mismatch: found=" << (definition != nullptr)
                  << " name=" << (definition == nullptr ? "" : definition->name)
                  << " u=" << (definition == nullptr ? -1.0 : definition->u)
                  << " v=" << (definition == nullptr ? -1.0 : definition->v)
                  << " up=" << (definition == nullptr ? -1.0 : definition->up)
                  << " vp=" << (definition == nullptr ? -1.0 : definition->vp)
                  << " reactors=" << (definition == nullptr ? 0u : definition->reactorHandles.size())
                  << '\n';
        return false;
    }
    const auto reactor = data.imageDefinitionReactors.begin();
    const bool matched = reactor->second.handle == image->m_imageDefReactorHandle
        && reactor->second.parentHandle == image->handle
        && reactor->second.m_classVersion == 2
        && image->clip == 1 && image->brightness == 61
        && image->contrast == 72 && image->fade == 13
        && image->clipPath.size() == 3u
        && image->clipMode == (targetVersion >= DRW::AC1024);
    if (!matched)
        std::cerr << "  entity/reactor fields mismatch: reactor handle="
                  << reactor->second.handle << " parent="
                  << reactor->second.parentHandle << " image=" << image->handle
                  << " version=" << reactor->second.m_classVersion
                  << " clip=" << image->clip << " brightness="
                  << image->brightness << " contrast=" << image->contrast
                  << " fade=" << image->fade << " path="
                  << image->clipPath.size() << " mode=" << image->clipMode
                  << '\n';
    return matched;
}

bool callerUnchanged(const ImageExport& output) {
    const DRW_Image& image = output.image;
    // `written` is adapter bookkeeping. The callback can complete writeImage()
    // before a later document-level allocation fails, so only the caller's
    // model object is part of the rollback contract.
    return image.handle
            == (output.plannedDefinition ? kSourceImageHandle : 0u)
        && image.ref
            == (output.plannedDefinition ? kSourceDefinitionHandle : 0u)
        && image.m_imageDefReactorHandle
            == (output.plannedDefinition ? kSourceReactorHandle : 0u)
        && image.basePoint.x == 1.0 && image.basePoint.y == 2.0
        && image.basePoint.z == 3.0 && image.sizeu == 640.0
        && image.sizev == 480.0 && image.clip == 1
        && image.brightness == 61 && image.contrast == 72
        && image.fade == 13 && image.clipPath.size() == 3u
        && image.clipMode;
}

bool readAll(const std::filesystem::path& path, std::string& bytes) {
    std::ifstream input(path, std::ios::binary);
    if (!input)
        return false;
    bytes.assign(std::istreambuf_iterator<char>{input}, {});
    return !input.bad();
}

bool run(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error)
        return false;
    const std::string sentinel = "prior IMAGE destination bytes\n";
    for (const DRW::Version version : {
             DRW::AC1014, DRW::AC1015, DRW::AC1018, DRW::AC1021,
             DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
      for (const bool plannedDefinition : {false, true}) {
        for (const bool binary : {false, true}) {
            const std::string suffix = std::to_string(
                static_cast<int>(version))
                + (plannedDefinition ? "-planned" : "-generated")
                + (binary ? "-binary" : "-ascii");
            const auto path = directory / ("image-" + suffix + ".dxf");
            const auto exportClean = [&]() {
                dx_data data;
                ImageExport adapter(plannedDefinition);
                adapter.cData = &data;
                adapter.currentBlock = data.mBlock;
                dxfRW writer(path.string().c_str());
                adapter.dxfW = &writer;
                const bool planned = installDefinitionPlan(
                    writer, plannedDefinition);
                const bool wrote = planned
                    && writer.write(&adapter, version, binary)
                    && adapter.written;
                adapter.dxfW = nullptr;
                return wrote;
            };

            if (!exportClean()) {
                std::cerr << "IMAGE clean baseline failed for " << suffix
                          << '\n';
                return false;
            }
            std::string cleanBytes;
            if (!readAll(path, cleanBytes) || cleanBytes.empty()
                || !readbackMatches(path, version, plannedDefinition)) {
                std::cerr << "IMAGE clean readback failed for " << suffix
                          << '\n';
                return false;
            }

            long failures = 0;
            bool reachedSuccess = false;
            for (long offset = 0; offset < 256; ++offset) {
                {
                    std::ofstream previous(path,
                        std::ios::binary | std::ios::trunc);
                    previous << sentinel;
                    if (!previous)
                        return false;
                }
                dx_data data;
                ImageExport adapter(plannedDefinition);
                adapter.cData = &data;
                adapter.currentBlock = data.mBlock;
                dxfRW writer(path.string().c_str());
                adapter.dxfW = &writer;
                if (!installDefinitionPlan(writer, plannedDefinition))
                    return false;
                allocationFailed = false;
                allocationsBeforeFailure = offset;
                bool wrote = false;
                try {
                    wrote = writer.write(&adapter, version, binary)
                        && adapter.written;
                } catch (const std::bad_alloc&) {
                    wrote = false;
                }
                allocationsBeforeFailure = -1;
                if (wrote) {
                    if (allocationFailed) {
                        std::cerr << "IMAGE write succeeded after allocation fault\n";
                        return false;
                    }
                    std::string actual;
                    if (!readAll(path, actual) || actual != cleanBytes
                        || !readbackMatches(path, version,
                                            plannedDefinition)) {
                        std::cerr << "IMAGE calibration bytes/readback differ for "
                                  << suffix << '\n';
                        return false;
                    }
                    reachedSuccess = true;
                    adapter.dxfW = nullptr;
                    break;
                }
                if (!allocationFailed) {
                    std::cerr << "IMAGE write failed without injected allocation for "
                              << suffix << '\n';
                    return false;
                }
                ++failures;
                std::string preserved;
                if (!readAll(path, preserved) || preserved != sentinel
                    || !callerUnchanged(adapter)) {
                    std::cerr << "IMAGE failure changed destination/caller for "
                              << suffix << " offset=" << offset
                              << " destination=" << (preserved == sentinel)
                              << " caller=" << callerUnchanged(adapter)
                              << " image.handle=" << adapter.image.handle
                              << " ref=" << adapter.image.ref
                              << " reactor="
                              << adapter.image.m_imageDefReactorHandle
                              << " written=" << adapter.written << '\n';
                    return false;
                }
                if (!writer.write(&adapter, version, binary)
                    || !adapter.written) {
                    std::cerr << "IMAGE same-writer retry failed for "
                              << suffix << " offset=" << offset << '\n';
                    return false;
                }
                std::string retryBytes;
                if (!readAll(path, retryBytes) || retryBytes != cleanBytes
                    || !readbackMatches(path, version,
                                        plannedDefinition)) {
                    std::cerr << "IMAGE retry/readback mismatch for "
                              << suffix << " offset=" << offset << '\n';
                    return false;
                }
                adapter.dxfW = nullptr;
            }
            if (!reachedSuccess || failures == 0) {
                std::cerr << "IMAGE allocation sweep incomplete for " << suffix
                          << '\n';
                return false;
            }
            std::cout << "IMAGE/IMAGEDEF/REACTOR "
                      << static_cast<int>(version) << ' '
                      << (plannedDefinition ? "planned " : "generated ")
                      << (binary ? "Binary" : "ASCII")
                      << ": allocation offsets=" << failures
                      << ", rollback/retry/readback=PASS\n";
        }
      }
    }
    std::filesystem::remove_all(directory, error);
    return true;
}
} // namespace

void* operator new(std::size_t size) { return allocate(size); }
void* operator new[](std::size_t size) { return allocate(size); }
void operator delete(void* memory) noexcept { std::free(memory); }
void operator delete[](void* memory) noexcept { std::free(memory); }
void operator delete(void* memory, std::size_t) noexcept { std::free(memory); }
void operator delete[](void* memory, std::size_t) noexcept {
    std::free(memory);
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: image_writer_version_allocation_tests SCRATCH_DIR\n";
        return 2;
    }
    if (!run(argv[1]))
        return 1;
    return 0;
}
