/* SPDX-License-Identifier: GPL-2.0-or-later */
#ifndef LIBDXFRW_DRW_DXF_EXTRUSION_H
#define LIBDXFRW_DRW_DXF_EXTRUSION_H

#include "drw_entities.h"

// Explicit recovery of the representation produced by exactly one legacy
// DRW_Circle/DRW_Arc::applyExtrusion() call. This is an opt-in conversion for
// DXF writers and the bounded AC1024 DWG writer route: it does not alter the
// semantics of existing readers or writers.
//
// The caller must know that the input was transformed exactly once and that
// subsequent edits have not invalidated its coordinate/normal interpretation.
// The input's haveExtrusion value must still be the one used by that call. If
// it was false, that call was a no-op and the output remains native. ARC angles
// reflect the legacy method's limited axial-negative-Z handling; this function
// is not a general conversion of arbitrary WCS angles.
//
// Exact built-in runtime types for both input and output, and matching input
// entity tags, are required to avoid slicing or partially overwriting derived
// payloads. For an applied extrusion, a direction or basis whose nonzero
// components or cross-products were lost or subnormal is refused because
// the inverse cannot reliably recover the original coordinates. A zero/nonfinite normal,
// nonfinite fields, type mismatch or unrepresentable result fails without
// changing the output; the input and output may alias. The returned copy is for
// native DXF OCS or AC1024 DWG output. Other DWG revisions are unqualified.
namespace DRW_DxfLegacyExtrusion {
[[nodiscard]] bool toNativeOcs(const DRW_Circle& input, DRW_Circle& output) noexcept;
[[nodiscard]] bool toNativeOcs(const DRW_Arc& input, DRW_Arc& output) noexcept;
// Recover all four OCS corners after exactly one legacy TRACE/SOLID extrusion
// application. The same caller preconditions and atomicity apply. These
// overloads are for DXF output; DWG output is not yet qualified. A 3DFACE is
// excluded even though it derives from DRW_Trace, because its points use WCS.
[[nodiscard]] bool toNativeOcs(const DRW_Trace& input, DRW_Trace& output) noexcept;
[[nodiscard]] bool toNativeOcs(const DRW_Solid& input, DRW_Solid& output) noexcept;
// Recover a TEXT insertion and alignment point after exactly one actual legacy
// extrusion application, as performed by the DXF reader when requested. This
// is for DXF output; the DWG reader does not currently apply that transform to
// TEXT. Exact runtime types exclude ATTRIB, ATTDEF, MTEXT and other subclasses.
[[nodiscard]] bool toNativeOcs(const DRW_Text& input, DRW_Text& output) noexcept;
// Recover the insertion point of an exact, un-attributed INSERT after one
// actual legacy extrusion application. Attached attributes, a retained SEQEND,
// or stored child handles are refused; their independent frames need separate
// recovery. This overload is qualified for DXF output only. The current DWG
// reader delivers INSERT in its native frame without applying this transform.
[[nodiscard]] bool toNativeOcs(const DRW_Insert& input, DRW_Insert& output) noexcept;
}

// Nonmutating extraction of LWPOLYLINE vertex positions in WCS. Input must
// still be in its native OCS, such as the value delivered by DXF read(ext=false):
// the entity does not record whether legacy applyExtrusion() already projected
// its vertices, so that state cannot be detected here. This returns vertex
// positions only; bulges, widths, thickness, and intervening curved segments
// retain their original OCS meanings and are not transformed or tessellated.
// Reject invalid normals, directions or bases with lost/subnormal components,
// null vertices, nonfinite values or unrepresentable coordinates without
// changing output. On success output is replaced.
namespace DRW_DxfGeometry {
[[nodiscard]] bool lwPolylineVerticesToWcs(const DRW_LWPolyline& input,
                                          std::vector<DRW_Coord>& output) noexcept;
}

#endif
