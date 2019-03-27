#ifndef JSON_VALUE_H
#define JSON_VALUE_H

#include <unordered_map>
#include <vector>
#include <variant>
#include <memory>
#include "types.h"

struct json_value;

using json_string = std::string;
using json_object = std::unordered_map<json_string, json_value>;
using json_object_pair = std::pair<json_string, json_value>;
using json_array = std::vector<json_value>;
using json_number = double;
using json_null = parse::none;

using json_value_variant = std::variant<
json_null,
bool,
json_string,
json_number,
json_object,
json_array>;

struct json_value {
    std::shared_ptr<json_value_variant> val;

    template <typename T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, json_value>>>
    json_value(T&& t) : val(std::make_shared<json_value_variant>(std::forward<T>(t))) {}

    template <typename T>
    decltype(auto) get() {return std::get<T>(*val);}

    template <typename T>
    decltype(auto) is_a() {return std::holds_alternative<T>(*val);}

    decltype(auto) operator[](size_t i) {return std::get<json_array>(*val)[i];}

    template <typename Key>
    decltype(auto) at(Key&& key) {return std::get<json_object>(*val).at(key);}

    size_t size() const {
        return std::visit([](const auto& v) {
            using type = std::decay_t<decltype(v)>;
            if constexpr (parse::types::is_one_of<type, json_array, json_object, json_string>) return v.size();
            else return size_t(0);
        }, *val);
    }

    template <typename Key>
    auto contains(const Key& key) {
        auto& map = get<json_object>();
        return map.find(key) != map.end();
    }
};

#endif // JSON_VALUE_H
