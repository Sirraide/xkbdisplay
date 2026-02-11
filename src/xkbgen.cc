#include <algorithm>
#include <array>
#include <charconv>
#include <clopts.hh>
#include <print>
#include <ranges>

#include <base/Base.hh>
#include <base/Text.hh>

#include <xkb++/layout.hh>
#include <xkb++/main.hh>
#include <xkbcommon/xkbcommon.h>

using namespace base;
using namespace layout;

constexpr usz LAYER_COUNT = 8;

// ============================================================================
//  Layout Definition
// ============================================================================
class ParsedLayout {
    struct Record {
        std::u32string name;
        std::vector<std::u32string> symbols;
    };

    std::vector<Record> records;

public:
    /// Write the layout to a file.
    auto emit(FILE* out, std::string_view name) -> Result<>;

    /// Parse a keyboard layout from a string.
    static auto Parse(std::string_view text) -> Result<ParsedLayout>;
};

// ============================================================================
//  Helpers
// ============================================================================
auto KeySymName(std::u32string_view sym) -> std::string {
    // Symbol is the empty symbol.
    if (sym.empty()) return "NoSymbol";

    // Symbol is the name of a symbol.
    if (sym.size() != 1) return text::ToUTF8(sym);

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
auto ParsedLayout::emit(FILE* o, std::string_view kb_name) -> Result<> {
    // Write header.
    std::println(o, "default xkb_symbols \"basic\" {{");
    std::println(o, "    name[Group1]=\"{}\";", kb_name);
    std::println(o);
    std::println(o, "    key.type[Group1] = \"EIGHT_LEVEL\";");

    for (auto& [name, symbols] : records)  {
        std::print(o, "    key <{}> {{ [", name);
        bool first = true;
        for (const auto& sym : symbols) {
            if (first) first = false;
            else std::print(o, ", ");
            std::print(o, "{}", KeySymName(sym));
        }

        // Pad with empty symbol to level count.
        for (usz i = symbols.size(); i < LAYER_COUNT; i++)
            std::print(o, ", {}", KeySymName(U""));

        std::println(o, "] }};");
    }

    // Write modifier keys.
    std::println(o);
    std::println(o, "    key.type[Group1] = \"ONE_LEVEL\";");
    std::println(o, "    key <RALT> {{ [ ISO_Level3_Shift ] }};");
    std::println(o, "    key <RWIN> {{ [ ISO_Level5_Shift ] }};");
    std::println(o, "    key <MENU> {{ [ ISO_Level5_Shift ] }};");
    std::println(o);
    std::println(o, "    include \"level3(ralt_switch)\"");
    std::println(o, "    include \"level5(menu_switch)\"");
    std::println(o, "}};");
    return {};
}

auto ParsedLayout::Parse(std::string_view text) -> Result<ParsedLayout> {
    ParsedLayout layout;

    // Remove comments.
    auto converted = text::ToUTF32(text);
    std::u32string text_no_comments;
    for (auto l : str32{converted}.lines()) {
        l = l.take_until([prev = char32_t(0)](char32_t c) mutable {
            if (c == '#' and prev != '"' and prev != '\'') return true;
            prev = c;
            return false;
        });

        if (l.empty()) continue;
        text_no_comments += l.trim().text();
        text_no_comments += '\n';
    }

    str32 s{text_no_comments};
    while (not s.trim_front().empty()) {
        if (not s.consume('<')) return Error("Expected '<' at start of key name");

        // Read symbolic key name.
        std::u32string name{s.take_until('>')};
        if (not s.consume('>')) return Error("Expected '>' at end of key name");

        // Read '=' and '['.
        s.trim_front();
        if (not s.consume('=')) return Error("Expected '=' after key index");
        s.trim_front();
        if (not s.consume('[')) return Error("Expected '[' before key symbols");
        s.trim_front();

        // Read keys.
        std::vector<std::u32string> symbol;
        do {
            str32 sym;
            if (not s.take_delimited_any(sym, U"'\"")) sym = s.take_until_any(U" \r\n\t\v\f]");
            symbol.emplace_back(str32{sym}.trim().text());
            s.trim_front();
        } while (not s.empty() and not s.starts_with(']'));
        if (s.empty()) return Error("Expected ']' after key symbols");
        s.drop();

        layout.records.emplace_back(std::move(name), std::move(symbol));
    }

    return layout;
}

auto Main(int argc, char** argv) -> Result<int> {
    using namespace command_line_options;
    using options = clopts<
        positional<"file", "The file to translate to a keymap", file<std::string, std::string>>,
        positional<"name", "The name of the kayout">,
        option<"-o", "Output file name">,
        help<>>;

    auto opts = options::parse(argc, argv);
    auto file = opts.get<"file">();
    auto output = opts.get<"-o">("-");
    auto name = opts.get<"name">();
    auto o = output == "-"sv ? stdout : fopen(output.data(), "w");
    if (not o) return Error("Failed to open output file '{}'", output);
    auto layout = Try(ParsedLayout::Parse(file->contents));
    Try(layout.emit(o, *name));
    return 0;
}
