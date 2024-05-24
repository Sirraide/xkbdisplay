#include <codecvt>
#include <xkb++/utils.hh>

auto ToUtf8(std::u32string_view sv) -> std::string {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.to_bytes(sv.data(), sv.data() + sv.size());
}

auto ToUtf8(char32_t sv) -> std::string {
    return ToUtf8(std::u32string_view{&sv, 1});
}

auto ToUtf32(std::string_view sv) -> std::u32string {
    std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
    return conv.from_bytes(sv.data(), sv.data() + sv.size());
}
