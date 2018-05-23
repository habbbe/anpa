#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <variant>
#include <map>
#include <functional>
#include "parser.h"

struct json_value;

using json_object = std::map<std::string, json_value>;
using json_object_pair = std::pair<std::string, json_value>;

using json_array = std::vector<json_value>;

struct json_value {
    std::variant<
    std::string,
    long,
    double,
    json_object,
    json_array,
    bool,
    std::nullptr_t
    > val;
    template <typename T>
    json_value(T &&t) : val(std::forward<T>(t)) {}
    auto &operator*() {return val;}
};

template <typename Parser>
constexpr auto eat(Parser p) {
    return parse::whitespace() >> p;
}

constexpr auto True = true;
constexpr auto False = false;
constexpr auto Null = nullptr;

constexpr auto string_parser = parse::between_token('"');
constexpr auto integer_parser = parse::long_fast();
constexpr auto double_parser = parse::double_fast();
constexpr auto bool_parser = (parse::string("true") >= True) || (parse::string("false") >= False);
constexpr auto null_parser = parse::string("null") >= Null;



auto tmp()
{
    return parse::lift_or_value<json_value>(string_parser);
}
using tmp_type = decltype(tmp());

//template <typename T>
tmp_type get_value_parser();

//template <typename T>
//auto get_array_parser() {
//    auto parser = parse::token('[') >> get_value_parser<T>() << parse::token(']');
//    return parse::parse_result_from(parse::between_tokens('[', ']'),
//                                    )
//    return parse::token('[') >> parse::many_to_vector<json_array>(eat(parse::succeed(parse::token(','))) >> get_value_parser()) << eat(parse::token(']'));
//}

//template <typename T>
auto get_pair_parser() {
    eat(monad::lift_value<std::pair<std::string, json_value>>(string_parser));
}

//template <typename T>
auto get_object_parser() {
    return parse::token('{') >> parse::many_to_unordered_map<json_array>(eat(parse::succeed(parse::token(','))) >> get_value_parser()) << eat(parse::token('}'));
}


//template <typename T>
tmp_type get_value_parser() {
    auto array_parser = parse::token('[') >> parse::many_to_vector(eat(parse::succeed(parse::token(','))) >> get_value_parser()) << eat(parse::token(']'));
    return parse::lift_or_value<json_value>(string_parser, integer_parser, double_parser,
                                                               bool_parser, null_parser,
                                                               array_parser);
}

//auto array_parser = parse::token('[') >>
//                    parse::many<json_array>(eat(parse::succeed(parse::token(','))) >> value_parser) << eat(parse::token(']'));


//constexpr auto object_map_parser = basic_json_parser(monad::lift<json_object_pair>(string_parser, ));
//constexpr auto object_parser = basic_json_parser(monad   parse::between_tokens('{', '}'));

//template <typename StringType>
//inline constexpr auto parse_json(StringType &s) {

//}

#endif // JSON_PARSER_H
