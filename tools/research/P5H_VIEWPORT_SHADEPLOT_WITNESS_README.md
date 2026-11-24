# P5-H native viewport shade-plot witness

This packet prepares a native witness; it does **not** establish support or
change the writer. The host, release, build and available APIs must first be
recorded in [the host intake](OBJECTARX_HOST_PROFILE_INTAKE.md). The procedure
below is an operator workflow, not a tested AutoCAD plug-in. If the selected
release does not offer a required operation, record the failure and retain the
gate rather than substituting a different state.

## Capture

1. On an identified AutoCAD host, create a new scratch drawing with a paper
   space layout and three identifiable viewports. Record the layout and each
   viewport handle in a transcript. Select one viewport per state: AsDisplayed,
   VisualStyle and RenderPreset. Set and query each state using the native
   release's supported UI or ObjectARX/.NET APIs; record the actual enum name,
   target object's handle, runtime class, owner and dictionary/key where
   applicable. Preserve an AsDisplayed/null-target control. Do not infer the
   native state from the DXF group values. Record unavailable modes or targets.
2. Save a DWG and export **separate ASCII and Binary DXFs** in the chosen
   same target release. Record the application's exact save/export operations,
   requested versions, native API values, file names and SHA-256 hashes. Use
   locally created scratch drawings; do not commit the generated files.
3. Close and independently reopen the DWG and each exported DXF in AutoCAD.
   Before changing the reopened document, record its file identity, hash,
   `DBMOD`, layout, viewport handle, `ShadePlot` and `ShadePlotId`, target
   class/owner/dictionary/key, and rendered appearance. Record null/missing
   targets and failures explicitly. Correlate using the handles observed in
   each reopened file; do not assume an export or import preserves handles.
   Record whether `DBMOD` stays zero during observation and hash the source
   files again. A dirty session, changed file, ambiguous source identity, or
   missing state does not qualify that witness.
4. Return the host profile, exact command/API transcript, hashes, screenshots
   or visual observations, per-file native reports, and all three unmodified
   files through the agreed private transfer route. No SDK, credentials, or
   third-party drawings need to enter the repository.

The native observer should be read-only: it must never save the drawing or
write its report over a drawing. If a DXF reopen creates a new drawing or the
application cannot establish the originating file, record that limitation
instead of claiming hash-bound native reopening. Use a separately recorded
import workflow if native DXF reopen semantics require it.

## Primary API mapping to capture

Autodesk's current `ShadePlotType` reference defines `kVisualStyle = 4` and
`kRenderPreset = 5`; its `AcDbViewport::setShadePlot` reference requires the
shade-plot object ID to identify an `AcDbVisualStyle` or
`AcDbRenderSettings`, respectively. The same setter says the object ID is
ignored and reset to null for `kAsDisplayed` and `kRendered`. Record these API
values and target classes before export, then inspect the emitted DXF rather
than assuming the API enum is serialized directly.

The published VIEWPORT DXF table still lists group 170 values only through 3;
it also lists group 333 as the optional shade-plot ID and group 348 as an
optional visual-style ID. Treat group 333 and group 348 as separate fields.
The API references establish the in-memory states and target-class constraints,
but do **not** establish whether AutoCAD exports modes 4/5 in group 170, how
group 333 is emitted for each, or whether group 348 is also present. Those are
the exact questions the ASCII/Binary producer pair must answer.

## Bounded wire inspection

Run on the received, hash-verified DXFs from this checkout:

```sh
python3 -B tools/research/audit_p5h_viewport_shadeplot.py --self-test
python3 -B tools/research/audit_p5h_viewport_shadeplot.py \
  /path/to/witness-ascii.dxf /path/to/witness-binary.dxf > /path/to/wire-report.json
```

The observer supports UTF-8 AC1021-and-later ASCII and Binary DXF. It reports
literal VIEWPORT group 91, 170, 333 and 348 values, source and owner handles,
and whether each group-333 handle has no target, one target, or multiple
candidate records. It rejects malformed handles, duplicate group-5 identities
within a record, malformed application-data nesting, missing EOF, and missing
VIEWPORT markers. It is not a full DXF grammar or graph validator; a unique
record is not proof of target semantics. Values inside group-102 application
data are excluded. It deliberately does not interpret 170 as an API enum.
Retain duplicate group 170/333 values as ambiguity, not an implicit choice.

Correlate the reported viewport and target handles with each file's native
report. A useful result distinguishes VisualStyle, RenderPreset and null
controls, records exact group values and target runtime classes, verifies the
requested `$ACADVER` and hash of each file, and explains any difference across
ASCII/Binary exports. Native rendering and fresh independent readback remain
separate acceptance checks. Only then can P5-H's exact guard and retry behavior
be reviewed and implemented; this packet cannot promote support by itself.

Autodesk references: [VIEWPORT DXF group codes](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-2602B0FB-02E4-4B9A-B03C-B1D904753D34.htm),
[ShadePlotType enum](https://help.autodesk.com/cloudhelp/2027/ENU/OARX-RefGuide/files/OARX-RefGuide-AcDbPlotSettings__ShadePlotType.html),
[AcDbViewport::setShadePlot](https://help.autodesk.com/cloudhelp/2017/ENU/OARXMAC-RefGuide/files/OREFMAC-AcDbViewport__setShadePlot_ShadePlotType_AcDbObjectId.html),
and [managed Viewport.SetShadePlot](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_Viewport_SetShadePlot_ShadePlotType_ObjectId.html).
