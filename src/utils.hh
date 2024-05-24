#ifndef UTILS_HH
#define UTILS_HH

#include <expected>
#include <format>

namespace rgs = std::ranges;
namespace vws = std::ranges::views;
using namespace std::literals;

using u8 = std::uint8_t;
using isz = std::make_signed_t<std::size_t>;
using usz = std::size_t;

#define LPAREN_ (
#define RPAREN_ )

/// Macro that propagates errors up the call stack.
///
/// The second optional argument to the macro, if present, should be an
/// expression that evaluates to a string that will be propagated up the
/// call stack as the error; the original error is in scope as `$`.
///
/// Example usage: Given
///
///     auto foo() -> Result<Bar> { ... }
///
/// we can write
///
///     Bar res = Try(foo());
///     Bar res = Try(foo(), std::format("Failed to do X: {}", $));
///
/// to invoke `foo` and propagate the error up the call stack, if there
/// is one; this way, we don’t have to actually write any verbose error
/// handling code.
///
/// (Yes, I know this macro is an abomination, but this is what happens
/// if you don’t have access to this as a language feature...)
#define Try(x, ...) ({                                                        \
    auto _res = x;                                                            \
    if (not _res) {                                                           \
        return std::unexpected(                                               \
            __VA_OPT__(                                                       \
                [&]([[maybe_unused]] std::string $) {                         \
            return __VA_ARGS__;                                               \
        }                                                                     \
            ) __VA_OPT__(LPAREN_) std::move(_res.error()) __VA_OPT__(RPAREN_) \
        );                                                                    \
    }                                                                         \
    *_res;                                                                    \
})

// clang-format on

template <typename Ty>
concept Reference = std::is_reference_v<Ty>;

template <typename Ty>
concept NotReference = not Reference<Ty>;

template <typename Ty>
struct ResultImpl;

template <Reference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<std::reference_wrapper<std::remove_reference_t<Ty>>, std::string>;
};

template <NotReference Ty>
struct ResultImpl<Ty> {
    using type = std::expected<Ty, std::string>;
};

template <typename Ty>
struct ResultImpl<std::reference_wrapper<Ty>> {
    using type = typename ResultImpl<Ty&>::type;
    static_assert(false, "Use Result<T&> instead of Result<reference_wrapper<T>>");
};

template <typename T = void>
using Result = typename ResultImpl<T>::type;

template <typename... Args>
auto Error(std::format_string<Args...> fmt, Args&&... args) -> std::unexpected<std::string> {
    return std::unexpected(std::format(fmt, std::forward<Args>(args)...));
}

std::string ToUTF8(std::u32string_view what);
std::u32string ToUTF32(std::string_view what);

#endif // UTILS_HH
