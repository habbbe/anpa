#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <algorithm>
#include <iterator>

/**
 * constexpr variants of some algorithms
 */
namespace parse::algorithm {

/**
 * constexpr version of std::equal (1)
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
 * constexpr version of std::find_if
 */
template <typename Iterator, typename Predicate>
inline constexpr auto find_if(Iterator begin, Iterator end, Predicate p) {
    for (;begin != end; ++begin) {
        if (p(*begin)) return begin;
    }
    return begin;
}

/**
 * constexpr version of std::find_if_not
 */
template <typename Iterator, typename Predicate>
inline constexpr auto find_if_not(Iterator begin, Iterator end, Predicate p) {
    return algorithm::find_if(begin, end, [=](const auto &val){return !p(val);});
}

/**
 * constexpr version of std::find
 */
template <typename Iterator, typename Element>
inline constexpr auto find(Iterator begin, Iterator end, const Element &element) {
    return algorithm::find_if(begin, end, [&](const auto &val){return val == element;});
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


template <typename Iterator>
constexpr bool is_random_access_iterator() {
    using category = typename std::iterator_traits<std::decay_t<Iterator>>::iterator_category;
    return std::is_same_v<category, std::random_access_iterator_tag>;
}

template <typename Iterator>
inline constexpr auto contains_elements(Iterator begin, Iterator end, long n) {
    // If we have a random access iterator, just use std::distance, otherwise
    // iterate so that we don't have to go all the way to end
    if constexpr (is_random_access_iterator<Iterator>()) {
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
