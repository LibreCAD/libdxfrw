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
#ifndef DWGREADER32_H
#define DWGREADER32_H

#include <map>
#include <list>
#include "drw_textcodec.h"
#include "dwgbuffer.h"
#include "dwgreader27.h"  // Inherit from dwgReader27 if minimal changes

class dwgReader32 : public dwgReader27 {  // Extend dwgReader27 for similarity
public:
    dwgReader32(std::ifstream *stream, ddwgReaderDriver *driver)
    : dwgReader27(stream, driver) {  // Call parent constructor
    }

    bool readFileHeader();
    bool readDwgHeader(DRW_Header& hdr);
    bool readDwgClasses();
    bool readDwgHandles();
    bool readDwgTables(DRW_Header& hdr);
    bool readDwgBlocks(DRW_Interface& intfa);
    bool readDwgEntities(DRW_Interface& intfa);
    bool readDwgObjects(DRW_Interface& intfa);
    // Override methods if AC1032 differs significantly
};

#endif // DWGREADER32_H
