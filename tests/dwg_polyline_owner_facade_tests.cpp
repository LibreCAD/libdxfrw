#include "lc3_compat_check.cpp"
#include "libdwgr.h"

#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {
class Writer final : public LC3CompatCheck {
public:
    Writer(dwgRW& output, bool three) : output_(output), three_(three) {}
    void writeEntities() override {
        DRW_Polyline p;
        p.flags = three_ ? 8 : 0;
        p.addVertex(DRW_Vertex(1., 2., 0., 0.));
        p.addVertex(DRW_Vertex(3., 4., three_ ? 1. : 0., 0.));
        // Collision inside one compound must fail before publishing any frame.
        p.vertlist[0]->handle = 0x90;
        p.vertlist[1]->handle = 0x90;
        const bool rejected = !output_.writePolyline(&p)
            && p.handle == 0 && p.vertlist[0]->handle == 0x90
            && p.vertlist[1]->handle == 0x90;
        p.vertlist[1]->handle = 0xA0;
        const bool written = output_.writePolyline(&p);
        DRW::DwgCompoundEntityWriteReceipt receipt;
        const bool captured = output_.getLastDwgCompoundEntityWriteReceipt(receipt)
            && receipt.valid && receipt.frames.size() == 4
            && receipt.frames.front().role == DRW::DwgCompoundEntityFrameRole::Parent
            && receipt.frames.back().role == DRW::DwgCompoundEntityFrameRole::SeqEnd;
        DRW_Line next;
        next.basePoint = {10., 11., 0.}; next.secPoint = {12., 13., 0.};
        ok_ = rejected && written && captured && output_.writeLine(&next);
    }
    bool finalizeDwgWrite() override { return ok_; }
    bool ok() const { return ok_; }
private:
    dwgRW& output_;
    bool three_;
    bool ok_ = false;
};

class Reader final : public LC3CompatCheck {
public:
    void addPolyline(const DRW_Polyline& p) override {
        ++count;
        if (p.vertlist.size() != 2 || !p.vertlist[0] || !p.vertlist[1]) return;
        const auto& a = *p.vertlist[0]; const auto& b = *p.vertlist[1];
        good = a.handle == 0x90 && b.handle == 0xA0
            && a.parentHandle == p.handle && b.parentHandle == p.handle
            && a.basePoint.x == 1. && a.basePoint.y == 2.
            && b.basePoint.x == 3. && b.basePoint.y == 4.
            && b.basePoint.z == (three ? 1. : 0.) && p.dwgSeqEndHandle() != 0;
    }
    void addLine(const DRW_Line& p) override {
        if (p.basePoint.x == 10. && p.secPoint.x == 12.) ++lines;
    }
    bool three = false;
    bool good = false;
    int count = 0, lines = 0;
};
}

int main() {
    const auto root = std::filesystem::temp_directory_path() /
        ("polyline-owner-facade-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(root);
    struct Cleanup { std::filesystem::path root; ~Cleanup() { std::error_code e; std::filesystem::remove_all(root, e); } } cleanup{root};
    for (const auto version : {DRW::AC1018, DRW::AC1021, DRW::AC1024, DRW::AC1027, DRW::AC1032}) {
        for (const bool three : {false, true}) {
            const auto path = root / (std::to_string(static_cast<int>(version)) + (three ? "-3.dwg" : "-2.dwg"));
            dwgRW output(path.string().c_str()); Writer writer(output, three);
            if (!output.write(&writer, version, true) || !writer.ok()) {
                std::cerr << "write/retry failed for " << static_cast<int>(version) << ", 3D=" << three << '\n'; return 1;
            }
            dwgRW input(path.string().c_str()); Reader reader; reader.three = three;
            if (!input.read(&reader, false) || reader.count != 1 || !reader.good || reader.lines != 1) {
                std::cerr << "readback failed for " << static_cast<int>(version) << ", 3D=" << three << '\n'; return 1;
            }
        }
    }
    return 0;
}
