# LibreCAD libdxfrw source lock

This file records the source-convergence inputs. It is intentionally separate
from the parent LibreCAD working tree: the import reads Git objects from the
pinned commit and never copies a dirty checkout.

- Standalone baseline: `origin/master` at
  `92d7466ed9146badcd4fb44c82d1dd8302b3c7db`.
- LibreCAD target: `origin/master` at
  `3c7785ebbcbfc8f3c8f79dbba093aff09cdec753`.
- Bundled `.snapshot-revision`:
  `89b762bef636c90eb370cb1af3cec80fe759cb32`.
- Source manifest: `metadata/libdxfrw-target-source-manifest.txt` (86 entries,
  SHA-256 `8e9ed232e9646d5f7f0009d8c8494eac9e27e1d1881283c9a21883cebedd34d4`).
- Deterministic source archive SHA-256:
  `f99cb84c519e282fb5b2e7ebccbecb9a12526a0fd42b828249b1d3253ab9ae65`.

## Import procedure

1. Refresh both remote refs and record any delta from the hashes above.
2. Review source/manifest paths and update the target lock once.
3. Extract `libraries/libdxfrw/src` and
   `libraries/libdxfrw/libdxfrw_sources.cmake` from Git objects, not a working
   tree.
4. Compare every extracted path/mode/blob with the manifest.
5. Keep standalone build, package, CLI, and compatibility changes on the
   reviewed allowlist in `metadata/adaptation-allowlist.json`.

The fixture-admission policy in `LIBRECAD_DXFRW_UPGRADE_PLAN.md` is mandatory:
only exact pre-lock repository blobs or genuinely local-from-scratch drawings
may be committed. External and derived DWG/DXF files stay outside Git.

## Verification record

Implementation evidence and the last successful standalone/LibreCAD consumer
checks are recorded in the live progress block of the upgrade plan and in
slice commit trailers. This file is updated only in the corresponding
plan-bearing slice commit.
