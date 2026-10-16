#!/usr/bin/env python3
"""Check local VisualStyle vector descriptors against the pinned candidate map.

This guards source/model ordering and manifest drift only. It deliberately
does not qualify candidate DXF group codes or emitted serialization behavior.
"""

import argparse
import json
import re
import sys
from pathlib import Path


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--vectors", required=True, type=Path)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    source = args.vectors.read_text(encoding="utf-8")
    slots = manifest.get("slotEvidence", {}).get("slots", [])
    if len(slots) != 58 or [slot.get("index") for slot in slots] != list(range(58)):
        fail("candidate manifest must contain the ordered 58-slot sequence")

    block = re.search(
        r"const std::array<StyleSlot, 58> kStyleSlots\{\{(.*?)\}\};",
        source,
        re.DOTALL,
    )
    if block is None:
        fail("could not find the local 58-slot StyleSlot descriptor")
    descriptors = re.findall(
        r'\{StyleField::(Long|Short|Double|Color|Bit|Text),\s*"([^"]+)"\}',
        block.group(1),
    )
    if len(descriptors) != 58:
        fail(f"local descriptor has {len(descriptors)} entries, expected 58")

    # The local synthetic vector emits the modern branch of versioned DWG
    # primitives. This is a test-vector type check, not a claim about DXF tags.
    def expected_kind(slot: dict[str, object]) -> str:
        dwg_kind = slot.get("dwg")
        if dwg_kind == "CMC":
            return "Color"
        if dwg_kind == "B":
            return "Bit"
        if dwg_kind == "T":
            return "Text"
        if dwg_kind == "BD" or dwg_kind == "BLd/BD versioned":
            return "Double"
        if slot.get("field") == "faceModifier":
            return "Short"
        return "Long"

    manifest_names = [slot.get("field") for slot in slots]
    local_names = [name for _, name in descriptors]
    if local_names != manifest_names:
        for index, (local, candidate) in enumerate(zip(local_names, manifest_names)):
            if local != candidate:
                fail(f"slot {index} destination differs: {local!r} != {candidate!r}")
        fail("local descriptor and candidate manifest differ in length/order")

    for index, ((local_kind, local_name), candidate) in enumerate(zip(descriptors, slots)):
        kind = expected_kind(candidate)
        if local_kind != kind:
            fail(f"slot {index} ({local_name}) primitive {local_kind} != {kind}")

    color_slots = [index for index, (kind, _) in enumerate(descriptors) if kind == "Color"]
    if color_slots != manifest.get("slotEvidence", {}).get("colorSlots"):
        fail("local CMC slots differ from candidate color-slot inventory")

    companion_slots = []
    for candidate in slots:
        # The generated metadata source is the available candidate for 31/33;
        # the separate spec/importer disagreement remains unresolved.
        companion_group = candidate.get(
            "peerGeneratedMetadataCompanionGroup",
            candidate.get("peerCompanionGroup"),
        )
        if companion_group == 176:
            companion_slots.append(candidate.get("index"))
    if companion_slots != list(range(58)):
        fail("candidate generated-metadata map must identify 58 group-176 companions")

    if 'version >= DRW::AC1027 ? 58u : 28u' not in source:
        fail("local literal vector no longer declares the 28/58 version boundary")
    if manifest.get("validation", {}).get("supportClaimsPromoted") is not False:
        fail("candidate manifest must not promote a support claim")
    if manifest.get("scopeBoundary", {}).get("externalItems") != {
        "J256": "DEFERRED_EXTERNAL",
        "J260": "DEFERRED_EXTERNAL",
        "J268": "DEFERRED_EXTERNAL",
        "J284": "DEFERRED_EXTERNAL",
        "J293": "DEFERRED_EXTERNAL",
        "J295": "DEFERRED_EXTERNAL",
    }:
        fail("external J statuses must remain deferred")

    print(
        "PASS: 58 candidate slots align with local names/kinds; "
        "8 CMC slots, 58 companion candidates, and 28/58 boundary retained"
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
