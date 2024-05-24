#include <xkb++/xkbdisplay.hh>
#include <xkb++/xkbgen.hh>
#include <print>
#include <clocale>

auto InvokeMain(int argc, char** argv) -> Result<int> {
    if (argc < 2 or argv[0] == "xkbdisplay"sv) return xkbdisplay::Main(argc, argv);
    return xkbgen::Main(argc, argv);
}

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "");
    auto res = InvokeMain(argc, argv);
    if (not res) {
        std::println(stderr, "{}", res.error());
        return 1;
    }
}
