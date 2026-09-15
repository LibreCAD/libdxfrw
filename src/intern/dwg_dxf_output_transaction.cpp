/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version
 * 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * ********************************************************************************
 */

#include "dwg_dxf_output_transaction.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <fcntl.h>
#  include <io.h>
#  include <sys/stat.h>
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

DwgDxfOutputTransaction::DwgDxfOutputTransaction(
    const std::string& target, std::ios::openmode mode)
    : m_target(std::filesystem::path(target)), m_mode(mode) {}

DwgDxfOutputTransaction::~DwgDxfOutputTransaction() {
    if (!m_committed)
        abort();
    else
        closeExclusiveDescriptor();
}

bool DwgDxfOutputTransaction::createExclusiveTemporary() {
    if (m_target.empty())
        return false;

    const std::filesystem::path directory =
        m_target.parent_path().empty() ? std::filesystem::path(".")
                                       : m_target.parent_path();
    const std::string name = m_target.filename().string();
    if (name.empty())
        return false;

#if defined(_WIN32)
    std::random_device random;
    for (std::uint32_t attempt = 0; attempt != 128; ++attempt) {
        m_temporary = directory /
            (name + ".libdxfrw-" + std::to_string(random()) + "-"
             + std::to_string(random()) + "-" + std::to_string(attempt));
        const int descriptor = _wopen(
            m_temporary.c_str(), _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
            _S_IREAD | _S_IWRITE);
        if (descriptor < 0)
            continue;
        m_exclusiveDescriptor = descriptor;
        return true;
    }
#else
    std::string pattern =
        (directory / (name + ".libdxfrw-XXXXXX")).string();
    std::vector<char> mutablePattern(pattern.begin(), pattern.end());
    mutablePattern.push_back('\0');
    const int descriptor = ::mkstemp(mutablePattern.data());
    if (descriptor >= 0) {
        m_temporary = std::filesystem::path(mutablePattern.data());
        m_exclusiveDescriptor = descriptor;
        return true;
    }
#endif
    m_temporary.clear();
    return false;
}

bool DwgDxfOutputTransaction::temporaryIdentityMatches() const noexcept {
    if (m_exclusiveDescriptor < 0 || m_temporary.empty())
        return false;
#if defined(_WIN32)
    struct _stat64 descriptorStatus {
    };
    struct _stat64 pathStatus {
    };
    return _fstat64(m_exclusiveDescriptor, &descriptorStatus) == 0
           && _wstat64(m_temporary.c_str(), &pathStatus) == 0
           && descriptorStatus.st_dev == pathStatus.st_dev
           && descriptorStatus.st_ino == pathStatus.st_ino;
#else
    struct stat descriptorStatus {
    };
    struct stat pathStatus {
    };
    return ::fstat(m_exclusiveDescriptor, &descriptorStatus) == 0
           && ::stat(m_temporary.c_str(), &pathStatus) == 0
           && descriptorStatus.st_dev == pathStatus.st_dev
           && descriptorStatus.st_ino == pathStatus.st_ino;
#endif
}

void DwgDxfOutputTransaction::closeExclusiveDescriptor() noexcept {
    if (m_exclusiveDescriptor < 0)
        return;
#if defined(_WIN32)
    _close(m_exclusiveDescriptor);
#else
    ::close(m_exclusiveDescriptor);
#endif
    m_exclusiveDescriptor = -1;
}

bool DwgDxfOutputTransaction::open() {
    if (m_stream.is_open() || !createExclusiveTemporary())
        return false;

    // The file was created exclusively above.  Do not pass ios::trunc here:
    // reopening with truncation would reintroduce a race with a stale name.
    m_stream.open(m_temporary, m_mode | std::ios::out);
    if (!m_stream.is_open() || !m_stream.good() || !temporaryIdentityMatches()) {
        abort();
        return false;
    }
    return true;
}

bool DwgDxfOutputTransaction::publish() {
#if defined(_WIN32)
    return MoveFileExW(m_temporary.c_str(), m_target.c_str(),
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)
        != 0;
#else
    std::error_code error;
    std::filesystem::rename(m_temporary, m_target, error);
    return !error;
#endif
}

bool DwgDxfOutputTransaction::commit() {
    if (m_committed || !m_stream.is_open())
        return false;
    m_stream.flush();
    if (!m_stream.good()) {
        abort();
        return false;
    }
    if (!temporaryIdentityMatches()) {
        abort();
        return false;
    }
    m_stream.close();
    if (m_stream.fail() || !temporaryIdentityMatches() || !publish()) {
        abort();
        return false;
    }
    m_committed = true;
    closeExclusiveDescriptor();
    m_temporary.clear();
    return true;
}

void DwgDxfOutputTransaction::abort() noexcept {
    if (m_stream.is_open())
        m_stream.close();
    if (!m_temporary.empty()) {
        if (temporaryIdentityMatches()) {
            std::error_code ignored;
            std::filesystem::remove(m_temporary, ignored);
        }
        closeExclusiveDescriptor();
        m_temporary.clear();
    }
}
