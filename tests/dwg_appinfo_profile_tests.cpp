#include "../src/intern/dwgappinfo.h"

#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

void appendUInt16(std::vector<std::uint8_t>& bytes, std::uint16_t value) {
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffu));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8u) & 0xffu));
}

void appendUInt32(std::vector<std::uint8_t>& bytes, std::uint32_t value) {
    for (unsigned int shift = 0u; shift < 32u; shift += 8u)
        bytes.push_back(static_cast<std::uint8_t>((value >> shift) & 0xffu));
}

void appendUtf16(std::vector<std::uint8_t>& bytes, const std::string& text) {
    appendUInt16(bytes, static_cast<std::uint16_t>(text.size() + 1u));
    for (unsigned char value : text)
        appendUInt16(bytes, value);
    appendUInt16(bytes, 0u);
}

void appendChecksum(std::vector<std::uint8_t>& bytes) {
    bytes.insert(bytes.end(), 16u, 0u);
}

std::vector<std::uint8_t> makeAppInfo(
    const std::string& version, const std::string& product,
    const std::string& registryVersion = "17.0") {
    std::vector<std::uint8_t> bytes;
    appendUInt32(bytes, 2u);
    appendUtf16(bytes, "AppInfoDataList");
    appendUInt32(bytes, 3u);
    appendChecksum(bytes);
    appendUtf16(bytes, version);
    appendChecksum(bytes);
    appendUtf16(bytes, "Autodesk DWG.");
    appendChecksum(bytes);
    appendUtf16(bytes, "\"<ProductInformation name =\\\"" + product
        + "\\\" registry_version=\\\"" + registryVersion
        + "\\\"/>\"");
    return bytes;
}

bool expect(const char *name, const std::vector<std::uint8_t>& bytes,
            dwgAppInfo::R21ProducerProfile expected) {
    const auto actual = dwgAppInfo::classifyR21Producer(
        bytes.data(), bytes.size());
    if (actual == expected)
        return true;
    std::cerr << name << ": unexpected producer profile\n";
    return false;
}

} // namespace

int main() {
    using Profile = dwgAppInfo::R21ProducerProfile;
    bool passed = true;
    auto autoCad2007 = makeAppInfo("17.0.48.0", "AutoCAD");
    passed = expect("AutoCAD 2007", autoCad2007, Profile::AutoCAD2007)
        && passed;

    auto autoCad2008 = makeAppInfo("17.1.0.0", "AutoCAD", "17.1");
    passed = expect("AutoCAD 2008", autoCad2008, Profile::Unknown) && passed;
    auto autoCad2009 = makeAppInfo("17.2.0.0", "AutoCAD", "17.2");
    passed = expect("AutoCAD 2009", autoCad2009, Profile::Unknown) && passed;
    auto malformedVersion = makeAppInfo("17.0.release", "AutoCAD");
    passed = expect("malformed product version", malformedVersion,
                    Profile::Unknown) && passed;
    auto otherProduct = makeAppInfo("17.0.48.0", "Teigha");
    passed = expect("other product", otherProduct, Profile::Unknown) && passed;
    auto conflictingRegistry = makeAppInfo("17.0.48.0", "AutoCAD", "17.1");
    passed = expect("conflicting metadata", conflictingRegistry,
                    Profile::Unknown) && passed;

    auto malformedLength = autoCad2007;
    malformedLength[4] = 1u;
    malformedLength[5] = 0u;
    passed = expect("malformed string length", malformedLength,
                    Profile::Unknown) && passed;
    auto truncated = autoCad2007;
    truncated.resize(20u);
    passed = expect("truncated section", truncated, Profile::Unknown)
        && passed;
    passed = dwgAppInfo::classifyR21Producer(nullptr, 0u) == Profile::Unknown
        && passed;

    if (!passed)
        return 1;
    std::cout << "AC1021 AppInfo producer-profile controls passed\n";
    return 0;
}
