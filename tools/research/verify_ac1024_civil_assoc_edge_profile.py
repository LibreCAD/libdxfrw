#!/usr/bin/env python3
"""Verify the bounded AC1024 civil Edge persistent-subentity profile."""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


EXPECTED_SOURCE_SHA256 = (
    "45b419d0c7382eb271079b80c3237a52183f4fa09e7b495c3d7a65c6baa927dc"
)
EXPECTED_CYCLES = {
    1776: (1754, {(201, 202), (202, 203), (203, 204), (204, 205), (205, 201)}),
    2478: (2474, {(201, 202), (202, 203), (203, 204), (204, 205), (205, 201)}),
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def json_handle(value: object) -> int | None:
    if isinstance(value, list) and value:
        tail = value[-1]
        return tail if isinstance(tail, int) else None
    return None


def verify(source: Path, profile: Path, peer: Path) -> dict[str, object]:
    source_hash = sha256(source)
    if source_hash != EXPECTED_SOURCE_SHA256:
        raise ValueError(f"source hash mismatch: {source_hash}")

    profile_text = profile.read_text(encoding="utf-8")
    if "sourceVersion=AC1024" not in profile_text:
        raise ValueError("local profile does not identify AC1024")
    if "objectParseFailures=0" not in profile_text:
        raise ValueError("local profile has object parse failures")

    line_re = re.compile(
        r"^DEP handle=(\d+) type=ACDBASSOCGEOMDEPENDENCY "
        r"owner=(\d+).*?depOn=(\d+).*?geomParsed=(\d+) "
        r"geomVer=(-?\d+).*?enabled=(\d+) subent=([^ ]*) "
        r"compound=(\d+) edgeParsed=(\d+) edgeVersion=(-?\d+) "
        r"index1=(-?\d+) index2=(-?\d+)$",
        re.MULTILINE,
    )
    local_rows = {
        int(match.group(1)): {
            "handle": int(match.group(1)),
            "owner": int(match.group(2)),
            "depOn": int(match.group(3)),
            "geomParsed": match.group(4) == "1",
            "geomVersion": int(match.group(5)),
            "enabled": match.group(6) == "1",
            "subentityClass": match.group(7),
            "compound": match.group(8) == "1",
            "edgeFieldsParsed": match.group(9) == "1",
            "edgeVersion": int(match.group(10)),
            "index1": int(match.group(11)),
            "index2": int(match.group(12)),
        }
        for match in line_re.finditer(profile_text)
    }
    rows = [
        row for row in local_rows.values()
        if row["subentityClass"] == "AcDbAssocEdgePersSubentId"
    ]
    if len(rows) != 10:
        raise ValueError(f"expected 10 parsed Edge records, found {len(rows)}")
    unresolved_pattern = re.compile(
        r"^DEP handle=3157 .*?bodyParsed=1 wire=0 .*?"
        r"namePresent=1 name=AcDbAssocEdgePersSubentId .*?"
        r"subent= compound=0 edgeParsed=0 edgeVersion=0 index1=0 index2=0$",
        re.MULTILINE,
    )
    if unresolved_pattern.search(profile_text) is None:
        raise ValueError("distinct handle-3157 profile no longer matches the refusal guard")

    peer_doc = json.loads(peer.read_text(encoding="utf-8"))
    peer_objects = peer_doc.get("OBJECTS")
    if not isinstance(peer_objects, list):
        raise ValueError("peer minJSON has no OBJECTS array")
    peer_by_handle = {
        json_handle(obj.get("handle")): obj
        for obj in peer_objects
        if isinstance(obj, dict) and json_handle(obj.get("handle")) is not None
    }

    per_owner: dict[int, dict[str, object]] = {}
    for row in rows:
        if (
            not row["geomParsed"]
            or row["geomVersion"] != 0
            or not row["enabled"]
            or not row["edgeFieldsParsed"]
            or row["edgeVersion"] != 0
            or row["compound"]
        ):
            raise ValueError(f"unexpected local Edge field state: {row}")
        peer_row = peer_by_handle.get(row["handle"])
        if not isinstance(peer_row, dict):
            raise ValueError(f"peer omitted Edge handle {row['handle']}")
        if peer_row.get("dxfname") != "ACDBASSOCGEOMDEPENDENCY":
            raise ValueError(f"peer type mismatch for handle {row['handle']}")
        if peer_row.get("classname") != "AcDbAssocEdgePersSubentId":
            raise ValueError(f"peer class mismatch for handle {row['handle']}")
        peer_dep_on = json_handle(peer_row.get("assocdep.dep_on"))
        peer_owner = json_handle(peer_row.get("ownerhandle"))
        if peer_dep_on != row["depOn"] or peer_owner != row["owner"]:
            raise ValueError(f"peer common-link mismatch for handle {row['handle']}")
        if peer_row.get("dependent_on_compound_object") != 1:
            raise ValueError(f"expected peer's known generic-field disagreement for {row['handle']}")

        group = per_owner.setdefault(
            row["owner"],
            {"target": row["depOn"], "handles": [], "pairs": set()},
        )
        if group["target"] != row["depOn"]:
            raise ValueError(f"owner {row['owner']} has inconsistent targets")
        group["handles"].append(row["handle"])
        group["pairs"].add((row["index1"], row["index2"]))

    if set(per_owner) != set(EXPECTED_CYCLES):
        raise ValueError(f"unexpected source group owners: {sorted(per_owner)}")
    cycle_rows = []
    for owner, (target, expected_pairs) in EXPECTED_CYCLES.items():
        group = per_owner[owner]
        if group["target"] != target or group["pairs"] != expected_pairs:
            raise ValueError(f"edge cycle mismatch for owner {owner}: {group}")
        entity = peer_by_handle.get(target)
        if not isinstance(entity, dict) or entity.get("entity") != "LWPOLYLINE":
            raise ValueError(f"target {target} is not a peer-decoded LWPOLYLINE")
        vertex_ids = entity.get("vertexids")
        if vertex_ids != [201, 202, 203, 204, 205]:
            raise ValueError(f"unexpected polyline vertex IDs for target {target}")
        cycle_rows.append({
            "owner": owner,
            "target": target,
            "edgeHandles": sorted(group["handles"]),
            "edgeIndexPairs": [list(pair) for pair in sorted(group["pairs"])],
            "targetEntity": entity["entity"],
            "targetVertexIds": vertex_ids,
            "closedCycleMatches": True,
        })

    unresolved = local_rows.get(3157)
    if unresolved is None or unresolved["edgeFieldsParsed"]:
        raise ValueError("the distinct handle-3157 incomplete frame was not retained as unresolved")

    return {
        "schema": "libdxfrw.research-result.v1",
        "source": {"path": str(source), "sha256": source_hash, "version": "AC1024"},
        "localProfile": {"path": str(profile), "sha256": sha256(profile)},
        "peerProfile": {"path": str(peer), "sha256": sha256(peer)},
        "boundedEdgeRecords": 10,
        "cycles": cycle_rows,
        "peerDisagreement": {
            "field": "dependent_on_compound_object",
            "peerValue": True,
            "localEdgeTailValue": False,
            "interpretation": "The peer applies the generic AcDbAssocPersSubentId body to these derived Edge records and does not consume the version/index tail; this mismatch is retained as a profile limitation, not averaged or normalized away.",
        },
        "unresolvedHandle3157": {
            "dependencyName": "AcDbAssocEdgePersSubentId",
            "geomSubentityClassName": "",
            "wireComplete": False,
            "action": "remain refused; not admitted by this Edge profile",
        },
        "supportClaimEffect": "none",
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("source", type=Path)
    parser.add_argument("profile", type=Path)
    parser.add_argument("peer_minjson", type=Path)
    args = parser.parse_args()
    result = verify(args.source, args.profile, args.peer_minjson)
    print(json.dumps(result, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
