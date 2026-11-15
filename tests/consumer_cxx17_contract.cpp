#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <cstring>
#include <functional>
#include <ios>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

// Windows headers commonly precede the public API in consumer translation
// units. On other platforms, emulate their function-like macros so this
// compile-only check also runs in ordinary CI builds.
#ifdef _WIN32
#ifdef NOMINMAX
#undef NOMINMAX
#endif
#include <windows.h>
#else
#define min(a, b) ((a) < (b) ? (a) : (b))
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#include <libdxfrw.h>
#if !defined(min) || !defined(max)
#error "libdxfrw public headers must preserve caller min/max macros"
#endif
#undef min
#undef max

#if !defined(__cpp_lib_optional) || __cpp_lib_optional < 201606L
#error "The dxfrw public target must propagate its C++17 requirement"
#endif

static_assert(sizeof(DRW_Variant::ActiveDwgLayerReference) > 0,
              "public C++17 API must be visible to downstream consumers");
