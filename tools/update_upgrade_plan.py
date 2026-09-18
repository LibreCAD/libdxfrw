#!/usr/bin/env python3
"""Validate and report the live execution ledger in the upgrade plan.

The updater deliberately has no third-party dependencies.  It treats the
delimited progress block as the only mutable part of the plan and refuses to
rewrite anything outside those markers.  The implementation is intentionally
small: the ledger is Markdown for human review, while this module supplies the
state/dependency checks that prevent a progress row from becoming an
unverifiable claim.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from tempfile import TemporaryDirectory


START = "<!-- UPGRADE_PROGRESS_START -->"
END = "<!-- UPGRADE_PROGRESS_END -->"
SLICE_STATES = {"PLANNED", "READY", "ACTIVE", "VERIFYING", "VERIFIED",
                "COMMITTED", "BLOCKED_HARD", "SUPERSEDED"}
ITEM_STATES = SLICE_STATES
CLAIMS = {"NOT_APPLICABLE", "NOT_EVALUATED", "SATISFIED",
          "DEFERRED_EXTERNAL", "EXPERIMENTAL", "PROMOTED"}
SLICE_ID = re.compile(r"^S[0-9]+[A-Za-z]*$")
ITEM_ID = re.compile(r"^[A-Za-z][A-Za-z0-9]*(?:\.[A-Za-z0-9]+)*$")


@dataclass(frozen=True)
class Row:
    kind: str
    ident: str
    fields: tuple[str, ...]
    line_no: int


def split_row(line: str) -> list[str] | None:
    if not line.startswith("|") or not line.rstrip().endswith("|"):
        return None
    parts = [part.strip() for part in line.rstrip().split("|")[1:-1]]
    if not parts or all(set(part) <= {"-", ":", " "} for part in parts):
        return None
    return parts


def progress_bounds(text: str) -> tuple[int, int]:
    start = text.find(START)
    end = text.find(END)
    if start < 0 or end < 0 or end <= start:
        raise ValueError("plan must contain one ordered progress block")
    if text.find(START, start + len(START)) >= 0:
        raise ValueError("plan contains multiple progress starts")
    if text.find(END, end + len(END)) >= 0:
        raise ValueError("plan contains multiple progress ends")
    return start, end + len(END)


def rows(text: str) -> list[Row]:
    start, end = progress_bounds(text)
    block = text[start:end].splitlines()
    result: list[Row] = []
    table_kind: str | None = None
    for offset, line in enumerate(block, start=text[:start].count("\n") + 1):
        parts = split_row(line)
        if parts is None:
            continue
        header = parts[0]
        if header == "Slice":
            table_kind = "slice"
            continue
        if header == "Parent item":
            table_kind = "parent"
            continue
        if header == "Child item":
            table_kind = "child"
            continue
        if table_kind == "slice" and len(parts) == 7 and SLICE_ID.match(parts[0]):
            result.append(Row("slice", parts[0], tuple(parts), offset))
        elif table_kind == "parent" and len(parts) == 6 and ITEM_ID.match(parts[0]):
            result.append(Row("parent", parts[0], tuple(parts), offset))
        elif table_kind == "child" and len(parts) == 8 and ITEM_ID.match(parts[0]):
            result.append(Row("child", parts[0], tuple(parts), offset))
    return result


def parse_dependencies(value: str) -> list[str]:
    value = value.strip()
    if value.lower() in {"", "none", "-"}:
        return []
    return [item.strip() for item in value.split(",") if item.strip()]


def validate(text: str, repo: Path | None = None) -> list[str]:
    diagnostics: list[str] = []
    parsed = rows(text)
    slices = {row.ident: row for row in parsed if row.kind == "slice"}
    parents = {row.ident: row for row in parsed if row.kind == "parent"}
    children = {row.ident: row for row in parsed if row.kind == "child"}
    if not slices:
        diagnostics.append("no slice rows found")
    if len(slices) != sum(row.kind == "slice" for row in parsed):
        diagnostics.append("duplicate slice identifier")
    if len(parents) != sum(row.kind == "parent" for row in parsed):
        diagnostics.append("duplicate parent identifier")
    if len(children) != sum(row.kind == "child" for row in parsed):
        diagnostics.append("duplicate child identifier")

    for row in slices.values():
        state = row.fields[3]
        if state not in SLICE_STATES:
            diagnostics.append(f"{row.ident}: invalid slice state {state}")
        for dep in parse_dependencies(row.fields[2]):
            if dep not in slices:
                diagnostics.append(f"{row.ident}: unknown slice dependency {dep}")
        if row.fields[0] != row.ident:
            diagnostics.append(f"{row.ident}: malformed slice row")
    for row in parents.values():
        state = row.fields[3]
        if state not in ITEM_STATES:
            diagnostics.append(f"{row.ident}: invalid parent state {state}")
        if row.fields[1] not in slices:
            diagnostics.append(f"{row.ident}: parent references unknown slice {row.fields[1]}")
        for dep in parse_dependencies(row.fields[2]):
            if dep not in parents:
                diagnostics.append(f"{row.ident}: unknown parent dependency {dep}")
        if row.fields[4] not in CLAIMS:
            diagnostics.append(f"{row.ident}: invalid claim disposition {row.fields[4]}")
    for row in children.values():
        state = row.fields[4]
        if state not in ITEM_STATES:
            diagnostics.append(f"{row.ident}: invalid child state {state}")
        parent, slice_id = (row.fields[1].split("/", 1) + [""])[:2]
        parent, slice_id = parent.strip(), slice_id.strip()
        if parent not in parents:
            diagnostics.append(f"{row.ident}: child references unknown parent {parent}")
        if slice_id not in slices:
            diagnostics.append(f"{row.ident}: child references unknown slice {slice_id}")
        if row.fields[5] not in CLAIMS:
            diagnostics.append(f"{row.ident}: invalid child claim disposition {row.fields[5]}")
        for dep in parse_dependencies(row.fields[3]):
            if dep not in children and dep not in parents and dep not in slices:
                diagnostics.append(f"{row.ident}: unknown child dependency {dep}")

    # A committed slice must have a matching commit trailer.  This catches a
    # status row accidentally staged without the implementation commit.  The
    # check is skipped for the self-test's synthetic repository.
    if repo is not None and (repo / ".git").exists():
        for row in slices.values():
            if row.fields[3] != "COMMITTED":
                continue
            try:
                log = subprocess.check_output(
                    ["git", "-C", str(repo), "log", "--all", "--format=%H%n%B",
                     "--grep", f"^Plan-Slice: {row.ident}$", "-n", "1"],
                    text=True, stderr=subprocess.STDOUT)
            except (OSError, subprocess.CalledProcessError):
                diagnostics.append(f"{row.ident}: cannot inspect commit trailers")
                continue
            if not log.strip():
                diagnostics.append(f"{row.ident}: no commit with matching Plan-Slice trailer")
    return diagnostics


def report(plan: Path, revision: str, repo: Path) -> str:
    text = plan.read_text(encoding="utf-8")
    try:
        message = subprocess.check_output(
            ["git", "-C", str(repo), "show", "-s", "--format=%H%n%s%n%B", revision],
            text=True, stderr=subprocess.STDOUT)
    except (OSError, subprocess.CalledProcessError) as exc:
        raise ValueError(f"cannot inspect {revision}: {exc}") from exc
    slice_match = re.search(r"(?m)^Plan-Slice:\s*(\S+)\s*$", message)
    if not slice_match:
        raise ValueError(f"{revision} has no Plan-Slice trailer")
    ident = slice_match.group(1)
    row = next((item for item in rows(text)
                if item.kind == "slice" and item.ident == ident), None)
    if row is None:
        raise ValueError(f"{ident} is absent from the plan")
    if row.fields[3] != "COMMITTED":
        raise ValueError(f"{ident} is {row.fields[3]}, not COMMITTED")
    items = re.search(r"(?m)^Plan-Items:\s*(.*?)\s*$", message)
    gates = re.search(r"(?m)^Plan-Gates:\s*(.*?)\s*$", message)
    return (f"Slice {ident} committed: {message.splitlines()[0]}\n"
            f"  Subject: {message.splitlines()[1] if len(message.splitlines()) > 1 else ''}\n"
            f"  Items: {items.group(1) if items else '(missing)'}\n"
            f"  Gates: {gates.group(1) if gates else '(missing)'}")


def replace_live_block(plan: Path, new_block: str) -> None:
    text = plan.read_text(encoding="utf-8")
    start, end = progress_bounds(text)
    if START not in new_block or END not in new_block:
        raise ValueError("replacement must contain both progress markers")
    plan.write_text(text[:start] + new_block + text[end:], encoding="utf-8")


def transition(plan: Path, ident: str, state: str) -> None:
    if state not in ITEM_STATES:
        raise ValueError(f"invalid state {state}")
    text = plan.read_text(encoding="utf-8")
    start, end = progress_bounds(text)
    block = text[start:end]
    lines = block.splitlines(keepends=True)
    changed = False
    for index, line in enumerate(lines):
        parts = split_row(line.rstrip("\n"))
        if parts is None or parts[0] != ident:
            continue
        state_index = 3 if len(parts) == 7 else 4 if len(parts) == 8 else None
        if state_index is None:
            raise ValueError(f"{ident} is not a slice or child row")
        parts[state_index] = state
        newline = "| " + " | ".join(parts) + " |\n"
        lines[index] = newline
        changed = True
        break
    if not changed:
        raise ValueError(f"item {ident} not found in the live block")
    replace_live_block(plan, "".join(lines).rstrip("\n") + "\n")


def set_slice_state(plan: Path, slice_id: str, state: str) -> None:
    """Change one slice state while preserving the surrounding Markdown."""
    if state not in SLICE_STATES:
        raise ValueError(f"invalid slice state {state}")
    text = plan.read_text(encoding="utf-8")
    start, end = progress_bounds(text)
    block = text[start:end]
    lines = block.splitlines(keepends=True)
    changed = False
    for index, line in enumerate(lines):
        parts = split_row(line.rstrip("\n"))
        if parts is None or len(parts) != 7 or parts[0] != slice_id:
            continue
        parts[3] = state
        lines[index] = "| " + " | ".join(parts) + " |\n"
        changed = True
        break
    if not changed:
        raise ValueError(f"slice {slice_id} not found in the live block")
    replace_live_block(plan, "".join(lines).rstrip("\n") + "\n")


def prepare_commit(plan: Path, slice_id: str) -> None:
    text = plan.read_text(encoding="utf-8")
    row = next((item for item in rows(text)
                if item.kind == "slice" and item.ident == slice_id), None)
    if row is None:
        raise ValueError(f"slice {slice_id} not found")
    if row.fields[3] != "VERIFIED":
        raise ValueError(f"{slice_id} must be VERIFIED before --prepare-commit")
    marker = f"<!-- PLAN_PREPARED:{slice_id} -->"
    if marker in text:
        raise ValueError(f"{slice_id} already has a prepared marker")
    set_slice_state(plan, slice_id, "COMMITTED")
    text = plan.read_text(encoding="utf-8")
    start, end = progress_bounds(text)
    text = text[:end - len(END)] + "\n" + marker + text[end - len(END):]
    plan.write_text(text, encoding="utf-8")


def abort_commit(plan: Path, slice_id: str) -> None:
    text = plan.read_text(encoding="utf-8")
    marker = f"<!-- PLAN_PREPARED:{slice_id} -->"
    if marker not in text:
        raise ValueError(f"{slice_id} has no prepared marker")
    set_slice_state(plan, slice_id, "VERIFIED")
    text = plan.read_text(encoding="utf-8").replace("\n" + marker, "")
    plan.write_text(text, encoding="utf-8")


def self_test() -> None:
    sample = f"prefix\n{START}\n" \
        "| Slice | Plan items | Dependencies | State | Required gates | Evidence / decision | Unblocks / next |\n" \
        "| --- | --- | --- | --- | --- | --- | --- |\n" \
        "| S00 | A0 | none | COMMITTED | gate | ok | S01 |\n" \
        "| Parent item | Slice | Dependencies | Execution state | Claim/evidence | Scope / current evidence |\n" \
        "| --- | --- | --- | --- | --- | --- |\n" \
        "| A0 | S00 | none | PLANNED | NOT_APPLICABLE | test |\n" \
        "| Child item | Parent / slice | WP/Phase references | Dependencies | Execution state | Claim/evidence | Direct gate | Evidence / unblocks |\n" \
        "| --- | --- | --- | --- | --- | --- | --- | --- |\n" \
        "| A0.1 | A0 / S00 | WP0 | none | COMMITTED | NOT_APPLICABLE | gate | ok |\n" \
        f"{END}\nsuffix\n"
    assert not validate(sample)
    assert len(rows(sample)) == 3
    with TemporaryDirectory() as directory:
        path = Path(directory) / "plan.md"
        path.write_text(sample, encoding="utf-8")
        transition(path, "A0.1", "VERIFIED")
        changed = path.read_text(encoding="utf-8")
        assert "| A0.1 | A0 / S00 | WP0 | none | VERIFIED |" in changed
        assert changed.startswith("prefix\n") and changed.endswith("suffix\n")
        # A prepared slice is reversible only through its explicit marker.
        prepared = sample.replace("| S00 | A0 | none | COMMITTED |",
                                  "| S00 | A0 | none | VERIFIED |")
        path.write_text(prepared, encoding="utf-8")
        prepare_commit(path, "S00")
        assert "PLAN_PREPARED:S00" in path.read_text(encoding="utf-8")
        abort_commit(path, "S00")
        assert "PLAN_PREPARED:S00" not in path.read_text(encoding="utf-8")
    bad = sample.replace("| S00 | A0 | none | COMMITTED |", "| S00 | A0 | S99 | COMMITTED |")
    assert validate(bad)
    print("update_upgrade_plan self-test: PASS")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--plan", type=Path, default=Path("LIBRECAD_DXFRW_UPGRADE_PLAN.md"))
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--self-test", action="store_true")
    parser.add_argument("--report", metavar="REVISION")
    parser.add_argument("--transition", nargs=2, metavar=("ITEM", "STATE"))
    parser.add_argument("--prepare-commit", action="store_true")
    parser.add_argument("--abort-commit", action="store_true")
    parser.add_argument("--slice", dest="slice_id")
    args = parser.parse_args(argv)
    if args.self_test:
        self_test()
        return 0
    try:
        if args.prepare_commit and args.abort_commit:
            raise ValueError("--prepare-commit and --abort-commit are mutually exclusive")
        if (args.prepare_commit or args.abort_commit) and not args.slice_id:
            raise ValueError("--slice is required with --prepare-commit/--abort-commit")
        if args.prepare_commit:
            prepare_commit(args.plan, args.slice_id)
        if args.abort_commit:
            abort_commit(args.plan, args.slice_id)
        if args.transition:
            transition(args.plan, args.transition[0], args.transition[1])
        if args.report:
            print(report(args.plan, args.report, args.plan.resolve().parent))
        if args.check or not (args.report or args.transition or
                               args.prepare_commit or args.abort_commit):
            diagnostics = validate(args.plan.read_text(encoding="utf-8"),
                                   args.plan.resolve().parent)
            if diagnostics:
                for diagnostic in diagnostics:
                    print(f"ERROR: {diagnostic}", file=sys.stderr)
                return 1
            print(f"Plan check: PASS ({args.plan})")
    except (OSError, ValueError, AssertionError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
