// SPDX-License-Identifier: GPL-2.0-or-later
#include <drw_acis.h>
#include <drw_base.h>
#include <drw_classes.h>
#include <drw_datastorage.h>
#include <drw_entities.h>
#include <drw_header.h>
#include <drw_interface.h>
#include <drw_objects.h>
#include <libdwgr.h>
#include <libdxfrw.h>

int main() {
    dxfRW codec("");
    if (codec.dxfCompatibilityProfile() !=
        dxfRW::DxfCompatibilityProfile::StandaloneSafe)
        return 1;
    codec.setDxfCompatibilityProfile(
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy);
    if (codec.dxfCompatibilityProfile() !=
        dxfRW::DxfCompatibilityProfile::LibreCadMasterLegacy)
        return 2;
    return codec.getError() == DRW::BAD_NONE ? 0 : 3;
}
