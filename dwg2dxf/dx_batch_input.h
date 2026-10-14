/******************************************************************************
**  Bounded dwg2dxf batch list acquisition.                                 **
**  GPL-2.0-or-later                                                        **
******************************************************************************/
#ifndef DX_BATCH_INPUT_H
#define DX_BATCH_INPUT_H

#include <cstddef>
#include <istream>
#include <string>
#include <utility>
#include <vector>

namespace dwg2dxfCli {

struct BatchInputLimits {
    std::size_t lineBytes = 128u * 1024u;
    std::size_t items = 10000u;
    std::size_t totalBytes = 16u * 1024u * 1024u;
};

// Acquire the complete bounded list before publishing any drawing. A failed
// acquisition leaves the caller's list unchanged. Only CR immediately before
// LF is a terminator: spaces, UTF-8 and a final literal CR are filename bytes.
inline bool readBatchList(std::istream& input, std::vector<std::string>& items,
                          std::string& error,
                          const BatchInputLimits& limits = {}) {
    std::vector<std::string> parsed;
    std::string line;
    std::size_t total = 0;
    bool pendingCR = false;
    const auto appendByte = [&](char byte) {
        if (line.size() == limits.lineBytes) {
            error = "batch line byte limit exceeded";
            return false;
        }
        line.push_back(byte);
        return true;
    };
    const auto finishLine = [&]() {
        if (line.empty())
            return true; // Ignore genuinely empty lines, not filename spaces.
        if (parsed.size() == limits.items) {
            error = "batch item limit exceeded";
            return false;
        }
        parsed.push_back(std::move(line));
        line.clear();
        return true;
    };
    char byte = 0;
    while (input.get(byte)) {
        if (total == limits.totalBytes) {
            error = "batch byte limit exceeded";
            return false;
        }
        ++total;
        if (byte == '\n') {
            pendingCR = false; // A deferred CR is part of CRLF, not the path.
            if (!finishLine())
                return false;
        } else {
            if (byte == '\0') {
                error = "NUL byte in batch filename";
                return false;
            }
            if (pendingCR && !appendByte('\r'))
                return false;
            pendingCR = byte == '\r';
            if (!pendingCR && !appendByte(byte))
                return false;
        }
    }
    if (input.bad() || (!input.eof() && input.fail())) {
        error = "batch list read failed";
        return false;
    }
    if ((pendingCR && !appendByte('\r')) || !finishLine())
        return false;
    if (parsed.empty()) {
        error = "batch list has no inputs";
        return false;
    }
    items = std::move(parsed);
    error.clear();
    return true;
}

} // namespace dwg2dxfCli
#endif
