#ifndef UTILS_H
#define UTILS_H

#include <codecvt>
#include <cstdlib>
#include <iostream>
#include <locale>

#define DIE_PRE_ERR_STR "\033[1;31mError:\033[1;37m " //__FILE__, ":", __LINE__, ": \033[1;31mError:\033[1;37m "

#define ConstexprNotImplemented(msg)                    \
	[]<bool flag = false> { static_assert(flag, msg); } \
	();

template <typename T>
[[noreturn]] void Die(T t) noexcept {
	std::cerr << DIE_PRE_ERR_STR << t << "\033[m\n";
	exit(1);
}

template <typename T>
[[noreturn]] void _Die(T t) noexcept {
	std::cerr << t << "\033[m\n";
	exit(1);
}

template <typename T, typename... Ts>
[[noreturn]] void _Die(T t, Ts... ts) noexcept {
	std::cerr << t;
	_Die(ts...);
}

template <typename T, typename... Ts>
[[noreturn]] void Die(T t, Ts... ts) noexcept {
	std::cerr << DIE_PRE_ERR_STR << t;
	_Die(ts...);
}

template <>
auto Verify(auto* ptr, const std::string& msg = "Pointer was NULL") -> decltype(ptr) {
	if (!ptr) Die(msg);
	return ptr;
}

std::string	   ToUTF8(const std::u32string& what);
std::u32string ToUTF32(const std::string& what);

#endif /* UTILS_H */
