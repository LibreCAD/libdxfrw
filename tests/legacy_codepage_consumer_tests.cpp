#include "drw_base.h"
#include "intern/drw_textcodec.h"

#include <iostream>
#include <string>
#include <utility>

int main() {
    DRW_LegacyCodePageDecoder unsupported(437);
    if (unsupported.supported() || unsupported.toUtf8("plain")) return 1;
    DRW_LegacyCodePageDecoder latin(1252);
    if (!latin.supported() || latin.toUtf8("\xE9") != std::string("\xC3\xA9")) return 2;
    DRW_LegacyCodePageDecoder japanese(932);
    if (!japanese.supported() || japanese.toUtf8("\x95\x5C") != std::string("\xE8\xA1\xA8")) return 3;
    // The legacy codec processes DXF escapes; plain-text users must segment
    // literal backslashes as their importers already do.
    if (japanese.toUtf8("\\U+0041") != std::string("A")) return 4;
    auto moved = std::move(japanese);
    if (japanese.supported() || !moved.supported() ||
        moved.toUtf8("\x95\x5C") != std::string("\xE8\xA1\xA8")) return 5;

    // The public bridge must preserve both consumer-used internal routes,
    // including undefined and trailing bytes. Check every two-byte CP932 input
    // against the direct converter, and single bytes for all selected pages.
    DRW_Conv932Table original932;
    for (unsigned i = 0; i != 256; ++i) {
        for (unsigned j = 0; j != 256; ++j) {
            const std::string bytes{static_cast<char>(i), static_cast<char>(j)};
            if (moved.toUtf8(bytes) != original932.toUtf8(bytes)) return 6;
        }
    }
    for (int page : {874, 932, 936, 949, 950, 1250, 1251, 1252, 1253,
                     1254, 1255, 1256, 1257, 1258}) {
        DRW_LegacyCodePageDecoder bridge(page);
        DRW_TextCodec original;
        original.setCodePage("ANSI_" + std::to_string(page), false);
        if (!bridge.supported()) return 7;
        for (unsigned i = 0; i != 256; ++i) {
            const std::string bytes(1, static_cast<char>(i));
            if (bridge.toUtf8(bytes) != original.toUtf8(bytes)) return 8;
        }
    }
    std::cout << "Legacy consumer codec controls passed\n";
}
