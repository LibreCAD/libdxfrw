#include "dx_batch_output_names.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <cerrno>
#include <fcntl.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <unistd.h>
#elif defined(__linux__)
#include <cerrno>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/magic.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/statfs.h>
#include <unistd.h>
#else
// Unsupported providers compile cleanly but batch conversion fails closed.
#endif

namespace {

struct CollationProfile {
    std::uint64_t volume = 0;
    bool caseSensitive = false;
};

#if defined(_WIN32)

struct WinIdentity {
    DWORD volume = 0;
    DWORD indexHigh = 0;
    DWORD indexLow = 0;
};

bool sameIdentity(const WinIdentity& left, const WinIdentity& right) {
    return left.volume == right.volume && left.indexHigh == right.indexHigh
        && left.indexLow == right.indexLow;
}

bool queryIdentity(const std::filesystem::path& path, WinIdentity& identity,
                   bool rejectReparse, std::string& error) {
    const DWORD flags = FILE_FLAG_BACKUP_SEMANTICS
        | (rejectReparse ? FILE_FLAG_OPEN_REPARSE_POINT : 0);
    HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, flags, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = "cannot inspect temporary output-name entry";
        return false;
    }
    BY_HANDLE_FILE_INFORMATION info{};
    const bool inspected = GetFileInformationByHandle(handle, &info) != 0;
    CloseHandle(handle);
    if (!inspected || !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        || (rejectReparse && (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))) {
        error = "temporary output-name entry identity is unavailable";
        return false;
    }
    identity.volume = info.dwVolumeSerialNumber;
    identity.indexHigh = info.nFileIndexHigh;
    identity.indexLow = info.nFileIndexLow;
    return true;
}

bool queryCollationProfile(const std::filesystem::path& path,
                           CollationProfile& profile, std::string& error) {
    HANDLE handle = CreateFileW(path.c_str(), FILE_READ_ATTRIBUTES,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        error = "cannot query output-directory filename capabilities";
        return false;
    }
    BY_HANDLE_FILE_INFORMATION fileInfo{};
    struct CaseSensitiveInfo {
        DWORD flags;
    } caseInfo{};
    // FILE_INFO_BY_HANDLE_CLASS assigns value 23 to FileCaseSensitiveInfo.
    const auto caseSensitiveInfoClass = static_cast<FILE_INFO_BY_HANDLE_CLASS>(23);
    const bool queried = GetFileInformationByHandle(handle, &fileInfo) != 0
        && GetFileInformationByHandleEx(handle, caseSensitiveInfoClass,
                                        &caseInfo, sizeof(caseInfo)) != 0;
    CloseHandle(handle);
    if (!queried) {
        error = "output filesystem does not expose directory case-sensitivity";
        return false;
    }
    profile.volume = fileInfo.dwVolumeSerialNumber;
    profile.caseSensitive = (caseInfo.flags & 0x1u) != 0;
    return true;
}

class OwnedNameNamespace {
public:
    explicit OwnedNameNamespace(std::size_t capacity) {
        entries_.reserve(capacity);
    }

    ~OwnedNameNamespace() noexcept {
        try {
            std::string ignored;
            cleanup(ignored);
        } catch (...) {
        }
    }

    bool create(const std::filesystem::path& outputDirectory, std::string& error) {
        outputDirectory_ = outputDirectory;
        if (!queryIdentity(outputDirectory_, outputIdentity_, false, error))
            return false;
        for (unsigned int attempt = 0; attempt < 64; ++attempt) {
            workDirectory_ = outputDirectory_
                / (L".dwg2dxf-namecheck-" + uniqueSuffix(attempt));
            if (CreateDirectoryW(workDirectory_.c_str(), nullptr)) {
                created_ = true;
                if (!queryIdentity(workDirectory_, workIdentity_, true, error))
                    return false;
                return true;
            }
            if (GetLastError() != ERROR_ALREADY_EXISTS) {
                error = "cannot create owned output-name namespace";
                return false;
            }
        }
        error = "cannot allocate a unique output-name namespace";
        return false;
    }

    bool add(const std::filesystem::path& filename, bool& collision,
             std::string& error) {
        collision = false;
        if (filename.empty() || filename == L"." || filename == L".."
            || filename.has_parent_path()) {
            error = "output filename is not a single safe path component";
            return false;
        }
        auto entry = workDirectory_ / filename;
        if (!CreateDirectoryW(entry.c_str(), nullptr)) {
            if (GetLastError() == ERROR_ALREADY_EXISTS) {
                collision = true;
                return true;
            }
            error = "cannot validate an output filename on this filesystem";
            return false;
        }
        WinIdentity identity;
        if (!queryIdentity(entry, identity, true, error)) {
            return false;
        }
        entries_.push_back({std::move(entry), identity});
        return true;
    }

    bool cleanup(std::string& error) {
        if (cleaned_)
            return true;
        bool clean = true;
        while (!entries_.empty()) {
            const auto& entry = entries_.back();
            WinIdentity current;
            std::string identityError;
            if (!queryIdentity(entry.path, current, true, identityError)
                || !sameIdentity(current, entry.identity)
                || !RemoveDirectoryW(entry.path.c_str())) {
                clean = false;
                if (error.empty())
                    error = "cannot safely remove owned output-name marker";
                break;
            }
            entries_.pop_back();
        }
        if (entries_.empty() && created_) {
            WinIdentity parent;
            std::string identityError;
            if (!queryIdentity(outputDirectory_, parent, false, identityError)
                || !sameIdentity(parent, outputIdentity_)) {
                clean = false;
                if (error.empty())
                    error = "output directory identity changed during preflight";
            } else {
                WinIdentity current;
                if (!queryIdentity(workDirectory_, current, true, identityError)
                    || !sameIdentity(current, workIdentity_)
                    || !RemoveDirectoryW(workDirectory_.c_str())) {
                    clean = false;
                    if (error.empty())
                        error = "cannot safely remove owned output-name namespace";
                } else {
                    created_ = false;
                }
            }
        }
        if (clean && entries_.empty() && !created_)
            cleaned_ = true;
        return clean;
    }

    const std::filesystem::path& workDirectory() const { return workDirectory_; }

private:
    struct Entry {
        std::filesystem::path path;
        WinIdentity identity;
    };

    static std::wstring uniqueSuffix(unsigned int attempt) {
        static std::atomic<std::uint64_t> counter{0};
        const auto ticks = static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        const auto value = ticks ^ counter.fetch_add(1, std::memory_order_relaxed)
            ^ static_cast<std::uint64_t>(GetCurrentProcessId());
        return std::to_wstring(value) + L"-" + std::to_wstring(attempt);
    }

    std::filesystem::path outputDirectory_;
    std::filesystem::path workDirectory_;
    WinIdentity outputIdentity_{};
    WinIdentity workIdentity_{};
    std::vector<Entry> entries_;
    bool created_ = false;
    bool cleaned_ = false;
};

#elif defined(__APPLE__)

struct PosixIdentity {
    dev_t device = 0;
    ino_t inode = 0;
};

bool sameIdentity(const PosixIdentity& left, const PosixIdentity& right) {
    return left.device == right.device && left.inode == right.inode;
}

bool queryCollationProfile(const std::filesystem::path& path,
                           CollationProfile& profile, std::string& error) {
    std::error_code pathError;
    const auto absolutePath = std::filesystem::absolute(path, pathError);
    if (pathError) {
        error = "cannot resolve output-directory path for volume query";
        return false;
    }
    struct statfs fsInfo{};
    struct stat fileInfo{};
    if (::statfs(absolutePath.c_str(), &fsInfo) != 0
        || ::stat(absolutePath.c_str(), &fileInfo) != 0) {
        error = "cannot query output-directory filename capabilities";
        return false;
    }
    if (std::string(fsInfo.f_fstypename) != "apfs") {
        error = "output filesystem is not a qualified APFS volume";
        return false;
    }
    const auto native = absolutePath.native();
    CFURLRef url = CFURLCreateFromFileSystemRepresentation(
        kCFAllocatorDefault,
        reinterpret_cast<const UInt8*>(native.data()),
        static_cast<CFIndex>(native.size()), true);
    if (url == nullptr) {
        error = "cannot construct output-directory volume query";
        return false;
    }
    CFTypeRef caseValue = nullptr;
    const Boolean queried = CFURLCopyResourcePropertyForKey(
        url, kCFURLVolumeSupportsCaseSensitiveNamesKey, &caseValue, nullptr);
    CFRelease(url);
    if (!queried || caseValue == nullptr
        || CFGetTypeID(caseValue) != CFBooleanGetTypeID()) {
        if (caseValue != nullptr)
            CFRelease(caseValue);
        error = "APFS volume did not report case-sensitivity capability";
        return false;
    }
    profile.volume = static_cast<std::uint64_t>(fileInfo.st_dev);
    profile.caseSensitive = CFBooleanGetValue(static_cast<CFBooleanRef>(caseValue)) != 0;
    CFRelease(caseValue);
    return true;
}

class OwnedNameNamespace {
public:
    explicit OwnedNameNamespace(std::size_t capacity) : entries_() {
        entries_.reserve(capacity);
    }

    ~OwnedNameNamespace() noexcept {
        try {
            std::string ignored;
            cleanup(ignored);
        } catch (...) {
        }
    }

    bool create(const std::filesystem::path& outputDirectory, std::string& error) {
        outputFd_ = ::open(outputDirectory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (outputFd_ < 0) {
            error = "cannot open output directory for safe name preflight";
            return false;
        }
        struct stat outputInfo{};
        if (::fstat(outputFd_, &outputInfo) != 0 || !S_ISDIR(outputInfo.st_mode)) {
            error = "cannot identify output directory for safe name preflight";
            return false;
        }
        for (unsigned int attempt = 0; attempt < 64; ++attempt) {
            workName_ = uniqueName(attempt);
            if (::mkdirat(outputFd_, workName_.c_str(), 0700) == 0) {
                created_ = true;
                workFd_ = ::openat(outputFd_, workName_.c_str(),
                                   O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
                struct stat workInfo{};
                if (workFd_ < 0 || ::fstat(workFd_, &workInfo) != 0
                    || !S_ISDIR(workInfo.st_mode)) {
                    if (::fstatat(outputFd_, workName_.c_str(), &workInfo,
                                  AT_SYMLINK_NOFOLLOW) == 0
                        && S_ISDIR(workInfo.st_mode))
                        workIdentity_ = {workInfo.st_dev, workInfo.st_ino};
                    error = "cannot anchor owned output-name namespace";
                    return false;
                }
                workIdentity_ = {workInfo.st_dev, workInfo.st_ino};
                return true;
            }
            if (errno != EEXIST) {
                error = "cannot create owned output-name namespace";
                return false;
            }
        }
        error = "cannot allocate a unique output-name namespace";
        return false;
    }

    bool add(const std::filesystem::path& filename, bool& collision,
             std::string& error) {
        collision = false;
        if (filename.empty() || filename == "." || filename == ".."
            || filename.has_parent_path()) {
            error = "output filename is not a single safe path component";
            return false;
        }
        const std::string name = filename.native();
        if (::mkdirat(workFd_, name.c_str(), 0700) == 0) {
            struct stat createdInfo{};
            if (::fstatat(workFd_, name.c_str(), &createdInfo, AT_SYMLINK_NOFOLLOW) != 0
                || !S_ISDIR(createdInfo.st_mode)) {
                error = "cannot identify owned output-name marker";
                return false;
            }
            entries_.push_back({std::move(name), {createdInfo.st_dev, createdInfo.st_ino}});
            return true;
        }
        if (errno == EEXIST) {
            collision = true;
            return true;
        }
        error = "cannot validate an output filename on this filesystem";
        return false;
    }

    bool cleanup(std::string& error) {
        if (cleaned_)
            return true;
        bool clean = true;
        while (!entries_.empty()) {
            const auto& entry = entries_.back();
            struct stat current{};
            const bool found = ::fstatat(workFd_, entry.name.c_str(), &current,
                                         AT_SYMLINK_NOFOLLOW) == 0;
            const PosixIdentity currentIdentity{current.st_dev, current.st_ino};
            if (!found || !S_ISDIR(current.st_mode)
                || !sameIdentity(currentIdentity, entry.identity)
                || ::unlinkat(workFd_, entry.name.c_str(), AT_REMOVEDIR) != 0) {
                clean = false;
                if (error.empty())
                    error = "cannot safely remove owned output-name marker";
                break;
            }
            entries_.pop_back();
        }
        if (entries_.empty() && created_) {
            struct stat current{};
            const bool found = ::fstatat(outputFd_, workName_.c_str(), &current,
                                         AT_SYMLINK_NOFOLLOW) == 0;
            const PosixIdentity currentIdentity{current.st_dev, current.st_ino};
            if (!found || !S_ISDIR(current.st_mode)
                || !sameIdentity(currentIdentity, workIdentity_)
                || ::unlinkat(outputFd_, workName_.c_str(), AT_REMOVEDIR) != 0) {
                clean = false;
                if (error.empty())
                    error = "cannot safely remove owned output-name namespace";
            } else {
                created_ = false;
            }
        }
        if (clean && entries_.empty() && !created_) {
            if (workFd_ >= 0) {
                ::close(workFd_);
                workFd_ = -1;
            }
            if (outputFd_ >= 0) {
                ::close(outputFd_);
                outputFd_ = -1;
            }
            cleaned_ = true;
        }
        return clean;
    }

    std::filesystem::path workPath(const std::filesystem::path& outputDirectory) const {
        return outputDirectory / std::filesystem::u8path(workName_);
    }

private:
    struct Entry {
        std::string name;
        PosixIdentity identity;
    };

    static std::string uniqueName(unsigned int attempt) {
        static std::atomic<std::uint64_t> counter{0};
        const auto ticks = static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        const auto value = ticks ^ counter.fetch_add(1, std::memory_order_relaxed)
            ^ static_cast<std::uint64_t>(::getpid());
        return ".dwg2dxf-namecheck-" + std::to_string(value) + "-"
            + std::to_string(attempt);
    }

    int outputFd_ = -1;
    int workFd_ = -1;
    std::string workName_;
    PosixIdentity workIdentity_{};
    std::vector<Entry> entries_;
    bool created_ = false;
    bool cleaned_ = false;
};

#elif defined(__linux__)

struct PosixIdentity {
    dev_t device = 0;
    ino_t inode = 0;
};

bool sameIdentity(const PosixIdentity& left, const PosixIdentity& right) {
    return left.device == right.device && left.inode == right.inode;
}

bool queryCollationProfile(const std::filesystem::path& path,
                           CollationProfile& profile, std::string& error) {
    struct statfs fsInfo{};
    struct stat fileInfo{};
    if (::statfs(path.c_str(), &fsInfo) != 0 || ::stat(path.c_str(), &fileInfo) != 0) {
        error = "cannot query output-directory filename capabilities";
        return false;
    }
#if defined(EXT4_SUPER_MAGIC) && defined(FS_CASEFOLD_FL) && defined(FS_IOC_GETFLAGS)
    if (static_cast<unsigned long>(fsInfo.f_type) != EXT4_SUPER_MAGIC) {
        error = "output filesystem is not a qualified ext4 volume";
        return false;
    }
    const int fd = ::open(path.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd < 0) {
        error = "cannot open output-directory filename capabilities";
        return false;
    }
    int flags = 0;
    const bool queried = ::ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0;
    ::close(fd);
    if (!queried) {
        error = "ext4 did not expose directory casefold capability";
        return false;
    }
    profile.volume = static_cast<std::uint64_t>(fileInfo.st_dev);
    profile.caseSensitive = (flags & FS_CASEFOLD_FL) == 0;
    return true;
#else
    (void)fsInfo;
    (void)fileInfo;
    error = "build headers do not expose ext4 casefold capability";
    return false;
#endif
}

class OwnedNameNamespace {
public:
    explicit OwnedNameNamespace(std::size_t capacity) {
        entries_.reserve(capacity);
    }

    ~OwnedNameNamespace() noexcept {
        try {
            std::string ignored;
            cleanup(ignored);
        } catch (...) {
        }
    }

    bool create(const std::filesystem::path& outputDirectory, std::string& error) {
        outputFd_ = ::open(outputDirectory.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
        if (outputFd_ < 0) {
            error = "cannot open output directory for safe name preflight";
            return false;
        }
        struct stat outputInfo{};
        if (::fstat(outputFd_, &outputInfo) != 0 || !S_ISDIR(outputInfo.st_mode)) {
            error = "cannot identify output directory for safe name preflight";
            return false;
        }
        for (unsigned int attempt = 0; attempt < 64; ++attempt) {
            workName_ = uniqueName(attempt);
            if (::mkdirat(outputFd_, workName_.c_str(), 0700) == 0) {
                created_ = true;
                workFd_ = ::openat(outputFd_, workName_.c_str(),
                                   O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
                struct stat workInfo{};
                if (workFd_ < 0 || ::fstat(workFd_, &workInfo) != 0
                    || !S_ISDIR(workInfo.st_mode)) {
                    error = "cannot anchor owned output-name namespace";
                    return false;
                }
                workIdentity_ = {workInfo.st_dev, workInfo.st_ino};
                return true;
            }
            if (errno != EEXIST) {
                error = "cannot create owned output-name namespace";
                return false;
            }
        }
        error = "cannot allocate a unique output-name namespace";
        return false;
    }

    bool add(const std::filesystem::path& filename, bool& collision,
             std::string& error) {
        collision = false;
        if (filename.empty() || filename == "." || filename == ".."
            || filename.has_parent_path()) {
            error = "output filename is not a single safe path component";
            return false;
        }
        const std::string name = filename.native();
        if (::mkdirat(workFd_, name.c_str(), 0700) == 0) {
            struct stat createdInfo{};
            if (::fstatat(workFd_, name.c_str(), &createdInfo, AT_SYMLINK_NOFOLLOW) != 0
                || !S_ISDIR(createdInfo.st_mode)) {
                error = "cannot identify owned output-name marker";
                return false;
            }
            entries_.push_back({std::move(name), {createdInfo.st_dev, createdInfo.st_ino}});
            return true;
        }
        if (errno == EEXIST) {
            collision = true;
            return true;
        }
        error = "cannot validate an output filename on this filesystem";
        return false;
    }

    bool cleanup(std::string& error) {
        if (cleaned_)
            return true;
        bool clean = true;
        while (!entries_.empty()) {
            const auto& entry = entries_.back();
            struct stat current{};
            const bool found = ::fstatat(workFd_, entry.name.c_str(), &current,
                                         AT_SYMLINK_NOFOLLOW) == 0;
            const PosixIdentity currentIdentity{current.st_dev, current.st_ino};
            if (!found || !S_ISDIR(current.st_mode)
                || !sameIdentity(currentIdentity, entry.identity)
                || ::unlinkat(workFd_, entry.name.c_str(), AT_REMOVEDIR) != 0) {
                clean = false;
                if (error.empty())
                    error = "cannot safely remove owned output-name marker";
                break;
            }
            entries_.pop_back();
        }
        if (entries_.empty() && created_) {
            struct stat current{};
            const bool found = ::fstatat(outputFd_, workName_.c_str(), &current,
                                         AT_SYMLINK_NOFOLLOW) == 0;
            const PosixIdentity currentIdentity{current.st_dev, current.st_ino};
            if (!found || !S_ISDIR(current.st_mode)
                || !sameIdentity(currentIdentity, workIdentity_)
                || ::unlinkat(outputFd_, workName_.c_str(), AT_REMOVEDIR) != 0) {
                clean = false;
                if (error.empty())
                    error = "cannot safely remove owned output-name namespace";
            } else {
                created_ = false;
            }
        }
        if (clean && entries_.empty() && !created_) {
            if (workFd_ >= 0) {
                ::close(workFd_);
                workFd_ = -1;
            }
            if (outputFd_ >= 0) {
                ::close(outputFd_);
                outputFd_ = -1;
            }
            cleaned_ = true;
        }
        return clean;
    }

    std::filesystem::path workPath(const std::filesystem::path& outputDirectory) const {
        return outputDirectory / std::filesystem::u8path(workName_);
    }

private:
    struct Entry {
        std::string name;
        PosixIdentity identity;
    };

    static std::string uniqueName(unsigned int attempt) {
        static std::atomic<std::uint64_t> counter{0};
        const auto ticks = static_cast<std::uint64_t>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        const auto value = ticks ^ counter.fetch_add(1, std::memory_order_relaxed)
            ^ static_cast<std::uint64_t>(::getpid());
        return ".dwg2dxf-namecheck-" + std::to_string(value) + "-"
            + std::to_string(attempt);
    }

    int outputFd_ = -1;
    int workFd_ = -1;
    std::string workName_;
    PosixIdentity workIdentity_{};
    std::vector<Entry> entries_;
    bool created_ = false;
    bool cleaned_ = false;
};

#else

bool queryCollationProfile(const std::filesystem::path&,
                           CollationProfile&, std::string& error) {
    error = "this platform has no qualified output-name collation adapter";
    return false;
}

class OwnedNameNamespace {
public:
    explicit OwnedNameNamespace(std::size_t) {}
    bool create(const std::filesystem::path&, std::string& error) {
        error = "this platform has no qualified output-name collation adapter";
        return false;
    }
    bool add(const std::filesystem::path&, bool&, std::string& error) {
        error = "this platform has no qualified output-name collation adapter";
        return false;
    }
    bool cleanup(std::string&) { return true; }
    std::filesystem::path workPath(const std::filesystem::path& path) const {
        return path;
    }
};

#endif

bool sameCollationProfile(const std::filesystem::path& parent,
                          const std::filesystem::path& child,
                          std::string& error) {
    CollationProfile parentProfile;
    CollationProfile childProfile;
    if (!queryCollationProfile(parent, parentProfile, error)
        || !queryCollationProfile(child, childProfile, error))
        return false;
    if (parentProfile.volume != childProfile.volume
        || parentProfile.caseSensitive != childProfile.caseSensitive) {
        error = "temporary namespace filename behavior differs from output directory";
        return false;
    }
    return true;
}

} // namespace

namespace dwg2dxfCli {

BatchOutputNameCheck checkBatchOutputNameCollisions(
        const std::filesystem::path& outputDirectory,
        const std::vector<std::filesystem::path>& outputs,
        std::string& error) {
    error.clear();
    // Creating each complete basename in this owned namespace asks the actual
    // filesystem to resolve case and normalization. No user destination is
    // opened or overwritten, and every created entry is identity-checked on
    // cleanup before batch conversion begins.
    OwnedNameNamespace nameSpace(outputs.size());
    if (!nameSpace.create(outputDirectory, error)) {
        std::string cleanupError;
        if (!nameSpace.cleanup(cleanupError) && error.empty())
            error = cleanupError;
        return BatchOutputNameCheck::Error;
    }
    const auto workDirectory = nameSpace.workPath(outputDirectory);
    if (!sameCollationProfile(outputDirectory, workDirectory, error)) {
        std::string cleanupError;
        if (!nameSpace.cleanup(cleanupError))
            error = cleanupError;
        return BatchOutputNameCheck::Error;
    }
    for (const auto& output : outputs) {
        bool collision = false;
        if (!nameSpace.add(output.filename(), collision, error)) {
            std::string cleanupError;
            if (!nameSpace.cleanup(cleanupError))
                error = cleanupError;
            return BatchOutputNameCheck::Error;
        }
        if (collision) {
            std::string cleanupError;
            if (!nameSpace.cleanup(cleanupError)) {
                error = cleanupError;
                return BatchOutputNameCheck::Error;
            }
            return BatchOutputNameCheck::Collision;
        }
    }
    if (!nameSpace.cleanup(error))
        return BatchOutputNameCheck::Error;
    return BatchOutputNameCheck::Clear;
}

} // namespace dwg2dxfCli
