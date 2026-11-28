#!/usr/bin/env python3
"""Check local VisualStyle descriptors against the pinned candidate map.

This guards source/model ordering and manifest drift only. It deliberately
does not qualify candidate DXF group semantics or emitted serialization
behavior. DXF group-code value types below are only the generic Autodesk
code-range contract; they do not establish which slot a group represents.
"""

import argparse
import json
import re
import sys
from pathlib import Path


DXF_GROUP_VALUE_TYPE_RANGES = (
    (0, 9, "string"),
    (40, 59, "float64"),
    (60, 79, "int16"),
    (90, 99, "int32"),
    (170, 179, "int16"),
    (290, 299, "boolean"),
    (300, 309, "string"),
    (420, 429, "int32"),
)


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True, type=Path)
    parser.add_argument("--grades", required=True, type=Path)
    parser.add_argument("--vectors", required=True, type=Path)
    args = parser.parse_args()

    manifest = json.loads(args.manifest.read_text(encoding="utf-8"))
    grades = json.loads(args.grades.read_text(encoding="utf-8"))
    source = args.vectors.read_text(encoding="utf-8")
    slots = manifest.get("slotEvidence", {}).get("slots", [])
    if len(slots) != 58 or [slot.get("index") for slot in slots] != list(range(58)):
        fail("candidate manifest must contain the ordered 58-slot sequence")
    if grades.get("candidateMap") != args.manifest.name:
        fail("evidence grades must name the exact candidate crosswalk")

    all_indexes = set(range(58))

    def dxf_group_value_type(group: object) -> str:
        if type(group) is not int:
            fail(f"DXF candidate group must be an integer, got {group!r}")
        for first, last, value_type in DXF_GROUP_VALUE_TYPE_RANGES:
            if first <= group <= last:
                return value_type
        fail(f"DXF group {group} has no pinned value-width classification")

    # This binds each candidate source field/group to a generic DXF wire value
    # width without upgrading the unresolved R2013b slot semantics.
    candidate_group_fields = (
        "groupR2010",
        "groupR2013bCandidate",
        "peerCompanionGroup",
        "peerSpecCompanionGroup",
        "peerGeneratedMetadataCompanionGroup",
        "importerCandidateValueGroup",
        "importerCandidateCompanionGroup",
    )
    observed_group_types = set()
    for slot in slots:
        for field in candidate_group_fields:
            group = slot.get(field)
            if group is not None:
                observed_group_types.add(dxf_group_value_type(group))
    if observed_group_types != {"string", "float64", "int16", "int32", "boolean"}:
        fail("candidate VisualStyle groups no longer exercise the frozen value-width families")

    def exact_coverage(indices, expected, label):
        if (len(indices) != len(set(indices))
                or set(indices) != set(expected)):
            fail(f"{label} does not cover its exact slot set")

    def grade_for_all(axis, grade, expected=all_indexes):
        row = grades.get("fieldGrades", {}).get(axis, {})
        if row.get("grade") != grade:
            fail(f"{axis} must remain graded {grade}")
        exact_coverage(row.get("slotIndices", []), expected, axis)

    grade_for_all("sourcePrimitiveAndDestination", "empirically-captured")
    grade_for_all("sameProfileDwgNoOpReplay", "policy-qualified")

    r2010 = grades.get("fieldGrades", {}).get("r2010ValueGroup", {})
    r2010_specified = set(r2010.get("specified", []))
    r2010_unresolved = set(r2010.get("unresolved", []))
    r2010_not_applicable = set(r2010.get("notApplicable", []))
    exact_coverage(r2010.get("specified", []), set(range(28)) - {6},
                   "R2010 specified DXF groups")
    exact_coverage(r2010.get("unresolved", []), {6},
                   "R2010 unresolved DXF groups")
    exact_coverage(r2010.get("notApplicable", []), set(range(28, 58)),
                   "R2010 non-applicable extension slots")
    for index, slot in enumerate(slots):
        group = slot.get("groupR2010")
        if index in r2010_specified | r2010_unresolved:
            if group is None:
                fail(f"R2010 slot {index} lacks its specified/unresolved group candidate")
            dxf_group_value_type(group)
        elif index in r2010_not_applicable and group is not None:
            fail(f"R2010 extension slot {index} must remain not applicable")

    # The paired runtime receipt observes equal ordered primary tag-code
    # sequences, but it does not establish field semantics or a producer
    # contract. Keep candidate group assignments unresolved.
    primary_tags = grades.get("fieldGrades", {}).get(
        "pairedWriterPrimaryTagSequence", {})
    if primary_tags.get("grade") != "empirically-observed":
        fail("paired writer tag-sequence agreement must remain observation-only")
    exact_coverage(primary_tags.get("slotIndices", []), all_indexes,
                   "paired writer primary tag-code sequence")
    if primary_tags.get("sourceReceipt") != (
            "3d-carrier-runtime-and-peer-writer-review-2026-10-15.json"):
        fail("paired tag-sequence observation must bind its exact receipt")

    r2013b = grades.get("fieldGrades", {}).get("r2013bValueGroup", {})
    exact_coverage(r2013b.get("unresolved", []), all_indexes,
                   "R2013b unresolved DXF groups")
    for index, slot in enumerate(slots):
        group = slot.get("groupR2013bCandidate")
        if group is None:
            fail(f"R2013b slot {index} lacks its explicitly non-authoritative candidate group")
        dxf_group_value_type(group)
    if r2013b.get("empirically-observed", []):
        fail("writer tag-sequence agreement cannot promote semantic group assignments")

    paired_companions = grades.get("fieldGrades", {}).get(
        "pairedWriterGroup176Output", {})
    if paired_companions.get("grade") != "empirically-observed":
        fail("paired group-176 results must be retained as observations")
    exact_coverage(paired_companions.get("odaSlotIndices", []), all_indexes,
                   "ODA group-176 output positions")
    exact_coverage(paired_companions.get("libreDwgMissingSlotIndices", []),
                   {31, 33}, "LibreDWG group-176 observed omissions")
    if (paired_companions.get("candidateGroup") != 176
            or paired_companions.get("sourceReceipt") != (
                "3d-carrier-runtime-and-peer-writer-review-2026-10-15.json")):
        fail("paired group-176 observation must bind the known group and receipt")

    paired_tuples = grades.get("fieldGrades", {}).get(
        "pairedWriterTupleComparison", {})
    if (paired_tuples.get("grade") != "empirically-observed"
            or paired_tuples.get("sourceReceipt") != (
                "3d-carrier-runtime-and-peer-writer-review-2026-10-15.json")):
        fail("paired tuple comparison must remain bound observation-only")
    exact_coverage(paired_tuples.get("differingSlotIndices", []),
                   {6, 31, 33, 41, 44, 51},
                   "paired writer differing tuple slots")
    if (paired_tuples.get("matchingSlotCount") != 52
            or paired_tuples.get("group177Counts") != {
                "libreDwg": 2, "oda": 3}
            or paired_tuples.get("group420Above24Bits") != [6, 41, 44]
            or paired_tuples.get("cmcValueDifferenceSlots") != [41, 51]):
        fail("paired writer observations differ from their frozen receipt")
    companions = grades.get("fieldGrades", {}).get(
        "perPropertyCompanionGroup", {})
    if companions.get("grade") != "unresolved":
        fail("group-176 companions cannot be promoted from candidate metadata")
    exact_coverage(companions.get("slotIndices", []), all_indexes,
                   "per-property companion slots")
    if companions.get("candidateGroup") != 176:
        fail("candidate companion group changed without evidence review")

    cmc = grades.get("fieldGrades", {}).get("cmcOutputPolicy", {})
    color_slots = set(manifest.get("slotEvidence", {}).get("colorSlots", []))
    exact_coverage(cmc.get("unresolved", []), color_slots,
                   "unresolved CMC output policy")
    exact_coverage(cmc.get("notApplicable", []), all_indexes - color_slots,
                   "non-CMC slots")
    serializer = grades.get("fieldGrades", {}).get("dxfSerializerAdmission", {})
    exact_coverage(serializer.get("unresolved", []), all_indexes,
                   "unresolved DXF serializer admission")
    if grades.get("serializerAdmission") != (
            "DENY_UNTIL_ALL_OUTPUT_GROUPS_CMC_POLICIES_OWNER_CLOSURE_"
            "AND_PRODUCER_READBACK_ARE_QUALIFIED"):
        fail("unresolved slot evidence must keep serializer admission denied")

    conflicts = {
        row.get("slotIndex")
        for row in grades.get("sourcePathConflicts", [])
        if "slotIndex" in row
    }
    for row in grades.get("sourcePathConflicts", []):
        conflicts.update(row.get("slotIndices", []))
        if ("slotIndex" in row
                and row.get("field") != slots[row["slotIndex"]].get("field")):
            fail("single-slot conflict name differs from the candidate crosswalk")
        for index, name in zip(row.get("slotIndices", []),
                               row.get("fields", [])):
            if slots[index].get("field") != name:
                fail("multi-slot conflict names differ from the candidate crosswalk")
    if not {6, 31, 33, 45}.issubset(conflicts):
        fail("known VisualStyle group/source-path conflicts must remain visible")
    if grades.get("supportClaimsPromoted") is not False:
        fail("slot evidence grades must not promote a support claim")
    if grades.get("externalItems") != {
        "J256": "DEFERRED_EXTERNAL",
        "J260": "DEFERRED_EXTERNAL",
        "J268": "DEFERRED_EXTERNAL",
        "J284": "DEFERRED_EXTERNAL",
        "J293": "DEFERRED_EXTERNAL",
        "J295": "DEFERRED_EXTERNAL",
    }:
        fail("external J statuses must remain deferred")

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

    print("PASS: 58 source/model slots align; paired writer observations, "
          "candidate DXF mappings, policy grades, and unresolved semantics "
          "remain separated; serializer denied")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
