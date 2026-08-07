#include <Json.hpp>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace json {

    Value::Value() : type(Type::Null), num_value(0.0), bool_value(false) {}

    void Value::SkipWhitespace(const std::string &text, size_t &pos) {
        while ((pos < text.size()) && std::isspace(static_cast<unsigned char>(text[pos]))) {
            pos++;
        }
    }

    Value Value::Parse(const std::string &text) {
        size_t pos = 0;
        Value::SkipWhitespace(text, pos);
        if (pos >= text.size()) {
            return Value();
        }
        return Value::ParseValue(text, pos);
    }

    Value Value::ParseValue(const std::string &text, size_t &pos) {
        Value::SkipWhitespace(text, pos);
        if (pos >= text.size()) {
            return Value();
        }

        switch (text[pos]) {
            case '{':
                return Value::ParseObject(text, pos);
            case '[':
                return Value::ParseArray(text, pos);
            case '"': {
                Value value;
                value.type = Type::String;
                value.str_value = Value::ParseString(text, pos);
                return value;
            }
            case 't':
            case 'f':
            case 'n':
                return Value::ParseKeyword(text, pos);
            default:
                return Value::ParseNumber(text, pos);
        }
    }

    Value Value::ParseObject(const std::string &text, size_t &pos) {
        Value value;
        value.type = Type::Object;
        pos++; // consume '{'

        Value::SkipWhitespace(text, pos);
        if ((pos < text.size()) && (text[pos] == '}')) {
            pos++;
            return value;
        }

        while (pos < text.size()) {
            Value::SkipWhitespace(text, pos);
            if ((pos >= text.size()) || (text[pos] != '"')) {
                break;
            }
            const auto key = Value::ParseString(text, pos);

            Value::SkipWhitespace(text, pos);
            if ((pos >= text.size()) || (text[pos] != ':')) {
                break;
            }
            pos++; // consume ':'

            value.obj_value[key] = Value::ParseValue(text, pos);

            Value::SkipWhitespace(text, pos);
            if ((pos < text.size()) && (text[pos] == ',')) {
                pos++;
                continue;
            }
            break;
        }

        Value::SkipWhitespace(text, pos);
        if ((pos < text.size()) && (text[pos] == '}')) {
            pos++;
        }
        return value;
    }

    Value Value::ParseArray(const std::string &text, size_t &pos) {
        Value value;
        value.type = Type::Array;
        pos++; // consume '['

        Value::SkipWhitespace(text, pos);
        if ((pos < text.size()) && (text[pos] == ']')) {
            pos++;
            return value;
        }

        while (pos < text.size()) {
            value.arr_value.push_back(Value::ParseValue(text, pos));

            Value::SkipWhitespace(text, pos);
            if ((pos < text.size()) && (text[pos] == ',')) {
                pos++;
                continue;
            }
            break;
        }

        Value::SkipWhitespace(text, pos);
        if ((pos < text.size()) && (text[pos] == ']')) {
            pos++;
        }
        return value;
    }

    std::string Value::ParseString(const std::string &text, size_t &pos) {
        std::string result;
        pos++; // consume opening '"'

        while ((pos < text.size()) && (text[pos] != '"')) {
            auto c = text[pos];
            if ((c == '\\') && ((pos + 1) < text.size())) {
                pos++;
                switch (text[pos]) {
                    case 'n': c = '\n'; break;
                    case 't': c = '\t'; break;
                    case 'r': c = '\r'; break;
                    case 'b': c = '\b'; break;
                    case 'f': c = '\f'; break;
                    default: c = text[pos]; break; // '"', '\\', '/', or anything unrecognized
                }
            }
            result += c;
            pos++;
        }

        if ((pos < text.size()) && (text[pos] == '"')) {
            pos++; // consume closing '"'
        }
        return result;
    }

    Value Value::ParseNumber(const std::string &text, size_t &pos) {
        const auto start = pos;
        while ((pos < text.size()) && (std::strchr("+-0123456789.eE", text[pos]) != nullptr)) {
            pos++;
        }

        Value value;
        value.type = Type::Number;
        value.num_value = std::atof(text.substr(start, pos - start).c_str());
        return value;
    }

    Value Value::ParseKeyword(const std::string &text, size_t &pos) {
        Value value;
        if (text.compare(pos, 4, "true") == 0) {
            value.type = Type::Bool;
            value.bool_value = true;
            pos += 4;
        }
        else if (text.compare(pos, 5, "false") == 0) {
            value.type = Type::Bool;
            value.bool_value = false;
            pos += 5;
        }
        else if (text.compare(pos, 4, "null") == 0) {
            pos += 4;
        }
        else {
            // Unrecognized token; consume one character so the caller's
            // parsing loop can't get stuck spinning on it.
            pos++;
        }
        return value;
    }

    bool Value::Has(const std::string &key) const {
        return (this->type == Type::Object) && (this->obj_value.find(key) != this->obj_value.end());
    }

    const Value &Value::operator[](const std::string &key) const {
        static const Value NullValue;
        if (this->type != Type::Object) {
            return NullValue;
        }
        const auto it = this->obj_value.find(key);
        if (it == this->obj_value.end()) {
            return NullValue;
        }
        return it->second;
    }

    const Value &Value::operator[](const size_t index) const {
        static const Value NullValue;
        if ((this->type != Type::Array) || (index >= this->arr_value.size())) {
            return NullValue;
        }
        return this->arr_value.at(index);
    }

    size_t Value::Count() const {
        if (this->type == Type::Array) {
            return this->arr_value.size();
        }
        if (this->type == Type::Object) {
            return this->obj_value.size();
        }
        return 0;
    }

    std::string Value::AsString(const std::string &fallback) const {
        return (this->type == Type::String) ? this->str_value : fallback;
    }

    double Value::AsNumber(const double fallback) const {
        return (this->type == Type::Number) ? this->num_value : fallback;
    }

    bool Value::AsBool(const bool fallback) const {
        return (this->type == Type::Bool) ? this->bool_value : fallback;
    }

}
