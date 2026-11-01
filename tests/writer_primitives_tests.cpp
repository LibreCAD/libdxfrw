#include <cstdint>
#include <cstdio>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "handle_allocator.h"
#include "intern/dwgbuffer.h"
#include "intern/dwgbufferw.h"
#include "intern/drw_textcodec.h"
#include "intern/dwg_dxf_output_transaction.h"
#include "intern/dxfwriter.h"
#include "intern/dwgsafety.h"
#include "intern/dwgwriter15.h"
#include "intern/dwgwriter21.h"
#include "intern/dwgwriter24.h"
#include "intern/dwgwriter27.h"
#include "intern/dwgwriter32.h"
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

class DictionaryParseProbe final : public DRW_Dictionary {
public:
    using DRW_Dictionary::parseDwg;
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

    std::uint8_t invalidBitDouble[] {0xC0};
    dwgBuffer invalidReader(invalidBitDouble, sizeof(invalidBitDouble));
    t.expect(invalidReader.getBitDouble() == 0.0
                 && !invalidReader.isGood(),
             "BD rejects reserved two-bit code 11");
}

void testWideBufferBitCountArithmetic(TestContext& t) {
    std::uint64_t bits = 0;
    dwgBufferW writer;
    writer.putRawChar8(0xA5u);
    t.expect(writer.bitCount() == 8u,
             "writer bit count preserves ordinary byte-aligned output");
    writer.putBit(1u);
    t.expect(writer.bitCount() == 9u,
             "writer bit count includes a partial trailing byte");

    constexpr std::uint64_t halfGiBBytes = 1ULL << 29;
    t.expect(dwgSafety::bufferBitCount(halfGiBBytes, 0u, bits)
                 && bits == (1ULL << 32),
             "buffer bit count remains wide beyond the 32-bit bit boundary");
    t.expect(dwgSafety::bufferBitCount(halfGiBBytes, 7u, bits)
                 && bits == std::numeric_limits<std::uint32_t>::max(),
             "partial final byte reaches the largest 32-bit bit count");
    t.expect(!dwgSafety::bufferBitCount(0u, 1u, bits)
                 && !dwgSafety::bufferBitCount(1u, 8u, bits),
             "buffer bit count rejects impossible trailing-bit cursors");
    t.expect(!dwgSafety::bufferBitCount(
                 std::numeric_limits<std::uint64_t>::max() / 8u + 1u,
                 0u, bits),
             "buffer bit count rejects 64-bit multiplication overflow");
}

void testDwgTextLengthBoundaries(TestContext& t) {
    constexpr std::size_t maxPayloadUnits = 0xFFFEu;
    constexpr std::uint8_t sentinel = 0xA5u;
    const std::string byteText(maxPayloadUnits, 'A');

    const auto checkByteText = [&](void (dwgBufferW::*writeText)(
                                      const std::string&),
                                   const char* successLabel,
                                   const char* rejectLabel) {
        dwgBufferW writer;
        (writer.*writeText)(byteText);
        writer.putRawChar8(sentinel);
        t.expect(writer.isGood(), successLabel);

        dwgBuffer reader(writer.data().data(), writer.data().size());
        const std::uint16_t encodedLength = reader.getBitShort();
        t.expect(encodedLength == 0xFFFFu,
                 "CP8/ENC length includes the terminator without wrapping");
        std::vector<std::uint8_t> encoded(encodedLength);
        const bool payloadRead = encodedLength != 0
            && reader.getBytes(encoded.data(), encoded.size());
        t.expect(payloadRead && encoded.front() == 'A'
                     && encoded[encoded.size() - 2u] == 'A'
                     && encoded.back() == 0,
                 "CP8/ENC maximum payload and terminator are byte-exact");
        t.expect(reader.getRawChar8() == sentinel && reader.isGood(),
                 "CP8/ENC following sentinel remains aligned at maximum");

        dwgBufferW rejected;
        (rejected.*writeText)(std::string(maxPayloadUnits + 1u, 'A'));
        t.expect(!rejected.isGood() && rejected.data().empty(), rejectLabel);
    };

    checkByteText(&dwgBufferW::putCP8Text,
                  "CP8 accepts the largest payload plus a terminator",
                  "CP8 rejects payload above the unsigned BS limit atomically");
    checkByteText(&dwgBufferW::putENCText,
                  "ENC accepts the largest payload plus a terminator",
                  "ENC rejects payload above the unsigned BS limit atomically");

    // The payload has 65,534 UTF-16 units: 65,532 BMP units and one astral
    // scalar encoded as a surrogate pair. The writer then appends its NUL.
    const std::string astral = "\xF0\x9F\x98\x80";
    const std::string unicodeAtLimit(0xFFFCu, 'A');
    const std::string unicodeText = unicodeAtLimit + astral;
    dwgBufferW unicodeWriter;
    unicodeWriter.putUCSText(unicodeText);
    unicodeWriter.putRawChar8(sentinel);
    t.expect(unicodeWriter.isGood(),
             "TU accepts 65,534 payload code units plus a terminator");

    dwgBuffer unicodeReader(unicodeWriter.data().data(),
                            unicodeWriter.data().size());
    const std::uint16_t encodedUnits = unicodeReader.getBitShort();
    t.expect(encodedUnits == 0xFFFFu,
             "TU length counts UTF-16 units and includes its terminator");
    std::vector<std::uint8_t> unicodeBytes(
        static_cast<std::size_t>(encodedUnits) * 2u);
    const bool unicodePayloadRead = unicodeReader.getBytes(
        unicodeBytes.data(), unicodeBytes.size());
    t.expect(unicodePayloadRead && unicodeBytes[0] == 'A'
                 && unicodeBytes[unicodeBytes.size() - 6u] == 0x3Du
                 && unicodeBytes[unicodeBytes.size() - 5u] == 0xD8u
                 && unicodeBytes[unicodeBytes.size() - 4u] == 0x00u
                 && unicodeBytes[unicodeBytes.size() - 3u] == 0xDEu
                 && unicodeBytes[unicodeBytes.size() - 2u] == 0u
                 && unicodeBytes.back() == 0u,
             "TU maximum payload preserves astral surrogate pair and NUL");
    t.expect(unicodeReader.getRawChar8() == sentinel && unicodeReader.isGood(),
             "TU following sentinel remains aligned at maximum");

    // One astral scalar contributes two units: 65,533 BMP units plus that
    // scalar is 65,535 payload units and must be rejected before the BS.
    const std::string unicodeOverLimit = std::string(0xFFFDu, 'A') + astral;
    dwgBufferW rejectedUnicode;
    rejectedUnicode.putUCSText(unicodeOverLimit);
    t.expect(!rejectedUnicode.isGood() && rejectedUnicode.data().empty(),
             "TU rejects 65,535 payload code units without a wrapped length");
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

void testLeaderAnnotationReferenceFailsClosed(TestContext& t) {
    DRW_Header header;
    WriterProbe writer(nullptr, &header);
    DRW_Leader leader;
    leader.handle = 0xD101u;
    leader.annotHandle = 0xD102u;

    const bool encoded = writer.encodeEntity(&leader);
    t.expect(!encoded,
             "DWG LEADER rejects an annotation hard pointer it cannot emit");
    t.expect(writer.buffer().empty(),
             "rejected DWG LEADER leaves no partial entity frame");
    t.expect(leader.handle == 0xD101u,
             "rejected DWG LEADER preserves its caller handle");
    leader.annotHandle = 0u;
    t.expect(writer.encodeEntity(&leader) && !writer.buffer().empty(),
             "same DWG writer succeeds after the rejected LEADER is corrected");
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

void testModernDictionaryStreamBoundary(TestContext& t) {
    // DICTIONARY ends its scalar body with the hard-owner RC, followed by
    // its counted TV array. Check the actual frame boundary independently
    // of DRW_Dictionary::parseDwg's detached-string seek.
    const std::string unicodeKey("\xE5\xAD\x97_\xF0\x9F\x98\x80");
    for (const auto version :
         {DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        DRW_TextCodec codec;
        codec.setVersion(version, false);
        for (const bool extensionMissing : {false, true}) {
            for (const bool hasReactor : {false, true}) {
                for (const std::string& key :
                     {std::string("FAST_GROUP_KEY"), unicodeKey,
                      std::string(2200u, 'A')}) {
                    DRW_Header header;
                    std::unique_ptr<dwgWriter15> writer;
                    switch (version) {
                    case DRW::AC1021:
                        writer =
                            std::make_unique<dwgWriter21>(nullptr, &header);
                        break;
                    case DRW::AC1024:
                        writer =
                            std::make_unique<dwgWriter24>(nullptr, &header);
                        break;
                    case DRW::AC1027:
                        writer =
                            std::make_unique<dwgWriter27>(nullptr, &header);
                        break;
                    default:
                        writer =
                            std::make_unique<dwgWriter32>(nullptr, &header);
                        break;
                    }
                    DRW_Dictionary dictionary;
                    dictionary.handle = 0xD100u;
                    dictionary.parentHandle = 0xCu;
                    dictionary.setDwgCommonObjectState(
                        hasReactor ? 1 : 0, extensionMissing ? 1 : 0, false);
                    if (hasReactor) {
                        dictionary.reactorHandles.push_back(0xCu);
                    }
                    dictionary.m_entries.push_back({key, 0xD101u});
                    const bool hasSecondEntry = key == unicodeKey;
                    if (hasSecondEntry)
                        dictionary.m_entries.push_back({"NEXT_KEY", 0xD102u});
                    const bool written = writer->writeDictionary(dictionary);
                    t.expect(written,
                             "modern dictionary boundary frame writes");
                    if (!written)
                        continue;
                    const auto& bytes = writer->buffer();
                    dwgBuffer framing(bytes.data(), bytes.size());
                    const auto frameBytes = framing.getModularShort();
                    const auto handleBits =
                        version > DRW::AC1021 ? framing.getUModularChar() : 0u;
                    const auto prefixBytes = framing.getPosition();
                    if (!framing.isGood() || frameBytes <= 0 ||
                        prefixBytes + static_cast<std::uint64_t>(frameBytes) +
                                2u !=
                            bytes.size()) {
                        t.expect(
                            false,
                            "R2007 dictionary has exact MS/body/CRC framing");
                        continue;
                    }
                    dwgBuffer body(bytes.data() + prefixBytes,
                                   static_cast<std::size_t>(frameBytes),
                                   &codec);
                    t.expect(body.getObjType(version) == 42u,
                             "modern dictionary has the fixed object type");
                    const auto endBit =
                        version == DRW::AC1021
                            ? body.getRawLong32()
                            : static_cast<std::uint64_t>(frameBytes) * 8u -
                                  handleBits;
                    t.expect(
                        body.getHandle().ref == dictionary.handle &&
                            body.getBitShort() == 0,
                        "R2007 dictionary handle and empty EED are literal");
                    t.expect(body.getBitLong() == (hasReactor ? 1 : 0) &&
                                 body.getBit() == (extensionMissing ? 1 : 0),
                             "modern dictionary common flags are exact");
                    if (version >= DRW::AC1027)
                        t.expect(body.getBit() == 0,
                                 "modern dictionary has no data-storage binary "
                                 "data");
                    t.expect(body.getBitLong() == (hasSecondEntry ? 2 : 1) &&
                                 body.getBitShort() == dictionary.cloning &&
                                 body.getRawChar8() == dictionary.hardOwner,
                             "R2007 dictionary scalar prefix is exact");
                    const auto scalarEndBit =
                        body.getPosition() * 8u + body.getBitPos();
                    std::uint64_t stringStartBit = 0, stringEndBit = 0;
                    t.expect(body.getR2007StringStreamBounds(
                                 endBit, stringStartBit, stringEndBit) &&
                                 stringStartBit == scalarEndBit,
                             "modern dictionary strings immediately follow "
                             "scalar data");
                    dwgBuffer contiguous = body.forkIndependent();
                    t.expect(
                        contiguous.getVariableText(version, false) == key &&
                            contiguous.isGood(),
                        "modern dictionary key decodes at the scalar boundary");
                    if (hasSecondEntry)
                        t.expect(contiguous.getVariableText(version, false)
                                         == "NEXT_KEY" && contiguous.isGood(),
                                 "modern dictionary counted names remain contiguous");
                    t.expect(endBit % 8u == 0u && stringEndBit < endBit &&
                                 endBit <
                                     static_cast<std::uint64_t>(frameBytes) *
                                         8u,
                             "R2007 footer ends immediately before the handle "
                             "stream");
                    body.setPosition(endBit / 8u);
                    body.setBitPos(0);
                    t.expect(
                        body.getOffsetHandle(dictionary.handle).ref ==
                            dictionary.parentHandle,
                        "modern dictionary owner remains in the handle stream");
                    if (hasReactor)
                        t.expect(body.getHandle().ref == 0xCu,
                                 "modern dictionary reactor remains in the "
                                 "handle stream");
                    if (!extensionMissing)
                        t.expect(body.getHandle().ref == 0,
                                 "modern dictionary null extension slot stays "
                                 "present");
                    const auto entry = body.getHandle();
                    t.expect(entry.code == 2 && entry.ref == 0xD101u &&
                                 body.isGood(),
                             "modern dictionary item has its exact ownership "
                             "handle");
                    if (hasSecondEntry) {
                        const auto nextEntry = body.getHandle();
                        t.expect(nextEntry.code == 2 && nextEntry.ref == 0xD102u
                                     && body.isGood(),
                                 "modern dictionary second item stays in order");
                    }
                    DRW::DwgObjectFrameReceipt receipt;
                    t.expect(writer->getLastDwgObjectFrame(receipt) &&
                                 receipt.dataBitSize == endBit,
                             "modern dictionary frame receipt follows the "
                             "actual stream "
                             "boundary");
                    dwgBufferW crc;
                    crc.putBytes(bytes.data(), bytes.size() - 2u);
                    const auto expectedCrc = crc.crc16(0xC0C1, 0, crc.size());
                    t.expect(bytes[bytes.size() - 2u] ==
                                     static_cast<std::uint8_t>(expectedCrc) &&
                                 bytes.back() == static_cast<std::uint8_t>(
                                                     expectedCrc >> 8u),
                             "modern dictionary CRC covers size prefixes and "
                             "shifted streams");
                }
            }
        }
    }

    struct DictionaryFrame {
        std::vector<std::uint8_t> bytes;
        std::size_t bodyOffset = 0;
        std::size_t bodyBytes = 0;
        std::uint32_t handleBits = 0;
        std::uint64_t stringStartBit = 0;
    };
    const auto makeDictionaryFrame = [](std::uint32_t childHandle) {
        DictionaryFrame result;
        DRW_Header header;
        dwgWriter27 writer(nullptr, &header);
        DRW_Dictionary dictionary;
        dictionary.handle = 0xD100u;
        dictionary.parentHandle = 0xCu;
        dictionary.setDwgCommonObjectState(0, 1, false);
        dictionary.m_entries.push_back({"A", childHandle});
        if (!writer.writeDictionary(dictionary))
            return result;

        result.bytes = writer.buffer();
        dwgBuffer framing(result.bytes.data(), result.bytes.size());
        const std::int32_t frameBytes = framing.getModularShort();
        result.handleBits = framing.getUModularChar();
        result.bodyOffset = static_cast<std::size_t>(framing.getPosition());
        if (!framing.isGood() || frameBytes <= 0
            || result.bodyOffset + static_cast<std::size_t>(frameBytes) + 2u
                != result.bytes.size()) {
            result.bytes.clear();
            return result;
        }
        result.bodyBytes = static_cast<std::size_t>(frameBytes);
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1027, false);
        dwgBuffer body(result.bytes.data() + result.bodyOffset,
                       result.bodyBytes, &codec);
        const bool scalarPrefix = body.getObjType(DRW::AC1027) == 42u
            && body.getHandle().ref == dictionary.handle
            && body.getBitShort() == 0
            && body.getBitLong() == 0
            && body.getBit() == 1
            && body.getBit() == 0
            && body.getBitLong() == 1;
        body.getBitShort(); // cloning
        body.getRawChar8(); // hard-owner flag
        result.stringStartBit = body.getPosition() * 8u + body.getBitPos();
        std::uint64_t stringStart = 0;
        std::uint64_t stringEnd = 0;
        const std::uint64_t endBit = result.bodyBytes * 8u
            - result.handleBits;
        if (!scalarPrefix || !body.isGood()
            || !body.getR2007StringStreamBounds(
                endBit, stringStart, stringEnd)
            || stringStart != result.stringStartBit
            || stringEnd < stringStart) {
            result.bytes.clear();
        }
        return result;
    };
    const auto patchFirstNameToEmpty = [](DictionaryFrame& frame) {
        if (frame.bytes.empty())
            return false;
        // A R2007+ TU length of zero is the two-bit value 10. Patch only that
        // length code so the generated frame keeps its original handle stream.
        for (unsigned bitIndex = 0; bitIndex < 2u; ++bitIndex) {
            const std::uint64_t bit = frame.bodyOffset * 8u
                + frame.stringStartBit + bitIndex;
            const std::uint8_t mask = static_cast<std::uint8_t>(
                1u << (7u - static_cast<unsigned>(bit % 8u)));
            if (bitIndex == 0u)
                frame.bytes[static_cast<std::size_t>(bit / 8u)] |= mask;
            else
                frame.bytes[static_cast<std::size_t>(bit / 8u)] &=
                    static_cast<std::uint8_t>(~mask);
        }
        return true;
    };
    const auto parseDictionaryFrame = [](
        const DictionaryFrame& frame, DictionaryParseProbe& dictionary) {
        DRW_TextCodec codec;
        codec.setVersion(DRW::AC1027, false);
        dwgBuffer input(frame.bytes.data() + frame.bodyOffset,
                        frame.bodyBytes, &codec);
        return dictionary.parseDwg(DRW::AC1027, &input, frame.handleBits);
    };

    DictionaryFrame emptyNullEntry = makeDictionaryFrame(0u);
    DictionaryFrame emptyNonNullEntry = makeDictionaryFrame(0xD101u);
    const bool prepared = patchFirstNameToEmpty(emptyNullEntry)
        && patchFirstNameToEmpty(emptyNonNullEntry);
    DictionaryParseProbe parsedDictionary;
    const bool parsedEmptyNonNull = prepared
        && parseDictionaryFrame(emptyNonNullEntry, parsedDictionary);
    const bool rejectedEmptyNonNull = prepared && !parsedEmptyNonNull
        && !parsedDictionary.hasCompleteDwgEntries()
        && parsedDictionary.m_entries.empty();
    const bool acceptedEmptyNull = prepared
        && parseDictionaryFrame(emptyNullEntry, parsedDictionary)
        && parsedDictionary.hasCompleteDwgEntries()
        && parsedDictionary.m_entries.size() == 1u
        && parsedDictionary.m_entries.front().m_name.empty()
        && parsedDictionary.m_entries.front().m_handle == 0u
        && !parsedDictionary.hasCompleteDxfEntries();
    t.expect(rejectedEmptyNonNull,
             "DICTIONARY rejects an empty name paired with a non-null handle and clears failed state");
    t.expect(acceptedEmptyNull,
             "DICTIONARY retains an observed empty-name/null-handle DWG pair while DXF output remains incomplete");
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
        t.expect(transaction.verifyRange(0, "published"),
                 "output transaction reads back an exact staged range");
        t.expect(!transaction.verifyRange(0, "publishEd"),
                 "output transaction rejects a mismatched staged range");
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
        const std::filesystem::path appeared =
            transactionTestPath("concurrent-create");
        std::filesystem::remove(appeared, ignored);
        DwgDxfOutputTransaction transaction(appeared.string(),
                                            std::ios::binary);
        t.expect(transaction.open(),
                 "missing-target transaction opens an exclusive temporary");
        transaction.stream() << "transaction-content";
        {
            std::ofstream concurrent(appeared,
                                     std::ios::binary | std::ios::trunc);
            concurrent << "concurrent-content";
        }
        t.expect(!transaction.commit(),
                 "publication rejects a destination created during writing");
        std::ifstream concurrent(appeared, std::ios::binary);
        const std::string concurrentContents(
            (std::istreambuf_iterator<char>(concurrent)),
            std::istreambuf_iterator<char>());
        t.expect(concurrentContents == "concurrent-content",
                 "concurrent destination creation is preserved");
        t.expect(transactionTemporaryCount(appeared) == 0,
                 "rejected concurrent creation cleans its temporary");
        std::filesystem::remove(appeared, ignored);
    }

    {
        const std::filesystem::path replaced =
            transactionTestPath("concurrent-replace");
        const std::filesystem::path concurrentPath =
            transactionTestPath("concurrent-replacement-content");
        std::filesystem::remove(replaced, ignored);
        std::filesystem::remove(concurrentPath, ignored);
        {
            std::ofstream seed(replaced, std::ios::binary | std::ios::trunc);
            seed << "original-content";
        }
        DwgDxfOutputTransaction transaction(replaced.string(),
                                            std::ios::binary);
        t.expect(transaction.open(),
                 "existing-target transaction opens an exclusive temporary");
        transaction.stream() << "transaction-content";
        {
            std::ofstream concurrent(concurrentPath,
                                     std::ios::binary | std::ios::trunc);
            concurrent << "concurrent-replacement";
        }
        std::filesystem::remove(replaced, ignored);
        std::filesystem::rename(concurrentPath, replaced, ignored);
        t.expect(!ignored,
                 "test installs the concurrent destination replacement");
        t.expect(!transaction.commit(),
                 "publication rejects a replaced destination");
        std::ifstream concurrent(replaced, std::ios::binary);
        const std::string concurrentContents(
            (std::istreambuf_iterator<char>(concurrent)),
            std::istreambuf_iterator<char>());
        t.expect(concurrentContents == "concurrent-replacement",
                 "concurrent destination replacement is preserved");
        t.expect(transactionTemporaryCount(replaced) == 0,
                 "rejected concurrent replacement cleans its temporary");
        std::filesystem::remove(replaced, ignored);
    }

    {
        const std::filesystem::path modified =
            transactionTestPath("concurrent-modify");
        std::filesystem::remove(modified, ignored);
        {
            std::ofstream seed(modified, std::ios::binary | std::ios::trunc);
            seed << "original";
        }
        DwgDxfOutputTransaction transaction(modified.string(),
                                            std::ios::binary);
        t.expect(transaction.open(),
                 "modified-target transaction opens an exclusive temporary");
        transaction.stream() << "transaction-content";
        {
            std::ofstream concurrent(modified,
                                     std::ios::binary | std::ios::trunc);
            concurrent << "concurrent-in-place-update";
        }
        t.expect(!transaction.commit(),
                 "publication rejects an in-place destination modification");
        std::ifstream concurrent(modified, std::ios::binary);
        const std::string concurrentContents(
            (std::istreambuf_iterator<char>(concurrent)),
            std::istreambuf_iterator<char>());
        t.expect(concurrentContents == "concurrent-in-place-update",
                 "concurrent in-place destination update is preserved");
        t.expect(transactionTemporaryCount(modified) == 0,
                 "rejected in-place modification cleans its temporary");
        std::filesystem::remove(modified, ignored);
    }

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

void testStagedRangeAuthenticationRollbackAndRetry(TestContext& t) {
    const std::filesystem::path target = transactionTestPath("staged-range");
    std::error_code ignored;
    std::filesystem::remove(target, ignored);
    {
        std::ofstream seed(target, std::ios::binary | std::ios::trunc);
        seed << "original";
    }

    bool rejected = false;
    {
        DwgDxfOutputTransaction transaction(target.string(), std::ios::binary);
        t.expect(transaction.open(), "mismatched staged range opens transaction");
        if (transaction.isOpen()) {
            dxfWriterAscii writer(&transaction.stream());
            DxfWriterRecordScope section(writer);
            t.expect(writer.writeString(0, "SECTION"),
                     "staged section body materializes for mismatch control");
            rejected = !section.commit(
                [](std::string_view bytes) { return !bytes.empty(); },
                [&transaction](std::streamoff offset, std::string_view bytes) {
                    std::string mismatch(bytes);
                    mismatch.front() ^= 1;
                    return transaction.verifyRange(offset, mismatch);
                });
        }
        t.expect(rejected,
                 "post-append byte mismatch refuses staged section commit");
        transaction.abort();
    }
    {
        std::ifstream preserved(target, std::ios::binary);
        const std::string value((std::istreambuf_iterator<char>(preserved)), {});
        t.expect(value == "original",
                 "staged section mismatch rolls back the published destination");
    }

    bool retried = false;
    {
        DwgDxfOutputTransaction transaction(target.string(), std::ios::binary);
        t.expect(transaction.open(), "corrected staged range opens retry transaction");
        if (transaction.isOpen()) {
            dxfWriterAscii writer(&transaction.stream());
            DxfWriterRecordScope section(writer);
            t.expect(writer.writeString(0, "SECTION"),
                     "retry section body materializes");
            retried = section.commit(
                [](std::string_view bytes) { return !bytes.empty(); },
                [&transaction](std::streamoff offset, std::string_view bytes) {
                    return transaction.verifyRange(offset, bytes);
                });
            t.expect(retried,
                     "corrected exact staged bytes pass append authentication");
            t.expect(retried && transaction.commit(),
                     "authenticated retry publishes atomically");
        }
    }
    {
        std::ifstream published(target, std::ios::binary);
        const std::string value((std::istreambuf_iterator<char>(published)), {});
        t.expect(retried && value != "original" && !value.empty(),
                 "authenticated retry replaces the sentinel destination");
    }
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
    testWideBufferBitCountArithmetic(context);
    testDwgTextLengthBoundaries(context);
    testHandleOccurrences(context);
    testAllocatorVectors(context);
    testWriterReservationVectors(context);
    testLeaderAnnotationReferenceFailsClosed(context);
    testFrameReceiptAndRollback(context);
    testModernDictionaryStreamBoundary(context);
    testWriteRejectionDoesNotTouchDestination(context);
    testOutputTransactionPublicationAndRollback(context);
    testStagedRangeAuthenticationRollbackAndRetry(context);
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
