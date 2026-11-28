#!/usr/bin/env python3
"""Inventory handle fields LibreDWG minJSON exposes for captured Cover frames."""

import argparse
import hashlib
import json
import re
from collections import Counter
from pathlib import Path


COMMON_FIELDS = {
    "handle",
    "ownerhandle",
    "reactors",
    "xdicobjhandle",
    "object",
    "index",
    "type",
    "size",
    "bitsize",
}


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def captured_frames(log_path):
    text = Path(log_path).read_text(encoding="utf-8")
    rows = {}
    pattern = re.compile(
        r"^ (opaque|standard) handle=(\d+) .*?\boffset=(\d+) size=(\d+)",
        re.MULTILINE,
    )
    for match in pattern.finditer(text):
        handle = int(match[2])
        if handle in rows:
            raise ValueError(f"duplicate local source-frame handle {handle}")
        rows[handle] = {
            "captureKind": match[1],
            "offset": int(match[3]),
            "size": int(match[4]),
        }
    return rows


def walk_handle_tuples(value, path=""):
    if isinstance(value, list):
        if (len(value) == 4 and all(isinstance(item, int) for item in value)
                and value[0] in (2, 3, 4, 5) and value[1] and value[2]):
            yield path, value[0], value[2]
            return
        for index, child in enumerate(value):
            yield from walk_handle_tuples(child, f"{path}[{index}]")
    elif isinstance(value, dict):
        for key, child in value.items():
            child_path = f"{path}.{key}" if path else key
            yield from walk_handle_tuples(child, child_path)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", required=True)
    parser.add_argument("--local-log", required=True)
    parser.add_argument("--peer-json", required=True)
    args = parser.parse_args()

    local = captured_frames(args.local_log)
    peer_doc = json.loads(Path(args.peer_json).read_text(encoding="utf-8"))
    peer_objects = peer_doc["OBJECTS"]
    peer_by_handle = {}
    for obj in peer_objects:
        raw_handle = obj.get("handle")
        handle = raw_handle[-1] if isinstance(raw_handle, list) else raw_handle
        if not isinstance(handle, int) or handle == 0:
            continue
        if handle in peer_by_handle:
            raise ValueError(f"duplicate peer object handle {handle}")
        peer_by_handle[handle] = obj

    edges = []
    size_matches = 0
    for handle, frame in sorted(local.items()):
        obj = peer_by_handle.get(handle)
        if obj is None:
            continue
        if obj.get("size") == frame["size"]:
            size_matches += 1
        for key, value in obj.items():
            if key in COMMON_FIELDS:
                continue
            for path, code, target in walk_handle_tuples(value, key):
                target_obj = peer_by_handle.get(target)
                edges.append({
                    "sourceHandle": handle,
                    "sourceObject": obj.get("object", "<unnamed entity>"),
                    "field": path,
                    "handleCode": code,
                    "targetHandle": target,
                    "targetObject": (
                        target_obj.get("object", "<unnamed entity>")
                        if target_obj is not None else None
                    ),
                    "targetResolves": target_obj is not None,
                })

    object_counts = Counter(
        peer_by_handle[handle].get("object", "<unnamed entity>")
        for handle in local if handle in peer_by_handle
    )
    unknown_rows = []
    for handle in sorted(local):
        obj = peer_by_handle.get(handle)
        if obj is None or not obj.get("num_unknown_bits"):
            continue
        raw_unknown = obj.get("unknown_bits", "")
        unknown_rows.append({
            "handle": handle,
            "object": obj.get("object", "<unnamed entity>"),
            "numUnknownBits": obj["num_unknown_bits"],
            "unknownBitsHexCharacters": len(raw_unknown)
                if isinstance(raw_unknown, str) else None,
        })

    joined = [handle for handle in local if handle in peer_by_handle]
    report = {
        "schema": "libdxfrw-peer-frame-body-handle-inventory-v1",
        "source": str(Path(args.source)),
        "sourceSha256": sha256(args.source),
        "localLog": str(Path(args.local_log)),
        "localLogSha256": sha256(args.local_log),
        "peerJson": str(Path(args.peer_json)),
        "peerJsonSha256": sha256(args.peer_json),
        "verifier": str(Path(__file__)),
        "verifierSha256": sha256(__file__),
        "capturedFrameCount": len(local),
        "peerFrameJoinCount": len(joined),
        "peerFrameSizeMatches": size_matches,
        "peerObjectCountsForCapturedFrames": dict(sorted(object_counts.items())),
        "peerBodyHandleEdgeCount": len(edges),
        "peerBodyHandleEdges": edges,
        "unresolvedPeerTargets": [
            edge for edge in edges if not edge["targetResolves"]
        ],
        "peerUnknownBodyRows": unknown_rows,
        "coverageLimit": (
            "This inventories handle tuples exposed by LibreDWG minJSON for the "
            "joined frames. It does not prove that minJSON decoded every source "
            "body field, and it is not a local raw-frame scanner or output proof."
        ),
    }
    print(json.dumps(report, indent=2, sort_keys=True))
    return 0 if len(joined) == len(local) and size_matches == len(local) \
        and not report["unresolvedPeerTargets"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
