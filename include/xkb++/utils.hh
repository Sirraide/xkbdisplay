#ifndef UTILS_HH
#define UTILS_HH

#include <expected>
#include <format>
#include <libassert/assert.hpp>
#include <type_traits>

namespace rgs = std::ranges;
namespace vws = std::ranges::views;
using namespace std::literals;

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using usz = std::size_t;

using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using isz = std::make_signed_t<std::size_t>;

#define LPAREN_ (
#define RPAREN_ )

#define Assert(cond, ...) LIBASSERT_ASSERT(cond __VA_OPT__(, std::format(__VA_ARGS__)))
#define Unreachable(...)  LIBASSERT_UNREACHABLE(__VA_OPT__(std::format(__VA_ARGS__)))

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
    using ResTy = std::remove_reference_t<decltype(*_res)>;                   \
    static_cast<typename ::detail::TryResultType<ResTy>::type>(*_res);        \
})
// clang-format on

// My IDE doesn’t know about __builtin_expect_with_probability, for some reason.
#if !__has_builtin(__builtin_expect_with_probability)
#    define __builtin_expect_with_probability(x, y, z) __builtin_expect(x, y)
#endif

namespace detail {
template <typename Ty>
struct TryResultType {
    using type = std::remove_reference_t<Ty>&&;
};

template <typename Ty>
requires std::is_void_v<Ty>
struct TryResultType<Ty> {
    using type = void;
};

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
}

template <typename T = void>
using Result = typename detail::ResultImpl<T>::type;

template <typename... Args>
[[nodiscard]] auto Error(std::format_string<Args...> fmt, Args&&... args) -> std::unexpected<std::string> {
    return std::unexpected(std::format(fmt, std::forward<Args>(args)...));
}

template <typename Container>
auto At(Container&& c, usz index) -> Result<typename std::remove_cvref_t<Container>::value_type&> {
    if (index >= c.size()) return Error("Index {} out of bounds", index);
    return std::ref(std::forward<Container>(c)[index]);
}

[[nodiscard]] bool IsDiacritic(char32_t c);
[[nodiscard]] auto ToUtf8(char32_t sv) -> std::string;
[[nodiscard]] auto ToUtf8(std::u32string_view what) -> std::string;
[[nodiscard]] auto ToUtf32(std::string_view what) -> std::u32string;

#endif // UTILS_HH
