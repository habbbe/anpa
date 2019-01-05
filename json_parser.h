#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>
#include <map>
#include <functional>
#include "parser.h"
#include "parser_state.h"
#include "parser_types.h"

struct json_value;

using json_object = std::map<std::string, json_value>;
using json_object_pair = std::pair<std::string, json_value>;
using json_array = std::vector<json_value>;


struct json_value {
    std::variant<std::nullptr_t, bool, std::string, double, json_object, json_array > val;

    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, json_value>>>
    json_value(T &&t) : val(std::forward<T>(t)) {}

    json_value(const json_value &other): val(other.val) {}

    template <typename T>
    decltype(auto) get() {return std::get<T>(val);}

    template <typename T>
    decltype(auto) is_a() {return std::holds_alternative<T>(val);}

    decltype(auto) operator[](size_t i) {return std::get<json_array>(val)[i];}

    template <typename Str, typename = std::enable_if_t<!std::is_integral_v<Str>>>
    decltype(auto) operator[](Str &&key) {return std::get<json_object>(val)[key];}

    template <typename Key>
    auto contains(const Key &key) {
        auto &map = get<json_object>();
        return map.find(key) != map.end();
    }
};

template <typename Parser>
constexpr auto eat(Parser p) {
    return parse::whitespace() >> p;
}

constexpr auto string_parser = eat(monad::lift_value<std::string>(parse::between_items('"', '"')));
constexpr auto number_parser = eat(parse::floating());
constexpr auto bool_parser = eat((parse::sequence("true") >= true) || (parse::sequence("false") >= false));
constexpr auto null_parser = eat(parse::sequence("null") >= nullptr);

template <typename F>
constexpr auto get_object_parser(F value_parser) {
    auto pair_parser = monad::lift_value<json_object_pair>(string_parser, eat(parse::item(':') >> value_parser));
    return eat(parse::item('{')) >> parse::many_to_map(pair_parser, eat(parse::item(','))) << eat(parse::item('}'));
}

template <typename F>
constexpr auto get_array_parser(F value_parser) {
    return eat(parse::item('[')) >> parse::many_to_vector(value_parser, eat(parse::item(','))) << eat(parse::item(']'));
}

constexpr auto value_parser = parse::recursive<json_value, void>([](auto val_parser) {
        return parse::lift_or_value<json_value>(string_parser,
                            bool_parser, null_parser, number_parser,
                            get_array_parser(val_parser), get_object_parser(val_parser));
});

constexpr auto array_parser = get_array_parser(value_parser);
constexpr auto object_parser = get_object_parser(value_parser);

constexpr auto json_parser = parse::lift_or_value<json_value>(array_parser || object_parser);

#endif // JSON_PARSER_H
