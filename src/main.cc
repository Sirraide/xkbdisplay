#include <xkb++/xkbdisplay.hh>
#include <xkb++/xkbgen.hh>
#include <print>

auto InvokeMain(int argc, char** argv) -> Result<int> {
    if (argc < 2 or argv[0] == "xkbdisplay") return xkbdisplay::Main(argc, argv);
    return xkbgen::Main(argc, argv);
}

int main(int argc, char** argv) {
    auto res = InvokeMain(argc, argv);
    if (not res) {
        std::println(stderr, "{}", res.error());
        return 1;
    }
}
