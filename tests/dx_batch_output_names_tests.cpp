// SPDX-License-Identifier: GPL-2.0-or-later
// Generated filesystem-backed batch output-name collision control.
#include "dx_batch_output_names.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

bool directoryIsEmpty(const std::filesystem::path& directory) {
    return std::filesystem::directory_iterator(directory)
        == std::filesystem::directory_iterator();
}

bool run() {
    const auto unique = std::chrono::steady_clock::now()
        .time_since_epoch().count();
    const auto directory = std::filesystem::temp_directory_path()
        / ("libdxfrw-batch-output-names-" + std::to_string(unique));
    if (!std::filesystem::create_directory(directory)) {
        std::cerr << "could not create batch-name test directory\n";
        return false;
    }

    std::string error;
    const auto duplicate = dwg2dxfCli::checkBatchOutputNameCollisions(
        directory, {"same.dwg", "same.dwg"}, error);
    if (duplicate != dwg2dxfCli::BatchOutputNameCheck::Collision
        || !error.empty() || !directoryIsEmpty(directory)) {
        std::cerr << "exact duplicate was not rejected atomically: "
                  << error << '\n';
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        return false;
    }

    const auto distinct = dwg2dxfCli::checkBatchOutputNameCollisions(
        directory, {"first.dwg", "second.dwg"}, error);
    if (distinct != dwg2dxfCli::BatchOutputNameCheck::Clear
        || !error.empty() || !directoryIsEmpty(directory)) {
        std::cerr << "distinct outputs did not pass with cleanup: "
                  << error << '\n';
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        return false;
    }

    const auto caseProbe = directory / "CaseProbe";
    {
        std::ofstream output(caseProbe);
        output << "probe";
        if (!output) {
            std::cerr << "could not create case-sensitivity probe\n";
            std::error_code ignored;
            std::filesystem::remove_all(directory, ignored);
            return false;
        }
    }
    const bool filesystemCaseInsensitive =
        std::filesystem::exists(directory / "caseprobe");
    std::error_code probeRemovalError;
    if (!std::filesystem::remove(caseProbe, probeRemovalError)
        || probeRemovalError) {
        std::cerr << "could not remove case-sensitivity probe: "
                  << probeRemovalError.message() << '\n';
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        return false;
    }

    const auto folded = dwg2dxfCli::checkBatchOutputNameCollisions(
        directory, {"Case.dwg", "case.dwg"}, error);
    const auto expectedFolded = filesystemCaseInsensitive
        ? dwg2dxfCli::BatchOutputNameCheck::Collision
        : dwg2dxfCli::BatchOutputNameCheck::Clear;
    if (folded != expectedFolded
        || !error.empty() || !directoryIsEmpty(directory)) {
        std::cerr << "case-folded result disagreed with filesystem behavior: "
                  << error << '\n';
        std::error_code ignored;
        std::filesystem::remove_all(directory, ignored);
        return false;
    }

    std::error_code cleanupError;
    if (!std::filesystem::remove(directory, cleanupError) || cleanupError) {
        std::cerr << "could not remove batch-name test directory: "
                  << cleanupError.message() << '\n';
        return false;
    }
    std::cout << "batch output names: duplicate refusal, distinct admission, "
              << (filesystemCaseInsensitive ? "case-insensitive" : "case-sensitive")
              << " filesystem behavior, and scratch cleanup passed\n";
    return true;
}

} // namespace

int main() {
    return run() ? 0 : 1;
}
