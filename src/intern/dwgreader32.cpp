/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  Copyright (C) 2025 librecad.org                                          **
**  Copyright (C) 2025 Dongxu Li (github.com/dxli)                           **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
**  You should have received a copy of the GNU General Public License        **
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.    **
******************************************************************************/

#include "dwgreader32.h"
#include "drw_textcodec.h"
#include "dwgutil.h"
#include "../drw_objects.h"
#include "../drw_entities.h"
#include "../libdwgr.h"

bool dwgReader32::readFileHeader() {
    DRW_DBG("\ndwgReader32::readFileHeader\n");
    char sg[7];
    sg[6] = '\0';
    filestr->read(sg, 6);
    std::string version(sg);
    DRW_DBG("dwgReader32::readFileHeader version: "); DRW_DBG(version.c_str()); DRW_DBG("\n");
    if (version != "AC1032") { // not supported version
        return false;
    }
    // Similar to dwgReader27, but check for format differences
    // e.g., seek to maintenance release version, etc.
    filestr->seekg(0x0D, std::ios::beg);
    maintenanceVersion = getc();
    DRW_DBG("maintenance version: "); DRW_DBG(maintenanceVersion); DRW_DBG("\n");
    // ... rest copied from dwgreader27.cpp, adjust if new header fields exist
    return true;
}

bool dwgReader32::readDwgHeader(DRW_Header& hdr){
    // Identical to dwgReader27 for now; customize for new vars like APPID_CONTROL or new flags
    // ...
}

// Add other methods similarly copied and adapted from dwgreader27.cpp
// Key areas to update: parseSysPage, parseDataPage, readDwgClasses, readDwgHandles, etc.
// If compression changed (e.g., new LZ or encryption), update dwgCompressor

// End of file (full implementation would be ~500+ lines like dwgreader27)
