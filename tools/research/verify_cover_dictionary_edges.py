#!/usr/bin/env python3
"""Compare captured Cover dictionary memberships with LibreDWG minJSON."""

import argparse
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def local_edges(log_path):
    text = Path(log_path).read_text(encoding="utf-8")
    edges = []
    patterns = (
        (r"^  dictionaryEntry owner=(\d+) key=(.*?) target=(\d+)$", 1),
        (r"^ NOD key=(.*?) target=(\d+)(?: |$)", 2),
    )
    for pattern, kind in patterns:
        for match in re.finditer(pattern, text, re.MULTILINE):
            if kind == 1:
                owner, key, target = int(match[1]), match[2], int(match[3])
            else:
                owner, key, target = 12, match[1], int(match[2])
            edges.append((owner, key, target))
    for match in re.finditer(
        r"^ typedBodyReference owner=(\d+) role=dictionary-entry "
        r"key=(.*?) target=(\d+)$",
        text,
        re.MULTILINE,
    ):
        edges.append((int(match[1]), match[2], int(match[3])))
    return edges


def peer_edges(peer_path):
    document = json.loads(Path(peer_path).read_text(encoding="utf-8"))
    objects = document["OBJECTS"]
    edges = []
    object_handles = set()
    for obj in objects:
        handle = obj.get("handle")
        if isinstance(handle, list):
            handle = handle[-1]
        if isinstance(handle, int) and handle != 0:
            object_handles.add(handle)
        if obj.get("object") not in ("DICTIONARY", "DICTIONARYWDFLT"):
            continue
        items = obj.get("items", {})
        if not isinstance(items, dict):
            raise ValueError(f"dictionary {handle} items is not an object")
        for key, ref in items.items():
            if (not isinstance(ref, list) or len(ref) != 4
                    or not all(isinstance(value, int) for value in ref)
                    or ref[0] != 2):
                raise ValueError(f"unsupported dictionary reference: {handle}/{key}")
            edges.append((handle, key, ref[2]))
    return edges, object_handles


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True, help="source DWG path")
    parser.add_argument("--local-log", required=True)
    parser.add_argument("--peer-json", required=True)
    args = parser.parse_args()

    local = local_edges(args.local_log)
    peer, object_handles = peer_edges(args.peer_json)
    local_counts = Counter(local)
    peer_counts = Counter(peer)
    missing_peer = sorted((local_counts - peer_counts).elements())
    missing_local = sorted((peer_counts - local_counts).elements())
    unresolved = sorted({target for _, _, target in local if target and target not in object_handles})
    null_slots = sum(target == 0 for _, _, target in local)
    report = {
        "schema": "libdxfrw-dictionary-edge-comparison-v1",
        "verifier": str(Path(__file__)),
        "verifierSha256": sha256(__file__),
        "source": str(Path(args.source)),
        "sourceSha256": sha256(args.source),
        "localLog": str(Path(args.local_log)),
        "localLogSha256": sha256(args.local_log),
        "peerJson": str(Path(args.peer_json)),
        "peerJsonSha256": sha256(args.peer_json),
        "localEdgeRows": len(local),
        "peerEdgeRows": len(peer),
        "matchedRows": sum((local_counts & peer_counts).values()),
        "nullSlots": null_slots,
        "unresolvedLocalTargets": unresolved,
        "localMissingFromPeer": missing_peer,
        "peerMissingFromLocal": missing_local,
        "exactMembershipMatch": not missing_peer and not missing_local,
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if report["exactMembershipMatch"] and not unresolved else 1


if __name__ == "__main__":
    raise SystemExit(main())
