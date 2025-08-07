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
#ifndef DWGUTIL_H
#define DWGUTIL_H

#include "drw_base.h"

class dwgCompressor {
public:
    bool decompress18(duint8 *cbuf, duint8 *dbuf, duint32 csize, duint32 dsize);
    static void decrypt18Hdr(duint8 *buf, duint32 size, duint32 offset);
    static duint32 copyBytes(duint32 size, duint8 *src, duint8 *dst);
};

#endif // DWGUTIL_H
