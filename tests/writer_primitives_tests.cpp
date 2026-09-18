#include <cstdint>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "handle_allocator.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/dwg_dxf_output_transaction.h"
#include "intern/dwgwriter15.h"
#include "libdwgr.h"

#if !defined(_WIN32)
#  include <sys/stat.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif

namespace {

struct TestContext {
    int failures {0};

    void expect(bool condition, const char* label) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << label << '\n';
        }
    }
};

class WriterProbe final : public dwgWriter15 {
public:
    using dwgWriter15::dwgWriter15;
    using dwgWriter::allocNextHandle;
    using dwgWriter::highWaterHandle;
    using dwgWriter::reserveHandle;
    using dwgWriter15::beginObject;
    using dwgWriter15::finishObject;
};

void testModularVectors(TestContext& t) {
    const std::vector<std::uint64_t> unsignedValues {0, 1, 127, 128,
                                                       16383, 16384,
                                                       0xFFFFFFFFu};
    for (const std::uint64_t value : unsignedValues) {
        dwgBufferW writer;
        t.expect(writer.putUModularChar(value),
                 "UMC accepts representable value");
        dwgBuffer reader(writer.data().data(), writer.data().size());
        t.expect(reader.getUModularChar() == value && reader.isGood(),
                 "UMC round-trips through reader");
    }

    const std::vector<std::int64_t> signedValues {-16384, -127, -64, -1,
                                                   0, 1, 63, 64, 127,
                                                   16384};
    for (const std::int64_t value : signedValues) {
        dwgBufferW writer;
        t.expect(writer.putModularChar(value),
                 "MC accepts representable value");
        dwgBuffer reader(writer.data().data(), writer.data().size());
        t.expect(reader.getModularChar() == value && reader.isGood(),
                 "MC round-trips through reader");
    }

    const std::vector<std::int32_t> shortValues {0, 0x7FFF, 0x8000,
                                                  0x3FFFFFFF};
    for (const std::int32_t value : shortValues) {
        dwgBufferW writer;
        writer.putModularShort(value);
        dwgBuffer reader(writer.data().data(), writer.data().size());
        t.expect(reader.getModularShort() == value && reader.isGood(),
                 "MS round-trips through reader");
    }

    dwgBufferW invalid;
    t.expect(!invalid.putUModularChar(std::uint64_t{1} << 35)
                 && !invalid.isGood() && invalid.data().empty(),
             "UMC rejects overflow without emitting bytes");
}

void testBitAndRawVectors(TestContext& t) {
    dwgBufferW writer;
    writer.putBitShort(0);
    writer.putBitShort(256);
    writer.putBitLong(-7);
    writer.putBitDouble(1.0);
    writer.putBitDouble(-2.5);
    writer.alignToByte();
    writer.putRawShort16(0xBEEF);
    writer.putRawLong32(0x12345678u);
    writer.putRawLong64(0x0123456789ABCDEFULL);

    dwgBuffer reader(writer.data().data(), writer.data().size());
    t.expect(reader.getBitShort() == 0, "BS zero shortcut");
    t.expect(reader.getBitShort() == 256, "BS 256 shortcut");
    t.expect(reader.getBitLong() == -7, "BL signed value");
    t.expect(reader.getBitDouble() == 1.0, "BD one shortcut");
    t.expect(reader.getBitDouble() == -2.5, "BD raw value");
    reader.setPosition(writer.data().size() - 14);
    t.expect(reader.getRawShort16() == 0xBEEF, "RS little-endian value");
    t.expect(reader.getRawLong32() == 0x12345678u, "RL little-endian value");
    t.expect(reader.getRawLong64() == 0x0123456789ABCDEFULL
                 && reader.isGood(),
             "RLL little-endian value");
}

void testHandleOccurrences(TestContext& t) {
    dwgBufferW writer;
    writer.putFixedHandle(5, 3, 0x123456u);
    const auto& occurrences = writer.handleOccurrences();
    t.expect(occurrences.size() == 1 && occurrences.front().code == 5
                 && occurrences.front().reference == 0x123456u
                 && occurrences.front().startBit == 0
                 && occurrences.front().endBit == 32,
             "fixed handle records exact token range");
    writer.truncate(0);
    t.expect(writer.handleOccurrences().empty() && writer.size() == 0,
             "buffer truncate clears handle tokens");
}

void testAllocatorVectors(TestContext& t) {
    HandleAllocator allocator;
    allocator.seedReserved();
    t.expect(allocator.next() == 0x30u, "seeded allocator starts at user range");
    allocator.reserve(0x31u);
    t.expect(allocator.next() == 0x32u && allocator.current() == 0x33u,
             "allocator skips explicit reservation deterministically");
    allocator.resetGenerated();
    t.expect(allocator.current() == 0x32u && allocator.next() == 0x32u,
             "allocator reset retains source reservation only");

    HandleAllocator high;
    high.reserve((std::numeric_limits<std::uint32_t>::max)() - 1u);
    t.expect(high.current() == (std::numeric_limits<std::uint32_t>::max)(),
             "allocator advances to maximum after high reservation");
    bool threw = false;
    try {
        high.next();
    } catch (const std::overflow_error&) {
        threw = true;
    }
    t.expect(threw, "allocator rejects exhaustion instead of wrapping");
}

void testWriterReservationVectors(TestContext& t) {
    DRW_Header header;
    WriterProbe writer(nullptr, &header);
    t.expect(writer.highWaterHandle() == 0x30u,
             "DWG writer seeds fixed handles below user range");
    writer.reserveHandle(0x30u);
    t.expect(writer.allocNextHandle() == 0x31u,
             "writer reservation prevents a preserved handle collision");
    writer.reserveHandle(0x40u);
    t.expect(writer.highWaterHandle() == 0x41u
                 && writer.allocNextHandle() == 0x41u,
             "writer high-water follows explicit source reservation");
}

void testFrameReceiptAndRollback(TestContext& t) {
    DRW_Header header;
    WriterProbe writer(nullptr, &header);
    const auto checkpoint = writer.checkpointCompoundWrite();
    writer.setDwgObjectFrameProvenance(42, 7, 99);
    dwgBufferW& body = writer.beginObject(0x42u);
    body.putObjType(DRW::AC1015, 42);
    body.putFixedHandle(5, 2, 0x1234u);
    writer.finishObject();

    DRW::DwgObjectFrameReceipt receipt;
    t.expect(writer.getLastDwgObjectFrame(receipt) && receipt.valid
                 && receipt.objectHandle == 0x42u
                 && receipt.version == DRW::AC1015
                 && receipt.classNumber == 42
                 && receipt.writerOperation == 7
                 && receipt.admissionToken == 99
                 && receipt.generation != 0
                 && receipt.occurrences.size() == 1,
             "writer publishes one complete frame receipt with provenance");
    t.expect(!writer.buffer().empty(),
             "writer frame publication appends serialized bytes");

    writer.rollbackCompoundWrite(checkpoint);
    DRW::DwgObjectFrameReceipt cleared;
    t.expect(writer.buffer().empty()
                 && !writer.getLastDwgObjectFrame(cleared),
             "writer rollback removes bytes and clears receipt");
}

void testWriteRejectionDoesNotTouchDestination(TestContext& t) {
    const std::filesystem::path path =
        std::filesystem::temp_directory_path()
        / "libdxfrw-writer-failure-gate.dwg";
    const std::string pathString = path.string();
    {
        std::ofstream seed(path, std::ios::binary | std::ios::trunc);
        seed << "sentinel";
    }

    dwgRW nullInterface(pathString.c_str());
    t.expect(!nullInterface.write(nullptr, DRW::AC1015, false)
                 && nullInterface.getError() == DRW::BAD_UNKNOWN,
             "supported-version write rejects null interface");
    std::ifstream afterNull(path, std::ios::binary);
    const std::string nullContents((std::istreambuf_iterator<char>(afterNull)),
                                   std::istreambuf_iterator<char>());
    t.expect(nullContents == "sentinel",
             "null-interface rejection leaves destination untouched");

    dwgRW unsupported(pathString.c_str());
    t.expect(!unsupported.write(nullptr, DRW::UNKNOWNV, false)
                 && unsupported.getError() == DRW::BAD_VERSION,
             "unsupported-version write rejects before opening destination");
    std::ifstream afterVersion(path, std::ios::binary);
    const std::string versionContents(
        (std::istreambuf_iterator<char>(afterVersion)),
        std::istreambuf_iterator<char>());
    t.expect(versionContents == "sentinel",
             "unsupported-version rejection leaves destination untouched");

    std::remove(pathString.c_str());
}

std::filesystem::path transactionTestPath(const char* suffix) {
    const auto stamp = std::chrono::steady_clock::now()
                           .time_since_epoch().count();
    return std::filesystem::temp_directory_path()
           / (std::string("libdxfrw-s248-") + suffix + "-"
              + std::to_string(stamp) + ".dwg");
}

std::vector<std::filesystem::path> transactionTemporaryPaths(
    const std::filesystem::path& target) {
    const std::filesystem::path directory =
        target.parent_path().empty() ? std::filesystem::path(".")
                                     : target.parent_path();
    const std::string prefix = target.filename().string() + ".libdxfrw-";
    std::vector<std::filesystem::path> paths;
    std::error_code error;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::directory_iterator(directory, error)) {
        if (error)
            break;
        const std::string name = entry.path().filename().string();
        if (name.rfind(prefix, 0) == 0)
            paths.push_back(entry.path());
    }
    return paths;
}

std::size_t transactionTemporaryCount(const std::filesystem::path& target) {
    return transactionTemporaryPaths(target).size();
}

void testOutputTransactionPublicationAndRollback(TestContext& t) {
    const std::filesystem::path target = transactionTestPath("publish");
    std::error_code ignored;
    std::filesystem::remove(target, ignored);

    {
        DwgDxfOutputTransaction transaction(target.string(), std::ios::binary);
        t.expect(transaction.open(),
                 "output transaction opens an exclusive temporary");
        transaction.stream() << "published";
        t.expect(transaction.commit(),
                 "output transaction commits a flushed temporary");
        t.expect(!transaction.isOpen(),
                 "committed output transaction closes its stream");
    }

    std::ifstream published(target, std::ios::binary);
    const std::string contents((std::istreambuf_iterator<char>(published)),
                               std::istreambuf_iterator<char>());
    t.expect(contents == "published",
             "committed output transaction atomically publishes content");
    t.expect(transactionTemporaryCount(target) == 0,
             "committed output transaction leaves no temporary file");

    {
        DwgDxfOutputTransaction first(target.string(), std::ios::binary);
        DwgDxfOutputTransaction second(target.string(), std::ios::binary);
        t.expect(first.open() && second.open(),
                 "parallel output transactions obtain distinct temporaries");
        t.expect(transactionTemporaryCount(target) == 2,
                 "parallel output transactions do not collide on names");
        first.abort();
        second.abort();
        t.expect(transactionTemporaryCount(target) == 0,
                 "parallel output transaction aborts clean up both files");
    }

    {
        std::ofstream seed(target, std::ios::binary | std::ios::trunc);
        seed << "original";
    }
    const std::size_t before = transactionTemporaryCount(target);
    {
        DwgDxfOutputTransaction transaction(target.string(), std::ios::binary);
        t.expect(transaction.open(),
                 "rollback transaction opens an exclusive temporary");
        transaction.stream() << "discarded";
        transaction.abort();
        t.expect(!transaction.isOpen(),
                 "aborted output transaction closes its stream");
        t.expect(transactionTemporaryCount(target) == before,
                 "aborted output transaction removes its temporary file");
    }

    std::ifstream preserved(target, std::ios::binary);
    const std::string preservedContents(
        (std::istreambuf_iterator<char>(preserved)),
        std::istreambuf_iterator<char>());
    t.expect(preservedContents == "original",
             "aborted output transaction preserves the destination");

    {
        DwgDxfOutputTransaction transaction(target.string(), std::ios::binary);
        t.expect(transaction.open(),
                 "identity-check transaction opens an exclusive temporary");
        transaction.stream() << "not published";
        const std::vector<std::filesystem::path> paths =
            transactionTemporaryPaths(target);
        t.expect(paths.size() == 1,
                 "identity-check transaction exposes one temporary path");
        const std::filesystem::path attacker =
            target.parent_path() / (target.filename().string() + ".libdxfrw-attacker");
        {
            std::ofstream replacement(attacker,
                                      std::ios::binary | std::ios::trunc);
            replacement << "substituted";
        }
        std::error_code substitutionError;
        std::filesystem::rename(attacker, paths.front(), substitutionError);
#if defined(_WIN32)
        if (substitutionError) {
            // The CRT stream does not grant FILE_SHARE_DELETE, so Windows
            // correctly blocks replacing an open transaction pathname.  The
            // identity-substitution attack is therefore unrepresentable while
            // the stream is open; verify the safe abort/cleanup contract and
            // leave the destination untouched.
            transaction.abort();
            std::filesystem::remove(attacker, ignored);
            t.expect(transactionTemporaryCount(target) == 0,
                     "blocked replacement aborts and removes the owned temporary");
        } else {
            t.expect(!transaction.commit(),
                     "identity-check replacement rejects commit");
            t.expect(std::filesystem::exists(paths.front()),
                     "failed identity check does not delete an unowned replacement");
            std::filesystem::remove(paths.front(), ignored);
        }
#else
        t.expect(!substitutionError,
                 "identity-check test substitutes the temporary pathname");
        t.expect(!transaction.commit(),
                 "temporary pathname substitution rejects commit");
        t.expect(std::filesystem::exists(paths.front()),
                 "failed identity check does not delete an unowned replacement");
        std::filesystem::remove(paths.front(), ignored);
#endif
    }

    std::ifstream preservedAfterSubstitution(target, std::ios::binary);
    const std::string substitutionContents(
        (std::istreambuf_iterator<char>(preservedAfterSubstitution)),
        std::istreambuf_iterator<char>());
    t.expect(substitutionContents == "original",
             "temporary pathname substitution preserves the destination");

#if !defined(_WIN32)
    {
        const std::filesystem::path target = transactionTestPath("symlink");
        const std::filesystem::path linked =
            transactionTestPath("symlink-target");
        std::filesystem::remove(target, ignored);
        std::filesystem::remove(linked, ignored);
        {
            std::ofstream seed(linked, std::ios::binary | std::ios::trunc);
            seed << "linked-original";
        }
        std::error_code linkError;
        std::filesystem::create_symlink(linked, target, linkError);
        if (!linkError) {
            DwgDxfOutputTransaction transaction(target.string(),
                                                std::ios::binary);
            t.expect(transaction.open(),
                     "symlink destination transaction opens");
            transaction.stream() << "symlink-replacement";
            t.expect(transaction.commit(),
                     "symlink destination transaction commits by replacement");
            std::ifstream linkedAfter(linked, std::ios::binary);
            const std::string linkedContents(
                (std::istreambuf_iterator<char>(linkedAfter)),
                std::istreambuf_iterator<char>());
            t.expect(linkedContents == "linked-original",
                     "symlink target remains unchanged by publication");
            t.expect(!std::filesystem::is_symlink(target),
                     "publication replaces the symlink itself");
        }
        t.expect(!linkError || !std::filesystem::exists(target),
                 "symlink policy leaves no stale destination after cleanup");
        std::filesystem::remove(target, ignored);
        std::filesystem::remove(linked, ignored);
    }

    {
        const std::filesystem::path target = transactionTestPath("hardlink");
        const std::filesystem::path linked =
            transactionTestPath("hardlink-target");
        std::filesystem::remove(target, ignored);
        std::filesystem::remove(linked, ignored);
        {
            std::ofstream seed(linked, std::ios::binary | std::ios::trunc);
            seed << "hardlink-original";
        }
        std::error_code linkError;
        std::filesystem::create_hard_link(linked, target, linkError);
        if (!linkError) {
            DwgDxfOutputTransaction transaction(target.string(),
                                                std::ios::binary);
            t.expect(transaction.open(),
                     "hardlink destination transaction opens");
            transaction.stream() << "hardlink-replacement";
            t.expect(transaction.commit(),
                     "hardlink destination transaction commits by replacement");
            std::ifstream linkedAfter(linked, std::ios::binary);
            const std::string linkedContents(
                (std::istreambuf_iterator<char>(linkedAfter)),
                std::istreambuf_iterator<char>());
            t.expect(linkedContents == "hardlink-original",
                     "hardlink target remains unchanged by publication");
        }
        t.expect(!linkError || !std::filesystem::exists(target),
                 "hardlink policy leaves no stale destination after cleanup");
        std::filesystem::remove(target, ignored);
        std::filesystem::remove(linked, ignored);
    }

    {
        const std::filesystem::path root = transactionTestPath("parent-race");
        const std::filesystem::path original = root / "original";
        const std::filesystem::path moved = root / "moved";
        const std::filesystem::path target = original / "out.dwg";
        std::filesystem::create_directory(root, ignored);
        std::filesystem::create_directory(original, ignored);
        {
            DwgDxfOutputTransaction transaction(target.string(),
                                                std::ios::binary);
            t.expect(transaction.open(),
                     "parent-race transaction opens before directory swap");
            transaction.stream() << "parent-race-output";
            std::error_code moveError;
            std::filesystem::rename(original, moved, moveError);
            t.expect(!moveError,
                     "parent-race test moves the original parent directory");
            if (!moveError) {
                std::filesystem::create_directory(original, ignored);
                t.expect(!transaction.commit(),
                         "parent-path replacement fails closed before publish");
                t.expect(!std::filesystem::exists(original / "out.dwg")
                             && !std::filesystem::exists(moved / "out.dwg"),
                         "parent-path replacement publishes no redirected output");
                t.expect(transactionTemporaryCount(moved / "out.dwg") == 0,
                         "parent-path replacement cleans the owned temporary in the moved directory");
            }
        }
        std::filesystem::remove(original / "out.dwg", ignored);
        std::filesystem::remove(moved / "out.dwg", ignored);
        std::filesystem::remove(original, ignored);
        std::filesystem::remove(moved, ignored);
        std::filesystem::remove(root, ignored);
    }
#endif

    const std::filesystem::path missing =
        target.parent_path() / "libdxfrw-s248-missing-parent" / "out.dwg";
    DwgDxfOutputTransaction failed(missing.string(), std::ios::binary);
    t.expect(!failed.open(),
             "output transaction rejects a missing parent directory");
    t.expect(!std::filesystem::exists(missing),
             "failed output transaction does not create a destination");

    std::filesystem::remove(target, ignored);
}


#if !defined(_WIN32)

// The mode the temporary carries is what the caller ends up with, because the
// temporary is renamed over the target. So it is part of the library's
// observable behaviour: a new file has to look like one an ordinary open()
// would have produced, and overwriting an existing file must not change its
// permissions.
//
// What this catches: the modes themselves. Against the implementation that
// preceded it -- mkstemp, which always creates 0600, with nothing restoring
// the mode -- every assertion below except the 0600 case fails.
//
// What it does not catch, and cannot: the reason the implementation creates
// the temporary with open(..., 0666) rather than reading the umask. There is
// no portable way to read the umask without setting it, and umask(0) followed
// by umask(mask) is a process-global write -- during that window every other
// thread in the host process creates files with no umask applied. That window
// is invisible to a single-threaded test and racing it would be flaky. The
// guard against it is structural: the implementation calls umask() nowhere.
// The assertion below only catches a variant that sets it and fails to
// restore it.
void testOutputTransactionPreservesFileMode(TestContext& t) {
    // Per-process directory: ctest may run this binary concurrently with another
    // copy of itself, and both would otherwise iterate each other's temporaries.
    const std::filesystem::path directory =
        std::filesystem::temp_directory_path()
        / ("libdxfrw-output-transaction-mode-" + std::to_string(::getpid()));
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
    std::filesystem::create_directories(directory, ignored);

    const auto writeThrough = [](const std::filesystem::path& target) {
        DwgDxfOutputTransaction transaction(target.string(),
                                            std::ios::out | std::ios::binary);
        if (!transaction.open())
            return false;
        transaction.stream() << "0\nEOF\n";
        return transaction.commit();
    };
    const auto modeOf = [](const std::filesystem::path& path) {
        struct stat status {};
        if (::stat(path.c_str(), &status) != 0)
            return -1;
        return static_cast<int>(status.st_mode & 07777);
    };

    // A new file gets the mode an ordinary open(..., 0666) would produce: the
    // kernel subtracts the umask. Before this was fixed the temporary's own
    // 0600 survived the rename and every written file was owner-only.
    const mode_t previousMask = ::umask(022);
    const std::filesystem::path fresh = directory / "fresh.dxf";
    t.expect(writeThrough(fresh), "a new file publishes");
    t.expect(modeOf(fresh) == 0644,
             "a new file under umask 022 is 0644, not the temporary's 0600");

    // The umask must be exactly what it was; see the note above for the part
    // this cannot reach.
    t.expect(::umask(previousMask) == 022,
             "the write left the process umask alone");

    // Overwriting keeps whatever mode the target already had, in both
    // directions -- a write must neither tighten nor loosen someone's file.
    struct ModeCase {
        const char* name;
        mode_t mode;
        const char* published;
        const char* preserved;
    };
    const ModeCase cases[] = {
        {"overwrite-0644.dxf", 0644, "overwriting a 0644 file publishes",
         "overwriting a 0644 file keeps it 0644"},
        {"overwrite-0600.dxf", 0600, "overwriting a 0600 file publishes",
         "overwriting a 0600 file keeps it 0600"},
        {"overwrite-0640.dxf", 0640, "overwriting a 0640 file publishes",
         "overwriting a 0640 file keeps it 0640"},
        {"overwrite-0664.dxf", 0664, "overwriting a 0664 file publishes",
         "overwriting a 0664 file keeps it 0664"},
        // Read-only targets: reference drawings and files restored from backup
        // are routinely 0444, and the previous implementation wrote them
        // happily.  Creating the temporary with the target's exact mode does
        // not -- the stream reopens it by name -- so these two pin the owner
        // write bit that makes it work.
        {"overwrite-0444.dxf", 0444, "overwriting a read-only file publishes",
         "overwriting a 0444 file keeps it 0444"},
        {"overwrite-0400.dxf", 0400, "overwriting an owner-read-only file publishes",
         "overwriting a 0400 file keeps it 0400"},
    };

    for (const ModeCase& testCase : cases) {
        const std::filesystem::path target = directory / testCase.name;
        {
            std::ofstream seed(target);
            seed << "0\nEOF\n";
        }
        if (::chmod(target.c_str(), testCase.mode) != 0) {
            t.expect(false, "seeding an existing target with a known mode");
            continue;
        }
        t.expect(writeThrough(target), testCase.published);
        t.expect(modeOf(target) == static_cast<int>(testCase.mode),
                 testCase.preserved);
    }

    // The temporary lives in the target's own directory for the whole duration
    // of the write, so it must never be more permissive than what it is about
    // to replace. Creating it at 0666 & ~umask would publish a private file's
    // new contents to every reader on the host until the rename lands.
    {
        // Its own directory, so "every file that is not the target" identifies
        // the temporary without the test having to know how it is named.
        const std::filesystem::path guardedDirectory = directory / "exposure";
        std::filesystem::create_directories(guardedDirectory, ignored);
        const std::filesystem::path guarded = guardedDirectory / "guarded.dxf";
        {
            std::ofstream seed(guarded);
            seed << "0\nEOF\n";
        }
        if (::chmod(guarded.c_str(), 0600) != 0) {
            t.expect(false, "seeding the 0600 target for the exposure check");
        } else {
            DwgDxfOutputTransaction transaction(
                guarded.string(), std::ios::out | std::ios::binary);
            t.expect(transaction.open(), "the overwrite transaction opens");
            transaction.stream() << "0\nEOF\n";

            // Mid-write: every file in the directory other than the target is
            // this transaction's temporary.
            int exposed = 0;
            int temporaries = 0;
            std::error_code walk;
            // The error_code overload: the throwing one would abort the whole
            // executable instead of failing this assertion.
            for (std::filesystem::directory_iterator entry(guardedDirectory, walk),
                     last;
                 !walk && entry != last; entry.increment(walk)) {
                if (entry->path() == guarded)
                    continue;
                struct stat status {};
                if (::stat(entry->path().c_str(), &status) == 0) {
                    ++temporaries;
                    exposed |= static_cast<int>(status.st_mode) & 0077;
                }
            }
            // Without this the assertion below passes for free if the walk
            // found nothing at all.
            t.expect(temporaries == 1,
                     "exactly one in-progress temporary was observed");
            t.expect(exposed == 0,
                     "the in-progress temporary is not group- or world-readable "
                     "while it replaces a 0600 file");
            t.expect(transaction.commit(), "the overwrite publishes");
            t.expect(modeOf(guarded) == 0600, "and the target is still 0600");
        }
    }

    // A symlinked target must not lend its destination's permissions to the
    // published file. renameat replaces the link itself, so the mode of the
    // file it pointed at is not the mode of anything this write touches --
    // and inheriting it would let anyone who can write the output directory
    // choose the permissions of somebody else's saved drawing.
    {
        const std::filesystem::path linkDirectory = directory / "symlink";
        std::filesystem::create_directories(linkDirectory, ignored);
        const std::filesystem::path bait = linkDirectory / "bait";
        {
            std::ofstream seed(bait);
            seed << "0\nEOF\n";
        }
        const std::filesystem::path link = linkDirectory / "link.dxf";
        std::error_code linked;
        std::filesystem::create_symlink(bait, link, linked);
        if (linked || ::chmod(bait.c_str(), 0777) != 0) {
            // Symlinks are not always available; skip rather than fail.
        } else {
            t.expect(writeThrough(link), "writing through a symlinked name publishes");
            struct stat published {};
            t.expect(::lstat(link.c_str(), &published) == 0
                         && S_ISREG(published.st_mode),
                     "the symlink is replaced by a regular file");
            t.expect((published.st_mode & 07777) == 0644,
                     "the published file takes the umask's mode, not the "
                     "symlink destination's 0777");
        }
    }

    // The content has to arrive too -- a permissions test that passes on an
    // empty file is worth nothing.
    std::ifstream written(fresh);
    std::string firstLine;
    std::getline(written, firstLine);
    t.expect(firstLine == "0", "the published file holds what was written");

    std::filesystem::remove_all(directory, ignored);
}

#endif // !_WIN32

} // namespace

int main() {
    TestContext context;
    testModularVectors(context);
    testBitAndRawVectors(context);
    testHandleOccurrences(context);
    testAllocatorVectors(context);
    testWriterReservationVectors(context);
    testFrameReceiptAndRollback(context);
    testWriteRejectionDoesNotTouchDestination(context);
    testOutputTransactionPublicationAndRollback(context);
#if !defined(_WIN32)
    // The _WIN32 branch creates its temporary in the target's own directory and
    // inherits the directory ACL; there is no umask and no mode to carry.
    testOutputTransactionPreservesFileMode(context);
#endif
    if (context.failures != 0) {
        std::cerr << context.failures << " writer primitive assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer primitives: PASS\n";
    return 0;
}
