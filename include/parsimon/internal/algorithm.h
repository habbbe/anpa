#ifndef PARSIMON_INTERNAL_ALGORITHM_H
#define PARSIMON_INTERNAL_ALGORITHM_H

#include <algorithm>
#include <iterator>
#include "parsimon/types.h"

/**
 * constexpr variants of some algorithms
 */
namespace parsimon::algorithm {

/**
 * constexpr version of `std::equal` (1)
 */
template <typename InputIt1, typename InputIt2>
inline constexpr auto equal(InputIt1 begin1, InputIt1 end1, InputIt2 begin2) {
    for (; begin1 != end1; ++begin1, ++begin2) {
        if (!(*begin1 == *begin2))
            return false;
    }
    return true;
}

/**
 * constexpr version of `std::equal` (5)
 */
template <typename InputIt1, typename InputIt2>
inline constexpr auto equal(InputIt1 begin1, InputIt1 end1, InputIt2 begin2, InputIt2 end2) {

    constexpr bool It1RAI = types::iterator_is_category_v<InputIt1, std::random_access_iterator_tag>;
    constexpr bool It2RAI = types::iterator_is_category_v<InputIt2, std::random_access_iterator_tag>;

    if constexpr (It1RAI && It2RAI) {
        return std::distance(begin1, end1) == std::distance(begin2, end2) && equal(begin1, end1, begin2);
    } else {
        for (; begin1 != end1 && begin2 != end2; ++begin1, ++begin2) {
            if (!(*begin1 == *begin2))
                return false;
        }
        return begin1 == end1 && begin2 == end2;
    }
}

/**
 * Like the other `equal` (1) but the first range is given by the template parameters.
 */
template <auto... vs, typename InputIt>
inline constexpr auto equal(InputIt begin) {
    return ((*begin++ == vs) && ...);
}

/**
 * Like the other `equal` (5) but the first range is given by the template parameters.
 */
template <auto... vs, typename InputIt>
inline constexpr auto equal(InputIt begin, InputIt end) {
    return ((begin != end && *begin++ == vs) && ...);
}

/**
 * constexpr version of `std::find_if`
 */
template <typename InputIt, typename Predicate>
inline constexpr auto find_if(InputIt begin, InputIt end, Predicate p) {
    for (;begin != end; ++begin) {
        if (p(*begin)) return begin;
    }
    return end;
}

/**
 * constexpr version of `std::find_if_not`
 */
template <typename InputIt, typename Predicate>
inline constexpr auto find_if_not(InputIt begin, InputIt end, Predicate p) {
    return algorithm::find_if(begin, end, [=](const auto& val){return !p(val);});
}

/**
 * constexpr version of `std::find`
 */
template <typename InputIt, typename Element>
inline constexpr auto find(InputIt begin, InputIt end, const Element& element) {
    return algorithm::find_if(begin, end, [&](const auto& val){return val == element;});
}

/**
 * Check if the supplied value is contained within the range described by [begin, end).
 */
template <typename InputIt, typename V>
inline constexpr auto contains(InputIt begin, InputIt end, const V& needle) {
    for (;begin != end; ++begin)
        if (*begin == needle) return true;
    return false;
}

/**
 * Check if the supplied value is contained within the given template parameters.
 */
template <auto... vs, typename V>
inline constexpr auto contains(const V& sought) {
    return ((vs == sought) || ...);
}

/**
 * Find a sub-sequence. Returns a pair of iterators, with (begin, end) if the sequence is found,
 * otherwise (end1, end1).
 */
template <typename InputIt1, typename InputIt2>
inline constexpr std::pair<InputIt1, InputIt1> search(InputIt1 begin1, InputIt1 end1, InputIt2 begin2, InputIt2 end2) {
    for (;;++begin1) {
        InputIt1 b1 = begin1;
        for (InputIt2 b2 = begin2; ; ++b1, ++b2) {
            if (b2 == end2) return {begin1, b1};
            if (b1 == end1) return {end1, end1};
            if (!(*b1 == *b2)) break;
        }
    }
}

/**
 * Check if a range contains at least `n` elements.
 */
template <typename InputIt>
inline constexpr auto contains_elements(InputIt begin, InputIt end, size_t n) {
    // If we have a random access iterator, just use std::distance, otherwise
    // iterate so that we don't have to go all the way to end
    if constexpr (types::iterator_is_category_v<InputIt, std::random_access_iterator_tag>) {
        return std::distance(begin, end) >= n;
    } else {
        auto start = begin;
        for (size_t i = 0; i < n; ++i, ++start)
            if (start == end) return false;
        return true;
    }
}

}

#endif // PARSIMON_INTERNAL_ALGORITHM_H
