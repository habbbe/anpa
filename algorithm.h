#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <algorithm>
#include <iterator>
#include "types.h"

/**
 * constexpr variants of some algorithms
 */
namespace parse::algorithm {

/**
 * constexpr version of `std::equal` (1)
 */
template <typename Iterator1, typename Iterator2>
inline constexpr auto equal(Iterator1 begin1, Iterator1 end1, Iterator2 begin2) {
    for (; begin1 != end1; ++begin1, ++begin2) {
        if (!(*begin1 == *begin2))
            return false;
    }
    return true;
}

/**
 * Like the other `equal` (1) but the first range is given by the template parameters.
 */
template <auto... vs, typename Iterator>
inline constexpr auto equal(Iterator begin) {
    return ((*begin++ == vs) && ...);
}

/**
 * Like the other `equal` (5) but the first range is given by the template parameters.
 */
template <auto... vs, typename Iterator>
inline constexpr auto equal(Iterator begin, Iterator end) {
    return ((begin != end && *begin++ == vs) && ...);
}

/**
 * constexpr version of `std::find_if`
 */
template <typename Iterator, typename Predicate>
inline constexpr auto find_if(Iterator begin, Iterator end, Predicate p) {
    for (;begin != end; ++begin) {
        if (p(*begin)) return begin;
    }
    return end;
}

/**
 * constexpr version of `std::find_if_not`
 */
template <typename Iterator, typename Predicate>
inline constexpr auto find_if_not(Iterator begin, Iterator end, Predicate p) {
    return algorithm::find_if(begin, end, [=](const auto& val){return !p(val);});
}

/**
 * constexpr version of `std::find`
 */
template <typename Iterator, typename Element>
inline constexpr auto find(Iterator begin, Iterator end, const Element& element) {
    return algorithm::find_if(begin, end, [&](const auto& val){return val == element;});
}

/**
 * Check if the supplied value is contained within the range described by [begin, end).
 */
template <typename Iterator, typename V>
inline constexpr auto contains(Iterator begin, Iterator end, const V& needle) {
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
template <typename Iterator1, typename Iterator2>
inline constexpr std::pair<Iterator1, Iterator1> search(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, Iterator2 end2) {
    for (;;++begin1) {
        Iterator1 b1 = begin1;
        for (Iterator2 b2 = begin2; ; ++b1, ++b2) {
            if (b2 == end2) return {begin1, b1};
            if (b1 == end1) return {end1, end1};
            if (!(*b1 == *b2)) break;
        }
    }
}

/**
 * Check if a range contains at least `n` elements.
 */
template <typename Iterator>
inline constexpr auto contains_elements(Iterator begin, Iterator end, long n) {
    // If we have a random access iterator, just use std::distance, otherwise
    // iterate so that we don't have to go all the way to end
    if constexpr (parse::types::iterator_is_category_v<Iterator, std::random_access_iterator_tag>) {
        return std::distance(begin, end) >= n;
    } else {
        auto start = begin;
        for (long i = 0; i<n; ++i, ++start)
            if (start == end) return false;
        return true;
    }
}

}

#endif // PARSE_UTILS_H
