#include "intern/dwgbuffer.h"
#include "drw_base.h"

#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

static std::uint64_t bit(const dwgBuffer& b) {
    return b.getPosition() * 8u + b.getBitPos();
}
static void spanInt(const char* name, std::uint64_t begin, const dwgBuffer& b,
                 std::int64_t value) {
    std::cout << "FIELD " << name << ' ' << begin << ' ' << bit(b) << ' '
              << value << '\n';
}
static void spanDouble(const char* name, std::uint64_t begin, const dwgBuffer& b,
                 double value) {
    std::cout << std::setprecision(17) << "FIELD " << name << ' ' << begin
              << ' ' << bit(b) << ' ' << value << '\n';
}
static bool seek(dwgBuffer& b, std::uint64_t position) {
    if (!b.setPosition(position >> 3)) return false;
    b.setBitPos(static_cast<std::uint8_t>(position & 7u));
    return b.isGood() && bit(b) == position;
}
int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: tablegeometry_span_scan <body.bin> <version> <handle-bits>\n";
        return 2;
    }
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    const DRW::Version version = std::string(argv[2]) == "AC1027"
        ? DRW::AC1027 : DRW::AC1032;
    const std::uint32_t handleBits = static_cast<std::uint32_t>(std::stoul(argv[3]));
    if (!in.is_open() || bytes.empty() || handleBits >= bytes.size() * 8u) return 1;
    dwgBuffer b(bytes.data(), bytes.size());
    auto mark = bit(b);
    const auto type = b.getObjType(version);
    spanInt("objectType", mark, b, type);
    if (type != 525) return 1;
    mark = bit(b);
    const auto own = b.getHandle();
    spanInt("objectHandle", mark, b, static_cast<std::int64_t>(own.ref64));
    mark = bit(b);
    const auto eed = b.getBitShort();
    spanInt("eedSize", mark, b, eed);
    if (eed != 0) {
        std::cerr << "scanner requires empty common EED; observed " << eed << '\n';
        return 3;
    }
    mark = bit(b);
    const auto reactors = b.getBitLong();
    spanInt("reactorCount", mark, b, reactors);
    mark = bit(b);
    const auto xdictMissing = b.getBit();
    spanInt("xdictMissing", mark, b, xdictMissing);
    mark = bit(b);
    const auto hasDsData = b.getBit();
    spanInt("hasDataStorage", mark, b, hasDsData);

    const std::uint64_t objectBits = bytes.size() * 8u - handleBits;
    const std::uint64_t bodyStart = bit(b);
    std::cout << "PROFILE " << argv[2] << ' ' << own.ref64 << ' '
              << bytes.size() << ' ' << objectBits << ' ' << handleBits
              << ' ' << bodyStart << '\n';
    auto readLong = [&](const char* name, std::int32_t& value) {
        const auto begin = bit(b); value = b.getBitLong(); spanInt(name, begin, b, value);
        return b.isGood() && bit(b) <= objectBits;
    };
    auto readDouble = [&](const char* name, double& value) {
        const auto begin = bit(b); value = b.getBitDouble(); spanDouble(name, begin, b, value);
        return b.isGood() && bit(b) <= objectBits;
    };
    auto read3 = [&](const char* name, double (&value)[3]) {
        const auto begin = bit(b); const auto point = b.get3BitDouble();
        value[0] = point.x; value[1] = point.y; value[2] = point.z;
        std::cout << std::setprecision(17) << "FIELD " << name << ' ' << begin
                  << ' ' << bit(b) << ' ' << value[0] << ',' << value[1] << ','
                  << value[2] << '\n';
        return b.isGood() && bit(b) <= objectBits;
    };
    std::int32_t rows = 0, columns = 0, cells = 0;
    if (!readLong("rows", rows) || !readLong("columns", columns)
        || !readLong("cellCount", cells) || rows < 0 || columns < 0
        || cells < 0 || cells > 10000 || rows * static_cast<std::int64_t>(columns) != cells)
        return 1;
    std::cout << "COUNTS " << rows << ' ' << columns << ' ' << cells << '\n';
    for (std::int32_t i = 0; i < cells; ++i) {
        std::int32_t flags = 0, contentCount = 0;
        const auto flagsName = "cellFlags[" + std::to_string(i) + "]";
        if (!readLong(flagsName.c_str(), flags)) return 1;
        double width = 0, height = 0;
        const auto widthName = "cellWidthWithGap[" + std::to_string(i) + "]";
        const auto heightName = "cellHeightWithGap[" + std::to_string(i) + "]";
        if (!readDouble(widthName.c_str(), width)
            || !readDouble(heightName.c_str(), height)) return 1;
        const auto countName = "contentCount[" + std::to_string(i) + "]";
        if (!readLong(countName.c_str(), contentCount) || contentCount < 0
            || contentCount > 10000) return 1;
        for (std::int32_t j = 0; j < contentCount; ++j) {
            double p[3]{};
            auto nm = [&](const char* field) { return std::string(field) + "[" + std::to_string(i) + "][" + std::to_string(j) + "]"; };
            if (!read3(nm("topLeft").c_str(), p)) return 1;
            if (!read3(nm("center").c_str(), p)) return 1;
            for (const char* f : {"contentWidth", "contentHeight", "contentW", "contentH", "unknownBD"}) {
                double value = 0;
                if (!readDouble(nm(f).c_str(), value)) return 1;
            }
        }
    }
    std::cout << "GEOMETRY_END " << bit(b) << " object_end " << objectBits << '\n';
    if (bit(b) > objectBits || objectBits - bit(b) > 24u) return 1;

    if (!seek(b, objectBits)) return 1;
    auto readHandle = [&](const char* role, int i) {
        const std::uint64_t begin = bit(b); const dwgHandle h = b.getHandle();
        const std::uint64_t end = bit(b);
        std::uint64_t resolved = h.ref64;
        if (h.code == 0x06) resolved = own.ref64 + 1u;
        else if (h.code == 0x08) resolved = own.ref64 - 1u;
        else if (h.code == 0x0a) resolved = own.ref64 + h.ref64;
        else if (h.code == 0x0c) resolved = own.ref64 - h.ref64;
        std::cout << "HANDLE " << role << ' ' << i << ' ' << begin << ' '
                  << end << ' ' << unsigned(h.code) << ' ' << unsigned(h.size)
                  << ' ' << h.ref64 << ' ' << resolved << '\n';
        return b.isGood() ? resolved : 0u;
    };
    const auto owner = readHandle("ownerSoftPointer", -1);
    for (std::int32_t i = 0; i < reactors; ++i)
        readHandle("reactorSoftPointer", i);
    if (xdictMissing == 0) readHandle("extensionDictionaryHardOwner", -1);
    for (std::int32_t i = 0; i < cells; ++i)
        readHandle("cellSoftPointer", i);
    std::cout << "HANDLES_END " << bit(b) << " owner " << owner
              << " padding " << (bytes.size() * 8u - bit(b)) << '\n';
    return b.isGood() ? 0 : 1;
}
