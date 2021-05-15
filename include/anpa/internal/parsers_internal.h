#ifndef PARSIMON_INTERNAL_PARSERS_INTERNAL_H
#define PARSIMON_INTERNAL_PARSERS_INTERNAL_H

#include <tuple>
#include <limits>
#include "anpa/internal/algorithm.h"
#include "anpa/options.h"

namespace anpa::internal {

/**
 * Parser for a single item
 */
template <options Options = options::none, typename State, typename Predicate, typename Item = no_arg>
inline constexpr auto item(State& s, Predicate pred, const Item& i = no_arg()) {
    constexpr bool has_item_arg = types::has_arg<Item>;
    constexpr bool return_arg = has_options(Options, options::return_arg);
    static_assert (has_item_arg || !return_arg,
            "If return_arg is `true` then `Item` cannot be `no_arg`");

    using return_type = std::conditional_t<return_arg, Item, decltype(s.front())>;
    if (!s.at_end()) {
        const auto& front = s.front();
        bool success = [pred, &front, &i]() {
            if constexpr (has_item_arg) {
                return pred(front, i);
            } else {
                return pred(front);
            }
        }();
        if (success) {
            s.advance(1);
            if constexpr (return_arg) {
                return s.return_success(i);
            } else {
                return s.return_success(front);
            }
        }
    }
    return s.template return_fail<return_type>();
}

template <typename State, typename Length>
inline constexpr auto consume(State& s, const Length& l) {
    if (s.has_at_least(l)) {
        auto start_pos = s.position;
        s.advance(l);
        return s.return_success(s.convert(start_pos, s.position));
    }
    return s.return_fail();
}

template <options Options, typename State, typename ItemType>
inline constexpr auto until_item(State& s, const ItemType& c) {
    constexpr bool include = has_options(Options, options::include);
    constexpr bool dont_eat = has_options(Options, options::dont_eat);
    if (auto pos = algorithm::find(s.position, s.end, c); pos != s.end) {
        auto res_start = s.position;
        auto res_end = std::next(pos, include);
        s.set_position(std::next(pos, !dont_eat));
        return s.return_success(s.convert(res_start, res_end));
    } else {
        return s.return_fail();
    }
}

template <options Options, typename Predicate>
inline constexpr auto while_if(Predicate predicate) {
    return parser([=](auto& s) {
        auto start_pos = s.position;
        auto result = [&]() {
            if constexpr (has_options(Options, options::negate)) {
                return algorithm::find_if(start_pos, s.end, predicate);
            } else {
                return algorithm::find_if_not(start_pos, s.end, predicate);
            }
        }();
        if constexpr (has_options(Options, options::fail_on_no_parse)) {
            if (result == start_pos) return s.return_fail();
        }
        s.set_position(result);
        return s.return_success(s.convert(start_pos, result));
    });
}

/**
 * Helper for parsing of sequences
 */
template <typename State, typename Eq>
inline constexpr auto seq(State& s, size_t size, Eq equal) {
    auto orig_pos = s.position;
    if (s.has_at_least(size) && equal(orig_pos)) {
        s.advance(size);
        return s.return_success(s.convert(orig_pos, size));
    } else {
        return s.return_fail();
    }
}

/**
 * Helper for parsing until a sequence
 */
template <options Options, typename State, typename Search>
inline constexpr auto until_seq(State& s, Search search) {
    if (auto [pos, new_end] = search(s.position, s.end); pos != s.end) {
        auto res_start = s.position;
        auto res_end = has_options(Options, options::include) ? new_end : pos;
        s.set_position(has_options(Options, options::dont_eat) ? pos : new_end);
        return s.return_success(s.convert(res_start, res_end));
    } else {
        return s.return_fail();
    }
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength,
          size_t EndLength,
          options Options,
          typename Start,
          typename End,
          typename EqualStart,
          typename EqualEnd>
inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto& s) {
        constexpr bool include = has_options(Options, options::include);
        constexpr bool nested = has_options(Options, options::nested);
        if (s.at_end() || !equal_start(s.position, std::next(s.position, StartLength), start))
            return s.return_fail();

        size_t to_match = 0;
        for (auto pos = std::next(s.position, StartLength); algorithm::contains_elements(pos, s.end, EndLength);) {
            if (equal_end(pos, std::next(pos, EndLength), end)) {
                if (to_match == 0) {
                    auto begin_iterator = std::next(s.position, (include ? 0 : StartLength));
                    auto result_end = std::next(pos, (include ? EndLength : 0));
                    s.set_position(std::next(pos, EndLength));
                    return s.return_success(s.convert(begin_iterator, result_end));
                } else if (nested) {
                    --to_match;
                    std::advance(pos, EndLength);
                }
            } else if (nested && equal_start(pos, std::next(pos, StartLength), start)) {
                ++to_match;
                std::advance(pos, StartLength);
            } else {
                ++pos;
            }
        }
        return s.return_fail();
    });
}

template <typename State, typename Result>
inline constexpr auto custom(State& s, Result&& result) {
    s.set_position(std::get<0>(std::forward<Result>(result)));
    if (std::get<1>(result)) {
        return s.return_success(*std::forward<Result>(result).second);
    } else {
        return s.template return_fail<decltype(*result.second)>();
    }
}

}

#endif // PARSIMON_INTERNAL_PARSERS_INTERNAL_H
