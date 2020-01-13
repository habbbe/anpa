#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include "json_value.h"
#include "parsimon/parsimon.h"

using namespace parsimon;

//Remove whitespace (if any) before evaluating `p`
template <typename Parser>
constexpr auto eat(Parser p) {
    return trim() >> p;
}

constexpr auto string_parser = []() {
    constexpr auto unicode = item<'u'>() >> times<4>(item_if([](const auto& f) {return std::isxdigit(f);}));
    constexpr auto escaped = item<'\\'>() >> (unicode || any_of<'"','\\','/','b','f','n','r','t'>());
    constexpr auto notEnd = escaped || not_item<'"'>();
    return lift_value<json_string>(item<'"'>() >> many(notEnd) << item<'"'>());
}();

constexpr auto number_parser = floating<json_number>();
constexpr auto bool_parser = seq<'t','r','u','e'>() >> mreturn<true>() ||
                                 seq<'f','a','l','s','e'>() >> mreturn<false>();
constexpr auto null_parser = seq<'n','u','l','l'>() >> mreturn_emplace<json_null>();

template <typename P>
constexpr auto get_object_parser(P value_parser) {
    auto shared_value_parser = lift([](auto&& r) {
        return std::make_shared<json_value>(std::forward<decltype(r)>(r));
    }, value_parser);

    return item<'{'>() >> many_to_map<options::no_trailing_separator>(eat(string_parser),
                                      eat(item<':'>() >> shared_value_parser),
                                      eat(item<','>())) << eat(item<'}'>());
}

template <typename P>
constexpr auto get_array_parser(P value_parser) {
    return item<'['>() >> many_to_vector<options::no_trailing_separator>(value_parser, eat(item<','>())) << eat(item<']'>());
}

constexpr auto json_parser = recursive<json_value>([](auto val_parser) {
    return eat(lift_or_value<json_value>(string_parser, number_parser,
                                         get_object_parser(val_parser), get_array_parser(val_parser),
                                         bool_parser, null_parser));
});

constexpr auto array_parser = get_array_parser(json_parser);
constexpr auto object_parser = get_object_parser(json_parser);

#endif // JSON_PARSER_H
