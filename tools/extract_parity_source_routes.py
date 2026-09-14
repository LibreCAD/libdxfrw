#!/usr/bin/env python3
"""Generate a deterministic, source-only LibreCAD/libdxfrw route inventory.

The target source is read exclusively from Git objects at the commit recorded
in ``metadata/libdxfrw-target-lock.json``.  The standalone side is read from
the current ``src/`` worktree and content-addressed, so the output detects
same-slice source changes without relying on a mutable checkout name or an
absolute path.  The result deliberately records dispatch/source routes only;
it is not a DXF/DWG support claim and it contains no drawing payloads.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
import subprocess
import sys
import tempfile
from dataclasses import dataclass, field
from functools import lru_cache
from pathlib import Path
from typing import Iterable


SCHEMA = 1
PARSER_SCHEMA = 9
MANIFEST_FIELDS = 4
SOURCE_PREFIX = "libraries/libdxfrw/"
SOURCE_ROOT = "libraries/libdxfrw/src"
TARGET_SOURCES_CMAKE = "libraries/libdxfrw/libdxfrw_sources.cmake"
HEX_RE = re.compile(r"^[0-9a-f]{40}$")

# These are source-derived anchors for the pinned target, not support claims.
# A target refresh must consciously update this contract after reviewing the
# changed source routes; silently producing a differently-shaped inventory is
# not implementation-ready evidence.
TARGET_ROUTE_COUNTS = {
    "dxfRW": {
        "dxf-block": 1,
        "dxf-class": 244,
        "dxf-class-required-group": 7,
        "dxf-entity": 62,
        "dxf-entity-dispatch-branch": 49,
        "dxf-object": 133,
        "dxf-object-dispatch-branch": 58,
        "dxf-object-predicate": 3,
        "dxf-section": 6,
        "dxf-section-dispatch-branch": 7,
        "dxf-table": 9,
        "dxf-table-dispatch-branch": 9,
        "dxf-transport-node": 16,
        "facade-method": 277,
        "group-code-raw-classifier": 1,
        "group-code-raw-rule": 33,
        "group-code-reader-range": 34,
        "public-header": 1,
        "public-inline-method": 25,
        "public-method": 132,
        "public-type": 5,
        "parser-publication": 211,
        "raw-flow": 27,
        "raw-route": 7,
        "writer-entrypoint": 110,
        "writer-version": 8,
    },
    "dwgRW": {
        "facade-method": 218,
        "fixed-entity": 52,
        "fixed-object": 19,
        "legacy-dwgr-facade": 1,
        "named-class-predicate": 2,
        "named-entity-class": 45,
        "named-object-class": 169,
        "object-context-class": 22,
        "parser-publication": 293,
        "public-alias": 1,
        "public-enum": 6,
        "public-header": 1,
        "public-inline-method": 13,
        "public-method": 207,
        "public-type": 9,
        "raw-custom-shell": 3,
        "raw-entity-shell": 20,
        "raw-flow": 23,
        "raw-object-shell": 6,
        "raw-route": 3,
        "reader-pipeline": 9,
        "reader-stage": 4,
        "reader-version": 19,
        "section": 17,
        "section-declaration": 21,
        "section-fallback": 1,
        "table-descriptor": 9,
        "writer-binding": 38,
        "writer-entrypoint": 101,
        "writer-pipeline": 7,
        "writer-version": 6,
    },
    "shared": {
        "callback": 151,
        "dwg-version-magic": 19,
        "helper-subsystem": 18,
        "internal-model-codec": 1,
        "interface-method": 171,
        "model-codec": 259,
        "pipeline-unit": 80,
        "public-enum": 55,
        "public-header": 8,
        "public-inline-function": 3,
        "public-inline-method": 610,
        "public-method": 702,
        "public-model": 259,
        "public-type": 303,
        "publication-stage": 24,
        "source-unit": 85,
        "version": 19,
    },
}

# Every pinned target ``src`` file has one explicit source-unit role.  This is
# deliberately an exact path contract rather than a filename glob: a target
# refresh that adds, removes, or repurposes a file must be reviewed before the
# inventory can claim source-surface closure.  ``functional`` units require
# method/pipeline edges in I0.2b; the two narrow supporting classes below are
# not allowed to hide an arbitrary implementation file behind a content hash.
SOURCE_UNIT_ROLES: dict[str, str] = {}


def _register_source_unit_role(role: str, *paths: str) -> None:
    for path in paths:
        if path in SOURCE_UNIT_ROLES:
            raise RuntimeError("duplicate source-unit role for %s" % path)
        SOURCE_UNIT_ROLES[path] = role


_register_source_unit_role(
    "model",
    "src/drw_acis.cpp", "src/drw_acis.h",
    "src/drw_base.cpp", "src/drw_base.h",
    "src/drw_classes.cpp", "src/drw_classes.h",
    "src/drw_datastorage.cpp", "src/drw_datastorage.h",
    "src/drw_entities.cpp", "src/drw_entities.h",
    "src/drw_header.cpp", "src/drw_header.h",
    "src/drw_interface.h",
    "src/drw_objects.cpp", "src/drw_objects.h",
)
_register_source_unit_role("public-support", "src/handle_allocator.h")
_register_source_unit_role(
    "generated-codepage-data",
    "src/intern/drw_cptable932.h", "src/intern/drw_cptable936.h",
    "src/intern/drw_cptable949.h", "src/intern/drw_cptable950.h",
)
_register_source_unit_role(
    "codec",
    "src/intern/drw_cptables.h", "src/intern/drw_textcodec.cpp",
    "src/intern/drw_textcodec.h", "src/intern/rscodec.cpp", "src/intern/rscodec.h",
)
_register_source_unit_role("diagnostics", "src/intern/drw_dbg.cpp", "src/intern/drw_dbg.h")
_register_source_unit_role("resource-limits", "src/intern/drw_reserve.h", "src/intern/dwgsafety.h")
_register_source_unit_role(
    "output-transaction",
    "src/intern/dwg_dxf_output_transaction.cpp", "src/intern/dwg_dxf_output_transaction.h",
    "src/intern/dwgwriterlayoutvalidation.cpp", "src/intern/dwgwriterlayoutvalidation.h",
)
_register_source_unit_role("dwg-handles", "src/intern/dwg_fixed_handles.h", "src/intern/dwgutil.cpp", "src/intern/dwgutil.h")
_register_source_unit_role(
    "dwg-buffer-frame",
    "src/intern/dwgbuffer.cpp", "src/intern/dwgbuffer.h",
    "src/intern/dwgbufferw.cpp", "src/intern/dwgbufferw.h",
    "src/intern/dwgobjectframe.cpp", "src/intern/dwgobjectframe.h",
)
_register_source_unit_role(
    "dwg-reader",
    "src/intern/dwgreader.cpp", "src/intern/dwgreader.h",
    "src/intern/dwgreader15.cpp", "src/intern/dwgreader15.h",
    "src/intern/dwgreader18.cpp", "src/intern/dwgreader18.h",
    "src/intern/dwgreader21.cpp", "src/intern/dwgreader21.h",
    "src/intern/dwgreader24.cpp", "src/intern/dwgreader24.h",
    "src/intern/dwgreader27.cpp", "src/intern/dwgreader27.h",
    "src/intern/dwgreader32.cpp", "src/intern/dwgreader32.h",
    "src/intern/dwgreaderR11.cpp", "src/intern/dwgreaderR11.h",
    "src/intern/dwgreaderR1_40.cpp", "src/intern/dwgreaderR1_40.h",
)
_register_source_unit_role(
    "dwg-writer",
    "src/intern/dwgwriter.h",
    "src/intern/dwgwriter15.cpp", "src/intern/dwgwriter15.h",
    "src/intern/dwgwriter18.cpp", "src/intern/dwgwriter18.h",
    "src/intern/dwgwriter21.cpp", "src/intern/dwgwriter21.h",
    "src/intern/dwgwriter24.cpp", "src/intern/dwgwriter24.h",
    "src/intern/dwgwriter27.cpp", "src/intern/dwgwriter27.h",
    "src/intern/dwgwriter32.cpp", "src/intern/dwgwriter32.h",
)
_register_source_unit_role("dxf-transport", "src/intern/dxfparserlimits.h", "src/intern/dxfreader.cpp", "src/intern/dxfreader.h", "src/intern/dxfwriter.cpp", "src/intern/dxfwriter.h")
_register_source_unit_role("proxy-acis", "src/intern/proxygraphicdecoder.cpp", "src/intern/proxygraphicdecoder.h")
_register_source_unit_role("dwg-facade", "src/libdwgr.cpp", "src/libdwgr.h")
_register_source_unit_role("dxf-facade", "src/libdxfrw.cpp", "src/libdxfrw.h")
_register_source_unit_role("documentation-only", "src/main_doc.h")

# The standalone has one reviewed compatibility helper absent from the pinned
# target.  Any further extra source path is rejected by the existing adaptation
# allowlist closure *and* this source-surface contract.
STANDALONE_SOURCE_UNIT_ROLES = {
    "src/intern/dxfcode.h": "standalone-compatibility",
}
SUPPORTING_SOURCE_UNIT_ROLES = {"generated-codepage-data", "documentation-only"}

# I0.2b closes the first-order execution graph for every functional source
# unit.  These labels identify an implementation area, not a claim that a
# particular file format feature is supported.  Keeping the table exact makes
# a new role fail closed until its pipeline placement is reviewed.
PIPELINE_STAGE_BY_SOURCE_ROLE = {
    "model": "model-codec",
    "public-support": "handle-allocation",
    "codec": "codec",
    "diagnostics": "diagnostics",
    "resource-limits": "resource-limits",
    "output-transaction": "output-transaction",
    "dwg-handles": "dwg-handles",
    "dwg-buffer-frame": "dwg-buffer-frame",
    "dwg-reader": "dwg-reader",
    "dwg-writer": "dwg-writer",
    "dxf-transport": "dxf-transport",
    "proxy-acis": "proxy-acis",
    "dwg-facade": "dwg-facade",
    "dxf-facade": "dxf-facade",
    "standalone-compatibility": "standalone-compatibility",
}

# Internal table-control parsing has no installed public declaration, but it
# is a real DWG model parse node.  Keep it explicit instead of silently
# excluding it from the direct-codec scan just because it is internal.
INTERNAL_MODEL_CODEC_TYPES = {"DRW_ObjControl"}

# Named helper routes make the cross-cutting components visible without
# reconstructing their source bodies.  A source file may appear in a model
# route and a helper route: these are deliberately different relationships.
HELPER_SUBSYSTEM_PATHS = {
    "codepage": (
        "src/intern/drw_cptables.h",
        "src/intern/drw_cptable932.h",
        "src/intern/drw_cptable936.h",
        "src/intern/drw_cptable949.h",
        "src/intern/drw_cptable950.h",
    ),
    "text-codec": ("src/intern/drw_textcodec.cpp", "src/intern/drw_textcodec.h"),
    "reeds-solomon": ("src/intern/rscodec.cpp", "src/intern/rscodec.h"),
    "dwg-compression": ("src/intern/dwgutil.cpp", "src/intern/dwgutil.h"),
    "dwg-buffer-read": ("src/intern/dwgbuffer.cpp", "src/intern/dwgbuffer.h"),
    "dwg-buffer-write": ("src/intern/dwgbufferw.cpp", "src/intern/dwgbufferw.h"),
    "dwg-object-frame": ("src/intern/dwgobjectframe.cpp", "src/intern/dwgobjectframe.h"),
    "handle-allocation": ("src/handle_allocator.h",),
    "fixed-handles": ("src/intern/dwg_fixed_handles.h",),
    "output-transaction": (
        "src/intern/dwg_dxf_output_transaction.cpp",
        "src/intern/dwg_dxf_output_transaction.h",
    ),
    "writer-layout-validation": (
        "src/intern/dwgwriterlayoutvalidation.cpp",
        "src/intern/dwgwriterlayoutvalidation.h",
    ),
    "diagnostic-hook": ("src/drw_base.cpp", "src/intern/drw_dbg.cpp", "src/intern/drw_dbg.h"),
    "reserve-bounds": ("src/intern/drw_reserve.h",),
    "dwg-safety": ("src/intern/dwgsafety.h",),
    "dxf-parser-limits": ("src/intern/dxfparserlimits.h",),
    "proxy-graphics": (
        "src/intern/proxygraphicdecoder.cpp",
        "src/intern/proxygraphicdecoder.h",
    ),
    "acis": ("src/drw_acis.cpp", "src/drw_acis.h"),
    "datastorage": ("src/drw_datastorage.cpp", "src/drw_datastorage.h"),
}

DXF_TRANSPORT_TYPES = (
    ("reader-base", "src/intern/dxfreader.h", "dxfReader"),
    ("reader-ascii", "src/intern/dxfreader.h", "dxfReaderAscii"),
    ("reader-binary", "src/intern/dxfreader.h", "dxfReaderBinary"),
    ("reader-binary-r12", "src/intern/dxfreader.h", "dxfReaderBinaryR12"),
    ("writer-base", "src/intern/dxfwriter.h", "dxfWriter"),
    ("writer-ascii", "src/intern/dxfwriter.h", "dxfWriterAscii"),
    ("writer-binary", "src/intern/dxfwriter.h", "dxfWriterBinary"),
    ("writer-binary-r12", "src/intern/dxfwriter.h", "dxfWriterBinaryR12"),
    ("writer-record-scope", "src/intern/dxfwriter.h", "DxfWriterRecordScope"),
)

DXF_TRANSPORT_SELECTIONS = (
    ("read-file-ascii", "dxfRW::read", "dxfReaderAscii", ("read",)),
    ("read-file-binary-r12", "dxfRW::read", "dxfReaderBinaryR12", ("read",)),
    ("read-file-binary", "dxfRW::read", "dxfReaderBinary", ("read",)),
    ("read-ascii-memory", "dxfRW::readAscii", "dxfReaderAscii", ("read",)),
    ("write-ascii", "dxfRW::write", "dxfWriterAscii", ("write",)),
    ("write-binary-r12", "dxfRW::write", "dxfWriterBinaryR12", ("write",)),
    ("write-binary", "dxfRW::write", "dxfWriterBinary", ("write",)),
)

# Each transport row carries its concrete branch predicate and constructor
# evidence.  ``branch`` records whether the constructor is selected by the
# predicate itself or by its direct ``else`` arm; this prevents a constructor
# name from being mistaken for a version/format decision.
DXF_TRANSPORT_SELECTION_RULES = {
    "read-file-ascii": {
        "guardPatterns": (r"std::memcmp\s*\(\s*line\s*,\s*line2\s*,\s*22\s*\)\s*==\s*0",),
        "construction": r"std::make_unique\s*<\s*dxfReaderAscii\s*>\s*\(\s*&filestr\s*\)",
        "branch": "else",
    },
    "read-file-binary-r12": {
        "guardPatterns": (r"std::memcmp\s*\(\s*line\s*,\s*line2\s*,\s*22\s*\)\s*==\s*0", r"static_cast<unsigned char>\s*\(\s*line\[23\]\s*\)\s*!=\s*0"),
        "construction": r"std::make_unique\s*<\s*dxfReaderBinaryR12\s*>\s*\(\s*&filestr\s*\)",
        "branch": "then",
    },
    "read-file-binary": {
        "guardPatterns": (r"std::memcmp\s*\(\s*line\s*,\s*line2\s*,\s*22\s*\)\s*==\s*0", r"static_cast<unsigned char>\s*\(\s*line\[23\]\s*\)\s*!=\s*0"),
        "construction": r"std::make_unique\s*<\s*dxfReaderBinary\s*>\s*\(\s*&filestr\s*\)",
        "branch": "else",
    },
    "read-ascii-memory": {
        "guardPatterns": (),
        "construction": r"std::make_unique\s*<\s*dxfReaderAscii\s*>\s*\(\s*&strstream\s*\)",
        "branch": "unconditional-entrypoint",
    },
    "write-ascii": {
        "guardPatterns": (r"\bif\s*\(\s*binFile\s*\)",),
        "construction": r"std::make_unique\s*<\s*dxfWriterAscii\s*>\s*\(&filestr\)",
        "branch": "else",
    },
    "write-binary-r12": {
        "guardPatterns": (r"\bif\s*\(\s*binFile\s*\)", r"version\s*<=\s*DRW::AC1009"),
        "construction": r"std::make_unique\s*<\s*dxfWriterBinaryR12\s*>\s*\(&filestr\)",
        "branch": "then",
    },
    "write-binary": {
        "guardPatterns": (r"\bif\s*\(\s*binFile\s*\)", r"version\s*<=\s*DRW::AC1009"),
        "construction": r"std::make_unique\s*<\s*dxfWriterBinary\s*>\s*\(&filestr\)",
        "branch": "else",
    },
}

DWG_READER_PIPELINES = {
    "base": {
        "reader": "dwgReader",
        "parent": None,
        "paths": ("src/intern/dwgreader.h", "src/intern/dwgreader.cpp"),
    },
    "r1_40": {
        "reader": "dwgReaderR1_40",
        "parent": "dwgReader",
        "paths": ("src/intern/dwgreaderR1_40.h", "src/intern/dwgreaderR1_40.cpp"),
    },
    "r11": {
        "reader": "dwgReaderR11",
        "parent": "dwgReader",
        "paths": ("src/intern/dwgreaderR11.h", "src/intern/dwgreaderR11.cpp"),
    },
    "15": {
        "reader": "dwgReader15",
        "parent": "dwgReader",
        "paths": ("src/intern/dwgreader15.h", "src/intern/dwgreader15.cpp"),
    },
    "18": {
        "reader": "dwgReader18",
        "parent": "dwgReader",
        "paths": ("src/intern/dwgreader18.h", "src/intern/dwgreader18.cpp"),
    },
    "21": {
        "reader": "dwgReader21",
        "parent": "dwgReader",
        "paths": ("src/intern/dwgreader21.h", "src/intern/dwgreader21.cpp"),
    },
    "24": {
        "reader": "dwgReader24",
        "parent": "dwgReader18",
        "paths": ("src/intern/dwgreader24.h", "src/intern/dwgreader24.cpp"),
    },
    "27": {
        "reader": "dwgReader27",
        "parent": "dwgReader18",
        "paths": ("src/intern/dwgreader27.h", "src/intern/dwgreader27.cpp"),
    },
    "32": {
        "reader": "dwgReader32",
        "parent": "dwgReader27",
        "paths": ("src/intern/dwgreader32.h", "src/intern/dwgreader32.cpp"),
    },
}

DWG_WRITER_PIPELINES = {
    "base": {
        "writer": "dwgWriter",
        "parent": None,
        "paths": ("src/intern/dwgwriter.h",),
    },
    "15": {
        "writer": "dwgWriter15",
        "parent": "dwgWriter",
        "paths": ("src/intern/dwgwriter15.h", "src/intern/dwgwriter15.cpp"),
    },
    "18": {
        "writer": "dwgWriter18",
        "parent": "dwgWriter15",
        "paths": ("src/intern/dwgwriter18.h", "src/intern/dwgwriter18.cpp"),
    },
    "21": {
        "writer": "dwgWriter21",
        "parent": "dwgWriter24",
        "paths": ("src/intern/dwgwriter21.h", "src/intern/dwgwriter21.cpp"),
    },
    "24": {
        "writer": "dwgWriter24",
        "parent": "dwgWriter18",
        "paths": ("src/intern/dwgwriter24.h", "src/intern/dwgwriter24.cpp"),
    },
    "27": {
        "writer": "dwgWriter27",
        "parent": "dwgWriter24",
        "paths": ("src/intern/dwgwriter27.h", "src/intern/dwgwriter27.cpp"),
    },
    "32": {
        "writer": "dwgWriter32",
        "parent": "dwgWriter27",
        "paths": ("src/intern/dwgwriter32.h", "src/intern/dwgwriter32.cpp"),
    },
}

# Raw-flow nodes are intentionally structural.  They record reviewed carrier
# handoffs and guards, never a claim that arbitrary opaque content can be
# replayed losslessly.  ``outputs`` names other atomic nodes in the same
# source-only graph; the extractor derives and validates the reciprocal input
# route IDs.  This makes a missing capture/publication/replay handoff visible
# instead of hiding it in a source-file-sized "raw" bucket.
RAW_FLOW_NODES = (
    # These read-side paths deliberately start at record-boundary functions,
    # not at common capture helpers.  The helpers are reused by many typed
    # parsers and do not themselves execute every raw eligibility check.
    {
        "flow": "dxf", "name": "dxf-read-raw-object-boundary", "phase": "capture", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawObject"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-object-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-object-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "requiresDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-object-raw-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-object-raw-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "hasRawDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-publish-object",),
    },
    {
        "flow": "dxf", "name": "dxf-read-raw-entity-boundary", "phase": "capture", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawEntity"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-entity-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-entity-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "requiresDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-entity-raw-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-entity-raw-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "hasRawDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-publish-entity",),
    },
    {
        "flow": "dxf", "name": "dxf-read-proxy-boundary", "phase": "capture", "facade": "dxfRW",
        "anchors": (
            ("src/libdxfrw.cpp", "dxfRW::processProxyEntity"),
            ("src/libdxfrw.cpp", "dxfRW::processProxyObject"),
        ),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-proxy-payload-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-proxy-payload-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "validateProxyDxfPayloads"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-proxy-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-proxy-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "requiresDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-read-proxy-raw-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-read-proxy-raw-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "hasRawDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-publish-proxy",),
    },
    {
        "flow": "dxf", "name": "dxf-read-typed-raw-template", "phase": "capture", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawCapturedObject"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-publish-typed-raw-carrier",),
    },
    {
        "flow": "dxf", "name": "dxf-read-raw-section-boundary", "phase": "capture", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawDxfSection"),),
        "carrier": "raw-dxf-section", "outputs": ("dxf-publish-section",),
    },
    {
        "flow": "dxf", "name": "dxf-ingress-raw-object-write-api", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::writeRawDxfObject"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-write-object-groups-gate",),
        "externalIngress": "caller-supplied-raw-dxf-object",
    },
    {
        "flow": "dxf", "name": "dxf-write-object-groups-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "validateRawDxfGroups"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-write-object-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-write-object-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "requiresDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-write-object-raw-self-handle-gate",),
    },
    {
        "flow": "dxf", "name": "dxf-write-object-raw-self-handle-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "hasRawDxfSelfHandle"),),
        "carrier": "raw-dxf-object", "outputs": ("dxf-replay-object",),
    },
    {
        "flow": "dxf", "name": "dxf-ingress-raw-section-setter", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.h", "dxfRW::setRawDxfSections"),),
        "carrier": "raw-dxf-section", "outputs": ("dxf-session-raw-section-buffer",),
        "externalIngress": "caller-supplied-raw-dxf-sections",
    },
    {
        "flow": "dxf", "name": "dxf-session-raw-section-buffer", "phase": "replay", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::write"),),
        "carrier": "raw-dxf-section", "outputs": ("dxf-replay-section",),
    },
    {
        "flow": "dxf", "name": "dxf-write-section-groups-gate", "phase": "eligibility", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "validateRawDxfGroups"),),
        "carrier": "raw-dxf-section", "outputs": ("dxf-replay-groups",),
    },
    {
        "flow": "dxf", "name": "dxf-publish-section", "phase": "publication", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawDxfSection"),),
        "carrier": "raw-dxf-section", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-setRawDxfSections-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dxf-ingress-raw-section-setter",),
    },
    {
        "flow": "dxf", "name": "dxf-publish-object", "phase": "publication", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawObject"),),
        "carrier": "raw-dxf-object", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDxfObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dxf-ingress-raw-object-write-api",),
    },
    {
        "flow": "dxf", "name": "dxf-publish-entity", "phase": "publication", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawEntity"),),
        "carrier": "raw-dxf-object", "publicationPlacement": "entity", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDxfObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dxf-ingress-raw-object-write-api",),
    },
    {
        "flow": "dxf", "name": "dxf-publish-proxy", "phase": "publication", "facade": "dxfRW",
        "anchors": (
            ("src/libdxfrw.cpp", "dxfRW::processProxyEntity"),
            ("src/libdxfrw.cpp", "dxfRW::processProxyObject"),
        ),
        "carrier": "raw-dxf-object", "publicationPlacement": "proxy", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDxfObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dxf-ingress-raw-object-write-api",),
    },
    {
        "flow": "dxf", "name": "dxf-publish-typed-raw-carrier", "phase": "publication", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::processRawCapturedObject"),),
        "carrier": "raw-dxf-object", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDxfObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dxf-ingress-raw-object-write-api",),
    },
    {
        "flow": "dxf", "name": "dxf-replay-groups", "phase": "replay", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::writeRawDxfGroups"),),
        "carrier": "raw-dxf-group", "outputs": (),
    },
    {
        "flow": "dxf", "name": "dxf-replay-object", "phase": "replay", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::writeRawDxfObject"),),
        "carrier": "raw-dxf-object", "outputs": (),
    },
    {
        "flow": "dxf", "name": "dxf-replay-section", "phase": "replay", "facade": "dxfRW",
        "anchors": (("src/libdxfrw.cpp", "dxfRW::writeRawDxfSection"),),
        "carrier": "raw-dxf-section", "outputs": ("dxf-write-section-groups-gate",),
    },
    {
        "flow": "dwg", "name": "dwg-capture-r2004", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader18.cpp", "dwgReader18::captureRawDwgDataSections"),),
        "carrier": "raw-dwg-section", "outputs": ("dwg-publish-section-carrier",),
        "readerPipelines": ("18", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-capture-r2007", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader21.cpp", "dwgReader21::captureRawDwgDataSections"),),
        "carrier": "raw-dwg-section", "outputs": ("dwg-publish-section-carrier",),
        "readerPipelines": ("21",),
    },
    {
        "flow": "dwg", "name": "dwg-capture-raw-entity-frame", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput::lambda(makeRawEntity)"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-publish-buffered-raw",),
    },
    {
        "flow": "dwg", "name": "dwg-capture-raw-control-frame", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader.cpp", "dwgReader::readDwgTables::lambda(addRawControl)"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-publish-buffered-raw",),
    },
    {
        "flow": "dwg", "name": "dwg-capture-raw-object-frame", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader.cpp", "dwgReader::readDwgObject::lambda(makeRawObject)"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-publish-direct-object",),
    },
    {
        "flow": "dwg", "name": "dwg-capture-datastorage-r2004", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader18.cpp", "dwgReader18::captureRawDwgDataSections"),),
        "carrier": "data-storage-index",
        "outputs": ("dwg-eligibility-datastorage-link", "dwg-publish-datastorage-index"),
        "readerPipelines": ("18", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-capture-datastorage-r2007", "phase": "capture", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader21.cpp", "dwgReader21::captureRawDwgDataSections"),),
        "carrier": "data-storage-index",
        "outputs": ("dwg-eligibility-datastorage-link", "dwg-publish-datastorage-index"),
        "readerPipelines": ("21",),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-datastorage-link", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader.cpp", "dwgReader::linkDataStorage"),),
        "carrier": "data-storage-index", "outputs": ("dwg-publish-datastorage-index",),
    },
    {
        "flow": "dwg", "name": "dwg-ingress-raw-object-write-api", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::writeRawDwgObject"),),
        "carrier": "raw-dwg-object",
        "outputs": (
            "dwg-eligibility-fixed-modeler", "dwg-eligibility-fixed-entity-shell",
            "dwg-eligibility-surface", "dwg-eligibility-custom",
            "dwg-replay-object",
        ),
        "externalIngress": "consumer-supplied-unsupported-object",
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-ingress-raw-object-class-registration-api", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::registerRawDwgObjectClass"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-replay-class-registration",),
        "externalIngress": "consumer-supplied-unsupported-object-class-registration",
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-fixed-modeler", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgwriter15.cpp", "isReplayableFixedModelerRawEntity"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-replay-object",),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-fixed-entity-shell", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgwriter15.cpp", "isReplayableFixedEntityShellRawEntity"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-replay-object",),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-surface", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgwriter15.cpp", "isReplayableSurfaceRawEntity"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-replay-object",),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-custom", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgwriter15.cpp", "isReplayableCustomRawEntity"),),
        "carrier": "raw-dwg-object",
        "outputs": ("dwg-eligibility-block-owner", "dwg-replay-object"),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-eligibility-block-owner", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/intern/dwgwriter15.cpp", "dwgWriter15::canRecordRawBlockOwnedEntity"),),
        "carrier": "raw-dwg-object", "outputs": ("dwg-replay-object",),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-publish-buffered-raw", "phase": "publication", "facade": "dwgRW",
        "anchors": (
            ("src/intern/dwgreader.cpp", "dwgReader::publishDeferredRawObjects"),
            ("src/intern/dwgreader.cpp", "dwgReader::publishDwgFramePublication"),
        ),
        "carrier": "raw-dwg-object", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDwgObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dwg-ingress-raw-object-write-api",),
    },
    {
        "flow": "dwg", "name": "dwg-publish-direct-object", "phase": "publication", "facade": "dwgRW",
        "anchors": (("src/intern/dwgreader.cpp", "dwgReader::readDwgObject"),),
        "carrier": "raw-dwg-object", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDwgObject-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dwg-ingress-raw-object-write-api",),
    },
    {
        "flow": "dwg", "name": "dwg-publish-section-carrier", "phase": "publication", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::processDwg"),),
        "carrier": "raw-dwg-section", "outputs": (),
        "terminalDisposition": "external-consumer-callback; replay-requires-explicit-writeRawDwgSection-on-a-write-session",
        "optionalCrossSessionHandoffs": ("dwg-ingress-raw-section-write-api",),
    },
    {
        "flow": "dwg", "name": "dwg-publish-datastorage-index", "phase": "publication", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::processDwg"),),
        "carrier": "data-storage-index", "outputs": (),
        "terminalDisposition": "external-consumer-callback; typed-index-not-written; opaque-source-bytes-may-be-replayed-through-explicit-parallel-raw-section",
        "requiresCompanionRawSectionCarrier": ("dwg-publish-section-carrier",),
    },
    {
        "flow": "dwg", "name": "dwg-ingress-raw-section-write-api", "phase": "eligibility", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::writeRawDwgSection"),),
        "carrier": "raw-dwg-section", "outputs": ("dwg-replay-section",),
        "externalIngress": "consumer-supplied-raw-dwg-section",
        "writerPipelines": ("18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-replay-class-registration", "phase": "replay", "facade": "dwgRW",
        "anchors": (("src/libdwgr.cpp", "dwgRW::registerRawDwgObjectClass"), ("src/intern/dwgwriter.h", "dwgWriter::registerRawObjectClass")),
        "carrier": "raw-dwg-object", "outputs": (),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-replay-object", "phase": "replay", "facade": "dwgRW",
        "anchors": (
            ("src/libdwgr.cpp", "dwgRW::writeRawDwgObject"),
            ("src/intern/dwgwriter15.cpp", "dwgWriter15::replayRawObject"),
            ("src/intern/dwgwriter15.cpp", "dwgWriter15::recordRawBlockOwnedEntity"),
        ),
        "carrier": "raw-dwg-object", "outputs": (),
        "writerPipelines": ("15", "18", "21", "24", "27", "32"),
    },
    {
        "flow": "dwg", "name": "dwg-replay-section", "phase": "replay", "facade": "dwgRW",
        "anchors": (
            ("src/libdwgr.cpp", "dwgRW::writeRawDwgSection"),
            ("src/intern/dwgwriter.h", "dwgWriter::addRawDwgSection"),
            ("src/intern/dwgwriter18.cpp", "dwgWriter18::addRawDwgSection"),
            ("src/intern/dwgwriter18.cpp", "dwgWriter18::finalize"),
            ("src/intern/dwgwriter21.cpp", "dwgWriter21::finalize"),
        ),
        "carrier": "raw-dwg-section", "outputs": (),
        "writerPipelines": ("18", "21", "24", "27", "32"),
    },
)

# Direction is a property of the concrete anchor, not its broad phase.  For
# example, validateRawDxfGroups is a write-time guard, while the similarly
# named captured-object validator is read-time.  A new node must consciously
# choose its own direction set here; phase alone is deliberately insufficient.
RAW_NODE_DIRECTIONS = {
    "dxf-read-raw-object-boundary": ("raw-capture", "read"),
    "dxf-read-object-self-handle-gate": ("read",),
    "dxf-read-object-raw-self-handle-gate": ("read",),
    "dxf-read-raw-entity-boundary": ("raw-capture", "read"),
    "dxf-read-entity-self-handle-gate": ("read",),
    "dxf-read-entity-raw-self-handle-gate": ("read",),
    "dxf-read-proxy-boundary": ("raw-capture", "read"),
    "dxf-read-proxy-payload-gate": ("read",),
    "dxf-read-proxy-self-handle-gate": ("read",),
    "dxf-read-proxy-raw-self-handle-gate": ("read",),
    "dxf-read-typed-raw-template": ("raw-capture", "read"),
    "dxf-read-raw-section-boundary": ("raw-capture", "read"),
    "dxf-ingress-raw-object-write-api": ("write",),
    "dxf-write-object-groups-gate": ("write",),
    "dxf-write-object-self-handle-gate": ("write",),
    "dxf-write-object-raw-self-handle-gate": ("write",),
    "dxf-ingress-raw-section-setter": ("write",),
    "dxf-session-raw-section-buffer": ("write",),
    "dxf-write-section-groups-gate": ("write",),
    "dxf-publish-section": ("publish", "read"),
    "dxf-publish-object": ("publish", "read"),
    "dxf-publish-entity": ("publish", "read"),
    "dxf-publish-proxy": ("publish", "read"),
    "dxf-publish-typed-raw-carrier": ("publish", "read"),
    "dxf-replay-groups": ("write",),
    "dxf-replay-object": ("write",),
    "dxf-replay-section": ("write",),
    "dwg-capture-r2004": ("raw-capture", "read"),
    "dwg-capture-r2007": ("raw-capture", "read"),
    "dwg-capture-raw-entity-frame": ("raw-capture", "read"),
    "dwg-capture-raw-control-frame": ("raw-capture", "read"),
    "dwg-capture-raw-object-frame": ("raw-capture", "read"),
    "dwg-capture-datastorage-r2004": ("raw-capture", "read"),
    "dwg-capture-datastorage-r2007": ("raw-capture", "read"),
    "dwg-eligibility-datastorage-link": ("read",),
    "dwg-ingress-raw-object-write-api": ("write",),
    "dwg-ingress-raw-object-class-registration-api": ("write",),
    "dwg-eligibility-fixed-modeler": ("write",),
    "dwg-eligibility-fixed-entity-shell": ("write",),
    "dwg-eligibility-surface": ("write",),
    "dwg-eligibility-custom": ("write",),
    "dwg-eligibility-block-owner": ("write",),
    "dwg-publish-buffered-raw": ("publish", "read"),
    "dwg-publish-direct-object": ("publish", "read"),
    "dwg-publish-section-carrier": ("publish", "read"),
    "dwg-publish-datastorage-index": ("publish", "read"),
    "dwg-ingress-raw-section-write-api": ("write",),
    "dwg-replay-class-registration": ("write",),
    "dwg-replay-object": ("write",),
    "dwg-replay-section": ("write",),
}

RAW_HANDOFF_SEMANTICS = "carrier-relationship-not-direct-call"
RAW_CROSS_SESSION_HANDOFF_SEMANTICS = (
    "optional-consumer-controlled-new-session-not-source-call"
)

# A differing carrier at a declared graph edge is never silently treated as
# identity.  The names are source-pipeline semantics only; they do not claim
# lossless conversion of arbitrary drawing payloads.
RAW_CARRIER_TRANSFORMS = {
    ("raw-dxf-group", "raw-dxf-object"): "group-to-current-object",
    ("raw-dxf-group", "raw-dxf-proxy"): "group-to-proxy-capture",
    ("raw-dxf-group", "raw-dxf-section"): "group-to-section-carrier",
    ("raw-dxf-group", "raw-dxf-entity"): "group-to-entity-carrier",
    ("raw-dxf-object", "raw-dxf-entity"): "object-to-entity-carrier",
    ("raw-dxf-object", "raw-dxf-proxy"): "object-to-proxy-carrier",
    ("raw-dxf-object", "raw-dxf-group"): "object-to-group-replay",
    ("raw-dxf-entity", "raw-dxf-group"): "entity-to-group-replay",
    ("raw-dxf-entity", "raw-dxf-object"): "entity-to-object-replay",
    ("raw-dxf-proxy", "raw-dxf-group"): "proxy-to-group-replay",
    ("raw-dxf-proxy", "raw-dxf-object"): "proxy-to-object-replay",
    ("raw-dxf-section", "raw-dxf-group"): "section-to-group-replay",
}

PARSER_PUBLICATION_CATEGORIES = {
    "dxfRW": {"dxf-block", "dxf-entity", "dxf-object", "dxf-section", "dxf-table"},
    "dwgRW": {"fixed-entity", "fixed-object", "named-entity-class", "named-object-class", "table-descriptor"},
}

# ``raw-route`` is intentionally broader than parser-publication: it also
# contains caller-facing write/replay anchors.  These read-side rules identify
# the five DXF carrier publication functions whose terminal callbacks must be
# proven separately from the structural raw-flow graph.  Keep the binding and
# callback names exact so a target refresh cannot silently turn a generic raw
# anchor into a publication claim.
RAW_DXF_SINGLE_PUBLICATION_RULES = {
    "dxf-publish-object": ("dxfRW::processRawObject", "DRW_RawDxfObject", "obj", "addRawDxfObject"),
    "dxf-publish-entity": ("dxfRW::processRawEntity", "DRW_RawDxfObject", "ent", "addRawDxfEntity"),
    "dxf-publish-section": ("dxfRW::processRawDxfSection", "DRW_RawDxfSection", "section", "addRawDxfSection"),
    "dxf-publish-typed-raw-carrier": (
        "dxfRW::processRawCapturedObject", "DRW_RawDxfObject", "raw", "addRawDxfObject"
    ),
}
RAW_DXF_PROXY_PUBLICATION_RULES = {
    "dxfRW::processProxyEntity": (
        "DRW_ProxyEntity", "entity", "addProxyEntity", "DRW_RawDxfObject", "raw", "addRawDxfEntity"
    ),
    "dxfRW::processProxyObject": (
        "DRW_ProxyObject", "object", "addProxyObject", "DRW_RawDxfObject", "raw", "addRawDxfObject"
    ),
}

# Read-side raw eligibility is a separate source-flow contract.  Every edge
# below is anchored at the concrete record function that executes it; the
# helper definition alone is never treated as a call-site proof.  Patterns are
# applied to masked C++ so comments and string literals cannot satisfy a gate.
RAW_DXF_ELIGIBILITY_RULES = {
    "dxf-read-raw-object-boundary": {
        "dxfRW::processRawObject": (
            {"name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec\s*\(\s*&code\s*\)\s*\)", "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)", "relation": "capture-loop"},
            {"name": "boundary-probe", "pattern": r"\bif\s*\(\s*0\s*==\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-probe"},
            {"name": "boundary-classifier", "pattern": r"\bsetEntityBoundary\s*\(\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-classify"},
            {"name": "boundary-error-reject", "pattern": r"boundary\s*==\s*DxfEntityBoundary::Error", "callee": "DxfEntityBoundary::Error", "calleeOverload": "enum-value", "relation": "reject"},
            {"name": "pair-limit", "pattern": r"\+\+\s*pairCount\s*>\s*DRW::kMaxDxfApplicationGroupPairs", "callee": "DRW::kMaxDxfApplicationGroupPairs", "calleeOverload": "constant-limit", "relation": "reject"},
            {"name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(\s*obj\s*,\s*code\s*,\s*true\s*\)", "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)", "relation": "capture"},
            {"name": "application-depth", "pattern": r"\bupdateRawDxfApplicationDepth\s*\(\s*obj\.groups\.back\s*\(\s*\)\s*,\s*applicationDepth\s*\)", "callee": "updateRawDxfApplicationDepth", "calleeOverload": "updateRawDxfApplicationDepth(const DRW_Variant&,int&)", "relation": "reject"},
        ),
    },
    "dxf-read-raw-entity-boundary": {
        "dxfRW::processRawEntity": (
            {"name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec\s*\(\s*&code\s*\)\s*\)", "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)", "relation": "capture-loop"},
            {"name": "boundary-probe", "pattern": r"\bif\s*\(\s*0\s*==\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-probe"},
            {"name": "boundary-classifier", "pattern": r"\bsetEntityBoundary\s*\(\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-classify"},
            {"name": "boundary-error-reject", "pattern": r"boundary\s*==\s*DxfEntityBoundary::Error", "callee": "DxfEntityBoundary::Error", "calleeOverload": "enum-value", "relation": "reject"},
            {"name": "entity-callback-boundary", "pattern": r"!\s*acceptEntityCallbackBoundary\s*\(\s*\)", "callee": "dxfRW::acceptEntityCallbackBoundary", "calleeOverload": "dxfRW::acceptEntityCallbackBoundary()", "relation": "reject"},
            {"name": "pair-limit", "pattern": r"\+\+\s*pairCount\s*>\s*DRW::kMaxDxfApplicationGroupPairs", "callee": "DRW::kMaxDxfApplicationGroupPairs", "calleeOverload": "constant-limit", "relation": "reject"},
            {"name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(\s*ent\s*,\s*code\s*,\s*true\s*\)", "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)", "relation": "capture"},
            {"name": "application-depth", "pattern": r"\bupdateRawDxfApplicationDepth\s*\(\s*ent\.groups\.back\s*\(\s*\)\s*,\s*applicationDepth\s*\)", "callee": "updateRawDxfApplicationDepth", "calleeOverload": "updateRawDxfApplicationDepth(const DRW_Variant&,int&)", "relation": "reject"},
        ),
    },
    "dxf-read-proxy-boundary": {
        "dxfRW::processProxyEntity": (
            {"name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec\s*\(\s*&code\s*\)\s*\)", "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)", "relation": "capture-loop"},
            {"name": "boundary-probe", "pattern": r"\bif\s*\(\s*code\s*==\s*0\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-probe"},
            {"name": "boundary-classifier", "pattern": r"\bsetEntityBoundary\s*\(\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-classify"},
            {"name": "boundary-error-reject", "pattern": r"boundary\s*==\s*DxfEntityBoundary::Error", "callee": "DxfEntityBoundary::Error", "calleeOverload": "enum-value", "relation": "reject"},
            {"name": "entity-callback-boundary", "pattern": r"!\s*acceptEntityCallbackBoundary\s*\(\s*\)", "callee": "dxfRW::acceptEntityCallbackBoundary", "calleeOverload": "dxfRW::acceptEntityCallbackBoundary()", "relation": "reject"},
            {"name": "application-depth", "pattern": r"capture\.applicationDepth\s*!=\s*0", "callee": "DxfProxyCapture::applicationDepth", "calleeOverload": "depth-state", "relation": "reject"},
            {"name": "payload-validation", "pattern": r"\bvalidateProxyDxfPayloads\s*\(\s*capture\s*\)", "callee": "validateProxyDxfPayloads", "calleeOverload": "validateProxyDxfPayloads(const DxfProxyCapture&)", "relation": "reject"},
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
            {"name": "pair-limit", "pattern": r"\+\+\s*pairCount\s*>\s*DRW::kMaxDxfApplicationGroupPairs", "callee": "DRW::kMaxDxfApplicationGroupPairs", "calleeOverload": "constant-limit", "relation": "reject"},
            {"name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(\s*raw\s*,\s*code\s*,\s*true\s*\)", "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)", "relation": "capture"},
            {"name": "proxy-payload-capture", "pattern": r"\bcollectProxyDxfGroup\s*\(\s*capture\s*,\s*raw\.groups\.back\s*\(\s*\)\s*,\s*true\s*\)", "callee": "collectProxyDxfGroup", "calleeOverload": "collectProxyDxfGroup(DxfProxyCapture&,const DRW_Variant&,bool)", "relation": "capture"},
        ),
        "dxfRW::processProxyObject": (
            {"name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec\s*\(\s*&code\s*\)\s*\)", "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)", "relation": "capture-loop"},
            {"name": "boundary-probe", "pattern": r"\bif\s*\(\s*code\s*==\s*0\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-probe"},
            {"name": "boundary-classifier", "pattern": r"\bsetEntityBoundary\s*\(\s*code\s*\)", "callee": "dxfRW::setEntityBoundary", "calleeOverload": "dxfRW::setEntityBoundary(int)", "relation": "boundary-classify"},
            {"name": "boundary-error-reject", "pattern": r"boundary\s*==\s*DxfEntityBoundary::Error", "callee": "DxfEntityBoundary::Error", "calleeOverload": "enum-value", "relation": "reject"},
            {"name": "end-block-reject", "pattern": r"boundary\s*==\s*DxfEntityBoundary::EndBlock", "callee": "DxfEntityBoundary::EndBlock", "calleeOverload": "enum-value", "relation": "reject"},
            {"name": "application-depth", "pattern": r"capture\.applicationDepth\s*!=\s*0", "callee": "DxfProxyCapture::applicationDepth", "calleeOverload": "depth-state", "relation": "reject"},
            {"name": "payload-validation", "pattern": r"\bvalidateProxyDxfPayloads\s*\(\s*capture\s*\)", "callee": "validateProxyDxfPayloads", "calleeOverload": "validateProxyDxfPayloads(const DxfProxyCapture&)", "relation": "reject"},
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
            {"name": "pair-limit", "pattern": r"\+\+\s*pairCount\s*>\s*DRW::kMaxDxfApplicationGroupPairs", "callee": "DRW::kMaxDxfApplicationGroupPairs", "calleeOverload": "constant-limit", "relation": "reject"},
            {"name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(\s*raw\s*,\s*code\s*,\s*true\s*\)", "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)", "relation": "capture"},
            {"name": "proxy-payload-capture", "pattern": r"\bcollectProxyDxfGroup\s*\(\s*capture\s*,\s*raw\.groups\.back\s*\(\s*\)\s*,\s*false\s*\)", "callee": "collectProxyDxfGroup", "calleeOverload": "collectProxyDxfGroup(DxfProxyCapture&,const DRW_Variant&,bool)", "relation": "capture"},
        ),
    },
    "dxf-read-object-self-handle-gate": {
        "dxfRW::processRawObject": (
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-object-raw-self-handle-gate": {
        "dxfRW::processRawObject": (
            {"name": "raw-self-handle-present", "pattern": r"\bhasRawDxfSelfHandle\s*\(\s*obj\s*\)", "callee": "hasRawDxfSelfHandle", "calleeOverload": "hasRawDxfSelfHandle(const DRW_RawDxfObject&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-entity-self-handle-gate": {
        "dxfRW::processRawEntity": (
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-entity-raw-self-handle-gate": {
        "dxfRW::processRawEntity": (
            {"name": "raw-self-handle-present", "pattern": r"\bhasRawDxfSelfHandle\s*\(\s*ent\s*\)", "callee": "hasRawDxfSelfHandle", "calleeOverload": "hasRawDxfSelfHandle(const DRW_RawDxfObject&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-proxy-payload-gate": {
        "dxfRW::processProxyEntity": (
            {"name": "payload-validation", "pattern": r"\bvalidateProxyDxfPayloads\s*\(\s*capture\s*\)", "callee": "validateProxyDxfPayloads", "calleeOverload": "validateProxyDxfPayloads(const DxfProxyCapture&)", "relation": "reject"},
        ),
        "dxfRW::processProxyObject": (
            {"name": "payload-validation", "pattern": r"\bvalidateProxyDxfPayloads\s*\(\s*capture\s*\)", "callee": "validateProxyDxfPayloads", "calleeOverload": "validateProxyDxfPayloads(const DxfProxyCapture&)", "relation": "reject"},
        ),
    },
    "dxf-read-proxy-self-handle-gate": {
        "dxfRW::processProxyEntity": (
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
        ),
        "dxfRW::processProxyObject": (
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*\*reader\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(const dxfReader&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-proxy-raw-self-handle-gate": {
        "dxfRW::processProxyEntity": (
            {"name": "raw-self-handle-present", "pattern": r"\bhasRawDxfSelfHandle\s*\(\s*raw\s*\)", "callee": "hasRawDxfSelfHandle", "calleeOverload": "hasRawDxfSelfHandle(const DRW_RawDxfObject&)", "relation": "conditional-reject"},
        ),
        "dxfRW::processProxyObject": (
            {"name": "raw-self-handle-present", "pattern": r"\bhasRawDxfSelfHandle\s*\(\s*raw\s*\)", "callee": "hasRawDxfSelfHandle", "calleeOverload": "hasRawDxfSelfHandle(const DRW_RawDxfObject&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-read-raw-section-boundary": {
        "dxfRW::processRawDxfSection": (
            {"name": "section-name-required", "pattern": r"\bsectionName\.empty\s*\(\s*\)", "callee": "std::string::empty", "calleeOverload": "std::string::empty()", "relation": "reject"},
            {"name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec\s*\(\s*&code\s*\)\s*\)", "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)", "relation": "capture-loop"},
            {"name": "pair-limit", "pattern": r"\+\+\s*pairCount\s*>\s*DRW::kMaxDxfApplicationGroupPairs", "callee": "DRW::kMaxDxfApplicationGroupPairs", "calleeOverload": "constant-limit", "relation": "reject"},
            {"name": "end-section-boundary", "pattern": r"\bcode\s*==\s*0\s*&&\s*dxfKeywordEquals\s*\(\s*reader->getString\s*\(\s*\)\s*,\s*\"ENDSEC\"\s*\)", "callee": "dxfKeywordEquals", "calleeOverload": "dxfKeywordEquals(const std::string&,const char*)", "relation": "boundary-probe"},
            {"name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(\s*group\s*,\s*code\s*,\s*true\s*\)", "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)", "relation": "capture"},
            {"name": "application-depth", "pattern": r"\bupdateRawDxfApplicationDepth\s*\(\s*group\.groups\.back\s*\(\s*\)\s*,\s*applicationDepth\s*\)", "callee": "updateRawDxfApplicationDepth", "calleeOverload": "updateRawDxfApplicationDepth(const DRW_Variant&,int&)", "relation": "reject"},
        ),
    },
}

RAW_DXF_WRITER_EVIDENCE_RULES = {
    "dxf-write-object-groups-gate": {
        "dxfRW::writeRawDxfObject": (
            {"name": "group-validation", "pattern": r"\bvalidateRawDxfGroups\s*\(\s*obj->groups\s*,\s*obj->rawValues", "callee": "validateRawDxfGroups", "calleeOverload": "validateRawDxfGroups(const vector<DRW_Variant>&,const vector<UTF8STRING>&,bool,bool,bool)", "relation": "reject"},
        ),
    },
    "dxf-write-object-self-handle-gate": {
        "dxfRW::writeRawDxfObject": (
            {"name": "self-handle-required", "pattern": r"\brequiresDxfSelfHandle\s*\(\s*version\s*\)", "callee": "requiresDxfSelfHandle", "calleeOverload": "requiresDxfSelfHandle(DRW::Version)", "relation": "conditional-reject"},
            {"name": "raw-self-handle-present", "pattern": r"\bhasRawDxfSelfHandle\s*\(\s*\*obj\s*\)", "callee": "hasRawDxfSelfHandle", "calleeOverload": "hasRawDxfSelfHandle(const DRW_RawDxfObject&)", "relation": "conditional-reject"},
        ),
    },
    "dxf-replay-object": {
        "dxfRW::writeRawDxfObject": (
            {"name": "group-replay", "pattern": r"\bwriteRawDxfGroups\s*\(\s*obj->groups\s*,\s*obj->rawValues", "callee": "dxfRW::writeRawDxfGroups", "calleeOverload": "writeRawDxfGroups(const vector<DRW_Variant>&,const vector<UTF8STRING>&,bool,DRW::Version,bool)", "relation": "raw-object-to-groups"},
        ),
    },
    "dxf-write-section-groups-gate": {
        "dxfRW::writeRawDxfSection": (
            {"name": "group-validation", "pattern": r"\bvalidateRawDxfGroups\s*\(\s*section\.m_groups\s*,\s*section\.m_rawValues", "callee": "validateRawDxfGroups", "calleeOverload": "validateRawDxfGroups(const vector<DRW_Variant>&,const vector<UTF8STRING>&,bool,bool,bool)", "relation": "reject"},
        ),
    },
    "dxf-replay-section": {
        "dxfRW::writeRawDxfSection": (
            {"name": "group-replay", "pattern": r"\bwriteRawDxfGroups\s*\(\s*section\.m_groups\s*,\s*section\.m_rawValues", "callee": "dxfRW::writeRawDxfGroups", "calleeOverload": "writeRawDxfGroups(const vector<DRW_Variant>&,const vector<UTF8STRING>&,bool,DRW::Version,bool)", "relation": "raw-section-to-groups"},
        ),
    },
}

# DWG raw replay is deliberately a separate contract from the DXF writer
# guards above.  The target implementation has a façade ingress, a writer15
# replay gate, an internal class-registration transaction, and a custom
# block-owned entity bookkeeping branch.  Keep these edges ordered so a
# seemingly harmless guard move cannot silently change which raw objects are
# accepted or how their class/owner state is committed.
DWG_RAW_REPLAY_EVIDENCE_RULES = {
    "dwg-replay-object": {
        "src/libdwgr.cpp": {
            "dwgRW::writeRawDwgObject": (
                {"name": "null-object-guard", "pattern": r"if\s*\(\s*object\s*==\s*nullptr\s*\)", "callee": "object == nullptr", "calleeOverload": "null-check", "relation": "reject"},
                {"name": "writer-type-guard", "pattern": r"auto\s*\*w\s*=\s*asWriter15\s*\(\s*writer\s*\)", "callee": "asWriter15", "calleeOverload": "asWriter15(dwgWriter*)", "relation": "writer-selection"},
                {"name": "replay-call", "pattern": r"\bw->replayRawObject\s*\(\s*\*object\s*\)", "callee": "dwgWriter15::replayRawObject", "calleeOverload": "replayRawObject(const DRW_UnsupportedObject&)", "relation": "raw-object-to-writer"},
                {"name": "rollback-on-failure", "pattern": r"\bw->rollbackRawObjectClassInstance\s*\(\s*\*object\s*\)", "callee": "dwgWriter15::rollbackRawObjectClassInstance", "calleeOverload": "rollbackRawObjectClassInstance(const DRW_UnsupportedObject&)", "relation": "failure-rollback"},
            ),
        },
        "src/intern/dwgwriter15.cpp": {
            "dwgWriter15::replayRawObject": (
                {"name": "block-control-entity-reject", "pattern": r"blockControlEmitted\s*\(\s*\)\s*&&\s*object\.m_isEntity", "callee": "blockControlEmitted", "calleeOverload": "blockControlEmitted()", "relation": "reject"},
                {"name": "version-match-reject", "pattern": r"object\.m_version\s*!=\s*m_version", "callee": "object.m_version != m_version", "calleeOverload": "version-compatibility-predicate", "relation": "reject"},
                {"name": "fixed-modeler-eligibility", "pattern": r"!isReplayableFixedModelerRawEntity\s*\(\s*object\s*\)", "callee": "isReplayableFixedModelerRawEntity", "calleeOverload": "isReplayableFixedModelerRawEntity(const DRW_UnsupportedObject&)", "relation": "eligibility"},
                {"name": "fixed-shell-eligibility", "pattern": r"!isReplayableFixedEntityShellRawEntity\s*\(\s*object\s*\)", "callee": "isReplayableFixedEntityShellRawEntity", "calleeOverload": "isReplayableFixedEntityShellRawEntity(const DRW_UnsupportedObject&)", "relation": "eligibility"},
                {"name": "surface-eligibility", "pattern": r"!isReplayableSurfaceRawEntity\s*\(\s*object\s*\)", "callee": "isReplayableSurfaceRawEntity", "calleeOverload": "isReplayableSurfaceRawEntity(const DRW_UnsupportedObject&)", "relation": "eligibility"},
                {"name": "custom-eligibility", "pattern": r"!isReplayableCustomRawEntity\s*\(\s*object\s*\)", "callee": "isReplayableCustomRawEntity", "calleeOverload": "isReplayableCustomRawEntity(const DRW_UnsupportedObject&)", "relation": "eligibility"},
                {"name": "zero-handle-reject", "pattern": r"object\.m_handle\s*==\s*0", "callee": "object.m_handle == 0", "calleeOverload": "handle-presence-predicate", "relation": "reject"},
                {"name": "empty-raw-bytes-reject", "pattern": r"object\.m_rawBytes\.empty\s*\(\s*\)", "callee": "object.m_rawBytes.empty", "calleeOverload": "vector::empty()", "relation": "reject"},
                {"name": "body-bit-size-bound", "pattern": r"static_cast<std::uint64_t>\(object\.m_bodyBitSize\)\s*>\s*static_cast<std::uint64_t>\(object\.m_rawBytes\.size\s*\(\)\s*\)\s*\*\s*8u", "callee": "object.m_bodyBitSize", "calleeOverload": "raw-body-bit-size-bound", "relation": "reject"},
                {"name": "object-size-match", "pattern": r"object\.m_objectSize\s*!=\s*0", "callee": "object.m_objectSize", "calleeOverload": "raw-object-size-compatibility", "relation": "reject"},
                {"name": "duplicate-handle-reject", "pattern": r"entry\.first\s*==\s*object\.m_handle", "callee": "m_objectMap", "calleeOverload": "object-handle-uniqueness", "relation": "reject"},
                {"name": "custom-owner-reject", "pattern": r"object\.m_blockOwnerHandle\s*!=\s*DRW::NoHandle\s*&&\s*object\.m_parentHandle\s*==\s*DRW::NoHandle", "callee": "raw block-owner relationship", "calleeOverload": "custom-entity-owner-compatibility", "relation": "reject"},
                {"name": "block-owner-preflight", "pattern": r"canRecordRawBlockOwnedEntity\s*\(\s*object\s*\)", "callee": "canRecordRawBlockOwnedEntity", "calleeOverload": "canRecordRawBlockOwnedEntity(const DRW_UnsupportedObject&)", "relation": "conditional-reject"},
                {"name": "class-registration", "pattern": r"registerRawObjectClass\s*\(\s*object\s*\)", "callee": "registerRawObjectClass", "calleeOverload": "registerRawObjectClass(const DRW_UnsupportedObject&)", "relation": "transaction-begin"},
                {"name": "class-provenance-match", "pattern": r"m_currentDwgObjectFrameProvenance\.classNumber\s*!=\s*writerType", "callee": "m_currentDwgObjectFrameProvenance.classNumber", "calleeOverload": "raw-frame-provenance", "relation": "reject"},
                {"name": "raw-handle-decode", "pattern": r"readRawObjectHandle\s*\(\s*\*bodyBytes\s*,\s*m_version\s*,\s*encodedHandle\s*\)", "callee": "readRawObjectHandle", "calleeOverload": "readRawObjectHandle(const vector<uint8_t>&,DRW::Version,dwgHandle&)", "relation": "raw-frame-parse"},
                {"name": "body-type-match", "pattern": r"actualType\s*!=\s*writerType", "callee": "dwgBuffer::getObjType", "calleeOverload": "getObjType(DRW::Version)", "relation": "reject"},
                {"name": "block-owner-record", "pattern": r"recordRawBlockOwnedEntity\s*\(\s*object\s*\)", "callee": "recordRawBlockOwnedEntity", "calleeOverload": "recordRawBlockOwnedEntity(const DRW_UnsupportedObject&)", "relation": "commit-owner-bookkeeping"},
            ),
            "dwgWriter15::recordRawBlockOwnedEntity": (
                {"name": "block-owner-preflight", "pattern": r"canRecordRawBlockOwnedEntity\s*\(\s*object\s*\)", "callee": "canRecordRawBlockOwnedEntity", "calleeOverload": "canRecordRawBlockOwnedEntity(const DRW_UnsupportedObject&)", "relation": "conditional-reject"},
                {"name": "space-owner-append", "pattern": r"handles\.push_back\s*\(\s*object\.m_handle\s*\)", "callee": "entityHandles", "calleeOverload": "owned-entity-handle-vector", "relation": "commit-owner-bookkeeping"},
                {"name": "user-block-owner-lookup", "pattern": r"std::find_if\s*\(\s*m_userBlocks\.begin", "callee": "m_userBlocks", "calleeOverload": "pending-user-block-lookup", "relation": "owner-selection"},
                {"name": "user-block-owner-append", "pattern": r"owner->entityHandles\.push_back\s*\(\s*object\.m_handle\s*\)", "callee": "PendingUserBlock::entityHandles", "calleeOverload": "owned-entity-handle-vector", "relation": "commit-owner-bookkeeping"},
            ),
        },
    },
    "dwg-replay-section": {
        "src/libdwgr.cpp": {
            "dwgRW::writeRawDwgSection": (
                {"name": "null-section-or-writer-guard", "pattern": r"section\s*==\s*nullptr\s*\|\|\s*writer\s*==\s*nullptr", "callee": "section/writer null checks", "calleeOverload": "raw-section-ingress-guard", "relation": "reject"},
                {"name": "section-buffer-call", "pattern": r"writer->addRawDwgSection\s*\(\s*\*section\s*\)", "callee": "dwgWriter::addRawDwgSection", "calleeOverload": "addRawDwgSection(const DRW_RawDwgSection&)", "relation": "raw-section-to-writer"},
            ),
        },
        "src/intern/dwgwriter.h": {
            "dwgWriter::addRawDwgSection": (
                {"name": "base-section-discard", "pattern": r"\(void\)section", "callee": "section", "calleeOverload": "base-writer-default-hook", "relation": "unsupported-default"},
                {"name": "base-section-reject", "pattern": r"return\s+false", "callee": "false", "calleeOverload": "base-writer-default-hook", "relation": "unsupported-default"},
            ),
        },
        "src/intern/dwgwriter18.cpp": {
            "dwgWriter18::addRawDwgSection": (
                {"name": "supported-name-classification", "pattern": r"const\s+bool\s+supportedName\s*=", "callee": "supportedName", "calleeOverload": "raw-section-name-classification", "relation": "eligibility"},
                {"name": "standard-name-classification", "pattern": r"const\s+bool\s+standardName\s*=", "callee": "standardName", "calleeOverload": "raw-section-name-classification", "relation": "eligibility"},
                {"name": "opaque-name-classification", "pattern": r"const\s+bool\s+opaqueName\s*=", "callee": "opaqueName", "calleeOverload": "raw-section-name-classification", "relation": "eligibility"},
                {"name": "prototype-version-gate", "pattern": r"const\s+bool\s+prototypeAllowed\s*=", "callee": "prototypeAllowed", "calleeOverload": "raw-section-version-gate", "relation": "conditional-reject"},
                {"name": "vba-version-gate", "pattern": r"const\s+bool\s+vbaAllowed\s*=", "callee": "vbaAllowed", "calleeOverload": "raw-section-version-gate", "relation": "conditional-reject"},
                {"name": "name-version-reject", "pattern": r"if\s*\(\s*\(!supportedName\s*&&\s*!opaqueName\s*\)\s*\|\|\s*section\.m_version\s*!=\s*m_version", "callee": "supportedName/opaqueName/m_version", "calleeOverload": "raw-section-name-version-gate", "relation": "reject"},
                {"name": "encoding-reject", "pattern": r"section\.m_encoding\s*!=\s*1\s*&&\s*section\.m_encoding\s*!=\s*2\s*&&\s*section\.m_encoding\s*!=\s*4", "callee": "section.m_encoding", "calleeOverload": "raw-section-encoding-gate", "relation": "reject"},
                {"name": "encryption-reject", "pattern": r"section\.m_encrypted\s*!=\s*0", "callee": "section.m_encrypted", "calleeOverload": "raw-section-encryption-gate", "relation": "reject"},
                {"name": "payload-size-reject", "pattern": r"section\.m_data\.size\s*\(\)\s*>\s*UINT32_MAX", "callee": "section.m_data.size", "calleeOverload": "raw-section-size-gate", "relation": "reject"},
                {"name": "duplicate-section-reject", "pattern": r"existing\.m_name\s*==\s*section\.m_name", "callee": "m_rawDwgSections", "calleeOverload": "raw-section-name-uniqueness", "relation": "reject"},
                {"name": "section-buffer-append", "pattern": r"m_rawDwgSections\.push_back\s*\(\s*section\s*\)", "callee": "m_rawDwgSections", "calleeOverload": "raw-section-buffer", "relation": "commit-buffer"},
            ),
            "dwgWriter18::finalize": (
                {"name": "finalizer-entry-guard", "pattern": r"objectWriteFailed\s*\(\s*\)\s*\|\|\s*m_stream\s*==\s*nullptr\s*\|\|\s*!m_stream->good\s*\(\s*\)", "callee": "objectWriteFailed/m_stream", "calleeOverload": "finalizer-entry-guard", "relation": "reject"},
                {"name": "raw-section-finalize-loop", "pattern": r"for\s*\(\s*const\s+DRW_RawDwgSection&\s+section\s+:\s*m_rawDwgSections\s*\)", "callee": "m_rawDwgSections", "calleeOverload": "raw-section-finalizer-loop", "relation": "flush"},
                {"name": "raw-section-page-append", "pattern": r"appendDataSection\s*\(\s*section\.m_name", "callee": "appendDataSection", "calleeOverload": "appendDataSection(string,bytes,uint32_t,...)", "relation": "flush"},
            ),
        },
        "src/intern/dwgwriter21.cpp": {
            "dwgWriter21::finalize": (
                {"name": "finalizer-entry-guard", "pattern": r"m_writeError\s*\|\|\s*m_stream\s*==\s*nullptr\s*\|\|\s*!m_stream->good\s*\(\s*\)", "callee": "m_writeError/m_stream", "calleeOverload": "finalizer-entry-guard", "relation": "reject"},
                {"name": "raw-section-finalize-loop", "pattern": r"for\s*\(const\s+DRW_RawDwgSection&\s+section\s+:\s*m_rawDwgSections\s*\)", "callee": "m_rawDwgSections", "calleeOverload": "raw-section-finalizer-loop", "relation": "flush"},
                {"name": "raw-section-page-append", "pattern": r"appendSection\s*\(\s*section\.m_name\.c_str", "callee": "appendSection", "calleeOverload": "appendSection(const char*,bytes,uint32_t,...)", "relation": "flush"},
            ),
        },
    },
}

# One ordered receipt/parse/map chain per table descriptor.  The block table
# intentionally remains a separate descriptor row: its map feeds the later
# BLOCK/ENDBLK ownership walk rather than a normal façade table callback.
DWG_TABLE_DELIVERY_FIELDS = {
    "kLTypeTable": ("ltControl", "lt", "ltypemap", "linetype"),
    "kLayerTable": ("layControl", "la", "layermap", "layer"),
    "kStyleTable": ("styControl", "sty", "stylemap", "text style"),
    "kDimStyleTable": ("dimstyControl", "sty", "dimstylemap", "dimension style"),
    "kVPortTable": ("vportControl", "vp", "vportmap", "viewport"),
    "kBlockTable": ("blockControl", "br", "blockRecordmap", "block record"),
    "kAppIdTable": ("appIdControl", "ai", "appIdmap", "AppId"),
    "kViewTable": ("viewControl", "vw", "viewmap", "view"),
    "kUcsTable": ("ucsControl", "u", "ucsmap", "UCS"),
}

# Compound entity case arms are source-owned transitions, not ordinary
# `entryParse` publications.  Each route retains the concrete helper call(s)
# selected by the ATTRIB/SEQEND, INSERT/MINSERT, VERTEX, and POLYLINE cases;
# the deeper helper state machines remain independently reviewable.
DWG_COMPOUND_TRANSITION_ANCHORS = {
    "dwgType::ATTRIB": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "attrib-stage", "pattern": r"stagePendingAttribute\s*\(\s*std::move\s*\(a\)\s*,\s*attribPublication\s*,\s*intfa\s*\)", "callee": "stagePendingAttribute", "calleeOverload": "stagePendingAttribute(shared_ptr<DRW_Attrib>,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::SEQEND": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "seqend-stage", "pattern": r"stagePendingSeqEnd\s*\(\s*obj\.handle\s*,\s*sequenceEnd\.parentHandle", "callee": "stagePendingSeqEnd", "calleeOverload": "stagePendingSeqEnd(handle,owner,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::INSERT": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "mapped-insert-stage", "pattern": r"stageMappedInsertAggregate\s*\(\s*std::move\s*\(e\)\s*,\s*insertPublication", "callee": "stageMappedInsertAggregate", "calleeOverload": "stageMappedInsertAggregate(DRW_Insert&&,publication,...)", "relation": "versioned-compound-stage"},
            {"name": "legacy-insert-stage", "pattern": r"stageLegacyInsertAggregate\s*\(\s*std::move\s*\(e\)\s*,\s*insertPublication", "callee": "stageLegacyInsertAggregate", "calleeOverload": "stageLegacyInsertAggregate(DRW_Insert&&,publication,...)", "relation": "versioned-compound-stage"},
        )),
    ),
    "dwgType::MINSERT": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "mapped-insert-stage", "pattern": r"stageMappedInsertAggregate\s*\(\s*std::move\s*\(e\)\s*,\s*insertPublication", "callee": "stageMappedInsertAggregate", "calleeOverload": "stageMappedInsertAggregate(DRW_Insert&&,publication,...)", "relation": "versioned-compound-stage"},
            {"name": "legacy-insert-stage", "pattern": r"stageLegacyInsertAggregate\s*\(\s*std::move\s*\(e\)\s*,\s*insertPublication", "callee": "stageLegacyInsertAggregate", "calleeOverload": "stageLegacyInsertAggregate(DRW_Insert&&,publication,...)", "relation": "versioned-compound-stage"},
        )),
    ),
    "dwgType::VERTEX_2D": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "vertex-stage", "pattern": r"stagePendingPolylineVertex\s*\(\s*std::move\s*\(vertex\)\s*,\s*vertexPublication\s*,\s*intfa\s*\)", "callee": "stagePendingPolylineVertex", "calleeOverload": "stagePendingPolylineVertex(DRW_Vertex&&,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::VERTEX_3D": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "vertex-stage", "pattern": r"stagePendingPolylineVertex\s*\(\s*std::move\s*\(vertex\)\s*,\s*vertexPublication\s*,\s*intfa\s*\)", "callee": "stagePendingPolylineVertex", "calleeOverload": "stagePendingPolylineVertex(DRW_Vertex&&,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::VERTEX_MESH": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "vertex-stage", "pattern": r"stagePendingPolylineVertex\s*\(\s*std::move\s*\(vertex\)\s*,\s*vertexPublication\s*,\s*intfa\s*\)", "callee": "stagePendingPolylineVertex", "calleeOverload": "stagePendingPolylineVertex(DRW_Vertex&&,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::VERTEX_PFACE": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "vertex-stage", "pattern": r"stagePendingPolylineVertex\s*\(\s*std::move\s*\(vertex\)\s*,\s*vertexPublication\s*,\s*intfa\s*\)", "callee": "stagePendingPolylineVertex", "calleeOverload": "stagePendingPolylineVertex(DRW_Vertex&&,publication,interface)", "relation": "compound-stage"},
        )),
    ),
    "dwgType::VERTEX_PFACE_FACE": (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "vertex-stage", "pattern": r"stagePendingPolylineVertex\s*\(\s*std::move\s*\(vertex\)\s*,\s*vertexPublication\s*,\s*intfa\s*\)", "callee": "stagePendingPolylineVertex", "calleeOverload": "stagePendingPolylineVertex(DRW_Vertex&&,publication,interface)", "relation": "compound-stage"},
        )),
    ),
}
for _polyline_symbol in (
    "dwgType::POLYLINE_2D", "dwgType::POLYLINE_3D",
    "dwgType::POLYLINE_PFACE", "dwgType::POLYLINE_MESH",
):
    DWG_COMPOUND_TRANSITION_ANCHORS[_polyline_symbol] = (
        ("src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput", (
            {"name": "mapped-polyline-stage", "pattern": r"stageMappedPolylineAggregate\s*\(\s*std::move\s*\(e\)\s*,\s*polylinePublication", "callee": "stageMappedPolylineAggregate", "calleeOverload": "stageMappedPolylineAggregate(DRW_Polyline&&,publication,...)", "relation": "versioned-compound-stage"},
            {"name": "legacy-polyline-stage", "pattern": r"stageLegacyPolylineChain\s*\(\s*std::move\s*\(e\)\s*,\s*polylinePublication", "callee": "stageLegacyPolylineChain", "calleeOverload": "stageLegacyPolylineChain(DRW_Polyline&&,publication,...)", "relation": "versioned-compound-stage"},
        )),
    )

DWG_COMPOUND_DELIVERY_ANCHORS = (
    ("src/intern/dwgreader.cpp", "dwgReader::readMappedDwgEntity", (
        {"name": "journal-contract-guard", "pattern": r"completion\s*==\s*DwgMappedEntityCompletion::Journal\s*&&\s*\(\s*output\s*==\s*nullptr", "callee": "DwgMappedEntityCompletion::Journal", "calleeOverload": "journal-output-contract", "relation": "journal-gate"},
        {"name": "immediate-contract-guard", "pattern": r"completion\s*==\s*DwgMappedEntityCompletion::Immediate\s*&&\s*\(\s*output\s*!=\s*nullptr", "callee": "DwgMappedEntityCompletion::Immediate", "calleeOverload": "immediate-output-contract", "relation": "direct-gate"},
        {"name": "journal-scope-binding", "pattern": r"ActiveBlockJournalScope\s+activeBlockJournalScope", "callee": "ActiveBlockJournalScope", "calleeOverload": "journal-scope-lifetime", "relation": "journal-scope"},
        {"name": "entity-output-selection", "pattern": r"DwgEntityOutput\s*&\s*entityOutput\s*=", "callee": "DwgEntityOutput", "calleeOverload": "immediate-or-journal-output-selection", "relation": "output-selection"},
        {"name": "typed-read-dispatch", "pattern": r"readDwgEntityWithOutput\s*\(\s*dbuf\s*,\s*lease\.object\s*,\s*intfa\s*,\s*entityOutput", "callee": "readDwgEntityWithOutput", "calleeOverload": "readDwgEntityWithOutput(...,DwgEntityOutput&,...)", "relation": "typed-dispatch"},
        {"name": "journal-stage", "pattern": r"else\s+if\s*\(\s*completion\s*==\s*DwgMappedEntityCompletion::Journal\s*&&\s*outcome\s*==\s*DwgMappedEntityOutcome::PublishedSimple", "callee": "DwgMappedEntityCompletion::Journal", "calleeOverload": "journal-stage-predicate", "relation": "journal-stage"},
        {"name": "journal-outcome", "pattern": r"outcome\s*=\s*DwgMappedEntityOutcome::JournalledSimple", "callee": "DwgMappedEntityOutcome::JournalledSimple", "calleeOverload": "journal-outcome", "relation": "journal-stage"},
        {"name": "journal-case", "pattern": r"case\s+DwgMappedEntityOutcome::JournalledSimple\s*:", "callee": "DwgMappedEntityOutcome::JournalledSimple", "calleeOverload": "journal-delivery-case", "relation": "journal-delivery"},
        {"name": "compound-case", "pattern": r"case\s+DwgMappedEntityOutcome::StagedCompound\s*:\s*case\s+DwgMappedEntityOutcome::CommittedCompound\s*:\s*case\s+DwgMappedEntityOutcome::DeferredObject\s*:", "callee": "DwgMappedEntityOutcome::StagedCompound/CommittedCompound/DeferredObject", "calleeOverload": "compound-disposition-case", "relation": "compound-disposition"},
    )),
    ("src/intern/dwgreader.cpp", "dwgReader::DwgBlockJournalOutput::replay", (
        {"name": "journal-event-loop", "pattern": r"for\s*\(\s*std::size_t\s+index\s*=\s*0\s*;\s*index\s*<\s*m_events\.size\s*\(\s*\)\s*;\s*\+\+index\s*\)", "callee": "m_events", "calleeOverload": "journal-event-sequence", "relation": "journal-loop"},
        {"name": "journal-event-replay", "pattern": r"replayEvent\s*\(\s*index\s*,\s*target\s*,\s*activeSource\s*,\s*nullptr\s*\)", "callee": "replayEvent", "calleeOverload": "replayEvent(index,target,activeSource,completesSource)", "relation": "journal-delivery"},
        {"name": "journal-replay-success", "pattern": r"return\s+true", "callee": "true", "calleeOverload": "journal-replay-result", "relation": "journal-delivery"},
    )),
    ("src/intern/dwgreader.cpp", "dwgReader::DwgBlockScopeTransaction::replay", (
        {"name": "transaction-finished-guard", "pattern": r"if\s*\(\s*m_finished\s*\)\s*return\s+false", "callee": "m_finished", "calleeOverload": "transaction-state-guard", "relation": "reject"},
        {"name": "transaction-event-replay", "pattern": r"m_output\.replayEvent\s*\(\s*index\s*,\s*target\s*,\s*&activeSource\s*,\s*&completesSource\s*\)", "callee": "m_output.replayEvent", "calleeOverload": "replayEvent(index,target,activeSource,completesSource)", "relation": "journal-delivery"},
        {"name": "transaction-lease-retire", "pattern": r"retireLease\s*\(\s*activeSource\s*\)", "callee": "retireLease", "calleeOverload": "retireLease(const DwgSourceFrameId&)", "relation": "source-lifecycle"},
        {"name": "transaction-finish", "pattern": r"m_finished\s*=\s*true", "callee": "m_finished", "calleeOverload": "transaction-state-commit", "relation": "journal-commit"},
    )),
    ("src/intern/dwgreader.cpp", "dwgReader::readDwgBlocks", (
        {"name": "journal-eligibility", "pattern": r"bool\s+journalEligible\s*=\s*version\s*>=\s*DRW::AC1018", "callee": "journalEligible", "calleeOverload": "versioned-journal-selection", "relation": "journal-gate"},
        {"name": "journal-transaction", "pattern": r"DwgBlockScopeTransaction\s+transaction\s*\(\s*\*this\s*\)", "callee": "DwgBlockScopeTransaction", "calleeOverload": "block-scope-transaction", "relation": "journal-scope"},
        {"name": "journal-block-callback", "pattern": r"transaction\.output\(\)\.appendValue\s*\(\s*deliveryBlock\s*,\s*&DRW_Interface::addBlock", "callee": "DRW_Interface::addBlock", "calleeOverload": "appendValue(DRW_Block,addBlock)", "relation": "journal-publication"},
        {"name": "journal-entity-walk", "pattern": r"walkJournalledBlockRecordEntities\s*\(\s*bkr\s*,\s*dbuf\s*,\s*intfa\s*,\s*transaction", "callee": "walkJournalledBlockRecordEntities", "calleeOverload": "walkJournalledBlockRecordEntities(...,DwgBlockScopeTransaction&,...)", "relation": "journal-delivery"},
        {"name": "journal-replay", "pattern": r"transaction\.replay\s*\(\s*intfa\s*\)", "callee": "DwgBlockScopeTransaction::replay", "calleeOverload": "replay(DRW_Interface&)", "relation": "journal-delivery"},
        {"name": "direct-block-callback", "pattern": r"intfa\.addBlock\s*\(\s*bk\s*\)", "callee": "DRW_Interface::addBlock", "calleeOverload": "addBlock(DRW_Block&)", "relation": "direct-publication"},
        {"name": "nondeferred-entity-walk", "pattern": r"if\s*\(\s*!deferredEntityWalk\s*\)\s*\{\s*const\s+bool\s+walked\s*=\s*walkBlockRecordEntities", "callee": "walkBlockRecordEntities", "calleeOverload": "walkBlockRecordEntities(...,DRW_Interface&,...)", "relation": "direct-delivery"},
        {"name": "direct-end-block", "pattern": r"intfa\.endBlock\s*\(\s*\)", "callee": "DRW_Interface::endBlock", "calleeOverload": "endBlock()", "relation": "direct-finalize"},
        {"name": "deferred-entity-walk", "pattern": r"if\s*\(\s*deferredEntityWalk\s*&&\s*!blockScopeFailure\s*\)\s*\{\s*try\s*\{\s*const\s+bool\s+walked\s*=\s*walkBlockRecordEntities", "callee": "walkBlockRecordEntities", "calleeOverload": "walkBlockRecordEntities(...,DRW_Interface&,...)", "relation": "direct-delivery"},
    )),
)

# ``processDwg`` is the reader's public lifecycle boundary.  Stage anchors
# alone do not prove that the façade invokes them in the required order, that
# the first failing stage owns the sticky error, or that coverage finalizers
# run on both normal and exceptional exits.  Keep one ordered, source-bound
# contract on the representative ``readDwgTables`` stage; the table/block/
# entity/object stage rows remain individually visible and this common row is
# deliberately not duplicated across them.
DWG_READER_LIFECYCLE_RULES = (
    {"name": "class-coverage-begin", "pattern": r"reader->beginDwgClassCoverage\s*\(\s*\)", "callee": "beginDwgClassCoverage", "calleeOverload": "dwgReader::beginDwgClassCoverage()", "relation": "lifecycle-begin"},
    {"name": "header-read", "pattern": r"ret\s*=\s*reader->readDwgHeader\s*\(\s*hdr\s*\)", "callee": "readDwgHeader", "calleeOverload": "dwgReader::readDwgHeader(DRW_Header&) const", "relation": "stage-read"},
    {"name": "header-error-gate", "pattern": r"if\s*\(\s*!ret\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_HEADER\s*;", "callee": "BAD_READ_HEADER", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "classes-read", "pattern": r"ret2\s*=\s*reader->readDwgClasses\s*\(\s*\)", "callee": "readDwgClasses", "calleeOverload": "dwgReader::readDwgClasses()", "relation": "stage-read"},
    {"name": "classes-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_CLASSES\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_CLASSES", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "handles-read", "pattern": r"ret2\s*=\s*reader->readDwgHandles\s*\(\s*\)", "callee": "readDwgHandles", "calleeOverload": "dwgReader::readDwgHandles()", "relation": "stage-read"},
    {"name": "handles-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_HANDLES\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_HANDLES", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "tables-read", "pattern": r"ret2\s*=\s*ret\s*&&\s*reader->readDwgTables\s*\(\s*hdr\s*\)", "callee": "readDwgTables", "calleeOverload": "dwgReader::readDwgTables(DRW_Header&)", "relation": "stage-read"},
    {"name": "tables-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_TABLES\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_TABLES", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "header-publication", "pattern": r"iface->addHeader\s*\(\s*&hdr\s*\)", "callee": "addHeader", "calleeOverload": "DRW_Interface::addHeader(DRW_Header*)", "relation": "publish"},
    {"name": "linetype-publication", "pattern": r"iface->addLType\s*\(\s*const_cast<DRW_LType&>\(\*lt\)\s*\)", "callee": "addLType", "calleeOverload": "DRW_Interface::addLType(DRW_LType&)", "relation": "publish"},
    {"name": "layer-publication", "pattern": r"iface->addLayer\s*\(\s*const_cast<DRW_Layer&>\(\*ly\)\s*\)", "callee": "addLayer", "calleeOverload": "DRW_Interface::addLayer(DRW_Layer&)", "relation": "publish"},
    {"name": "text-style-publication", "pattern": r"iface->addTextStyle\s*\(\s*const_cast<DRW_Textstyle&>\(\*ly\)\s*\)", "callee": "addTextStyle", "calleeOverload": "DRW_Interface::addTextStyle(DRW_Textstyle&)", "relation": "publish"},
    {"name": "dimension-style-publication", "pattern": r"iface->addDimStyle\s*\(\s*const_cast<DRW_Dimstyle&>\(\*ly\)\s*\)", "callee": "addDimStyle", "calleeOverload": "DRW_Interface::addDimStyle(DRW_Dimstyle&)", "relation": "publish"},
    {"name": "viewport-publication", "pattern": r"iface->addVport\s*\(\s*const_cast<DRW_Vport&>\(\*ly\)\s*\)", "callee": "addVport", "calleeOverload": "DRW_Interface::addVport(DRW_Vport&)", "relation": "publish"},
    {"name": "appid-publication", "pattern": r"iface->addAppId\s*\(\s*const_cast<DRW_AppId&>\(\*ly\)\s*\)", "callee": "addAppId", "calleeOverload": "DRW_Interface::addAppId(DRW_AppId&)", "relation": "publish"},
    {"name": "view-publication", "pattern": r"iface->addView\s*\(\s*const_cast<DRW_View&>\(\*vw\)\s*\)", "callee": "addView", "calleeOverload": "DRW_Interface::addView(DRW_View&)", "relation": "publish"},
    {"name": "ucs-publication", "pattern": r"iface->addUCS\s*\(\s*const_cast<DRW_UCS&>\(\*u\)\s*\)", "callee": "addUCS", "calleeOverload": "DRW_Interface::addUCS(DRW_UCS&)", "relation": "publish"},
    {"name": "deferred-table-publication", "pattern": r"reader->publishDeferredTableFramePublications\s*\(\s*\*iface\s*\)", "callee": "publishDeferredTableFramePublications", "calleeOverload": "dwgReader::publishDeferredTableFramePublications(DRW_Interface&)", "relation": "publish-finalize"},
    {"name": "blocks-read", "pattern": r"ret2\s*=\s*reader->readDwgBlocks\s*\(\s*\*iface\s*\)", "callee": "readDwgBlocks", "calleeOverload": "dwgReader::readDwgBlocks(DRW_Interface&)", "relation": "stage-read"},
    {"name": "blocks-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_BLOCKS\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_BLOCKS", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "entities-read", "pattern": r"ret2\s*=\s*reader->readDwgEntities\s*\(\s*\*iface\s*\)", "callee": "readDwgEntities", "calleeOverload": "dwgReader::readDwgEntities(DRW_Interface&)", "relation": "stage-read"},
    {"name": "entities-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_ENTITIES\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_ENTITIES", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "objects-read", "pattern": r"ret2\s*=\s*reader->readDwgObjects\s*\(\s*\*iface\s*\)", "callee": "readDwgObjects", "calleeOverload": "dwgReader::readDwgObjects(DRW_Interface&)", "relation": "stage-read"},
    {"name": "objects-error-gate", "pattern": r"if\s*\(\s*ret\s*&&\s*!ret2\s*\)\s*\{[^}]*?error\s*=\s*DRW::BAD_READ_OBJECTS\s*;\s*ret\s*=\s*ret2\s*;", "callee": "BAD_READ_OBJECTS", "calleeOverload": "sticky-error-gate", "relation": "first-failure"},
    {"name": "raw-section-publication", "pattern": r"iface->addRawDwgSection\s*\(\s*section\s*\)", "callee": "addRawDwgSection", "calleeOverload": "DRW_Interface::addRawDwgSection(const DRW_RawDwgSection&)", "relation": "publish"},
    {"name": "datastorage-publication", "pattern": r"iface->addDataStorage\s*\(\s*storage\s*\)", "callee": "addDataStorage", "calleeOverload": "DRW_Interface::addDataStorage(const DRW_DataStorageSection&)", "relation": "publish"},
    {"name": "normal-finalizers", "pattern": r"finalizeClassCoverage\s*\(\s*\)\s*;\s*finalizeCoverage\s*\(\s*ret\s*\)", "callee": "finalizeClassCoverage/finalizeCoverage", "calleeOverload": "normal-finalizer-pair", "relation": "finalize-success"},
    {"name": "success-return", "pattern": r"return\s+ret\s*;", "callee": "ret", "calleeOverload": "lifecycle-result", "relation": "return"},
    {"name": "exception-finalizers", "pattern": r"finalizeClassCoverage\s*\(\s*\)\s*;\s*finalizeCoverage\s*\(\s*false\s*\)", "callee": "finalizeClassCoverage/finalizeCoverage", "calleeOverload": "exception-finalizer-pair", "relation": "finalize-exception"},
)

# The BLOCK_RECORD map is the ownership root for BLOCK/ENDBLK delimiters and
# modern entMap children.  This source-only contract proves the guard and
# publication order around that root without pretending to qualify the DWG
# wire fields.  It is represented once on the kBlockTable descriptor; the
# compound/direct-journal contracts remain separate evidence for delivery.
DWG_BLOCK_OWNERSHIP_RULES = (
    {"name": "compound-state-reset", "pattern": r"m_consumedCompoundChildHandles\.clear\s*\(\s*\)", "callee": "m_consumedCompoundChildHandles", "calleeOverload": "compound-state-reset", "relation": "ownership-begin"},
    {"name": "claim-block-delimiter", "pattern": r"claimHandle\s*\(\s*record\s*,\s*record->block\s*\)\s*;", "callee": "claimHandle(block)", "calleeOverload": "BLOCK handle claim", "relation": "ownership-claim"},
    {"name": "claim-endblock-delimiter", "pattern": r"claimHandle\s*\(\s*record\s*,\s*record->endBlock\s*\)\s*;", "callee": "claimHandle(endBlock)", "calleeOverload": "ENDBLK handle claim", "relation": "ownership-claim"},
    {"name": "claim-entmap-handles", "pattern": r"for\s*\(\s*const\s+std::uint32_t\s+handle\s*:\s*record->entMap\s*\)\s*claimHandle\s*\(\s*record\s*,\s*handle\s*\)", "callee": "claimHandle(entMap)", "calleeOverload": "owned-entity handle claims", "relation": "ownership-claim"},
    {"name": "ownership-quarantine", "pattern": r"if\s*\(\s*!invalidOwnershipRecords\.empty\s*\(\s*\)\s*\)", "callee": "invalidOwnershipRecords", "calleeOverload": "ownership-quarantine-gate", "relation": "reject"},
    {"name": "polyline-ownership-preflight", "pattern": r"preflightMappedPolylineOwnership\s*\(\s*records\s*,\s*dbuf\s*\)", "callee": "preflightMappedPolylineOwnership", "calleeOverload": "preflightMappedPolylineOwnership(records,dwgBuffer*)", "relation": "ownership-preflight"},
    {"name": "block-record-walk", "pattern": r"for\s*\(\s*auto\s+it\s*=\s*blockRecordmap\.begin\s*\(\s*\)\s*;\s*it\s*!?=\s*blockRecordmap\.end\s*\(\s*\)\s*;\s*\+\+it\s*\)", "callee": "blockRecordmap", "calleeOverload": "BLOCK_RECORD ownership walk", "relation": "ownership-walk"},
    {"name": "block-lookup", "pattern": r"auto\s+mit\s*=\s*ObjectMap\.find\s*\(\s*bkr->block\s*\)", "callee": "ObjectMap.find(block)", "calleeOverload": "BLOCK source lookup", "relation": "delimiter-lookup"},
    {"name": "block-frame-read", "pattern": r"!frame\.readAt\s*\(\s*\*dbuf\s*,\s*version\s*,\s*oc\.loc\s*\)", "callee": "DwgObjectFrame::readAt", "calleeOverload": "BLOCK frame read", "relation": "frame-validate"},
    {"name": "block-type-check", "pattern": r"typeBuffer\.getObjType\s*\(\s*version\s*\)\s*!=\s*dwgType::BLOCK", "callee": "dwgType::BLOCK", "calleeOverload": "BLOCK type guard", "relation": "typed-validate"},
    {"name": "block-body-parse", "pattern": r"!parseBlock\s*\(\s*bk\s*,\s*buff\s*,\s*frame\.bodyBitSize\s*\(\s*\)\s*\)", "callee": "parseBlock(BLOCK)", "calleeOverload": "BLOCK body parse", "relation": "typed-parse"},
    {"name": "block-handle-identity", "pattern": r"bk\.handle\s*!=\s*oc\.handle\s*\|\|\s*bk\.handle\s*!=\s*bkr->block", "callee": "bk.handle", "calleeOverload": "BLOCK handle identity", "relation": "identity-reject"},
    {"name": "entmap-shape-check", "pattern": r"bkr->entMap\.size\s*\(\s*\)\s*>\s*static_cast<std::size_t>\s*\(\s*std::numeric_limits<int>::max\s*\(\s*\)\s*\)", "callee": "bkr->entMap", "calleeOverload": "owned-entity list bound", "relation": "ownership-validate"},
    {"name": "entmap-entity-type-check", "pattern": r"if\s*\(\s*classification\.route\s*!=\s*DwgFrameClassification::Route::Entity\s*\)\s*\{\s*validOwnership", "callee": "DwgFrameClassification::Route::Entity", "calleeOverload": "owned-entity frame classification", "relation": "ownership-validate"},
    {"name": "endblock-lookup", "pattern": r"auto\s+endIt\s*=\s*ObjectMap\.find\s*\(\s*bkr->endBlock\s*\)", "callee": "ObjectMap.find(endBlock)", "calleeOverload": "ENDBLK source lookup", "relation": "delimiter-lookup"},
    {"name": "endblock-frame-read", "pattern": r"!endFrame\.readAt\s*\(\s*\*dbuf\s*,\s*version\s*,\s*oc\.loc\s*\)", "callee": "DwgObjectFrame::readAt", "calleeOverload": "ENDBLK frame read", "relation": "frame-validate"},
    {"name": "endblock-type-check", "pattern": r"endTypeBuffer\.getObjType\s*\(\s*version\s*\)\s*!=\s*dwgType::ENDBLK", "callee": "dwgType::ENDBLK", "calleeOverload": "ENDBLK type guard", "relation": "typed-validate"},
    {"name": "endblock-body-parse", "pattern": r"!parseBlock\s*\(\s*end\s*,\s*buff1\s*,\s*endFrame\.bodyBitSize\s*\(\s*\)\s*\)", "callee": "parseBlock(ENDBLK)", "calleeOverload": "ENDBLK body parse", "relation": "typed-parse"},
    {"name": "endblock-handle-identity", "pattern": r"end\.handle\s*!=\s*oc\.handle\s*\|\|\s*end\.handle\s*!=\s*bkr->endBlock", "callee": "end.handle", "calleeOverload": "ENDBLK handle identity", "relation": "identity-reject"},
    {"name": "delimiter-parent-match", "pattern": r"bk\.parentHandle\s*!=\s*end\.parentHandle", "callee": "parentHandle", "calleeOverload": "BLOCK/ENDBLK owner equality", "relation": "owner-validate"},
    {"name": "space-owner-classification", "pattern": r"const\s+bool\s+isSpaceBlockRecord\s*=\s*isSpaceBlockRecordName\s*\(\s*bk\.name\s*\)", "callee": "isSpaceBlockRecordName", "calleeOverload": "modelspace-paperspace owner rule", "relation": "owner-validate"},
    {"name": "delimiter-owner-gate", "pattern": r"const\s+bool\s+validDelimiterOwner\s*=", "callee": "validDelimiterOwner", "calleeOverload": "BLOCK/ENDBLK owner gate", "relation": "owner-validate"},
    {"name": "delimiter-publication-build", "pattern": r"makeTypedEntityFramePublication\s*\(\s*version\s*,\s*blockObject\s*,\s*dwgType::BLOCK", "callee": "makeTypedEntityFramePublication(BLOCK)", "calleeOverload": "BLOCK frame publication", "relation": "publication-build"},
    {"name": "journal-selection", "pattern": r"bool\s+journalEligible\s*=\s*version\s*>=\s*DRW::AC1018", "callee": "journalEligible", "calleeOverload": "versioned-journal-selection", "relation": "delivery-select"},
    {"name": "direct-block-publication", "pattern": r"intfa\.addBlock\s*\(\s*bk\s*\)", "callee": "DRW_Interface::addBlock", "calleeOverload": "addBlock(DRW_Block&)", "relation": "direct-publication"},
    {"name": "direct-owned-entity-walk", "pattern": r"walkBlockRecordEntities\s*\(\s*bkr\s*,\s*dbuf\s*,\s*intfa\s*,\s*bk\.parentHandle", "callee": "walkBlockRecordEntities", "calleeOverload": "owned-entity walk", "relation": "direct-delivery"},
    {"name": "direct-endblock-publication", "pattern": r"intfa\.endBlock\s*\(\s*\)", "callee": "DRW_Interface::endBlock", "calleeOverload": "endBlock()", "relation": "direct-finalize"},
    {"name": "space-owned-entity-walk", "pattern": r"walkBlockRecordEntities\s*\(\s*bkr\s*,\s*dbuf\s*,\s*intfa\s*,\s*DRW::NoHandle", "callee": "walkBlockRecordEntities", "calleeOverload": "deferred space-owned entity walk", "relation": "direct-delivery"},
    {"name": "delimiter-frame-commit", "pattern": r"commitDelimiter\s*\(\s*blockObject\.handle\s*,\s*blockLease\s*\)", "callee": "commitDelimiter(BLOCK)", "calleeOverload": "delimiter frame commit", "relation": "frame-commit"},
    {"name": "delimiter-frame-publication", "pattern": r"publishDwgFramePublication\s*\(\s*intfa\s*,\s*blockPublication\s*\)\s*\|\|\s*!?\s*publishDwgFramePublication\s*\(\s*intfa\s*,\s*endBlockPublication\s*\)", "callee": "publishDwgFramePublication(BLOCK,ENDBLK)", "calleeOverload": "delimiter frame publication", "relation": "publication-finalize"},
    {"name": "block-scope-quarantine", "pattern": r"if\s*\(\s*blockScopeFailure\s*\)\s*\{[\s\S]*?quarantineOwnedEntities\s*\(\s*\*bkr\s*\)", "callee": "quarantineOwnedEntities", "calleeOverload": "failed block ownership quarantine", "relation": "failure-cleanup"},
)

# These route nodes cover the deliberate deferred/publication machinery that
# cannot truthfully be reduced to one local parser-function → callback call.
# A staged parser row must point to at least one of these exact anchors.
PUBLICATION_STAGE_ANCHORS = (
    ("entity-output-append", "function", "src/intern/dwgreader.cpp", "dwgReader::DwgEntityOutput::appendValue"),
    ("immediate-entity-output", "class", "src/intern/dwgreader.cpp", "dwgReader::DwgImmediateEntityOutput"),
    ("block-journal-replay", "function", "src/intern/dwgreader.cpp", "dwgReader::DwgBlockJournalOutput::replay"),
    ("block-scope-replay", "function", "src/intern/dwgreader.cpp", "dwgReader::DwgBlockScopeTransaction::replay"),
    ("entry-parse", "function", "src/intern/dwgreader.h", "entryParse"),
    ("emit-with-extrusion", "function", "src/intern/dwgreader.h", "emitWithExtrusion"),
    ("frame-publication", "function", "src/intern/dwgreader.cpp", "dwgReader::publishDwgFramePublication"),
    ("deferred-raw-publication", "function", "src/intern/dwgreader.cpp", "dwgReader::publishDeferredRawObjects"),
    ("deferred-table-publication", "function", "src/intern/dwgreader.cpp", "dwgReader::publishDeferredTableFramePublications"),
    ("dwg-facade-process", "function", "src/libdwgr.cpp", "dwgRW::processDwg"),
    ("dxf-raw-captured-template", "function", "src/libdxfrw.cpp", "dxfRW::processRawCapturedObject"),
    ("dxf-block-event-flush", "class-method", "src/libdxfrw.cpp", "DxfBlockEventSink::flush"),
    ("dxf-image-def-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processImageDef"),
    ("dxf-blocks-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processBlocks"),
    ("dxf-entities-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processEntities"),
    ("dxf-header-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processHeader"),
    ("dxf-objects-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processObjects"),
    ("dxf-tables-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processTables"),
    ("dxf-block-record-dispatch", "function", "src/libdxfrw.cpp", "dxfRW::processBlockRecord"),
    ("dwg-fixed-entity-dispatch", "function", "src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput"),
    ("dwg-fixed-object-dispatch", "function", "src/intern/dwgreader.cpp", "dwgReader::readDwgObject"),
    ("dwg-named-entity-class-dispatch", "function", "src/intern/dwgreader.cpp", "dwgReader::readDwgEntityWithOutput"),
    ("dwg-named-object-class-dispatch", "function", "src/intern/dwgreader.cpp", "dwgReader::readDwgObject"),
    ("dwg-table-descriptor-dispatch", "function", "src/intern/dwgreader.cpp", "dwgReader::readDwgTables"),
)

DXF_STAGED_PUBLICATION_TARGETS = {
    "processImageDef": "dxf-image-def-dispatch",
    "processBlocks": "dxf-blocks-dispatch",
    "processEntities": "dxf-entities-dispatch",
    "processHeader": "dxf-header-dispatch",
    "processObjects": "dxf-objects-dispatch",
    "processTables": "dxf-tables-dispatch",
    "processBlockRecord": "dxf-block-record-dispatch",
}

DXF_STAGED_PUBLICATION_CATEGORIES = {
    "dxf-block": "dxf-blocks-dispatch",
    "dxf-entity": "dxf-entities-dispatch",
    "dxf-object": "dxf-objects-dispatch",
    "dxf-table": "dxf-tables-dispatch",
}

DWG_STAGED_PUBLICATION_CATEGORIES = {
    "fixed-entity": "entry-parse",
    "fixed-object": "entry-parse",
    "named-entity-class": "entry-parse",
    "named-object-class": "entry-parse",
    "table-descriptor": "deferred-table-publication",
}

# Per-header public-surface contracts prevent an aggregate count from masking a
# declaration that quietly moved between installed headers.  These are route
# counts, not ABI/support claims, and are refreshed only with a reviewed target
# pin update.
TARGET_PUBLIC_HEADER_ROUTE_COUNTS = {
    "src/drw_acis.h": {"public-enum": 3, "public-header": 1, "public-inline-method": 1, "public-method": 2, "public-type": 11},
    "src/drw_base.h": {"public-enum": 17, "public-header": 1, "public-inline-method": 61, "public-method": 61, "public-type": 17},
    "src/drw_classes.h": {"public-header": 1, "public-method": 2, "public-type": 4},
    "src/drw_datastorage.h": {"public-enum": 2, "public-header": 1, "public-inline-function": 1, "public-inline-method": 3, "public-method": 3, "public-type": 10},
    "src/drw_entities.h": {"public-enum": 11, "public-header": 1, "public-inline-method": 225, "public-method": 254, "public-type": 102},
    "src/drw_header.h": {"public-enum": 1, "public-header": 1, "public-inline-method": 10, "public-method": 18, "public-type": 8},
    "src/drw_interface.h": {"public-header": 1, "public-inline-method": 119, "public-method": 119, "public-type": 3},
    "src/drw_objects.h": {"public-enum": 21, "public-header": 1, "public-inline-function": 2, "public-inline-method": 191, "public-method": 243, "public-type": 148},
    "src/libdwgr.h": {"public-alias": 1, "public-enum": 6, "public-header": 1, "public-inline-method": 13, "public-method": 207, "public-type": 9},
    "src/libdxfrw.h": {"public-header": 1, "public-inline-method": 25, "public-method": 132, "public-type": 5},
}


class RouteError(RuntimeError):
    """Raised when a pinned source or fail-closed source extraction is invalid."""


def sha256_bytes(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha256_text(value: str) -> str:
    return sha256_bytes(value.encode("utf-8"))


def canonical_json(value: object) -> str:
    return json.dumps(value, ensure_ascii=True, indent=2, sort_keys=True) + "\n"


def read_json(path: Path) -> dict:
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise RouteError("cannot read JSON %s: %s" % (path, exc)) from exc
    if not isinstance(value, dict):
        raise RouteError("JSON root must be an object: %s" % path)
    return value


def run(command: list[str], *, cwd: Path | None = None) -> str:
    try:
        return subprocess.check_output(
            command, cwd=str(cwd) if cwd else None, text=True, stderr=subprocess.STDOUT
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RouteError("command failed: %s" % exc) from exc


def run_bytes(command: list[str], *, cwd: Path | None = None) -> bytes:
    try:
        return subprocess.check_output(
            command, cwd=str(cwd) if cwd else None, stderr=subprocess.STDOUT
        )
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RouteError("command failed: %s" % exc) from exc


def ensure_command(command: list[str], *, cwd: Path) -> None:
    try:
        subprocess.check_output(command, cwd=str(cwd), stderr=subprocess.STDOUT)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise RouteError("required integrity check failed: %s" % exc) from exc


def line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def normalized_identifier(value: str) -> str:
    result = re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("_")
    return result or "unnamed"


def cpp_unquote(value: str) -> str:
    """Decode the small C++ string subset used by dispatch identifiers."""
    result: list[str] = []
    index = 0
    while index < len(value):
        char = value[index]
        if char != "\\" or index + 1 == len(value):
            result.append(char)
            index += 1
            continue
        index += 1
        escaped = value[index]
        result.append({"n": "\n", "r": "\r", "t": "\t"}.get(escaped, escaped))
        index += 1
    return "".join(result)


def mask_cpp(text: str, *, keep_strings: bool = False) -> str:
    """Mask comments and, optionally, literals while preserving every offset.

    It intentionally is a lexer rather than a C++ parser: balanced-body
    extraction happens after comments and literals cannot manufacture braces,
    function names, cases, or selectors.
    """
    output = list(text)

    def blank(start: int, end: int) -> None:
        for position in range(start, end):
            if output[position] != "\n":
                output[position] = " "

    index = 0
    length = len(text)
    raw_start = re.compile(r"(?:u8|u|U|L)?R\"([^()\\\s]{0,16})\(")
    while index < length:
        if text.startswith("//", index):
            end = text.find("\n", index)
            if end < 0:
                end = length
            blank(index, end)
            index = end
            continue
        if text.startswith("/*", index):
            end = text.find("*/", index + 2)
            end = length if end < 0 else end + 2
            blank(index, end)
            index = end
            continue
        raw = raw_start.match(text, index)
        if raw:
            delimiter = raw.group(1)
            closing = ")" + delimiter + '"'
            end = text.find(closing, raw.end())
            end = length if end < 0 else end + len(closing)
            if not keep_strings:
                blank(index, end)
            index = end
            continue
        if text[index] in {'"', "'"}:
            quote = text[index]
            end = index + 1
            while end < length:
                if text[end] == "\\":
                    end += 2
                    continue
                if text[end] == quote:
                    end += 1
                    break
                end += 1
            if not keep_strings:
                blank(index, min(end, length))
            index = end
            continue
        index += 1
    return "".join(output)


def matching_delimiter(code: str, start: int, opening: str = "{", closing: str = "}") -> int:
    if start >= len(code) or code[start] != opening:
        raise RouteError("expected %r at source offset %d" % (opening, start))
    depth = 0
    for index in range(start, len(code)):
        char = code[index]
        if char == opening:
            depth += 1
        elif char == closing:
            depth -= 1
            if depth == 0:
                return index
    raise RouteError("unbalanced %r/%r delimiters at source offset %d" % (opening, closing, start))


@dataclass
class SourceFile:
    path: str
    text: str
    sha256: str
    git_blob: str | None = None
    mode: str | None = None
    _code: str | None = field(default=None, init=False, repr=False)
    _comment_text: str | None = field(default=None, init=False, repr=False)

    @property
    def code(self) -> str:
        if self._code is None:
            self._code = mask_cpp(self.text)
        return self._code

    @property
    def comment_text(self) -> str:
        if self._comment_text is None:
            self._comment_text = mask_cpp(self.text, keep_strings=True)
        return self._comment_text


@dataclass(frozen=True)
class FunctionBody:
    source: SourceFile
    symbol: str
    symbol_start: int
    body_start: int
    body_end: int

    @property
    def code(self) -> str:
        return self.source.code[self.body_start : self.body_end + 1]

    @property
    def text(self) -> str:
        return self.source.text[self.body_start : self.body_end + 1]

    @property
    def comment_text(self) -> str:
        return self.source.comment_text[self.body_start : self.body_end + 1]

    @property
    def line(self) -> int:
        return line_number(self.source.text, self.symbol_start)

    @property
    def span_sha256(self) -> str:
        return sha256_text(self.source.text[self.symbol_start : self.body_end + 1])


@dataclass(frozen=True)
class PublicationDispatchBranch:
    """A narrowed DWG dispatch arm plus the selector evidence it omitted."""

    body: FunctionBody
    dispatch_ancestry: tuple[dict, ...]


# Publication extraction revisits the same narrow branch for each physical
# typed/raw callback.  Cache lexical scans by immutable source span so the
# fail-closed ancestry checks stay a fast source-only gate on the full target.
_LEXICAL_BRANCH_ANCESTRY_CACHE: dict[tuple[str, str, int, int, int, int], tuple[dict, ...]] = {}
_SWITCH_CASE_ANCESTRY_CACHE: dict[tuple[str, str, int, int, int, int], tuple[dict, ...]] = {}


def publication_ancestry_cache_key(body: FunctionBody, call_offset: int) -> tuple[str, str, int, int, int, int]:
    return (
        body.source.sha256, body.symbol, body.symbol_start, body.body_start,
        body.body_end, call_offset,
    )


def body_after_signature(source: SourceFile, symbol: str, symbol_start: int, open_paren: int) -> FunctionBody | None:
    code = source.code
    close_paren = matching_delimiter(code, open_paren, "(", ")")
    index = close_paren + 1
    while index < len(code):
        char = code[index]
        if char == "{":
            return FunctionBody(source, symbol, symbol_start, index, matching_delimiter(code, index))
        if char == ";":
            return None
        index += 1
    raise RouteError("unterminated declaration for %s in %s" % (symbol, source.path))


def declaration_context(source: SourceFile, symbol_start: int) -> bool:
    """Reject call expressions whose closing ``)`` happens to precede a block."""
    # A definition name cannot sit inside the parenthesized condition of a
    # multiline if/while/call expression.  The former line-only heuristic
    # missed exactly that shape and could misclassify a call as a definition.
    prefix_code = source.code[:symbol_start]
    if prefix_code.count("(") != prefix_code.count(")"):
        return False
    line_start = source.code.rfind("\n", 0, symbol_start) + 1
    prefix = source.code[line_start:symbol_start]
    if re.search(r"\b(?:if|for|while|switch|return|sizeof|decltype)\b", prefix):
        return False
    return not any(marker in prefix for marker in ("=", "!", "->", ".", "("))


def function_bodies(source: SourceFile, symbol: str) -> list[FunctionBody]:
    """Return every real definition for a symbol, including overloads.

    The inventory generally uses a one-definition expectation, but pipeline
    closure must account for deliberately overloaded capture/read/write entry
    points.  Calls and control-flow expressions remain filtered by
    ``declaration_context``.
    """
    pattern = re.compile(r"(?<![A-Za-z0-9_:])" + re.escape(symbol) + r"\s*\(")
    candidates: list[FunctionBody] = []
    for match in pattern.finditer(source.code):
        if not declaration_context(source, match.start()):
            continue
        open_paren = source.code.find("(", match.start(), match.end() + 1)
        candidate = body_after_signature(source, symbol, match.start(), open_paren)
        if candidate is not None:
            candidates.append(candidate)
    return candidates


def function_body(source: SourceFile, symbol: str) -> FunctionBody:
    candidates = function_bodies(source, symbol)
    if len(candidates) != 1:
        raise RouteError("expected exactly one definition for %s in %s; found %d" % (symbol, source.path, len(candidates)))
    return candidates[0]


def qualified_definitions(source: SourceFile, prefix: str) -> list[FunctionBody]:
    pattern = re.compile(r"(?<![A-Za-z0-9_:])(" + re.escape(prefix) + r"[A-Za-z0-9_]*)\s*\(")
    result: list[FunctionBody] = []
    seen: set[tuple[str, int]] = set()
    for match in pattern.finditer(source.code):
        symbol = match.group(1)
        if not declaration_context(source, match.start()):
            continue
        open_paren = source.code.find("(", match.start(), match.end() + 1)
        candidate = body_after_signature(source, symbol, match.start(), open_paren)
        if candidate is not None and (symbol, candidate.symbol_start) not in seen:
            result.append(candidate)
            seen.add((symbol, candidate.symbol_start))
    if not result:
        raise RouteError("no definitions with prefix %s in %s" % (prefix, source.path))
    return result


def signature_fingerprint(body: FunctionBody) -> str:
    """Fingerprint a definition signature so C++ overloads stay distinct."""
    signature = " ".join(body.source.code[body.symbol_start : body.body_start].split())
    if not signature:
        raise RouteError("definition has no signature: %s" % body.symbol)
    return sha256_text(signature)


def callback_parameter_contract(signature: str) -> dict[str, object]:
    """Describe the single DRW callback parameter without parsing C++ bodies.

    Parser/publication edges are only meaningful when the physical callback
    accepts the model that the parser constructed.  The interface signatures
    are deliberately simple enough to retain the DRW parameter spelling and
    pointer/reference form without reconstructing the declaration text.
    """
    opening = signature.find("(")
    closing = signature.rfind(")")
    if opening < 0 or closing <= opening:
        raise RouteError("callback signature has no parameter list")
    parameters = signature[opening + 1 : closing]
    matches = list(
        re.finditer(
            r"\b(DRW_[A-Za-z0-9_]+)\b\s*(?:const\s*)?([*&]?)",
            parameters,
        )
    )
    if len(matches) != 1:
        return {
            "parameterModel": None,
            "parameterPassing": "none" if not matches else "multiple",
            "parameterModelCount": len(matches),
        }
    model, modifier = matches[0].groups()
    return {
        "parameterModel": model,
        "parameterPassing": {"*": "pointer", "&": "reference"}.get(modifier, "value"),
        "parameterModelCount": 1,
    }


def virtual_method_signature_end(source: SourceFile, name_end: int) -> tuple[int, bool]:
    """Return the signature terminator for one virtual declaration/definition.

    Default inline callbacks have bodies containing semicolons.  Looking for
    the next semicolon therefore merges later callbacks into their contract;
    follow the method parameter list and stop at its first body or declaration
    terminator instead.
    """
    opening = source.code.find("(", name_end)
    if opening < 0:
        raise RouteError("virtual method has no parameter list")
    closing = matching_delimiter(source.code, opening, "(", ")")
    index = closing + 1
    while index < len(source.code):
        char = source.code[index]
        if char == "{":
            return index, True
        if char == ";":
            return index + 1, False
        index += 1
    raise RouteError("virtual method has no terminator")


def named_array_body(source: SourceFile, name: str) -> FunctionBody:
    pattern = re.compile(r"\b" + re.escape(name) + r"\s*\[\s*\]\s*=")
    matches = list(pattern.finditer(source.code))
    if len(matches) != 1:
        raise RouteError("expected exactly one array %s in %s; found %d" % (name, source.path, len(matches)))
    match = matches[0]
    start = source.code.find("{", match.end())
    if start < 0:
        raise RouteError("array %s has no initializer body" % name)
    return FunctionBody(source, name, match.start(), start, matching_delimiter(source.code, start))


def direct_initializer_entries(body: FunctionBody) -> list[FunctionBody]:
    """Return direct braced entries of an aggregate initializer."""
    result: list[FunctionBody] = []
    code = body.code
    depth = 0
    index = 0
    while index < len(code):
        char = code[index]
        if char == "{":
            if depth == 1:
                absolute_start = body.body_start + index
                absolute_end = matching_delimiter(body.source.code, absolute_start)
                result.append(
                    FunctionBody(
                        body.source,
                        body.symbol + "::entry",
                        absolute_start,
                        absolute_start,
                        absolute_end,
                    )
                )
                index = absolute_end - body.body_start + 1
                continue
            depth += 1
        elif char == "}":
            depth -= 1
        index += 1
    if not result:
        raise RouteError("aggregate initializer has no direct entries: %s" % body.symbol)
    return result


def enum_body(source: SourceFile, name: str) -> FunctionBody:
    pattern = re.compile(r"\benum(?:\s+class)?\s+" + re.escape(name) + r"\s*\{")
    matches = list(pattern.finditer(source.code))
    if len(matches) != 1:
        raise RouteError("expected exactly one enum %s in %s; found %d" % (name, source.path, len(matches)))
    match = matches[0]
    start = source.code.find("{", match.start(), match.end())
    return FunctionBody(source, name, match.start(), start, matching_delimiter(source.code, start))


def namespace_enum_body(source: SourceFile, namespace: str, enum_name: str) -> FunctionBody:
    namespace_pattern = re.compile(r"\bnamespace\s+" + re.escape(namespace) + r"\s*\{")
    matches = list(namespace_pattern.finditer(source.code))
    if len(matches) != 1:
        raise RouteError("expected exactly one namespace %s in %s" % (namespace, source.path))
    namespace_start = source.code.find("{", matches[0].start(), matches[0].end())
    namespace_end = matching_delimiter(source.code, namespace_start)
    subset = source.code[namespace_start : namespace_end + 1]
    enum_pattern = re.compile(r"\benum(?:\s+class)?\s+" + re.escape(enum_name) + r"\s*\{")
    enum_matches = list(enum_pattern.finditer(subset))
    if len(enum_matches) != 1:
        raise RouteError("expected enum %s in namespace %s" % (enum_name, namespace))
    absolute = namespace_start + enum_matches[0].start()
    start = source.code.find("{", absolute, namespace_start + enum_matches[0].end())
    return FunctionBody(source, "%s::%s" % (namespace, enum_name), absolute, start, matching_delimiter(source.code, start))


def enum_values(body: FunctionBody) -> list[tuple[str, int, int]]:
    result: list[tuple[str, int, int]] = []
    value = -1
    for match in re.finditer(
        r"\b([A-Z][A-Z0-9_]*)\s*(?:=\s*(0[xX][0-9A-Fa-f]+|\d+))?\s*(?=,|\})",
        body.code[1:],
    ):
        name = match.group(1)
        if match.group(2) is not None:
            value = int(match.group(2), 0)
        else:
            value += 1
        absolute = body.body_start + 1 + match.start(1)
        result.append((name, value, absolute))
    if not result:
        raise RouteError("enum %s produced no values" % body.symbol)
    return result


STRING_RE = re.compile(r'"((?:\\.|[^"\\])*)"')


def string_literals(text: str) -> list[str]:
    return [cpp_unquote(match.group(1)) for match in STRING_RE.finditer(text)]


def keyword_literals(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    for match in re.finditer(r"\bdxfKeywordEquals\s*\(", body.code):
        open_paren = body.code.find("(", match.start(), match.end() + 1)
        close_paren = matching_delimiter(body.code, open_paren, "(", ")")
        values = string_literals(body.text[open_paren : close_paren + 1])
        for value in values:
            absolute = body.body_start + match.start()
            result.append((value, line_number(body.source.text, absolute)))
    return result


def equality_literals(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    pattern = re.compile(
        r'\b(?:nextentity|normalizedEntity|sectionstr|sectionname)\s*==\s*"((?:\\.|[^"\\])*)"'
    )
    for match in pattern.finditer(body.comment_text):
        absolute = body.body_start + match.start(1)
        result.append((cpp_unquote(match.group(1)), line_number(body.source.text, absolute)))
    return result


def selector_literals(body: FunctionBody) -> list[tuple[str, int]]:
    values = keyword_literals(body) + equality_literals(body)
    return sorted(set(values), key=lambda item: (item[0], item[1]))


def initializer_strings(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    for match in re.finditer(r'\{\s*"((?:\\.|[^"\\])*)"', body.comment_text):
        absolute = body.body_start + match.start(1)
        result.append((cpp_unquote(match.group(1)), line_number(body.source.text, absolute)))
    return result


def dxf_class_entries(body: FunctionBody) -> list[tuple[dict, int]]:
    """Extract the complete dxfClassForRecordName table rows."""
    pattern = re.compile(
        r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*'
        r'"((?:\\.|[^"\\])*)"\s*,\s*'
        r'"((?:\\.|[^"\\])*)"\s*,\s*'
        r'(-?\d+)\s*,\s*([01])\s*\}'
    )
    result: list[tuple[dict, int]] = []
    for match in pattern.finditer(body.comment_text):
        record, class_name, app_name, flag, is_entity = match.groups()
        selector = {
            "kind": "class-entry",
            "recordName": cpp_unquote(record),
            "className": cpp_unquote(class_name),
            "appName": cpp_unquote(app_name),
            "proxyFlag": int(flag),
            "isEntity": bool(int(is_entity)),
        }
        result.append((selector, line_number(body.source.text, body.body_start + match.start())))
    return result


def all_string_literals(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    for match in STRING_RE.finditer(body.comment_text):
        absolute = body.body_start + match.start(1)
        result.append((cpp_unquote(match.group(1)), line_number(body.source.text, absolute)))
    return result


def switch_cases(body: FunctionBody) -> list[tuple[str, int]]:
    """Return direct case labels from a switch body.

    ``FunctionBody`` is also useful for an enclosing function, so callers that
    need a particular switch should first use :func:`switch_body`.  Keeping
    the depth check here prevents an incidental nested switch from becoming a
    top-level dispatch route.
    """
    result: list[tuple[str, int]] = []
    pattern = re.compile(r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*|0[xX][0-9A-Fa-f]+|\d+)\s*:")
    for match in pattern.finditer(body.code):
        depth = body.code[: match.start()].count("{") - body.code[: match.start()].count("}")
        if depth != 1:
            continue
        absolute = body.body_start + match.start(1)
        result.append((match.group(1), line_number(body.source.text, absolute)))
    return result


def switch_body(body: FunctionBody, expression: str | None = None) -> FunctionBody:
    """Find one brace-balanced switch inside ``body``.

    The extractor deliberately ties DWG fixed dispatch to ``switch (oType)``.
    That makes a nested validation switch non-authoritative rather than an
    accidental additional entity or object route.
    """
    matches: list[tuple[int, FunctionBody]] = []
    pattern = re.compile(r"\bswitch\s*\(")
    wanted = None if expression is None else " ".join(expression.split())
    for match in pattern.finditer(body.code):
        depth = body.code[: match.start()].count("{") - body.code[: match.start()].count("}")
        open_paren = body.code.find("(", match.start(), match.end() + 1)
        close_paren = matching_delimiter(body.code, open_paren, "(", ")")
        condition = " ".join(body.code[open_paren + 1 : close_paren].split())
        if wanted is not None and condition != wanted:
            continue
        index = close_paren + 1
        while index < len(body.code) and body.code[index].isspace():
            index += 1
        if index >= len(body.code) or body.code[index] != "{":
            raise RouteError("switch in %s has no braced body" % body.symbol)
        absolute_start = body.body_start + index
        matches.append(
            (
                depth,
                FunctionBody(
                body.source,
                "%s::switch(%s)" % (body.symbol, condition),
                body.body_start + match.start(),
                absolute_start,
                matching_delimiter(body.source.code, absolute_start),
                ),
            )
        )
    if matches:
        shallowest = min(depth for depth, _candidate in matches)
        matches = [item for item in matches if item[0] == shallowest]
    if len(matches) != 1:
        selector = expression if expression is not None else "any expression"
        raise RouteError(
            "expected exactly one switch (%s) in %s; found %d"
            % (selector, body.symbol, len(matches))
        )
    return matches[0][1]


def raw_classifier_rules(body: FunctionBody) -> list[tuple[dict, int, str]]:
    """Extract the explicit DXF raw-value classifier rules.

    Older bundled sources express this as an ordered ``if`` chain, while the
    standalone implementation uses a switch over the canonical range table.
    They are intentionally different classifiers, so the rule form (not a
    generic predicate label) is retained in the source-only inventory.
    """
    switch_matches = list(re.finditer(r"\bswitch\s*\(", body.code))
    if switch_matches:
        switch = switch_body(body)
        label_pattern = re.compile(
            r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*|0[xX][0-9A-Fa-f]+|\d+)\s*:|\b(default)\s*:"
        )
        labels = []
        for match in label_pattern.finditer(switch.code):
            depth = switch.code[: match.start()].count("{") - switch.code[: match.start()].count("}")
            if depth != 1:
                continue
            labels.append((match, match.group(1) or "default"))
        if not labels:
            raise RouteError("raw DXF classifier switch has no cases")
        result: list[tuple[dict, int, str]] = []
        for match, label in labels:
            # Consecutive case labels intentionally share the first following
            # return (Bin/Str/Unknown all map to raw strings in the canonical
            # table), so do not stop at the next label.
            end = len(switch.code)
            returned = re.search(
                r"\breturn\s+(RawValType::[A-Za-z_]\w*)\s*;", switch.code[match.end() : end]
            )
            if returned is None:
                raise RouteError("raw DXF classifier label %s lacks one return type" % label)
            raw_value = returned.group(1)
            line = line_number(switch.source.text, switch.body_start + match.start())
            if label == "default":
                selector = {"kind": "fallback", "value": "default", "rawValue": raw_value}
            else:
                selector = {"kind": "case", "value": label, "rawValue": raw_value}
            result.append((selector, line, label))
        return result

    result = []
    pattern = re.compile(
        r"\b(?:if|else\s+if)\s*\(\s*([^(){};]+?)\s*\)\s*"
        r"return\s+(RawValType::[A-Za-z_]\w*)\s*;"
    )
    for ordinal, match in enumerate(pattern.finditer(body.code), 1):
        condition = " ".join(match.group(1).split())
        value = match.group(2)
        absolute = body.body_start + match.start(1)
        result.append(
            (
                {
                    "kind": "condition",
                    "ordinal": ordinal,
                    "condition": condition,
                    "value": value,
                },
                line_number(body.source.text, absolute),
                "condition-%03d" % ordinal,
            )
        )
    fallback_pattern = re.compile(r"\breturn\s+(RawValType::[A-Za-z_]\w*)\s*;")
    returns = list(fallback_pattern.finditer(body.code))
    if returns:
        fallback = returns[-1]
        absolute = body.body_start + fallback.start(1)
        result.append(
            (
                {"kind": "fallback", "value": fallback.group(1)},
                line_number(body.source.text, absolute),
                "fallback",
            )
        )
    if not result:
        raise RouteError("raw DXF classifier has no observable rules")
    return result


def drw_versions(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    for match in re.finditer(r"\bDRW::(AC\d+|UNKNOWNV)\b", body.code):
        absolute = body.body_start + match.start(1)
        result.append((match.group(1), line_number(body.source.text, absolute)))
    return result


def switch_case_outcomes(switch: FunctionBody) -> list[tuple[str, str, int]]:
    """Return direct ``case`` labels with their first direct terminal outcome."""
    label_pattern = re.compile(r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*)\s*:")
    labels: list[tuple[re.Match[str], str]] = []
    for match in label_pattern.finditer(switch.code):
        depth = switch.code[: match.start()].count("{") - switch.code[: match.start()].count("}")
        if depth == 1:
            labels.append((match, match.group(1)))
    if not labels:
        raise RouteError("switch has no direct case labels: %s" % switch.symbol)
    outcome_pattern = re.compile(r"\breturn\s+(.+?);|\bbreak\s*;", re.DOTALL)
    result: list[tuple[str, str, int]] = []
    for match, label in labels:
        outcome = outcome_pattern.search(switch.code, match.end())
        if outcome is None:
            raise RouteError("switch case %s has no terminal outcome" % label)
        raw = "break" if outcome.group(0).lstrip().startswith("break") else " ".join(
            outcome.group(1).split()
        )
        line = line_number(switch.source.text, switch.body_start + match.start(1))
        result.append((label, raw, line))
    return result


def constexpr_integer_values(source: SourceFile) -> dict[str, int]:
    values = {
        match.group(1): int(match.group(2), 0)
        for match in re.finditer(
            r"\bstatic\s+constexpr\s+int\s+([A-Za-z_]\w*)\s*=\s*(0[xX][0-9A-Fa-f]+|\d+)\s*;",
            source.code,
        )
    }
    if not values:
        raise RouteError("source has no constexpr integer values: %s" % source.path)
    return values


def switch_string_returns(body: FunctionBody, expression: str) -> list[tuple[str, str, int]]:
    """Extract direct switch labels and their immediately selected strings."""
    switch = switch_body(body, expression)
    pattern = re.compile(r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*|\d+)\s*:")
    result: list[tuple[str, str, int]] = []
    for match in pattern.finditer(switch.code):
        depth = switch.code[: match.start()].count("{") - switch.code[: match.start()].count("}")
        if depth != 1:
            continue
        returned = re.search(
            r'\breturn\s+"((?:\\.|[^"\\])*)"\s*;', switch.comment_text[match.end() :]
        )
        if returned is None:
            raise RouteError("switch case %s has no string outcome" % match.group(1))
        label = match.group(1)
        line = line_number(switch.source.text, switch.body_start + match.start(1))
        result.append((label, cpp_unquote(returned.group(1)), line))
    if not result:
        raise RouteError("switch has no direct string-return cases: %s" % body.symbol)
    return result


def predicates(body: FunctionBody, names: Iterable[str]) -> list[tuple[str, int]]:
    """Return complete open-ended condition expressions, never line fragments."""
    expression_names = tuple(names)
    result: list[tuple[str, int]] = []
    for match in re.finditer(r"\bif\s*\(", body.code):
        open_paren = body.code.find("(", match.start(), match.end() + 1)
        close_paren = matching_delimiter(body.code, open_paren, "(", ")")
        condition_code = body.code[open_paren + 1 : close_paren]
        if not any(re.search(r"\b" + re.escape(name) + r"\b", condition_code) for name in expression_names):
            continue
        if not re.search(r"\.\s*(?:find|rfind|starts_with)\s*\(", condition_code):
            continue
        normalized = " ".join(body.comment_text[open_paren + 1 : close_paren].split())
        if normalized:
            absolute = body.body_start + match.start()
            result.append((normalized, line_number(body.source.text, absolute)))
    return sorted(set(result), key=lambda item: (item[0], item[1]))


def named_class_literals(body: FunctionBody) -> list[tuple[str, int]]:
    result: list[tuple[str, int]] = []
    pattern = re.compile(r'\b(?:recName|className|rn|cn)\s*==\s*"((?:\\.|[^"\\])*)"')
    for match in pattern.finditer(body.comment_text):
        absolute = body.body_start + match.start(1)
        result.append((cpp_unquote(match.group(1)), line_number(body.source.text, absolute)))
    return sorted(set(result), key=lambda item: (item[0], item[1]))


def named_class_dispatches(body: FunctionBody) -> list[dict]:
    """Bind finite DWG class spellings to their publication callbacks."""
    code = body.code
    text = body.comment_text
    result: list[dict] = []
    for match in re.finditer(r"\bif\s*(?:constexpr\s*)?\(", code):
        open_paren = code.find("(", match.start(), match.end() + 1)
        close_paren = matching_delimiter(code, open_paren, "(", ")")
        condition_body = FunctionBody(
            body.source,
            body.symbol,
            body.body_start + open_paren,
            body.body_start + open_paren,
            body.body_start + close_paren,
        )
        literals = named_class_literals(condition_body)
        if not literals:
            continue
        statement_start, statement_end = statement_span(code, close_paren + 1)
        statement = code[statement_start : statement_end + 1]
        callbacks = sorted(
            set(
                (item.group(1) or item.group(2))
                for item in re.finditer(
                    r"&\s*DRW_Interface::(add[A-Za-z0-9_]+)\b|\bintfa\.(add[A-Za-z0-9_]+)\s*\(",
                    statement,
                )
            )
        )
        models = sorted(
            set(
                item.group(1)
                for item in re.finditer(r"\b(DRW_[A-Za-z0-9_]+)\s+e\b", statement)
            )
        )
        if not callbacks and not models:
            continue
        condition_text = " ".join(text[open_paren + 1 : close_paren].split())
        for name, _line in literals:
            result.append(
                {
                    "name": name,
                    "callbacks": callbacks,
                    "models": models,
                    "conditionFingerprint": sha256_text(condition_text),
                    "line": line_number(body.source.text, body.body_start + match.start()),
                }
            )
    return result


def statement_span(code: str, start: int) -> tuple[int, int]:
    """Return one C++ statement/body span beginning at ``start``.

    Dispatch registration uses short assignments in either a braced body or a
    single statement.  This small balanced scanner is intentionally narrower
    than a C++ parser, but it is enough to bind a condition to its selected
    ``process*`` implementation without scanning into the next branch.
    """
    index = start
    while index < len(code) and code[index].isspace():
        index += 1
    if index >= len(code):
        raise RouteError("expected statement after source offset %d" % start)
    if code[index] == "{":
        return index, matching_delimiter(code, index)
    depth = 0
    for end in range(index, len(code)):
        char = code[end]
        if char in "([":
            depth += 1
        elif char in ")]":
            depth -= 1
        elif char == ";" and depth == 0:
            return index, end
    raise RouteError("unterminated statement after source offset %d" % start)


def branch_literals(text: str, names: Iterable[str]) -> list[str]:
    """Return finite DXF/DWG literals from one already-selected condition."""
    result: list[str] = []
    name_pattern = "(?:%s)" % "|".join(re.escape(name) for name in names)
    keyword = re.compile(
        r'\bdxfKeywordEquals\s*\(\s*' + name_pattern
        + r'\s*,\s*"((?:\\.|[^"\\])*)"\s*\)'
    )
    direct = re.compile(
        r'\b' + name_pattern + r'\s*==\s*"((?:\\.|[^"\\])*)"'
    )
    reverse = re.compile(
        r'"((?:\\.|[^"\\])*)"\s*==\s*\b' + name_pattern
    )
    for pattern in (keyword, direct, reverse):
        result.extend(cpp_unquote(match.group(1)) for match in pattern.finditer(text))
    return sorted(set(result))


def dispatch_branches(
    body: FunctionBody,
    *,
    names: Iterable[str],
    assignment: str = "processed",
    grammar_literals: Iterable[str] = (),
) -> list[dict]:
    """Bind selected ``if``/``else`` dispatch branches to ``process*`` calls.

    Finite equality spellings are deliberately kept separate from open-ended
    prefix/dynamic conditions.  Each branch carries a stable ordinal so an
    implementation changing priority cannot hide behind the same name set.
    """
    names_tuple = tuple(names)
    grammar = set(grammar_literals)
    code = body.code
    text = body.comment_text
    branch_rows: list[dict] = []
    if_pattern = re.compile(r"\bif\s*\(")
    target_pattern = re.compile(
        r"\b" + re.escape(assignment) + r"\s*=\s*(process[A-Za-z0-9_]*)\s*\("
    )

    def direct_targets(start: int, end: int) -> list[re.Match[str]]:
        """Ignore registrations nested inside a control-flow substatement."""
        brace_depth = code[:start].count("{") - code[:start].count("}")
        wanted_depth = brace_depth + 1 if code[start] == "{" else brace_depth
        return [
            candidate
            for candidate in target_pattern.finditer(code, start, end + 1)
            if code[: candidate.start()].count("{") - code[: candidate.start()].count("}")
            == wanted_depth
        ]

    for match in if_pattern.finditer(code):
        open_paren = code.find("(", match.start(), match.end() + 1)
        close_paren = matching_delimiter(code, open_paren, "(", ")")
        condition_code = code[open_paren + 1 : close_paren]
        if not any(re.search(r"\b" + re.escape(name) + r"\b", condition_code) for name in names_tuple):
            continue
        statement_start, statement_end = statement_span(code, close_paren + 1)
        targets = direct_targets(statement_start, statement_end)
        if not targets:
            nested_targets = list(target_pattern.finditer(code, statement_start, statement_end + 1))
            if nested_targets:
                # This is a grammar/guard condition enclosing a later
                # dispatch chain, not an individual dispatch branch.
                continue
            unbound_literals = set(branch_literals(text[open_paren + 1 : close_paren], names_tuple))
            if unbound_literals and unbound_literals <= grammar:
                continue
            raise RouteError(
                "selected dispatch condition in %s is not bound to a %s=process* target"
                % (body.symbol, assignment)
            )
        if len(targets) != 1:
            raise RouteError(
                "dispatch condition in %s does not bind exactly one %s=process* target"
                % (body.symbol, assignment)
            )
        target = targets[0].group(1)
        condition_text = " ".join(text[open_paren + 1 : close_paren].split())
        literals = branch_literals(text[open_paren + 1 : close_paren], names_tuple)
        open_ended = bool(
            re.search(r"\.\s*(?:find|rfind|starts_with)\s*\(", condition_code)
            or re.search(r"\b[A-Za-z_]\w*(?:::[A-Za-z_]\w*)+\s*\(", condition_code)
        )
        branch_rows.append(
            {
                "position": match.start(),
                "line": line_number(body.source.text, body.body_start + match.start()),
                "target": target,
                "literals": literals,
                "conditionFingerprint": sha256_text(condition_text),
                "matchKind": (
                    "mixed" if literals and open_ended else "exact" if literals else "open-ended"
                ),
                "openEnded": open_ended,
                "fallback": False,
            }
        )

    # The terminal raw/unknown branch normally has no selector variable, so
    # bind it separately.  Constrain it to the shallowest relevant chain depth
    # to avoid an unrelated inner ``else`` becoming a dispatch fallback.
    if branch_rows:
        branch_depth = min(
            code[: row["position"]].count("{") - code[: row["position"]].count("}")
            for row in branch_rows
        )
        else_pattern = re.compile(r"\belse\b(?!\s*if\b)")
        for match in else_pattern.finditer(code):
            depth = code[: match.start()].count("{") - code[: match.start()].count("}")
            if depth != branch_depth:
                continue
            statement_start, statement_end = statement_span(code, match.end())
            targets = direct_targets(statement_start, statement_end)
            if not targets:
                continue
            if len(targets) != 1:
                raise RouteError(
                    "dispatch fallback in %s does not bind exactly one %s=process* target"
                    % (body.symbol, assignment)
                )
            target = targets[0].group(1)
            branch_rows.append(
                {
                    "position": match.start(),
                    "line": line_number(body.source.text, body.body_start + match.start()),
                    "target": target,
                    "literals": [],
                    "conditionFingerprint": sha256_text("fallback:" + target),
                    "matchKind": "fallback",
                    "openEnded": False,
                    "fallback": True,
                }
            )

    branch_rows.sort(key=lambda row: row["position"])
    for ordinal, row in enumerate(branch_rows, 1):
        row["ordinal"] = ordinal
    if not branch_rows:
        raise RouteError("dispatch anchor has no selected branches: %s" % body.symbol)
    return branch_rows


def evidence(body: FunctionBody, *, line: int | None = None) -> dict:
    return {
        "path": body.source.path,
        "line": body.line if line is None else line,
        "symbol": body.symbol,
        "spanSha256": body.span_sha256,
    }


class RouteCollector:
    def __init__(self) -> None:
        self._routes: dict[str, dict] = {}

    def add(
        self,
        facade: str,
        category: str,
        selector: dict,
        body: FunctionBody,
        *,
        identifier: str,
        directions: Iterable[str],
        line: int | None = None,
    ) -> None:
        route_id = "%s/%s/%s" % (facade, category, normalized_identifier(identifier))
        candidate = {
            "id": route_id,
            "facade": facade,
            "category": category,
            "selector": selector,
            "directions": sorted(set(directions)),
            "sourceDisposition": "source-route-only",
            "evidence": [evidence(body, line=line)],
        }
        existing = self._routes.get(route_id)
        if existing is None:
            self._routes[route_id] = candidate
            return
        for field in ("facade", "category", "selector", "directions", "sourceDisposition"):
            if existing[field] != candidate[field]:
                raise RouteError("conflicting route definition for %s" % route_id)
        existing["evidence"].extend(candidate["evidence"])

    def routes(self) -> list[dict]:
        result: list[dict] = []
        for route_id in sorted(self._routes):
            route = self._routes[route_id]
            route["evidence"] = sorted(
                {
                    (item["path"], item["line"], item["symbol"], item["spanSha256"]): item
                    for item in route["evidence"]
                }.values(),
                key=lambda item: (item["path"], item["line"], item["symbol"], item["spanSha256"]),
            )
            result.append(route)
        return result


@dataclass
class SourceTree:
    label: str
    files: dict[str, SourceFile]
    metadata: dict

    def require(self, path: str) -> SourceFile:
        try:
            return self.files[path]
        except KeyError as exc:
            raise RouteError("%s source tree lacks %s" % (self.label, path)) from exc


def manifest_entries(path: Path) -> dict[str, tuple[str, str, str]]:
    result: dict[str, tuple[str, str, str]] = {}
    for number, line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        if not line or line.startswith("#"):
            continue
        fields = line.split("|")
        if len(fields) != MANIFEST_FIELDS or not all(fields):
            raise RouteError("malformed source manifest line %d" % number)
        name, mode, blob, classification = fields
        if name in result or not HEX_RE.fullmatch(blob):
            raise RouteError("invalid source manifest entry on line %d" % number)
        result[name] = (mode, blob, classification)
    if not result:
        raise RouteError("source manifest is empty")
    return result


def parse_cmake_public_headers(text: str, source_name: str) -> tuple[str, ...]:
    """Extract the explicit installed-header list from a locked CMake blob."""
    matches = list(
        re.finditer(
            r"(?ms)^\s*set\s*\(\s*LIBDXFRW_PUBLIC_HEADERS\b(.*?)^\s*\)",
            text,
        )
    )
    if len(matches) != 1:
        raise RouteError(
            "%s must contain exactly one LIBDXFRW_PUBLIC_HEADERS set block; found %d"
            % (source_name, len(matches))
        )
    body = matches[0].group(1)
    values = re.findall(r'"([^"]+)"', body)
    residue = re.sub(r'"[^"]+"', "", body).strip()
    if residue:
        raise RouteError("%s public-header block has unsupported CMake syntax" % source_name)
    prefix = "${CMAKE_CURRENT_LIST_DIR}/"
    paths = []
    for value in values:
        if not value.startswith(prefix):
            raise RouteError("%s public-header entry is not source-relative: %s" % (source_name, value))
        path = value[len(prefix) :]
        if not path.startswith("src/") or not path.endswith(".h"):
            raise RouteError("%s public-header entry is not a src header: %s" % (source_name, path))
        paths.append(path)
    if not paths or len(paths) != len(set(paths)):
        raise RouteError("%s public-header list is empty or contains duplicates" % source_name)
    return tuple(paths)


def target_public_header_paths(root: Path, target_repo: Path, source_lock: dict, tree: SourceTree) -> tuple[str, ...]:
    """Read only the pinned target CMake blob which declares installed headers."""
    manifest = manifest_entries(root / "metadata/libdxfrw-target-source-manifest.txt")
    try:
        _mode, expected_blob, _classification = manifest[TARGET_SOURCES_CMAKE]
    except KeyError as exc:
        raise RouteError("target manifest omits %s" % TARGET_SOURCES_CMAKE) from exc
    commit = source_lock["libreCAD"]["commit"]
    observed_blob = run(
        ["git", "-C", str(target_repo), "rev-parse", "%s:%s" % (commit, TARGET_SOURCES_CMAKE)]
    ).strip()
    if observed_blob != expected_blob:
        raise RouteError("target public-header CMake blob disagrees with source manifest")
    content = run_bytes(
        ["git", "-C", str(target_repo), "show", "%s:%s" % (commit, TARGET_SOURCES_CMAKE)]
    )
    if b"\0" in content:
        raise RouteError("target public-header CMake blob contains a NUL")
    try:
        paths = parse_cmake_public_headers(content.decode("utf-8"), TARGET_SOURCES_CMAKE)
    except UnicodeDecodeError as exc:
        raise RouteError("target public-header CMake blob is not UTF-8") from exc
    missing = sorted(set(paths) - set(tree.files))
    if missing:
        raise RouteError("target public-header list names unloaded source files: %s" % missing)
    return paths


def git_dir(target_repo: Path) -> Path:
    value = run(["git", "-C", str(target_repo), "rev-parse", "--git-dir"]).strip()
    path = Path(value)
    return path if path.is_absolute() else (target_repo / path).resolve()


def verify_target_inputs(root: Path, target_repo: Path, source_lock: dict) -> None:
    source_lock_path = root / "metadata/libdxfrw-target-lock.json"
    manifest = root / "metadata/libdxfrw-target-source-manifest.txt"
    ensure_command(
        [
            sys.executable,
            str(root / "tools/check_parity_inventory_inputs.py"),
            "--target-repo",
            str(target_repo),
        ],
        cwd=root,
    )
    ensure_command(
        [
            sys.executable,
            str(root / "tools/check_libdxfrw_sync.py"),
            "--target-git-dir",
            str(git_dir(target_repo)),
            "--target-commit",
            source_lock["libreCAD"]["commit"],
            "--standalone-commit",
            source_lock["standalone"]["commit"],
            "--manifest",
            str(manifest),
            "--lock",
            str(source_lock_path),
        ],
        cwd=root,
    )


def load_target_tree(root: Path, target_repo: Path, source_lock: dict) -> SourceTree:
    manifest = manifest_entries(root / "metadata/libdxfrw-target-source-manifest.txt")
    commit = source_lock["libreCAD"]["commit"]
    target_entries = {
        name: value
        for name, value in manifest.items()
        if name.startswith(SOURCE_ROOT + "/") and name.endswith((".cpp", ".h"))
    }
    if not target_entries:
        raise RouteError("source manifest has no target source files")
    files: dict[str, SourceFile] = {}
    for target_path, (mode, blob, _classification) in sorted(target_entries.items()):
        relative = "src/" + target_path[len(SOURCE_ROOT) + 1 :]
        content = run_bytes(["git", "-C", str(target_repo), "show", "%s:%s" % (commit, target_path)])
        if b"\0" in content:
            raise RouteError("target source contains a NUL byte: %s" % target_path)
        try:
            text = content.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise RouteError("target source is not UTF-8: %s" % target_path) from exc
        if mode not in {"100644", "100755"}:
            raise RouteError("unexpected target source mode for %s" % target_path)
        files[relative] = SourceFile(relative, text, sha256_bytes(content), blob, mode)
    return SourceTree(
        "target",
        files,
        {
            "repository": source_lock["libreCAD"].get("repository"),
            "commit": commit,
            "snapshotRevision": source_lock["libreCAD"]["snapshotRevision"],
            "sourceRoot": SOURCE_ROOT,
        },
    )


def allowed_source_extras(root: Path) -> set[str]:
    allowlist = read_json(root / "metadata/adaptation-allowlist.json")
    values = allowlist.get("allowedPaths")
    if not isinstance(values, list) or any(not isinstance(value, str) for value in values):
        raise RouteError("adaptation allowlist has invalid allowedPaths")
    return set(values)


def is_allowed_extra(path: str, allowed: set[str]) -> bool:
    return any(path == entry or (entry.endswith("/") and path.startswith(entry)) for entry in allowed)


def content_tree_sha256(files: dict[str, SourceFile]) -> str:
    digest = hashlib.sha256()
    for path in sorted(files):
        digest.update(path.encode("utf-8"))
        digest.update(b"\0")
        digest.update((files[path].mode or "unknown").encode("ascii"))
        digest.update(b"\0")
        digest.update(files[path].sha256.encode("ascii"))
        digest.update(b"\n")
    return digest.hexdigest()


def load_standalone_tree(root: Path, target_paths: set[str]) -> SourceTree:
    source_root = root / "src"
    files: dict[str, SourceFile] = {}
    for path in sorted(source_root.rglob("*")):
        if path.is_symlink():
            raise RouteError("standalone source tree contains a symlink: %s" % path.relative_to(root))
        if not path.is_file() or path.suffix not in {".cpp", ".h"}:
            continue
        relative = path.relative_to(root).as_posix()
        content = path.read_bytes()
        if b"\0" in content:
            raise RouteError("standalone source contains a NUL byte: %s" % relative)
        try:
            text = content.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise RouteError("standalone source is not UTF-8: %s" % relative) from exc
        mode = "100755" if path.stat().st_mode & 0o111 else "100644"
        files[relative] = SourceFile(relative, text, sha256_bytes(content), None, mode)
    paths = set(files)
    target_only = sorted(target_paths - paths)
    standalone_only = sorted(paths - target_paths)
    if target_only:
        raise RouteError(
            "standalone source-path closure omits target paths: %s" % target_only
        )
    allowed = allowed_source_extras(root)
    unexplained = [path for path in standalone_only if not is_allowed_extra(path, allowed)]
    if unexplained:
        raise RouteError("standalone source extras lack an adaptation allowlist entry: %s" % unexplained)
    return SourceTree(
        "standalone",
        files,
        {
            "sourceRoot": "src",
            "mode": "working-tree-content",
            "contentTreeSha256": content_tree_sha256(files),
            "targetOnlySourcePaths": target_only,
            "standaloneOnlySourcePaths": standalone_only,
        },
    )


def source_file_fingerprints(tree: SourceTree) -> list[dict]:
    result: list[dict] = []
    for path in sorted(tree.files):
        source = tree.files[path]
        value = {"path": path, "sha256": source.sha256}
        if source.mode is not None:
            value["mode"] = source.mode
        if source.git_blob is not None:
            value["gitBlob"] = source.git_blob
        result.append(value)
    return result


def source_unit_roles_for(tree: SourceTree) -> dict[str, str]:
    """Return the exact reviewed source-unit role map for one inventory side."""
    expected = dict(SOURCE_UNIT_ROLES)
    if tree.label == "standalone":
        expected.update(STANDALONE_SOURCE_UNIT_ROLES)
    unexpected = sorted(set(tree.files) - set(expected))
    missing = sorted(set(expected) - set(tree.files))
    if unexpected or missing:
        raise RouteError(
            "%s source-unit contract changed: unexpected=%s missing=%s"
            % (tree.label, unexpected, missing)
        )
    return expected


def source_unit_body(source: SourceFile) -> FunctionBody:
    if not source.text:
        raise RouteError("source-unit is unexpectedly empty: %s" % source.path)
    return FunctionBody(source, source.path, 0, 0, len(source.text) - 1)


def add_source_units(collector: RouteCollector, tree: SourceTree) -> None:
    """Emit one classified source-surface route for every locked source file.

    This is deliberately distinct from the source-file fingerprint list.  A
    source unit is classified by its execution role and says whether I0.2b must
    add method/pipeline edges for it; only generated codepage tables and the
    documentation header are narrow supporting exceptions.
    """
    roles = source_unit_roles_for(tree)
    for path in sorted(roles):
        role = roles[path]
        classification = role if role in SUPPORTING_SOURCE_UNIT_ROLES else "functional"
        collector.add(
            "shared",
            "source-unit",
            {
                "kind": "source-unit",
                "path": path,
                "role": role,
                "classification": classification,
                "requiresPipelineEdges": role not in SUPPORTING_SOURCE_UNIT_ROLES,
            },
            source_unit_body(tree.require(path)),
            identifier=path,
            directions=["source-inventory"],
            line=1,
        )


def class_definition_anchor(tree: SourceTree, name: str) -> FunctionBody:
    """Locate exactly one complete class/struct definition for a pipeline node."""
    candidates: list[FunctionBody] = []
    pattern = re.compile(
        r"\b(?:class|struct)\s+" + re.escape(name) + r"\b(?!\s*::)"
    )
    for source in tree.files.values():
        for match in pattern.finditer(source.code):
            opening = source.code.find("{", match.end())
            semicolon = source.code.find(";", match.end())
            if opening < 0 or (semicolon >= 0 and semicolon < opening):
                continue
            candidates.append(
                FunctionBody(
                    source,
                    name,
                    match.start(),
                    opening,
                    matching_delimiter(source.code, opening),
                )
            )
    if len(candidates) != 1:
        raise RouteError(
            "expected exactly one complete class/struct definition for %s; found %d"
            % (name, len(candidates))
        )
    return candidates[0]


def class_definition_parent(body: FunctionBody) -> str | None:
    """Return the direct public base spelling for one complete class body."""
    declaration = body.source.code[body.symbol_start : body.body_start]
    match = re.search(r":\s*public\s+([A-Za-z_][A-Za-z0-9_]*)\s*$", declaration)
    if match is None:
        if ":" in declaration:
            raise RouteError("unsupported class inheritance declaration for %s" % body.symbol)
        return None
    return match.group(1)


def inline_class_method_body(source: SourceFile, qualified_symbol: str) -> FunctionBody:
    """Resolve one inline method body while retaining its qualified identity."""
    if "::" not in qualified_symbol:
        raise RouteError("inline class-method anchor is not qualified: %s" % qualified_symbol)
    class_name, method = qualified_symbol.rsplit("::", 1)
    class_body = class_definition_anchor(
        SourceTree("inline-anchor", {source.path: source}, {}), class_name
    )
    candidates: list[FunctionBody] = []
    pattern = re.compile(r"\b" + re.escape(method) + r"\s*\(")
    start = class_body.body_start
    end = class_body.body_end + 1
    for match in pattern.finditer(source.code, start, end):
        opening = source.code.find("(", match.start(), match.end())
        candidate = body_after_signature(source, qualified_symbol, match.start(), opening)
        if candidate is None:
            continue
        if candidate.body_start < class_body.body_start or candidate.body_end > class_body.body_end:
            continue
        candidates.append(candidate)
    if len(candidates) != 1:
        raise RouteError(
            "expected exactly one inline definition for %s in %s; found %d"
            % (qualified_symbol, source.path, len(candidates))
        )
    return candidates[0]


def model_qualified_definitions(source: SourceFile) -> list[FunctionBody]:
    """Return direct ``DRW_*`` member definitions from one source unit.

    This narrow lexer intentionally inventories only parse/encode/write
    implementations later; it does not try to understand C++ bodies or infer
    a format capability from a declaration.
    """
    pattern = re.compile(
        r"(?<![A-Za-z0-9_:])"
        r"(DRW_[A-Za-z0-9_]+(?:\s*::\s*[A-Za-z0-9_]+)+)\s*\("
    )
    result: list[FunctionBody] = []
    seen: set[tuple[str, int]] = set()
    for match in pattern.finditer(source.code):
        if not declaration_context(source, match.start()):
            continue
        symbol = re.sub(r"\s*::\s*", "::", match.group(1))
        open_paren = source.code.find("(", match.start(), match.end() + 1)
        candidate = body_after_signature(source, symbol, match.start(), open_paren)
        if candidate is not None and (symbol, candidate.symbol_start) not in seen:
            result.append(candidate)
            seen.add((symbol, candidate.symbol_start))
    return result


def public_model_declaration_bodies(tree: SourceTree) -> dict[str, list[FunctionBody]]:
    """Mirror the indexed ``public-model`` scan with usable declaration evidence."""
    model_headers = sorted(
        path
        for path in tree.files
        if path.startswith("src/drw_") and path.endswith(".h")
    ) + ["src/libdwgr.h", "src/libdxfrw.h"]
    result: dict[str, list[FunctionBody]] = {}
    for path in model_headers:
        source = tree.require(path)
        for match in re.finditer(r"\b(?:class|struct)\s+(DRW_[A-Za-z0-9_]+)\b", source.code):
            model = match.group(1)
            result.setdefault(model, []).append(
                FunctionBody(source, model, match.start(1), match.start(1), match.end(1) - 1)
            )
    if not result:
        raise RouteError("public model declaration scan produced no DRW_ types")
    return result


def model_codec_operation(name: str) -> str | None:
    """Classify only direct parser/encoder method spellings, never their wire semantics."""
    if name == "parseCode":
        return "dxf-parse"
    if name.startswith("parseDwg"):
        return "dwg-parse"
    if name.startswith("parse"):
        return "parse-helper"
    if name.startswith("encodeDwg") or name.startswith("writeDwg"):
        return "dwg-encode"
    if name.startswith("write"):
        return "dxf-write"
    return None


def direct_model_codec_operations(tree: SourceTree) -> dict[str, list[tuple[str, FunctionBody]]]:
    """Collect direct parser/encoder definitions by their outer ``DRW_*`` type."""
    result: dict[str, list[tuple[str, FunctionBody]]] = {}
    for source in tree.files.values():
        for body in model_qualified_definitions(source):
            components = body.symbol.split("::")
            model = components[0]
            operation = model_codec_operation(components[-1])
            if operation is not None:
                result.setdefault(model, []).append((operation, body))
    return result


def add_pipeline_unit_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Attach every functional source unit to exactly one reviewed pipeline area."""
    for path, role in sorted(source_unit_roles_for(tree).items()):
        if role in SUPPORTING_SOURCE_UNIT_ROLES:
            continue
        try:
            stage = PIPELINE_STAGE_BY_SOURCE_ROLE[role]
        except KeyError as exc:
            raise RouteError("functional source-unit has no pipeline stage: %s" % path) from exc
        collector.add(
            "shared",
            "pipeline-unit",
            {
                "kind": "source-unit-pipeline",
                "path": path,
                "role": role,
                "stage": stage,
                "sourceUnit": "shared/source-unit/" + normalized_identifier(path),
            },
            source_unit_body(tree.require(path)),
            identifier=path,
            directions=["pipeline"],
            line=1,
        )


def add_model_codec_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Record direct model parse/encode/write definitions for every public model.

    An empty operation list means that the model has no direct definition in
    the locked source surface; it is deliberately not an unsupported-feature
    disposition because inheritance and caller-level composition are resolved
    later by the mapping/evidence work.
    """
    declarations = public_model_declaration_bodies(tree)
    operations: dict[str, list[tuple[str, FunctionBody]]] = {
        model: [] for model in declarations
    }
    direct_operations = direct_model_codec_operations(tree)
    for model in operations:
        operations[model].extend(direct_operations.get(model, []))
    for model in sorted(declarations):
        op_rows = [
            {
                "operation": operation,
                "symbol": body.symbol,
                "signatureFingerprint": signature_fingerprint(body),
            }
            for operation, body in operations[model]
        ]
        op_rows.sort(
            key=lambda row: (row["operation"], row["symbol"], row["signatureFingerprint"])
        )
        selector = {
            "kind": "direct-model-codec",
            "model": model,
            "declarationRoute": "shared/public-model/" + normalized_identifier(model),
            "operations": op_rows,
            "directDefinitionStatus": "present" if op_rows else "no-direct-definition",
        }
        directions = sorted(
            {
                "read" if row["operation"] in {"dxf-parse", "dwg-parse", "parse-helper"} else "write"
                for row in op_rows
            }
        ) or ["model"]
        collector.add(
            "shared",
            "model-codec",
            selector,
            declarations[model][0],
            identifier=model,
            directions=directions,
        )
        for _operation, body in operations[model]:
            collector.add(
                "shared",
                "model-codec",
                selector,
                body,
                identifier=model,
                directions=directions,
            )

    internal_models = set(direct_operations) - set(declarations)
    if internal_models != INTERNAL_MODEL_CODEC_TYPES:
        raise RouteError(
            "internal model codec set changed without review: expected %s, observed %s"
            % (sorted(INTERNAL_MODEL_CODEC_TYPES), sorted(internal_models))
        )
    for model in sorted(INTERNAL_MODEL_CODEC_TYPES):
        op_rows = [
            {
                "operation": operation,
                "symbol": body.symbol,
                "signatureFingerprint": signature_fingerprint(body),
            }
            for operation, body in direct_operations[model]
        ]
        op_rows.sort(
            key=lambda row: (row["operation"], row["symbol"], row["signatureFingerprint"])
        )
        if not op_rows:
            raise RouteError("internal model codec has no direct operation: %s" % model)
        selector = {
            "kind": "direct-internal-model-codec",
            "model": model,
            "scope": "internal",
            "operations": op_rows,
        }
        body = class_definition_anchor(tree, model)
        collector.add(
            "shared",
            "internal-model-codec",
            selector,
            body,
            identifier=model,
            directions=["read"],
        )
        for _operation, operation_body in direct_operations[model]:
            collector.add(
                "shared",
                "internal-model-codec",
                selector,
                operation_body,
                identifier=model,
                directions=["read"],
            )


def add_helper_subsystem_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Add reviewed, named cross-cutting subsystem edges."""
    for subsystem, paths in sorted(HELPER_SUBSYSTEM_PATHS.items()):
        if not paths:
            raise RouteError("helper subsystem has no source paths: %s" % subsystem)
        selector = {
            "kind": "helper-subsystem",
            "name": subsystem,
            "paths": list(paths),
        }
        first = source_unit_body(tree.require(paths[0]))
        collector.add(
            "shared",
            "helper-subsystem",
            selector,
            first,
            identifier=subsystem,
            directions=["pipeline-helper"],
            line=1,
        )
        for path in paths[1:]:
            collector.add(
                "shared",
                "helper-subsystem",
                selector,
                source_unit_body(tree.require(path)),
                identifier=subsystem,
                directions=["pipeline-helper"],
                line=1,
            )


def transport_branch_contains(
    source: str, guard_offset: int, construction_offset: int, branch: str
) -> bool:
    """Check that a constructor lies in the selected direct if/else arm."""
    if branch == "unconditional-entrypoint":
        return True
    candidates = []
    for match in re.finditer(r"\bif\s*\(", source):
        opening = source.find("(", match.start(), match.end())
        closing = matching_delimiter(source, opening, "(", ")")
        if match.start() <= guard_offset < closing:
            candidates.append((opening, closing))
    if len(candidates) != 1:
        return False
    _opening, closing = candidates[0]
    then_start, then_end = statement_span(source, closing + 1)
    if branch == "then":
        return then_start <= construction_offset <= then_end
    if branch != "else":
        return False
    else_match = re.match(r"\s*else\b", source[then_end + 1 :])
    if else_match is None:
        return False
    else_start = then_end + 1 + else_match.end()
    else_body_start, else_body_end = statement_span(source, else_start)
    return else_body_start <= construction_offset <= else_body_end


def transport_selection_metadata(
    body: FunctionBody, selection: str, implementation: str
) -> dict:
    """Prove one transport constructor and its exact branch predicates."""
    try:
        rule = DXF_TRANSPORT_SELECTION_RULES[selection]
    except KeyError as exc:
        raise RouteError("transport selection lacks a reviewed rule: %s" % selection) from exc
    source = body.comment_text
    guards: list[dict] = []
    previous_offset = -1
    for order, pattern in enumerate(rule["guardPatterns"], 1):
        matches = list(re.finditer(pattern, source))
        if len(matches) != 1:
            raise RouteError(
                "transport guard is missing or ambiguous in %s: %s"
                % (body.symbol, selection)
            )
        match = matches[0]
        if match.start() <= previous_offset:
            raise RouteError("transport guard order changed: %s" % selection)
        previous_offset = match.start()
        guards.append(
            {
                "order": order,
                "sourceOffset": match.start(),
                "predicateFingerprint": sha256_text(
                    " ".join(source[match.start() : match.end()].split())
                ),
                "sourceEvidence": _source_location(
                    body,
                    line_number(body.source.text, body.body_start + match.start()),
                ),
            }
        )
    constructions = list(re.finditer(rule["construction"], source))
    if len(constructions) != 1:
        raise RouteError(
            "transport constructor is missing or ambiguous in %s: %s"
            % (body.symbol, implementation)
        )
    construction = constructions[0]
    if guards and construction.start() <= guards[-1]["sourceOffset"]:
        raise RouteError("transport constructor precedes its guard: %s" % selection)
    if not transport_branch_contains(
        source,
        guards[-1]["sourceOffset"] if guards else construction.start(),
        construction.start(),
        rule["branch"],
    ):
        raise RouteError("transport constructor is outside its selected branch: %s" % selection)
    return {
        "selection": selection,
        "implementation": implementation,
        "branch": rule["branch"],
        "guardPredicates": guards,
        "construction": {
            "sourceOffset": construction.start(),
            "sourceEvidence": _source_location(
                body,
                line_number(body.source.text, body.body_start + construction.start()),
            ),
        },
    }


def add_dxf_transport_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Inventory the concrete ASCII/binary/R12 transport nodes and façade gates."""
    for transport_kind, path, symbol in DXF_TRANSPORT_TYPES:
        source = tree.require(path)
        # A complete definition (rather than a forward declaration) makes an
        # accidental transport removal fail during extraction.
        body = class_definition_anchor(SourceTree(tree.label, {path: source}, {}), symbol)
        collector.add(
            "dxfRW",
            "dxf-transport-node",
            {
                "kind": "transport-type",
                "transport": transport_kind,
                "symbol": symbol,
                "sourcePath": path,
            },
            body,
            identifier=transport_kind,
            directions=["read"] if transport_kind.startswith("reader") else ["write"],
        )
    facade_source = tree.require("src/libdxfrw.cpp")
    for selection, symbol, implementation, directions in DXF_TRANSPORT_SELECTIONS:
        body = function_body(facade_source, symbol)
        if not re.search(
            r"\bstd::make_unique\s*<\s*" + re.escape(implementation) + r"\s*>",
            body.code,
        ):
            raise RouteError(
                "DXF transport selection %s does not construct %s" % (selection, implementation)
            )
        collector.add(
            "dxfRW",
            "dxf-transport-node",
            {
                "kind": "transport-selection",
                "selection": selection,
                "entrypoint": symbol,
                "implementation": implementation,
                "signatureFingerprint": signature_fingerprint(body),
                "transportSelectionEvidence": transport_selection_metadata(
                    body, selection, implementation
                ),
            },
            body,
            identifier=selection,
            directions=directions,
        )
    if tree.label == "standalone":
        source = tree.require("src/intern/dxfcode.h")
        collector.add(
            "dxfRW",
            "dxf-transport-node",
            {
                "kind": "group-code-range-source",
                "sourcePath": source.path,
                "standaloneAdaptation": True,
            },
            source_unit_body(source),
            identifier="group-code-range-source",
            directions=["read", "write", "raw-capture"],
            line=1,
        )


def _pipeline_version_routes(collector: RouteCollector, category: str) -> list[dict]:
    return [
        route
        for route in collector.routes()
        if route["facade"] == "dwgRW" and route["category"] == category
    ]


def add_dwg_version_pipeline_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Link all factory versions to reviewed reader/writer inheritance nodes."""
    facade_source = tree.require("src/libdwgr.cpp")
    reader_factory = function_body(facade_source, "dwgRW::createReaderForVersion")
    writer_factory = function_body(facade_source, "dwgRW::write")
    reader_versions = [
        route
        for route in _pipeline_version_routes(collector, "reader-version")
        if route["selector"].get("outcome") == "supported"
    ]
    writer_versions = [
        route
        for route in _pipeline_version_routes(collector, "writer-version")
        if route["selector"].get("outcome") == "supported"
    ]

    def add_family(
        family: dict[str, dict],
        routes: list[dict],
        implementation_key: str,
        category: str,
        kind: str,
        factory: FunctionBody,
        directions: list[str],
    ) -> None:
        expected_implementations = {
            definition[implementation_key] for definition in family.values() if definition["parent"] is not None
        }
        observed_implementations = {route["selector"].get(implementation_key) for route in routes}
        if observed_implementations != expected_implementations:
            raise RouteError(
                "%s factory implementation set changed: expected %s, observed %s"
                % (category, sorted(expected_implementations), sorted(observed_implementations))
            )
        for identifier, definition in family.items():
            implementation = definition[implementation_key]
            matched = [
                route for route in routes if route["selector"].get(implementation_key) == implementation
            ]
            versions = sorted(route["selector"].get("version") for route in matched)
            route_ids = sorted(route["id"] for route in matched)
            class_body = class_definition_anchor(tree, implementation)
            if class_definition_parent(class_body) != definition["parent"]:
                raise RouteError(
                    "%s inheritance changed: expected %s, observed %s"
                    % (implementation, definition["parent"], class_definition_parent(class_body))
                )
            selector = {
                "kind": kind,
                implementation_key: implementation,
                "parent": definition["parent"],
                "factoryVersions": versions,
                "factoryRoutes": route_ids,
                "sourcePaths": list(definition["paths"]),
            }
            collector.add(
                "dwgRW",
                category,
                selector,
                class_body,
                identifier=identifier,
                directions=directions,
            )
            for path in definition["paths"]:
                collector.add(
                    "dwgRW",
                    category,
                    selector,
                    source_unit_body(tree.require(path)),
                    identifier=identifier,
                    directions=directions,
                    line=1,
                )
            for _route in matched:
                collector.add(
                    "dwgRW",
                    category,
                    selector,
                    factory,
                    identifier=identifier,
                    directions=directions,
                )

    add_family(
        DWG_READER_PIPELINES,
        reader_versions,
        "reader",
        "reader-pipeline",
        "reader-pipeline",
        reader_factory,
        ["read", "publish"],
    )
    add_family(
        DWG_WRITER_PIPELINES,
        writer_versions,
        "writer",
        "writer-pipeline",
        "writer-pipeline",
        writer_factory,
        ["write"],
    )


def raw_flow_route_id(facade: str, name: str) -> str:
    return "%s/raw-flow/%s" % (facade, normalized_identifier(name))


def raw_flow_inputs_by_name() -> dict[str, tuple[str, ...]]:
    """Derive reciprocal raw-flow inputs and reject dangling local names."""
    nodes = {node["name"]: node for node in RAW_FLOW_NODES}
    if len(nodes) != len(RAW_FLOW_NODES):
        raise RouteError("raw-flow node names are not unique")
    inputs: dict[str, list[str]] = {name: [] for name in nodes}
    for name, node in nodes.items():
        for output in node["outputs"]:
            if output not in nodes:
                raise RouteError("raw-flow output is not a declared node: %s -> %s" % (name, output))
            if nodes[output]["flow"] != node["flow"] or nodes[output]["facade"] != node["facade"]:
                raise RouteError("raw-flow output crosses an unreviewed flow: %s -> %s" % (name, output))
            inputs[output].append(name)
    return {name: tuple(sorted(values)) for name, values in inputs.items()}


def named_lambda_body(source: SourceFile, qualified_symbol: str) -> FunctionBody:
    """Resolve a named local lambda as a narrow source anchor.

    Raw DWG object construction happens in two named lambdas rather than a
    free/member function.  Keeping the enclosing symbol plus lambda name in
    the route identity prevents a whole dispatcher body from masquerading as
    an exact capture edge.
    """
    match = re.fullmatch(r"(.+)::lambda\(([A-Za-z_][A-Za-z0-9_]*)\)", qualified_symbol)
    if match is None:
        raise RouteError("invalid named-lambda anchor: %s" % qualified_symbol)
    enclosing, name = match.groups()
    outer = function_body(source, enclosing)
    declaration = re.search(
        r"\bauto\s+" + re.escape(name) + r"\s*=\s*", outer.code
    )
    if declaration is None:
        raise RouteError("named-lambda anchor is missing: %s" % qualified_symbol)
    start = outer.body_start + declaration.start()
    capture = outer.code.find("[", declaration.end())
    if capture < 0:
        raise RouteError("named-lambda capture is missing: %s" % qualified_symbol)
    parameters = outer.code.find("(", capture)
    if parameters < 0:
        raise RouteError("named-lambda parameters are missing: %s" % qualified_symbol)
    closing = matching_delimiter(outer.code, parameters, "(", ")")
    opening = outer.code.find("{", closing)
    if opening < 0:
        raise RouteError("named-lambda body is missing: %s" % qualified_symbol)
    absolute_opening = outer.body_start + opening
    return FunctionBody(
        source,
        qualified_symbol,
        start,
        absolute_opening,
        matching_delimiter(source.code, absolute_opening),
    )


def raw_carrier_transform(source_carrier: str, target_carrier: str) -> str:
    if source_carrier == target_carrier:
        return "identity"
    try:
        return RAW_CARRIER_TRANSFORMS[(source_carrier, target_carrier)]
    except KeyError as exc:
        raise RouteError(
            "raw-flow carrier transition lacks reviewed transform: %s -> %s"
            % (source_carrier, target_carrier)
        ) from exc


def raw_flow_bodies(tree: SourceTree, anchors: tuple[tuple[str, str], ...]) -> list[FunctionBody]:
    bodies: list[FunctionBody] = []
    for path, symbol in anchors:
        source = tree.require(path)
        if "::lambda(" in symbol:
            found = [named_lambda_body(source, symbol)]
        else:
            found = function_bodies(source, symbol)
        if not found and "::" in symbol:
            found = [inline_class_method_body(source, symbol)]
        if not found:
            raise RouteError("raw-flow anchor lacks a definition: %s in %s" % (symbol, path))
        bodies.extend(found)
    return bodies


def raw_eligibility_edge_rows(
    body: FunctionBody, edge_specs: tuple[dict, ...]
) -> list[dict]:
    """Extract one exact ordered raw eligibility edge per reviewed pattern."""
    source = body.comment_text
    rows: list[dict] = []
    previous_offset = -1
    for order, spec in enumerate(edge_specs, 1):
        matches = list(re.finditer(spec["pattern"], source))
        if len(matches) != 1:
            raise RouteError(
                "raw eligibility edge %s is missing or ambiguous in %s"
                % (spec["name"], body.symbol)
            )
        match = matches[0]
        if match.start() <= previous_offset:
            raise RouteError(
                "raw eligibility edge order changed in %s: %s"
                % (body.symbol, spec["name"])
            )
        previous_offset = match.start()
        rows.append(
            {
                "edge": spec["name"],
                "order": order,
                "predecessorEdges": [item["name"] for item in edge_specs[: order - 1]],
                "callee": spec["callee"],
                "calleeOverload": spec["calleeOverload"],
                "relation": spec["relation"],
                "sourceOffset": match.start(),
                "guardFingerprint": sha256_text(
                    " ".join(source[match.start() : match.end()].split())
                ),
                "callEvidence": _source_location(
                    body,
                    line_number(
                        body.source.text, body.body_start + match.start()
                    ),
                ),
            }
        )
    return rows


def raw_eligibility_metadata(
    node_name: str, bodies: list[FunctionBody]
) -> list[dict]:
    """Attach concrete read-side raw eligibility edges to flow nodes."""
    if node_name == "dxf-read-typed-raw-template":
        if len(bodies) != 1:
            raise RouteError("raw template eligibility anchor shape changed")
        body = bodies[0]
        if body.symbol != "dxfRW::processRawCapturedObject":
            raise RouteError("raw template eligibility symbol changed")
        # The bridge verifier below is the canonical source-of-truth for this
        # templated lambda.  Reuse its exact branch locations rather than
        # approximating a lambda body with a generic callback scan.
        bridge = verify_dxf_raw_captured_template_bridge(
            SourceTree(body.source.path, {body.source.path: body.source}, {})
        )
        if bridge is None or not bridge.get("orderedEdges"):
            raise RouteError("raw template eligibility bridge is unproved")
        return [{"bodySymbol": body.symbol, "edges": bridge["orderedEdges"]}]
    rules = RAW_DXF_ELIGIBILITY_RULES.get(node_name)
    if rules is None:
        return []
    source = bodies[0].source
    call_bodies: dict[str, FunctionBody] = {}
    for symbol in rules:
        found = function_bodies(source, symbol)
        if len(found) != 1:
            raise RouteError(
                "raw eligibility call-site anchor is not unique: %s" % symbol
            )
        call_bodies[symbol] = found[0]
    rows: list[dict] = []
    for symbol in sorted(rules):
        body = call_bodies[symbol]
        try:
            edge_specs = rules[body.symbol]
        except KeyError as exc:
            raise RouteError(
                "raw eligibility anchor is unexpected for %s: %s"
                % (node_name, body.symbol)
            ) from exc
        rows.append(
            {
                "bodySymbol": body.symbol,
                "edges": raw_eligibility_edge_rows(body, edge_specs),
            }
        )
    if {row["bodySymbol"] for row in rows} != set(rules):
        raise RouteError("raw eligibility body coverage changed: %s" % node_name)
    return rows


def raw_writer_metadata(node_name: str, tree: SourceTree, bodies: list[FunctionBody]) -> list[dict]:
    """Attach exact DXF raw writer guard/replay calls to flow nodes."""
    rules = RAW_DXF_WRITER_EVIDENCE_RULES.get(node_name)
    if rules is None:
        return []
    source = tree.require("src/libdxfrw.cpp")
    rows: list[dict] = []
    for symbol, edge_specs in sorted(rules.items()):
        found = function_bodies(source, symbol)
        if len(found) != 1:
            raise RouteError("raw writer call-site anchor is not unique: %s" % symbol)
        rows.append(
            {
                "bodySymbol": symbol,
                "edges": raw_eligibility_edge_rows(found[0], edge_specs),
            }
        )
    if {row["bodySymbol"] for row in rows} != set(rules):
        raise RouteError("raw writer body coverage changed: %s" % node_name)
    return rows


def dwg_raw_replay_metadata(
    node_name: str, tree: SourceTree, bodies: list[FunctionBody]
) -> list[dict]:
    """Attach exact DWG raw replay ingress, guards, and commit edges."""
    rules = DWG_RAW_REPLAY_EVIDENCE_RULES.get(node_name)
    if rules is None:
        return []
    body_by_symbol = {body.symbol: body for body in bodies}
    rows: list[dict] = []
    for path, path_rules in sorted(rules.items()):
        source = tree.require(path)
        for symbol, edge_specs in sorted(path_rules.items()):
            body = body_by_symbol.get(symbol)
            if body is None:
                found = function_bodies(source, symbol)
                if len(found) != 1:
                    raise RouteError(
                        "DWG raw replay call-site anchor is not unique: %s" % symbol
                    )
                body = found[0]
            if body.source.path != path:
                raise RouteError("DWG raw replay body path changed: %s" % symbol)
            rows.append(
                {
                    "sourcePath": path,
                    "bodySymbol": symbol,
                    "edges": raw_eligibility_edge_rows(body, edge_specs),
                }
            )
    expected = {(path, symbol) for path, path_rules in rules.items() for symbol in path_rules}
    observed = {(row["sourcePath"], row["bodySymbol"]) for row in rows}
    if observed != expected:
        raise RouteError("DWG raw replay body coverage changed: %s" % node_name)
    return rows


def dwg_table_delivery_metadata(
    tree: SourceTree, descriptor: str
) -> list[dict]:
    """Extract one concrete control-receipt/record-map chain."""
    try:
        control, record, table_map, record_name = DWG_TABLE_DELIVERY_FIELDS[descriptor]
    except KeyError as exc:
        raise RouteError("unknown DWG table descriptor: %s" % descriptor) from exc
    body = function_body(tree.require("src/intern/dwgreader.cpp"), "dwgReader::readDwgTables")
    specs = (
        {"name": "control-parse", "pattern": rf"parseControl\s*\(\s*oc\s*,\s*{re.escape(descriptor)}\s*,\s*{re.escape(control)}\s*\)", "callee": "parseControl", "calleeOverload": "table-control-parser", "relation": "receipt"},
        {"name": "control-handle-claim", "pattern": rf"claimControlHandles\s*\(\s*{re.escape(control)}\s*\)", "callee": "claimControlHandles", "calleeOverload": "claimControlHandles(const DRW_ObjControl&)", "relation": "eligibility"},
        {"name": "control-receipt-stage", "pattern": rf"stageControlReceipt\s*\(\s*oc\s*,\s*{re.escape(descriptor)}\s*,\s*{re.escape(control)}\s*\)", "callee": "stageControlReceipt", "calleeOverload": "stageControlReceipt(const objHandle&,const DwgTableDescriptor&,const DRW_ObjControl&)", "relation": "deferred-receipt"},
        {"name": "record-parse", "pattern": rf"parseTableRecord\s*\(\s*oc\s*,\s*{re.escape(descriptor)}\s*,\s*{re.escape(record)}\s*\)", "callee": "parseTableRecord", "calleeOverload": "parseTableRecord(const objHandle&,const DwgTableDescriptor&,record)", "relation": "typed-record"},
        {"name": "typed-map-insert", "pattern": rf"insertTableRecord\s*\(\s*{re.escape(table_map)}\s*,\s*std::move\s*\(\s*{re.escape(record)}\s*\)", "callee": "insertTableRecord", "calleeOverload": "insertTableRecord(map,record,recordName,handle,type)", "relation": "map-and-publication"},
    )
    return [{"bodySymbol": body.symbol, "edges": raw_eligibility_edge_rows(body, specs), "recordName": record_name}]


def dwg_compound_transition_metadata(
    tree: SourceTree, enum_symbol: str
) -> list[dict]:
    """Extract the concrete helper transition selected by a compound case."""
    anchors = DWG_COMPOUND_TRANSITION_ANCHORS.get(enum_symbol)
    if anchors is None:
        return []
    rows: list[dict] = []
    for path, symbol, edge_specs in anchors:
        body = function_body(tree.require(path), symbol)
        rows.append(
            {
                "sourcePath": path,
                "bodySymbol": symbol,
                "edges": raw_eligibility_edge_rows(body, edge_specs),
            }
        )
    return rows


def dwg_compound_delivery_metadata(tree: SourceTree) -> list[dict]:
    """Extract the common immediate/journal block delivery lifecycle."""
    rows: list[dict] = []
    for path, symbol, edge_specs in DWG_COMPOUND_DELIVERY_ANCHORS:
        body = function_body(tree.require(path), symbol)
        rows.append(
            {
                "sourcePath": path,
                "bodySymbol": symbol,
                "edges": raw_eligibility_edge_rows(body, edge_specs),
            }
        )
    return rows


def dwg_reader_lifecycle_metadata(tree: SourceTree) -> list[dict]:
    """Extract the ordered public DWG reader lifecycle and sticky error gates."""
    body = function_body(tree.require("src/libdwgr.cpp"), "dwgRW::processDwg")
    return [{
        "sourcePath": body.source.path,
        "bodySymbol": body.symbol,
        "edges": raw_eligibility_edge_rows(body, DWG_READER_LIFECYCLE_RULES),
    }]


def dwg_block_ownership_metadata(tree: SourceTree) -> list[dict]:
    """Extract BLOCK/ENDBLK owner, reachability, and publication ordering."""
    body = function_body(
        tree.require("src/intern/dwgreader.cpp"), "dwgReader::readDwgBlocks"
    )
    return [{
        "sourcePath": body.source.path,
        "bodySymbol": body.symbol,
        "edges": raw_eligibility_edge_rows(body, DWG_BLOCK_OWNERSHIP_RULES),
    }]


def add_raw_flow_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Close raw carriers, guards, handoffs, and replay phases explicitly."""
    inputs = raw_flow_inputs_by_name()
    publication_models = _publication_model_routes(collector)
    publication_callbacks = _publication_callback_routes(collector)
    publication_parents = publication_model_parents(tree, set(publication_models))
    for node in RAW_FLOW_NODES:
        flow = node["flow"]
        name = node["name"]
        phase = node["phase"]
        facade = node["facade"]
        anchors = node["anchors"]
        if name not in RAW_NODE_DIRECTIONS:
            raise RouteError("raw-flow node has no reviewed direction set: %s" % name)
        evidence_bodies = raw_flow_bodies(tree, anchors)
        nodes = {candidate["name"]: candidate for candidate in RAW_FLOW_NODES}
        output_transforms = [
            {
                "outputRouteId": raw_flow_route_id(facade, output_name),
                "kind": raw_carrier_transform(node["carrier"], nodes[output_name]["carrier"]),
            }
            for output_name in node["outputs"]
        ]
        selector = {
            "kind": "raw-flow-node",
            "flow": flow,
            "name": name,
            "phase": phase,
            "symbols": sorted({body.symbol for body in evidence_bodies}),
            "carrier": node["carrier"],
            "inputRouteIds": [raw_flow_route_id(facade, input_name) for input_name in inputs[name]],
            "outputRouteIds": [raw_flow_route_id(facade, output_name) for output_name in node["outputs"]],
            "outputCarrierTransforms": output_transforms,
            "readerPipelineIds": [
                "dwgRW/reader-pipeline/" + identifier
                for identifier in node.get("readerPipelines", ())
            ],
            "writerPipelineIds": [
                "dwgRW/writer-pipeline/" + identifier
                for identifier in node.get("writerPipelines", ())
            ],
            "handoffSemantics": RAW_HANDOFF_SEMANTICS,
            "optionalCrossSessionHandoffRouteIds": [
                raw_flow_route_id(facade, output_name)
                for output_name in node.get("optionalCrossSessionHandoffs", ())
            ],
            "optionalCrossSessionHandoffCarrierTransforms": [
                {
                    "outputRouteId": raw_flow_route_id(facade, output_name),
                    "kind": raw_carrier_transform(node["carrier"], nodes[output_name]["carrier"]),
                }
                for output_name in node.get("optionalCrossSessionHandoffs", ())
            ],
            "optionalCrossSessionHandoffSemantics": RAW_CROSS_SESSION_HANDOFF_SEMANTICS,
            "companionRawCarrierRouteIds": [
                raw_flow_route_id(facade, output_name)
                for output_name in node.get("requiresCompanionRawSectionCarrier", ())
            ],
        }
        if "terminalDisposition" in node:
            selector["terminalDisposition"] = node["terminalDisposition"]
        if "externalIngress" in node:
            selector["externalIngress"] = node["externalIngress"]
        if name in RAW_DXF_ELIGIBILITY_RULES or name == "dxf-read-typed-raw-template":
            selector["rawEligibilityEvidence"] = raw_eligibility_metadata(
                name, evidence_bodies
            )
        if name in RAW_DXF_WRITER_EVIDENCE_RULES:
            selector["rawWriterEvidence"] = raw_writer_metadata(
                name, tree, evidence_bodies
            )
        if name in DWG_RAW_REPLAY_EVIDENCE_RULES:
            selector["dwgRawReplayEvidence"] = dwg_raw_replay_metadata(
                name, tree, evidence_bodies
            )
        if phase == "publication":
            predecessor_route_ids = [
                raw_flow_route_id(facade, input_name)
                for input_name in inputs[name]
            ]
            selector["rawPublicationEvidence"] = raw_route_publication_metadata(
                name,
                evidence_bodies,
                publication_callbacks,
                publication_models,
                publication_parents,
                predecessor_route_ids,
            )
        collector.add(
            facade,
            "raw-flow",
            selector,
            evidence_bodies[0],
            identifier=name,
            directions=RAW_NODE_DIRECTIONS[name],
        )
        for body in evidence_bodies[1:]:
            collector.add(
                facade,
                "raw-flow",
                selector,
                body,
                identifier=name,
                directions=RAW_NODE_DIRECTIONS[name],
            )


def _publication_callback_routes(collector: RouteCollector) -> dict[str, dict]:
    """Return one registry row per parser-visible data publication callback."""
    result: dict[str, dict] = {}
    for route in collector.routes():
        if route["facade"] != "shared" or route["category"] != "callback":
            continue
        name = route["selector"].get("name")
        if not isinstance(name, str) or name in result:
            raise RouteError("callback registry is ambiguous: %r" % name)
        result[name] = route
    if not result:
        raise RouteError("callback registry is empty")
    return result


def _publication_model_routes(collector: RouteCollector) -> dict[str, dict]:
    result: dict[str, dict] = {}
    for route in collector.routes():
        if route["facade"] != "shared" or route["category"] != "public-model":
            continue
        name = route["selector"].get("name")
        if not isinstance(name, str) or name in result:
            raise RouteError("public-model registry is ambiguous: %r" % name)
        result[name] = route
    if not result:
        raise RouteError("public-model registry is empty")
    return result


def publication_model_parents(tree: SourceTree, models: set[str]) -> dict[str, str | None]:
    """Return direct public DRW base classes used by callback compatibility.

    This is intentionally a small declaration-level relation, not an inferred
    behavioral compatibility table.  A callback pair may use an ancestor only
    when the pinned source declares that inheritance edge directly.
    """
    result: dict[str, str | None] = {model: None for model in models}
    pattern = re.compile(
        r"\b(?:class|struct)\s+(DRW_[A-Za-z0-9_]+)\b\s*"
        r"(?::\s*public\s+(DRW_[A-Za-z0-9_]+)\b)?"
    )
    for source in tree.files.values():
        for match in pattern.finditer(source.code):
            model, parent = match.groups()
            if model not in result:
                continue
            previous = result[model]
            if previous is not None and parent is not None and previous != parent:
                raise RouteError("public model has conflicting direct parents: %s" % model)
            if parent is not None:
                result[model] = parent
    return result


def callback_accepts_model(
    callback_route: dict,
    model: str,
    parents: dict[str, str | None],
    argument_passing: str | None = None,
) -> tuple[bool, str]:
    """Prove exact or declared-ancestor callback compatibility."""
    expected = callback_route["selector"].get("parameterModel")
    if not isinstance(expected, str):
        return False, "callback-has-no-single-DRW-parameter"
    current = model
    seen: set[str] = set()
    while current not in seen:
        seen.add(current)
        if current == expected:
            compatibility = "exact" if current == model else "declared-ancestor"
            if argument_passing is not None:
                expected_passing = callback_route["selector"].get("parameterPassing")
                if expected_passing == "pointer" and argument_passing != "pointer":
                    return False, "callback-pointer-argument-mismatch"
                if expected_passing in {"reference", "value"} and argument_passing != "object":
                    return False, "callback-object-argument-mismatch"
                if expected_passing not in {"pointer", "reference", "value"}:
                    return False, "callback-parameter-passing-unproved"
            return True, compatibility
        current = parents.get(current)
        if current is None:
            break
    return False, "callback-parameter-mismatch"


def _source_location(body: FunctionBody, line: int) -> dict:
    return {
        "path": body.source.path,
        "symbol": body.symbol,
        "line": line,
        "spanSha256": body.span_sha256,
    }


def lexical_branch_ancestry(body: FunctionBody, call_offset: int) -> list[dict]:
    """Return outer-to-inner lexical if/else evidence for a source offset.

    This intentionally proves only lexical containment.  A preceding
    terminating guard belongs to reachability evidence, not this list; callers
    that need it must add a separate, explicitly labeled guard row.  Keeping
    the distinction avoids treating text proximity as a complete CFG proof.
    """
    if call_offset < 0 or call_offset >= len(body.code):
        raise RouteError("callback offset is outside its source body: %s" % body.symbol)
    cache_key = publication_ancestry_cache_key(body, call_offset)
    cached = _LEXICAL_BRANCH_ANCESTRY_CACHE.get(cache_key)
    if cached is not None:
        return [dict(context) for context in cached]
    code = body.code
    text = body.comment_text
    candidates: list[tuple[int, str, int, int]] = []
    for match in re.finditer(r"\bif\s*\(", code):
        opening = code.find("(", match.start(), match.end())
        closing = matching_delimiter(code, opening, "(", ")")
        then_start, then_end = statement_span(code, closing + 1)
        if then_start <= call_offset <= then_end:
            candidates.append((match.start(), "then", opening, closing))
        cursor = then_end + 1
        while cursor < len(code) and code[cursor].isspace():
            cursor += 1
        if code.startswith("else", cursor) and (
            cursor + 4 == len(code) or not (code[cursor + 4].isalnum() or code[cursor + 4] == "_")
        ):
            else_start, else_end = statement_span(code, cursor + 4)
            if else_start <= call_offset <= else_end:
                candidates.append((match.start(), "else", opening, closing))
    result: list[dict] = []
    for if_offset, branch_kind, opening, closing in sorted(set(candidates)):
        condition = " ".join(text[opening + 1 : closing].split())
        result.append(
            {
                "conditionFingerprint": sha256_text(condition),
                "branchKind": branch_kind,
                "branchGroupFingerprint": sha256_text(
                    "%s:%d" % (body.symbol, body.body_start + if_offset)
                ),
                "sourceEvidence": _source_location(
                    body, line_number(body.source.text, body.body_start + if_offset)
                ),
            }
        )
    _LEXICAL_BRANCH_ANCESTRY_CACHE[cache_key] = tuple(result)
    return [dict(context) for context in result]


def switch_case_branch_ancestry(body: FunctionBody, call_offset: int) -> list[dict]:
    """Return the enclosing ``switch`` case selection for one callback.

    A callback nested under a ``switch`` is not unconditional merely because
    its model/callback endpoint occurs only once in the source.  Retain the
    selector and case-domain evidence, and distinguish an explicit direct
    ``break`` from a fall-through or an unproved transition.  The extractor is
    deliberately conservative: a non-trivial fall-through domain leaves
    delivery cardinality unresolved rather than pretending the adjacent case
    labels are independent paths.
    """
    if call_offset < 0 or call_offset >= len(body.code):
        raise RouteError("callback offset is outside its source body: %s" % body.symbol)
    cache_key = publication_ancestry_cache_key(body, call_offset)
    cached = _SWITCH_CASE_ANCESTRY_CACHE.get(cache_key)
    if cached is not None:
        return [dict(context) for context in cached]
    code = body.code
    text = body.comment_text
    result: list[dict] = []
    case_pattern = re.compile(
        r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*|0[xX][0-9A-Fa-f]+|\d+)\s*:"
        r"|\b(default)\s*:"
    )
    for switch_match in re.finditer(r"\bswitch\s*\(", code):
        opening = code.find("(", switch_match.start(), switch_match.end())
        closing = matching_delimiter(code, opening, "(", ")")
        brace = closing + 1
        while brace < len(code) and code[brace].isspace():
            brace += 1
        if brace >= len(code) or code[brace] != "{":
            raise RouteError("switch in %s has no braced body" % body.symbol)
        switch_end = matching_delimiter(code, brace)
        if not brace < call_offset < switch_end:
            continue
        switch_depth = code[:brace].count("{") - code[:brace].count("}")
        labels: list[tuple[re.Match[str], str]] = []
        for label in case_pattern.finditer(code, brace + 1, switch_end):
            label_depth = code[: label.start()].count("{") - code[: label.start()].count("}")
            if label_depth == switch_depth + 1:
                labels.append((label, label.group(1) or label.group(2)))
        selected = [index for index, (label, _token) in enumerate(labels) if label.start() <= call_offset]
        if not selected:
            continue
        selected_index = selected[-1]
        selected_label, selected_token = labels[selected_index]
        next_label_start = (
            labels[selected_index + 1][0].start()
            if selected_index + 1 < len(labels)
            else switch_end
        )
        if call_offset >= next_label_start:
            continue

        # Consecutive labels are a single fall-through domain.  Preserve that
        # fact as a disjunction in one condition fingerprint; representing the
        # labels as separate ancestry entries would falsely imply they are
        # conjunctive predicates.
        chain_start = selected_index
        while chain_start > 0:
            previous_label = labels[chain_start - 1][0]
            between = code[previous_label.end() : labels[chain_start][0].start()].strip()
            if between in {"", "[[fallthrough]];"}:
                chain_start -= 1
                continue
            break
        chain_tokens = [token for _label, token in labels[chain_start : selected_index + 1]]
        selector = " ".join(text[opening + 1 : closing].split())

        # A direct break after the callback proves the selected case does not
        # continue into another direct label.  Nested breaks are deliberately
        # ignored: they do not establish this switch's transition.
        label_body = code[selected_label.end() : next_label_start]
        case_start = selected_label.end()
        while case_start < len(code) and code[case_start].isspace():
            case_start += 1
        expected_break_depth = (
            code[:case_start].count("{") - code[:case_start].count("}") + 1
            if case_start < len(code) and code[case_start] == "{"
            else code[:selected_label.start()].count("{") - code[:selected_label.start()].count("}")
        )
        def is_direct_case_break(break_offset: int, break_depth: int) -> bool:
            if break_depth != expected_break_depth:
                return False
            # A same-depth break can still belong to an unbraced ``if`` or
            # loop.  Reject any break nested in a control statement within
            # this case; only a transition owned directly by the case/switch
            # establishes non-fall-through delivery.
            control_pattern = re.compile(
                r"\b(?:if\s*(?:constexpr\s*)?\(|for\s*\(|while\s*\(|switch\s*\(|do\b)"
            )
            for control in control_pattern.finditer(code, selected_label.end(), break_offset):
                try:
                    if control.group(0).rstrip().endswith("do"):
                        control_start, control_end = statement_span(code, control.end())
                    else:
                        control_opening = code.find("(", control.start(), control.end())
                        control_closing = matching_delimiter(code, control_opening, "(", ")")
                        control_start, control_end = statement_span(code, control_closing + 1)
                except RouteError:
                    continue
                if control_start <= break_offset <= control_end:
                    return False
            return True

        has_direct_break = False
        for break_match in re.finditer(r"\bbreak\s*;", label_body):
            absolute_break = selected_label.end() + break_match.start()
            break_depth = code[:absolute_break].count("{") - code[:absolute_break].count("}")
            if absolute_break > call_offset and is_direct_case_break(
                absolute_break, break_depth
            ):
                has_direct_break = True
                break
        if chain_start != selected_index:
            branch_kind = "case-fallthrough"
            transition = "fallthrough"
        elif has_direct_break:
            branch_kind = "case-break"
            transition = "direct-break"
        else:
            branch_kind = "case-unresolved"
            transition = "transition-unproved"
        condition = "switch (%s) cases (%s) transition=%s" % (
            selector, " | ".join(chain_tokens), transition
        )
        first_label = labels[chain_start][0]
        result.append(
            {
                "conditionFingerprint": sha256_text(condition),
                "branchKind": branch_kind,
                "branchGroupFingerprint": sha256_text(
                    "%s:%d:switch" % (body.symbol, body.body_start + switch_match.start())
                ),
                "sourceEvidence": _source_location(
                    body, line_number(body.source.text, body.body_start + first_label.start())
                ),
            }
        )
    _SWITCH_CASE_ANCESTRY_CACHE[cache_key] = tuple(result)
    return [dict(context) for context in result]


@lru_cache(maxsize=None)
def enclosing_brace_scope_stack(code: str, offset: int) -> tuple[int, ...]:
    """Return every concrete enclosing braced scope for ``offset``.

    Brace *depth* is not a lexical-scope identity: sibling lambdas and sibling
    compound statements can have the same depth.  Publication reachability
    must therefore compare the actual opening-brace offset, not a count.
    """
    if offset < 0 or offset >= len(code):
        raise RouteError("scope offset is outside source body")
    stack: list[int] = []
    for index, character in enumerate(code[:offset]):
        if character == "{":
            stack.append(index)
        elif character == "}":
            if not stack:
                raise RouteError("unbalanced lexical scope")
            stack.pop()
    return tuple(stack)


def is_direct_terminating_statement(code: str, start: int, end: int) -> bool:
    """Accept only a direct return/throw guard, never a nested substring."""
    statement = code[start : end + 1].strip()
    if re.match(r"^(?:return|throw)\b", statement):
        return True
    if not (statement.startswith("{") and statement.endswith("}")):
        return False
    inner = statement[1:-1].strip()
    # A braced direct terminator is safe only when the first executable
    # statement itself terminates.  Do not infer a general CFG from nested
    # conditionals, loops, or lambdas.
    return bool(re.match(r"^(?:return|throw)\b", inner)) and "{" not in inner


def preceding_terminating_guard_ancestry(
    body: FunctionBody, call_offset: int
) -> list[dict]:
    """Record same-scope earlier ``if (...) return`` reachability guards.

    This is deliberately narrower than a general control-flow graph: it only
    retains a guard in an ancestor concrete braced scope of the delivery,
    before the call, whose selected then-statement directly terminates.  The
    actual scope stack excludes sibling lambdas/blocks that merely share a
    brace depth.  If both locations are selected by a switch, the selected
    case must also agree.  The explicit branch kind makes clear that the
    callback is reached by falling through the *opposite* of that terminating
    condition; it does not claim mutual exclusion with other callback sites.
    """
    if call_offset < 0 or call_offset >= len(body.code):
        raise RouteError("callback offset is outside its source body: %s" % body.symbol)
    code = body.code
    text = body.comment_text
    call_scope_stack = enclosing_brace_scope_stack(code, call_offset)
    call_lexical = lexical_branch_ancestry(body, call_offset)
    call_switches = {
        context["branchGroupFingerprint"]: context["conditionFingerprint"]
        for context in switch_case_branch_ancestry(body, call_offset)
    }
    result: list[dict] = []
    for match in re.finditer(r"\bif\s*(?:constexpr\s*)?\(", code):
        if match.start() >= call_offset:
            break
        guard_scope_stack = enclosing_brace_scope_stack(code, match.start())
        if guard_scope_stack != call_scope_stack[: len(guard_scope_stack)]:
            continue
        guard_lexical = lexical_branch_ancestry(body, match.start())
        if guard_lexical != call_lexical[: len(guard_lexical)]:
            # An unbraced nested guard can share a concrete brace scope with
            # a later sibling statement while still being conditional on an
            # outer ``if`` that does not reach this callback.
            continue
        guard_switches = {
            context["branchGroupFingerprint"]: context["conditionFingerprint"]
            for context in switch_case_branch_ancestry(body, match.start())
        }
        if any(
            call_switches.get(group) != fingerprint
            for group, fingerprint in guard_switches.items()
        ):
            continue
        opening = code.find("(", match.start(), match.end())
        closing = matching_delimiter(code, opening, "(", ")")
        then_start, then_end = statement_span(code, closing + 1)
        if then_end >= call_offset or not is_direct_terminating_statement(
            code, then_start, then_end
        ):
            continue
        condition = " ".join(text[opening + 1 : closing].split())
        result.append(
            {
                "conditionFingerprint": sha256_text(condition),
                "branchKind": "fallthrough-after-then-return",
                "branchGroupFingerprint": sha256_text(
                    "%s:%d" % (body.symbol, body.body_start + match.start())
                ),
                "sourceEvidence": _source_location(
                    body, line_number(body.source.text, body.body_start + match.start())
                ),
            }
        )
    return result


def publication_branch_context(body: FunctionBody, call_offset: int) -> dict:
    """Return the innermost lexical branch, retaining legacy compact evidence."""
    ancestry = lexical_branch_ancestry(body, call_offset)
    if ancestry:
        return ancestry[-1]
    fingerprint = sha256_text("unconditional:" + body.symbol)
    return {
        "conditionFingerprint": fingerprint,
        "branchKind": "unconditional",
        "branchGroupFingerprint": fingerprint,
        "sourceEvidence": _source_location(
            body, line_number(body.source.text, body.body_start + call_offset)
        ),
    }


def canonical_condition_ancestry(contexts: Iterable[dict]) -> list[dict]:
    """Deduplicate condition contexts while preserving source evaluation order."""
    return sorted(
        {canonical_json(context): context for context in contexts}.values(),
        key=lambda context: (
            context["sourceEvidence"]["line"],
            context["branchGroupFingerprint"],
        ),
    )


def enclosing_dispatch_ancestry(body: FunctionBody, call_offset: int) -> tuple[dict, ...]:
    """Return only predicates enclosing a narrowed dispatch branch.

    ``dwg_named_branch_body_map`` trims the named ``if`` body into a synthetic
    FunctionBody.  Its surrounding ``oType``/class-map guards and enclosing
    switch/default branch would otherwise disappear before local publication
    extraction.  Capture those predicates from the original dispatcher first;
    the selected named condition itself is appended by the caller.
    """
    contexts: list[dict] = []
    contexts.extend(lexical_branch_ancestry(body, call_offset))
    contexts.extend(switch_case_branch_ancestry(body, call_offset))
    contexts.extend(preceding_terminating_guard_ancestry(body, call_offset))
    return tuple(canonical_condition_ancestry(contexts))


def publication_condition_ancestry(
    body: FunctionBody, call_offset: int, dispatch_ancestry: Iterable[dict] = ()
) -> list[dict]:
    """Compose reviewed dispatch evidence with local lexical branch evidence."""
    result = [dict(context) for context in dispatch_ancestry]
    result.extend(lexical_branch_ancestry(body, call_offset))
    result.extend(switch_case_branch_ancestry(body, call_offset))
    result.extend(preceding_terminating_guard_ancestry(body, call_offset))
    result = canonical_condition_ancestry(result)
    if not result:
        result.append(publication_branch_context(body, call_offset))
    return result


def _strip_outer_parentheses(value: str) -> str:
    value = value.strip()
    while value.startswith("(") and value.endswith(")"):
        try:
            if matching_delimiter(value, 0, "(", ")") != len(value) - 1:
                break
        except RouteError:
            break
        value = value[1:-1].strip()
    return value


def _direct_binding_name(argument: str) -> tuple[str, str] | None:
    """Return a local binding plus its physical object/pointer call form."""
    value = _strip_outer_parentheses(argument)
    match = re.fullmatch(r"([&*]?)\s*([A-Za-z_][A-Za-z0-9_]*)", value)
    if match is not None:
        modifier, binding = match.groups()
        # ``*local`` is an object expression; bare locals are also objects.
        return binding, "pointer" if modifier == "&" else "object"
    smart_pointer = re.fullmatch(r"([A-Za-z_][A-Za-z0-9_]*)\.get\s*\(\s*\)", value)
    return (smart_pointer.group(1), "pointer") if smart_pointer is not None else None


def _dxf_local_typed_bindings(body: FunctionBody, models: set[str]) -> dict[str, list[dict]]:
    """Find unambiguous local/lambda DRW bindings without name heuristics.

    A later publication is eligible for a direct pair only when its argument
    resolves to exactly one entry in this map.  Multiple declarations of the
    same spelling (including shadowing) intentionally make the binding
    ambiguous and force a staged route instead of guessing C++ scope.
    """
    result: dict[str, list[dict]] = {}
    # A selected switch/if branch has no enclosing braces in its synthetic
    # FunctionBody.  Seed a lexical whole-body scope so the same extractor can
    # safely serve full DXF functions and narrow DWG dispatch branches.
    scopes: list[tuple[int, int]] = [(0, max(0, len(body.code) - 1))]
    stack: list[int] = []
    for offset, character in enumerate(body.code):
        if character == "{":
            stack.append(offset)
        elif character == "}":
            if not stack:
                raise RouteError("unbalanced lexical scope in %s" % body.symbol)
            scopes.append((stack.pop(), offset))
    if stack:
        raise RouteError("unterminated lexical scope in %s" % body.symbol)

    def scope_for(offset: int, override: tuple[int, int] | None = None) -> tuple[int, int]:
        if override is not None:
            return override
        containing = [scope for scope in scopes if scope[0] <= offset <= scope[1]]
        if not containing:
            raise RouteError("typed binding has no lexical scope in %s" % body.symbol)
        return min(containing, key=lambda scope: scope[1] - scope[0])

    def add(
        model: str, binding: str, kind: str, absolute: int, relative: int,
        scope_override: tuple[int, int] | None = None,
    ) -> None:
        if model not in models:
            return
        scope_start, scope_end = scope_for(relative, scope_override)
        row = {
            "model": model,
            "binding": binding,
            "bindingKind": kind,
            "evidence": _source_location(
                body, line_number(body.source.text, absolute)
            ),
            "_declarationOffset": relative,
            "_scopeStart": scope_start,
            "_scopeEnd": scope_end,
        }
        rows = result.setdefault(binding, [])
        if row not in rows:
            rows.append(row)

    # Lambda parameters must be identified first so the ordinary declaration
    # scan does not label them as a generic local variable.
    lambda_spans: list[tuple[int, int]] = []
    lambda_pattern = re.compile(
        r"\[[^\]]*\]\s*\(\s*(?:const\s+)?(DRW_[A-Za-z0-9_]+)\s*"
        r"(?:[&*]\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*\)"
    )
    for match in lambda_pattern.finditer(body.code):
        lambda_spans.append((match.start(1), match.end(2)))
        opening = body.code.find("{", match.end())
        if opening < 0:
            raise RouteError("typed lambda has no body: %s" % body.symbol)
        lambda_scope = (opening, matching_delimiter(body.code, opening))
        add(
            match.group(1), match.group(2), "lambda-param",
            body.body_start + match.start(1), match.start(1), lambda_scope,
        )

    declaration_pattern = re.compile(
        r"\b(?:const\s+)?(DRW_[A-Za-z0-9_]+)\s*(?:const\s+)?"
        r"(?:[&*]\s*)?([A-Za-z_][A-Za-z0-9_]*)\s*(?=[=;,:\)(])"
    )
    for match in declaration_pattern.finditer(body.code):
        if any(start <= match.start(1) <= end for start, end in lambda_spans):
            continue
        add(
            match.group(1), match.group(2), "local",
            body.body_start + match.start(1), match.start(1),
        )

    smart_pointer_pattern = re.compile(
        r"\bstd::(?:unique_ptr|shared_ptr)\s*<\s*(DRW_[A-Za-z0-9_]+)\s*>\s*"
        r"([A-Za-z_][A-Za-z0-9_]*)"
    )
    for match in smart_pointer_pattern.finditer(body.code):
        add(
            match.group(1), match.group(2), "smart-pointer-local",
            body.body_start + match.start(1), match.start(1),
        )
    return result


def _resolve_dxf_binding(
    bindings: dict[str, list[dict]], binding: str, call_offset: int
) -> dict | None:
    """Resolve a binding at one callsite without guessing across scopes."""
    candidates = [
        row for row in bindings.get(binding, [])
        if row["_declarationOffset"] <= call_offset
        and row["_scopeStart"] <= call_offset <= row["_scopeEnd"]
    ]
    if not candidates:
        return None
    narrowest = min(
        row["_scopeEnd"] - row["_scopeStart"] for row in candidates
    )
    candidates = [
        row for row in candidates
        if row["_scopeEnd"] - row["_scopeStart"] == narrowest
    ]
    latest = max(row["_declarationOffset"] for row in candidates)
    candidates = [row for row in candidates if row["_declarationOffset"] == latest]
    return candidates[0] if len(candidates) == 1 else None


def _dxf_member_bindings(tree: SourceTree, models: set[str]) -> dict[str, dict]:
    """Verify the narrow dxfRW member binding needed by processHeader."""
    if "src/libdxfrw.h" not in tree.files:
        # Synthetic lexical tests deliberately provide only the source under
        # inspection; production source trees are required to prove this
        # member binding below.
        return {}
    source = tree.require("src/libdxfrw.h")
    class_body = class_definition_anchor(SourceTree(tree.label, {source.path: source}, {}), "dxfRW")
    match = re.search(r"\b(DRW_Header)\s+(header)\s*;", class_body.code)
    if match is None or match.group(1) not in models:
        raise RouteError("dxfRW header member binding is missing or not public")
    return {
        match.group(2): {
            "model": match.group(1),
            "binding": match.group(2),
            "bindingKind": "member",
            "evidence": _source_location(
                class_body, line_number(source.text, class_body.body_start + match.start(1))
            ),
        }
    }


def _interface_publications(body: FunctionBody, callbacks: dict[str, dict]) -> list[dict]:
    """Locate physical interface calls and retain their one raw argument."""
    result: list[dict] = []
    pattern = re.compile(
        r"\b(?:iface|interface|intfa)\s*(?:->|\.)\s*([A-Za-z_][A-Za-z0-9_]*)\s*\("
    )
    for match in pattern.finditer(body.code):
        callback = match.group(1)
        if callback not in callbacks:
            continue
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        argument = body.code[opening + 1 : closing]
        # A comma means the callback argument cannot be bound safely by this
        # lightweight extractor; keep the parser route staged.
        if "," in argument:
            continue
        result.append(
            {
                "callback": callback,
                "argument": argument,
                "line": line_number(body.source.text, body.body_start + match.start(1)),
                "offset": match.start(1),
            }
        )
    return result


def _same_publication_branch(left: list[dict], right: list[dict]) -> bool:
    """Compare branch predicates while ignoring per-call unconditional markers."""
    return [item for item in left if item.get("branchKind") != "unconditional"] == [
        item for item in right if item.get("branchKind") != "unconditional"
    ]


def _raw_binding_evidence(body: FunctionBody, model: str, binding: str) -> dict:
    """Resolve one exact local raw-carrier declaration in a route body."""
    pattern = re.compile(
        r"\b(?:const\s+)?" + re.escape(model) + r"\s+"
        + re.escape(binding) + r"\s*;"
    )
    matches = list(pattern.finditer(body.code))
    if len(matches) != 1:
        raise RouteError(
            "raw publication binding is missing or ambiguous in %s: %s %s"
            % (body.symbol, model, binding)
        )
    match = matches[0]
    return _source_location(
        body, line_number(body.source.text, body.body_start + match.start())
    )


def _raw_route_publication_row(
    body: FunctionBody,
    invocation: dict,
    model: str,
    binding: str,
    callback: str,
    callbacks: dict[str, dict],
    models: dict[str, dict],
    parents: dict[str, str | None],
    predecessor_route_ids: list[str],
) -> dict:
    """Build a source-owned raw callback row from one physical invocation."""
    if callback not in callbacks:
        raise RouteError("raw publication callback is not registered: %s" % callback)
    resolved = _direct_binding_name(invocation["argument"])
    if resolved != (binding, "object"):
        raise RouteError(
            "raw publication callback has an unproved argument in %s: %s(%s)"
            % (body.symbol, callback, invocation["argument"])
        )
    if model not in models:
        raise RouteError("raw publication carrier is not a public model: %s" % model)
    compatible, compatibility = callback_accepts_model(
        callbacks[callback], model, parents, "object"
    )
    if not compatible:
        raise RouteError(
            "raw publication callback/carrier mismatch in %s: %s -> %s (%s)"
            % (body.symbol, model, callback, compatibility)
        )
    condition_ancestry = publication_condition_ancestry(body, invocation["offset"])
    return {
        "carrier": model,
        "carrierRoute": models[model]["id"],
        "binding": binding,
        "bindingKind": "local",
        "argumentPassing": "object",
        "callbackPassingVia": "direct",
        "callStyle": "interface-call",
        "branchContext": condition_ancestry[-1],
        "conditionAncestry": condition_ancestry,
        "callback": callback,
        "callbackRoute": callbacks[callback]["id"],
        "callbackCompatibility": compatibility,
        "callEvidence": _source_location(body, invocation["line"]),
        "bindingEvidence": _raw_binding_evidence(body, model, binding),
        "rawFlowPredecessorRouteIds": list(predecessor_route_ids),
    }


def raw_route_publication_metadata(
    node_name: str,
    bodies: list[FunctionBody],
    callbacks: dict[str, dict],
    models: dict[str, dict],
    parents: dict[str, str | None],
    predecessor_route_ids: list[str],
) -> list[dict]:
    """Prove terminal DXF raw callbacks and proxy typed-to-raw ordering.

    The structural raw-flow node proves that a publication phase exists.  This
    companion metadata proves the actual interface call, local carrier, and,
    for proxy records, the typed callback that immediately precedes the raw
    carrier.  Missing or reordered calls fail closed instead of leaving a
    generic terminal that could be mistaken for delivery.
    """
    if node_name in RAW_DXF_SINGLE_PUBLICATION_RULES:
        expected_symbol, model, binding, callback = RAW_DXF_SINGLE_PUBLICATION_RULES[node_name]
        if len(bodies) != 1 or bodies[0].symbol != expected_symbol:
            raise RouteError("raw publication anchor shape changed: %s" % node_name)
        body = bodies[0]
        invocations = [
            item for item in _interface_publications(body, callbacks)
            if item["callback"] == callback
        ]
        if len(invocations) != 1:
            raise RouteError(
                "raw publication callback count changed in %s: %s"
                % (body.symbol, callback)
            )
        return [
            _raw_route_publication_row(
                body, invocations[0], model, binding, callback, callbacks,
                models, parents, predecessor_route_ids,
            )
        ]

    if node_name != "dxf-publish-proxy":
        # DWG raw publication and DXF replay terminals are intentionally kept
        # structural until their own source-flow children land.
        return []

    rows: list[dict] = []
    for body in bodies:
        try:
            (
                typed_model, typed_binding, typed_callback,
                raw_model, raw_binding, raw_callback,
            ) = RAW_DXF_PROXY_PUBLICATION_RULES[body.symbol]
        except KeyError as exc:
            raise RouteError("unexpected proxy raw publication body: %s" % body.symbol) from exc
        invocations = _interface_publications(body, callbacks)
        typed_invocations = [item for item in invocations if item["callback"] == typed_callback]
        raw_invocations = [item for item in invocations if item["callback"] == raw_callback]
        if len(typed_invocations) != 1 or len(raw_invocations) != 1:
            raise RouteError("proxy typed/raw callback count changed in %s" % body.symbol)
        typed = _raw_route_publication_row(
            body, typed_invocations[0], typed_model, typed_binding, typed_callback,
            callbacks, models, parents, predecessor_route_ids,
        )
        raw = _raw_route_publication_row(
            body, raw_invocations[0], raw_model, raw_binding, raw_callback,
            callbacks, models, parents, predecessor_route_ids,
        )
        if typed["callEvidence"]["line"] >= raw["callEvidence"]["line"]:
            raise RouteError("proxy typed callback does not precede raw carrier: %s" % body.symbol)
        if not _same_publication_branch(
            typed["conditionAncestry"], raw["conditionAncestry"]
        ):
            raise RouteError("proxy typed/raw callbacks do not share a branch: %s" % body.symbol)
        raw["typedToRawRelation"] = "typed-callback-before-raw-carrier"
        raw["typedPredecessor"] = typed
        rows.append(raw)
    return rows


def raw_captured_template_invocation(
    body: FunctionBody, bound: dict
) -> dict | None:
    """Return the exact raw-template invocation enclosing one lambda binding.

    A lambda that happens to call ``iface->add*`` is not automatically an
    instance of ``processRawCapturedObject``.  The latter has a reviewed
    capture/boundary/publication contract; an arbitrary helper lambda does not.
    Keep unknown helper lambdas out of the direct route inventory until their
    own bridge is modeled rather than borrowing the raw-template label.
    """
    declaration = bound.get("_declarationOffset")
    if not isinstance(declaration, int):
        raise RouteError(
            "lambda publication binding has no declaration offset: %s" % body.symbol
        )
    pattern = re.compile(
        r"\bprocessRawCapturedObject\s*(?:<[^(){};]*>)?\s*\("
    )
    for match in pattern.finditer(body.code):
        opening = match.end() - 1
        closing = matching_delimiter(body.code, opening, "(", ")")
        arguments = body.code[opening + 1 : closing]
        spans = top_level_argument_spans(arguments)
        if len(spans) != 2:
            continue
        second_start, second_end = spans[1]
        second_argument = arguments[second_start:second_end]
        # The raw-captured contract invokes AddFn, the second top-level
        # parameter.  A lambda nested in the debug-name argument (or wrapped
        # in an arbitrary expression) is not an AddFn callback proof.
        if not second_argument.lstrip().startswith("["):
            continue
        absolute_second_start = opening + 1 + second_start
        absolute_second_end = opening + 1 + second_end
        if absolute_second_start < declaration < absolute_second_end:
            line = line_number(body.source.text, body.body_start + match.start())
            return {
                "offset": match.start(),
                "evidence": _source_location(body, line),
            }
    return None


def lambda_is_raw_captured_template_argument(body: FunctionBody, bound: dict) -> bool:
    """Compatibility predicate for callers that only need bridge eligibility."""
    return raw_captured_template_invocation(body, bound) is not None


def dxf_direct_publication(
    body: FunctionBody,
    models: dict[str, dict],
    callbacks: dict[str, dict],
    member_bindings: dict[str, dict],
    parents: dict[str, str | None],
    raw_template_bridge: dict | None = None,
) -> tuple[list[dict], list[dict]]:
    """Extract only one-to-one, physical DXF model/publication pairs.

    The old union of every DRW token with every add callback fabricated a
    Cartesian relationship.  Here the callback argument itself must resolve
    to one unique local, lambda, or verified member binding.  Raw carriers are
    recorded separately, never as a typed model/callback pair.
    """
    bindings = _dxf_local_typed_bindings(body, set(models))
    pairs: list[dict] = []
    raw_publications: list[dict] = []
    interface_invocations = _interface_publications(body, callbacks)

    def is_single_unconditional_template_addfn(
        bound: dict, invocation: dict
    ) -> bool:
        """Prove the template's typed/raw callbacks execute as one pair.

        The raw template always publishes its raw carrier after invoking
        AddFn.  A conditional/multiple callback inside the lambda would not
        establish the same delivery domain, so it must remain staged until a
        richer lambda CFG proof exists.
        """
        scope_start = bound.get("_scopeStart")
        scope_end = bound.get("_scopeEnd")
        if not isinstance(scope_start, int) or not isinstance(scope_end, int):
            return False
        lambda_callbacks = [
            item for item in interface_invocations
            if scope_start <= item["offset"] <= scope_end
        ]
        if len(lambda_callbacks) != 1 or lambda_callbacks[0]["offset"] != invocation["offset"]:
            return False
        callback_scope_stack = enclosing_brace_scope_stack(body.code, invocation["offset"])
        if not callback_scope_stack or callback_scope_stack[-1] != scope_start:
            # A callback captured by a nested/deferred lambda is not executed
            # by AddFn at the template boundary.
            return False
        lambda_code = body.code[scope_start : scope_end + 1]
        return not re.search(r"\b(?:if|for|while|switch|do|return|throw)\b", lambda_code)

    for invocation in interface_invocations:
        resolved_binding = _direct_binding_name(invocation["argument"])
        if resolved_binding is None:
            continue
        binding, argument_passing = resolved_binding
        bound = _resolve_dxf_binding(bindings, binding, invocation["offset"])
        if bound is None and binding in member_bindings:
            bound = member_bindings[binding]
        if bound is None:
            continue
        normalized_argument = _strip_outer_parentheses(invocation["argument"])
        if normalized_argument.endswith(")") and ".get" in normalized_argument and bound["bindingKind"] != "smart-pointer-local":
            continue
        callback = invocation["callback"]
        callback_route = callbacks[callback]
        call_evidence = _source_location(body, invocation["line"])
        condition_ancestry = publication_condition_ancestry(body, invocation["offset"])
        compatible, compatibility = callback_accepts_model(
            callback_route, bound["model"], parents, argument_passing
        )
        if not compatible:
            raise RouteError(
                "DXF publication callback/model mismatch in %s: %s -> %s (%s)"
                % (body.symbol, bound["model"], callback, compatibility)
            )
        template_invocation = (
            raw_captured_template_invocation(body, bound)
            if bound["bindingKind"] == "lambda-param"
            else None
        )
        template_mediated = template_invocation is not None
        if bound["bindingKind"] == "lambda-param" and not template_mediated:
            # The physical callback is nested in an unreviewed helper.  Do not
            # claim either an ordinary direct call or the raw-captured template
            # bridge; the surrounding parser route will remain staged/unproved.
            continue
        if template_mediated and (
            raw_template_bridge is None
            or not is_single_unconditional_template_addfn(bound, invocation)
        ):
            # Do not fabricate a shared typed/raw delivery bundle when the
            # wrapper contract is unavailable or the AddFn body can skip or
            # repeat its typed callback.  The parser route falls back to its
            # explicit staged disposition instead.
            continue
        call_style = "dxf-raw-captured-template" if template_mediated else "interface-call"
        passing_via = call_style if template_mediated else "direct"
        if bound["model"] == "DRW_RawDxfObject":
            if callback.startswith("addRawDxf"):
                raw_publications.append(
                    {
                        "carrier": "DRW_RawDxfObject",
                        "binding": binding,
                        "bindingKind": bound["bindingKind"],
                        "argumentPassing": argument_passing,
                        "sourceArgumentPassing": argument_passing,
                        "callbackPassingVia": passing_via,
                        "callStyle": call_style,
                        "branchContext": condition_ancestry[-1],
                        "conditionAncestry": condition_ancestry,
                        "callback": callback,
                        "callbackRoute": callback_route["id"],
                        "callbackCompatibility": compatibility,
                        "callEvidence": call_evidence,
                        "bindingEvidence": bound["evidence"],
                    }
                )
            continue
        pair = {
                "model": bound["model"],
                "modelRoute": models[bound["model"]]["id"],
                "callback": callback,
                "callbackRoute": callback_route["id"],
                "callbackKind": callback_route["selector"].get("callbackKind"),
                "callbackCompatibility": compatibility,
                "binding": binding,
                "bindingKind": bound["bindingKind"],
                "argumentPassing": argument_passing,
                "sourceArgumentPassing": argument_passing,
                "callbackPassingVia": passing_via,
                "callStyle": call_style,
                "branchContext": condition_ancestry[-1],
                "conditionAncestry": condition_ancestry,
                "callEvidence": call_evidence,
                "bindingEvidence": bound["evidence"],
            }
        if template_mediated:
            pair["templateInvocationEvidence"] = template_invocation["evidence"]
        pairs.append(pair)
        if template_mediated and raw_template_bridge is not None:
            raw_callback = "addRawDxfObject"
            raw_callback_route = callbacks.get(raw_callback)
            if raw_callback_route is None:
                raise RouteError("DXF raw template callback is not registered")
            raw_compatible, raw_compatibility = callback_accepts_model(
                raw_callback_route, "DRW_RawDxfObject", parents, "object"
            )
            if not raw_compatible:
                raise RouteError("DXF raw template callback contract is not proven")
            raw_publications.append(
                {
                    "carrier": "DRW_RawDxfObject",
                    "binding": "template-raw-carrier@%d" % template_invocation["evidence"]["line"],
                    "bindingKind": "template-raw-carrier",
                    "argumentPassing": "object",
                    "sourceArgumentPassing": "object",
                    "callbackPassingVia": "dxf-raw-captured-template",
                    "callStyle": "dxf-raw-captured-template",
                    "branchContext": condition_ancestry[-1],
                    "conditionAncestry": condition_ancestry,
                    "callback": raw_callback,
                    "callbackRoute": raw_callback_route["id"],
                    "callbackCompatibility": raw_compatibility,
                    "callEvidence": raw_template_bridge["rawCallbackEvidence"],
                    "bindingEvidence": raw_template_bridge["rawBindingEvidence"],
                    "templateInvocationEvidence": template_invocation["evidence"],
                    "templateBridgeEvidence": raw_template_bridge["templateEvidence"],
                }
            )
    pairs = sorted(
        {json.dumps(pair, sort_keys=True): pair for pair in pairs}.values(),
        key=lambda pair: (pair["model"], pair["callback"], pair["binding"], pair["callEvidence"]["line"]),
    )
    raw_publications = sorted(
        {json.dumps(row, sort_keys=True): row for row in raw_publications}.values(),
        key=lambda row: (row["callback"], row["binding"], row["callEvidence"]["line"]),
    )
    return pairs, raw_publications


def top_level_arguments(code: str) -> list[str]:
    """Split one already-balanced C++ call argument list at top-level commas."""
    result: list[str] = []
    start = 0
    depth = 0
    pairs = {"(": ")", "[": "]", "{": "}"}
    reverse = {value: key for key, value in pairs.items()}
    for index, character in enumerate(code):
        if character in pairs:
            depth += 1
        elif character in reverse:
            depth -= 1
            if depth < 0:
                raise RouteError("unbalanced call arguments")
        elif character == "," and depth == 0:
            result.append(code[start:index].strip())
            start = index + 1
    if depth != 0:
        raise RouteError("unterminated nested call arguments")
    result.append(code[start:].strip())
    return result


def top_level_argument_spans(code: str) -> list[tuple[int, int]]:
    """Return source-relative spans for balanced top-level call arguments."""
    spans: list[tuple[int, int]] = []
    start = 0
    depth = 0
    pairs = {"(": ")", "[": "]", "{": "}"}
    reverse = {value: key for key, value in pairs.items()}
    for index, character in enumerate(code):
        if character in pairs:
            depth += 1
        elif character in reverse:
            depth -= 1
            if depth < 0:
                raise RouteError("unbalanced call arguments")
        elif character == "," and depth == 0:
            spans.append((start, index))
            start = index + 1
    if depth != 0:
        raise RouteError("unterminated nested call arguments")
    spans.append((start, len(code)))
    return spans


def interface_callback_from_member_pointer(expression: str) -> str | None:
    match = re.fullmatch(
        r"&\s*DRW_Interface::([A-Za-z_][A-Za-z0-9_]*)", _strip_outer_parentheses(expression)
    )
    return match.group(1) if match is not None else None


def direct_publication_pair(
    body: FunctionBody,
    models: dict[str, dict],
    callbacks: dict[str, dict],
    parents: dict[str, str | None],
    bound: dict,
    binding: str,
    argument_passing: str,
    callback: str,
    call_style: str,
    line: int,
    call_offset: int,
    source_argument_passing: str | None = None,
    callback_passing_via: str = "direct",
    dispatch_ancestry: Iterable[dict] = (),
) -> dict:
    if callback not in callbacks:
        raise RouteError("publication callback is not registered: %s" % callback)
    if bound["model"] not in models:
        raise RouteError("publication model is not registered: %s" % bound["model"])
    callback_route = callbacks[callback]
    compatible, compatibility = callback_accepts_model(
        callback_route, bound["model"], parents, argument_passing
    )
    if not compatible:
        raise RouteError(
            "publication callback/model mismatch in %s: %s -> %s (%s)"
            % (body.symbol, bound["model"], callback, compatibility)
        )
    condition_ancestry = publication_condition_ancestry(
        body, call_offset, dispatch_ancestry
    )
    return {
        "model": bound["model"],
        "modelRoute": models[bound["model"]]["id"],
        "callback": callback,
        "callbackRoute": callback_route["id"],
        "callbackKind": callback_route["selector"].get("callbackKind"),
        "callbackCompatibility": compatibility,
        "binding": binding,
        "bindingKind": bound["bindingKind"],
        "argumentPassing": argument_passing,
        "sourceArgumentPassing": source_argument_passing or argument_passing,
        "callbackPassingVia": callback_passing_via,
        "callStyle": call_style,
        "branchContext": condition_ancestry[-1],
        "conditionAncestry": condition_ancestry,
        "callEvidence": _source_location(body, line),
        "bindingEvidence": bound["evidence"],
    }


def raw_dwg_publication(
    body: FunctionBody,
    callbacks: dict[str, dict],
    parents: dict[str, str | None],
    binding: str,
    binding_kind: str,
    argument_passing: str,
    callback: str,
    line: int,
    call_offset: int,
    binding_evidence: dict,
    source_argument_passing: str | None = None,
    callback_passing_via: str = "direct",
    call_style: str = "interface-call",
    dispatch_ancestry: Iterable[dict] = (),
) -> dict:
    if callback != "addUnsupportedObject" or callback not in callbacks:
        raise RouteError("DWG raw carrier uses an unreviewed callback: %s" % callback)
    callback_route = callbacks[callback]
    compatible, compatibility = callback_accepts_model(
        callback_route, "DRW_UnsupportedObject", parents, argument_passing
    )
    if not compatible:
        raise RouteError("DWG raw callback compatibility is not proven: %s" % compatibility)
    condition_ancestry = publication_condition_ancestry(
        body, call_offset, dispatch_ancestry
    )
    return {
        "carrier": "DRW_UnsupportedObject",
        "binding": binding,
        "bindingKind": binding_kind,
        "argumentPassing": argument_passing,
        "sourceArgumentPassing": source_argument_passing or argument_passing,
        "callbackPassingVia": callback_passing_via,
        "callStyle": call_style,
        "branchContext": condition_ancestry[-1],
        "conditionAncestry": condition_ancestry,
        "callback": callback,
        "callbackRoute": callback_route["id"],
        "callbackCompatibility": compatibility,
        "callEvidence": _source_location(body, line),
        "bindingEvidence": binding_evidence,
    }


def dwg_switch_case_bodies(body: FunctionBody) -> dict[str, FunctionBody]:
    """Return every direct ``switch (oType)`` arm, preserving fall-through."""
    dispatch = switch_body(body, "oType")
    pattern = re.compile(
        r"\bcase\s+((?:[A-Za-z_]\w*::)*[A-Za-z_]\w*|\d+)\s*:|\b(default)\s*:"
    )
    labels: list[tuple[re.Match[str], str]] = []
    for match in pattern.finditer(dispatch.code):
        depth = dispatch.code[: match.start()].count("{") - dispatch.code[: match.start()].count("}")
        if depth == 1:
            labels.append((match, match.group(1) or match.group(2)))
    result: dict[str, FunctionBody] = {}
    for index, (selected_match, token) in enumerate(labels):
        if token == "default":
            continue
        # Consecutive labels intentionally share the first real branch body.
        # Do not turn a fall-through selector into an empty, generic stage.
        body_label = index
        while body_label + 1 < len(labels):
            next_match, _next_token = labels[body_label + 1]
            if dispatch.code[labels[body_label][0].end() : next_match.start()].strip():
                break
            body_label += 1
        start = labels[body_label][0].end()
        end = (
            labels[body_label + 1][0].start() - 1
            if body_label + 1 < len(labels)
            else len(dispatch.code) - 2
        )
        if token in result:
            raise RouteError("duplicate DWG switch selector: %s in %s" % (token, body.symbol))
        result[token] = FunctionBody(
            body.source,
            body.symbol + "::case(" + token + ")",
            dispatch.body_start + selected_match.start(),
            dispatch.body_start + start,
            dispatch.body_start + end,
        )
    return result


def dwg_switch_case_body(body: FunctionBody, token: str) -> FunctionBody:
    """Return one direct ``switch (oType)`` arm without scanning sibling arms."""
    try:
        return dwg_switch_case_bodies(body)[token]
    except KeyError as exc:
        raise RouteError("DWG switch arm is missing: %s in %s" % (token, body.symbol)) from exc


def dwg_named_branch_bodies(
    body: FunctionBody, name: str, route_lines: set[int]
) -> list[PublicationDispatchBranch]:
    """Return the exact named-class ``if`` statements evidenced by one route."""
    result: list[PublicationDispatchBranch] = []
    for match in re.finditer(r"\bif\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        line = line_number(body.source.text, body.body_start + match.start())
        if route_lines and line not in route_lines:
            continue
        condition = FunctionBody(
            body.source, body.symbol, body.body_start + opening,
            body.body_start + opening, body.body_start + closing,
        )
        if name not in {value for value, _line in named_class_literals(condition)}:
            continue
        start, end = statement_span(body.code, closing + 1)
        branch = FunctionBody(
                body.source,
                body.symbol + "::class(" + name + ")",
                body.body_start + match.start(),
                body.body_start + start,
                body.body_start + end,
            )
        condition_text = " ".join(body.comment_text[opening + 1 : closing].split())
        selector_context = {
            "conditionFingerprint": sha256_text(condition_text),
            "branchKind": "then",
            "branchGroupFingerprint": sha256_text(
                "%s:%d" % (body.symbol, body.body_start + match.start())
            ),
            "sourceEvidence": _source_location(body, line),
        }
        result.append(
            PublicationDispatchBranch(
                branch,
                tuple(canonical_condition_ancestry([
                    *enclosing_dispatch_ancestry(body, match.start()), selector_context,
                ])),
            )
        )
    return result


def dwg_named_branch_body_map(
    body: FunctionBody,
) -> dict[tuple[str, int], list[PublicationDispatchBranch]]:
    result: dict[tuple[str, int], list[PublicationDispatchBranch]] = {}
    for match in re.finditer(r"\bif\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        line = line_number(body.source.text, body.body_start + match.start())
        condition = FunctionBody(
            body.source, body.symbol, body.body_start + opening,
            body.body_start + opening, body.body_start + closing,
        )
        values = {value for value, _line in named_class_literals(condition)}
        if not values:
            continue
        start, end = statement_span(body.code, closing + 1)
        for name in values:
            branch = FunctionBody(
                    body.source,
                    body.symbol + "::class(" + name + ")",
                    body.body_start + match.start(),
                    body.body_start + start,
                    body.body_start + end,
                )
            condition_text = " ".join(body.comment_text[opening + 1 : closing].split())
            selector_context = {
                "conditionFingerprint": sha256_text(condition_text),
                "branchKind": "then",
                "branchGroupFingerprint": sha256_text(
                    "%s:%d" % (body.symbol, body.body_start + match.start())
                ),
                "sourceEvidence": _source_location(body, line),
            }
            result.setdefault((name, line), []).append(
                PublicationDispatchBranch(
                    branch,
                    tuple(canonical_condition_ancestry([
                        *enclosing_dispatch_ancestry(body, match.start()), selector_context,
                    ])),
                )
            )
    return result


def build_dwg_publication_branch_cache(source: SourceFile) -> dict[str, object]:
    entity = function_body(source, "dwgReader::readDwgEntityWithOutput")
    objects = function_body(source, "dwgReader::readDwgObject")
    return {
        "fixed-entity": dwg_switch_case_bodies(entity),
        "fixed-object": dwg_switch_case_bodies(objects),
        "named-entity-class": dwg_named_branch_body_map(entity),
        "named-object-class": dwg_named_branch_body_map(objects),
    }


def dwg_fixed_selector_ancestry(source_route: dict, dispatch_symbol: str) -> tuple[dict, ...]:
    """Retain the ``switch (oType)`` selector omitted by a narrowed arm."""
    selector = source_route["selector"]
    token = selector.get("enumSymbol")
    if not isinstance(token, str):
        raise RouteError("fixed DWG selector lacks its enum spelling")
    evidence = source_route.get("evidence")
    if not isinstance(evidence, list) or not evidence:
        raise RouteError("fixed DWG selector lacks source evidence")
    location = evidence[0]
    if not isinstance(location, dict):
        raise RouteError("fixed DWG selector evidence is malformed")
    condition = "oType == " + token
    return (
        {
            "conditionFingerprint": sha256_text(condition),
            "branchKind": "case",
            "branchGroupFingerprint": sha256_text(dispatch_symbol + ":switch(oType)"),
            "sourceEvidence": location,
        },
    )


def dwg_publication_branch_bodies(
    source_route: dict, source: SourceFile, cache: dict[str, object] | None = None
) -> list[PublicationDispatchBranch]:
    category = source_route["category"]
    selector = source_route["selector"]
    if category == "fixed-entity":
        token = selector.get("enumSymbol")
        if not isinstance(token, str):
            raise RouteError("fixed DWG entity lacks enum selector")
        lookup = cache[category] if cache is not None else dwg_switch_case_bodies(
            function_body(source, "dwgReader::readDwgEntityWithOutput")
        )
        try:
            return [PublicationDispatchBranch(  # type: ignore[index]
                lookup[token],
                dwg_fixed_selector_ancestry(
                    source_route, "dwgReader::readDwgEntityWithOutput"
                ),
            )]
        except KeyError as exc:
            raise RouteError("fixed DWG entity selector is missing: %s" % token) from exc
    if category == "fixed-object":
        token = selector.get("enumSymbol")
        if not isinstance(token, str):
            raise RouteError("fixed DWG object lacks enum selector")
        lookup = cache[category] if cache is not None else dwg_switch_case_bodies(
            function_body(source, "dwgReader::readDwgObject")
        )
        try:
            return [PublicationDispatchBranch(  # type: ignore[index]
                lookup[token],
                dwg_fixed_selector_ancestry(source_route, "dwgReader::readDwgObject"),
            )]
        except KeyError as exc:
            raise RouteError("fixed DWG object selector is missing: %s" % token) from exc
    if category in {"named-entity-class", "named-object-class"}:
        name = selector.get("canonical")
        if not isinstance(name, str):
            raise RouteError("named DWG route lacks canonical class spelling")
        lines = {item["line"] for item in source_route["evidence"]}
        if cache is None:
            symbol = (
                "dwgReader::readDwgEntityWithOutput"
                if category == "named-entity-class"
                else "dwgReader::readDwgObject"
            )
            return dwg_named_branch_bodies(function_body(source, symbol), name, lines)
        lookup = cache[category]
        result: list[PublicationDispatchBranch] = []
        for line in lines:
            result.extend(lookup.get((name, line), []))  # type: ignore[union-attr]
        return result
    if category == "table-descriptor":
        return []
    raise RouteError("unexpected DWG parser-publication category: %s" % category)


def dwg_direct_publication(
    body: FunctionBody,
    models: dict[str, dict],
    callbacks: dict[str, dict],
    parents: dict[str, str | None],
    dispatch_ancestry: Iterable[dict] = (),
) -> tuple[list[dict], list[dict]]:
    """Extract physical DWG append/emit callback pairs from one dispatch arm."""
    bindings = _dxf_local_typed_bindings(body, set(models))
    pairs: list[dict] = []
    raw_rows: list[dict] = []

    def consume_bound(
        argument: str, callback: str, call_style: str, line: int, call_offset: int
    ) -> None:
        callback_passing_via = (
            call_style
            if call_style in {"dwg-output-append", "dwg-emit-with-extrusion"}
            else "direct"
        )
        resolved = _direct_binding_name(argument)
        if resolved is None:
            if (
                callback == "addUnsupportedObject"
                and ("makeRawEntity" in argument or "makeRawObject" in argument)
            ):
                raw_rows.append(
                    raw_dwg_publication(
                        body, callbacks, parents, "raw-factory-temporary",
                        "raw-factory-temporary", "object", callback, line,
                        call_offset,
                        _source_location(body, line),
                        callback_passing_via=callback_passing_via,
                        call_style=call_style,
                        dispatch_ancestry=dispatch_ancestry,
                    )
                )
            return
        binding, argument_passing = resolved
        bound = _resolve_dxf_binding(bindings, binding, call_offset)
        if bound is None:
            return
        if bound["model"] == "DRW_UnsupportedObject":
            if callback == "addUnsupportedObject":
                callback_argument_passing = (
                    "pointer"
                    if call_style in {"dwg-output-append", "dwg-emit-with-extrusion"}
                    and callbacks[callback]["selector"].get("parameterPassing") == "pointer"
                    else argument_passing
                )
                raw_rows.append(
                    raw_dwg_publication(
                        body, callbacks, parents, binding, bound["bindingKind"],
                        callback_argument_passing, callback, line, call_offset, bound["evidence"],
                        argument_passing, callback_passing_via, call_style,
                        dispatch_ancestry,
                    )
                )
            return
        callback_argument_passing = (
            "pointer"
            if call_style in {"dwg-output-append", "dwg-emit-with-extrusion"}
            and callbacks[callback]["selector"].get("parameterPassing") == "pointer"
            else argument_passing
        )
        pairs.append(
            direct_publication_pair(
                body, models, callbacks, parents, bound, binding,
                callback_argument_passing, callback, call_style, line,
                call_offset,
                argument_passing, callback_passing_via,
                dispatch_ancestry,
            )
        )

    for match in re.finditer(r"\boutput\s*\.\s*appendValue\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        arguments = top_level_arguments(body.code[opening + 1 : closing])
        callback = interface_callback_from_member_pointer(arguments[1]) if len(arguments) == 2 else None
        if callback is not None:
            consume_bound(
                arguments[0], callback, "dwg-output-append",
                line_number(body.source.text, body.body_start + match.start()), match.start(),
            )

    for match in re.finditer(r"\bemitWithExtrusion\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        arguments = top_level_arguments(body.code[opening + 1 : closing])
        callback = interface_callback_from_member_pointer(arguments[2]) if len(arguments) == 3 else None
        if callback is not None and _strip_outer_parentheses(arguments[1]) == "output":
            consume_bound(
                arguments[0], callback, "dwg-emit-with-extrusion",
                line_number(body.source.text, body.body_start + match.start()), match.start(),
            )

    for invocation in _interface_publications(body, callbacks):
        consume_bound(
            invocation["argument"], invocation["callback"], "interface-call", invocation["line"],
            invocation["offset"],
        )
    # Factory temporaries contain commas, so the narrow ordinary interface
    # call scanner deliberately skips them.  Record only the raw-carrier form.
    for match in re.finditer(r"\bintfa\s*\.\s*addUnsupportedObject\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        argument = body.code[opening + 1 : closing]
        if "makeRawObject" in argument:
            line = line_number(body.source.text, body.body_start + match.start())
            raw_rows.append(
                raw_dwg_publication(
                    body, callbacks, parents, "raw-factory-temporary",
                    "raw-factory-temporary", "object", "addUnsupportedObject", line,
                    match.start(),
                    _source_location(body, line),
                    dispatch_ancestry=dispatch_ancestry,
                )
            )
    pairs = sorted(
        {json.dumps(row, sort_keys=True): row for row in pairs}.values(),
        key=lambda row: (row["model"], row["callback"], row["binding"], row["callEvidence"]["line"]),
    )
    raw_rows = sorted(
        {json.dumps(row, sort_keys=True): row for row in raw_rows}.values(),
        key=lambda row: (row["callback"], row["binding"], row["callEvidence"]["line"]),
    )
    return pairs, raw_rows


def publication_source_routes(collector: RouteCollector) -> list[dict]:
    return [
        route
        for route in collector.routes()
        if route["category"] in PARSER_PUBLICATION_CATEGORIES.get(route["facade"], set())
        and not route["selector"].get("preDispatchDisposition")
    ]


def publication_stage_route_id(name: str) -> str:
    return "shared/publication-stage/" + normalized_identifier(name)


def annotate_publication_delivery_cardinality(
    parser_route: str, pairs: list[dict], raw_publications: list[dict]
) -> None:
    """Fail closed when one parser row exposes duplicate callback endpoints.

    Source lines alone cannot distinguish mutually exclusive guard paths from
    genuine repeated publication.  Keep every physical call, attach its local
    branch evidence, and mark duplicate logical endpoints unresolved so a
    later cardinality mapping cannot quietly promote them to two deliveries.
    """
    for rows, endpoint in (
        (pairs, lambda row: (row["model"], row["callback"])),
        (raw_publications, lambda row: (row["carrier"], row["callback"])),
    ):
        for row in rows:
            ancestry = row.get("conditionAncestry")
            if not isinstance(ancestry, list) or not ancestry:
                raise RouteError(
                    "publication row has no condition ancestry for delivery bundle: %s"
                    % parser_route
                )
            # A bundle crosses typed and raw carriers only when they share the
            # same selected source branch.  Endpoint cardinality remains a
            # separate property below; this is not an exclusivity proof.
            row["deliveryBundle"] = sha256_text(
                parser_route + ":delivery:" + canonical_json(ancestry)
            )
        grouped: dict[tuple[str, str], list[dict]] = {}
        for row in rows:
            grouped.setdefault(endpoint(row), []).append(row)
        for key, grouped_rows in grouped.items():
            if len(grouped_rows) == 1:
                ancestry = grouped_rows[0]["conditionAncestry"]
                # A unique source endpoint does not prove an invocation count:
                # readers commonly loop, and even a case-break says nothing
                # about the enclosing loop's post-callback exit.  Until a
                # reviewed terminal/control-flow proof is represented on this
                # row, fail closed as unresolved.  The exact branch bundle is
                # still useful for typed/raw co-delivery analysis.
                _ = ancestry
                grouped_rows[0]["deliveryCardinality"] = "conditional-or-repeated-unresolved"
                grouped_rows[0]["alternativeBranchGroup"] = sha256_text(
                    parser_route + ":unresolved:" + key[0] + ":" + key[1]
                )
                continue
            group = sha256_text(parser_route + ":" + key[0] + ":" + key[1])
            for row in grouped_rows:
                row["deliveryCardinality"] = "conditional-or-repeated-unresolved"
                row["alternativeBranchGroup"] = group


PUBLICATION_CONDITION_CONTEXT_KEYS = {
    "conditionFingerprint", "branchKind", "branchGroupFingerprint", "sourceEvidence",
}
PUBLICATION_CONDITION_BRANCH_KINDS = {
    "then", "else", "unconditional", "case", "case-break", "case-fallthrough",
    "case-unresolved", "fallthrough-after-then-return",
}


def validate_publication_condition_ancestry(
    value: object, branch_context: object, tree: SourceTree, parser_route: str
) -> None:
    """Validate ordered, source-owned predicate evidence for one delivery."""
    if not isinstance(value, list) or not value:
        raise RouteError("parser-publication condition ancestry is absent: %s" % parser_route)
    previous_line = -1
    for context in value:
        if (
            not isinstance(context, dict)
            or set(context) != PUBLICATION_CONDITION_CONTEXT_KEYS
            or context.get("branchKind") not in PUBLICATION_CONDITION_BRANCH_KINDS
            or not all(
                isinstance(context.get(key), str) and context[key]
                for key in (
                    "conditionFingerprint", "branchKind", "branchGroupFingerprint"
                )
            )
        ):
            raise RouteError("parser-publication condition ancestry is malformed: %s" % parser_route)
        location = context.get("sourceEvidence")
        if (
            not isinstance(location, dict)
            or set(location) != {"path", "symbol", "line", "spanSha256"}
            or location.get("path") not in tree.files
            or not isinstance(location.get("line"), int)
            or location["line"] < previous_line
        ):
            raise RouteError("parser-publication condition evidence is malformed: %s" % parser_route)
        previous_line = location["line"]
    if branch_context != value[-1]:
        raise RouteError("parser-publication branch context is not ancestry tail: %s" % parser_route)


def _validate_raw_publication_location(
    value: object, tree: SourceTree, symbols: set[str], field: str, route_id: str
) -> dict:
    if (
        not isinstance(value, dict)
        or set(value) != {"path", "symbol", "line", "spanSha256"}
        or value.get("path") not in tree.files
        or value.get("symbol") not in symbols
        or not isinstance(value.get("line"), int)
        or value["line"] < 1
        or not isinstance(value.get("spanSha256"), str)
        or not value["spanSha256"]
    ):
        raise RouteError("raw publication %s evidence is malformed: %s" % (field, route_id))
    return value


def _validate_raw_publication_row(
    row: object,
    tree: SourceTree,
    symbols: set[str],
    predecessor_route_ids: list[str],
    model: str,
    model_route_id: str,
    binding: str,
    callback: str,
    callback_route_id: str,
    route_id: str,
) -> None:
    if not isinstance(row, dict):
        raise RouteError("raw publication evidence row is malformed: %s" % route_id)
    if (
        row.get("carrier") != model
        or row.get("carrierRoute") != model_route_id
        or row.get("binding") != binding
        or row.get("bindingKind") != "local"
        or row.get("argumentPassing") != "object"
        or row.get("callbackPassingVia") != "direct"
        or row.get("callStyle") != "interface-call"
        or row.get("callback") != callback
        or row.get("callbackRoute") != callback_route_id
        or row.get("callbackCompatibility") not in {"exact", "declared-ancestor"}
        or row.get("rawFlowPredecessorRouteIds") != predecessor_route_ids
    ):
        raise RouteError("raw publication binding/route contract changed: %s" % route_id)
    call = _validate_raw_publication_location(
        row.get("callEvidence"), tree, symbols, "call", route_id
    )
    binding_evidence = _validate_raw_publication_location(
        row.get("bindingEvidence"), tree, symbols, "binding", route_id
    )
    if call["symbol"] != binding_evidence["symbol"] or call["path"] != binding_evidence["path"]:
        raise RouteError("raw publication call/binding ownership changed: %s" % route_id)
    validate_publication_condition_ancestry(
        row.get("conditionAncestry"), row.get("branchContext"), tree, route_id
    )


def validate_raw_route_publication_metadata(
    tree: SourceTree,
    route: dict,
    raw_ids: set[str],
    model_route_ids: dict[str, str],
    callback_route_ids: dict[str, str],
) -> None:
    """Validate exact DXF raw callback rows attached to raw-flow terminals."""
    selector = route["selector"]
    name = selector.get("name")
    if route["facade"] != "dxfRW" or selector.get("phase") != "publication":
        if "rawPublicationEvidence" in selector and selector.get("rawPublicationEvidence") != []:
            raise RouteError("non-DXF raw-flow publication has callback evidence: %s" % route["id"])
        return
    rows = selector.get("rawPublicationEvidence")
    if not isinstance(rows, list):
        raise RouteError("DXF raw-flow publication evidence is absent: %s" % route["id"])
    predecessor_route_ids = [
        route_id for route_id in selector.get("inputRouteIds", [])
        if isinstance(route_id, str)
    ]
    if predecessor_route_ids != sorted(predecessor_route_ids) or not predecessor_route_ids:
        raise RouteError("DXF raw-flow publication predecessors are malformed: %s" % route["id"])
    if not set(predecessor_route_ids) <= raw_ids:
        raise RouteError("DXF raw-flow publication has a dangling predecessor: %s" % route["id"])

    if name in RAW_DXF_SINGLE_PUBLICATION_RULES:
        expected_symbol, model, binding, callback = RAW_DXF_SINGLE_PUBLICATION_RULES[name]
        if len(rows) != 1:
            raise RouteError("DXF raw publication row count changed: %s" % route["id"])
        _validate_raw_publication_row(
            rows[0], tree, {expected_symbol}, predecessor_route_ids,
            model, model_route_ids.get(model, ""), binding, callback,
            callback_route_ids.get(callback, ""), route["id"],
        )
        return

    if name == "dxf-publish-proxy":
        if len(rows) != len(RAW_DXF_PROXY_PUBLICATION_RULES):
            raise RouteError("DXF proxy raw publication row count changed: %s" % route["id"])
        seen_symbols: set[str] = set()
        for row in rows:
            if not isinstance(row, dict):
                raise RouteError("DXF proxy raw publication row is malformed: %s" % route["id"])
            call = row.get("callEvidence")
            symbol = call.get("symbol") if isinstance(call, dict) else None
            if symbol not in RAW_DXF_PROXY_PUBLICATION_RULES or symbol in seen_symbols:
                raise RouteError("DXF proxy raw publication symbol changed: %s" % route["id"])
            seen_symbols.add(symbol)
            (
                typed_model, typed_binding, typed_callback,
                raw_model, raw_binding, raw_callback,
            ) = RAW_DXF_PROXY_PUBLICATION_RULES[symbol]
            _validate_raw_publication_row(
                row, tree, {symbol}, predecessor_route_ids,
                raw_model, model_route_ids.get(raw_model, ""), raw_binding,
                raw_callback, callback_route_ids.get(raw_callback, ""), route["id"],
            )
            if row.get("typedToRawRelation") != "typed-callback-before-raw-carrier":
                raise RouteError("DXF proxy typed/raw relation changed: %s" % route["id"])
            typed = row.get("typedPredecessor")
            _validate_raw_publication_row(
                typed, tree, {symbol}, predecessor_route_ids,
                typed_model, model_route_ids.get(typed_model, ""), typed_binding,
                typed_callback, callback_route_ids.get(typed_callback, ""), route["id"],
            )
            raw_line = row["callEvidence"]["line"]
            typed_line = typed["callEvidence"]["line"]
            if typed_line >= raw_line or not _same_publication_branch(
                typed.get("conditionAncestry", []), row.get("conditionAncestry", [])
            ):
                raise RouteError("DXF proxy typed/raw order or branch changed: %s" % route["id"])
        if seen_symbols != set(RAW_DXF_PROXY_PUBLICATION_RULES):
            raise RouteError("DXF proxy raw publication coverage is incomplete: %s" % route["id"])
        return

    if rows:
        raise RouteError("unexpected DXF raw-flow publication evidence: %s" % route["id"])


def validate_raw_eligibility_metadata(tree: SourceTree, route: dict) -> None:
    """Validate shape and ordering of concrete DXF raw eligibility edges."""
    selector = route["selector"]
    name = selector.get("name")
    expected_rules = RAW_DXF_ELIGIBILITY_RULES.get(name)
    if name == "dxf-read-typed-raw-template":
        expected_symbols = {"dxfRW::processRawCapturedObject"}
        expected_edges = {
            "dxfRW::processRawCapturedObject":
            ("boundary-acceptance", "typed-callback", "raw-callback", "return")
        }
    elif expected_rules is not None:
        expected_symbols = set(expected_rules)
        expected_edges = {
            symbol: tuple(spec["name"] for spec in specs)
            for symbol, specs in expected_rules.items()
        }
    else:
        if selector.get("rawEligibilityEvidence", []) not in ([], None):
            raise RouteError("unexpected raw eligibility evidence: %s" % route["id"])
        return
    rows = selector.get("rawEligibilityEvidence")
    if not isinstance(rows, list) or {row.get("bodySymbol") for row in rows if isinstance(row, dict)} != expected_symbols:
        raise RouteError("raw eligibility body coverage changed: %s" % route["id"])
    seen_symbols: set[str] = set()
    for row in rows:
        if not isinstance(row, dict):
            raise RouteError("raw eligibility row is malformed: %s" % route["id"])
        symbol = row.get("bodySymbol")
        if symbol not in expected_symbols or symbol in seen_symbols:
            raise RouteError("raw eligibility body is duplicated or unexpected: %s" % route["id"])
        seen_symbols.add(symbol)
        edges = row.get("edges")
        names = expected_edges[symbol]
        if not isinstance(edges, list) or len(edges) != len(names):
            raise RouteError("raw eligibility edge count changed: %s" % route["id"])
        previous_offset = -1
        for order, edge in enumerate(edges, 1):
            if not isinstance(edge, dict):
                raise RouteError("raw eligibility edge is malformed: %s" % route["id"])
            if (
                edge.get("edge") != names[order - 1]
                or edge.get("order") != order
                or edge.get("predecessorEdges") != list(names[: order - 1])
                or not isinstance(edge.get("callee"), str)
                or not isinstance(edge.get("calleeOverload"), str)
                or not isinstance(edge.get("relation"), str)
                or not isinstance(edge.get("sourceOffset"), int)
                or edge["sourceOffset"] <= previous_offset
                or not isinstance(edge.get("guardFingerprint"), str)
            ):
                raise RouteError("raw eligibility edge contract changed: %s" % route["id"])
            previous_offset = edge["sourceOffset"]
            _validate_raw_publication_location(
                edge.get("callEvidence"), tree, {symbol}, "eligibility", route["id"]
            )


def validate_transport_selection_metadata(
    tree: SourceTree, route: dict
) -> None:
    selector = route["selector"]
    selection = selector.get("selection")
    implementation = selector.get("implementation")
    try:
        rule = DXF_TRANSPORT_SELECTION_RULES[selection]
    except (KeyError, TypeError) as exc:
        raise RouteError("unknown DXF transport selection evidence: %s" % route["id"]) from exc
    evidence = selector.get("transportSelectionEvidence")
    if not isinstance(evidence, dict):
        raise RouteError("DXF transport selection evidence is absent: %s" % route["id"])
    if (
        evidence.get("selection") != selection
        or evidence.get("implementation") != implementation
        or evidence.get("branch") != rule["branch"]
    ):
        raise RouteError("DXF transport selection evidence changed: %s" % route["id"])
    guards = evidence.get("guardPredicates")
    patterns = rule["guardPatterns"]
    if not isinstance(guards, list) or len(guards) != len(patterns):
        raise RouteError("DXF transport guard evidence changed: %s" % route["id"])
    previous_offset = -1
    for order, guard in enumerate(guards, 1):
        if (
            not isinstance(guard, dict)
            or guard.get("order") != order
            or not isinstance(guard.get("sourceOffset"), int)
            or guard["sourceOffset"] <= previous_offset
            or not isinstance(guard.get("predicateFingerprint"), str)
        ):
            raise RouteError("DXF transport guard evidence is malformed: %s" % route["id"])
        previous_offset = guard["sourceOffset"]
        location = _validate_raw_publication_location(
            guard.get("sourceEvidence"), tree, {selector["entrypoint"]},
            "transport-guard", route["id"],
        )
        if location["path"] != "src/libdxfrw.cpp":
            raise RouteError("DXF transport guard path changed: %s" % route["id"])
    construction = evidence.get("construction")
    if (
        not isinstance(construction, dict)
        or not isinstance(construction.get("sourceOffset"), int)
        or construction["sourceOffset"] <= previous_offset
    ):
        raise RouteError("DXF transport constructor evidence is malformed: %s" % route["id"])
    location = _validate_raw_publication_location(
        construction.get("sourceEvidence"), tree, {selector["entrypoint"]},
        "transport-constructor", route["id"],
    )
    if location["path"] != "src/libdxfrw.cpp":
        raise RouteError("DXF transport constructor path changed: %s" % route["id"])


def validate_raw_writer_metadata(tree: SourceTree, route: dict) -> None:
    name = route["selector"].get("name")
    rules = RAW_DXF_WRITER_EVIDENCE_RULES.get(name)
    if rules is None:
        if route["selector"].get("rawWriterEvidence", []) not in ([], None):
            raise RouteError("unexpected raw writer evidence: %s" % route["id"])
        return
    rows = route["selector"].get("rawWriterEvidence")
    if not isinstance(rows, list) or {row.get("bodySymbol") for row in rows if isinstance(row, dict)} != set(rules):
        raise RouteError("raw writer body coverage changed: %s" % route["id"])
    for row in rows:
        symbol = row.get("bodySymbol")
        if not isinstance(row, dict) or symbol not in rules:
            raise RouteError("raw writer row is malformed: %s" % route["id"])
        names = tuple(spec["name"] for spec in rules[symbol])
        edges = row.get("edges")
        if not isinstance(edges, list) or len(edges) != len(names):
            raise RouteError("raw writer edge count changed: %s" % route["id"])
        previous_offset = -1
        for order, edge in enumerate(edges, 1):
            if (
                not isinstance(edge, dict)
                or edge.get("edge") != names[order - 1]
                or edge.get("order") != order
                or edge.get("predecessorEdges") != list(names[: order - 1])
                or not isinstance(edge.get("sourceOffset"), int)
                or edge["sourceOffset"] <= previous_offset
                or not isinstance(edge.get("callee"), str)
                or not isinstance(edge.get("calleeOverload"), str)
                or not isinstance(edge.get("relation"), str)
                or not isinstance(edge.get("guardFingerprint"), str)
            ):
                raise RouteError("raw writer edge contract changed: %s" % route["id"])
            previous_offset = edge["sourceOffset"]
            _validate_raw_publication_location(
                edge.get("callEvidence"), tree, {symbol}, "raw-writer", route["id"]
            )


def validate_dwg_raw_replay_metadata(tree: SourceTree, route: dict) -> None:
    """Fail closed on DWG raw replay identity and ordered writer edges."""
    name = route["selector"].get("name")
    rules = DWG_RAW_REPLAY_EVIDENCE_RULES.get(name)
    if rules is None:
        if route["selector"].get("dwgRawReplayEvidence", []) not in ([], None):
            raise RouteError("unexpected DWG raw replay evidence: %s" % route["id"])
        return
    rows = route["selector"].get("dwgRawReplayEvidence")
    expected = {(path, symbol) for path, path_rules in rules.items() for symbol in path_rules}
    if not isinstance(rows, list) or {
        (row.get("sourcePath"), row.get("bodySymbol"))
        for row in rows if isinstance(row, dict)
    } != expected:
        raise RouteError("DWG raw replay body coverage changed: %s" % route["id"])
    seen: set[tuple[str, str]] = set()
    for row in rows:
        if not isinstance(row, dict):
            raise RouteError("DWG raw replay row is malformed: %s" % route["id"])
        path = row.get("sourcePath")
        symbol = row.get("bodySymbol")
        key = (path, symbol)
        if key not in expected or key in seen:
            raise RouteError("DWG raw replay row is duplicated or unexpected: %s" % route["id"])
        seen.add(key)
        edge_specs = rules[path][symbol]
        names = tuple(spec["name"] for spec in edge_specs)
        edges = row.get("edges")
        if not isinstance(edges, list) or len(edges) != len(names):
            raise RouteError("DWG raw replay edge count changed: %s" % route["id"])
        previous_offset = -1
        for order, edge in enumerate(edges, 1):
            if (
                not isinstance(edge, dict)
                or edge.get("edge") != names[order - 1]
                or edge.get("order") != order
                or edge.get("predecessorEdges") != list(names[: order - 1])
                or not isinstance(edge.get("callee"), str)
                or not isinstance(edge.get("calleeOverload"), str)
                or not isinstance(edge.get("relation"), str)
                or not isinstance(edge.get("sourceOffset"), int)
                or edge["sourceOffset"] <= previous_offset
                or not isinstance(edge.get("guardFingerprint"), str)
            ):
                raise RouteError("DWG raw replay edge contract changed: %s" % route["id"])
            previous_offset = edge["sourceOffset"]
            _validate_raw_publication_location(
                edge.get("callEvidence"), tree, {symbol}, "dwg-raw-replay", route["id"]
            )
            if edge["callEvidence"]["path"] != path:
                raise RouteError("DWG raw replay edge path changed: %s" % route["id"])


def validate_named_publication_selector(
    source_route: dict, ancestry: object, parser_route: str
) -> None:
    """Require a named DWG publication to retain its exact selector pair."""
    if source_route["category"] not in {"named-entity-class", "named-object-class"}:
        return
    selectors = source_route["selector"].get("conditionSelectors")
    if (
        not isinstance(selectors, list)
        or not selectors
        or any(
            not isinstance(item, dict)
            or set(item) != {"line", "conditionFingerprint"}
            or not isinstance(item.get("line"), int)
            or not isinstance(item.get("conditionFingerprint"), str)
            or not item["conditionFingerprint"]
            for item in selectors
        )
    ):
        raise RouteError("named DWG source selector metadata is malformed: %s" % parser_route)
    expected = {
        (item["line"], item["conditionFingerprint"])
        for item in selectors
    }
    if not isinstance(ancestry, list) or not any(
        context.get("branchKind") == "then"
        and (
            context.get("sourceEvidence", {}).get("line"),
            context.get("conditionFingerprint"),
        ) in expected
        for context in ancestry
        if isinstance(context, dict)
    ):
        raise RouteError("named DWG publication lost its exact selector: %s" % parser_route)


def dxf_stage_references(target: str, body: FunctionBody) -> list[dict]:
    names: list[tuple[str, str]] = []
    if target == "processBlock":
        names.append(("dxf-block-event-flush", "conditional-block-buffered"))
    # Raw-captured wrappers now emit both exact typed and raw publication rows
    # with invocation/template evidence.  Do not retain a generic stage as a
    # second, self-referential substitute for that physical delivery.
    _ = body
    return [
        {"routeId": publication_stage_route_id(name), "reason": reason}
        for name, reason in sorted(set(names))
    ]


def dwg_stage_references(category: str, bodies: list[FunctionBody]) -> list[dict]:
    """Bridge only to concrete DWG publication machinery, never back to self."""
    names: list[tuple[str, str]] = []
    for body in bodies:
        if "appendValue" in body.code:
            names.append(("entity-output-append", "buffered-output-event"))
        if "emitWithExtrusion" in body.code:
            names.append(("emit-with-extrusion", "extrusion-output-helper"))
        if "entryParse" in body.code:
            names.append(("entry-parse", "typed-parse-before-publication"))
    if category == "table-descriptor":
        names.append(("deferred-table-publication", "descriptor-to-deferred-table-frame"))
        names.append(("dwg-facade-process", "table-callback-consumed-by-facade-loop"))
    if not names:
        fallback = DWG_STAGED_PUBLICATION_CATEGORIES[category]
        names.append((fallback, "branch-local-pair-not-proved"))
    return [
        {"routeId": publication_stage_route_id(name), "reason": reason}
        for name, reason in sorted(set(names))
    ]


def verify_dwg_output_callback_bridge(tree: SourceTree) -> None:
    """Prove the object-to-pointer adaptation used by DWG output helpers."""
    append = function_body(
        tree.require("src/intern/dwgreader.cpp"),
        "dwgReader::DwgEntityOutput::appendValue",
    )
    required_append_fragments = (
        "std::is_invocable_v<Callback, DRW_Interface &, const T &>",
        "std::invoke(m_callback, target, m_value)",
        "std::invoke(m_callback, target, &m_value)",
        "append(std::make_unique<ReferenceEvent>",
        "append(std::make_unique<PointerEvent>",
    )
    if any(fragment not in append.code for fragment in required_append_fragments):
        raise RouteError("DWG appendValue callback adaptation changed without review")
    extrusion = function_body(
        tree.require("src/intern/dwgreader.h"), "emitWithExtrusion"
    )
    if "output.appendValue" not in extrusion.code:
        raise RouteError("DWG emitWithExtrusion no longer delegates to appendValue")


def verify_dxf_raw_captured_template_bridge(tree: SourceTree) -> dict | None:
    """Prove and locate the template's capture/boundary/publish contract."""
    source = tree.require("src/libdxfrw.cpp")
    if "dxfRW::processRawCapturedObject" not in source.code:
        # Focused synthetic publication tests do not need to re-create the
        # whole wrapper; their lambda test still proves that only an explicit
        # wrapper invocation earns the template call style.
        return None
    body = function_body(source, "dxfRW::processRawCapturedObject")
    required_fragments = (
        "acceptObjectBoundary(code)",
        "addTyped(data)",
        "iface->addRawDxfObject(raw)",
        "captureAndParseRawDxfGroup(raw, code, data)",
    )
    if any(fragment not in body.code for fragment in required_fragments):
        raise RouteError("DXF raw-captured template bridge changed without review")
    ordered_branch: tuple[int, int, int, int, int] | None = None
    for match in re.finditer(r"\bif\s*\(", body.code):
        opening = body.code.find("(", match.start(), match.end())
        closing = matching_delimiter(body.code, opening, "(", ")")
        condition = " ".join(body.comment_text[opening + 1 : closing].split())
        if condition not in {"code == 0", "0 == code"}:
            continue
        branch_start, branch_end = statement_span(body.code, closing + 1)
        branch_code = body.code[branch_start : branch_end + 1]
        boundary = branch_code.find("acceptObjectBoundary(code)")
        typed = branch_code.find("addTyped(data)")
        raw = branch_code.find("iface->addRawDxfObject(raw)")
        returns = [item.start() for item in re.finditer(r"\breturn\b", branch_code)]
        return_offset = next((position for position in returns if position > raw), -1)
        if boundary >= 0 and typed > boundary and raw > typed and return_offset >= 0:
            ordered_branch = (branch_start, boundary, typed, raw, return_offset)
            break
    if ordered_branch is None:
        raise RouteError(
            "DXF raw-captured template lacks ordered boundary/typed/raw/return branch"
        )
    branch_start, boundary_offset, typed_offset, raw_offset, return_offset = ordered_branch
    raw_call_offset = branch_start + raw_offset
    raw_call = re.search(r"\biface\s*->\s*addRawDxfObject\s*\(\s*raw\s*\)", body.code)
    raw_binding = re.search(r"\bDRW_RawDxfObject\s+raw\s*;", body.code)
    if raw_call is None or raw_binding is None or raw_call.start() != raw_call_offset:
        raise RouteError("DXF raw-captured template lacks physical raw carrier evidence")
    ordered_specs = (
        ("boundary-acceptance", boundary_offset, "acceptObjectBoundary", "acceptObjectBoundary(int)", "eligibility-reject"),
        ("typed-callback", typed_offset, "AddFn(data)", "bound AddFn(data)", "typed-publication"),
        ("raw-callback", raw_offset, "iface->addRawDxfObject", "DRW_Interface::addRawDxfObject(DRW_RawDxfObject&)", "raw-publication"),
        ("return", return_offset, "return", "bool-return", "terminal"),
    )
    ordered_edges = []
    template_text = body.comment_text
    for order, (name, offset, callee, overload, relation) in enumerate(ordered_specs, 1):
        ordered_edges.append(
            {
                "edge": name,
                "order": order,
                "predecessorEdges": [item[0] for item in ordered_specs[: order - 1]],
                "callee": callee,
                "calleeOverload": overload,
                "relation": relation,
                "sourceOffset": branch_start + offset,
                "guardFingerprint": sha256_text(
                    " ".join(template_text[branch_start + offset : branch_start + offset + len(callee)].split())
                ),
                "callEvidence": _source_location(
                    body,
                    line_number(
                        body.source.text, body.body_start + branch_start + offset
                    ),
                ),
            }
        )
    return {
        "orderedEdges": ordered_edges,
        "templateEvidence": _source_location(body, body.line),
        "rawCallbackEvidence": _source_location(
            body, line_number(body.source.text, body.body_start + raw_call.start())
        ),
        "rawBindingEvidence": _source_location(
            body, line_number(body.source.text, body.body_start + raw_binding.start())
        ),
    }


def add_parser_publication_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Bind parser dispatch to proven pairs or an explicit publication bridge."""
    models = _publication_model_routes(collector)
    callbacks = _publication_callback_routes(collector)
    member_bindings = _dxf_member_bindings(tree, set(models))
    parents = publication_model_parents(tree, set(models))
    if "src/intern/dwgreader.h" in tree.files:
        verify_dwg_output_callback_bridge(tree)
    dxf_source = tree.require("src/libdxfrw.cpp")
    raw_template_bridge = verify_dxf_raw_captured_template_bridge(tree)
    dwg_source = tree.require("src/intern/dwgreader.cpp")
    dwg_branch_cache = (
        build_dwg_publication_branch_cache(dwg_source)
        if any(route["facade"] == "dwgRW" for route in publication_source_routes(collector))
        else None
    )
    for source_route in publication_source_routes(collector):
        facade = source_route["facade"]
        source_selector = source_route["selector"]
        target = source_selector.get("target")
        direct_pairs: list[dict] = []
        raw_publications: list[dict] = []
        stage_refs: list[dict] = []
        evidence_body: FunctionBody
        if facade == "dxfRW" and isinstance(target, str):
            bodies = function_bodies(dxf_source, "dxfRW::" + target)
            if not bodies:
                raise RouteError("DXF parser-publication target lacks a definition: %s" % target)
            evidence_body = bodies[0]
            for body in bodies:
                pairs, raw_rows = dxf_direct_publication(
                    body, models, callbacks, member_bindings, parents, raw_template_bridge
                )
                direct_pairs.extend(pairs)
                raw_publications.extend(raw_rows)
                stage_refs.extend(dxf_stage_references(target, body))
        elif facade == "dwgRW":
            branches = dwg_publication_branch_bodies(source_route, dwg_source, dwg_branch_cache)
            if branches:
                evidence_body = branches[0].body
                for branch in branches:
                    pairs, raw_rows = dwg_direct_publication(
                        branch.body, models, callbacks, parents, branch.dispatch_ancestry
                    )
                    direct_pairs.extend(pairs)
                    raw_publications.extend(raw_rows)
                # ``appendValue`` and ``emitWithExtrusion`` adapt/replay the
                # callback later; retain their concrete stage edge even though
                # the model/callback pair is exact.  Plain ``intfa.add*``
                # calls remain direct physical interface invocations.
                has_direct_delivery = bool(direct_pairs or raw_publications)
                helper_delivery = any(
                    pair["callStyle"] in {"dwg-output-append", "dwg-emit-with-extrusion"}
                    for pair in direct_pairs
                ) or any(
                    row["callStyle"] in {"dwg-output-append", "dwg-emit-with-extrusion"}
                    for row in raw_publications
                )
                if not has_direct_delivery or helper_delivery:
                    stage_refs.extend(
                        dwg_stage_references(
                            source_route["category"], [branch.body for branch in branches]
                        )
                    )
            else:
                stage_refs.extend(dwg_stage_references(source_route["category"], []))
                stage_anchor = next(
                    anchor for anchor in PUBLICATION_STAGE_ANCHORS
                    if anchor[0] == DWG_STAGED_PUBLICATION_CATEGORIES[source_route["category"]]
                )
                evidence_body = function_body(tree.require(stage_anchor[2]), stage_anchor[3])
        else:
            raise RouteError("unexpected parser-publication source route: %s" % source_route["id"])
        direct_pairs = sorted(
            {json.dumps(pair, sort_keys=True): pair for pair in direct_pairs}.values(),
            key=lambda pair: (pair["model"], pair["callback"], pair["binding"], pair["callEvidence"]["line"]),
        )
        raw_publications = sorted(
            {json.dumps(row, sort_keys=True): row for row in raw_publications}.values(),
            key=lambda row: (row["callback"], row["binding"], row["callEvidence"]["line"]),
        )
        annotate_publication_delivery_cardinality(
            source_route["id"], direct_pairs, raw_publications
        )
        stage_refs = sorted(
            {json.dumps(ref, sort_keys=True): ref for ref in stage_refs}.values(),
            key=lambda ref: (ref["routeId"], ref["reason"]),
        )
        has_direct_delivery = bool(direct_pairs or raw_publications)
        if facade == "dxfRW" and not has_direct_delivery and not stage_refs:
            stage_name = DXF_STAGED_PUBLICATION_TARGETS.get(target)
            if stage_name is None:
                stage_name = DXF_STAGED_PUBLICATION_CATEGORIES.get(source_route["category"])
            if stage_name is None:
                raise RouteError("DXF parser-publication route lacks a stage mapping: %s" % source_route["id"])
            stage_refs = [{
                "routeId": publication_stage_route_id(stage_name),
                "reason": "dispatch-or-context",
            }]
        if has_direct_delivery:
            mode = "mixed" if stage_refs else "direct"
        else:
            mode = "stage"
        if mode == "stage" and not stage_refs:
            raise RouteError("parser-publication route is unbound: %s" % source_route["id"])
        selector = {
            "kind": "parser-publication",
            "parserRoute": source_route["id"],
            "parserTarget": target if isinstance(target, str) else None,
            "mode": mode,
            "directPairs": direct_pairs,
            "rawCarrierPublications": raw_publications,
            "stageRouteIds": [ref["routeId"] for ref in stage_refs],
            "stageRefs": stage_refs,
        }
        collector.add(
            facade,
            "parser-publication",
            selector,
            evidence_body,
            identifier=source_route["id"],
            directions=["read", "publish"],
        )
        for pair in direct_pairs:
            collector.add(
                facade,
                "parser-publication",
                selector,
                evidence_body,
                identifier=source_route["id"],
                directions=["read", "publish"],
                line=pair["callEvidence"]["line"],
            )


def add_publication_stage_routes(collector: RouteCollector, tree: SourceTree) -> None:
    """Keep deferred callback/publication machinery visible as first-class edges."""
    for name, anchor_kind, path, symbol in PUBLICATION_STAGE_ANCHORS:
        source = tree.require(path)
        if anchor_kind == "function":
            body = function_body(source, symbol)
        elif anchor_kind == "class":
            body = class_definition_anchor(SourceTree(tree.label, {path: source}, {}), symbol)
        elif anchor_kind == "class-method":
            body = inline_class_method_body(source, symbol)
        else:
            raise RouteError("unknown publication stage anchor kind: %s" % anchor_kind)
        collector.add(
            "shared",
            "publication-stage",
            {
                "kind": "publication-stage",
                "name": name,
                "anchorKind": anchor_kind,
                "sourcePath": path,
                "symbol": symbol,
            },
            body,
            identifier=name,
            directions=["publish"],
        )


def mask_preprocessor_directives(code: str) -> str:
    """Blank preprocessor directives without changing offsets or line numbers."""
    output = list(code)
    index = 0
    continued = False
    while index < len(code):
        line_end = code.find("\n", index)
        if line_end < 0:
            line_end = len(code)
        line = code[index:line_end]
        directive = continued or line.lstrip().startswith("#")
        if directive:
            for position in range(index, line_end):
                output[position] = " "
        continued = directive and line.rstrip().endswith("\\")
        index = line_end + 1
    return "".join(output)


def public_header_facade(path: str) -> str:
    if path == "src/libdxfrw.h":
        return "dxfRW"
    if path == "src/libdwgr.h":
        return "dwgRW"
    return "shared"


def public_header_body(source: SourceFile, symbol: str, start: int, end: int) -> FunctionBody:
    return FunctionBody(source, symbol, start, start, end)


def public_scope_prefix(code: str, scope_start: int, position: int) -> str:
    """Return the declaration prefix since the preceding direct separator."""
    boundary = max(
        code.rfind(";", scope_start, position),
        code.rfind("{", scope_start, position),
        code.rfind("}", scope_start, position),
    )
    return code[boundary + 1 : position]


def enum_public_values(source: SourceFile, body_start: int, body_end: int) -> list[dict]:
    """Return ordered direct enum entries with non-reconstructive fingerprints."""
    code = source.code
    entries: list[dict] = []
    start = body_start + 1
    depth = 0
    for index in range(start, body_end + 1):
        char = code[index] if index < body_end else ","
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        if char != "," or depth != 0:
            continue
        fragment = code[start:index].strip()
        start = index + 1
        if not fragment:
            continue
        match = re.match(r"([A-Za-z_][A-Za-z0-9_]*)(?:\s*=\s*(.*))?$", fragment, re.DOTALL)
        if match is None:
            raise RouteError("unsupported enum entry in %s: %s" % (source.path, fragment))
        name, initializer = match.groups()
        entries.append(
            {
                "name": name,
                "ordinal": len(entries),
                "initializerFingerprint": None
                if initializer is None
                else sha256_text(" ".join(initializer.split())),
            }
        )
    if not entries:
        raise RouteError("public enum has no entries in %s" % source.path)
    return entries


def class_body_for_public_scan(source: SourceFile, code: str, start: int, end: int, name: str) -> FunctionBody | None:
    """Find one direct class/struct body starting at a declaration keyword."""
    name_match = re.match(r"\s+([A-Za-z_][A-Za-z0-9_]*)", code[start + len(name) : end])
    if name_match is None:
        return None
    name_start = start + len(name) + name_match.start(1)
    opening = code.find("{", name_start, end)
    semicolon = code.find(";", name_start, end)
    if semicolon >= 0 and (opening < 0 or semicolon < opening):
        return public_header_body(source, name_match.group(1), start, semicolon)
    if opening < 0:
        raise RouteError("class/struct declaration lacks terminator in %s" % source.path)
    return FunctionBody(
        source,
        name_match.group(1),
        start,
        opening,
        matching_delimiter(code, opening),
    )


def public_class_method_entries(
    source: SourceFile,
    code: str,
    class_body: FunctionBody,
    owner: str,
    default_visibility: str,
) -> list[tuple[str, str, str, FunctionBody, str]]:
    """Find direct public class methods, including inline/default/delete forms.

    The return tuples are ``name, signature hash, implementation kind,
    evidence body, visibility``.  Calls, lambdas, and nested class bodies are
    excluded by their brace depth relative to the enclosing class.
    """
    body_start = class_body.body_start
    body_end = class_body.body_end
    access_labels: list[tuple[int, str]] = [(body_start + 1, default_visibility)]
    for match in re.finditer(r"\b(public|private|protected)\s*:", code[body_start + 1 : body_end]):
        absolute = body_start + 1 + match.start()
        depth = code[body_start:absolute].count("{") - code[body_start:absolute].count("}")
        if depth == 1:
            access_labels.append((absolute, match.group(1)))
    access_labels.sort()

    def visibility_at(position: int) -> str:
        value = default_visibility
        for label_position, label in access_labels:
            if label_position > position:
                break
            value = label
        return value

    pattern = re.compile(
        r"(?<![A-Za-z0-9_:])"
        r"(operator\s*(?:[A-Za-z_][A-Za-z0-9_:<>]*|[^\s(]+)|~?[A-Za-z_][A-Za-z0-9_]*)\s*\("
    )
    result: list[tuple[str, str, str, FunctionBody, str]] = []
    seen: set[tuple[str, str]] = set()
    for match in pattern.finditer(code[body_start + 1 : body_end]):
        absolute = body_start + 1 + match.start(1)
        depth = code[body_start:absolute].count("{") - code[body_start:absolute].count("}")
        if depth != 1 or visibility_at(absolute) != "public":
            continue
        open_paren = code.find("(", absolute, body_end)
        close_paren = matching_delimiter(code, open_paren, "(", ")")
        suffix = code[close_paren + 1 : body_end]
        terminator = re.match(
            r"\s*(?:(?:(?:const|volatile|override|final|&|&&)|(?:noexcept(?:\s*\([^()]*\))?)|(?:->\s*[A-Za-z_:][A-Za-z0-9_:<>,\s*&]*))\s*)*(\{|;|=\s*(?:default|delete)\s*;)",
            suffix,
        )
        if terminator is None:
            continue
        token = terminator.group(1)
        terminator_start = close_paren + 1 + terminator.start(1)
        name = " ".join(match.group(1).split())
        signature_start = max(
            code.rfind(";", body_start + 1, absolute),
            code.rfind("{", body_start + 1, absolute),
            code.rfind("}", body_start + 1, absolute),
        ) + 1
        signature = " ".join(code[signature_start : close_paren + 1].split())
        if not signature or re.search(r"\bfriend\b", signature):
            continue
        signature_hash = sha256_text(signature)
        key = (name, signature_hash)
        if key in seen:
            continue
        seen.add(key)
        if token == "{":
            body = FunctionBody(
                source,
                owner + "::" + name,
                absolute,
                terminator_start,
                matching_delimiter(code, terminator_start),
            )
            implementation_kind = "body"
        else:
            end = terminator_start + len(token)
            body = public_header_body(source, owner + "::" + name, absolute, end)
            if token == ";":
                implementation_kind = "declaration"
            elif "default" in token:
                implementation_kind = "defaulted"
            else:
                implementation_kind = "deleted"
        result.append((name, signature_hash, implementation_kind, body, "public"))
    return result


def add_public_header_routes(
    collector: RouteCollector,
    tree: SourceTree,
    public_headers: tuple[str, ...],
) -> None:
    """Inventory declarations in the pinned installed-header surface.

    The parser is intentionally lexical and source-only.  It records API
    declarations/inlines rather than claiming ABI or behavior parity; comments,
    strings, directives, local bodies, and inaccessible nested declarations do
    not become public routes.
    """
    if not public_headers or len(public_headers) != len(set(public_headers)):
        raise RouteError("public-header tuple is empty or non-unique")
    for path in public_headers:
        source = tree.require(path)
        facade = public_header_facade(path)
        code = mask_preprocessor_directives(source.code)
        collector.add(
            facade,
            "public-header",
            {"kind": "installed-header", "path": path, "cmakeList": "LIBDXFRW_PUBLIC_HEADERS"},
            source_unit_body(source),
            identifier=path,
            directions=["public-api"],
            line=1,
        )

        def emit_type(
            kind: str,
            name: str,
            qualified: str,
            form: str,
            visibility: str,
            body: FunctionBody,
            scope: tuple[str, ...],
        ) -> None:
            collector.add(
                facade,
                "public-type",
                {
                    "kind": kind,
                    "declarationForm": form,
                    "qualifiedName": qualified,
                    "scope": "::".join(scope) if scope else "global",
                    "visibility": visibility,
                },
                body,
                identifier="%s:%s:%s" % (path, form, qualified),
                directions=["public-api"],
            )

        def emit_enum(
            qualified: str,
            scoped: bool,
            underlying: str,
            values: list[dict],
            visibility: str,
            body: FunctionBody,
        ) -> None:
            collector.add(
                facade,
                "public-enum",
                {
                    "kind": "enum",
                    "qualifiedName": qualified,
                    "scoped": scoped,
                    "underlyingTypeFingerprint": sha256_text(" ".join(underlying.split())),
                    "values": values,
                    "visibility": visibility,
                },
                body,
                identifier="%s:%s" % (path, qualified),
                directions=["public-api"],
            )

        def emit_alias(
            category: str,
            qualified: str,
            target: str,
            visibility: str,
            body: FunctionBody,
        ) -> None:
            normalized_target = " ".join(target.split())
            selector = {
                "kind": "alias" if category == "public-alias" else "using-declaration",
                "qualifiedName": qualified,
                "targetFingerprint": sha256_text(normalized_target),
                "visibility": visibility,
            }
            if re.fullmatch(r"[A-Za-z_:][A-Za-z0-9_:]*", normalized_target):
                selector["target"] = normalized_target
            collector.add(
                facade,
                category,
                selector,
                body,
                identifier="%s:%s" % (path, qualified),
                directions=["public-api"],
            )

        def emit_class_methods(
            class_body: FunctionBody,
            owner: str,
            default_visibility: str,
            declaration_visibility: str,
        ) -> None:
            if declaration_visibility != "public" or class_body.body_start >= len(code) or code[class_body.body_start] != "{":
                return
            for name, signature_hash, implementation_kind, body, visibility in public_class_method_entries(
                source, code, class_body, owner, default_visibility
            ):
                qualified = owner + "::" + name
                identifier = "%s:%s:%s" % (path, owner, signature_hash)
                selector = {
                    "kind": "method",
                    "qualifiedName": qualified,
                    "owner": owner,
                    "signatureFingerprint": signature_hash,
                    "visibility": visibility,
                }
                collector.add(
                    facade,
                    "public-method",
                    selector,
                    body,
                    identifier=identifier,
                    directions=["public-api"],
                )
                if implementation_kind != "declaration":
                    collector.add(
                        facade,
                        "public-inline-method",
                        {**selector, "implementationKind": implementation_kind},
                        body,
                        identifier=identifier,
                        directions=["public-api"],
                    )

        def emit_free_inline(opening: int, scope_start: int, scope: tuple[str, ...]) -> None:
            """Emit a direct namespace/global inline definition before ``opening``."""
            statement = public_scope_prefix(code, scope_start, opening)
            if not re.search(r"\binline\b", statement):
                return
            statement_start = opening - len(statement)
            candidates = list(re.finditer(r"([A-Za-z_][A-Za-z0-9_]*)\s*\(", statement))
            selected: tuple[str, int, int] | None = None
            for match in candidates:
                absolute_open = statement_start + statement.find("(", match.start(), match.end() + 1)
                try:
                    absolute_close = matching_delimiter(code, absolute_open, "(", ")")
                except RouteError:
                    continue
                suffix = code[absolute_close + 1 : opening]
                if re.fullmatch(r"\s*(?:(?:const|volatile|noexcept|&|&&)|(?:noexcept\s*\([^()]*\))|(?:->\s*[A-Za-z_:][A-Za-z0-9_:<>,\s*&]*))*\s*", suffix):
                    selected = (match.group(1), statement_start + match.start(1), absolute_close)
            if selected is None:
                return
            name, name_start, close_paren = selected
            qualified = "::".join(scope + (name,))
            signature_hash = sha256_text(" ".join(code[statement_start : close_paren + 1].split()))
            body = FunctionBody(source, qualified, name_start, opening, matching_delimiter(code, opening))
            collector.add(
                facade,
                "public-inline-function",
                {
                    "kind": "function",
                    "qualifiedName": qualified,
                    "scope": "::".join(scope) if scope else "global",
                    "signatureFingerprint": signature_hash,
                    "implementationKind": "body",
                    "visibility": "public",
                },
                body,
                identifier="%s:%s:%s" % (path, qualified, signature_hash),
                directions=["public-api"],
            )

        token_pattern = re.compile(r"\b(namespace|class|struct|enum|using|typedef|public|private|protected)\b")

        def scan_scope(
            start: int,
            end: int,
            scope: tuple[str, ...],
            default_visibility: str,
            class_scope: bool = False,
        ) -> None:
            visibility = default_visibility
            position = start
            while position < end:
                token = token_pattern.search(code, position, end)
                next_open = code.find("{", position, end)
                next_close = code.find("}", position, end)
                next_brace = min(value for value in (next_open, next_close, end) if value >= 0)
                if token is None or next_brace < token.start():
                    if next_brace >= end:
                        return
                    if code[next_brace] == "{":
                        if not class_scope:
                            emit_free_inline(next_brace, start, scope)
                        position = matching_delimiter(code, next_brace) + 1
                    else:
                        return
                    continue
                keyword = token.group(1)
                absolute = token.start()
                if keyword in {"public", "private", "protected"}:
                    colon = absolute + len(keyword)
                    while colon < end and code[colon].isspace():
                        colon += 1
                    if colon < end and code[colon] == ":":
                        visibility = keyword
                        position = colon + 1
                        continue
                    position = token.end()
                    continue
                if keyword == "namespace":
                    match = re.match(r"\s*([A-Za-z_][A-Za-z0-9_]*)?\s*\{", code[token.end() : end])
                    if match is None:
                        position = token.end()
                        continue
                    name = match.group(1)
                    opening = token.end() + match.end() - 1
                    closing = matching_delimiter(code, opening)
                    if name is not None:
                        scan_scope(opening + 1, closing, scope + (name,), "public")
                    position = closing + 1
                    continue
                if keyword in {"class", "struct"}:
                    prefix = public_scope_prefix(code, start, absolute)
                    angle_depth = prefix.count("<") - prefix.count(">")
                    if angle_depth > 0 or re.search(r"\bfriend\b", prefix):
                        position = token.end()
                        continue
                    body = class_body_for_public_scan(source, code, absolute, end, keyword)
                    if body is None:
                        position = token.end()
                        continue
                    name = body.symbol
                    qualified = "::".join(scope + (name,))
                    form = "definition" if body.body_start < len(code) and code[body.body_start] == "{" else "forward"
                    if visibility == "public":
                        emit_type(keyword, name, qualified, form, visibility, body, scope)
                        emit_class_methods(body, qualified, "private" if keyword == "class" else "public", visibility)
                    if form == "definition" and visibility == "public":
                        scan_scope(
                            body.body_start + 1,
                            body.body_end,
                            scope + (name,),
                            "private" if keyword == "class" else "public",
                            class_scope=True,
                        )
                        position = body.body_end + 1
                    else:
                        position = body.body_end + 1
                    continue
                if keyword == "enum":
                    match = re.match(r"\s*(class\s+)?([A-Za-z_][A-Za-z0-9_]*)", code[token.end() : end])
                    if match is None:
                        position = token.end()
                        continue
                    scoped = match.group(1) is not None
                    name = match.group(2)
                    name_end = token.end() + match.end()
                    opening = code.find("{", name_end, end)
                    semicolon = code.find(";", name_end, end)
                    paren = code.find("(", name_end, end)
                    first_terminator = min(value for value in (opening, semicolon, end) if value >= 0)
                    between = code[name_end:first_terminator]
                    if (paren >= 0 and paren < first_terminator) or between.lstrip().startswith("::"):
                        # ``enum TYPE type()`` is a method return type, not a
                        # declaration of a new public enum; likewise
                        # ``enum DRW::TTYPE value`` is a field declaration.
                        position = name_end
                        continue
                    if opening < 0 or (semicolon >= 0 and semicolon < opening):
                        position = (semicolon + 1) if semicolon >= 0 else name_end
                        continue
                    closing = matching_delimiter(code, opening)
                    if visibility == "public":
                        qualified = "::".join(scope + (name,))
                        emit_enum(
                            qualified,
                            scoped,
                            code[name_end:opening],
                            enum_public_values(source, opening, closing),
                            visibility,
                            FunctionBody(source, qualified, absolute, opening, closing),
                        )
                    position = closing + 1
                    continue
                if keyword in {"using", "typedef"}:
                    semicolon = code.find(";", token.end(), end)
                    if semicolon < 0:
                        raise RouteError("%s declaration lacks semicolon in %s" % (keyword, source.path))
                    declaration = code[token.end() : semicolon].strip()
                    if visibility == "public" and not declaration.startswith("namespace "):
                        body = public_header_body(source, keyword, absolute, semicolon)
                        alias_match = (
                            re.fullmatch(r"\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)", declaration, re.DOTALL)
                            if keyword == "using"
                            else None
                        )
                        if alias_match is not None:
                            name, target = alias_match.groups()
                            emit_alias("public-alias", "::".join(scope + (name,)), target, visibility, body)
                        elif keyword == "using":
                            emit_alias("public-using-declaration", "::".join(scope + (declaration,)), declaration, visibility, body)
                        else:
                            names = re.findall(r"[A-Za-z_][A-Za-z0-9_]*", declaration)
                            if names:
                                emit_alias("public-alias", "::".join(scope + (names[-1],)), declaration, visibility, body)
                    position = semicolon + 1
                    continue
                position = token.end()

        scan_scope(0, len(code), (), "public")


def add_legacy_dwgr_route(collector: RouteCollector, tree: SourceTree) -> None:
    """Make the target alias / standalone composition-wrapper delta explicit."""
    source = tree.require("src/libdwgr.h")
    code = mask_preprocessor_directives(source.code)
    alias = re.search(r"\busing\s+dwgR\s*=\s*dwgRW\s*;", code)
    wrapper = re.search(r"\bclass\s+dwgR\b[^{};]*\{", code)
    if tree.label == "target":
        if alias is None or wrapper is not None:
            raise RouteError("target deprecated dwgR surface is not exactly an alias")
        body = public_header_body(source, "dwgR", alias.start(), alias.end() - 1)
        selector = {
            "kind": "legacy-facade",
            "representation": "alias",
            "underlying": "dwgRW",
            "compatibilityDecision": "dwgR",
        }
    else:
        if wrapper is None or alias is not None:
            raise RouteError("standalone deprecated dwgR surface is not exactly a composition wrapper")
        opening = code.find("{", wrapper.start(), wrapper.end())
        body = FunctionBody(source, "dwgR", wrapper.start(), opening, matching_delimiter(code, opening))
        forwards = []
        for name, signature_hash, implementation_kind, method_body, _visibility in public_class_method_entries(
            source, code, body, "dwgR", "private"
        ):
            if implementation_kind == "declaration":
                continue
            if implementation_kind != "body" or "implementation." not in method_body.text:
                raise RouteError("standalone dwgR method is not a direct composition forward: %s" % name)
            forwards.append({"name": name, "signatureFingerprint": signature_hash})
        if len(forwards) != 10:
            raise RouteError("standalone dwgR wrapper must expose 10 direct forwards; found %d" % len(forwards))
        selector = {
            "kind": "legacy-facade",
            "representation": "composition-wrapper",
            "underlying": "dwgRW",
            "forwardingMethods": sorted(forwards, key=lambda item: (item["name"], item["signatureFingerprint"])),
            "compatibilityDecision": "dwgR",
        }
    collector.add(
        "dwgRW",
        "legacy-dwgr-facade",
        selector,
        body,
        identifier="dwgR",
        directions=["public-api", "compatibility"],
    )


def add_versions(collector: RouteCollector, tree: SourceTree) -> None:
    base = tree.require("src/drw_base.h")
    versions = enum_body(base, "Version")
    for name, _value, position in enum_values(versions):
        collector.add(
            "shared",
            "version",
            {"kind": "enum", "name": name},
            versions,
            identifier=name,
            directions=["read", "write"],
            line=line_number(base.text, position),
        )
    magic_pattern = re.compile(
        r'\{\s*"((?:\\.|[^"\\])*)"\s*,\s*DRW::([A-Z][A-Z0-9_]*)\s*\}'
    )
    magic_rows = list(magic_pattern.finditer(base.comment_text))
    if not magic_rows:
        raise RouteError("dwgVersionStrings has no magic/version mappings")
    for match in magic_rows:
        magic, version = match.groups()
        magic_body = FunctionBody(
            base, "DRW::dwgVersionStrings", match.start(), match.start(), match.end() - 1
        )
        collector.add(
            "shared",
            "dwg-version-magic",
            {"kind": "magic", "magic": cpp_unquote(magic), "version": version},
            magic_body,
            identifier=cpp_unquote(magic),
            directions=["read"],
            line=line_number(base.text, match.start(1)),
        )


def add_facade_methods(collector: RouteCollector, tree: SourceTree) -> None:
    """Emit the catch-all public façade definition surface.

    Specialized route families remain the useful implementation checklist; the
    catch-all prevents a newly-added public entrypoint from being invisible
    merely because no specialized extractor recognizes it yet.
    """
    for facade, path in (("dxfRW", "src/libdxfrw.cpp"), ("dwgRW", "src/libdwgr.cpp")):
        source = tree.require(path)
        ordinals: dict[str, int] = {}
        for body in qualified_definitions(source, facade + "::"):
            signature = signature_fingerprint(body)
            name = body.symbol.rsplit("::", 1)[1]
            ordinals[name] = ordinals.get(name, 0) + 1
            collector.add(
                facade,
                "facade-method",
                {"kind": "definition", "name": body.symbol, "signatureFingerprint": signature},
                body,
                identifier="%s-%03d" % (name, ordinals[name]),
                directions=["facade"],
            )


def add_dxf_routes(collector: RouteCollector, tree: SourceTree) -> None:
    source = tree.require("src/libdxfrw.cpp")
    for symbol, category, names, grammar, directions in (
        ("dxfRW::processDxf", "dxf-section", ("sectionname",), (), ["read"]),
        ("dxfRW::processTables", "dxf-table", ("sectionstr",), ("ENDSEC",), ["read"]),
        ("dxfRW::processEntities", "dxf-entity", ("nextentity",), (), ["read", "publish"]),
        ("dxfRW::processObjects", "dxf-object", ("normalizedEntity",), (), ["read", "publish"]),
    ):
        body = function_body(source, symbol)
        branches = dispatch_branches(body, names=names, grammar_literals=grammar)
        for branch in branches:
            branch_identifier = "%03d-%s" % (branch["ordinal"], branch["target"])
            branch_route_id = "%s/%s-dispatch-branch/%s" % (
                "dxfRW",
                category,
                normalized_identifier(branch_identifier),
            )
            collector.add(
                "dxfRW",
                category + "-dispatch-branch",
                {
                    "kind": branch["matchKind"],
                    "ordinal": branch["ordinal"],
                    "target": branch["target"],
                    "conditionFingerprint": branch["conditionFingerprint"],
                    "openEnded": branch["openEnded"],
                },
                body,
                identifier=branch_identifier,
                directions=directions,
                line=branch["line"],
            )
            for value in branch["literals"]:
                collector.add(
                    "dxfRW",
                    category,
                    {
                        "kind": "exact",
                        "canonical": value,
                        "aliases": [value],
                        "branch": branch_route_id,
                        "target": branch["target"],
                    },
                    body,
                    identifier=value,
                    directions=directions,
                    line=branch["line"],
                )
            if branch["openEnded"]:
                collector.add(
                    "dxfRW",
                    category + "-predicate",
                    {
                        "kind": "open-ended",
                        "branch": branch_route_id,
                        "target": branch["target"],
                        "conditionFingerprint": branch["conditionFingerprint"],
                    },
                    body,
                    identifier=branch_identifier,
                    directions=directions,
                    line=branch["line"],
                )

    blocks = function_body(source, "dxfRW::processBlocks")
    block_literals = branch_literals(blocks.comment_text, ("nextentity",))
    if block_literals != ["BLOCK"]:
        raise RouteError("BLOCKS grammar anchor has unexpected BLOCK selectors")
    collector.add(
        "dxfRW",
        "dxf-block",
        {"kind": "exact", "canonical": "BLOCK", "aliases": ["BLOCK"], "target": "processBlock"},
        blocks,
        identifier="BLOCK",
        directions=["read", "publish"],
    )

    classes = function_body(source, "dxfRW::dxfClassForRecordName")
    class_entries = dxf_class_entries(classes)
    if not class_entries:
        raise RouteError("DXF class table has no source names")
    for selector, line in class_entries:
        name = selector["recordName"]
        collector.add(
            "dxfRW",
            "dxf-class",
            selector,
            classes,
            identifier=name,
            directions=["read", "write"],
            line=line,
        )

    class_parser = function_body(source, "dxfRW::processClasses")
    class_switch = switch_body(class_parser, "code")
    class_groups = switch_cases(class_switch)
    expected_class_groups = {"1", "2", "3", "90", "91", "280", "281"}
    if {value for value, _line in class_groups} != expected_class_groups:
        raise RouteError("DXF CLASS required-group switch differs from the pinned contract")
    for value, line in class_groups:
        group = int(value)
        collector.add(
            "dxfRW",
            "dxf-class-required-group",
            {
                "kind": "group-code",
                "code": group,
                "requirement": "version-conditional" if group == 91 else "required",
            },
            class_parser,
            identifier=value,
            directions=["read"],
            line=line,
        )

    raw_symbols = (
        "dxfRW::processRawDxfSection",
        "dxfRW::processRawEntity",
        "dxfRW::processRawObject",
        "dxfRW::processProxyEntity",
        "dxfRW::processProxyObject",
        "dxfRW::writeRawDxfObject",
        "dxfRW::writeRawDxfSection",
    )
    for symbol in raw_symbols:
        body = function_body(source, symbol)
        collector.add(
            "dxfRW",
            "raw-route",
            {"kind": "anchor", "symbol": symbol},
            body,
            identifier=symbol.rsplit("::", 1)[1],
            directions=["write"] if symbol.startswith("dxfRW::write") else ["read", "publish"],
        )

    writer_ordinals: dict[str, int] = {}
    for body in qualified_definitions(source, "dxfRW::write"):
        signature = signature_fingerprint(body)
        name = body.symbol.rsplit("::", 1)[1]
        writer_ordinals[name] = writer_ordinals.get(name, 0) + 1
        collector.add(
            "dxfRW",
            "writer-entrypoint",
            {"kind": "symbol", "name": body.symbol, "signatureFingerprint": signature},
            body,
            identifier="%s-%03d" % (name, writer_ordinals[name]),
            directions=["write"],
        )

    version_body = function_body(source, "isSupportedDxfWriteVersion")
    version_switch = switch_body(version_body, "version")
    for selector, outcome, line in switch_case_outcomes(version_switch):
        if not selector.startswith("DRW::") or outcome != "true":
            raise RouteError("cannot classify DXF writer version outcome for %s" % selector)
        version = selector.split("::", 1)[1]
        collector.add(
            "dxfRW",
            "writer-version",
            {"kind": "version-writer", "version": version, "outcome": "supported"},
            version_body,
            identifier=version,
            directions=["write"],
            line=line,
        )

    code_source = tree.files.get("src/intern/dxfcode.h")
    if code_source is None:
        code_source = tree.require("src/intern/dxfreader.cpp")
    codes = named_array_body(code_source, "kDxfCodeRanges")
    ranges = re.finditer(
        r"\{\s*(-?\d+)\s*,\s*(-?\d+)\s*,\s*DxfValueKind::(\w+)\s*\}",
        codes.code,
    )
    count = 0
    for match in ranges:
        lower, upper, value_kind = match.groups()
        collector.add(
            "dxfRW",
            "group-code-reader-range",
            {"kind": "range", "lower": int(lower), "upper": int(upper), "valueKind": value_kind},
            codes,
            identifier="%s-%s" % (lower, upper),
            directions=["read", "write", "raw-capture"],
            line=line_number(codes.source.text, codes.body_start + match.start()),
        )
        count += 1
    if count == 0:
        raise RouteError("DXF group-code range table is empty")

    raw_classifier = function_body(source, "classifyDxfCode")
    collector.add(
        "dxfRW",
        "group-code-raw-classifier",
        {"kind": "anchor", "symbol": raw_classifier.symbol},
        raw_classifier,
        identifier=raw_classifier.symbol,
        directions=["raw-capture", "write"],
    )
    for selector, line, identifier in raw_classifier_rules(raw_classifier):
        collector.add(
            "dxfRW",
            "group-code-raw-rule",
            selector,
            raw_classifier,
            identifier=identifier,
            directions=["raw-capture", "write"],
            line=line,
        )


def add_dwg_routes(collector: RouteCollector, tree: SourceTree) -> None:
    util = tree.require("src/intern/dwgutil.h")
    entity_enum = namespace_enum_body(util, "dwgType", "Entity")
    object_enum = namespace_enum_body(util, "dwgObjType", "Object")
    entities = {name: value for name, value, _position in enum_values(entity_enum)}
    objects = {name: value for name, value, _position in enum_values(object_enum)}
    source = tree.require("src/intern/dwgreader.cpp")
    mappings = {"dwgType": entities, "dwgObjType": objects}
    for symbol, category in (
        ("dwgReader::readDwgEntityWithOutput", "fixed-entity"),
        ("dwgReader::readDwgObject", "fixed-object"),
    ):
        body = function_body(source, symbol)
        seen = 0
        dispatch = switch_body(body, "oType")
        for token, line in switch_cases(dispatch):
            if "::" not in token:
                continue
            namespace, name = token.split("::", 1)
            if namespace not in {"dwgType", "dwgObjType"}:
                continue
            mapping = mappings[namespace]
            if name not in mapping:
                raise RouteError("unresolved DWG enum selector %s in %s" % (token, symbol))
            route_selector = {"kind": "numeric", "value": mapping[name], "enumSymbol": token}
            # DBCOLOR appears in the entity switch, but the fixed-object gate
            # preceding it defers the frame before that switch is reachable.
            if category == "fixed-entity" and token == "dwgObjType::DBCOLOR":
                route_selector.update(
                    {
                        "preDispatchDisposition": "deferred-object-pass",
                        "objectPassRoute": "dwgRW/fixed-object/1004-DBCOLOR",
                    }
                )
            if category == "fixed-entity":
                compound_evidence = dwg_compound_transition_metadata(
                    tree, token
                )
                if compound_evidence:
                    route_selector["compoundTransitionEvidence"] = compound_evidence
                    if token == "dwgType::INSERT":
                        route_selector["compoundDeliveryEvidence"] = dwg_compound_delivery_metadata(
                            tree
                        )
            collector.add(
                "dwgRW",
                category,
                route_selector,
                body,
                identifier="%d-%s" % (mapping[name], name),
                directions=["read", "publish"],
                line=line,
            )
            seen += 1
        if seen == 0:
            raise RouteError("DWG dispatch anchor has no fixed selectors: %s" % symbol)
        named_category = "named-entity-class" if category == "fixed-entity" else "named-object-class"
        named_dispatches = named_class_dispatches(body)
        if not named_dispatches:
            raise RouteError("DWG custom-class dispatch is empty: %s" % symbol)
        dispatches_by_name: dict[str, list[dict]] = {}
        for route in named_dispatches:
            dispatches_by_name.setdefault(route["name"], []).append(route)
        for name, routes in sorted(dispatches_by_name.items()):
            selector = {
                "kind": "exact",
                "canonical": name,
                "aliases": [name],
                "callbacks": sorted({callback for route in routes for callback in route["callbacks"]}),
                "models": sorted({model for route in routes for model in route["models"]}),
                "conditionFingerprints": sorted(
                    {route["conditionFingerprint"] for route in routes}
                ),
                # Keep line/fingerprint pairs, not two independent sets.  A
                # parser-publication row must later prove that it retained the
                # selector actually associated with this named route.
                "conditionSelectors": sorted(
                    {
                        (route["line"], route["conditionFingerprint"])
                        for route in routes
                    }
                ),
            }
            selector["conditionSelectors"] = [
                {"line": line, "conditionFingerprint": fingerprint}
                for line, fingerprint in selector["conditionSelectors"]
            ]
            for route in routes:
                collector.add(
                    "dwgRW",
                    named_category,
                    selector,
                    body,
                    identifier=name,
                    directions=["read", "publish"],
                    line=route["line"],
                )
        for expression, line in predicates(body, ("recName", "className", "rn", "cn")):
            digest = sha256_text(expression)[:16]
            collector.add(
                "dwgRW",
                "named-class-predicate",
                {
                    "kind": "open-ended",
                    "condition": expression,
                    "conditionFingerprint": sha256_text(expression),
                },
                body,
                identifier=digest,
                directions=["read", "publish"],
                line=line,
            )

    table_pattern = re.compile(
        r"\bconstexpr\s+DwgTableDescriptor\s+(k[A-Za-z0-9_]+)\s*\{\s*"
        r"(DRW::[A-Za-z0-9_]+)\s*,\s*(DRW::[A-Za-z0-9_]+)\s*,\s*"
        r'"((?:\\.|[^"\\])*)"\s*\}'
    )
    table_descriptors = list(table_pattern.finditer(source.comment_text))
    if not table_descriptors:
        raise RouteError("DWG table descriptor inventory is empty")
    for match in table_descriptors:
        name, control_type, record_type, receipt_name = match.groups()
        delivery_evidence = dwg_table_delivery_metadata(tree, name)
        selector = {
            "kind": "table",
            "name": name,
            "controlType": control_type,
            "recordType": record_type,
            "controlReceiptName": cpp_unquote(receipt_name),
            "deliveryEvidence": delivery_evidence,
        }
        if name == "kBlockTable":
            selector["blockOwnershipEvidence"] = dwg_block_ownership_metadata(tree)
        collector.add(
            "dwgRW",
            "table-descriptor",
            selector,
            FunctionBody(source, name, match.start(), match.start(), match.end() - 1),
            identifier=name,
            directions=["read", "publish"],
            line=line_number(source.text, match.start()),
        )

    raw_header = tree.require("src/drw_objects.h")
    raw_constants = constexpr_integer_values(raw_header)
    for symbol, category in (
        ("fixedEntityShellName", "raw-entity-shell"),
        ("fixedObjectShellName", "raw-object-shell"),
    ):
        shell_function = function_body(raw_header, symbol)
        for token, shell_name, line in switch_string_returns(shell_function, "type"):
            if token.isdigit():
                value = int(token)
            elif token in raw_constants:
                value = raw_constants[token]
            else:
                raise RouteError("raw shell selector lacks an integer value: %s" % token)
            collector.add(
                "dwgRW",
                category,
                {"kind": "fixed-shell", "type": value, "name": shell_name},
                shell_function,
                identifier="%d-%s" % (value, shell_name),
                directions=["read", "publish", "raw-capture"],
                line=line,
            )
    raw_custom = function_body(source, "isValidatedRawCustomObjectShell")
    custom_shells = sorted(set(value for value, _line in all_string_literals(raw_custom)))
    if not custom_shells:
        raise RouteError("raw custom shell selector table is empty")
    for shell_name in custom_shells:
        collector.add(
            "dwgRW",
            "raw-custom-shell",
            {"kind": "named-shell", "name": shell_name},
            raw_custom,
            identifier=shell_name,
            directions=["read", "publish", "raw-capture"],
        )
    context_helper = function_body(source, "objectContextKindFromClassNames")
    context_pattern = re.compile(
        r"\bif\s*\(\s*matches\s*\((.*?)\)\s*\)\s*\{\s*"
        r"kind\s*=\s*DRW_ObjectContextData::Kind::([A-Za-z0-9_]+)\s*;",
        re.DOTALL,
    )
    context_rows = list(context_pattern.finditer(context_helper.comment_text))
    if not context_rows:
        raise RouteError("object-context class mapping is empty")
    for match in context_rows:
        names = string_literals(match.group(1))
        context_kind = match.group(2)
        if not names:
            raise RouteError("object-context mapping lacks class spellings")
        for name in names:
            collector.add(
                "dwgRW",
                "object-context-class",
                {
                    "kind": "exact",
                    "canonical": name,
                    "aliases": [name],
                    "contextKind": context_kind,
                },
                context_helper,
                identifier=name,
                directions=["read", "publish"],
                line=line_number(
                    context_helper.source.text, context_helper.body_start + match.start(1)
                ),
            )

    reader_stages = (
        "dwgReader::readDwgTables",
        "dwgReader::readDwgBlocks",
        "dwgReader::readDwgEntities",
        "dwgReader::readDwgObjects",
    )
    for symbol in reader_stages:
        body = function_body(source, symbol)
        selector = {"kind": "anchor", "symbol": symbol}
        if symbol == "dwgReader::readDwgTables":
            selector["readerLifecycleEvidence"] = dwg_reader_lifecycle_metadata(tree)
        collector.add(
            "dwgRW",
            "reader-stage",
            selector,
            body,
            identifier=symbol.rsplit("::", 1)[1],
            directions=["read", "publish"],
        )

    dwg = tree.require("src/libdwgr.cpp")
    factory = function_body(dwg, "dwgRW::createReaderForVersion")
    writer = function_body(dwg, "dwgRW::write")
    reader_switch = switch_body(factory, "version")
    for selector, outcome, line in switch_case_outcomes(reader_switch):
        if not selector.startswith("DRW::"):
            raise RouteError("unexpected DWG reader version selector: %s" % selector)
        version = selector.split("::", 1)[1]
        reader_match = re.search(r"\bnew\s+(dwgReader[A-Za-z0-9_]*)\s*\(", outcome)
        if reader_match is not None:
            route_selector = {
                "kind": "version-reader",
                "version": version,
                "reader": reader_match.group(1),
                "outcome": "supported",
            }
        elif outcome == "break" or "nullptr" in outcome:
            route_selector = {
                "kind": "version-reader",
                "version": version,
                "reader": None,
                "outcome": "unsupported",
            }
        else:
            raise RouteError("cannot classify DWG reader outcome for %s" % selector)
        collector.add(
            "dwgRW",
            "reader-version",
            route_selector,
            factory,
            identifier=version,
            directions=["read"],
            line=line,
        )

    allowed_versions = sorted(set(re.findall(r"\bver\s*!=\s*DRW::(AC\d+)\b", writer.code)))
    writer_assignments = list(
        re.finditer(
            r"\b(?:if|else\s+if)\s*\(\s*ver\s*==\s*DRW::(AC\d+)\s*\)\s*"
            r"writer\s*=\s*std::make_unique\s*<\s*(dwgWriter[A-Za-z0-9_]*)\s*>",
            writer.code,
        )
    )
    if not allowed_versions or not writer_assignments:
        raise RouteError("DWG writer version anchor has no version/writer bindings")
    writer_bindings = {
        match.group(1): (match.group(2), line_number(dwg.text, writer.body_start + match.start(1)))
        for match in writer_assignments
    }
    fallback = re.search(
        r"\belse\s+writer\s*=\s*std::make_unique\s*<\s*(dwgWriter[A-Za-z0-9_]*)\s*>",
        writer.code,
    )
    remaining_versions = sorted(set(allowed_versions) - set(writer_bindings))
    if fallback is None or len(remaining_versions) != 1:
        raise RouteError("DWG writer version fallback is ambiguous")
    writer_bindings[remaining_versions[0]] = (
        fallback.group(1),
        line_number(dwg.text, writer.body_start + fallback.start(1)),
    )
    if set(writer_bindings) != set(allowed_versions):
        raise RouteError("DWG writer bindings do not cover exactly the allowed versions")
    for version in sorted(writer_bindings):
        writer_class, line = writer_bindings[version]
        collector.add(
            "dwgRW",
            "writer-version",
            {
                "kind": "version-writer",
                "version": version,
                "writer": writer_class,
                "outcome": "supported",
            },
            writer,
            identifier=version,
            directions=["write"],
            line=line,
        )

    writer_ordinals: dict[str, int] = {}
    for body in qualified_definitions(dwg, "dwgRW::write"):
        signature = signature_fingerprint(body)
        name = body.symbol.rsplit("::", 1)[1]
        writer_ordinals[name] = writer_ordinals.get(name, 0) + 1
        collector.add(
            "dwgRW",
            "writer-entrypoint",
            {"kind": "symbol", "name": body.symbol, "signatureFingerprint": signature},
            body,
            identifier="%s-%03d" % (name, writer_ordinals[name]),
            directions=["write"],
        )

    capabilities = named_array_body(dwg, "kDwgDataStorageWriterCapabilities")
    capability_entries = direct_initializer_entries(capabilities)
    for entry in capability_entries:
        binding_match = re.search(r"\bBinding::([A-Za-z0-9_]+)\b", entry.code)
        operation_match = re.search(r"\bOperation::([A-Za-z0-9_]+)\b", entry.code)
        phase_match = re.search(r"\bDwgDataStorageWriterPhase::([A-Za-z0-9_]+)\b", entry.code)
        versions = re.findall(r"\bDRW::([A-Z][A-Z0-9_]*)\b", entry.code)
        strings = string_literals(entry.comment_text)
        class_match = re.search(r"\b(DRW_[A-Za-z0-9_]+::kDwgClassNum)\b", entry.code)
        fixed_match = re.search(r"\b(dwgObjType::[A-Za-z0-9_]+)\b", entry.code)
        bools = re.findall(r"\b(true|false)\b", entry.code)
        if (
            binding_match is None
            or operation_match is None
            or phase_match is None
            or len(strings) != 3
            or len(versions) != 2
            or len(bools) < 2
        ):
            raise RouteError("malformed DWG DataStorage capability entry")
        binding = binding_match.group(1)
        selector = {
            "kind": "binding",
            "binding": binding,
            "operation": operation_match.group(1),
            "family": strings[0],
            "className": strings[1],
            "recordName": strings[2],
            "classNumber": class_match.group(1) if class_match is not None else None,
            "fixedObjectType": fixed_match.group(1) if fixed_match is not None else None,
            "minimumVersion": versions[0],
            "maximumVersion": versions[1],
            "phase": phase_match.group(1),
            "requiresClass": bools[-2] == "true",
            "supported": bools[-1] == "true",
        }
        collector.add(
            "dwgRW",
            "writer-binding",
            selector,
            entry,
            identifier=binding,
            directions=["write"],
        )

    section_declarations = namespace_enum_body(util, "secEnum", "DWGSection")
    for name, value, position in enum_values(section_declarations):
        collector.add(
            "dwgRW",
            "section-declaration",
            {"kind": "enum", "name": name, "value": value},
            section_declarations,
            identifier=name,
            directions=["read", "raw-capture"],
            line=line_number(util.text, position),
        )
    section_enum = function_body(tree.require("src/intern/dwgutil.cpp"), "secEnum::getEnum")
    section_pattern = re.compile(
        r'\b(?:if|else\s+if)\s*\(\s*nameSec\s*==\s*"((?:\\.|[^"\\])*)"\s*\)\s*\{?\s*'
        r"return\s+([A-Z][A-Z0-9_]*)\s*;"
    )
    section_rows = list(section_pattern.finditer(section_enum.comment_text))
    if not section_rows:
        raise RouteError("DWG section enum anchor has no named mappings")
    for match in section_rows:
        section, enum_name = match.groups()
        collector.add(
            "dwgRW",
            "section",
            {
                "kind": "exact",
                "canonical": cpp_unquote(section),
                "aliases": [cpp_unquote(section)],
                "section": enum_name,
            },
            section_enum,
            identifier=cpp_unquote(section),
            directions=["read", "raw-capture"],
            line=line_number(section_enum.source.text, section_enum.body_start + match.start(1)),
        )
    if not re.search(r"\breturn\s+UNKNOWNS\s*;", section_enum.code):
        raise RouteError("DWG section enum anchor lacks an unknown fallback")
    collector.add(
        "dwgRW",
        "section-fallback",
        {"kind": "fallback", "section": "UNKNOWNS"},
        section_enum,
        identifier="UNKNOWNS",
        directions=["read", "raw-capture"],
    )

    process = function_body(dwg, "dwgRW::processDwg")
    collector.add(
        "dwgRW",
        "raw-route",
        {"kind": "anchor", "symbol": process.symbol},
        process,
        identifier=process.symbol.rsplit("::", 1)[1],
        directions=["read", "publish"],
    )
    raw_count = 0
    for reader in ("18", "21"):
        file = tree.require("src/intern/dwgreader%s.cpp" % reader)
        body = function_body(file, "dwgReader%s::captureRawDwgDataSections" % reader)
        collector.add(
            "dwgRW",
            "raw-route",
            {"kind": "anchor", "symbol": body.symbol},
            body,
            identifier=body.symbol.replace("::", "-"),
            directions=["read", "publish"],
        )
        raw_count += 1
    if raw_count != 2:
        raise RouteError("expected R2004 and R2007 raw-section capture anchors; found %d" % raw_count)


def add_shared_routes(
    collector: RouteCollector, tree: SourceTree, public_headers: tuple[str, ...]
) -> None:
    # ``public-model`` remains a deliberately indexed DRW_* subset used by
    # later model/pipeline work.  The complete installed declaration surface is
    # emitted separately by ``add_public_header_routes``.
    add_public_header_routes(collector, tree, public_headers)
    add_legacy_dwgr_route(collector, tree)
    interface = tree.require("src/drw_interface.h")
    methods = list(
        re.finditer(
            r"\bvirtual\b[^;{}()]*?\b([A-Za-z_][A-Za-z0-9_]*)\s*\(",
            interface.code,
            re.DOTALL,
        )
    )
    if not methods:
        raise RouteError("DRW_Interface contains no virtual methods")
    callbacks = 0
    method_ordinals: dict[str, int] = {}
    for match in methods:
        method = match.group(1)
        method_ordinals[method] = method_ordinals.get(method, 0) + 1
        declaration_end, has_inline_body = virtual_method_signature_end(interface, match.end() - 1)
        signature_end = declaration_end - 1 if has_inline_body else declaration_end
        signature = " ".join(interface.code[match.start() : signature_end].split())
        signature_hash = sha256_text(signature)
        evidence_body = FunctionBody(
            interface, "DRW_Interface", match.start(1), match.start(1), match.end(1) - 1
        )
        collector.add(
            "shared",
            "interface-method",
            {"kind": "virtual", "name": method, "signatureFingerprint": signature_hash},
            evidence_body,
            identifier="%s-%03d" % (method, method_ordinals[method]),
            directions=["publish"],
            line=line_number(interface.text, match.start(1)),
        )
        callback_kind = "data-link" if method in {"linkImage", "linkUnderlay"} else "data-add"
        if not method.startswith("add") and callback_kind != "data-link":
            continue
        callbacks += 1
        collector.add(
            "shared",
            "callback",
            {
                "kind": "symbol",
                "name": method,
                "callbackKind": callback_kind,
                "signatureFingerprint": signature_hash,
                **callback_parameter_contract(signature),
            },
            evidence_body,
            identifier="%s-%03d" % (method, method_ordinals[method]),
            directions=["publish"],
            line=line_number(interface.text, match.start(1)),
        )
    if callbacks == 0:
        raise RouteError("DRW_Interface contains no virtual add callbacks")

    model_headers = sorted(
        path
        for path in tree.files
        if path.startswith("src/drw_") and path.endswith(".h")
    ) + ["src/libdwgr.h", "src/libdxfrw.h"]
    model_count = 0
    for path in model_headers:
        source = tree.require(path)
        for match in re.finditer(r"\b(?:class|struct)\s+(DRW_[A-Za-z0-9_]+)\b", source.code):
            model = match.group(1)
            collector.add(
                "shared",
                "public-model",
                {"kind": "symbol", "name": model},
                FunctionBody(source, model, match.start(1), match.start(1), match.end(1) - 1),
                identifier=model,
                directions=["read", "write", "publish"],
                line=line_number(source.text, match.start(1)),
            )
            model_count += 1
    if model_count == 0:
        raise RouteError("public model scan produced no DRW_ types")


def inventory_tree(tree: SourceTree, public_headers: tuple[str, ...]) -> dict[str, list[dict]]:
    collector = RouteCollector()
    add_source_units(collector, tree)
    add_versions(collector, tree)
    add_facade_methods(collector, tree)
    add_dxf_routes(collector, tree)
    add_dwg_routes(collector, tree)
    add_shared_routes(collector, tree, public_headers)
    # I0.2b deliberately runs after the anchor and public-surface routes above:
    # pipeline rows link to those already-established source identities rather
    # than inventing a second, incompatible feature inventory.
    add_pipeline_unit_routes(collector, tree)
    add_model_codec_routes(collector, tree)
    add_dxf_transport_routes(collector, tree)
    add_dwg_version_pipeline_routes(collector, tree)
    add_parser_publication_routes(collector, tree)
    add_publication_stage_routes(collector, tree)
    add_raw_flow_routes(collector, tree)
    add_helper_subsystem_routes(collector, tree)
    all_routes = collector.routes()
    return {
        facade: [route for route in all_routes if route["facade"] == facade]
        for facade in ("dxfRW", "dwgRW", "shared")
    }


def inventory_category_routes(
    inventory: dict[str, list[dict]], facade: str, category: str
) -> list[dict]:
    return [route for route in inventory[facade] if route["category"] == category]


def validate_pipeline_closure(tree: SourceTree, inventory: dict[str, list[dict]]) -> None:
    """Fail closed on every I0.2b source-only pipeline relationship."""
    roles = source_unit_roles_for(tree)
    functional = {
        path: role for path, role in roles.items() if role not in SUPPORTING_SOURCE_UNIT_ROLES
    }
    pipeline_units = inventory_category_routes(inventory, "shared", "pipeline-unit")
    by_path = {route["selector"].get("path"): route for route in pipeline_units}
    if len(pipeline_units) != len(by_path) or set(by_path) != set(functional):
        raise RouteError("%s functional source-unit pipeline closure is incomplete" % tree.label)
    for path, role in functional.items():
        selector = by_path[path]["selector"]
        if selector.get("role") != role or selector.get("stage") != PIPELINE_STAGE_BY_SOURCE_ROLE[role]:
            raise RouteError("source-unit pipeline placement changed without review: %s" % path)
        expected_source = "shared/source-unit/" + normalized_identifier(path)
        if selector.get("sourceUnit") != expected_source:
            raise RouteError("pipeline unit lacks its source-unit edge: %s" % path)

    helper_routes = inventory_category_routes(inventory, "shared", "helper-subsystem")
    helpers = {route["selector"].get("name"): route for route in helper_routes}
    if len(helper_routes) != len(helpers) or set(helpers) != set(HELPER_SUBSYSTEM_PATHS):
        raise RouteError("helper subsystem closure changed without review")
    for name, paths in HELPER_SUBSYSTEM_PATHS.items():
        selector = helpers[name]["selector"]
        if selector.get("paths") != list(paths):
            raise RouteError("helper subsystem path contract changed: %s" % name)
        evidence_paths = {item["path"] for item in helpers[name]["evidence"]}
        if evidence_paths != set(paths):
            raise RouteError("helper subsystem evidence is incomplete: %s" % name)

    public_model_rows = inventory_category_routes(inventory, "shared", "public-model")
    public_models = {route["selector"].get("name") for route in public_model_rows}
    model_route_ids = {
        route["selector"].get("name"): route["id"] for route in public_model_rows
    }
    callback_rows = inventory_category_routes(inventory, "shared", "callback")
    callback_route_ids: dict[str, str] = {}
    for callback_route in callback_rows:
        callback_name = callback_route["selector"].get("name")
        if not isinstance(callback_name, str) or callback_name in callback_route_ids:
            raise RouteError("callback route registry is ambiguous during raw closure")
        callback_route_ids[callback_name] = callback_route["id"]
    model_routes = inventory_category_routes(inventory, "shared", "model-codec")
    codecs = {route["selector"].get("model"): route for route in model_routes}
    if len(model_routes) != len(codecs) or set(codecs) != public_models:
        raise RouteError("model codec closure does not cover every public model")
    known_operations = {"dxf-parse", "dwg-parse", "parse-helper", "dwg-encode", "dxf-write"}
    for model, route in codecs.items():
        selector = route["selector"]
        operations = selector.get("operations")
        status = selector.get("directDefinitionStatus")
        expected_declaration = "shared/public-model/" + normalized_identifier(model)
        if selector.get("declarationRoute") != expected_declaration:
            raise RouteError("model codec lacks its public declaration edge: %s" % model)
        if not isinstance(operations, list) or status not in {"present", "no-direct-definition"}:
            raise RouteError("model codec route is malformed: %s" % route["id"])
        if (bool(operations)) != (status == "present"):
            raise RouteError("model codec direct-definition status disagrees: %s" % model)
        operation_keys = []
        evidence_symbols = {item["symbol"] for item in route["evidence"]}
        for operation in operations:
            if not isinstance(operation, dict) or operation.get("operation") not in known_operations:
                raise RouteError("model codec operation is malformed: %s" % model)
            symbol = operation.get("symbol")
            signature = operation.get("signatureFingerprint")
            if not isinstance(symbol, str) or not isinstance(signature, str) or symbol not in evidence_symbols:
                raise RouteError("model codec operation lacks owned evidence: %s" % model)
            operation_keys.append((operation["operation"], symbol, signature))
        if operation_keys != sorted(set(operation_keys)):
            raise RouteError("model codec operations are non-deterministic: %s" % model)

    internal_model_routes = inventory_category_routes(inventory, "shared", "internal-model-codec")
    internal_models = {route["selector"].get("model"): route for route in internal_model_routes}
    if len(internal_model_routes) != len(internal_models) or set(internal_models) != INTERNAL_MODEL_CODEC_TYPES:
        raise RouteError("internal model codec closure changed without review")
    for model, route in internal_models.items():
        selector = route["selector"]
        if selector.get("scope") != "internal" or not selector.get("operations"):
            raise RouteError("internal model codec route is malformed: %s" % model)

    transport_routes = inventory_category_routes(inventory, "dxfRW", "dxf-transport-node")
    type_routes = {
        route["selector"].get("transport"): route
        for route in transport_routes
        if route["selector"].get("kind") == "transport-type"
    }
    expected_types = {entry[0]: entry[1:] for entry in DXF_TRANSPORT_TYPES}
    if len(type_routes) != len(expected_types) or set(type_routes) != set(expected_types):
        raise RouteError("DXF transport type closure changed without review")
    for transport, (path, symbol) in expected_types.items():
        selector = type_routes[transport]["selector"]
        if selector.get("sourcePath") != path or selector.get("symbol") != symbol:
            raise RouteError("DXF transport type route is malformed: %s" % transport)
    selection_routes = {
        route["selector"].get("selection"): route
        for route in transport_routes
        if route["selector"].get("kind") == "transport-selection"
    }
    expected_selections = {
        selection: (symbol, implementation)
        for selection, symbol, implementation, _directions in DXF_TRANSPORT_SELECTIONS
    }
    if len(selection_routes) != len(expected_selections) or set(selection_routes) != set(expected_selections):
        raise RouteError("DXF transport selection closure changed without review")
    for selection, (entrypoint, implementation) in expected_selections.items():
        selector = selection_routes[selection]["selector"]
        if selector.get("entrypoint") != entrypoint or selector.get("implementation") != implementation:
            raise RouteError("DXF transport selection is malformed: %s" % selection)
        validate_transport_selection_metadata(tree, selection_routes[selection])
    standalone_range_routes = [
        route
        for route in transport_routes
        if route["selector"].get("kind") == "group-code-range-source"
    ]
    if tree.label == "standalone":
        if len(standalone_range_routes) != 1 or standalone_range_routes[0]["selector"].get("sourcePath") != "src/intern/dxfcode.h":
            raise RouteError("standalone DXF group-code source edge is missing")
    elif standalone_range_routes:
        raise RouteError("target inventory unexpectedly has a standalone group-code source edge")

    for version_category, pipeline_category, implementation_key, family in (
        ("reader-version", "reader-pipeline", "reader", DWG_READER_PIPELINES),
        ("writer-version", "writer-pipeline", "writer", DWG_WRITER_PIPELINES),
    ):
        source_routes = [
            route
            for route in inventory_category_routes(inventory, "dwgRW", version_category)
            if route["selector"].get("outcome") == "supported"
        ]
        expected_factory_routes = {route["id"] for route in source_routes}
        pipelines = inventory_category_routes(inventory, "dwgRW", pipeline_category)
        actual_by_implementation = {
            route["selector"].get(implementation_key): route for route in pipelines
        }
        expected_by_implementation = {
            definition[implementation_key]: definition for definition in family.values()
        }
        if len(pipelines) != len(actual_by_implementation) or set(actual_by_implementation) != set(expected_by_implementation):
            raise RouteError("%s implementation closure changed without review" % pipeline_category)
        observed_factory_route_entries: list[str] = []
        for implementation, definition in expected_by_implementation.items():
            selector = actual_by_implementation[implementation]["selector"]
            if selector.get("parent") != definition["parent"] or selector.get("sourcePaths") != list(definition["paths"]):
                raise RouteError("%s pipeline inheritance/source path changed: %s" % (pipeline_category, implementation))
            route_ids = selector.get("factoryRoutes")
            versions = selector.get("factoryVersions")
            if not isinstance(route_ids, list) or not isinstance(versions, list):
                raise RouteError("%s pipeline factory links are malformed: %s" % (pipeline_category, implementation))
            if route_ids != sorted(set(route_ids)):
                raise RouteError("%s pipeline factory links are non-deterministic: %s" % (pipeline_category, implementation))
            observed_factory_route_entries.extend(route_ids)
            expected_versions = sorted(
                route["selector"].get("version")
                for route in source_routes
                if route["selector"].get(implementation_key) == implementation
            )
            if versions != expected_versions:
                raise RouteError("%s pipeline versions disagree: %s" % (pipeline_category, implementation))
        if (
            len(observed_factory_route_entries) != len(set(observed_factory_route_entries))
            or set(observed_factory_route_entries) != expected_factory_routes
        ):
            raise RouteError("%s does not close every supported version route" % pipeline_category)

    table_routes = inventory_category_routes(inventory, "dwgRW", "table-descriptor")
    table_by_name = {route["selector"].get("name"): route for route in table_routes}
    if len(table_routes) != len(table_by_name) or set(table_by_name) != set(DWG_TABLE_DELIVERY_FIELDS):
        raise RouteError("DWG table descriptor closure changed without review")
    for descriptor, route in table_by_name.items():
        selector = route["selector"]
        expected_evidence = dwg_table_delivery_metadata(tree, descriptor)
        if selector.get("deliveryEvidence") != expected_evidence:
            raise RouteError("DWG table delivery evidence changed: %s" % descriptor)
        expected_ownership = (
            dwg_block_ownership_metadata(tree) if descriptor == "kBlockTable" else []
        )
        if selector.get("blockOwnershipEvidence", []) != expected_ownership:
            raise RouteError("DWG BLOCK/ENDBLK ownership evidence changed: %s" % descriptor)

    reader_stage_routes = inventory_category_routes(inventory, "dwgRW", "reader-stage")
    reader_stages = {route["selector"].get("symbol"): route for route in reader_stage_routes}
    expected_reader_stages = {
        "dwgReader::readDwgTables",
        "dwgReader::readDwgBlocks",
        "dwgReader::readDwgEntities",
        "dwgReader::readDwgObjects",
    }
    if len(reader_stage_routes) != len(reader_stages) or set(reader_stages) != expected_reader_stages:
        raise RouteError("DWG reader-stage closure changed without review")
    lifecycle_route = reader_stages["dwgReader::readDwgTables"]
    if lifecycle_route["selector"].get("readerLifecycleEvidence") != dwg_reader_lifecycle_metadata(tree):
        raise RouteError("DWG reader lifecycle evidence changed")
    for symbol, route in reader_stages.items():
        if symbol != "dwgReader::readDwgTables" and route["selector"].get("readerLifecycleEvidence", []) != []:
            raise RouteError("DWG reader lifecycle evidence must be represented once")

    for route in inventory_category_routes(inventory, "dwgRW", "fixed-entity"):
        enum_symbol = route["selector"].get("enumSymbol")
        expected_evidence = dwg_compound_transition_metadata(tree, enum_symbol)
        if route["selector"].get("compoundTransitionEvidence", []) != expected_evidence:
            raise RouteError("DWG compound transition evidence changed: %s" % route["id"])
        expected_delivery = (
            dwg_compound_delivery_metadata(tree)
            if enum_symbol == "dwgType::INSERT" else []
        )
        if route["selector"].get("compoundDeliveryEvidence", []) != expected_delivery:
            raise RouteError("DWG compound delivery evidence changed: %s" % route["id"])

    raw_expected = {node["name"]: node for node in RAW_FLOW_NODES}
    if set(RAW_NODE_DIRECTIONS) != set(raw_expected):
        raise RouteError("raw-flow direction contract does not match node closure")
    raw_inputs = raw_flow_inputs_by_name()
    raw_routes = [
        route
        for facade in ("dxfRW", "dwgRW")
        for route in inventory_category_routes(inventory, facade, "raw-flow")
    ]
    raw_by_name = {route["selector"].get("name"): route for route in raw_routes}
    if len(raw_routes) != len(raw_by_name) or set(raw_by_name) != set(raw_expected):
        raise RouteError("raw-flow node closure changed without review")
    observed_phases: dict[str, set[str]] = {"dxf": set(), "dwg": set()}
    raw_ids = {route["id"] for route in raw_routes}
    reader_pipeline_ids = {
        route["id"] for route in inventory_category_routes(inventory, "dwgRW", "reader-pipeline")
    }
    writer_pipeline_ids = {
        route["id"] for route in inventory_category_routes(inventory, "dwgRW", "writer-pipeline")
    }
    for name, node in raw_expected.items():
        flow = node["flow"]
        phase = node["phase"]
        facade = node["facade"]
        anchors = node["anchors"]
        selector = raw_by_name[name]["selector"]
        if selector.get("flow") != flow or selector.get("phase") != phase:
            raise RouteError("raw-flow node has the wrong flow/phase: %s" % name)
        if raw_by_name[name]["facade"] != facade:
            raise RouteError("raw-flow node has the wrong façade: %s" % name)
        expected_symbols = sorted({symbol for _path, symbol in anchors})
        if selector.get("symbols") != expected_symbols:
            raise RouteError("raw-flow node symbols changed without review: %s" % name)
        expected_inputs = [raw_flow_route_id(facade, input_name) for input_name in raw_inputs[name]]
        expected_outputs = [raw_flow_route_id(facade, output_name) for output_name in node["outputs"]]
        expected_cross_session = [
            raw_flow_route_id(facade, output_name)
            for output_name in node.get("optionalCrossSessionHandoffs", ())
        ]
        expected_cross_session_transforms = [
            {
                "outputRouteId": raw_flow_route_id(facade, output_name),
                "kind": raw_carrier_transform(node["carrier"], raw_expected[output_name]["carrier"]),
            }
            for output_name in node.get("optionalCrossSessionHandoffs", ())
        ]
        expected_companion_carriers = [
            raw_flow_route_id(facade, output_name)
            for output_name in node.get("requiresCompanionRawSectionCarrier", ())
        ]
        if selector.get("carrier") != node["carrier"]:
            raise RouteError("raw-flow node carrier changed without review: %s" % name)
        if selector.get("inputRouteIds") != expected_inputs or selector.get("outputRouteIds") != expected_outputs:
            raise RouteError("raw-flow node handoff closure changed without review: %s" % name)
        expected_transforms = [
            {
                "outputRouteId": raw_flow_route_id(facade, output_name),
                "kind": raw_carrier_transform(node["carrier"], raw_expected[output_name]["carrier"]),
            }
            for output_name in node["outputs"]
        ]
        if selector.get("outputCarrierTransforms") != expected_transforms:
            raise RouteError("raw-flow node carrier transform changed without review: %s" % name)
        if selector.get("handoffSemantics") != RAW_HANDOFF_SEMANTICS:
            raise RouteError("raw-flow node handoff semantics changed without review: %s" % name)
        if (
            selector.get("optionalCrossSessionHandoffRouteIds") != expected_cross_session
            or selector.get("optionalCrossSessionHandoffCarrierTransforms")
            != expected_cross_session_transforms
            or selector.get("optionalCrossSessionHandoffSemantics")
            != RAW_CROSS_SESSION_HANDOFF_SEMANTICS
        ):
            raise RouteError("raw-flow node cross-session handoff changed without review: %s" % name)
        if set(expected_cross_session) & set(expected_outputs):
            raise RouteError("raw-flow node conflates a source edge with cross-session handoff: %s" % name)
        if selector.get("companionRawCarrierRouteIds") != expected_companion_carriers:
            raise RouteError("raw-flow node companion carrier changed without review: %s" % name)
        if selector.get("terminalDisposition") != node.get("terminalDisposition"):
            raise RouteError("raw-flow node terminal disposition changed without review: %s" % name)
        if selector.get("externalIngress") != node.get("externalIngress"):
            raise RouteError("raw-flow node external ingress changed without review: %s" % name)
        if not set(expected_inputs + expected_outputs) <= raw_ids:
            raise RouteError("raw-flow node has a dangling handoff: %s" % name)
        if not set(expected_cross_session) <= raw_ids:
            raise RouteError("raw-flow node has a dangling cross-session handoff: %s" % name)
        if not set(expected_companion_carriers) <= raw_ids:
            raise RouteError("raw-flow node has a dangling companion carrier: %s" % name)
        expected_reader_pipelines = [
            "dwgRW/reader-pipeline/" + identifier
            for identifier in node.get("readerPipelines", ())
        ]
        expected_writer_pipelines = [
            "dwgRW/writer-pipeline/" + identifier
            for identifier in node.get("writerPipelines", ())
        ]
        if (
            selector.get("readerPipelineIds") != expected_reader_pipelines
            or selector.get("writerPipelineIds") != expected_writer_pipelines
        ):
            raise RouteError("raw-flow pipeline applicability changed without review: %s" % name)
        if not set(expected_reader_pipelines) <= reader_pipeline_ids:
            raise RouteError("raw-flow node references a missing reader pipeline: %s" % name)
        if not set(expected_writer_pipelines) <= writer_pipeline_ids:
            raise RouteError("raw-flow node references a missing writer pipeline: %s" % name)
        if raw_by_name[name]["directions"] != sorted(set(RAW_NODE_DIRECTIONS[name])):
            raise RouteError("raw-flow node has incorrect reviewed directions: %s" % name)
        validate_raw_eligibility_metadata(tree, raw_by_name[name])
        validate_raw_writer_metadata(tree, raw_by_name[name])
        validate_dwg_raw_replay_metadata(tree, raw_by_name[name])
        validate_raw_route_publication_metadata(
            tree, raw_by_name[name], raw_ids, model_route_ids, callback_route_ids
        )
        observed_phases[flow].add(phase)
    for flow, phases in observed_phases.items():
        if phases != {"capture", "eligibility", "publication", "replay"}:
            raise RouteError("raw-flow lacks a required phase: %s" % flow)

    # Topology follows only physical, intra-session source calls.  A consumer
    # can transfer a carrier to a later writer session, but that optional
    # handoff is deliberately separate from ``outputs`` and never manufactures
    # a reader-to-writer call edge.  Every nonterminal still needs a path to a
    # reviewed terminal: either a writer replay or a reader callback handoff.
    for flow in ("dxf", "dwg"):
        names = {name for name, node in raw_expected.items() if node["flow"] == flow}
        roots = {name for name in names if not raw_inputs[name]}
        if not roots or any(
            raw_expected[name]["phase"] != "capture"
            and not isinstance(raw_expected[name].get("externalIngress"), str)
            for name in roots
        ):
            raise RouteError("raw-flow has an undeclared non-capture root: %s" % flow)
        reachable: set[str] = set()
        pending = sorted(roots)
        while pending:
            current = pending.pop()
            if current in reachable:
                continue
            reachable.add(current)
            pending.extend(raw_expected[current]["outputs"])
        if reachable != names:
            raise RouteError("raw-flow has an unreachable node: %s" % sorted(names - reachable))
        terminals = {name for name in names if not raw_expected[name]["outputs"]}
        if not terminals or any(
            raw_expected[name]["phase"] != "replay"
            and not (
                raw_expected[name]["phase"] == "publication"
                and isinstance(raw_expected[name].get("terminalDisposition"), str)
                and raw_expected[name]["terminalDisposition"].startswith("external-consumer-callback;")
            )
            for name in terminals
        ):
            raise RouteError("raw-flow has an unreviewed terminal: %s" % flow)
        reverse_reachable: set[str] = set()
        pending = sorted(terminals)
        while pending:
            current = pending.pop()
            if current in reverse_reachable:
                continue
            reverse_reachable.add(current)
            pending.extend(raw_inputs[current])
        if reverse_reachable != names:
            raise RouteError("raw-flow has no terminal path: %s" % sorted(names - reverse_reachable))

    stage_routes = inventory_category_routes(inventory, "shared", "publication-stage")
    stages = {route["selector"].get("name"): route for route in stage_routes}
    expected_stages = {
        name: (anchor_kind, path, symbol)
        for name, anchor_kind, path, symbol in PUBLICATION_STAGE_ANCHORS
    }
    if len(stage_routes) != len(stages) or set(stages) != set(expected_stages):
        raise RouteError("deferred publication-stage closure changed without review")
    for name, (anchor_kind, path, symbol) in expected_stages.items():
        selector = stages[name]["selector"]
        if (
            selector.get("anchorKind") != anchor_kind
            or selector.get("sourcePath") != path
            or selector.get("symbol") != symbol
        ):
            raise RouteError("publication-stage anchor changed without review: %s" % name)

    source_routes = [
        route
        for facade, categories in PARSER_PUBLICATION_CATEGORIES.items()
        for route in inventory[facade]
        if route["category"] in categories
        and not route["selector"].get("preDispatchDisposition")
    ]
    fixed_object_ids = {
        route["id"]
        for route in inventory_category_routes(inventory, "dwgRW", "fixed-object")
    }
    deferred_entity_routes = [
        route
        for route in inventory_category_routes(inventory, "dwgRW", "fixed-entity")
        if route["selector"].get("preDispatchDisposition") is not None
    ]
    for route in deferred_entity_routes:
        selector = route["selector"]
        if (
            selector.get("preDispatchDisposition") != "deferred-object-pass"
            or selector.get("objectPassRoute") not in fixed_object_ids
            or route["id"] in {source_route["id"] for source_route in source_routes}
        ):
            raise RouteError("DWG pre-dispatch object disposition is malformed: %s" % route["id"])
    if tree.label == "target":
        if [route["id"] for route in deferred_entity_routes] != ["dwgRW/fixed-entity/1004-DBCOLOR"]:
            raise RouteError("pinned DWG pre-dispatch object disposition changed without review")
        entity_reader = function_body(tree.require("src/intern/dwgreader.cpp"), "dwgReader::readDwgEntityWithOutput")
        if "dwgObjType::isFixedObject(oType)" not in entity_reader.code or "deferObject(obj)" not in entity_reader.code:
            raise RouteError("DWG pre-dispatch object gate changed without review")
    expected_parser_routes = {route["id"] for route in source_routes}
    source_routes_by_id = {route["id"]: route for route in source_routes}
    publication_routes = [
        route
        for facade in PARSER_PUBLICATION_CATEGORIES
        for route in inventory[facade]
        if route["category"] == "parser-publication"
    ]
    publications = {route["selector"].get("parserRoute"): route for route in publication_routes}
    if len(publication_routes) != len(publications) or set(publications) != expected_parser_routes:
        raise RouteError("parser-to-publication route closure is incomplete")
    callback_routes = {
        route["selector"].get("name"): route
        for route in inventory_category_routes(inventory, "shared", "callback")
    }
    model_routes = {
        route["selector"].get("name"): route
        for route in inventory_category_routes(inventory, "shared", "public-model")
    }
    model_parents = publication_model_parents(tree, set(model_routes))
    stage_route_ids = {route["id"] for route in stage_routes}
    for parser_route, route in publications.items():
        source_route = source_routes_by_id[parser_route]
        selector = route["selector"]
        mode = selector.get("mode")
        pairs = selector.get("directPairs")
        raw_publications = selector.get("rawCarrierPublications")
        stage_ids = selector.get("stageRouteIds")
        stage_refs = selector.get("stageRefs")
        if (
            mode not in {"direct", "mixed", "stage"}
            or not isinstance(pairs, list)
            or not isinstance(raw_publications, list)
            or not isinstance(stage_ids, list)
            or not isinstance(stage_refs, list)
            or "models" in selector
            or "callbacks" in selector
        ):
            raise RouteError("parser-publication route is malformed: %s" % parser_route)
        if stage_ids != sorted(set(stage_ids)) or set(stage_ids) - stage_route_ids:
            raise RouteError("parser-publication route has an unresolved stage bridge: %s" % parser_route)
        if [ref.get("routeId") for ref in stage_refs] != stage_ids:
            raise RouteError("parser-publication stage references are not canonical: %s" % parser_route)
        if any(
            set(ref) != {"routeId", "reason"}
            or not isinstance(ref.get("reason"), str)
            or not ref["reason"]
            for ref in stage_refs
        ):
            raise RouteError("parser-publication stage reference is malformed: %s" % parser_route)
        pair_keys = []
        for pair in pairs:
            if not isinstance(pair, dict):
                raise RouteError("parser-publication pair is malformed: %s" % parser_route)
            model = pair.get("model")
            callback = pair.get("callback")
            if (
                model not in model_routes
                or callback not in callback_routes
                or model == "DRW_RawDxfObject"
                or pair.get("modelRoute") != model_routes[model]["id"]
                or pair.get("callbackRoute") != callback_routes[callback]["id"]
                or pair.get("callbackKind") != callback_routes[callback]["selector"].get("callbackKind")
                or pair.get("bindingKind") not in {"local", "lambda-param", "smart-pointer-local", "member"}
                or pair.get("argumentPassing") not in {"object", "pointer"}
                or pair.get("sourceArgumentPassing") not in {"object", "pointer"}
                or pair.get("callbackPassingVia") not in {
                    "direct", "dxf-raw-captured-template", "dwg-output-append",
                    "dwg-emit-with-extrusion",
                }
                or pair.get("callStyle") not in {
                    "interface-call", "dxf-raw-captured-template", "dwg-output-append",
                    "dwg-emit-with-extrusion",
                }
            ):
                raise RouteError("parser-publication pair has an unresolved endpoint: %s" % parser_route)
            compatible, compatibility = callback_accepts_model(
                callback_routes[callback], model, model_parents, pair["argumentPassing"]
            )
            if not compatible or pair.get("callbackCompatibility") != compatibility:
                raise RouteError(
                    "parser-publication callback/model compatibility is not proven: %s"
                    % parser_route
                )
            context = pair.get("branchContext")
            if (
                pair.get("deliveryCardinality") not in {
                    "single", "conditional-or-repeated-unresolved",
                }
                or not isinstance(pair.get("deliveryBundle"), str)
                or not pair["deliveryBundle"]
                or (
                    pair.get("deliveryCardinality") == "single"
                    and "alternativeBranchGroup" in pair
                )
                or (
                    pair.get("deliveryCardinality") == "conditional-or-repeated-unresolved"
                    and not isinstance(pair.get("alternativeBranchGroup"), str)
                )
            ):
                raise RouteError("parser-publication pair branch evidence is malformed: %s" % parser_route)
            validate_publication_condition_ancestry(
                pair.get("conditionAncestry"), context, tree, parser_route
            )
            validate_named_publication_selector(
                source_route, pair.get("conditionAncestry"), parser_route
            )
            call_style = pair["callStyle"]
            passing_via = pair["callbackPassingVia"]
            source_passing = pair["sourceArgumentPassing"]
            effective_passing = pair["argumentPassing"]
            if call_style == "interface-call":
                if passing_via != "direct" or source_passing != effective_passing:
                    raise RouteError(
                        "direct parser-publication pair has an unproved adaptation: %s"
                        % parser_route
                    )
            elif call_style == "dxf-raw-captured-template":
                if passing_via != call_style or source_passing != effective_passing:
                    raise RouteError(
                        "DXF template parser-publication adaptation is malformed: %s"
                        % parser_route
                    )
                template_invocation = pair.get("templateInvocationEvidence")
                if (
                    pair.get("bindingKind") != "lambda-param"
                    or not isinstance(template_invocation, dict)
                    or set(template_invocation) != {"path", "symbol", "line", "spanSha256"}
                    or template_invocation.get("path") not in tree.files
                    or not isinstance(template_invocation.get("line"), int)
                ):
                    raise RouteError(
                        "DXF template parser-publication lacks invocation proof: %s"
                        % parser_route
                    )
            else:
                # The DWG output helpers receive a local object, then may adapt
                # it to the pointer form demanded by the registered callback.
                if (
                    passing_via != call_style
                    or source_passing != "object"
                    or effective_passing not in {"object", "pointer"}
                ):
                    raise RouteError(
                        "DWG parser-publication helper adaptation is malformed: %s"
                        % parser_route
                    )
            for location_name in ("callEvidence", "bindingEvidence"):
                location = pair.get(location_name)
                if not isinstance(location, dict) or set(location) != {"path", "symbol", "line", "spanSha256"}:
                    raise RouteError("parser-publication pair lacks exact evidence: %s" % parser_route)
                if location["path"] not in tree.files or not isinstance(location["line"], int):
                    raise RouteError("parser-publication pair has invalid evidence ownership: %s" % parser_route)
            pair_keys.append(json.dumps(pair, sort_keys=True))
        if len(pair_keys) != len(set(pair_keys)) or pairs != sorted(
            pairs,
            key=lambda pair: (
                pair["model"], pair["callback"], pair["binding"],
                pair["callEvidence"]["line"],
            ),
        ):
            raise RouteError("parser-publication pairs are non-deterministic: %s" % parser_route)
        pair_endpoint_groups: dict[tuple[str, str], list[dict]] = {}
        for pair in pairs:
            pair_endpoint_groups.setdefault((pair["model"], pair["callback"]), []).append(pair)
        for endpoint, grouped_pairs in pair_endpoint_groups.items():
            if len(grouped_pairs) == 1:
                row = grouped_pairs[0]
                if row["deliveryCardinality"] == "single":
                    # ``single`` is reserved for a future explicit
                    # post-delivery terminal proof.  The current extractor
                    # intentionally never manufactures that proof.
                    raise RouteError(
                        "parser-publication single cardinality lacks terminal proof: %s"
                        % parser_route
                    )
                if not isinstance(row.get("alternativeBranchGroup"), str) or not row["alternativeBranchGroup"]:
                    raise RouteError("unresolved parser-publication endpoint lacks guard: %s" % parser_route)
                continue
            groups = {pair.get("alternativeBranchGroup") for pair in grouped_pairs}
            if (
                any(pair["deliveryCardinality"] != "conditional-or-repeated-unresolved" for pair in grouped_pairs)
                or len(groups) != 1
                or not isinstance(next(iter(groups)), str)
            ):
                raise RouteError("duplicate parser-publication endpoint lacks cardinality guard: %s/%s" % (parser_route, endpoint))
        raw_keys = []
        for raw in raw_publications:
            callback = raw.get("callback") if isinstance(raw, dict) else None
            carrier = raw.get("carrier") if isinstance(raw, dict) else None
            expected_callback = {
                "DRW_RawDxfObject": "addRawDxf",
                "DRW_UnsupportedObject": "addUnsupportedObject",
            }.get(carrier)
            if (
                not isinstance(raw, dict)
                or expected_callback is None
                or callback not in callback_routes
                or not isinstance(callback, str)
                or (
                    not callback.startswith(expected_callback)
                    if expected_callback == "addRawDxf"
                    else callback != expected_callback
                )
                or raw.get("callbackRoute") != callback_routes[callback]["id"]
                or raw.get("argumentPassing") not in {"object", "pointer"}
                or raw.get("sourceArgumentPassing") not in {"object", "pointer"}
                or raw.get("callbackPassingVia") not in {
                    "direct", "dxf-raw-captured-template", "dwg-output-append",
                    "dwg-emit-with-extrusion",
                }
                or raw.get("callStyle") not in {
                    "interface-call", "dxf-raw-captured-template", "dwg-output-append",
                    "dwg-emit-with-extrusion",
                }
                or raw.get("bindingKind") not in {
                    "local", "lambda-param", "smart-pointer-local", "member",
                    "raw-factory-temporary", "template-raw-carrier",
                }
            ):
                raise RouteError("parser-publication raw carrier is malformed: %s" % parser_route)
            raw_model = carrier
            compatible, compatibility = callback_accepts_model(
                callback_routes[callback], raw_model, model_parents, raw["argumentPassing"]
            )
            if not compatible or raw.get("callbackCompatibility") != compatibility:
                raise RouteError("parser-publication raw callback compatibility is not proven: %s" % parser_route)
            context = raw.get("branchContext")
            if (
                raw.get("deliveryCardinality") not in {
                    "single", "conditional-or-repeated-unresolved",
                }
                or not isinstance(raw.get("deliveryBundle"), str)
                or not raw["deliveryBundle"]
                or (
                    raw.get("deliveryCardinality") == "single"
                    and "alternativeBranchGroup" in raw
                )
                or (
                    raw.get("deliveryCardinality") == "conditional-or-repeated-unresolved"
                    and not isinstance(raw.get("alternativeBranchGroup"), str)
                )
            ):
                raise RouteError("raw parser-publication branch evidence is malformed: %s" % parser_route)
            validate_publication_condition_ancestry(
                raw.get("conditionAncestry"), context, tree, parser_route
            )
            validate_named_publication_selector(
                source_route, raw.get("conditionAncestry"), parser_route
            )
            call_style = raw["callStyle"]
            passing_via = raw["callbackPassingVia"]
            source_passing = raw["sourceArgumentPassing"]
            effective_passing = raw["argumentPassing"]
            if call_style == "interface-call":
                if passing_via != "direct" or source_passing != effective_passing:
                    raise RouteError(
                        "direct raw parser-publication has an unproved adaptation: %s"
                        % parser_route
                    )
            elif call_style == "dxf-raw-captured-template":
                if passing_via != call_style or source_passing != effective_passing:
                    raise RouteError(
                        "DXF template raw parser-publication adaptation is malformed: %s"
                        % parser_route
                    )
                if (
                    raw.get("bindingKind") != "template-raw-carrier"
                    or carrier != "DRW_RawDxfObject"
                    or callback != "addRawDxfObject"
                ):
                    raise RouteError(
                        "DXF template raw carrier has the wrong endpoint: %s" % parser_route
                    )
                for location_name in (
                    "templateInvocationEvidence", "templateBridgeEvidence",
                ):
                    location = raw.get(location_name)
                    if (
                        not isinstance(location, dict)
                        or set(location) != {"path", "symbol", "line", "spanSha256"}
                        or location.get("path") not in tree.files
                        or not isinstance(location.get("line"), int)
                    ):
                        raise RouteError(
                            "DXF template raw carrier lacks bridge evidence: %s" % parser_route
                        )
            elif (
                passing_via != call_style
                or source_passing != "object"
                or effective_passing not in {"object", "pointer"}
            ):
                raise RouteError(
                    "DWG raw parser-publication helper adaptation is malformed: %s"
                    % parser_route
                )
            for location_name in ("callEvidence", "bindingEvidence"):
                location = raw.get(location_name)
                if (
                    not isinstance(location, dict)
                    or set(location) != {"path", "symbol", "line", "spanSha256"}
                    or location.get("path") not in tree.files
                    or not isinstance(location.get("line"), int)
                ):
                    raise RouteError("raw parser-publication lacks exact evidence: %s" % parser_route)
            raw_keys.append(json.dumps(raw, sort_keys=True))
        if len(raw_keys) != len(set(raw_keys)) or raw_publications != sorted(
            raw_publications,
            key=lambda raw: (raw["callback"], raw["binding"], raw["callEvidence"]["line"]),
        ):
            raise RouteError("parser-publication raw carriers are non-deterministic: %s" % parser_route)
        raw_endpoint_groups: dict[tuple[str, str], list[dict]] = {}
        for raw in raw_publications:
            raw_endpoint_groups.setdefault((raw["carrier"], raw["callback"]), []).append(raw)
        for endpoint, grouped_raw in raw_endpoint_groups.items():
            if len(grouped_raw) == 1:
                row = grouped_raw[0]
                if row["deliveryCardinality"] == "single":
                    raise RouteError(
                        "raw parser-publication single cardinality lacks terminal proof: %s"
                        % parser_route
                    )
                if not isinstance(row.get("alternativeBranchGroup"), str) or not row["alternativeBranchGroup"]:
                    raise RouteError("unresolved raw parser-publication endpoint lacks guard: %s" % parser_route)
                continue
            groups = {raw.get("alternativeBranchGroup") for raw in grouped_raw}
            if (
                any(raw["deliveryCardinality"] != "conditional-or-repeated-unresolved" for raw in grouped_raw)
                or len(groups) != 1
                or not isinstance(next(iter(groups)), str)
            ):
                raise RouteError("duplicate raw parser-publication endpoint lacks cardinality guard: %s/%s" % (parser_route, endpoint))
        template_pairs = [
            pair for pair in pairs
            if pair.get("callStyle") == "dxf-raw-captured-template"
        ]
        template_raw = [
            raw for raw in raw_publications
            if raw.get("bindingKind") == "template-raw-carrier"
        ]
        if template_pairs or template_raw:
            bridge = verify_dxf_raw_captured_template_bridge(tree)
            if bridge is None:
                raise RouteError("template publication has no verified raw-capture bridge: %s" % parser_route)
            typed_by_invocation: dict[str, list[dict]] = {}
            raw_by_invocation: dict[str, list[dict]] = {}
            for pair in template_pairs:
                typed_by_invocation.setdefault(
                    canonical_json(pair["templateInvocationEvidence"]), []
                ).append(pair)
            for raw in template_raw:
                raw_by_invocation.setdefault(
                    canonical_json(raw["templateInvocationEvidence"]), []
                ).append(raw)
            if set(typed_by_invocation) != set(raw_by_invocation):
                raise RouteError("template typed/raw invocation closure is incomplete: %s" % parser_route)
            for invocation in typed_by_invocation:
                typed_rows = typed_by_invocation[invocation]
                raw_rows = raw_by_invocation[invocation]
                if len(typed_rows) != 1 or len(raw_rows) != 1:
                    raise RouteError("template invocation is not one typed/raw pair: %s" % parser_route)
                typed_row, raw_row = typed_rows[0], raw_rows[0]
                if (
                    typed_row["conditionAncestry"] != raw_row["conditionAncestry"]
                    or typed_row["deliveryBundle"] != raw_row["deliveryBundle"]
                    or raw_row.get("templateBridgeEvidence") != bridge["templateEvidence"]
                    or raw_row.get("callEvidence") != bridge["rawCallbackEvidence"]
                    or raw_row.get("bindingEvidence") != bridge["rawBindingEvidence"]
                ):
                    raise RouteError("template typed/raw co-delivery proof changed: %s" % parser_route)
        has_direct_delivery = bool(pairs or raw_publications)
        if mode == "direct" and (not has_direct_delivery or stage_ids):
            raise RouteError("direct parser-publication route has invalid staging: %s" % parser_route)
        if mode == "mixed" and (not has_direct_delivery or not stage_ids):
            raise RouteError("mixed parser-publication route has invalid staging: %s" % parser_route)
        if mode == "stage" and (has_direct_delivery or not stage_ids):
            raise RouteError("staged parser-publication route is unbound or overclaimed: %s" % parser_route)


def validate_inventory(tree: SourceTree, inventory: dict[str, list[dict]]) -> None:
    """Check stable route identities and the pinned target's anchor shape."""
    for facade, routes in inventory.items():
        ids = [route["id"] for route in routes]
        if ids != sorted(ids) or len(ids) != len(set(ids)):
            raise RouteError("%s inventory has non-deterministic or duplicate route IDs" % facade)
        for route in routes:
            if route["facade"] != facade or not route["evidence"]:
                raise RouteError("%s inventory has an unowned route" % facade)
            for location in route["evidence"]:
                if location["path"].startswith("/") or "\\" in location["path"]:
                    raise RouteError("inventory leaked a non-logical source path")
    source_units = [route for route in inventory["shared"] if route["category"] == "source-unit"]
    observed = {route["selector"].get("path") for route in source_units}
    expected = set(source_unit_roles_for(tree))
    if len(source_units) != len(observed) or observed != expected:
        raise RouteError(
            "%s source-unit closure failed: expected=%s observed=%s"
            % (tree.label, sorted(expected), sorted(observed))
        )
    for route in source_units:
        selector = route["selector"]
        path = selector["path"]
        role = selector.get("role")
        if role != source_unit_roles_for(tree)[path]:
            raise RouteError("source-unit role changed without review: %s" % path)
        if selector.get("classification") == "functional" and not selector.get("requiresPipelineEdges"):
            raise RouteError("functional source-unit omits pipeline-edge requirement: %s" % path)
    validate_pipeline_closure(tree, inventory)
    if tree.label != "target":
        return
    public_categories = {
        category
        for counts in TARGET_PUBLIC_HEADER_ROUTE_COUNTS.values()
        for category in counts
    }
    observed_public: dict[str, dict[str, int]] = {}
    for routes in inventory.values():
        for route in routes:
            if route["category"] not in public_categories:
                continue
            paths = {item["path"] for item in route["evidence"]}
            if len(paths) != 1:
                raise RouteError("public header route has ambiguous evidence: %s" % route["id"])
            path = next(iter(paths))
            category_counts = observed_public.setdefault(path, {})
            category_counts[route["category"]] = category_counts.get(route["category"], 0) + 1
    if observed_public != TARGET_PUBLIC_HEADER_ROUTE_COUNTS:
        raise RouteError(
            "pinned target public-header contract changed: expected %s, observed %s"
            % (TARGET_PUBLIC_HEADER_ROUTE_COUNTS, observed_public)
        )
    for facade, expected in TARGET_ROUTE_COUNTS.items():
        actual: dict[str, int] = {}
        for route in inventory[facade]:
            actual[route["category"]] = actual.get(route["category"], 0) + 1
        if actual != expected:
            raise RouteError(
                "pinned target route contract changed for %s: expected %s, observed %s"
                % (facade, expected, actual)
            )


def comparable_route(route: dict) -> dict:
    return {
        key: route[key]
        for key in ("id", "facade", "category", "selector", "directions", "sourceDisposition")
    }


def compare_inventories(target: dict[str, list[dict]], standalone: dict[str, list[dict]]) -> dict:
    result: dict[str, dict] = {}
    for facade in ("dxfRW", "dwgRW", "shared"):
        target_by_id = {route["id"]: route for route in target[facade]}
        standalone_by_id = {route["id"]: route for route in standalone[facade]}
        common = sorted(set(target_by_id) & set(standalone_by_id))
        mismatches = [
            route_id
            for route_id in common
            if comparable_route(target_by_id[route_id]) != comparable_route(standalone_by_id[route_id])
        ]
        result[facade] = {
            "targetOnly": sorted(set(target_by_id) - set(standalone_by_id)),
            "standaloneOnly": sorted(set(standalone_by_id) - set(target_by_id)),
            "semanticMismatches": mismatches,
        }
    return result


def summarize(inventories: dict[str, dict[str, list[dict]]], comparison: dict) -> dict:
    result: dict[str, dict] = {}
    for side in ("target", "standalone"):
        routes = [route for facade in inventories[side].values() for route in facade]
        categories: dict[str, int] = {}
        for route in routes:
            categories[route["category"]] = categories.get(route["category"], 0) + 1
        result[side] = {"routeCount": len(routes), "categoryCounts": dict(sorted(categories.items()))}
    result["comparison"] = {
        facade: {
            key: len(value)
            for key, value in comparison[facade].items()
        }
        for facade in ("dxfRW", "dwgRW", "shared")
    }
    return result


def verify_public_surface_metadata(root: Path) -> None:
    """Keep the source-only public routes linked to the reviewed ABI records."""
    baseline = read_json(root / "metadata/baseline-api-surface-v1.json")
    if not isinstance(baseline.get("headers"), list) or not baseline["headers"]:
        raise RouteError("baseline API surface has no header inventory")
    decisions = read_json(root / "metadata/compatibility-decisions-v1.json")
    values = decisions.get("decisions")
    if not isinstance(values, list) or not any(
        isinstance(value, dict) and value.get("surface") == "dwgR" for value in values
    ):
        raise RouteError("compatibility decisions omit the deprecated dwgR surface")


def generate(root: Path, target_repo: Path) -> dict:
    source_lock = read_json(root / "metadata/libdxfrw-target-lock.json")
    if source_lock.get("schema") != 1 or not isinstance(source_lock.get("libreCAD"), dict):
        raise RouteError("invalid target source lock")
    if source_lock["libreCAD"].get("sourceRoot") != SOURCE_ROOT:
        raise RouteError("target source lock has unexpected source root")
    verify_target_inputs(root, target_repo, source_lock)
    verify_public_surface_metadata(root)
    target_tree = load_target_tree(root, target_repo, source_lock)
    public_headers = target_public_header_paths(root, target_repo, source_lock, target_tree)
    standalone_tree = load_standalone_tree(root, set(target_tree.files))
    target_inventory = inventory_tree(target_tree, public_headers)
    standalone_inventory = inventory_tree(standalone_tree, public_headers)
    validate_inventory(target_tree, target_inventory)
    validate_inventory(standalone_tree, standalone_inventory)
    comparison = compare_inventories(target_inventory, standalone_inventory)
    inventories = {"target": target_inventory, "standalone": standalone_inventory}
    return {
        "schema": SCHEMA,
        "kind": "libdxfrw-source-route-inventory",
        "fixturePolicy": "no-drawing-payloads; source-and-metadata-only",
        "generator": {
            "path": "tools/extract_parity_source_routes.py",
            "parserSchema": PARSER_SCHEMA,
        },
        "inputs": {
            "target": {
                **target_tree.metadata,
                "files": source_file_fingerprints(target_tree),
                "publicHeaders": list(public_headers),
                "publicHeaderCMake": TARGET_SOURCES_CMAKE,
            },
            "standalone": {
                **standalone_tree.metadata,
                "files": source_file_fingerprints(standalone_tree),
                "targetPublicHeaders": list(public_headers),
            },
        },
        "inventories": inventories,
        "comparison": comparison,
        "summary": summarize(inventories, comparison),
    }


def artifact_payloads(generated: dict, output: Path) -> tuple[str, list[tuple[Path, str]]]:
    """Split the deterministic route ledger into small, reviewable shards."""
    shard_dir = output.parent / output.stem
    shards: list[tuple[Path, str]] = []
    shard_index: list[dict] = []
    inventories = generated["inventories"]
    for side in ("target", "standalone"):
        for facade in ("dxfRW", "dwgRW", "shared"):
            routes = inventories[side][facade]
            shard_path = shard_dir / (side + "-" + facade + ".json")
            shard = {
                "schema": SCHEMA,
                "kind": "libdxfrw-source-route-shard",
                "side": side,
                "facade": facade,
                "routes": routes,
            }
            rendered = canonical_json(shard)
            shards.append((shard_path, rendered))
            shard_index.append(
                {
                    "path": shard_path.relative_to(output.parent).as_posix(),
                    "side": side,
                    "facade": facade,
                    "routeCount": len(routes),
                    "sha256": sha256_text(rendered),
                }
            )
    index = {key: value for key, value in generated.items() if key != "inventories"}
    index["shards"] = shard_index
    return canonical_json(index), shards


def write_or_check_artifacts(generated: dict, output: Path, check: bool) -> None:
    index, shards = artifact_payloads(generated, output)
    expected_paths = {path for path, _text in shards}
    if check:
        payloads = [(output, index), *shards]
        for path, expected in payloads:
            try:
                actual = path.read_text(encoding="utf-8")
            except OSError as exc:
                raise RouteError("cannot read checked inventory %s: %s" % (path, exc)) from exc
            if actual != expected:
                raise RouteError("inventory is stale or non-deterministic: %s" % path)
        shard_dir = output.parent / output.stem
        actual_paths = set(shard_dir.glob("*.json")) if shard_dir.is_dir() else set()
        if actual_paths != expected_paths:
            raise RouteError("inventory shard set is stale or incomplete: %s" % shard_dir)
        print("parity source route inventory check: PASS (%s)" % output)
        return
    output.parent.mkdir(parents=True, exist_ok=True)
    for path, rendered in shards:
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(rendered, encoding="utf-8")
    output.write_text(index, encoding="utf-8")
    print("parity source route inventory: wrote %s and %d shards" % (output, len(shards)))


def self_test() -> None:
    source = SourceFile(
        "src/example.cpp",
        """
// dxfRW::ignored() { case dwgType::FAKE: }
bool dxfRW::real() { const char *s = \"case dwgType::FAKE\"; if (true) { return true; } }
bool dxfRW::raw() { const char *s = R\"tag({ fake })tag\"; return true; }
bool dxfRW::dispatch() {
    const char *ignored = R\"tag(dxfKeywordEquals(nextentity, \"FAKE\"))tag\";
    if (dxfKeywordEquals(nextentity, \"LINE\")
        || dxfKeywordEquals(nextentity, \"LINE_ALIAS\")) {
        processed = processLine();
    } else if (DRW_Dynamic::matches(nextentity)) {
        processed = processDynamic();
    } else {
        processed = processRawEntity();
    }
}
bool dxfRW::unbound() {
    if (dxfKeywordEquals(nextentity, \"BROKEN\"))
        return false;
    return true;
}
void dxfRW::writeThing(int) {}
void dxfRW::writeThing(double) {}
bool dwgReader::route() {
    switch (oType) {
    case dwgType::LINE: {
        switch (inner) { case dwgType::INNER: break; }
        break;
    }
    default: break;
    }
    return true;
}
bool dwgReader::switchPairs() {
    switch (oType) {
    case dwgType::LINE:
    case dwgType::RAY: {
        intfa.addLine(line);
        break;
    }
    case dwgType::CIRCLE: {
        intfa.addCircle(circle);
        break;
    }
    default: {
        intfa.addArc(fallback);
        break;
    }
    }
    return true;
}
RawValType classifyRaw(int code) {
    switch (code) {
    case DxfValueKind::Dbl: return RawValType::Dbl;
    case DxfValueKind::Bin:
    default: return RawValType::Str;
    }
}
void useMultilineCall() {
    if (
        dxfRW::notADeclaration(
            42)) {
    }
}
namespace dwgType { enum Entity { LINE = 19, ARC }; }
""",
        "0" * 64,
    )
    real = function_body(source, "dxfRW::real")
    assert "FAKE" not in real.code
    assert real.text.count("{") >= 2
    raw = function_body(source, "dxfRW::raw")
    assert "fake" not in raw.code
    enum = namespace_enum_body(source, "dwgType", "Entity")
    assert [(name, value) for name, value, _ in enum_values(enum)] == [("LINE", 19), ("ARC", 20)]
    try:
        function_body(source, "dxfRW::missing")
    except RouteError:
        pass
    else:
        raise AssertionError("missing required anchor was accepted")
    try:
        function_body(source, "dxfRW::notADeclaration")
    except RouteError:
        pass
    else:
        raise AssertionError("multiline call expression was accepted as a definition")
    dispatch = function_body(source, "dxfRW::dispatch")
    branches = dispatch_branches(dispatch, names=("nextentity",))
    assert len(branches) == 3
    assert branches[0]["literals"] == ["LINE", "LINE_ALIAS"]
    assert branches[1]["openEnded"] and branches[1]["target"] == "processDynamic"
    assert branches[2]["fallback"] and branches[2]["target"] == "processRawEntity"
    try:
        dispatch_branches(function_body(source, "dxfRW::unbound"), names=("nextentity",))
    except RouteError:
        pass
    else:
        raise AssertionError("unbound dispatch condition was accepted")
    route_switch = switch_body(function_body(source, "dwgReader::route"), "oType")
    assert len(switch_cases(route_switch)) == 1
    switch_pairs = dwg_switch_case_bodies(function_body(source, "dwgReader::switchPairs"))
    assert "intfa.addLine(line)" in switch_pairs["dwgType::LINE"].code
    assert "intfa.addLine(line)" in switch_pairs["dwgType::RAY"].code
    assert "intfa.addCircle(circle)" in switch_pairs["dwgType::CIRCLE"].code
    assert "addArc" not in switch_pairs["dwgType::CIRCLE"].code
    assert "default" not in switch_pairs
    raw_rules = raw_classifier_rules(function_body(source, "classifyRaw"))
    assert raw_rules[0][0]["rawValue"] == "RawValType::Dbl"
    overloads = qualified_definitions(source, "dxfRW::writeThing")
    assert len(overloads) == 2
    assert len(function_bodies(source, "dxfRW::writeThing")) == 2
    assert signature_fingerprint(overloads[0]) != signature_fingerprint(overloads[1])
    collector = RouteCollector()
    collector.add(
        "dxfRW",
        "dxf-entity",
        {"kind": "exact", "canonical": "LINE", "aliases": ["LINE"]},
        real,
        identifier="LINE",
        directions=["read"],
    )
    collector.add(
        "dxfRW",
        "dxf-entity",
        {"kind": "exact", "canonical": "LINE", "aliases": ["LINE"]},
        raw,
        identifier="LINE",
        directions=["read"],
    )
    assert len(collector.routes()) == 1

    pipeline_source = SourceFile(
        "src/pipeline.cpp",
        """
class TransportBase {};
class TransportChild final : public TransportBase {};
bool DRW_Line::parseCode(int) { return true; }
bool DRW_Line::encodeDwg(int) { return true; }
bool dxfRW::processLine() { DRW_Line line; iface->addLine(line); return true; }
""",
        "5" * 64,
    )
    pipeline_tree = SourceTree("target", {"src/pipeline.cpp": pipeline_source}, {})
    child = class_definition_anchor(pipeline_tree, "TransportChild")
    assert class_definition_parent(child) == "TransportBase"
    model_defs = model_qualified_definitions(pipeline_source)
    assert {body.symbol for body in model_defs} == {"DRW_Line::parseCode", "DRW_Line::encodeDwg"}
    assert {model_codec_operation(body.symbol.rsplit("::", 1)[1]) for body in model_defs} == {"dxf-parse", "dwg-encode"}
    publication_collector = RouteCollector()
    declaration = FunctionBody(pipeline_source, "DRW_Line", 0, 0, 0)
    publication_collector.add(
        "shared", "public-model", {"kind": "symbol", "name": "DRW_Line"}, declaration,
        identifier="DRW_Line", directions=["publish"], line=1,
    )
    publication_collector.add(
        "shared", "callback", {
            "kind": "symbol", "name": "addLine", "callbackKind": "data-add",
            "parameterModel": "DRW_Line", "parameterPassing": "reference",
            "parameterModelCount": 1,
        }, declaration,
        identifier="addLine", directions=["publish"], line=1,
    )
    process_line = function_body(pipeline_source, "dxfRW::processLine")
    publication_collector.add(
        "dxfRW", "dxf-entity",
        {"kind": "exact", "canonical": "LINE", "aliases": ["LINE"], "target": "processLine"},
        process_line, identifier="LINE", directions=["read", "publish"],
    )
    publication_tree = SourceTree(
        "target",
        {
            "src/libdxfrw.cpp": pipeline_source,
            "src/intern/dwgreader.cpp": SourceFile("src/intern/dwgreader.cpp", "void noop() {}", "6" * 64),
        },
        {},
    )
    add_parser_publication_routes(publication_collector, publication_tree)
    publication = [
        route for route in publication_collector.routes()
        if route["category"] == "parser-publication"
    ]
    assert len(publication) == 1
    assert publication[0]["selector"]["mode"] == "direct"
    assert publication[0]["selector"]["directPairs"] == [
        {
            "model": "DRW_Line",
            "modelRoute": "shared/public-model/DRW_Line",
            "callback": "addLine",
            "callbackRoute": "shared/callback/addLine",
            "callbackKind": "data-add",
            "binding": "line",
            "bindingKind": "local",
            "argumentPassing": "object",
            "sourceArgumentPassing": "object",
            "callbackPassingVia": "direct",
            "callStyle": "interface-call",
            "callbackCompatibility": "exact",
            "branchContext": publication[0]["selector"]["directPairs"][0]["branchContext"],
            "conditionAncestry": publication[0]["selector"]["directPairs"][0]["conditionAncestry"],
            "deliveryCardinality": "conditional-or-repeated-unresolved",
            "alternativeBranchGroup": publication[0]["selector"]["directPairs"][0]["alternativeBranchGroup"],
            "deliveryBundle": publication[0]["selector"]["directPairs"][0]["deliveryBundle"],
            "callEvidence": publication[0]["selector"]["directPairs"][0]["callEvidence"],
            "bindingEvidence": publication[0]["selector"]["directPairs"][0]["bindingEvidence"],
        }
    ]

    pair_source = SourceFile(
        "src/pairs.cpp",
        """
bool dxfRW::exactPairs() {
    DRW_Line line;
    DRW_Circle circle;
    DRW_RawDxfObject raw;
    iface->addLine(line);
    iface->addCircle(circle);
    iface->addRawDxfObject(raw);
    return true;
}
bool dxfRW::lambdaPair() {
    return helper([](DRW_Line& data) { iface->addLine(data); });
}
bool dxfRW::rawTemplatePair() {
    return processRawCapturedObject<DRW_Line>(
        "rawTemplatePair", [this](DRW_Line& data) { iface->addLine(data); });
}
bool dxfRW::debugLambdaPair() {
    return processRawCapturedObject<DRW_Line>(
        deriveDebugName([this](DRW_Line& data) { iface->addLine(data); }),
        [](DRW_Line&) {});
}
bool dxfRW::conditionalTemplatePair() {
    return processRawCapturedObject<DRW_Line>(
        "conditionalTemplatePair", [this](DRW_Line& data) {
            if (emit) iface->addLine(data);
        });
}
bool dxfRW::nestedLambdaTemplatePair() {
    return processRawCapturedObject<DRW_Line>(
        "nestedLambdaTemplatePair", [this](DRW_Line& data) {
            auto deferred = [&]() { iface->addLine(data); };
        });
}
bool dxfRW::switchPairs(int selector) {
    switch (selector) {
    case 0:
    case 1: {
        DRW_Line line;
        iface->addLine(line);
        break;
    }
    case 2: {
        DRW_Circle circle;
        iface->addCircle(circle);
        break;
    }
    }
    return true;
}
bool dxfRW::conditionalBreakPairs(int selector) {
    switch (selector) {
    case 0:
        DRW_Line line;
        iface->addLine(line);
        if (stop) break;
    case 1: {
        DRW_Circle circle;
        iface->addCircle(circle);
        break;
    }
    }
    return true;
}
bool dxfRW::outerGuardCasePairs(int selector) {
    if (outerOne) return false;
    if (outerTwo) return false;
    switch (selector) {
    case 0: {
        DRW_Line line;
        iface->addLine(line);
        break;
    }
    }
    return true;
}
bool dxfRW::priorCaseGuardPairs(int selector) {
    switch (selector) {
    case 0:
        if (priorOnly) return false;
    case 1: {
        DRW_Line line;
        iface->addLine(line);
        break;
    }
    }
    return true;
}
bool dxfRW::nonDirectBreakPairs(int selector) {
    switch (selector) {
    case 0:
        DRW_Line line;
        iface->addLine(line);
        if constexpr (stop) break;
    case 1: {
        DRW_Circle circle;
        iface->addCircle(circle);
        break;
    }
    }
    return true;
}
bool dxfRW::doBreakPairs(int selector) {
    switch (selector) {
    case 0:
        DRW_Line line;
        iface->addLine(line);
        do break; while (false);
    case 1: {
        DRW_Circle circle;
        iface->addCircle(circle);
        break;
    }
    }
    return true;
}
bool dxfRW::unbracedNestedGuardPairs() {
    if (outer) if (stop) return false;
    DRW_Line line;
    iface->addLine(line);
    return true;
}
bool dxfRW::siblingLambdaScope() {
    auto consume = [this]() {
        if (innerStop) return false;
        return true;
    };
    if (outerStop) return false;
    DRW_Line line;
    iface->addLine(line);
    return true;
}
bool dxfRW::ambiguousPair() {
    { DRW_Line value; iface->addLine(value); }
    { DRW_Circle value; iface->addCircle(value); }
    return true;
}
bool dxfRW::physicalForms() {
    DRW_ModelerGeometry geom(kind);
    DRW_Line line;
    std::unique_ptr<DRW_Surface> surface;
    iface->addModelerGeometry(geom);
    iface->addLinePtr(&line);
    iface->addSurface(surface.get());
    return true;
}
bool dxfRW::incompatiblePair() {
    DRW_Line line;
    iface->addCircle(line);
    return true;
}
""",
        "7" * 64,
    )
    pair_models = {
        name: {"id": "shared/public-model/" + name}
        for name in (
            "DRW_Line", "DRW_Circle", "DRW_RawDxfObject",
            "DRW_ModelerGeometry", "DRW_Surface",
        )
    }
    pair_callbacks = {
        "addLine": {
            "id": "shared/callback/addLine",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_Line", "parameterPassing": "reference"},
        },
        "addCircle": {
            "id": "shared/callback/addCircle",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_Circle", "parameterPassing": "reference"},
        },
        "addRawDxfObject": {
            "id": "shared/callback/addRawDxfObject",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_RawDxfObject", "parameterPassing": "reference"},
        },
        "addModelerGeometry": {
            "id": "shared/callback/addModelerGeometry",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_ModelerGeometry", "parameterPassing": "reference"},
        },
        "addLinePtr": {
            "id": "shared/callback/addLinePtr",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_Line", "parameterPassing": "pointer"},
        },
        "addSurface": {
            "id": "shared/callback/addSurface",
            "selector": {"callbackKind": "data-add", "parameterModel": "DRW_Surface", "parameterPassing": "pointer"},
        },
    }
    exact_pairs, raw_pairs = dxf_direct_publication(
        function_body(pair_source, "dxfRW::exactPairs"), pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert [(pair["model"], pair["callback"]) for pair in exact_pairs] == [
        ("DRW_Circle", "addCircle"), ("DRW_Line", "addLine")
    ]
    assert [(row["carrier"], row["callback"]) for row in raw_pairs] == [
        ("DRW_RawDxfObject", "addRawDxfObject")
    ]
    lambda_pairs, lambda_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::lambdaPair"), pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not lambda_pairs and not lambda_raw
    raw_template_pairs, raw_template_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::rawTemplatePair"), pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not raw_template_pairs and not raw_template_raw
    template_body = function_body(pair_source, "dxfRW::rawTemplatePair")
    template_bridge = {
        "templateEvidence": _source_location(template_body, template_body.line),
        "rawCallbackEvidence": _source_location(template_body, template_body.line),
        "rawBindingEvidence": _source_location(template_body, template_body.line),
    }
    template_pairs, template_raw = dxf_direct_publication(
        template_body, pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
        template_bridge,
    )
    assert [
        (
            pair["model"], pair["callback"], pair["bindingKind"],
            pair["callStyle"], pair["callbackPassingVia"],
        )
        for pair in template_pairs
    ] == [
        ("DRW_Line", "addLine", "lambda-param", "dxf-raw-captured-template",
         "dxf-raw-captured-template")
    ]
    assert [
        (row["carrier"], row["callback"], row["bindingKind"], row["callStyle"])
        for row in template_raw
    ] == [
        ("DRW_RawDxfObject", "addRawDxfObject", "template-raw-carrier",
         "dxf-raw-captured-template")
    ]
    annotate_publication_delivery_cardinality(
        "synthetic/raw-template", template_pairs, template_raw
    )
    assert (
        template_pairs[0]["conditionAncestry"] == template_raw[0]["conditionAncestry"]
        and template_pairs[0]["deliveryBundle"] == template_raw[0]["deliveryBundle"]
    )
    debug_pairs, debug_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::debugLambdaPair"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
        template_bridge,
    )
    assert not debug_pairs and not debug_raw
    conditional_template_pairs, conditional_template_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::conditionalTemplatePair"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
        template_bridge,
    )
    assert not conditional_template_pairs and not conditional_template_raw
    nested_template_pairs, nested_template_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::nestedLambdaTemplatePair"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
        template_bridge,
    )
    assert not nested_template_pairs and not nested_template_raw
    switch_pairs, switch_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::switchPairs"), pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not switch_raw
    assert {
        pair["callback"]: tuple(
            context["branchKind"] for context in pair["conditionAncestry"]
        )
        for pair in switch_pairs
    } == {
        "addLine": ("case-fallthrough",),
        "addCircle": ("case-break",),
    }
    annotate_publication_delivery_cardinality("synthetic/switch", switch_pairs, switch_raw)
    assert all(
        pair["deliveryCardinality"] == "conditional-or-repeated-unresolved"
        and pair.get("alternativeBranchGroup")
        for pair in switch_pairs
    )
    conditional_break_pairs, conditional_break_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::conditionalBreakPairs"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not conditional_break_raw
    conditional_line = next(
        pair for pair in conditional_break_pairs if pair["callback"] == "addLine"
    )
    assert any(
        context["branchKind"] == "case-unresolved"
        for context in conditional_line["conditionAncestry"]
    )
    outer_guard_pairs, outer_guard_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::outerGuardCasePairs"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not outer_guard_raw and {
        sha256_text("outerOne"), sha256_text("outerTwo")
    } <= {
        context["conditionFingerprint"]
        for context in outer_guard_pairs[0]["conditionAncestry"]
    }
    prior_case_pairs, prior_case_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::priorCaseGuardPairs"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not prior_case_raw and sha256_text("priorOnly") not in {
        context["conditionFingerprint"]
        for context in prior_case_pairs[0]["conditionAncestry"]
    }
    non_direct_break_pairs, non_direct_break_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::nonDirectBreakPairs"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not non_direct_break_raw
    non_direct_line = next(
        pair for pair in non_direct_break_pairs if pair["callback"] == "addLine"
    )
    assert any(
        context["branchKind"] == "case-unresolved"
        for context in non_direct_line["conditionAncestry"]
    )
    do_break_pairs, do_break_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::doBreakPairs"),
        pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert not do_break_raw
    do_break_line = next(pair for pair in do_break_pairs if pair["callback"] == "addLine")
    assert any(
        context["branchKind"] == "case-unresolved"
        for context in do_break_line["conditionAncestry"]
    )
    nested_guard_body = function_body(pair_source, "dxfRW::unbracedNestedGuardPairs")
    nested_guard_contexts = preceding_terminating_guard_ancestry(
        nested_guard_body, nested_guard_body.code.index("iface->addLine")
    )
    assert not nested_guard_contexts
    scope_body = function_body(pair_source, "dxfRW::siblingLambdaScope")
    scope_guards = preceding_terminating_guard_ancestry(
        scope_body, scope_body.code.index("iface->addLine")
    )
    assert [guard["conditionFingerprint"] for guard in scope_guards] == [
        sha256_text("outerStop")
    ]
    ambiguous_pairs, ambiguous_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::ambiguousPair"), pair_models, pair_callbacks, {},
        {"DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None},
    )
    assert [(pair["model"], pair["callback"]) for pair in ambiguous_pairs] == [
        ("DRW_Circle", "addCircle"), ("DRW_Line", "addLine")
    ] and not ambiguous_raw
    physical_pairs, physical_raw = dxf_direct_publication(
        function_body(pair_source, "dxfRW::physicalForms"), pair_models, pair_callbacks, {},
        {
            "DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None,
            "DRW_ModelerGeometry": None, "DRW_Surface": None,
        },
    )
    assert [
        (pair["model"], pair["callback"], pair["argumentPassing"], pair["bindingKind"])
        for pair in physical_pairs
    ] == [
        ("DRW_Line", "addLinePtr", "pointer", "local"),
        ("DRW_ModelerGeometry", "addModelerGeometry", "object", "local"),
        ("DRW_Surface", "addSurface", "pointer", "smart-pointer-local"),
    ] and not physical_raw
    try:
        dxf_direct_publication(
            function_body(pair_source, "dxfRW::incompatiblePair"), pair_models, pair_callbacks, {},
            {
                "DRW_Line": None, "DRW_Circle": None, "DRW_RawDxfObject": None,
                "DRW_ModelerGeometry": None, "DRW_Surface": None,
            },
        )
    except RouteError:
        pass
    else:
        raise AssertionError("incompatible direct callback pair was accepted")

    raw_route_source = SourceFile(
        "src/raw-routes.cpp",
        """
bool dxfRW::processRawObject() {
    DRW_RawDxfObject obj;
    iface->addRawDxfObject(obj);
    return true;
}
bool dxfRW::processRawEntity() {
    DRW_RawDxfObject ent;
    iface->addRawDxfEntity(ent);
    return true;
}
bool dxfRW::processRawDxfSection() {
    DRW_RawDxfSection section;
    iface->addRawDxfSection(section);
    return true;
}
bool dxfRW::processRawCapturedObject() {
    DRW_RawDxfObject raw;
    iface->addRawDxfObject(raw);
    return true;
}
bool dxfRW::processProxyEntity() {
    DRW_ProxyEntity entity;
    DRW_RawDxfObject raw;
    iface->addProxyEntity(entity);
    iface->addRawDxfEntity(raw);
    return true;
}
bool dxfRW::processProxyObject() {
    DRW_ProxyObject object;
    DRW_RawDxfObject raw;
    iface->addProxyObject(object);
    iface->addRawDxfObject(raw);
    return true;
}
""",
        "a" * 64,
    )
    raw_models = {
        name: {"id": "shared/public-model/" + name}
        for name in (
            "DRW_RawDxfObject", "DRW_RawDxfSection", "DRW_ProxyEntity", "DRW_ProxyObject",
        )
    }
    raw_callbacks = {
        name: {
            "id": "shared/callback/" + name,
            "selector": {
                "parameterModel": model,
                "parameterPassing": "reference",
            },
        }
        for name, model in (
            ("addRawDxfObject", "DRW_RawDxfObject"),
            ("addRawDxfEntity", "DRW_RawDxfObject"),
            ("addRawDxfSection", "DRW_RawDxfSection"),
            ("addProxyEntity", "DRW_ProxyEntity"),
            ("addProxyObject", "DRW_ProxyObject"),
        )
    }
    raw_parents = {name: None for name in raw_models}
    raw_predecessors = ["dxfRW/raw-flow/synthetic-predecessor"]
    raw_route_source_tree = SourceTree(
        "target", {raw_route_source.path: raw_route_source}, {}
    )
    for node_name, rule in RAW_DXF_SINGLE_PUBLICATION_RULES.items():
        body = function_body(raw_route_source, rule[0])
        rows = raw_route_publication_metadata(
            node_name, [body], raw_callbacks, raw_models, raw_parents, raw_predecessors
        )
        assert len(rows) == 1 and rows[0]["callback"] == rule[3]
        assert rows[0]["rawFlowPredecessorRouteIds"] == raw_predecessors
    raw_object_body = function_body(raw_route_source, "dxfRW::processRawObject")
    raw_object_route = {
        "id": "dxfRW/raw-flow/dxf-publish-object",
        "facade": "dxfRW",
        "selector": {
            "name": "dxf-publish-object",
            "phase": "publication",
            "inputRouteIds": list(raw_predecessors),
            "rawPublicationEvidence": raw_route_publication_metadata(
                "dxf-publish-object", [raw_object_body], raw_callbacks, raw_models,
                raw_parents, raw_predecessors,
            ),
        },
    }
    raw_model_ids = {name: row["id"] for name, row in raw_models.items()}
    raw_callback_ids = {name: row["id"] for name, row in raw_callbacks.items()}
    validate_raw_route_publication_metadata(
        raw_route_source_tree, raw_object_route, set(raw_predecessors),
        raw_model_ids, raw_callback_ids,
    )
    bad_predecessor_route = {
        **raw_object_route,
        "selector": {
            **raw_object_route["selector"],
            "inputRouteIds": [],
        },
    }
    try:
        validate_raw_route_publication_metadata(
            raw_route_source_tree, bad_predecessor_route, set(raw_predecessors),
            raw_model_ids, raw_callback_ids,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("raw-flow predecessor omission was accepted")
    missing_carrier_source = SourceFile(
        raw_route_source.path,
        raw_route_source.text.replace("DRW_RawDxfObject obj;", "DRW_RawDxfObject omitted;"),
        raw_route_source.git_blob,
    )
    try:
        raw_route_publication_metadata(
            "dxf-publish-object",
            [function_body(missing_carrier_source, "dxfRW::processRawObject")],
            raw_callbacks, raw_models, raw_parents, raw_predecessors,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("raw carrier omission was accepted")
    eligibility_source = SourceFile(
        "src/raw-eligibility.cpp",
        """
bool dxfRW::probe() {
    while (reader->readRec(&code)) {
        captureRawGroup(obj, code, true);
    }
}
""",
        "b" * 64,
    )
    eligibility_body = function_body(eligibility_source, "dxfRW::probe")
    eligibility_specs = (
        {
            "name": "record-loop", "pattern": r"\bwhile\s*\(\s*reader\s*->\s*readRec",
            "callee": "reader->readRec", "calleeOverload": "dxfReader::readRec(int*)",
            "relation": "capture-loop",
        },
        {
            "name": "raw-capture", "pattern": r"\bcaptureRawGroup\s*\(",
            "callee": "dxfRW::captureRawGroup", "calleeOverload": "captureRawGroup(DRW_RawDxfObject&,int,bool)",
            "relation": "capture",
        },
    )
    eligibility_rows = raw_eligibility_edge_rows(eligibility_body, eligibility_specs)
    assert [row["edge"] for row in eligibility_rows] == ["record-loop", "raw-capture"]
    reversed_eligibility = SourceFile(
        eligibility_source.path,
        eligibility_source.text.replace(
            "while (reader->readRec(&code)) {\n        captureRawGroup(obj, code, true);",
            "captureRawGroup(obj, code, true);\n    while (reader->readRec(&code)) {",
        ),
        eligibility_source.git_blob,
    )
    try:
        raw_eligibility_edge_rows(
            function_body(reversed_eligibility, "dxfRW::probe"), eligibility_specs
        )
    except RouteError:
        pass
    else:
        raise AssertionError("raw eligibility order inversion was accepted")
    dwg_replay_specs = DWG_RAW_REPLAY_EVIDENCE_RULES["dwg-replay-object"][
        "src/intern/dwgwriter15.cpp"
    ]["dwgWriter15::replayRawObject"]
    dwg_replay_source = SourceFile(
        "src/intern/dwgwriter15.cpp",
        """
bool dwgWriter15::replayRawObject(const DRW_UnsupportedObject& object) {
    if (blockControlEmitted() && object.m_isEntity) return false;
    if (object.m_version != m_version) return false;
    if (object.m_isEntity && !isReplayableFixedModelerRawEntity(object)
        && !isReplayableFixedEntityShellRawEntity(object)
        && !isReplayableSurfaceRawEntity(object)
        && !isReplayableCustomRawEntity(object)) return false;
    if (object.m_handle == 0 || object.m_rawBytes.empty()) return false;
    if (static_cast<std::uint64_t>(object.m_bodyBitSize)
        > static_cast<std::uint64_t>(object.m_rawBytes.size()) * 8u) return false;
    if (object.m_objectSize != 0) return false;
    if (entry.first == object.m_handle) return false;
    if (object.m_blockOwnerHandle != DRW::NoHandle
        && object.m_parentHandle == DRW::NoHandle) return false;
    if (replayableCustomEntity && !canRecordRawBlockOwnedEntity(object)) return false;
    if (!registerRawObjectClass(object)) return false;
    if (m_currentDwgObjectFrameProvenance.classNumber != writerType) return false;
    if (!readRawObjectHandle(*bodyBytes, m_version, encodedHandle)) return false;
    if (!typeReader.isGood() || actualType != writerType) return false;
    if (replayableCustomEntity && !recordRawBlockOwnedEntity(object)) return false;
    return true;
}
""",
        "d" * 64,
    )
    dwg_replay_body = function_body(
        dwg_replay_source, "dwgWriter15::replayRawObject"
    )
    dwg_replay_rows = raw_eligibility_edge_rows(dwg_replay_body, dwg_replay_specs)
    assert [row["edge"] for row in dwg_replay_rows] == [
        spec["name"] for spec in dwg_replay_specs
    ]
    reversed_dwg_replay = SourceFile(
        dwg_replay_source.path,
        dwg_replay_source.text.replace(
            "if (blockControlEmitted() && object.m_isEntity) return false;\n    if (object.m_version != m_version)",
            "if (object.m_version != m_version) return false;\n    if (blockControlEmitted() && object.m_isEntity)",
        ),
        dwg_replay_source.git_blob,
    )
    try:
        raw_eligibility_edge_rows(
            function_body(reversed_dwg_replay, "dwgWriter15::replayRawObject"),
            dwg_replay_specs,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG raw replay guard order inversion was accepted")
    dwg_section_specs = DWG_RAW_REPLAY_EVIDENCE_RULES["dwg-replay-section"][
        "src/intern/dwgwriter18.cpp"
    ]["dwgWriter18::finalize"]
    dwg_section_source = SourceFile(
        "src/intern/dwgwriter18.cpp",
        """
bool dwgWriter18::finalize() {
    if (objectWriteFailed() || m_stream == nullptr || !m_stream->good()) return false;
    for (const DRW_RawDwgSection& section : m_rawDwgSections) {
        appendDataSection(section.m_name, nullptr, 0);
    }
    return true;
}
""",
        "e" * 64,
    )
    dwg_section_body = function_body(dwg_section_source, "dwgWriter18::finalize")
    dwg_section_rows = raw_eligibility_edge_rows(dwg_section_body, dwg_section_specs)
    assert [row["edge"] for row in dwg_section_rows] == [
        spec["name"] for spec in dwg_section_specs
    ]
    reversed_dwg_section = SourceFile(
        dwg_section_source.path,
        dwg_section_source.text.replace(
            "for (const DRW_RawDwgSection& section : m_rawDwgSections) {\n        appendDataSection(section.m_name, nullptr, 0);",
            "appendDataSection(section.m_name, nullptr, 0);\n    for (const DRW_RawDwgSection& section : m_rawDwgSections) {",
        ),
        dwg_section_source.git_blob,
    )
    try:
        raw_eligibility_edge_rows(
            function_body(reversed_dwg_section, "dwgWriter18::finalize"),
            dwg_section_specs,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG raw section finalizer order inversion was accepted")
    dwg_table_source = SourceFile(
        "src/intern/dwgreader.cpp",
        """
bool dwgReader::readDwgTables(DRW_Header&) {
    parseControl(oc, kBlockTable, blockControl);
    claimControlHandles(blockControl);
    stageControlReceipt(oc, kBlockTable, blockControl);
    parseTableRecord(oc, kBlockTable, br);
    insertTableRecord(blockRecordmap, std::move(br), "block record", oc.handle,
                      kBlockTable.recordType);
    return true;
}
""",
        "f" * 64,
    )
    dwg_table_tree = SourceTree(
        "target", {dwg_table_source.path: dwg_table_source}, {}
    )
    table_evidence = dwg_table_delivery_metadata(dwg_table_tree, "kBlockTable")
    assert [row["edge"] for row in table_evidence[0]["edges"]] == [
        "control-parse", "control-handle-claim", "control-receipt-stage",
        "record-parse", "typed-map-insert",
    ]
    reversed_table_source = SourceFile(
        dwg_table_source.path,
        dwg_table_source.text.replace(
            "claimControlHandles(blockControl);\n    stageControlReceipt",
            "stageControlReceipt(oc, kBlockTable, blockControl);\n    claimControlHandles(blockControl);\n    stageControlReceipt",
        ).replace(
            "stageControlReceipt(oc, kBlockTable, blockControl);\n    parseTableRecord",
            "parseTableRecord(oc, kBlockTable, br);\n    stageControlReceipt(oc, kBlockTable, blockControl);\n    parseTableRecord",
        ),
        dwg_table_source.git_blob,
    )
    try:
        dwg_table_delivery_metadata(
            SourceTree("target", {reversed_table_source.path: reversed_table_source}, {}),
            "kBlockTable",
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG table delivery order inversion was accepted")
    dwg_delivery_specs = DWG_COMPOUND_DELIVERY_ANCHORS[0][2]
    dwg_delivery_source = SourceFile(
        "src/intern/dwgreader.cpp",
        """
bool dwgReader::readMappedDwgEntity() {
    if (completion == DwgMappedEntityCompletion::Journal && (output == nullptr)) return false;
    if (completion == DwgMappedEntityCompletion::Immediate && (output != nullptr)) return false;
    ActiveBlockJournalScope activeBlockJournalScope;
    DwgEntityOutput &entityOutput = output != nullptr ? *output : immediate;
    readDwgEntityWithOutput(dbuf, lease.object, intfa, entityOutput, frameFailure, offsetSpace);
    else if (completion == DwgMappedEntityCompletion::Journal && outcome == DwgMappedEntityOutcome::PublishedSimple) return false;
    outcome = DwgMappedEntityOutcome::JournalledSimple;
    case DwgMappedEntityOutcome::JournalledSimple:
    case DwgMappedEntityOutcome::StagedCompound:
    case DwgMappedEntityOutcome::CommittedCompound:
    case DwgMappedEntityOutcome::DeferredObject:
    return true;
}
""",
        "g" * 64,
    )
    dwg_delivery_body = function_body(
        dwg_delivery_source, "dwgReader::readMappedDwgEntity"
    )
    dwg_delivery_rows = raw_eligibility_edge_rows(
        dwg_delivery_body, dwg_delivery_specs
    )
    assert [row["edge"] for row in dwg_delivery_rows] == [
        spec["name"] for spec in dwg_delivery_specs
    ]
    reversed_dwg_delivery = SourceFile(
        dwg_delivery_source.path,
        dwg_delivery_source.text.replace(
            "ActiveBlockJournalScope activeBlockJournalScope;\n    DwgEntityOutput",
            "DwgEntityOutput &entityOutput = output != nullptr ? *output : immediate;\n    ActiveBlockJournalScope activeBlockJournalScope;\n    DwgEntityOutput",
        ),
        dwg_delivery_source.git_blob,
    )
    try:
        raw_eligibility_edge_rows(
            function_body(reversed_dwg_delivery, "dwgReader::readMappedDwgEntity"),
            dwg_delivery_specs,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG direct/journal delivery order inversion was accepted")
    dwg_lifecycle_source = SourceFile(
        "src/libdwgr.cpp",
        """
bool dwgRW::processDwg() {
try {
    reader->beginDwgClassCoverage();
    ret = reader->readDwgHeader(hdr);
    if (!ret) { error = DRW::BAD_READ_HEADER; }
    ret2 = reader->readDwgClasses();
    if (ret && !ret2) { error = DRW::BAD_READ_CLASSES; ret = ret2; }
    ret2 = reader->readDwgHandles();
    if (ret && !ret2) { error = DRW::BAD_READ_HANDLES; ret = ret2; }
    ret2 = ret && reader->readDwgTables(hdr);
    if (ret && !ret2) { error = DRW::BAD_READ_TABLES; ret = ret2; }
    iface->addHeader(&hdr);
    for (auto it=reader->ltypemap.begin(); it!=reader->ltypemap.end(); ++it) {
        DRW_LType *lt = it->second; iface->addLType(const_cast<DRW_LType&>(*lt));
    }
    for (auto it=reader->layermap.begin(); it!=reader->layermap.end(); ++it) {
        DRW_Layer *ly = it->second; iface->addLayer(const_cast<DRW_Layer&>(*ly));
    }
    for (auto it=reader->stylemap.begin(); it!=reader->stylemap.end(); ++it) {
        DRW_Textstyle *ly = it->second; iface->addTextStyle(const_cast<DRW_Textstyle&>(*ly));
    }
    for (auto it=reader->dimstylemap.begin(); it!=reader->dimstylemap.end(); ++it) {
        DRW_Dimstyle *ly = it->second; iface->addDimStyle(const_cast<DRW_Dimstyle&>(*ly));
    }
    for (auto it=reader->vportmap.begin(); it!=reader->vportmap.end(); ++it) {
        DRW_Vport *ly = it->second; iface->addVport(const_cast<DRW_Vport&>(*ly));
    }
    for (auto it=reader->appIdmap.begin(); it!=reader->appIdmap.end(); ++it) {
        DRW_AppId *ly = it->second; iface->addAppId(const_cast<DRW_AppId&>(*ly));
    }
    for (auto it=reader->viewmap.begin(); it!=reader->viewmap.end(); ++it) {
        DRW_View *vw = it->second; iface->addView(const_cast<DRW_View&>(*vw));
    }
    for (auto it=reader->ucsmap.begin(); it!=reader->ucsmap.end(); ++it) {
        DRW_UCS *u = it->second; iface->addUCS(const_cast<DRW_UCS&>(*u));
    }
    reader->publishDeferredTableFramePublications(*iface);
    ret2 = reader->readDwgBlocks(*iface);
    if (ret && !ret2) { error = DRW::BAD_READ_BLOCKS; ret = ret2; }
    ret2 = reader->readDwgEntities(*iface);
    if (ret && !ret2) { error = DRW::BAD_READ_ENTITIES; ret = ret2; }
    ret2 = reader->readDwgObjects(*iface);
    if (ret && !ret2) { error = DRW::BAD_READ_OBJECTS; ret = ret2; }
    for (const DRW_RawDwgSection& section : reader->m_rawDwgSections)
        iface->addRawDwgSection(section);
    for (const DRW_DataStorageSection& storage : reader->m_dataStorageSections)
        iface->addDataStorage(storage);
    finalizeClassCoverage(); finalizeCoverage(ret);
    return ret;
} catch (...) {
    finalizeClassCoverage(); finalizeCoverage(false);
    return false;
}
}
""",
        "h" * 64,
    )
    lifecycle_tree = SourceTree(
        "target", {dwg_lifecycle_source.path: dwg_lifecycle_source}, {}
    )
    lifecycle_evidence = dwg_reader_lifecycle_metadata(lifecycle_tree)
    assert [row["edge"] for row in lifecycle_evidence[0]["edges"]] == [
        spec["name"] for spec in DWG_READER_LIFECYCLE_RULES
    ]
    reversed_lifecycle_source = SourceFile(
        dwg_lifecycle_source.path,
        dwg_lifecycle_source.text.replace(
            "ret2 = reader->readDwgClasses();\n    if (ret && !ret2) { error = DRW::BAD_READ_CLASSES; ret = ret2; }\n    ret2 = reader->readDwgHandles();",
            "ret2 = reader->readDwgHandles();\n    if (ret && !ret2) { error = DRW::BAD_READ_HANDLES; ret = ret2; }\n    ret2 = reader->readDwgClasses();",
        ),
        dwg_lifecycle_source.sha256,
    )
    try:
        dwg_reader_lifecycle_metadata(
            SourceTree("target", {reversed_lifecycle_source.path: reversed_lifecycle_source}, {})
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG reader lifecycle order inversion was accepted")
    dwg_ownership_source = SourceFile(
        "src/intern/dwgreader.cpp",
        """
bool dwgReader::readDwgBlocks(DRW_Interface &intfa, dwgBuffer *dbuf) {
    m_consumedCompoundChildHandles.clear();
    for (const auto &item : blockRecordmap) {
        const DRW_Block_Record *record = item.second;
        claimHandle(record, record->block);
        claimHandle(record, record->endBlock);
        for (const std::uint32_t handle : record->entMap)
            claimHandle(record, handle);
    }
    if (!invalidOwnershipRecords.empty()) {
        quarantineOwnedEntities(*record);
    }
    if (version >= DRW::AC1018) {
        if (!preflightMappedPolylineOwnership(records, dbuf)) return false;
    }
    for (auto it = blockRecordmap.begin(); it != blockRecordmap.end(); ++it) {
        auto mit = ObjectMap.find(bkr->block);
        if (!frame.readAt(*dbuf, version, oc.loc)) return false;
        if (typeBuffer.getObjType(version) != dwgType::BLOCK || !typeBuffer.isGood()) return false;
        if (!parseBlock(bk, buff, frame.bodyBitSize())) return false;
        if (bk.handle != oc.handle || bk.handle != bkr->block) return false;
        if (bkr->entMap.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) return false;
        if (classification.route != DwgFrameClassification::Route::Entity) { validOwnership = false; }
        auto endIt = ObjectMap.find(bkr->endBlock);
        if (!endFrame.readAt(*dbuf, version, oc.loc)) return false;
        if (endTypeBuffer.getObjType(version) != dwgType::ENDBLK || !endTypeBuffer.isGood()) return false;
        if (!parseBlock(end, buff1, endFrame.bodyBitSize())) return false;
        if (end.handle != oc.handle || end.handle != bkr->endBlock) return false;
        if (bk.parentHandle != end.parentHandle) return false;
        const bool isSpaceBlockRecord = isSpaceBlockRecordName(bk.name);
        const bool validDelimiterOwner = isSpaceBlockRecord ? bk.parentHandle == DRW::NoHandle : bk.parentHandle == bkr->handle;
        makeTypedEntityFramePublication(version, blockObject, dwgType::BLOCK, bk);
        bool journalEligible = version >= DRW::AC1018;
        intfa.addBlock(bk);
        walkBlockRecordEntities(bkr, dbuf, intfa, bk.parentHandle, bkr->handle);
        intfa.endBlock();
        walkBlockRecordEntities(bkr, dbuf, intfa, DRW::NoHandle, bkr->handle);
        if (!commitDelimiter(blockObject.handle, blockLease) || !commitDelimiter(endBlockObject.handle, endBlockLease)) return false;
        if (!publishDwgFramePublication(intfa, blockPublication) || !publishDwgFramePublication(intfa, endBlockPublication)) return false;
        if (blockScopeFailure) {
            quarantineOwnedEntities(*bkr);
        }
    }
    return ret;
}
""",
        "i" * 64,
    )
    ownership_tree = SourceTree(
        "target", {dwg_ownership_source.path: dwg_ownership_source}, {}
    )
    ownership_evidence = dwg_block_ownership_metadata(ownership_tree)
    assert [row["edge"] for row in ownership_evidence[0]["edges"]] == [
        spec["name"] for spec in DWG_BLOCK_OWNERSHIP_RULES
    ]
    reversed_ownership_source = SourceFile(
        dwg_ownership_source.path,
        dwg_ownership_source.text.replace(
            "claimHandle(record, record->block);\n        claimHandle(record, record->endBlock);",
            "claimHandle(record, record->endBlock);\n        claimHandle(record, record->block);",
        ),
        dwg_ownership_source.sha256,
    )
    try:
        dwg_block_ownership_metadata(
            SourceTree("target", {reversed_ownership_source.path: reversed_ownership_source}, {})
        )
    except RouteError:
        pass
    else:
        raise AssertionError("DWG BLOCK/ENDBLK ownership order inversion was accepted")
    transport_source = SourceFile(
        "src/transport.cpp",
        """
bool dxfRW::write() {
    if (binFile) {
        if (version <= DRW::AC1009)
            writer = std::make_unique<dxfWriterBinaryR12>(&filestr);
        else
            writer = std::make_unique<dxfWriterBinary>(&filestr);
    } else {
        writer = std::make_unique<dxfWriterAscii>(&filestr);
    }
}
""",
        "c" * 64,
    )
    transport_body = function_body(transport_source, "dxfRW::write")
    for selection, implementation in (
        ("write-ascii", "dxfWriterAscii"),
        ("write-binary-r12", "dxfWriterBinaryR12"),
        ("write-binary", "dxfWriterBinary"),
    ):
        evidence = transport_selection_metadata(
            transport_body, selection, implementation
        )
        assert evidence["implementation"] == implementation
    swapped_transport = SourceFile(
        transport_source.path,
        transport_source.text.replace(
            "writer = std::make_unique<dxfWriterBinaryR12>(&filestr);\n        else\n            writer = std::make_unique<dxfWriterBinary>(&filestr);",
            "writer = std::make_unique<dxfWriterBinary>(&filestr);\n        else\n            writer = std::make_unique<dxfWriterBinaryR12>(&filestr);",
        ),
        transport_source.git_blob,
    )
    try:
        transport_selection_metadata(
            function_body(swapped_transport, "dxfRW::write"),
            "write-binary-r12", "dxfWriterBinaryR12",
        )
    except RouteError:
        pass
    else:
        raise AssertionError("transport branch inversion was accepted")
    proxy_rows = raw_route_publication_metadata(
        "dxf-publish-proxy",
        [
            function_body(raw_route_source, "dxfRW::processProxyEntity"),
            function_body(raw_route_source, "dxfRW::processProxyObject"),
        ],
        raw_callbacks,
        raw_models,
        raw_parents,
        raw_predecessors,
    )
    assert {
        row["callEvidence"]["symbol"] for row in proxy_rows
    } == set(RAW_DXF_PROXY_PUBLICATION_RULES)
    assert all(
        row["typedToRawRelation"] == "typed-callback-before-raw-carrier"
        and row["typedPredecessor"]["callEvidence"]["line"] < row["callEvidence"]["line"]
        for row in proxy_rows
    )
    bad_proxy_source = SourceFile(
        raw_route_source.path,
        raw_route_source.text.replace("iface->addRawDxfEntity(raw);", ""),
        raw_route_source.git_blob,
    )
    try:
        raw_route_publication_metadata(
            "dxf-publish-proxy",
            [
                function_body(bad_proxy_source, "dxfRW::processProxyEntity"),
                function_body(bad_proxy_source, "dxfRW::processProxyObject"),
            ],
            raw_callbacks,
            raw_models,
            raw_parents,
            raw_predecessors,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("proxy raw carrier omission was accepted")
    reversed_proxy_source = SourceFile(
        raw_route_source.path,
        raw_route_source.text.replace(
            "iface->addProxyEntity(entity);\n    iface->addRawDxfEntity(raw);",
            "iface->addRawDxfEntity(raw);\n    iface->addProxyEntity(entity);",
        ),
        raw_route_source.git_blob,
    )
    try:
        raw_route_publication_metadata(
            "dxf-publish-proxy",
            [
                function_body(reversed_proxy_source, "dxfRW::processProxyEntity"),
                function_body(reversed_proxy_source, "dxfRW::processProxyObject"),
            ],
            raw_callbacks,
            raw_models,
            raw_parents,
            raw_predecessors,
        )
    except RouteError:
        pass
    else:
        raise AssertionError("proxy typed/raw order inversion was accepted")

    dwg_pair_source = SourceFile(
        "src/dwg-pairs.cpp",
        """
bool dwgReader::helperPairs() {
    DRW_Line line;
    DRW_UnsupportedObject raw;
    output.appendValue(line, &DRW_Interface::addLinePtr);
    emitWithExtrusion(line, output, &DRW_Interface::addLinePtr);
    output.appendValue(raw, &DRW_Interface::addUnsupportedObject);
    intfa.addUnsupportedObject(makeRawObject(1, 2));
    return true;
}
""",
        "8" * 64,
    )
    dwg_pairs, dwg_raw = dwg_direct_publication(
        function_body(dwg_pair_source, "dwgReader::helperPairs"),
        {
            "DRW_Line": pair_models["DRW_Line"],
            "DRW_UnsupportedObject": {"id": "shared/public-model/DRW_UnsupportedObject"},
        },
        {
            "addLinePtr": pair_callbacks["addLinePtr"],
            "addUnsupportedObject": {
                "id": "shared/callback/addUnsupportedObject",
                "selector": {"callbackKind": "data-add", "parameterModel": "DRW_UnsupportedObject", "parameterPassing": "reference"},
            },
        },
        {"DRW_Line": None, "DRW_UnsupportedObject": None},
    )
    assert [
        (pair["callStyle"], pair["callbackPassingVia"], pair["sourceArgumentPassing"], pair["argumentPassing"])
        for pair in dwg_pairs
    ] == [
        ("dwg-output-append", "dwg-output-append", "object", "pointer"),
        ("dwg-emit-with-extrusion", "dwg-emit-with-extrusion", "object", "pointer"),
    ]
    assert [
        (row["callStyle"], row["callbackPassingVia"], row["sourceArgumentPassing"], row["argumentPassing"])
        for row in dwg_raw
    ] == [
        ("dwg-output-append", "dwg-output-append", "object", "object"),
        ("interface-call", "direct", "object", "object"),
    ]

    named_pair_source = SourceFile(
        "src/named-dwg-pairs.cpp",
        """
bool dwgReader::namedPairs(int oType) {
    switch (oType) {
    default:
        if (oType >= 500) {
            if (cit && cit->className == "ACME") {
                DRW_Line e;
                if (ret) {
                    output.appendValue(e, &DRW_Interface::addLinePtr);
                }
            }
        }
        break;
    }
    return true;
}
""",
        "9" * 64,
    )
    named_body = function_body(named_pair_source, "dwgReader::namedPairs")
    named_dispatch = named_class_dispatches(named_body)
    assert len(named_dispatch) == 1 and named_dispatch[0]["name"] == "ACME"
    named_branches = dwg_named_branch_bodies(
        named_body, "ACME", {named_dispatch[0]["line"]}
    )
    assert len(named_branches) == 1
    named_pairs, named_raw = dwg_direct_publication(
        named_branches[0].body,
        {"DRW_Line": pair_models["DRW_Line"]},
        {"addLinePtr": pair_callbacks["addLinePtr"]},
        {"DRW_Line": None},
        named_branches[0].dispatch_ancestry,
    )
    assert not named_raw and len(named_pairs) == 1
    named_ancestry = named_pairs[0]["conditionAncestry"]
    assert {
        sha256_text("oType >= 500"),
        named_dispatch[0]["conditionFingerprint"],
        sha256_text("ret"),
    } <= {context["conditionFingerprint"] for context in named_ancestry}
    named_source_route = {
        "category": "named-object-class",
        "selector": {
            "conditionSelectors": [
                {
                    "line": named_dispatch[0]["line"],
                    "conditionFingerprint": named_dispatch[0]["conditionFingerprint"],
                }
            ]
        },
    }
    validate_named_publication_selector(
        named_source_route, named_ancestry, "synthetic/named"
    )
    try:
        validate_named_publication_selector(
            {
                "category": "named-object-class",
                "selector": {
                    "conditionSelectors": [
                        {"line": named_dispatch[0]["line"], "conditionFingerprint": "wrong"}
                    ]
                },
            },
            named_ancestry,
            "synthetic/named",
        )
    except RouteError:
        pass
    else:
        raise AssertionError("named selector mismatch was accepted")

    cmake_headers = parse_cmake_public_headers(
        """
set(LIBDXFRW_PUBLIC_HEADERS
    \"${CMAKE_CURRENT_LIST_DIR}/src/one.h\"
    \"${CMAKE_CURRENT_LIST_DIR}/src/two.h\"
)
""",
        "synthetic.cmake",
    )
    assert cmake_headers == ("src/one.h", "src/two.h")
    for invalid in (
        "set(LIBDXFRW_PUBLIC_HEADERS \"${CMAKE_CURRENT_LIST_DIR}/src/one.h\" \"${CMAKE_CURRENT_LIST_DIR}/src/one.h\")",
        "set(LIBDXFRW_PUBLIC_HEADERS ${OTHER}/src/one.h)",
        "set(LIBDXFRW_PRIVATE_HEADERS \"${CMAKE_CURRENT_LIST_DIR}/src/one.h\")",
    ):
        try:
            parse_cmake_public_headers(invalid, "invalid.cmake")
        except RouteError:
            pass
        else:
            raise AssertionError("invalid public-header CMake input was accepted")

    header = SourceFile(
        "src/public.h",
        """
#define FAKE_DECL class Fake { public: void fake() {} };
inline int freeValue(int value) { return value; }
namespace Api {
struct Base {};
class Widget {
public:
    struct Exposed { int field {0}; };
    enum class Mode : unsigned { First = 7, Second };
    using Alias = Base;
    using Base::operator=;
    Widget() = default;
    ~Widget() = default;
    int value() const noexcept { return 1; }
    explicit operator bool() const { return true; }
private:
    struct Hidden {};
    void hidden() { auto lambda = [] { class Local {}; }; }
};
} // namespace Api
""",
        "1" * 64,
    )
    public_collector = RouteCollector()
    add_public_header_routes(
        public_collector,
        SourceTree("target", {"src/public.h": header}, {}),
        ("src/public.h",),
    )
    public_routes = public_collector.routes()
    public_names = {
        route["selector"].get("qualifiedName")
        for route in public_routes
        if route["category"] == "public-type"
    }
    assert "Api::Widget" in public_names and "Api::Widget::Exposed" in public_names
    assert "Api::Widget::Hidden" not in public_names and "Fake" not in public_names
    assert any(
        route["category"] == "public-enum"
        and route["selector"]["qualifiedName"] == "Api::Widget::Mode"
        and route["selector"]["values"][0]["initializerFingerprint"] is not None
        for route in public_routes
    )
    assert any(
        route["category"] == "public-alias"
        and route["selector"]["qualifiedName"] == "Api::Widget::Alias"
        for route in public_routes
    )
    assert any(route["category"] == "public-using-declaration" for route in public_routes)
    inline_names = {
        route["selector"]["qualifiedName"]
        for route in public_routes
        if route["category"] == "public-inline-method"
    }
    assert "Api::Widget::value" in inline_names, inline_names
    assert "Api::Widget::operator bool" in inline_names, inline_names
    assert any(
        route["category"] == "public-inline-function"
        and route["selector"]["qualifiedName"] == "freeValue"
        for route in public_routes
    )

    assert len(SOURCE_UNIT_ROLES) == 85
    try:
        source_unit_roles_for(SourceTree("target", {"src/unreviewed.h": header}, {}))
    except RouteError:
        pass
    else:
        raise AssertionError("unreviewed target source-unit path was accepted")

    target_legacy = SourceTree(
        "target",
        {"src/libdwgr.h": SourceFile("src/libdwgr.h", "using dwgR = dwgRW;", "2" * 64)},
        {},
    )
    target_legacy_collector = RouteCollector()
    add_legacy_dwgr_route(target_legacy_collector, target_legacy)
    assert target_legacy_collector.routes()[0]["selector"]["representation"] == "alias"
    forwards = "\n".join(
        "bool method%d() { return implementation.method%d(); }" % (index, index)
        for index in range(10)
    )
    standalone_legacy = SourceTree(
        "standalone",
        {
            "src/libdwgr.h": SourceFile(
                "src/libdwgr.h",
                "class dwgR { public: %s private: dwgRW implementation; };" % forwards,
                "3" * 64,
            )
        },
        {},
    )
    standalone_legacy_collector = RouteCollector()
    add_legacy_dwgr_route(standalone_legacy_collector, standalone_legacy)
    assert standalone_legacy_collector.routes()[0]["selector"]["representation"] == "composition-wrapper"
    broken_legacy = SourceTree(
        "standalone",
        {
            "src/libdwgr.h": SourceFile(
                "src/libdwgr.h",
                "class dwgR { public: %s private: dwgRW implementation; };" % "\n".join(forwards.splitlines()[:-1]),
                "4" * 64,
            )
        },
        {},
    )
    try:
        add_legacy_dwgr_route(RouteCollector(), broken_legacy)
    except RouteError:
        pass
    else:
        raise AssertionError("broken dwgR forwarding surface was accepted")
    print("extract_parity_source_routes self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path("."))
    parser.add_argument("--target-repo", type=Path)
    parser.add_argument("--output", type=Path, default=Path("metadata/parity-source-routes-v1.json"))
    parser.add_argument("--check", action="store_true", help="compare the deterministic output without writing")
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.target_repo is None:
            parser.error("--target-repo is required unless --self-test is used")
        root = args.root.resolve()
        output = args.output if args.output.is_absolute() else root / args.output
        write_or_check_artifacts(generate(root, args.target_repo.resolve()), output, args.check)
        return 0
    except (OSError, UnicodeError, ValueError, RouteError, AssertionError) as exc:
        print("parity source route inventory: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
