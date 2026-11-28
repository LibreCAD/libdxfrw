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

To compare the LAYOUT graph with an ODA conversion, separately convert the two
originals and the two resaves to ASCII DXF, preserving their base names in two
outside-repository directories. With `ezdxf` and LibreDWG installed, run:

```sh
python3 -B tools/research/audit_ac1012_layout_peer.py \
  --sample-dir /tmp/ac1012-distinct-pair \
  --oda-original-dir /tmp/ac1012-oda-original \
  --oda-resaved-dir /tmp/ac1012-oda-resaved \
  --dwgread /path/to/dwgread \
  --output /tmp/ac1012-layout-peer-audit.json
```

The audit checks the source hashes, LibreDWG's selected root, dictionary,
LAYOUT and BLOCK_HEADER identities, physical DXF BLOCK_RECORD backlinks before
document loading, and ezdxf's selected reciprocal links and audit result. The
supplied DXF files and their conversion provenance must be recorded separately;
the audit alone does not prove which converter produced them. In the recorded
run, ODA creates new layout identities for the unrooted originals, retains the
resaved layout identities and assigns a model viewport in both cases. Do not
interpret these transformations as proof of native semantics or lossless
conversion.

For a hash-bound AC1012 scratch sample, inspect an individual class against
the complete single-page physical object map with:

```sh
python3 -B tools/research/audit_ac1012_class_census.py \
  --source /tmp/sample.dwg --sha256 <source-sha256> \
  --dwgread /path/to/dwgread --class-name GEODATA \
  --output /tmp/ac1012-class-census.json
python3 -B tools/research/test_ac1012_class_census.py
```

This checks mapped frames and the physical CLASS section, not bytes omitted
from the map or the semantics of emitting the class to DXF. R13 class records
do not contain an instance-count field; a decoded default zero is not evidence
that no instance exists.

For native qualification, keep the drawings scratch-only and follow the
[existing native witness procedure](AC1012_DICTIONARY_NATIVE_WITNESS.md) on a
profiled AutoCAD host. Record the exact source hashes and producer revision,
normal-open and audit results, clone API and arguments, object identities,
merge outcomes and close/reopen behavior. Do not treat ODA conversion, peer
agreement, this wire audit, or the writer's own reopen as native clone proof.
