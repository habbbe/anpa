#ifndef PARSE_UTILS_H
#define PARSE_UTILS_H

template <typename Iterator1, typename Iterator2>
inline constexpr auto equal(Iterator1 begin1, Iterator1 end, Iterator2 begin2) {
    while (begin1 != end) {
        if (!(*begin1++ == *begin2++)) return false;
    }
    return true;
}

template <typename Iterator, typename Element>
inline constexpr auto find(Iterator begin, Iterator end, const Element &element) {
    for (;begin != end; ++begin) {
        if ((*begin == element)) return begin;
    }
    return begin;
}

template <typename Iterator1, typename Iterator2>
inline constexpr auto search(Iterator1 begin1, Iterator1 end1, Iterator2 begin2, Iterator2 end2) {
    while (begin1 != end1) {
        Iterator1 b1 = begin1;
        Iterator2 b2 = begin2;
        for (;b2 != end2; ++b2, ++begin1) {
            if (!(*begin1 == *b2)) {
                break;
            }
        }
        if (b2 == end2) return b1;
        else if (b1 == begin1) ++begin1;
    }
    return begin1;
}

#endif // PARSE_UTILS_H
