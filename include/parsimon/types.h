#ifndef PARSIMON_TYPES_H
#define PARSIMON_TYPES_H

#include <type_traits>
#include <iterator>

namespace parsimon {

/**
 * Empty type used to denote empty-ish results.
 */
struct empty_result {
    bool operator==(const empty_result&) const {return true;}
    bool operator!=(const empty_result&) const {return false;}
};

/**
 * Empty type used to denote the lack of optional parameters.
 */
struct no_arg {};

}

namespace parsimon::types {

template <typename T>
constexpr bool has_arg = !std::is_same_v<std::decay_t<T>, no_arg>;

template <typename T, typename... Ts>
using first_template_arg = T;

template <typename T, typename... Ts>
constexpr bool is_one_of = (std::is_same_v<T, Ts> || ...);

template <typename T>
constexpr bool is_string_literal_type = is_one_of<T, char, wchar_t, char16_t, char32_t>;

template <typename T>
using enable_if_string_literal_type = std::enable_if_t<is_string_literal_type<T>>;

template <typename Iterator, typename Tag>
constexpr bool iterator_is_category_v =
    std::is_same_v<typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category, Tag>;

template <typename... Parsers>
inline constexpr auto assert_parsers_not_empty() {
    static_assert(sizeof...(Parsers) > 0, "At least one parser must be provided");
}

template <typename State, typename Fn, typename V, typename... Ps>
inline constexpr void assert_functor_application_modify() {
    static_assert(std::is_invocable_v<Fn, V&, decltype(*apply(std::declval<Ps>(), std::declval<State>()))...>,
            "The provided functor is not invocable with expected arguments. Make sure that "
            "the first argument is a reference to the value to be modified, and that the rest "
            "of the arguments correspond to the number of parsers passed to the combinator (or "
            "a variadic parameter pack)");
}

template <typename State, typename Fn, typename... Ps>
inline constexpr void assert_functor_application() {
    static_assert(std::is_same_v<Fn, no_arg> || std::is_invocable_v<Fn, decltype(*apply(std::declval<Ps>(), std::declval<State>()))...>,
            "The provided functor is not invocable with expected arguments. Make sure that "
            "the number of arguments correspond to the number of parsers passed to the combinator "
            "(or a variadic parameter pack)");
}

template <typename T>
inline constexpr void assert_copyable_mreturn() {
    static_assert(std::is_copy_constructible_v<T>,
            "An argument to `mreturn` or `mreturn_emplace` is non-copyable, which is not allowed. "
            "For non-copyable argumens see the documentation for `mreturn` and `mreturn_emplace`");
}

}



#endif // PARSIMON_TYPES_H
