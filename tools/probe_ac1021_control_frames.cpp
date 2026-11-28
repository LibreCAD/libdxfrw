// SPDX-License-Identifier: GPL-2.0-or-later
// Extract scratch writer control bodies for the AC1021 boundary comparison.
// Link once against the pinned old writer and once against the current writer.
#include "intern/dwgwriter21.h"
#include "intern/dwgbuffer.h"
#include "intern/dwg_fixed_handles.h"
#include "drw_header.h"
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>

class DwgBlockRecordXrefWireTestAccess {
public:
    static bool run(const char* directory) {
        DRW_Header header;
        dwgWriter21 writer(nullptr, &header);
        if (!writer.prepareDwgClassManifest() || !writer.writeFileHeaderStub()
            || !writer.writeDwgHeader() || !writer.writeDwgClasses()
            || !writer.writeDwgObjects()) return false;
        for (const auto handle : {2u, 3u, 6u, 7u, 8u, 9u, 10u}) {
            const auto entry = std::find_if(writer.m_objectMap.begin(), writer.m_objectMap.end(),
                [handle](const auto& item) { return item.first == handle; });
            if (entry == writer.m_objectMap.end()) return false;
            const auto& bytes = writer.m_buf.data();
            if (entry->second >= bytes.size()) return false;
            dwgBuffer frame(const_cast<std::uint8_t*>(bytes.data() + entry->second),
                            bytes.size() - entry->second);
            const auto size = frame.getModularShort();
            const auto offset = entry->second + frame.getPosition();
            if (!frame.isGood() || size <= 0 || offset + size > bytes.size()) return false;
            std::ofstream output(std::filesystem::path(directory) / std::to_string(handle),
                                 std::ios::binary);
            output.write(reinterpret_cast<const char*>(bytes.data() + offset), size);
            if (!output) return false;
        }
        return true;
    }
};

int main(int argc, char** argv) {
    return argc == 2 && DwgBlockRecordXrefWireTestAccess::run(argv[1]) ? 0 : 1;
}
