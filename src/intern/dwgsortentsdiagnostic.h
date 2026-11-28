/******************************************************************************
**  libDXFrw - Library to read/write DXF files (ascii & binary)              **
**                                                                           **
**  This file is part of libDXFrw and is licensed under the GNU GPL.         **
******************************************************************************/

#ifndef DWG_SORTENTS_DIAGNOSTIC_H
#define DWG_SORTENTS_DIAGNOSTIC_H

namespace DRW {

// Enables parser cursor-span diagnostics for the in-tree source inventory
// utility. Disabled by default; not part of the installed public API.
void setSortEntsSpanDiagnostic(bool enabled) noexcept;

} // namespace DRW

#endif // DWG_SORTENTS_DIAGNOSTIC_H
