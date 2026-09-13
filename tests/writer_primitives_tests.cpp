#include <cstdint>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "handle_allocator.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
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
    const std::string path = "/private/tmp/libdxfrw-writer-failure-gate.dwg";
    {
        std::ofstream seed(path, std::ios::binary | std::ios::trunc);
        seed << "sentinel";
    }

    dwgRW nullInterface(path.c_str());
    t.expect(!nullInterface.write(nullptr, DRW::AC1015, false)
                 && nullInterface.getError() == DRW::BAD_UNKNOWN,
             "supported-version write rejects null interface");
    std::ifstream afterNull(path, std::ios::binary);
    const std::string nullContents((std::istreambuf_iterator<char>(afterNull)),
                                   std::istreambuf_iterator<char>());
    t.expect(nullContents == "sentinel",
             "null-interface rejection leaves destination untouched");

    dwgRW unsupported(path.c_str());
    t.expect(!unsupported.write(nullptr, DRW::UNKNOWNV, false)
                 && unsupported.getError() == DRW::BAD_VERSION,
             "unsupported-version write rejects before opening destination");
    std::ifstream afterVersion(path, std::ios::binary);
    const std::string versionContents(
        (std::istreambuf_iterator<char>(afterVersion)),
        std::istreambuf_iterator<char>());
    t.expect(versionContents == "sentinel",
             "unsupported-version rejection leaves destination untouched");

    std::remove(path.c_str());
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
    if (context.failures != 0) {
        std::cerr << context.failures << " writer primitive assertion(s) failed\n";
        return 1;
    }
    std::cout << "Writer primitives: PASS\n";
    return 0;
}
