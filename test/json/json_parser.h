#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_value.h"
#include "parsimon/parsimon.h"

using namespace parsimon;

/**
 * Remove whitespace (if any) before evaluating `p`
 */
template <typename Parser>
constexpr auto eat(Parser p) {
    return whitespace() >> p;
}

constexpr auto string_parser = []() {
    constexpr auto unicode = item<'u'>() >> times<4>(item_if([](const auto& f) {return std::isxdigit(f);}));
    constexpr auto escaped = item<'\\'>() >> (unicode || any_of<'"','\\','/','b','f','n','r','t'>());
    constexpr auto notEnd = escaped || not_item<'"'>();
    return lift_value<json_string>(eat(item<'"'>()) >> many(notEnd) << item<'"'>());
}();

constexpr auto number_parser = eat(floating<json_number, true>());
constexpr auto bool_parser = eat((seq<'t','r','u','e'>() >= true) ||
                                 (seq<'f','a','l','s','e'>() >= false));
constexpr auto null_parser = eat(seq<'n','u','l','l'>() >= json_null());

template <typename F>
constexpr auto get_object_parser(F value_parser) {
    auto pair_parser = lift_value<json_object_pair>(string_parser, eat(item<':'>() >> value_parser));
    return eat(item<'{'>()) >> many_to_map(pair_parser, eat(item<','>())) << eat(item<'}'>());
}

template <typename F>
constexpr auto get_array_parser(F value_parser) {
    return eat(item<'['>()) >> many_to_vector(value_parser, eat(item<','>())) << eat(item<']'>());
}

constexpr auto json_parser = recursive<json_value>([](auto val_parser) {
        return lift_or_value<json_value>(string_parser, number_parser,
                                                get_object_parser(val_parser), get_array_parser(val_parser),
                                                bool_parser, null_parser);
});

constexpr auto array_parser = get_array_parser(json_parser);
constexpr auto object_parser = get_object_parser(json_parser);

#endif // JSON_PARSER_H
