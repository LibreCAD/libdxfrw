// SPDX-License-Identifier: GPL-2.0-or-later
// This source-specific scratch probe intentionally includes the adapter
// implementation so it calls the exact internal admission matcher. Link only
// libdxfrw.a, not a second compiled dx_iface.cpp object. The accompanying
// audit wrapper must verify the physical witness hash before invoking it.
#define main legacy_source_probe_main
#include "probe_ac1024_rastervariables_source.cpp"
#undef main
#include "../dwg2dxf/dx_iface.cpp"
int main(int argc, char** argv) {
    if (argc != 2 || legacy_source_probe_main(argc, argv) != 0) return 1;
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    std::vector<std::uint8_t> page(29696); dwgCompressor compressor;
    if (bytes.size() < 1696+6670 || !compressor.decompress18(bytes.data()+1696,page.data(),6670,page.size())) return 2;
    Frame frame; if (!frameAt(page,2608,frame)) return 3;
    DRW_UnsupportedObject raw;
    raw.m_rawBytes = frame.body; raw.m_bodyBitSize=frame.handleBits;
    raw.m_version=DRW::AC1024; raw.m_objectType=512; raw.m_handle=0x2f;
    if (!matchesDirectNodRasterWire(raw, 0xA, 0) || matchesDirectNodRasterWire(raw,0xA,1)
        || matchesDirectNodRasterWire(raw,0xB,0)) return 4;
    std::cout << "actual AC1024 matcher accepts hash-bound frame0 and rejects mismatched frame/owner\n";
}
