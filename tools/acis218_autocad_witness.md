# AutoCAD ACIS-218 / ACDS known-size witness

`acis218_autocad_witness.lsp` creates one exact, asymmetric `3DSOLID` box in a
new drawing through AutoCAD's Windows ActiveX modeler. It records the requested
dimensions, AutoCAD `GetBoundingBox` extents, volume, product version, DWG
format version, and drawing units, then saves AC1027 DWG and DXF variants with
`INSUNITS=4` (millimetres) and `INSUNITS=1` (inches). It refuses to overwrite
any of its five output paths and does not edit the drawing that was active
before it ran.

This is an evidence-generation tool, not a checked-in drawing fixture. Run it
only in full AutoCAD for Windows, not AutoCAD LT:

1. Run full AutoCAD 2013 or newer on Windows (not AutoCAD LT).
2. Load this file with `APPLOAD`, run `LIBDXFRWMAKEACISWITNESS`, and choose an
   existing output directory where none of the five named outputs exists. The
   command creates and closes a separate drawing; it leaves the active drawing
   unchanged.
3. Keep all five generated files outside the repository. Preserve the manifest
   verbatim and calculate SHA-256 for each file after AutoCAD exits.
4. Independently inspect each DWG with ODA File Converter and at least one
   separately implemented ACIS reader. Compare the exact DWG→DXF output, ACDS
   owner key, schema/history references, SAB bytes/hash, raw extents, converted
   millimetre extents, volume, and topology against the manifest's analytic
   box. Record all versions, commands, logs, hashes, and tolerances.

The dimensions are center `(13,-17,19)` and lengths `(5,7,11)`, giving expected
WCS bounds `(10.5,-20.5,13.5)` to `(15.5,-13.5,24.5)` and volume `385` in
drawing-units cubed. The second pair changes only `INSUNITS`; the script
rechecks AutoCAD's modeler bounds and volume before saving it. The comparison
of the two SAB hashes and downstream reader results is deliberately external:
matching files or the script's own self-check cannot prove the ACIS unit
contract.

Autodesk documents that `ModelSpace.AddBox` returns a 3DSolid from a center and
three positive edge lengths, `GetBoundingBox` returns WCS min/max, and
`Document.SaveAs` accepts AC2013 DWG/DXF types. Autodesk documents `INSUNITS=1`
as inches and `INSUNITS=4` as millimetres, and scopes `AddBox`, `GetBoundingBox`,
and ActiveX `SaveAs` to Windows. This file has not been executed in AutoCAD;
the resulting evidence remains pending native execution and independent
readback. Do not use its outputs to claim general ACIS, ACDS, or FreeCAD solid
support until those checks pass.
