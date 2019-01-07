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

constexpr auto string_parser2 = parse::lift_value<json_string>(eat(parse::parser([](auto &s) {
    if (s.empty() || s.front() != '"') {
        return s.return_fail();
    }

    s.advance(1);

    auto start = s.position;

    while (true) {
        if (s.empty()) {
            return s.return_fail();
        }
        auto front = s.front();

        if (front == '"') {
            auto end = s.position;
            s.advance(1);
            return s.return_success(s.convert(start, end));
        }
        else if (front == '\\') {
            s.advance(1);
            if (s.empty()) {
                return s.return_fail();
            }
            auto c = s.front();
            switch (c) {
            case 'u':
                s.advance(1);
                for (int i = 0; i<4; ++i, s.advance(1)) {
                    if (s.empty())
                        return s.return_fail();
                    else if (!std::isxdigit(s.front()))
                        return s.return_fail();
                }
                break;
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
                break;
            default:
                return s.return_fail();
            }
        }
        s.advance(1);
    }
})));

constexpr auto string_parser = []() {
    constexpr auto unicode = parse::item('u') >> parse::times(4, parse::item_if([](auto &f) {return std::isxdigit(f);}));
    constexpr auto escaped = parse::item('\\') >> (unicode || parse::any_of("\"\\bfnrt"));
    constexpr auto notEnd = parse::any_item() || escaped;
    return parse::lift_value<json_string>(eat(parse::item('"')) >> parse::many(notEnd, nullptr, parse::item('"')));
}();

constexpr auto string_parser5 = eat(parse::lift_value<json_string>(parse::between_items('"', '"')));
constexpr auto string_parser4 = eat(parse::item('"'))
        >> parse::lift_value<json_string>(parse::many(parse::sequence<'\\', '"'>() || parse::not_item<'"'>()) << parse::consume(1));
constexpr auto number_parser = eat(parse::floating());
constexpr auto bool_parser = eat((parse::sequence("true") >> parse::mreturn<true>()) ||
                                 (parse::sequence("false") >> parse::mreturn<false>()));
constexpr auto null_parser = eat(parse::sequence("null") >> parse::mreturn<nullptr>());

template <typename F>
constexpr auto get_object_parser(F value_parser) {
    auto pair_parser = parse::lift_value<json_object_pair>(string_parser, eat(parse::item<':'>() >> value_parser));
    return eat(parse::item<'{'>()) >> parse::many_to_map(pair_parser, eat(parse::item<','>())) << eat(parse::item<'}'>());
}

template <typename F>
constexpr auto get_array_parser(F value_parser) {
    return eat(parse::item<'['>()) >> parse::many_to_vector(value_parser, eat(parse::item<','>())) << eat(parse::item<']'>());
}

constexpr auto json_parser = parse::recursive<json_value, void>([](auto val_parser) {
        return parse::lift_or_value<json_value>(string_parser,
                            bool_parser, null_parser, number_parser,
                            get_array_parser(val_parser), get_object_parser(val_parser));
});

constexpr auto array_parser = get_array_parser(json_parser);
constexpr auto object_parser = get_object_parser(json_parser);

#endif // JSON_PARSER_H
