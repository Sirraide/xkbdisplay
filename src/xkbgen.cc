#include <algorithm>
#include <array>
#include <charconv>
#include <clopts.hh>
#include <generator>
#include <print>
#include <ranges>
#include <stream/stream.hh>
#include <xkb++/utils.hh>
#include <xkbcommon/xkbcommon.h>
#include <xkb++/xkbgen.hh>

#define LAYOUT_ISO_105 "iso-105"
constexpr usz LAYER_COUNT = 8;

namespace detail {
using namespace command_line_options;
using options = clopts<
    positional<"file", "The file to translate to a keymap", file<std::string, std::string>>,
    positional<"layout", "The keyboard layout format to use", values<LAYOUT_ISO_105>>,
    option<"-o", "Output file name">,
    help<>>;
}

// ============================================================================
//  Layout Definition
// ============================================================================
/// A single physical key on a keyboard layout.
struct Key {
    std::vector<std::u32string> symbols;
};

/// The ISO 105-key keyboard layout.
class ISO105 {
    std::array<Key, 13> row1;
    std::array<Key, 12> row2;
    std::array<Key, 12> row3;
    std::array<Key, 11> row4;

public:
    /// Get the XKB name for a key at a given row and index.
    auto key_name(isz row, isz idx) -> std::string;

    /// Get a range that iterates over all rows.
    auto rows() -> std::generator<std::span<Key>>;

    /// Get a key at a given row and index.
    auto operator[](usz row, usz idx) -> Result<Key&>;
};

class Layout {
    ISO105 keys;

public:
    /// Write the layout to a file.
    auto emit(FILE* out) -> Result<>;

    /// Get the keyboard layout.
    auto raw_layout() -> ISO105& { return keys; }

    /// Parse a keyboard layout from a string.
    static auto Parse(std::string_view text) -> Result<Layout>;
};

// ============================================================================
//  Helpers
// ============================================================================
template <typename Container>
auto At(Container&& c, usz index) -> Result<typename std::remove_cvref_t<Container>::value_type&> {
    if (index >= c.size()) return Error("Index {} out of bounds", index);
    return std::ref(std::forward<Container>(c)[index]);
}

auto KeySymName(std::u32string_view sym) -> std::string {
    // Symbol is the empty symbol.
    if (sym.empty()) return "NoSymbol";

    // Symbol is the name of a symbol.
    if (sym.size() != 1) return ToUtf8(sym);

    // Symbol is a single keysym. It may have a name in XKB.
    std::array<char, 1'024> buf;
    auto x = xkb_utf32_to_keysym(sym.front());
    auto sz = xkb_keysym_get_name(x, buf.data(), buf.size());

    // It does not.
    if (sz == -1) return std::format("U{:04X}", usz(sym.front()));

    // It does.
    return std::string{buf.data(), usz(sz)};
}

// ============================================================================
//  Layout Implementation
// ============================================================================
auto ISO105::key_name(isz row, isz idx) -> std::string {
    switch (row) {
        default: Unreachable();
        case 0: {
            if (idx == 0) return "TLDE";
            return std::format("AE{:02}", idx);
        }

        case 1: return std::format("AD{:02}", idx + 1);
        case 2: {
            if (idx == 12) return "BKSL";
            return std::format("AC{:02}", idx + 1);
        }

        case 3: {
            if (idx == 0) return "LSGT";
            return std::format("AB{:02}", idx + 1);
        }
    }
}

auto ISO105::rows() -> std::generator<std::span<Key>> {
    co_yield row1;
    co_yield row2;
    co_yield row3;
    co_yield row4;
}

auto ISO105::operator[](usz row, usz idx) -> Result<Key&> {
    switch (row) {
        case 0: return At(row1, idx);
        case 1: return At(row2, idx);
        case 2: return At(row3, idx);
        case 3: return At(row4, idx);
        default: return Error("Invalid row index {}@{}", row, idx);
    }
}

auto Layout::emit(FILE* o) -> Result<> {
    // Write header.
    std::println(o, "default xkb_symbols \"basic\" {{");
    std::println(o, "    name[Group1]=\"English with IPA\";");
    std::println(o);
    std::println(o, "    key.type[Group1] = \"EIGHT_LEVEL\";");

    // Write keys.
    for (auto [row, keys_in_row] : keys.rows() | vws::enumerate) {
        for (auto [col, key] : keys_in_row | vws::enumerate) {
            bool first = true;
            std::print(o, "    key <{}> {{ [", keys.key_name(row, col));
            for (const auto& sym : key.symbols) {
                if (first) first = false;
                else std::print(o, ", ");
                std::print(o, "{}", KeySymName(sym));
            }

            // Pad with empty symbol to level count.
            for (usz i = key.symbols.size(); i < LAYER_COUNT; i++)
                std::print(o, ", {}", KeySymName(U""));

            std::println(o, "] }};");
        }
    }

    // Write modifier keys.
    std::println(o);
    std::println(o, "    key.type[Group1] = \"ONE_LEVEL\";");
    std::println(o, "    key <RALT> {{ [ ISO_Level3_Shift ] }};");
    std::println(o, "    key <MENU> {{ [ ISO_Level5_Shift ] }};");
    std::println(o);
    std::println(o, "    include \"level3(ralt_switch)\"");
    std::println(o, "    include \"level5(menu_switch)\"");
    std::println(o, "}};");
    return {};
}

auto Layout::Parse(std::string_view text) -> Result<Layout> {
    Layout L;
    using Stream = streams::u32stream;

    static constexpr auto Digit = [](char32_t c) -> Result<usz> {
        if (U'0' <= c and c <= U'9') return usz(c - U'0');
        if (U'A' <= c and c <= U'Z') return usz(c - U'A' + 10);
        if (U'a' <= c and c <= U'z') return usz(c - U'a' + 10);
        return Error("Invalid digit '{}'", ToUtf8(c));
    };

    // Remove comments.
    auto converted = ToUtf32(text);
    std::u32string text_no_comments;
    for (auto l : Stream{converted}.lines()) {
        l = l.take_until([prev = char32_t(0)](char32_t c) mutable {
            if (c == '#' and prev != '"' and prev != '\'') return true;
            prev = c;
            return false;
        });

        if (l.empty()) continue;
        text_no_comments += l.trim().text();
        text_no_comments += '\n';
    }

    Stream s{text_no_comments};
    while (not s.empty()) {
        // Read key row and index.
        char32_t row, key;
        if (not s.extract(row, key)) return Error("Expected key row and index");

        // Read '=' and '['.
        s.trim_front();
        if (not s.consume('=')) return Error("Expected '=' after key index");
        s.trim_front();
        if (not s.consume('[')) return Error("Expected '[' before key symbols");
        s.trim_front();
        // Read keys.
        std::vector<std::u32string> symbol;
        do {
            std::u32string_view sym;
            if (not s.take_delimited_any(sym, U"'\"")) sym = s.take_until_any(U" \r\n\t\v\f]");
            symbol.emplace_back(Stream{sym}.trim().text());
            s.trim_front();
        } while (not s.empty() and not s.starts_with(']'));
        if (s.empty()) return Error("Expected ']' after key symbols");
        s.drop();

        // Add key.
        Key& entry = Try((L.keys[Try(Digit(row)), Try(Digit(key))]));
        entry.symbols = std::move(symbol);
        s.trim_front();
    }

    return L;
}

auto Main(detail::options::optvals_type& opts) -> Result<> {
    auto file = opts.get<"file">();
    auto output = opts.get_or<"-o">("-");
    auto o = output == "-"sv ? stdout : fopen(output.data(), "w");
    if (not o) return Error("Failed to open output file '{}'", output);
    auto layout = Try(Layout::Parse(file->contents));
    return layout.emit(o);
}
