#include <xkb++/main.hh>
#include <clocale>
#include <print>

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "");
    auto res = Main(argc, argv);
    if (not res) {
        std::println(stderr, "{}", res.error());
        return 1;
    }
    return res.value();
}
