#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>
#include <functional>
#include "json_value.h"
#include "parser.h"
#include "state.h"
#include "types.h"
#include "lazy.h"

template <typename Parser>
constexpr auto eat(Parser p) {
    return parse::whitespace() >> p;
}

constexpr auto string_parser = []() {
    constexpr auto unicode = parse::item<'u'>() >> parse::times(4, parse::item_if([](auto &f) {return std::isxdigit(f);}));
    constexpr auto escaped = parse::item<'\\'>() >> (unicode || parse::any_of<'"','\\','/','b','f','n','r','t'>());
    constexpr auto notEnd = escaped || parse::any_item();
    return parse::lift_value<json_string>(eat(parse::item<'"'>()) >> parse::many(notEnd, {}, parse::item<'"'>()));
}();

constexpr auto number_parser = eat(parse::floating());
constexpr auto bool_parser = eat((parse::sequence<'t','r','u','e'>() >> parse::mreturn<true>()) ||
                                 (parse::sequence<'f','a','l','s','e'>() >> parse::mreturn<false>()));
constexpr auto null_parser = eat(parse::sequence<'n','u','l','l'>() >> parse::mreturn<nullptr>());

template <typename F>
constexpr auto get_object_parser(F value_parser) {
    auto pair_parser = parse::lift_value<json_object_pair>(string_parser, eat(parse::item<':'>() >> value_parser));
    return eat(parse::item<'{'>()) >> parse::many_to_map(pair_parser, eat(parse::item<','>())) << eat(parse::item<'}'>());
}

template <typename F>
constexpr auto get_array_parser(F value_parser) {
    return eat(parse::item<'['>()) >> parse::many_to_vector(value_parser, eat(parse::item<','>())) << eat(parse::item<']'>());
}

constexpr auto json_parser = parse::recursive<json_value>([](auto val_parser) {
        return parse::lift_or_value<json_value>(string_parser,
                            bool_parser, null_parser, number_parser,
                            get_array_parser(val_parser), get_object_parser(val_parser));
});

constexpr auto array_parser = get_array_parser(json_parser);
constexpr auto object_parser = get_object_parser(json_parser);

#endif // JSON_PARSER_H
