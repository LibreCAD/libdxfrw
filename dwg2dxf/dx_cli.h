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
#include <string>

namespace dwg2dxfCli {

struct Options {
    DRW::Version version = DRW::UNKNOWNV;
    bool haveVersion = false;
    bool binary = false;
    bool overwrite = false;
    bool batch = false;
    bool debug = false;
    DRW::DxfCodePagePolicy codePagePolicy = DRW::DxfCodePagePolicy::Compatible;
    bool haveCodePagePolicy = false;
};

inline DRW::Version versionOption(const std::string& token) {
    if (token == "-R12") return DRW::AC1009;
    if (token == "-v2000") return DRW::AC1015;
    if (token == "-v2004") return DRW::AC1018;
    if (token == "-v2007") return DRW::AC1021;
    if (token == "-v2010") return DRW::AC1024;
    return DRW::UNKNOWNV;
}

// Parse complete tokens before any conversion/file operation. In particular,
// a prefix such as -yUnexpected must never grant overwrite authorization.
inline bool applyOption(const std::string& token, Options& options,
                        bool allowBatch) {
    if (token == "-b") {
        options.binary = true;
    } else if (token == "-y" || token == "-Y") {
        options.overwrite = true;
    } else if (token == "-d" || token == "-D") {
        options.debug = true;
    } else if (token == "-B" && allowBatch) {
        options.batch = true;
    } else if (token == "--dxf-codepage-policy=compatible"
               || token == "--dxf-codepage-policy=exact-named") {
        if (options.haveCodePagePolicy) return false;
        options.codePagePolicy = token == "--dxf-codepage-policy=exact-named"
            ? DRW::DxfCodePagePolicy::ExactNamed : DRW::DxfCodePagePolicy::Compatible;
        options.haveCodePagePolicy = true;
    } else {
        const DRW::Version version = versionOption(token);
        if (version == DRW::UNKNOWNV || options.haveVersion)
            return false;
        options.version = version;
        options.haveVersion = true;
    }
    return true;
}

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
