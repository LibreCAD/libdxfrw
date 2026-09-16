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
    else {
        closeExclusiveDescriptor();
        closeDirectoryDescriptor();
    }
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
    int directoryFlags = O_RDONLY;
#  if defined(O_DIRECTORY)
    directoryFlags |= O_DIRECTORY;
#  endif
#  if defined(O_CLOEXEC)
    directoryFlags |= O_CLOEXEC;
#  endif
    m_directoryDescriptor = ::open(directory.c_str(), directoryFlags);
    if (m_directoryDescriptor < 0)
        return false;
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
#if !defined(_WIN32)
    closeDirectoryDescriptor();
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
    const std::filesystem::path temporaryName = m_temporary.filename();
    if (m_directoryDescriptor < 0 || temporaryName.empty())
        return false;
    // Resolve the temporary name relative to the directory descriptor that
    // created it.  This remains valid when the parent directory is renamed,
    // and AT_SYMLINK_NOFOLLOW prevents a pathname substitution from being
    // mistaken for our still-open file.
    const int flags =
#  if defined(AT_SYMLINK_NOFOLLOW)
        AT_SYMLINK_NOFOLLOW;
#  else
        0;
#  endif
    return ::fstat(m_exclusiveDescriptor, &descriptorStatus) == 0
           && ::fstatat(m_directoryDescriptor, temporaryName.c_str(),
                        &pathStatus, flags) == 0
           && descriptorStatus.st_dev == pathStatus.st_dev
           && descriptorStatus.st_ino == pathStatus.st_ino;
#endif
}

bool DwgDxfOutputTransaction::directoryIdentityMatchesPath() const noexcept {
#if defined(_WIN32)
    // The Windows implementation publishes with MoveFileExW and has no
    // portable directory descriptor to compare with the target pathname.
    return true;
#else
    if (m_directoryDescriptor < 0 || m_target.empty())
        return false;
    const std::filesystem::path directory =
        m_target.parent_path().empty() ? std::filesystem::path(".")
                                       : m_target.parent_path();
    struct stat descriptorStatus {
    };
    struct stat pathStatus {
    };
    return ::fstat(m_directoryDescriptor, &descriptorStatus) == 0
           && ::stat(directory.c_str(), &pathStatus) == 0
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

void DwgDxfOutputTransaction::closeDirectoryDescriptor() noexcept {
#if defined(_WIN32)
    return;
#else
    if (m_directoryDescriptor >= 0)
        ::close(m_directoryDescriptor);
    m_directoryDescriptor = -1;
#endif
}

bool DwgDxfOutputTransaction::flushFileToStorage() const noexcept {
    if (m_exclusiveDescriptor < 0)
        return false;
#if defined(_WIN32)
    const intptr_t nativeHandle = _get_osfhandle(m_exclusiveDescriptor);
    if (nativeHandle == static_cast<intptr_t>(-1))
        return false;
    return FlushFileBuffers(reinterpret_cast<HANDLE>(nativeHandle)) != 0;
#else
    return ::fsync(m_exclusiveDescriptor) == 0;
#endif
}

bool DwgDxfOutputTransaction::flushParentDirectoryToStorage() const noexcept {
#if defined(_WIN32)
    // MoveFileExW(..., MOVEFILE_WRITE_THROUGH) is the native Windows
    // equivalent used by publish(); Windows does not provide a portable
    // directory descriptor that can be flushed like a POSIX directory.
    return true;
#else
    if (m_directoryDescriptor < 0)
        return false;
    return ::fsync(m_directoryDescriptor) == 0;
#endif
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
    if (m_directoryDescriptor < 0)
        return false;
    const std::filesystem::path temporaryName = m_temporary.filename();
    const std::filesystem::path targetName = m_target.filename();
    return ::renameat(m_directoryDescriptor, temporaryName.c_str(),
                      m_directoryDescriptor, targetName.c_str()) == 0;
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
    if (!temporaryIdentityMatches() || !directoryIdentityMatchesPath()) {
        abort();
        return false;
    }
    if (!flushFileToStorage()) {
        abort();
        return false;
    }
    // Verify that the containing directory can be synchronized before the
    // pathname publication.  The post-rename sync below is best effort: the
    // replacement is already atomically visible and cannot be rolled back if
    // a platform reports a late durability failure.
    if (!flushParentDirectoryToStorage()) {
        abort();
        return false;
    }
    m_stream.close();
    if (m_stream.fail() || !temporaryIdentityMatches()
        || !directoryIdentityMatchesPath() || !publish()) {
        abort();
        return false;
    }
    m_committed = true;
    closeExclusiveDescriptor();
    (void)flushParentDirectoryToStorage();
    closeDirectoryDescriptor();
    m_temporary.clear();
    return true;
}

void DwgDxfOutputTransaction::abort() noexcept {
    if (m_stream.is_open())
        m_stream.close();
    if (!m_temporary.empty()) {
        const bool owned = temporaryIdentityMatches();
#if defined(_WIN32)
        if (owned) {
            std::error_code ignored;
            std::filesystem::remove(m_temporary, ignored);
        }
#else
        if (owned && m_directoryDescriptor >= 0) {
            const std::filesystem::path temporaryName = m_temporary.filename();
            if (!temporaryName.empty())
                (void)::unlinkat(m_directoryDescriptor, temporaryName.c_str(),
                                 0);
        }
#endif
        closeExclusiveDescriptor();
        closeDirectoryDescriptor();
        m_temporary.clear();
    }
}
