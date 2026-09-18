# DWG object type-code reference (empirical capture)

Captured by running `dwg2dxf -d` against in-tree samples and tallying the
`Remaining object Handle, loc, type=<N>` debug log values that fall through
the `readDwgObject` dispatch in
[dwgreader.cpp:1240](../src/intern/dwgreader.cpp#L1240).

Cross-referenced against:
- ODA Spec PDF sec 19 (fixed-class types 1-89).
- The per-file `classesmap` (`DRW_Class` records read by `readDwgClasses`)
  for non-fixed types ≥500.

## Fixed-class object types (universal across files)

These are stable per the ODA spec — safe to hardcode dispatch cases:

| oType | Spec name           | Class to dispatch | Plan band |
|-------|---------------------|-------------------|-----------|
| 42    | DICTIONARY          | `DRW_Dictionary`  | B2.4      |
| 73    | MLINESTYLE          | `DRW_MLineStyle`  | B2.5      |
| 79    | XRECORD             | `DRW_XRecord`     | B6        |
| 82    | LAYOUT              | `DRW_Layout`      | B2.3      |
| 89    | PLOTSETTINGS        | `DRW_PlotSettings` | B2.2 (already exists) |

## Non-fixed types (per-file CLASSES section, ≥500)

These vary per file. The class-name string in the file's `classesmap` is the
unambiguous identifier. Each must be looked up by name, not by number, before
any dispatch is added. Examples observed:

- `tablet.dwg`: 500=AcDbDictionaryWithDefault, 501=AcDbPlaceHolder,
  502=AcDbLayout, 503=AcDbDictionaryVar, 504=AcDbTableStyle,
  505=AcDbMaterial, 506=AcDbVisualStyle.
- `visualization_-_aerial.dwg`: 503=…, 505=…, 506=…, 510-512=… (different
  numbers for the same logical classes).

To dispatch on a non-fixed class:

```cpp
// In readDwgObject — find the class name for this oType
const auto cls_it = classesmap.find(oType);
if (cls_it != classesmap.end()) {
    const std::string& name = cls_it->second->dxfClassName;
    if (name == "AcDbLayout") {
        // dispatch to DRW_Layout fallback path
    }
}
```

## Per-sample frequency tables (raw capture, May 2026)

### `tests/samples/AC1021/tablet.dwg`

```
  96 type=10  (ATTDEF/ATTRIB - fixed)
  20 type=12  (BLOCK_HEADER - fixed, internal)
  16 type=506 (AcDbVisualStyle)
  15 type=42  ← DICTIONARY
   5 type=6
   3 type=82  ← LAYOUT
   3 type=79  ← XRECORD
   3 type=505 (AcDbMaterial)
   2 type=503 (AcDbDictionaryVar)
   1 type=73  ← MLINESTYLE
```

### `tests/samples/AC1021/blocks_and_tables_-_imperial.dwg`

```
 494 type=2
 477 type=42  ← DICTIONARY
 261 type=79  ← XRECORD
 142 type=508 (custom class)
 126 type=6
  71 type=520, 70 type=514, 17 type=538/537 (custom classes)
```

### `tests/samples/AC1024/visualization_-_aerial.dwg`

```
 131 type=79  ← XRECORD
  47 type=42  ← DICTIONARY
  32 type=512, 27 type=506 (AcDbVisualStyle), 12 type=518 (custom classes)
   9 type=505 (AcDbMaterial)
```

## Sync procedure (when adding a new sample)

1. Run `dwg2dxf -d <sample.dwg> -y -v2010 /tmp/x.dxf 2>&1 | grep "Remaining
   object" | awk '{print $NF}' | sort | uniq -c | sort -rn`.
2. For each new high-frequency type ≥500, find it in the same log via
   `grep "Class number: <N>"` and read the adjacent `class name:` line.
3. If the new sample contradicts the fixed-type mapping above (say, a
   tested file shows type=42 mapping to something other than
   AcDbDictionary in its classesmap), STOP and re-read the ODA spec — the
   mapping may be version-conditional.
