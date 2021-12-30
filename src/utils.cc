#include "utils.h"

std::string ToUTF8(const std::u32string& what) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.to_bytes(what);
}

std::u32string ToUTF32(const std::string& what) {
	std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
	return conv.from_bytes(what);
}
