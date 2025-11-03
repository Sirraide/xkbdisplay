#ifndef LAYOUT_HH
#define LAYOUT_HH

#include <array>
#include <base/Base.hh>
#include <functional>
#include <numeric>
#include <span>
#include <X11/X.h>

/// Libclopts unfortunately only accepts string literals in values<> options...
#define LAYOUT_NAME_ISO105 "iso-105"
#define LAYOUT_NAME_ANSI104 "ansi-104"

namespace layout {
using namespace base;
struct LayoutDescription {
    /// Name of the keyboard layout.
    std::string_view name;

    /// How many keys each row has.
    std::array<usz, 4> rows;

    /// Flat list of the labels of each key.
    std::u32string_view labels;

    /// Flat list of all key codes.
    std::initializer_list<KeyCode> codes;

    /// Get the number of keys that this layout has.
    [[nodiscard]] constexpr auto num_keys() const {
        return std::accumulate(rows.begin(), rows.end(), 0zu);
    }
};

inline constexpr LayoutDescription ISO105 { // clang-format off
    .name = LAYOUT_NAME_ISO105,
    .rows = {13, 12, 12, 11},
    .labels =
        U"¬1234567890-="
        U"QWERTYUIOP[]"
        U"ASDFGHJKL;'^"
        U"´ZXCVBNM,./",
    .codes = {
        49, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35,
        38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 51,
        94, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    },
}; // clang-format on

inline constexpr LayoutDescription ANSI104 { // clang-format off
    .name = LAYOUT_NAME_ANSI104,
    .rows = {13, 13, 11, 10},
    .labels =
        U"¬1234567890-="
        U"QWERTYUIOP[]\\"
        U"ASDFGHJKL;'"
        U"ZXCVBNM,./",
    .codes = {
        49, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21,
        24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 51,
        38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
        52, 53, 54, 55, 56, 57, 58, 59, 60, 61,
    },
}; // clang-format on
}
#endif // LAYOUT_HH
