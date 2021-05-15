#ifndef PARSIMON_PARSE_ERROR_H
#define PARSIMON_PARSE_ERROR_H

namespace anpa {

template <typename Message, typename InputIt>
struct parse_error {
    Message message;
    InputIt position;
    constexpr parse_error(Message message, InputIt position) : message{message}, position{position} {}
};

}



#endif // PARSIMON_PARSE_ERROR_H
