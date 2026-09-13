# LibreCAD system-package mode handoff

This handoff is intentionally separate from the standalone `libdxfrw`
repository. The inspected LibreCAD target at
`3c7785ebbcbfc8f3c8f79dbba093aff09cdec753` still compiles
`libraries/libdxfrw/src` directly through `SHARED_SOURCES` and
`SHARED_INCLUDES`; this repository cannot safely rewrite the user's dirty
LibreCAD checkout.

The LibreCAD integration change should add an opt-in
`LIBRECAD_USE_SYSTEM_LIBDXFRW` CMake option. When enabled it must:

1. call `find_package(libdxfrw 2.0 CONFIG REQUIRED)` and link
   `libdxfrw::libdxfrw` to `librecad_lib`, `librecad_filter_compile_check`, and
   the libdxfrw fast-test targets;
2. omit `libraries/libdxfrw/src` and `libraries/libdxfrw/src/intern` from
   `SHARED_INCLUDES`, and omit `${LIBDXFRW_ALL_FILES}` from `SHARED_SOURCES`;
3. preserve `DWGSUPPORT`, C++17, `/bigobj`, and the existing filter/test
   registration; and
4. run the filter compile check, `libdxfrw_test_core` replacements, and the
   generic package consumer in this mode.

The gate is a compile-command audit: with the option enabled, no compile or
link command may contain `libraries/libdxfrw/src` or a bundled libdxfrw object.
The standalone package prefix and all test inputs remain fixture-free.
