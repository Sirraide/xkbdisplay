#ifndef GEN_HH
#define GEN_HH

#include <clopts.hh>
#include <expected>
#include <utils.hh>

#define LAYOUT_ISO_105 "iso-105"
constexpr usz LAYER_COUNT = 8;

// clang-format off
namespace xkbgen {
using namespace command_line_options;
using options = clopts<
	positional<"file", "The file to translate to a keymap", file<std::string, std::string>>,
	positional<"layout", "The keyboard layout format to use", values<LAYOUT_ISO_105>>,
	option<"-o", "Output file name">,
	help<>
>;

auto Main(int argc, char**argv) -> Result<int>;
} // clang-format on

#endif // GEN_HH
