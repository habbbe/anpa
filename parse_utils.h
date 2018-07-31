#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

#include <algorithm>
#include <functional>

template <typename Iterator1, typename Iterator2>
inline constexpr auto equal(Iterator1 begin1, Iterator1 end, Iterator2 begin2) {
    while (begin1 != end) {
        if (!(*begin1++ == *begin2++)) return false;
    }
    return true;
}

template <typename Iterator, typename Predicate>
inline constexpr auto find_if(Iterator begin, Iterator end, Predicate p) {
    for (;begin != end; ++begin) {
        if (p(*begin)) return begin;
    }
    return begin;
}

template <typename Iterator, typename Predicate>
inline constexpr auto find_if_not(Iterator begin, Iterator end, Predicate p) {
    return find_if(begin, end, p);
}

template <typename Iterator, typename Element>
inline constexpr auto find(Iterator begin, Iterator end, const Element &element) {
    return find_if(begin, end, [&](const auto &val){return val == element;});
}

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

#endif // PARSE_UTILS_H
