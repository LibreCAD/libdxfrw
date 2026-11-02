/*
 * ********************************************************************************
 * This file is part of the LibreCAD project, a 2D CAD program
 *
 * Copyright (C) 2026 LibreCAD.org
 * Copyright (C) 2026 Dongxu Li (github.com/dxli)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * ********************************************************************************
 */

#ifndef DWG_VALUE_KIND_H
#define DWG_VALUE_KIND_H

#include <cstdint>

namespace drwDwgValue {

enum class Kind {
    Invalid,
    Real,
    Int8,
    Int16,
    Int32,
    Int64,
    String,
    Point3d,
    Binary,
    Handle,
    ObjectId,
    Bool
};

// Group-code discriminator used by AcDbEvalVariant. Keep this separate from
// the value decoder: some well-typed DXF group-code kinds remain unsupported
// in the associative DWG value grammar and must still fail closed there.
inline Kind resbufValueKind(std::int32_t groupCode) {
    if (groupCode >= 300) {
        if (groupCode >= 440) {
            if (groupCode >= 1000) {
                if (groupCode == 1004)
                    return Kind::Binary;
                if (groupCode == 1005)
                    return Kind::Handle;
                if (groupCode <= 1009)
                    return Kind::String;
                if (groupCode <= 1039)
                    return Kind::Point3d;
                if (groupCode <= 1042)
                    return Kind::Real;
                if (groupCode <= 1069)
                    return Kind::Point3d;
                if (groupCode <= 1070)
                    return Kind::Int16;
                if (groupCode == 1071)
                    return Kind::Int32;
            } else {
                if (groupCode <= 459)
                    return Kind::Int32;
                if (groupCode <= 469)
                    return Kind::Real;
                if (groupCode <= 479)
                    return Kind::String;
                if (groupCode == 999)
                    return Kind::String;
            }
        } else if (groupCode >= 390) {
            if (groupCode <= 399)
                return Kind::Handle;
            if (groupCode <= 409)
                return Kind::Int16;
            if (groupCode <= 419)
                return Kind::String;
            if (groupCode <= 429)
                return Kind::Int32;
            if (groupCode <= 439)
                return Kind::String;
        } else {
            if (groupCode <= 309)
                return Kind::String;
            if (groupCode <= 319)
                return Kind::Binary;
            if (groupCode <= 329)
                return Kind::Handle;
            if (groupCode <= 369)
                return Kind::ObjectId;
            if (groupCode <= 389)
                return Kind::Int16;
        }
    } else if (groupCode >= 105) {
        if (groupCode >= 210) {
            if (groupCode <= 239)
                return Kind::Point3d;
            if (groupCode <= 269)
                return Kind::Invalid;
            if (groupCode <= 279)
                return Kind::Int16;
            if (groupCode <= 289)
                return Kind::Int8;
            if (groupCode <= 299)
                return Kind::Bool;
        } else {
            if (groupCode == 105)
                return Kind::Handle;
            // These unassigned codes are not the 3D-point range (110-139).
            if (groupCode <= 109)
                return Kind::Invalid;
            if (groupCode <= 139)
                return Kind::Point3d;
            if (groupCode <= 149)
                return Kind::Real;
            if (groupCode <= 159)
                return Kind::Invalid;
            if (groupCode <= 169)
                return Kind::Int64;
            if (groupCode <= 179)
                return Kind::Int16;
        }
    } else if (groupCode >= 38) {
        if (groupCode <= 59)
            return Kind::Real;
        if (groupCode <= 79)
            return Kind::Int16;
        if (groupCode <= 89)
            return Kind::Invalid;
        if (groupCode <= 99)
            return Kind::Int32;
        if (groupCode == 100 || groupCode == 102)
            return Kind::String;
    } else {
        if (groupCode < 0)
            return Kind::Handle;
        if (groupCode <= 4)
            return Kind::String;
        if (groupCode == 5)
            return Kind::Handle;
        if (groupCode <= 9)
            return Kind::String;
        if (groupCode <= 37)
            return Kind::Point3d;
    }
    return Kind::Invalid;
}

// AcDbAssocAction's AcDbEvalVariant body is not covered by the generic
// XDATA/resbuf wire description.  In particular, the broad DXF 1000-1009
// string range is not a safe DWG payload decoder: 1005 is an entity-handle
// type in the ODA EED description, while the class-specific EvalVariant
// encoding for 1005-1009 is not specified there.  Keep those values opaque
// until that consumer's exact wire grammar is established.
inline Kind associativeActionValueKind(std::int32_t groupCode) {
    // DXF's generic group-code category is not an associative-action DWG
    // payload grammar. Keep the locally observed/previously admitted value
    // categories narrow until an exact class layout or producer trace exists.
    if ((groupCode >= 280 && groupCode <= 289)
        || (groupCode >= 1005 && groupCode <= 1009))
        return Kind::Invalid;
    return resbufValueKind(groupCode);
}

} // namespace drwDwgValue

#endif // DWG_VALUE_KIND_H
