#ifndef LAYOUT_HH
#define LAYOUT_HH

#include <array>
#include <functional>
#include <span>
#include <X11/X.h>
#include <xkb++/utils.hh>

/// Libclopts unfortunately only accepts string literals in values<> options...
#define LAYOUT_NAME_ISO105 "iso-105"

struct ISO105Traits {
    static constexpr std::array<usz, 4> Rows = {13, 12, 12, 11};
    static constexpr usz KeyCount = 13 + 12 + 12 + 11;
    static constexpr std::u32string_view KeyLabels = U"¬1234567890-=QWERTYUIOP[]ASDFGHJKL;'^´ZXCVBNM,./";
    static constexpr std::array<KeyCode, KeyCount> KeyCodes{// clang-format off
        49, 10, 11, 12, 13, 14, 15, 16, 17, 18,
        19, 20, 21, 24, 25, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 38, 39, 40, 41, 42,
        43, 44, 45, 46, 47, 48, 51, 94, 52, 53,
        54, 55, 56, 57, 58, 59, 60, 61
    }; // clang-format on

    /// Get the XKB name for a key at a given row and index.
    static auto KeyName(isz row, isz idx) -> std::string;
};

/// The ISO 105-key keyboard layout.
template <typename KeyTy>
class ISO105 {
public:
    using KeyType = KeyTy;
    using Traits = ISO105Traits;

private:
    std::array<KeyType, Traits::KeyCount> key_array;

public:
    /// Get a range that iterates over all keys.
    auto keys() -> std::span<KeyType> { return key_array; }

    /// Get a range that iterates over all rows.
    template <typename Callable>
    void for_all_rows(Callable cb) {
        usz start = 0;
        for (auto row : Traits::Rows) {
            std::invoke(cb, std::span<KeyType>(key_array.data() + start, row));
            start += row;
        }
    }

    /// Get a key at a given row and index.
    auto operator[](usz row, usz idx) -> Result<KeyType&> {
        switch (row) {
            case 3: idx += Traits::Rows[2]; [[fallthrough]];
            case 2: idx += Traits::Rows[1]; [[fallthrough]];
            case 1: idx += Traits::Rows[0]; [[fallthrough]];
            case 0: return At(key_array, idx);
            default: return Error("Invalid row index {}@{}", row, idx);
        }
    }
};

#endif // LAYOUT_HH
