#!/usr/bin/env python3
"""Validate and update the live libdxfrw convergence plan.

The updater deliberately edits only the UPGRADE_PROGRESS block.  It has no
third-party dependencies so it can run before the imported C++ tree builds.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path


START = "<!-- UPGRADE_PROGRESS_START -->"
END = "<!-- UPGRADE_PROGRESS_END -->"
EXECUTION_STATES = {
    "PLANNED",
    "READY",
    "ACTIVE",
    "VERIFYING",
    "VERIFIED",
    "COMMITTED",
    "BLOCKED_HARD",
    "SUPERSEDED",
}
CLAIM_STATES = {
    "NOT_APPLICABLE",
    "NOT_EVALUATED",
    "SATISFIED",
    "DEFERRED_EXTERNAL",
    "EXPERIMENTAL",
    "PROMOTED",
}
SLICE_RE = re.compile(r"^\|\s*(S\d+[a-z]?)\s*\|")
PARENT_RE = re.compile(r"^\|\s*([A-Z]\d+)\s*\|")
CHILD_RE = re.compile(r"^\|\s*([A-Z]\d+\.\d+[a-z]?)\s*\|")
TRAILER_RE = re.compile(r"(?m)^Plan-Slice:\s*(S\d+[a-z]?)\s*$")


class PlanError(RuntimeError):
    pass


def split_cells(line):
    if not line.startswith("|"):
        return []
    return [cell.strip() for cell in line.strip().split("|")[1:-1]]


def marker_bounds(text):
    starts = [m.start() for m in re.finditer(re.escape(START), text)]
    ends = [m.start() for m in re.finditer(re.escape(END), text)]
    if len(starts) != 1 or len(ends) != 1 or starts[0] >= ends[0]:
        raise PlanError("live progress markers must occur exactly once and be ordered")
    return starts[0], ends[0]


def live_block(text):
    start, end = marker_bounds(text)
    return text[start : end + len(END)]


def all_rows(block, matcher):
    result = []
    for line_number, line in enumerate(block.splitlines(), 1):
        match = matcher.match(line)
        if match:
            cells = split_cells(line)
            result.append((match.group(1), line_number, cells, line))
    return result


def dependency_names(value):
    value = value.strip()
    if value.lower() in {"none", "", "—", "-"}:
        return []
    return [token.strip() for token in re.split(r"[,;]", value) if token.strip()]


def state_index(kind):
    if kind == "slice" or kind == "parent":
        return 3
    return 4


def classify_item(item_id):
    if SLICE_RE.match("| %s |" % item_id):
        return "slice"
    if CHILD_RE.match("| %s |" % item_id):
        return "child"
    if PARENT_RE.match("| %s |" % item_id):
        return "parent"
    raise PlanError("unknown item ID: %s" % item_id)


def replace_state(text, item_id, new_state):
    if new_state not in EXECUTION_STATES:
        raise PlanError("invalid execution state: %s" % new_state)
    start, end = marker_bounds(text)
    block = text[start : end + len(END)]
    kind = classify_item(item_id)
    matcher = {"slice": SLICE_RE, "parent": PARENT_RE, "child": CHILD_RE}[kind]
    rows = all_rows(block, matcher)
    matches = [row for row in rows if row[0] == item_id]
    if len(matches) != 1:
        raise PlanError("expected one live %s row for %s, found %d" % (kind, item_id, len(matches)))
    _, _, cells, line = matches[0]
    index = state_index(kind)
    if len(cells) <= index:
        raise PlanError("row for %s has no execution-state column" % item_id)
    cells[index] = new_state
    new_line = "| " + " | ".join(cells) + " |"
    old_start = block.find(line)
    if old_start < 0:
        raise PlanError("could not locate row for %s" % item_id)
    new_block = block[:old_start] + new_line + block[old_start + len(line) :]
    return text[:start] + new_block + text[end + len(END) :]


def parse_tables(block):
    slices = {}
    parents = {}
    children = {}
    for item_id, _, cells, _ in all_rows(block, SLICE_RE):
        if item_id in slices:
            raise PlanError("duplicate slice ID: %s" % item_id)
        if len(cells) < 7:
            raise PlanError("slice row %s has %d cells; expected at least 7" % (item_id, len(cells)))
        slices[item_id] = {"cells": cells, "state": cells[3], "deps": dependency_names(cells[2]), "items": cells[1]}
    for item_id, _, cells, _ in all_rows(block, PARENT_RE):
        if item_id.startswith("S") or CHILD_RE.match("| %s |" % item_id):
            continue
        if item_id in parents:
            raise PlanError("duplicate parent item ID: %s" % item_id)
        if len(cells) < 6:
            raise PlanError("parent row %s has %d cells; expected at least 6" % (item_id, len(cells)))
        parents[item_id] = {"cells": cells, "state": cells[3], "claim": cells[4], "slice": cells[1], "deps": dependency_names(cells[2])}
    for item_id, _, cells, _ in all_rows(block, CHILD_RE):
        if item_id in children:
            raise PlanError("duplicate child item ID: %s" % item_id)
        if len(cells) < 8:
            raise PlanError("child row %s has %d cells; expected at least 8" % (item_id, len(cells)))
        children[item_id] = {"cells": cells, "state": cells[4], "claim": cells[5], "parent": cells[1], "deps": dependency_names(cells[3])}
    return slices, parents, children


def validate(text):
    block = live_block(text)
    slices, parents, children = parse_tables(block)
    if not slices:
        raise PlanError("live block has no slice rows")
    for item_id, record in slices.items():
        if record["state"] not in EXECUTION_STATES:
            raise PlanError("slice %s has invalid state %s" % (item_id, record["state"]))
        for dependency in record["deps"]:
            if dependency not in slices:
                raise PlanError("slice %s depends on unknown slice %s" % (item_id, dependency))
    for item_id, record in parents.items():
        if record["state"] not in EXECUTION_STATES:
            raise PlanError("parent %s has invalid state %s" % (item_id, record["state"]))
        if record["claim"] not in CLAIM_STATES:
            raise PlanError("parent %s has invalid claim disposition %s" % (item_id, record["claim"]))
        if record["slice"] not in slices:
            raise PlanError("parent %s names unknown slice %s" % (item_id, record["slice"]))
        for dependency in record["deps"]:
            if dependency not in parents:
                raise PlanError("parent %s depends on unknown parent %s" % (item_id, dependency))
    for item_id, record in children.items():
        if record["state"] not in EXECUTION_STATES:
            raise PlanError("child %s has invalid state %s" % (item_id, record["state"]))
        if record["claim"] not in CLAIM_STATES:
            raise PlanError("child %s has invalid claim disposition %s" % (item_id, record["claim"]))
        parent = record["parent"].split("/")[0].strip()
        if parent not in parents:
            raise PlanError("child %s names unknown parent %s" % (item_id, parent))
        for dependency in record["deps"]:
            if dependency not in children and dependency not in parents:
                raise PlanError("child %s depends on unknown item %s" % (item_id, dependency))
    for item_id, record in parents.items():
        member_children = [child for child in children.values() if child["parent"].split("/")[0].strip() == item_id]
        if record["state"] in {"VERIFIED", "COMMITTED"} and any(child["state"] not in {"VERIFIED", "COMMITTED"} for child in member_children):
            raise PlanError("parent %s is %s but a child is not verified" % (item_id, record["state"]))
    check_acyclic(slices, "slice")
    check_acyclic(parents, "parent")
    return slices, parents, children


def check_acyclic(records, kind):
    visiting = set()
    visited = set()

    def visit(node):
        if node in visiting:
            raise PlanError("cycle in %s dependency graph at %s" % (kind, node))
        if node in visited:
            return
        visiting.add(node)
        for dependency in records[node]["deps"]:
            visit(dependency)
        visiting.remove(node)
        visited.add(node)

    for node in records:
        visit(node)


def write_plan(path, text):
    path.write_text(text, encoding="utf-8")


def prepare_commit(text, slice_id):
    slices, parents, children = validate(text)
    if slice_id not in slices:
        raise PlanError("unknown slice: %s" % slice_id)
    member_ids = [token.split(":", 1)[0].strip() for token in slices[slice_id]["items"].split("+")]
    if not member_ids:
        raise PlanError("slice %s has no plan items" % slice_id)
    for item_id in member_ids:
        if item_id not in parents:
            raise PlanError("slice %s names unknown parent item %s" % (slice_id, item_id))
        if parents[item_id]["state"] not in {"VERIFIED", "COMMITTED"}:
            raise PlanError("item %s is %s; it must be VERIFIED before prepare-commit" % (item_id, parents[item_id]["state"]))
        member_children = [child_id for child_id, child in children.items() if child["parent"].split("/")[0].strip() == item_id]
        if any(children[child_id]["state"] not in {"VERIFIED", "COMMITTED"} for child_id in member_children):
            raise PlanError("item %s has a child that is not verified" % item_id)
    result = text
    for item_id in member_ids:
        result = replace_state(result, item_id, "COMMITTED")
        for child_id, child in children.items():
            if child["parent"].split("/")[0].strip() == item_id and child["state"] != "COMMITTED":
                result = replace_state(result, child_id, "COMMITTED")
    return replace_state(result, slice_id, "COMMITTED")


def abort_commit(text, slice_id):
    slices, parents, children = validate(text)
    if slice_id not in slices:
        raise PlanError("unknown slice: %s" % slice_id)
    result = text
    member_ids = [token.split(":", 1)[0].strip() for token in slices[slice_id]["items"].split("+")]
    for item_id in member_ids:
        if item_id in parents and parents[item_id]["state"] == "COMMITTED":
            result = replace_state(result, item_id, "VERIFIED")
            for child_id, child in children.items():
                if child["parent"].split("/")[0].strip() == item_id and child["state"] == "COMMITTED":
                    result = replace_state(result, child_id, "VERIFIED")
    if slices[slice_id]["state"] == "COMMITTED":
        result = replace_state(result, slice_id, "VERIFIED")
    return result


def git_output(args):
    try:
        return subprocess.check_output(args, text=True, stderr=subprocess.STDOUT).strip()
    except (OSError, subprocess.CalledProcessError) as exc:
        raise PlanError("command failed: %s" % exc) from exc


def report(plan_path, revision):
    text = plan_path.read_text(encoding="utf-8")
    validate(text)
    sha = git_output(["git", "rev-parse", "--verify", revision])
    message = git_output(["git", "show", "-s", "--format=%B", sha])
    matches = TRAILER_RE.findall(message)
    if len(matches) != 1:
        raise PlanError("commit %s must contain exactly one Plan-Slice trailer" % sha[:12])
    slice_id = matches[0]
    subject = git_output(["git", "show", "-s", "--format=%s", sha])
    changed = git_output(["git", "diff-tree", "--no-commit-id", "--name-only", "-r", sha]).splitlines()
    if plan_path.name not in {Path(name).name for name in changed}:
        raise PlanError("commit %s does not contain the live plan" % sha[:12])
    print("Slice %s committed: %s — %s" % (slice_id, sha[:12], subject))
    print("Plan path: %s" % plan_path)
    print("Trailer validation: PASS (%s)" % sha)
    print("Changed paths: %s" % (", ".join(changed) if changed else "none"))


def self_test():
    sample = chr(10).join([
        "prefix", START, "Current checkpoint: pre-A",
        "| Slice | Plan items | Dependencies | State | Gates | Evidence | Next |",
        "| --- | --- | --- | --- | --- | --- | --- |",
        "| S01 | A0 | none | READY | check | pending | S02 |",
        "| S02 | A1 | S01 | PLANNED | check | pending | none |",
        "| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope |",
        "| --- | --- | --- | --- | --- | --- |",
        "| A0 | S01 | none | READY | NOT_APPLICABLE | test |",
        "| A1 | S02 | A0 | PLANNED | NOT_EVALUATED | test |",
        "| Child item | Parent / slice | WP/Phase references | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |",
        "| --- | --- | --- | --- | --- | --- | --- | --- |",
        "| A0.1 | A0 / S01 | WP0.1 | none | VERIFIED | NOT_APPLICABLE | check | evidence |",
        END, "suffix", ""
    ])
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "plan.md"
        write_plan(path, sample)
        validate(sample)
        active = replace_state(sample, "A0", "ACTIVE")
        assert "| A0 | S01 | none | ACTIVE |" in active
        verifying = replace_state(active, "A0", "VERIFYING")
        assert "| A0 | S01 | none | VERIFYING |" in verifying
        assert verifying.startswith("prefix\n") and verifying.endswith("suffix\n")
        try:
            replace_state(verifying, "A0", "NOPE")
        except PlanError:
            pass
        else:
            raise AssertionError("invalid state was accepted")
        prepared = replace_state(verifying, "A0", "VERIFIED")
        prepared = prepare_commit(prepared, "S01")
        assert "| S01 | A0 | none | COMMITTED |" in prepared
        assert "| A0 | S01 | none | COMMITTED |" in prepared
        assert "| A0.1 | A0 / S01 | WP0.1 | none | COMMITTED |" in prepared
        aborted = abort_commit(prepared, "S01")
        assert "| S01 | A0 | none | VERIFIED |" in aborted
        assert "| A0 | S01 | none | VERIFIED |" in aborted
        assert TRAILER_RE.findall("Subject\n\nPlan-Slice: S01\n") == ["S01"]
    print("update_upgrade_plan self-test: PASS")


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=Path("LIBRECAD_DXFRW_UPGRADE_PLAN.md"))
    parser.add_argument("--check", action="store_true", help="validate the live plan")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--transition", nargs=2, metavar=("ITEM", "STATE"))
    parser.add_argument("--prepare-commit", metavar="SLICE")
    parser.add_argument("--abort-commit", metavar="SLICE")
    parser.add_argument("--report", metavar="REVISION")
    args = parser.parse_args(argv)
    try:
        if args.self_test:
            self_test()
            return 0
        if args.report:
            report(args.plan, args.report)
            return 0
        text = args.plan.read_text(encoding="utf-8")
        if args.transition:
            text = replace_state(text, args.transition[0], args.transition[1])
            write_plan(args.plan, text)
        elif args.prepare_commit:
            text = prepare_commit(text, args.prepare_commit)
            write_plan(args.plan, text)
        elif args.abort_commit:
            text = abort_commit(text, args.abort_commit)
            write_plan(args.plan, text)
        validate(args.plan.read_text(encoding="utf-8"))
        if args.check or not any((args.transition, args.prepare_commit, args.abort_commit)):
            print("plan check: PASS (%s)" % args.plan)
        return 0
    except (OSError, UnicodeError, PlanError, AssertionError) as exc:
        print("plan check: FAIL: %s" % exc, file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.exit(main())
