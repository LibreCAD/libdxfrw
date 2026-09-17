# LibreCAD system-package mode handoff

The pinned LibreCAD target at
`3c7785ebbcbfc8f3c8f79dbba093aff09cdec753` now has the opt-in integration in
clean target commit `6969e0a003414f9a7084349ac54bc2b32515e16b` on branch
`codex/libdxfrw-system-package`. The user's dirty LibreCAD checkout was not
modified.

The LibreCAD integration change should add an opt-in
`LIBRECAD_USE_SYSTEM_LIBDXFRW` CMake option. When enabled it must:

1. call `find_package(libdxfrw 2.0 CONFIG REQUIRED)` and link
   `libdxfrw::libdxfrw` to `librecad_lib`, `librecad_filter_compile_check`, and
   the `libdxfrw_system_fast_tests` target;
2. omit `libraries/libdxfrw/src` and `libraries/libdxfrw/src/intern` from
   `SHARED_INCLUDES`, and omit `${LIBDXFRW_ALL_FILES}` from `SHARED_SOURCES`;
3. preserve `DWGSUPPORT`, C++17, `/bigobj`, and the existing filter/test
   registration; and
4. run the filter compile check, public-only system fast tests, and the
   generic package consumer in this mode. The bundled parser-only targets are
   intentionally disabled in system mode because they include private headers
   that are not part of the installed consumer contract.

The gate is a compile-command audit: with the option enabled, no compile or
link command may contain `libraries/libdxfrw/src` or a bundled libdxfrw object.
The standalone package prefix `/private/tmp/libdxfrw-s16-prefix` was used for
the clean verification. The system-mode `librecad_lib` target built to 100%,
the focused CTest passed, the default bundled-mode filter compile passed, and
all 1,245 generated system-mode compile commands had zero bundled-path hits.
The package and all test inputs remain fixture-free.
