#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>
#include <functional>
#include "json_value.h"
#include "parser.h"
#include "parser_state.h"
#include "parser_types.h"
#include "lazy.h"

template <typename Parser>
constexpr auto eat(Parser p) {
    return parse::whitespace() >> p;
}

constexpr auto string_parser = eat(parse::lift_value<std::string>(parse::between_items('"', '"')));
constexpr auto number_parser = eat(parse::floating());
constexpr auto bool_parser = eat((parse::sequence("true") >= true) || (parse::sequence("false") >= false));
constexpr auto null_parser = eat(parse::sequence("null") >= nullptr);

template <typename F>
constexpr auto get_object_parser(F value_parser) {
    auto pair_parser = parse::lift_value<json_object_pair>(string_parser, eat(parse::item(':') >> parse::lift_shared(value_parser)));
    return eat(parse::item('{')) >> parse::many_to_map<true>(pair_parser, eat(parse::item(','))) << eat(parse::item('}'));
}

template <typename F>
constexpr auto get_array_parser(F value_parser) {
    return eat(parse::item('[')) >> parse::many_to_vector(parse::lift_shared(value_parser), eat(parse::item(','))) << eat(parse::item(']'));
}

constexpr auto json_parser = parse::recursive<json_value, void>([](auto val_parser) {
        return parse::lift_or_value<json_value>(string_parser,
                            bool_parser, null_parser, number_parser,
                            get_array_parser(val_parser), get_object_parser(val_parser));
});

#endif // JSON_PARSER_H
