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
        t.expect(!substitutionError,
                 "identity-check test substitutes the temporary pathname");
        t.expect(!transaction.commit(),
                 "temporary pathname substitution rejects commit");
        t.expect(std::filesystem::exists(paths.front()),
                 "failed identity check does not delete an unowned replacement");
        std::filesystem::remove(paths.front(), ignored);
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
    if (context.failures != 0) {
        std::cerr << context.failures << " writer primitive assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer primitives: PASS\n";
    return 0;
}
