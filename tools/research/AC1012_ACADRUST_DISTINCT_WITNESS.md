# AC1012 independent-writer dictionary witness

This scratch-only witness supplements the existing ODA and LibreDWG-generated
pairs. It creates two AC1012 drawings with one shared `SAME_KEY` MLINESTYLE
(15° versus 37°), one side-specific style, a model-space LINE and two elements
per selected style. The generator also reopens and resaves each drawing with
the same writer. None of these generated drawings establishes AutoCAD normal
open, duplicate-record cloning, or native dictionary semantics.

Use a clean checkout of [acadrust](https://github.com/hakanaktt/acadrust) at
commit `d7c71f5032fc2d8844142ad7df7a6f0ee5ac535f`, Cargo/Rust, and an existing
LibreDWG `dwgread`. Keep the output outside this repository:

```sh
python3 -B tools/research/generate_ac1012_acadrust_pair.py \
  --acadrust-root /path/to/clean/acadrust \
  --output-dir /tmp/ac1012-distinct-pair
python3 -B tools/research/audit_ac1012_acadrust_pair.py \
  --manifest /tmp/ac1012-distinct-pair/manifest.json \
  --dwgread /path/to/dwgread \
  --output /tmp/ac1012-distinct-audit.json
```

The output directory must be empty. The generator records the actual Cargo lock,
compiler versions, source revision, generator hash and four drawing hashes. For
repeatability, retain its `Cargo.lock` and supply `--cargo-lock /path/to/Cargo.lock`
on a later run; Cargo then uses `--locked`. `--offline` is available when all
pinned dependencies are already cached, and `--cargo-target-dir` can reuse an
external build directory.

The audit checks the supplied manifest and exact sample hashes, uses LibreDWG
for independent object location and decoding, and applies the shared bounded
R13 frame, CRC, dictionary, MLINESTYLE and handle-role walkers. Selected
wrong-role and truncation controls must fail, followed by a successful retry.
The original and same-writer resaved selected style frames must agree. The
resave adds an `ACAD_LAYOUT` root entry in this exact producer profile; it does
not prove that graph's validity or authorize a projection.

For native qualification, keep the drawings scratch-only and follow the
[existing native witness procedure](AC1012_DICTIONARY_NATIVE_WITNESS.md) on a
profiled AutoCAD host. Record the exact source hashes and producer revision,
normal-open and audit results, clone API and arguments, object identities,
merge outcomes and close/reopen behavior. Do not treat ODA conversion, peer
agreement, this wire audit, or the writer's own reopen as native clone proof.
