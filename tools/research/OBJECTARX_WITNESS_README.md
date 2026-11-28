# Native AutoCAD scratch witness packet

This packet prepares a **new scratch drawing**, captures an AutoCAD-produced
DWG/ASCII-DXF pair and reports raw DXF records. It does not upload anything,
need credentials, use an external drawing, or establish support by itself. It
has not been run on the remote host. Run it interactively as the host operator;
record the exact AutoCAD product, year, build, OS, language, and commands. A
full AutoCAD version with AutoLISP, `layerstate-save` / `layerstate-restore`,
Block Editor and DXF export is required. Availability varies by product and
release; stop and record an unavailable API instead of substituting a guessed
implementation. The companion LISP uses documented AutoLISP APIs, **not** a
compiled ObjectARX binary or a claim that every AutoCAD variant supports them.
The scanner requires Python 3.6+; it does not require AutoCAD or extra modules.

## Controlled creation

1. Make a new empty local directory reserved for these scratch outputs. Enable
   AutoCAD's command history/log recording or capture the entire text window;
   retain its transcript in that directory. Ensure no secrets, drawings, or
   unrelated work are in the captured log. Start a fresh blank, untitled,
   unmodified drawing from a blank template. `DXFRWCREATE` refuses a titled,
   modified or graphically nonempty drawing, or conflicting witness names.
   A template that itself marks the document modified will be refused: use a
   genuinely blank template rather than weakening the check. These guards are
   safeguards, not an assertion that every possible template is empty.
2. Load `objectarx_witness.lsp` with `APPLOAD`, then invoke `DXFRWCREATE`.
   Capture every printed value and any failure. The command makes two colored,
   overlapping SOLIDs in the named block `DXFRW_ORDER`, inserts the block, and
   saves native layer states `DXFRW_DEFAULT` and `DXFRW_EXPLICIT` with mask
   255. It sets and restores layer `DXFRW_A` lineweight and prints the observed
   values. It stops on a failed native operation. Any partial in-memory work is
   scratch; discard the untitled drawing on failure and restart, do not rerun
   into it. In particular, if the producer rejects or normalizes `-3`, record
   that rather than claiming a `-3` witness. The command creates no files.
3. In Block Editor, open `DXFRW_ORDER`; use AutoCAD's own Draw Order command
   to reverse the visual stacking of the overlapping SOLIDs, then save and
   close Block Editor. Record exactly which colored entity was moved and the
   before/after visible order. This may or may not create a named-block
   SORTENTSTABLE; inspect the output instead of assuming it does. Native API
   semantics allow sort keys that do not identify existing entities, but this
   GUI experiment does **not** create or prove such a case. A separate native
   ObjectARX experiment is required for a non-existing sort-key witness.
4. Also create a named view `DXFRW_VIEW` with the producer's **Save Layer
   Snapshot with View** option, if the installed version exposes it. Record
   the view's creation settings, layer visibility and current layer-weight
   values. This is a candidate for `ACAD_VIEWS_...`, not a claim that the
   producer must store it that way. Preserve a screenshot or written record.
5. Run `DXFRWREPORT`, and verify that it reports exactly one INSERT with a
   nonempty INSERT handle and owner handle; record both handles. Then save a
   native **AutoCAD 2010 DWG** as
   `witness.dwg` in the reserved directory. From the same unmodified drawing,
   export **AutoCAD 2010 ASCII DXF**, precision sufficient for round-trip,
   as `witness.dxf`. Use version choices offered by that product, record exact
   selections and warnings, and never overwrite existing files. If either
   export is unavailable, retain the failure and do not call the pair AC1024.
   Do not infer pairing merely from matching names or timestamps.
6. Open the saved DWG, record any errors and warnings, load the helper and run
   `DXFRWREPORT`. Verify the matching INSERT count is still one and its owner
   resolves; compare handle and owner against the original, noting any DXF
   remapping explicitly. Restore each saved state using AutoCAD's layer-state manager
   or `(layerstate-restore "DXFRW_DEFAULT" nil)` and
   `(layerstate-restore "DXFRW_EXPLICIT" nil)`; run `DXFRWREPORT` after each.
   Record displayed lineweights, view/layer-snapshot behavior and the block's
   visible draw order. Separately open the DXF and repeat the same observations.
   Do not resave either original over itself during these checks. A successful
   raw export without reopen and restoration is not semantic qualification.
7. Run the raw scanner (replace paths with the reserved directory):

   ```sh
   python3 objectarx_witness_scan.py --self-test
   python3 objectarx_witness_scan.py --dwg witness.dwg --dxf witness.dxf --out witness-observations.json
   ```

   Use `python` instead of `python3` if appropriate on Windows. The output is
   created exclusively and cannot overwrite an existing file. It contains
   SHA-256 hashes, DWG magic, reported DXF `$ACADVER`, and ordered raw groups
   for dictionaries, XRECORDs, block records, blocks and SORTENTSTABLEs. The
   scanner rejects binary DXF, malformed pairs and files over 64 MiB. Its
   cp1252 fallback is merely an unverified decoding choice, not a codepage
   determination. It is not a full DXF validator. Keep the transcript, both
   native files, observations and
   operator notes together; the files remain local until deliberately shared.

## Interpret and report

Identify the relevant layer-state dictionary and XRECORD through the recorded
handles and keys; report actual ordered group-370 values including `-3`, and
whether the *same* state restores correctly after opening each native format.
The DXF specification's generic XRECORD group range does not by itself assign
meaning to group 370. Also identify a SORTENTSTABLE's actual owner and 331/5
entries and compare block ownership and behavior across DWG and DXF. Raw
records and screenshots are separate evidence; neither proves DWG physical
field spans. Absence, export rejection, lost data or differing reopen behavior
are results and keep that profile unqualified. A named-block table or a key
without a live entity must be observed, not inferred from the documented API.
If available, provide exact AutoCAD/ObjectARX version and SDK/compiler versions
for a subsequent native API probe; no SDK build has been claimed here.

References: Autodesk [layerstate-save](https://help.autodesk.com/cloudhelp/2026/PTB/AutoCAD-LT-AutoLISP-Reference/files/GUID-05B5EA5C-2E7F-47A5-9C6F-27902DB60F30.htm),
[layerstate-restore](https://help.autodesk.com/cloudhelp/2015/CHS/AutoCAD-AutoLISP/files/GUID-84C7195D-12C1-4282-B5EE-8B9D6DDB705A.htm),
[entmake](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-AutoLISP-Reference/files/GUID-D47983BA-1E5D-417D-85B8-6F3DE5F506BA.htm),
[SORTENTSTABLE DXF](https://help.autodesk.com/cloudhelp/2024/ENU/AutoCAD-DXF/files/GUID-462F4378-F850-4E89-90F2-3C1880F55779.htm),
and [DrawOrderTable](https://help.autodesk.com/cloudhelp/2026/ENU/OARX-ManagedRefGuide/files/OARX-ManagedRefGuide-Autodesk_AutoCAD_DatabaseServices_DrawOrderTable.html).
