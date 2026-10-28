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

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <random>
#include <string>

#if defined(_WIN32)
#  include <fcntl.h>
#  include <io.h>
#  include <share.h>
#  include <sys/stat.h>
#  include <windows.h>
#else
#  include <fcntl.h>
#  include <sys/stat.h>
#  include <unistd.h>
#endif

#if !defined(_WIN32)
namespace {
const char* temporaryFileName(const std::filesystem::path& path) noexcept {
    const auto& native = path.native();
    const auto slash = native.find_last_of('/');
    return native.c_str() + (slash == std::string::npos ? 0u : slash + 1u);
}
}
#endif

DwgDxfOutputTransaction::DwgDxfOutputTransaction(
    const std::string& target, std::ios::openmode mode)
    : m_target(std::filesystem::u8path(target)),
      m_targetName(m_target.filename()), m_directory(m_target.parent_path()),
      m_mode(mode) {
    if (m_directory.empty()) m_directory = ".";
}

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

    const auto& directory = m_directory;
    // Keep the basename native (UTF-16 on Windows); append only ASCII suffixes.
    const auto& name = m_targetName;
    if (name.empty())
        return false;

#if defined(_WIN32)
    if (!captureTargetSnapshot())
        return false;
    std::random_device random;
    for (std::uint32_t attempt = 0; attempt != 128; ++attempt) {
        m_temporary = directory / name;
        m_temporary += ".libdxfrw-" + std::to_string(random()) + "-"
                       + std::to_string(random()) + "-" + std::to_string(attempt);
        int descriptor = -1;
        if (_wsopen_s(&descriptor, m_temporary.c_str(),
                      _O_CREAT | _O_EXCL | _O_WRONLY | _O_BINARY,
                      _SH_DENYNO, _S_IREAD | _S_IWRITE) != 0
            || descriptor < 0)
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
    // Remember what this write replaces, if anything.  The mode is applied to
    // the temporary just before the rename, so an overwrite keeps the
    // permissions the target already had rather than the temporary's.
    //
    // Resolved against the directory descriptor and without following a
    // symlink, the same way temporaryIdentityMatches() below resolves its own
    // name.  A plain stat() would follow a link and take the mode from a file
    // this transaction is never going to touch -- renameat replaces the link
    // itself -- which is how an attacker who can write the output directory
    // would get to choose the permissions of somebody else's saved drawing.
    // Only a regular file has a mode worth inheriting: for anything else the
    // published file is treated as new.
    if (!captureTargetSnapshot()) {
        closeDirectoryDescriptor();
        return false;
    }

    // Create the temporary the same way the Windows branch above does, rather
    // than with mkstemp.  mkstemp always requests 0600, which would have to be
    // widened afterwards to what an ordinary open() would have produced -- and
    // the only portable way to learn that is umask(0) followed by umask(mask),
    // which is a process-global write.  A library cannot do that: between the
    // two calls every other thread in the host process creates files with no
    // umask applied at all.  Passing a mode to open() lets the kernel subtract
    // the umask atomically, which is exactly the intent.
    //
    // Which mode depends on what this write replaces.  The temporary lives in
    // the target's own directory for the whole duration of the write, so it
    // must never be more permissive than what it is about to become: asking
    // for 0666 while overwriting somebody's 0600 file would publish that
    // file's new contents to every reader on the host until the rename.  A
    // replacement therefore asks for the target's own bits, and only a genuinely
    // new file asks for 0666 -- where there is no existing content to expose and
    // 0666 & ~umask is the answer wanted anyway.
    // Owner-write is always kept, whatever the target's own bits are: the
    // stream below reopens this temporary by name, so creating it read-only
    // would refuse the write outright -- and saving over a read-only drawing
    // is something the previous implementation did happily.  It costs nothing
    // to keep: publish() fchmods to the target's exact mode before the rename,
    // and S_IWUSR is owner-only, so it widens none of the group or other bits
    // this narrowing exists to withhold.
    const mode_t creationMode =
        m_targetMode >= 0 ? static_cast<mode_t>(m_targetMode) | S_IWUSR
                          : mode_t{0666};
    int openFlags = O_CREAT | O_EXCL | O_RDWR;
#  if defined(O_CLOEXEC)
    openFlags |= O_CLOEXEC;
#  endif
    // The suffix is the same sixteen bytes mkstemp produced -- ".libdxfrw-"
    // and six random characters -- because its length is a limit on what can
    // be written at all.  A longer suffix refuses basenames the previous
    // implementation accepted, and refuses them with ENAMETOOLONG, which is
    // not EEXIST and so ends the loop rather than retrying.  Six characters
    // from this alphabet is also the entropy mkstemp itself used.
    static constexpr char alphabet[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    static constexpr std::size_t alphabetSize = sizeof(alphabet) - 1u;
    std::random_device random;
    std::uniform_int_distribution<std::size_t> pick(0u, alphabetSize - 1u);
    for (std::uint32_t attempt = 0; attempt != 128; ++attempt) {
        std::string suffix(".libdxfrw-");
        for (int character = 0; character != 6; ++character)
            suffix.push_back(alphabet[pick(random)]);
        std::filesystem::path candidate = directory / name;
        candidate += suffix;
        const int descriptor = ::open(candidate.c_str(), openFlags, creationMode);
        if (descriptor < 0) {
            if (errno == EEXIST)
                continue;
            break;
        }

        m_temporary = candidate;
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

bool DwgDxfOutputTransaction::captureTargetSnapshot() noexcept {
    if (m_target.empty() || m_targetName.empty())
        return false;
#if defined(_WIN32)
    const HANDLE targetHandle = CreateFileW(
        m_target.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS
            | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (targetHandle == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            return false;
        m_targetSnapshot = TargetSnapshot {};
        m_targetSnapshot.captured = true;
        return true;
    }
    BY_HANDLE_FILE_INFORMATION information {};
    const bool ok = GetFileInformationByHandle(targetHandle, &information) != 0;
    CloseHandle(targetHandle);
    if (!ok)
        return false;

    ULARGE_INTEGER modified {};
    modified.HighPart = information.ftLastWriteTime.dwHighDateTime;
    modified.LowPart = information.ftLastWriteTime.dwLowDateTime;
    ULARGE_INTEGER created {};
    created.HighPart = information.ftCreationTime.dwHighDateTime;
    created.LowPart = information.ftCreationTime.dwLowDateTime;
    m_targetSnapshot.captured = true;
    m_targetSnapshot.exists = true;
    m_targetSnapshot.device = information.dwVolumeSerialNumber;
    m_targetSnapshot.file =
        (static_cast<std::uint64_t>(information.nFileIndexHigh) << 32u)
        | information.nFileIndexLow;
    m_targetSnapshot.size =
        (static_cast<std::uint64_t>(information.nFileSizeHigh) << 32u)
        | information.nFileSizeLow;
    m_targetSnapshot.modifiedSeconds = modified.QuadPart;
    m_targetSnapshot.changedSeconds = created.QuadPart;
    m_targetSnapshot.type = information.dwFileAttributes;
    return true;
#else
    if (m_directoryDescriptor < 0)
        return false;
    struct stat targetStatus {};
    const auto& targetName = m_targetName;
    const int flags =
#  if defined(AT_SYMLINK_NOFOLLOW)
        AT_SYMLINK_NOFOLLOW;
#  else
        0;
#  endif
    if (::fstatat(m_directoryDescriptor, targetName.c_str(), &targetStatus,
                  flags) != 0) {
        if (errno != ENOENT)
            return false;
        m_targetSnapshot = TargetSnapshot {};
        m_targetSnapshot.captured = true;
        m_targetMode = -1;
        return true;
    }
#  if defined(__APPLE__)
    const std::uint64_t modifiedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_mtimespec.tv_nsec);
    const std::uint64_t changedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_ctimespec.tv_nsec);
#  else
    const std::uint64_t modifiedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_mtim.tv_nsec);
    const std::uint64_t changedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_ctim.tv_nsec);
#  endif
    m_targetSnapshot.captured = true;
    m_targetSnapshot.exists = true;
    m_targetSnapshot.device = static_cast<std::uint64_t>(targetStatus.st_dev);
    m_targetSnapshot.file = static_cast<std::uint64_t>(targetStatus.st_ino);
    m_targetSnapshot.size = static_cast<std::uint64_t>(targetStatus.st_size);
    m_targetSnapshot.modifiedSeconds =
        static_cast<std::uint64_t>(targetStatus.st_mtime);
    m_targetSnapshot.modifiedSubseconds = modifiedSubseconds;
    m_targetSnapshot.changedSeconds =
        static_cast<std::uint64_t>(targetStatus.st_ctime);
    m_targetSnapshot.changedSubseconds = changedSubseconds;
    m_targetSnapshot.type = static_cast<std::uint32_t>(targetStatus.st_mode);
    if (S_ISREG(targetStatus.st_mode)) {
        // Permission bits only.  setuid, setgid and the sticky bit are not
        // carried onto what is a newly created inode, possibly with a
        // different owner -- replacing a file must not hand its privileges to
        // the replacement.
        m_targetMode = static_cast<int>(targetStatus.st_mode & 0777);
    } else {
        m_targetMode = -1;
    }
    return true;
#endif
}

bool DwgDxfOutputTransaction::targetSnapshotMatches() const noexcept {
    if (!m_targetSnapshot.captured || m_target.empty())
        return false;
#if defined(_WIN32)
    const HANDLE targetHandle = CreateFileW(
        m_target.c_str(), FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS
            | FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (targetHandle == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        return !m_targetSnapshot.exists
            && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND);
    }
    if (!m_targetSnapshot.exists) {
        CloseHandle(targetHandle);
        return false;
    }
    BY_HANDLE_FILE_INFORMATION information {};
    const bool ok = GetFileInformationByHandle(targetHandle, &information) != 0;
    CloseHandle(targetHandle);
    if (!ok)
        return false;
    ULARGE_INTEGER modified {};
    modified.HighPart = information.ftLastWriteTime.dwHighDateTime;
    modified.LowPart = information.ftLastWriteTime.dwLowDateTime;
    ULARGE_INTEGER created {};
    created.HighPart = information.ftCreationTime.dwHighDateTime;
    created.LowPart = information.ftCreationTime.dwLowDateTime;
    const std::uint64_t file =
        (static_cast<std::uint64_t>(information.nFileIndexHigh) << 32u)
        | information.nFileIndexLow;
    const std::uint64_t size =
        (static_cast<std::uint64_t>(information.nFileSizeHigh) << 32u)
        | information.nFileSizeLow;
    return information.dwVolumeSerialNumber == m_targetSnapshot.device
        && file == m_targetSnapshot.file && size == m_targetSnapshot.size
        && modified.QuadPart == m_targetSnapshot.modifiedSeconds
        && created.QuadPart == m_targetSnapshot.changedSeconds
        && information.dwFileAttributes == m_targetSnapshot.type;
#else
    if (m_directoryDescriptor < 0)
        return false;
    struct stat targetStatus {};
    const auto& targetName = m_targetName;
    const int flags =
#  if defined(AT_SYMLINK_NOFOLLOW)
        AT_SYMLINK_NOFOLLOW;
#  else
        0;
#  endif
    if (::fstatat(m_directoryDescriptor, targetName.c_str(), &targetStatus,
                  flags) != 0) {
        return !m_targetSnapshot.exists && errno == ENOENT;
    }
    if (!m_targetSnapshot.exists)
        return false;
#  if defined(__APPLE__)
    const std::uint64_t modifiedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_mtimespec.tv_nsec);
    const std::uint64_t changedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_ctimespec.tv_nsec);
#  else
    const std::uint64_t modifiedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_mtim.tv_nsec);
    const std::uint64_t changedSubseconds =
        static_cast<std::uint64_t>(targetStatus.st_ctim.tv_nsec);
#  endif
    return static_cast<std::uint64_t>(targetStatus.st_dev)
               == m_targetSnapshot.device
        && static_cast<std::uint64_t>(targetStatus.st_ino)
               == m_targetSnapshot.file
        && static_cast<std::uint64_t>(targetStatus.st_size)
               == m_targetSnapshot.size
        && static_cast<std::uint64_t>(targetStatus.st_mtime)
               == m_targetSnapshot.modifiedSeconds
        && modifiedSubseconds == m_targetSnapshot.modifiedSubseconds
        && static_cast<std::uint64_t>(targetStatus.st_ctime)
               == m_targetSnapshot.changedSeconds
        && changedSubseconds == m_targetSnapshot.changedSubseconds
        && static_cast<std::uint32_t>(targetStatus.st_mode)
               == m_targetSnapshot.type;
#endif
}

bool DwgDxfOutputTransaction::temporaryIdentityMatches() const noexcept {
    if (m_exclusiveDescriptor < 0 || m_temporary.empty())
        return false;
#if defined(_WIN32)
    // The CRT _stat64 st_ino field is not a stable Windows file identity
    // (and can differ between _fstat64 and _wstat64).  Compare the native
    // volume/file-index tuple instead, opening the pathname read-only with
    // read/write sharing while the exclusive descriptor remains owned.
    const intptr_t nativeDescriptor = _get_osfhandle(m_exclusiveDescriptor);
    if (nativeDescriptor == static_cast<intptr_t>(-1))
        return false;
    BY_HANDLE_FILE_INFORMATION descriptorInfo {
    };
    if (GetFileInformationByHandle(
            reinterpret_cast<HANDLE>(nativeDescriptor), &descriptorInfo)
        == 0)
        return false;
    const HANDLE pathHandle = CreateFileW(
        m_temporary.c_str(), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (pathHandle == INVALID_HANDLE_VALUE)
        return false;
    BY_HANDLE_FILE_INFORMATION pathInfo {
    };
    const bool pathInfoOk = GetFileInformationByHandle(pathHandle, &pathInfo)
        != 0;
    CloseHandle(pathHandle);
    return pathInfoOk
        && descriptorInfo.dwVolumeSerialNumber == pathInfo.dwVolumeSerialNumber
        && descriptorInfo.nFileIndexHigh == pathInfo.nFileIndexHigh
        && descriptorInfo.nFileIndexLow == pathInfo.nFileIndexLow;
#else
    struct stat descriptorStatus {
    };
    struct stat pathStatus {
    };
    const char* temporaryName = temporaryFileName(m_temporary);
    if (m_directoryDescriptor < 0 || *temporaryName == '\0')
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
           && ::fstatat(m_directoryDescriptor, temporaryName,
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
    const auto& directory = m_directory;
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
    const bool streamOpen = m_stream.is_open();
    const bool streamGood = m_stream.good();
    const bool identityMatches = temporaryIdentityMatches();
    if (!streamOpen || !streamGood || !identityMatches) {
#if defined(_WIN32)
        std::fprintf(stderr,
                     "DwgDxfOutputTransaction open failed: stream=%d good=%d "
                     "identity=%d errno=%d gle=%lu\n",
                     streamOpen ? 1 : 0, streamGood ? 1 : 0,
                     identityMatches ? 1 : 0, errno,
                     static_cast<unsigned long>(GetLastError()));
#endif
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
    // An overwrite keeps the mode the target had, exactly.  The temporary was
    // already created asking for those bits, but open() subtracts the umask
    // from what it is asked for, so a target the umask would have masked --
    // 0664 under umask 022, say -- still needs the difference restored here.
    // That only ever widens within the target's own bits, and only at the
    // instant the temporary becomes the target.  A failure is not fatal: the
    // content is correct either way, and refusing to publish it over a
    // permissions detail would lose the drawing.
    if (m_targetMode >= 0 && m_exclusiveDescriptor >= 0)
        ::fchmod(m_exclusiveDescriptor, static_cast<mode_t>(m_targetMode));

    const char* temporaryName = temporaryFileName(m_temporary);
    const auto& targetName = m_targetName;
    return ::renameat(m_directoryDescriptor, temporaryName,
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
    if (!temporaryIdentityMatches() || !directoryIdentityMatchesPath()
        || !targetSnapshotMatches()) {
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
    if (m_stream.fail()) {
        abort();
        return false;
    }
#if defined(_WIN32)
    // MoveFileExW cannot replace a pathname while the CRT descriptor created
    // by _wopen is still open: the CRT handle does not grant FILE_SHARE_DELETE.
    // Verify ownership while the descriptor is available, then close it before
    // publishing.  A failed publish removes only the uniquely named temporary
    // file and leaves the target untouched.
    if (!temporaryIdentityMatches() || !directoryIdentityMatchesPath()) {
        abort();
        return false;
    }
    closeExclusiveDescriptor();
    if (!publish()) {
        std::error_code ignored;
        std::filesystem::remove(m_temporary, ignored);
        m_temporary.clear();
        return false;
    }
#else
    if (!temporaryIdentityMatches() || !directoryIdentityMatchesPath()
        || !targetSnapshotMatches() || !publish()) {
        abort();
        return false;
    }
#endif
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
            // The CRT descriptor does not grant FILE_SHARE_DELETE, so the
            // pathname cannot be removed until the descriptor is closed.
            // Close only after the identity check above; an attacker-owned
            // replacement must never be removed by abort().
            closeExclusiveDescriptor();
            std::error_code ignored;
            std::filesystem::remove(m_temporary, ignored);
        } else {
            closeExclusiveDescriptor();
        }
#else
        if (owned && m_directoryDescriptor >= 0) {
            const char* temporaryName = temporaryFileName(m_temporary);
            if (*temporaryName != '\0')
                (void)::unlinkat(m_directoryDescriptor, temporaryName,
                                 0);
        }
#endif
        closeExclusiveDescriptor();
        closeDirectoryDescriptor();
        m_temporary.clear();
    }
}
