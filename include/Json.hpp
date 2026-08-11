#pragma once

#include <string>
#include <vector>
#include <map>

namespace json {

    enum class Type {
        Null,
        Bool,
        Number,
        String,
        Array,
        Object
    };

    // Minimal recursive-descent JSON reader covering the full value grammar
    // (object/array/string/number/bool/null), since callers may grow the
    // lang files into using lists/numbers later, even though today's
    // translations only use nested objects of strings.
    class Value {
        public:
            Value();

            // Parses text as a single JSON value. Malformed input yields a
            // Null value rather than failing loudly, matching the rest of
            // this codebase's style of degrading quietly on bad input.
            static Value Parse(const std::string &text);

            inline Type GetType() const {
                return this->type;
            }

            inline bool IsObject() const {
                return this->type == Type::Object;
            }

            inline bool IsString() const {
                return this->type == Type::String;
            }

            bool Has(const std::string &key) const;

            // Both return a Null Value (never a dangling/invalid reference)
            // when this isn't the right container type or the key/index
            // doesn't exist, so lookups can be chained safely.
            const Value &operator[](const std::string &key) const;
            const Value &operator[](const size_t index) const;

            size_t Count() const;

            std::string AsString(const std::string &fallback = "") const;
            double AsNumber(const double fallback = 0.0) const;
            bool AsBool(const bool fallback = false) const;

        private:
            static void SkipWhitespace(const std::string &text, size_t &pos);
            static Value ParseValue(const std::string &text, size_t &pos);
            static Value ParseObject(const std::string &text, size_t &pos);
            static Value ParseArray(const std::string &text, size_t &pos);
            static std::string ParseString(const std::string &text, size_t &pos);
            static Value ParseNumber(const std::string &text, size_t &pos);
            static Value ParseKeyword(const std::string &text, size_t &pos);

            Type type;
            std::string str_value;
            double num_value;
            bool bool_value;
            std::map<std::string, Value> obj_value;
            std::vector<Value> arr_value;
    };

}
