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
#include "dwgreader18.h"
#include "drw_textcodec.h"
#include "dwgutil.h"
#include "../drw_objects.h"
#include "../drw_entities.h"
#include "../libdwgr.h"

// Other includes and code before the changed parts...

duint32 dwgReader18::checksum(duint32 seed, duint8* data, duint64 sz){
    // Implementation of checksum...
    // (Assuming the original code, as not changed in the patch)
}

//called: Section page map: 0x41630e3b
bool dwgReader18::parseSysPage(duint8 *decompSec, duint32 decompSize){
    DRW_DBG("\nparseSysPage:\n ");
    duint32 compSize = fileBuf->getRawLong32();
    DRW_DBG("Compressed size= "); DRW_DBG(compSize); DRW_DBG(", "); DRW_DBGH(compSize);

    // Other code in the function...

 #endif
    DRW_DBG("decompressing "); DRW_DBG(compSize); DRW_DBG(" bytes in "); DRW_DBG(decompSize); DRW_DBG(" bytes\n");
    dwgCompressor comp;
    if (!comp.decompress18(tmpCompSec.data(), decompSec, compSize, decompSize)) {
        return false;
    }

 #ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< decompSize;i++) {
        DRW_DBGH( decompSec[i]);
        if (j == 7) { DRW_DBG("\n"); j = 0;
        } else { DRW_DBG(", "); j++; }
    } DRW_DBG("\n");
 #endif

    return true;
}

//called ???: Section map: 0x4163003b

// Other code...

bool dwgReader18::parseDataPage(const dwgSectionInfo &si/*, duint8 *dData*/){
    // Other code...

    pi.uSize = si.maxSize;
    DRW_DBG("decompressing "); DRW_DBG(pi.cSize); DRW_DBG(" bytes in "); DRW_DBG(pi.uSize); DRW_DBG(" bytes\n");
    dwgCompressor comp;
    if (!comp.decompress18(pi.cData.data(), pi.uData.data(), pi.cSize, pi.uSize)) {
        return false;
    }
 #ifdef DRW_DBG_DUMP
    for (unsigned int i=0, j=0; i< pi.uSize;i++) {
        DRW_DBGH( pi.uData[i]);
        // Other code...
    }

    // Other code after...
}
