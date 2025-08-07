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
#ifndef DWGREADER18_H
#define DWGREADER18_H

#include <map>
#include <list>
#include "drw_textcodec.h"
#include "dwgbuffer.h"
#include "dwgreader.h"

class dwgReader18 : public dwgReader {
public:
    dwgReader18(std::ifstream *stream, ddwgReaderDriver *driver): dwgReader(stream, driver) {
    }

    bool readFileHeader();
    bool readDwgHeader(DRW_Header& hdr);
    bool readDwgClasses();
    bool readDwgHandles();
    bool parseSysPage(duint8 *decompSec, duint32 decompSize);
    bool parseDataPage(const dwgSectionInfo &si);
    duint32 checksum(duint32 seed, duint8* data, duint64 sz);
    bool checkSentinel(duint8 *sentinel, dwgSentinel type);
};

#endif // DWGREADER18_H
