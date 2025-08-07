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
#include "dwgreader.h"
#include "dwgreader15.h"
#include "dwgreader18.h"
#include "dwgreader21.h"
#include "dwgreader24.h"
#include "dwgreader27.h"
#include "dwgreader32.h"

dwgReader *dwgReader::getReader(std::ifstream *stream, dwgR *p) {
    if (!stream)
        return NULL;

    // Other code...

    if (version.substr(0,6) == "AC1027") {
        reader = new dwgReader27(stream, p);
    }
    else if (version.substr(0,6) == "AC1032") {
        reader = new dwgReader32(stream, p);
    }
    else {
        DRW_DBG("dwgReader::readFileHeader: Not supported version\n");
        return NULL;
    }

    // Other code...
}
