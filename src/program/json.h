#pragma once

#include <string>
#include <unordered_map>

#include "containers/darray.h"
#include "math/basic_types.h"

namespace JSON
{

    struct Value;
    typedef std::unordered_map<std::string, Value> JSONObject;
    typedef gn::darray<Value> JSONArray;

    struct Value
    {
        enum struct Type
        {
            Null,
            Bool,
            Int,
            Float,
            String,
            Object,
            Array,
            Error
        };

        Type type;
        
        union
        {
            bool         Bool;
            s64          Int;
            f64          Float;
            std::string* String;
            JSONObject*  Object;
            JSONArray*   Array;
        };

        Value();
        Value(bool b);
        Value(int i);
        Value(s64 i);
        Value(f64 f);
        Value(std::string* s);
        Value(char* s);
        Value(const std::string& s);
        Value(JSONObject* o);
        Value(JSONArray* a);
        Value(Type forceType);
        Value(Value&& other);
        
        void Free();

        Value& operator=(Value&& rhs);
        Value& operator[](u64 idx);
        Value& operator[](const std::string& key);

        bool&        GetBool();
        s64&         GetInt();
        f64&         GetFloat();
        std::string& GetString();
        u64          GetArraySize() const;

        std::string Value::GetTypeName() const;
    };

    struct Token
    {
        enum struct Type
        {
            Value,
            Int,
            Float,
            String,

            Comma = ',',
            Colon = ':',
            
            SquareBracket_Open  = '[',
            SquareBracket_Close = ']',
            CurlyBracket_Open   = '{',
            CurlyBracket_Close  = '}',

            Error   // @TODO: Use this later to implement proper error checking.
        };

        Type type;
        std::string value;

        Token();
        Token(const Token& other);
        Token(Token&& other);

        Token& operator=(Token&& rhs);
    };

    Value LoadJSONFile(const std::string& filepath);
    Value ParseNext(std::string& data, u64& index, const std::string& filepath, u32& lineNumber);

} // namespace JSON

typedef JSON::Value Json;