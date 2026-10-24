#ifndef DX_BATCH_OUTPUT_NAMES_H
#define DX_BATCH_OUTPUT_NAMES_H

#include <filesystem>
#include <string>
#include <vector>

namespace dwg2dxfCli {

enum class BatchOutputNameCheck {
    Clear,
    Collision,
    Error
};

BatchOutputNameCheck checkBatchOutputNameCollisions(
        const std::filesystem::path& outputDirectory,
        const std::vector<std::filesystem::path>& outputs,
        std::string& error);

} // namespace dwg2dxfCli

#endif // DX_BATCH_OUTPUT_NAMES_H
