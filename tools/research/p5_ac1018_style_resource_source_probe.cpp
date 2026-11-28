#include "dx_iface.h"
#include "libdwgr.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace {
constexpr std::uint32_t kStyleControlHandle = 0x03u;

class GeneratedSource final : public dx_iface {
public:
    explicit GeneratedSource(dwgRW& writer) : writer_(writer) {
        style.handle = 0x11u;
        style.name = "Standard";
        style.font = "arial.ttf";
        style.height = 2.5;
        style.lastHeight = 2.5;
        style.extData.push_back(new DRW_Variant(1001, "ACAD"));
        style.extData.push_back(new DRW_Variant(1000, "Arial"));
        appId.name = "ACAD";
    }

    void writeHeader(DRW_Header& header) override { header.vars.clear(); }
    void collectDwgAppIds() override {}
    void writeBlocks() override {}
    void writeBlockRecords() override {}
    void writeEntities() override {}
    void writeLTypes() override {}
    void writeLayers() override {}
    void writeTextstyles() override { styleWritten = writer_.addTextstyle(&style); }
    void writeVports() override {}
    void writeViews() override {}
    void writeUCSs() override {}
    void writeDimstyles() override {}
    void writeAppId() override { appIdWritten = writer_.addAppId(&appId); }

    DRW_Textstyle style;
    DRW_AppId appId;
    bool styleWritten{false};
    bool appIdWritten{false};

private:
    dwgRW& writer_;
};

bool containsAppIdEed(const DRW_Textstyle& style, const std::string& name) {
    for (const DRW_Variant* value : style.extData) {
        if (value != nullptr && value->code() == 1001
            && value->stringEquals(name))
            return true;
    }
    return false;
}

bool run(const std::filesystem::path& path) {
    if (std::filesystem::exists(path))
        return false;
    struct Cleanup {
        std::filesystem::path path;
        ~Cleanup() {
            std::error_code error;
            std::filesystem::remove(path, error);
        }
    } cleanup{path};

    dwgRW writer(path.string().c_str());
    GeneratedSource generated(writer);
    if (!writer.write(&generated, DRW::AC1018, true)
        || !generated.styleWritten || !generated.appIdWritten
        || generated.style.handle == 0u
        || generated.style.handle == DRW::NoHandle)
        return false;

    std::ifstream input(path, std::ios::binary);
    const std::vector<char> original((std::istreambuf_iterator<char>(input)), {});
    if (!input || original.size() < 64u)
        return false;

    dx_iface reader;
    dx_data data;
    const bool selected = reader.enableVerifiedStyleResourceSource(
        kStyleControlHandle, generated.style.handle);
    const bool imported = selected && reader.fileImport(path.string(), &data, false);
    const bool sourceBound = imported
        && dx_iface::hasUnchangedVerifiedStyleResourceSource(
            data, kStyleControlHandle, generated.style.handle);
    auto style = std::find_if(data.textStyles.begin(), data.textStyles.end(),
        [&generated](const DRW_Textstyle& row) {
            return row.handle == generated.style.handle;
        });
    auto appId = std::find_if(data.appIds.begin(), data.appIds.end(),
        [](const DRW_AppId& row) { return row.name == "ACAD"; });

    bool styleMutationRefused = false;
    bool styleRestoreAccepted = false;
    bool appIdMetadataMutationRefused = false;
    bool appIdMetadataRestoreAccepted = false;
    if (sourceBound && style != data.textStyles.end()) {
        const double originalHeight = style->height;
        style->height += 0.125;
        styleMutationRefused =
            !dx_iface::hasUnchangedVerifiedStyleResourceSource(
                data, kStyleControlHandle, generated.style.handle);
        style->height = originalHeight;
        styleRestoreAccepted =
            dx_iface::hasUnchangedVerifiedStyleResourceSource(
                data, kStyleControlHandle, generated.style.handle);
    }
    if (styleRestoreAccepted && appId != data.appIds.end()) {
        const auto originalUnknown71 = appId->unknown71;
        appId->unknown71 = static_cast<std::uint8_t>(originalUnknown71 ^ 1u);
        appIdMetadataMutationRefused =
            !dx_iface::hasUnchangedVerifiedStyleResourceSource(
                data, kStyleControlHandle, generated.style.handle);
        appId->unknown71 = originalUnknown71;
        appIdMetadataRestoreAccepted =
            dx_iface::hasUnchangedVerifiedStyleResourceSource(
                data, kStyleControlHandle, generated.style.handle);
    }

    const auto scratchPath = path.parent_path()
        / (path.stem().string() + ".truncated.dwg");
    bool truncatedWrite = false;
    {
        std::ofstream broken(scratchPath, std::ios::binary);
        broken.write(original.data(), static_cast<std::streamsize>(original.size() / 2u));
        truncatedWrite = broken.good();
    }
    dx_data partial;
    const bool partialImported = truncatedWrite
        && reader.fileImport(scratchPath.string(), &partial, false);
    const bool partialWithheld = !partialImported
        && !dx_iface::hasUnchangedVerifiedStyleResourceSource(
            partial, kStyleControlHandle, generated.style.handle);
    std::error_code cleanupError;
    std::filesystem::remove(scratchPath, cleanupError);

    dx_data retry;
    const bool retryImported = reader.fileImport(path.string(), &retry, false);
    const bool retryBound = retryImported
        && dx_iface::hasUnchangedVerifiedStyleResourceSource(
            retry, kStyleControlHandle, generated.style.handle);
    const auto retryStyle = std::find_if(retry.textStyles.begin(), retry.textStyles.end(),
        [&generated](const DRW_Textstyle& row) {
            return row.handle == generated.style.handle;
        });
    const auto retryAppId = std::find_if(retry.appIds.begin(), retry.appIds.end(),
        [](const DRW_AppId& row) { return row.name == "ACAD"; });
    const bool eedClosed = retryStyle != retry.textStyles.end()
        && retryAppId != retry.appIds.end()
        && containsAppIdEed(*retryStyle, retryAppId->name);

    const bool passed = sourceBound && styleMutationRefused && styleRestoreAccepted
        && appIdMetadataMutationRefused && appIdMetadataRestoreAccepted
        && truncatedWrite && partialWithheld && retryBound && eedClosed;
    std::cout << "generated=1 selected=" << selected << " imported=" << imported
              << " sourceBound=" << sourceBound
              << " styleMutationRefused=" << styleMutationRefused
              << " styleRestoreAccepted=" << styleRestoreAccepted
              << " appIdMetadataMutationRefused=" << appIdMetadataMutationRefused
              << " appIdMetadataRestoreAccepted=" << appIdMetadataRestoreAccepted
              << " partialWithheld=" << partialWithheld
              << " retryBound=" << retryBound << " eedClosed=" << eedClosed
              << " result=" << (passed ? "PASS" : "FAIL") << '\n';
    return passed;
}
} // namespace

int main(int argc, char** argv) {
    if (argc != 2)
        return 2;
    return run(argv[1]) ? 0 : 1;
}
