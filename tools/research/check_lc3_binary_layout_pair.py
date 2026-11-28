#!/usr/bin/env python3
"""Compare a bounded LibreCAD_3 binary layout source/replay with ezdxf.

This is a research control, not a producer or native-application qualification.
Requires ezdxf. Each path must be binary DXF unless its corresponding
--allow-ascii-source or --allow-ascii-replay option is specified. Versions
must match.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

import ezdxf
from ezdxf.lldxf.const import DXFError


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fields(entity, excluded):
    return {key: value for key, value in entity.dxf.all_existing_dxf_attribs().items()
            if key not in excluded}


def check_graph(doc):
    root = doc.rootdict
    layouts = root.get("ACAD_LAYOUT")
    require(layouts is not None and layouts.dxf.owner == root.dxf.handle,
            "layout dictionary is missing or has the wrong owner")
    plot = root.get("ACAD_PLOTSTYLENAME")
    require(plot is not None and plot.dxf.owner == root.dxf.handle,
            "plot-style dictionary is missing or has the wrong owner")
    normal = plot.get("Normal")
    require(normal is not None and normal.dxf.owner == plot.dxf.handle and
            plot.dxf.default == normal.dxf.handle, "Normal plot-style links differ")
    names = ("Model", "Layout1", "Layout2")
    require({key for key, _ in layouts.items()} == set(names), "layout names differ")
    for name in names:
        layout = doc.layouts.get(name)
        block = layout.block_record
        item = layouts.get(name)
        require(item.dxf.handle == layout.dxf_layout.dxf.handle and
                item.dxf.owner == layouts.dxf.handle and
                item.dxf.block_record_handle == block.dxf.handle and
                block.dxf.layout == item.dxf.handle and block.dxf.owner == "1",
                f"layout/block ownership or reciprocal links differ for {name}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("replay", type=Path)
    parser.add_argument("--allow-ascii-source", action="store_true")
    parser.add_argument("--allow-ascii-replay", action="store_true")
    args = parser.parse_args()
    sentinel = b"AutoCAD Binary DXF\r\n\x1a\x00"
    try:
        source_binary = args.source.read_bytes().startswith(sentinel)
        replay_binary = args.replay.read_bytes().startswith(sentinel)
        require(source_binary or args.allow_ascii_source,
                "ASCII source requires --allow-ascii-source")
        require(replay_binary or args.allow_ascii_replay,
                "ASCII replay requires --allow-ascii-replay")
        source, replay = ezdxf.readfile(args.source), ezdxf.readfile(args.replay)
        require(source.dxfversion == replay.dxfversion and
                source.dxfversion in {"AC1015", "AC1018", "AC1021", "AC1024", "AC1027"},
                "unsupported or mismatched version")
        for doc in (source, replay):
            audit = doc.audit()
            require(not audit.errors and not audit.fixes, "reader audit reports errors or fixes")
            check_graph(doc)
        counts = {}
        for name in ("Model", "Layout1", "Layout2"):
            old, new = source.layouts.get(name), replay.layouts.get(name)
            require(fields(old.dxf_layout, {"handle", "owner", "block_record_handle"}) ==
                    fields(new.dxf_layout, {"handle", "owner", "block_record_handle"}),
                    f"layout fields differ for {name}")
            require(fields(old.block_record, {"handle", "owner", "layout"}) ==
                    fields(new.block_record, {"handle", "owner", "layout"}),
                    f"block-record fields differ for {name}")
            def lines(layout):
                return [(tuple(line.dxf.start), tuple(line.dxf.end))
                        for line in layout.query("LINE")]
            old_lines, new_lines = lines(old), lines(new)
            require(old_lines == new_lines, f"LINE geometry differs for {name}")
            counts[name] = len(old_lines)
        print(json.dumps({"ok": True, "reader": f"ezdxf {ezdxf.__version__}",
                          "version": source.dxfversion, "source_encoding": "binary" if source_binary else "ascii",
                          "replay_encoding": "binary" if replay_binary else "ascii", "source_sha256": digest(args.source),
                          "replay_sha256": digest(args.replay), "line_counts": counts,
                          "audit_errors": 0, "audit_fixes": 0}, sort_keys=True))
    except (DXFError, OSError, ValueError, KeyError, AttributeError) as error:
        print(json.dumps({"ok": False, "error": str(error)}, sort_keys=True), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
