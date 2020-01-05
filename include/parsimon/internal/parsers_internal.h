#ifndef PARSIMON_INTERNAL_PARSERS_INTERNAL_H
#define PARSIMON_INTERNAL_PARSERS_INTERNAL_H

#include <tuple>
#include <limits>
#include "algorithm.h"

namespace parsimon::internal {

/**
 * Parser for a single item
 */
template <bool ReturnArg = false, typename State, typename Predicate, typename Item = no_arg>
inline constexpr auto item(State& s, Predicate pred, const Item& i = no_arg()) {
    constexpr bool has_item_arg = types::has_arg<Item>;

    static_assert (has_item_arg || !ReturnArg,
            "If ReturnArg is `true` then `Item` cannot be `no_arg`");

    using return_type = std::conditional_t<ReturnArg, Item, decltype(s.front())>;
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
            if constexpr (ReturnArg) {
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

template <bool Eat = true, bool Include = false, typename State, typename ItemType>
inline constexpr auto until_item(State& s, const ItemType& c) {
    if (auto pos = algorithm::find(s.position, s.end, c); pos != s.end) {
        auto res_start = s.position;
        auto res_end = std::next(pos, Include);
        s.set_position(std::next(pos, Eat));
        return s.return_success(s.convert(res_start, res_end));
    } else {
        return s.return_fail();
    }
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
template <bool Eat = true, bool Include = false, typename State, typename Search>
inline constexpr auto until_seq(State& s, Search search) {
    if (auto [pos, new_end] = search(s.position, s.end); pos != s.end) {
        auto res_start = s.position;
        auto res_end = Include ? new_end : pos;
        s.set_position(Eat ? new_end : pos);
        return s.return_success(s.convert(res_start, res_end));
    } else {
        return s.return_fail();
    }
}

// General matching algorithm with supplied equality functions.
template <size_t StartLength,
          size_t EndLength,
          bool Nested = false,
          bool Include = false,
          typename Start,
          typename End,
          typename EqualStart,
          typename EqualEnd>
inline constexpr auto between_general(Start start, End end, EqualStart equal_start, EqualEnd equal_end) {
    return parser([=](auto& s) {
        if (s.at_end() || !equal_start(s.position, std::next(s.position, StartLength), start))
            return s.return_fail();

        size_t to_match = 0;
        for (auto pos = std::next(s.position, StartLength); algorithm::contains_elements(pos, s.end, EndLength);) {
            if (equal_end(pos, std::next(pos, EndLength), end)) {
                if (to_match == 0) {
                    auto begin_iterator = std::next(s.position, (Include ? 0 : StartLength));
                    auto result_end = std::next(pos, (Include ? EndLength : 0));
                    s.set_position(std::next(pos, EndLength));
                    return s.return_success(s.convert(begin_iterator, result_end));
                } else if (Nested) {
                    --to_match;
                    std::advance(pos, EndLength);
                }
            } else if (Nested && equal_start(pos, std::next(pos, StartLength), start)) {
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
