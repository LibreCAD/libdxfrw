// SPDX-License-Identifier: GPL-2.0-or-later
// Source-bound manual probe. Build against libdxfrw and the test adapter headers;
// pass an eligible AC1027 source with a typed-null LAYER 0 at handle 0x10.
#include "lc3_compat_check.cpp"
#include "libdwgr.h"
#include "intern/dwgbufferw.h"
#include <fstream>
#include <iostream>
class Capture final : public LC3CompatCheck {
public:
    void addLayer(const DRW_Layer& value) override {
        if (value.handle == 0x10u) { layer = value; ++count; }
    }
    DRW_Layer layer;
    unsigned count = 0;
};
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    std::ifstream input(argv[1], std::ios::binary);
    char magic[6]{};
    if (!input.read(magic, sizeof(magic))
        || std::string(magic, sizeof(magic)) != "AC1027") return 2;
    dwgR reader(argv[1]);
    Capture capture;
    if (!reader.read(&capture, false) || capture.count != 1u) return 3;
    const auto& layer = capture.layer;
    if (layer.unknownHandle.ref != 0u || layer.unknownHandle.rawCode != DRW::DwgHardPointer
        || layer.xrefBlockHandle.rawCode != DRW::DwgHardPointer) return 4;
    dwgBufferW body, strings, handles;
    if (!layer.encodeDwg(DRW::AC1027, &body, &strings, &handles)
        || handles.data().empty() || handles.data().back() != 0x50u) return 5;
    std::cout << "source handle=0x10 unknown=5:0 reencoded final=0x50 xref=5:0\n";
}
