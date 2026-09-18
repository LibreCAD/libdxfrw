# DWG read-test fixtures

Each subdirectory holds `.dwg` samples for one AC version. Tests in
`tests/CMakeLists.txt` glob `samples/<AC_VERSION>/*.dwg` at configure time
and register one CTest per sample.

A sample only belongs in a version directory if its first 6 bytes literally
match the directory name — the test harness validates this and fails fast on
a mis-versioned fixture.

## AC1024 (R2010)

Currently-passing AutoCAD 2010 samples used to guard against regressions in
the `dwgReader24` code path.

## AC1027 (R2013)

Empty — drop AC1027 `.dwg` files here to enable the AC1027 test suite.
The `dwgReader27` code path is currently a thin pass-through over
`dwgReader18` (R2004 page format) and will benefit from coverage as
real-world R2013 deltas surface.

## Adding new samples

1. Verify the file's AC version: `head -c 6 myfile.dwg`
2. Drop it into the matching `samples/<AC_VERSION>/` directory.
3. Re-run CMake configure; the new test registers automatically.
4. Run `ctest -R dwg_read --output-on-failure`.

Do **not** add files that fail today: the suite is meant to prevent
regression of currently-working files. To reproduce a known failure, write
a separate `xfail`-style test (not yet supported here).

Note on AC1028: there is no AC1028 DWG version. Real version codes after
AC1024 are AC1027 (R2013) and AC1032 (R2018). If samples for AC1032 land
later, add a `samples/AC1032/` directory and a matching entry to
`_dwg_test_versions` in `tests/CMakeLists.txt`.
