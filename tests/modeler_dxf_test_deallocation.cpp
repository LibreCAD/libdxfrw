// SPDX-License-Identifier: GPL-2.0-or-later
// The matching test operator new uses malloc and injects allocation failures.
// Keep its free-based counterparts in a separate translation unit so GCC does
// not mistake standard-library new expressions for mismatched allocation pairs.
#include <cstddef>
#include <cstdlib>

void operator delete(void *value) noexcept { std::free(value); }
void operator delete[](void *value) noexcept { std::free(value); }
void operator delete(void *value, std::size_t) noexcept { std::free(value); }
void operator delete[](void *value, std::size_t) noexcept { std::free(value); }
