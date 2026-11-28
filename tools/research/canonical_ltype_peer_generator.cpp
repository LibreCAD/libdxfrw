// SPDX-License-Identifier: GPL-2.0-or-later
// Generate a scratch DXF for independent canonical LTYPE peer checks.
// Compile with -std=c++17 -I tests -I src and link the current libdxfrw.
#include "lc3_compat_check.cpp"
#include "libdxfrw.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <utility>

namespace {
class Export final : public LC3CompatCheck {
public:
    Export(dxfRW& writer, std::string description, int flags) : codec(writer) {
        row.name = "CONTINUOUS";
        row.desc = std::move(description);
        row.flags = flags;
        row.handle = 0xD731u;
    }
    void writeHeader(DRW_Header&) override {}
    void writeLTypes() override { accepted = codec.writeLineType(&row); }
    bool finalizeDxfWrite() override { return accepted; }
    dxfRW& codec;
    DRW_LType row;
    bool accepted = false;
};
} // namespace

int main(int argc, char** argv) {
    if (argc != 5 || (std::string(argv[2]) != "ascii" && std::string(argv[2]) != "binary")
        || (std::string(argv[4]) != "0" && std::string(argv[4]) != "64")) {
        std::cerr << "usage: canonical_ltype_peer_generator OUTPUT ascii|binary DESCRIPTION 0|64\n";
        return 2;
    }
    const std::filesystem::path output(argv[1]);
    std::error_code error;
    const auto status = std::filesystem::symlink_status(output, error);
    if (error == std::errc::no_such_file_or_directory) error.clear();
    if (error || status.type() != std::filesystem::file_type::not_found) {
        std::cerr << "output already exists or cannot be inspected\n";
        return 2;
    }
    dxfRW writer(output.string().c_str());
    Export exporter(writer, argv[3], std::string(argv[4]) == "64" ? 64 : 0);
    writer.setCanonicalLineTypeMetadata({exporter.row});
    return writer.write(&exporter, DRW::AC1015, std::string(argv[2]) == "binary")
        && exporter.accepted ? 0 : 1;
}
