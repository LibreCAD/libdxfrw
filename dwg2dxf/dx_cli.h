/******************************************************************************
**  FreeCAD-compatible dwg2dxf CLI version policy.                          **
**                                                                           **
**  This library is free software, licensed under the terms of the GNU       **
**  General Public License as published by the Free Software Foundation,     **
**  either version 2 of the License, or (at your option) any later version.  **
******************************************************************************/

#ifndef DX_CLI_H
#define DX_CLI_H

#include "drw_base.h"

namespace dwg2dxfCli {

inline DRW::Version defaultOutputVersion(DRW::Version sourceVersion){
    if (sourceVersion == DRW::AC1012)
        return DRW::AC1014;

    switch (sourceVersion) {
    case DRW::AC1009:
    case DRW::AC1014:
    case DRW::AC1015:
    case DRW::AC1018:
    case DRW::AC1021:
    case DRW::AC1024:
    case DRW::AC1027:
    case DRW::AC1032:
        return sourceVersion;
    default:
        return DRW::UNKNOWNV;
    }
}

} // namespace dwg2dxfCli

#endif // DX_CLI_H
