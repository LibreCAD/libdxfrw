// SPDX-License-Identifier: GPL-2.0-or-later
// Locally authored AC1024 RTEXT successor probe. The two supported handles
// isolate the original sparse-identity warning from a compact handle map.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
class Author : public LC3CompatCheck {
public:
    Author(dwgRW& output, std::uint32_t entityHandle)
        : output_(output), entityHandle_(entityHandle) {}

    void writeDwgClasses() override {
        registered = output_.registerDwgEntityClassInstance(
            DRW_RText::kDwgClassNum, entityHandle_);
    }
    void writeEntities() override {
        DRW_RText entity;
        entity.handle = entityHandle_;
        entity.text = "LOCAL_RTEXT";
        entity.basePoint = {90., 91., 0.};
        entity.extPoint = {0., 0., 1.};
        entity.angle = 15.;
        entity.height = 2.;
        entity.m_rTextFlags = 1;
        written = output_.writeRText(&entity);
    }
    bool registered = false;
    bool written = false;

private:
    dwgRW& output_;
    std::uint32_t entityHandle_;
};

class Capture : public LC3CompatCheck {
public:
    void addText(const DRW_Text& value) override {
        if (value.text != "LOCAL_RTEXT") return;
        ++count;
        const auto* typed = dynamic_cast<const DRW_RText*>(&value);
        matches = typed && value.basePoint.x == 90. && value.basePoint.y == 91.
            && value.basePoint.z == 0. && std::abs(value.angle - 15.) <= 1e-12;
    }
    int count = 0;
    bool matches = false;
};
}

int main(int argc, char** argv) {
    if (argc != 3 || (std::string(argv[2]) != "original"
                      && std::string(argv[2]) != "compact")) {
        std::cerr << "usage: ac1024_rtext_successor_v3 OUTPUT.dwg original|compact\n";
        return 2;
    }
    const std::uint32_t handle = std::string(argv[2]) == "original" ? 0xED00u : 0x30u;
    dwgRW writer(argv[1]);
    Author author(writer, handle);
    if (!writer.write(&author, DRW::AC1024, true)
        || !author.registered || !author.written) {
        std::cerr << "RTEXT authoring failed\n";
        return 1;
    }
    dwgRW reader(argv[1]);
    Capture capture;
    if (!reader.read(&capture, false) || capture.count != 1 || !capture.matches) {
        std::cerr << "RTEXT local readback failed\n";
        return 1;
    }
    std::cout << "RTEXT insertion (90, 91, 0) and angle 15 degrees: PASS\n";
    return 0;
}
