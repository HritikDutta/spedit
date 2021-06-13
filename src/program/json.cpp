#include "json.h"

#ifdef DEBUG
#define JSON_VERBOSE
#include <iostream>
#endif

#include <string>

#include "containers/darray.h"
#include "containers/hash_table.h"
#include "math/basic_types.h"
#include "platform/fileio.h"
#include "json_helpers.h"

namespace JSON
{

    Value::Value()
    :   type(Type::Null), Int(0) {}

    Value::Value(bool b)
    :   type(Type::Bool), Bool(b) {}

    Value::Value(int i)
    :   type(Type::Int), Int(i) {}

    Value::Value(s64 i)
    :   type(Type::Int), Int(i) {}

    Value::Value(f64 f)
    :   type(Type::Float), Float(f) {}
    
    Value::Value(std::string* s)
    :   type(Type::String), String(s) {}

    Value::Value(char* s)
    :   type(Type::String), String(new std::string(s)) {}

    Value::Value(const std::string& s)
    :   type(Type::String), String(new std::string(s)) {}

    Value::Value(JSONObject* o)
    :   type(Type::Object), Object(o) {}

    Value::Value(JSONArray* a)
    :   type(Type::Array), Array(a) {}

    Value::Value(Type forceType)
    :   type(forceType), Int(0) {}

    // Only setting Int since it's the largest data type in the union
    Value::Value(Value&& other)
    :   type(other.type), Int(std::move(other.Int)) {}

    void
    Value::Free()
    {
        switch (type)
        {
            case Type::String:
                if (String)
                {
                    delete String; 
                }
                break;
            case Type::Object:
                if (Object)
                {
                    for (auto& it : *Object)
                        it.second.Free();

                    delete Object;
                }
                break;
            case Type::Array:
                if (Array)
                {
                    for (Value& it : *Array)
                        it.Free();

                    delete Array;
                }
                break;
        }
    }

    Value&
    Value::operator=(Value&& rhs)
    {
        type = rhs.type;
        Int  = std::move(rhs.Int);

        rhs.type = Type::Null;
        return *this;
    }
    
    Value&
    Value::operator[](u64 idx)
    {
#       ifdef DEBUG
        if (type != Type::Array || Array == nullptr)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as Array" << std::endl;
#       endif // DEBUG

        return (*Array)[idx];
    }
    
    Value&
    Value::operator[](const std::string& key)
    {
#       ifdef DEBUG
        if (type != Type::Object || Object == nullptr)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as Object" << std::endl;
#       endif //  DEBUG
        return (*Object)[key];
    }

    std::string&
    Value::GetString()
    {
#       ifdef DEBUG
        if (type != Type::String)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as String" << std::endl;
#       endif // DEBUG

        return *String;
    }

    s64&
    Value::GetInt()
    {
#       ifdef DEBUG
        if (type != Type::Int)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as Int" << std::endl;
#       endif // DEBUG
    
        return Int;
    }
    
    f64&
    Value::GetFloat()
    {
#       ifdef DEBUG
        if (type != Type::Float)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as Float" << std::endl;
#       endif // DEBUG

        return Float;
    }
    
    bool&
    Value::GetBool()
    {
#       ifdef DEBUG
        if (type != Type::Bool)
            std::cerr << "Error: Trying to read object of type " << GetTypeName() << " as Bool" << std::endl;
#       endif //  DEBUG

        return Bool;
    }

    u64
    Value::GetArraySize() const
    {
        if (type != Type::Array)
        {
#           ifdef DEBUG
            std::cerr << "Error: Trying to get size of Array from type " << GetTypeName() << std::endl;
#           endif // DEBUG

            return 0Ui64;
        }

        return Array->size();
    }

    std::string
    Value::GetTypeName() const
    {
        switch (type)
        {
            case Value::Type::Null:    return "Null";
            case Value::Type::Bool:    return "Bool";
            case Value::Type::Int:     return "Int";
            case Value::Type::Float:   return "Float";
            case Value::Type::String:  return "String";
            case Value::Type::Object:  return "Object";
            case Value::Type::Array:   return "Array";
            case Value::Type::Error:   return "Error";
            default:                   return "[Type Not Found]";
        }
    }


    Token::Token()
    :   type(Type::Value) {}

    Token::Token(const Token& other)
    :   type(other.type), value(other.value) {}

    Token::Token(Token&& other)
    :   type(other.type), value(std::move(other.value))
    {
        other.type = Type::Error;
    }

    Token&
    Token::operator=(Token&& rhs)
    {
        type = rhs.type;
        value = std::move(rhs.value);

        rhs.type = Type::Error;
        return *this;
    }

    // Parsing File

#   ifdef DEBUG
    void
    PrintIllegalTokenError(const Token& token, const std::string& filepath, const u32& lineNumber, const std::string& expected)
    {
        std::cerr << "Error: Illegal token " << token.value << " at: " << filepath << ":(" << lineNumber << ")" << std::endl;
        std::cerr << "\tExpected " << expected << std::endl;
    }
#   endif // DEBUG

    Token
    GetNextToken(std::string& data, u64& index, u32& lineNumber)
    {
        EatSpacesAndCountLines(data, index, lineNumber);

        // Collect next token
        Token t;

        u64 offset = 0;
        char ch = data[index + offset];

        bool isSingleCharToken = false;
    
        if (   ch == ',' || ch == ':'
            || ch == '[' || ch == ']'
            || ch == '{' || ch == '}')
        {
            // All single character tokens are represented by
            // their ASCII code so no need to collect value.

            t.type = (Token::Type)ch;
            offset++;
            isSingleCharToken = true;
        }

        if (!isSingleCharToken)
        {
            if (ch == '\"')
            {
                // Strings are represented by the ASCII code for '\"'
                t.type = Token::Type::String;
                offset++;
            }
            else if (IsDigit(ch) || ch == '-')
            {
                // If it's a number then the token is considered as an Int.
                // If a '.' or an 'e' is found while parsing then
                //      the type is changed to Float.

                t.type = Token::Type::Int;
                offset++;
            }

            while ((index + offset) < data.size())
            {
                ch = data[index + offset];

                // Strings only end at the next '"'
                // @TODO: handle the case if '"' isn't found
                if (t.type == Token::Type::String && ch == '\"')
                {
                    offset++;
                    break;
                }
                
                // Everything else ends at the first whitespace character
                if (t.type != Token::Type::String && IsWhiteSpace(ch))
                    break;

                if (t.type == Token::Type::Value && !IsAlphabet(ch))
                    break;

                if ((ch == '.' || ch == 'e' || ch == 'E') && t.type == Token::Type::Int)
                    t.type = Token::Type::Float;

                if ((t.type == Token::Type::Int || t.type == Token::Type::Float)
                    && !IsPartOfANumber(ch))
                    break;

                offset++;
            }
        }

        t.value = data.substr(index, offset);
        index   += offset;
        return std::move(t);
    }

    s64
    ParseInt(std::string& token)
    {
        s64 neg = (token[0] == '-') ? -1 : 1;
        u64 idx = (token[0] == '-') ? 1 : 0;
        s64 num = 0;

        while (token[idx])
        {
            num = (10 * num) + (token[idx] - '0');
            idx++;
        }

        return neg * num;
    }

    f64
    ParseFloat(std::string& token)
    {
        f64 neg = (token[0] == '-') ? -1 : 1;
        u64 idx = (token[0] == '-') ? 1 : 0;
        f64 num = 0.0;
        f64 mul = 1.0;

        while (token[idx])
        {
            if (token[idx] == '.')
            {
                mul = 0.1;
                idx++;
                continue;
            }

            if (mul == 1.0)
            {
                // If '.' hasn't been reached
                num = (10 * num) + (token[idx] - '0');
            }
            else
            {
                // If '.' has been reached
                num = num + (mul * (token[idx] - '0'));
                mul *= 0.1;
            }
            
            idx++;
        }

        return neg * num;
    }

    // Remove '"'s at both ends and unescape all escaped characters    
    std::string
    ParseString(const std::string& token)
    {
        u32 readIdx;
        readIdx = (token[0] == '\"') ? 1 : 0;

        std::string str;
        while (token[readIdx] != '\"')
        {
            // Handle escape characters
            if (token[readIdx] == '\\')
            {
                readIdx++;
                switch (token[readIdx])
                {
                    case '\"': str += '\"'; break;
                    case '\\': str += '\\'; break;
                    case 'b':  str += '\b'; break;
                    case 'f':  str += '\f'; break;
                    case 'n':  str += '\n'; break;
                    case 'r':  str += '\r'; break;
                    case 't':  str += '\t'; break;

#                   ifdef DEBUG
                    default:   std::cerr << "Error: Escape character '" << token[readIdx++] << "' not supported" << std::endl;   // Ignore wrong escaped character
#                   endif // DEBUG
                }
            }
            else
            {
                str += token[readIdx];
            }

            readIdx++;
        }

        return std::move(str);
    }

    JSONObject*
    ParseObject(std::string& data, u64& index, const std::string& filepath, u32& lineNumber)
    {
        JSONObject* object = new JSONObject();

        u32 currentLine = lineNumber;
        Token t = GetNextToken(data, index, lineNumber);
        while (true)
        {
            if (t.type == Token::Type::String)
            {
                std::string key = ParseString(t.value);
                currentLine = lineNumber;
                t = GetNextToken(data, index, lineNumber);

                if (t.type != Token::Type::Colon)
                {
#                   ifdef DEBUG
                    PrintIllegalTokenError(t, filepath, currentLine, "':'");
#                   endif // DEBUG

                    delete object;
                    return nullptr;
                }

                Value& currentValue = (*object)[key] = ParseNext(data, index, filepath, lineNumber); 
                if (currentValue.type == Value::Type::Error)
                {
#                   ifdef DEBUG
                    std::cerr << "Error: Encountered parsing value for key: " << key << std::endl;
#                   endif // DEBUG

                    delete object;
                    return nullptr;
                }
            
                // Look for ',' or '}' after reading value
                currentLine = lineNumber;
                t = GetNextToken(data, index, lineNumber);
                if (t.type != Token::Type::Comma && t.type != Token::Type::CurlyBracket_Close)
                {
#                   ifdef DEBUG
                    PrintIllegalTokenError(t, filepath, currentLine, "',' or '}'");
#                   endif // DEBUG

                    delete object;
                    return nullptr;
                }
            }
            else if (t.type != Token::Type::CurlyBracket_Close)
            {
#               ifdef DEBUG
                PrintIllegalTokenError(t, filepath, currentLine, "Property");
#               endif // DEBUG

                delete object;
                return nullptr;
            }

            if (t.type == Token::Type::CurlyBracket_Close) break;

            t = GetNextToken(data, index, lineNumber);
        }

        return object;
    }

    JSONArray*
    ParseArray(std::string& data, u64& index, const std::string& filepath, u32& lineNumber)
    {
        JSONArray* array = new JSONArray();

        while (true)
        {
            size_t prevIndex = index, prevLineNumber = lineNumber;
            Token t = GetNextToken(data, index, lineNumber);
            if (t.type == Token::Type::SquareBracket_Close) break;
            if (t.type == Token::Type::Comma) continue;

            index = prevIndex;
            lineNumber = prevLineNumber;

            array->push_back(ParseNext(data, index, filepath, lineNumber));   
            u32 currentLine = lineNumber;
        }

        return array;
    }

    Value
    ParseNext(std::string& data, u64& index, const std::string& filepath, u32& lineNumber)
    {
        u32 currentLine = lineNumber;
        Token t = GetNextToken(data, index, lineNumber);

        if (t.type == Token::Type::CurlyBracket_Open)
            return Value(ParseObject(data, index, filepath, lineNumber));

        if (t.type == Token::Type::SquareBracket_Open)
            return Value(ParseArray(data, index, filepath, lineNumber));

        if (t.type == Token::Type::String)
            return Value(ParseString(t.value));
        
        if (t.type == Token::Type::Int)
            return Value(ParseInt(t.value));
        
        if (t.type == Token::Type::Float)
            return Value(ParseFloat(t.value));

        if (t.type == Token::Type::Value)
        {
            ToLower(t.value);

            if (t.value == "null")  return Value();
            if (t.value == "true")  return Value(true);
            if (t.value == "false") return Value(false);
        }

#       ifdef DEBUG
        std::cerr << "Error: Unexpected token " << t.value << " at: " << filepath << ":(" << currentLine << ")" << std::endl;
#       endif // DEBUG

        return Value(Value::Type::Error);
    }
    
    Value
    LoadJSONFile(const std::string& filepath)
    {
        #ifdef DEBUG
        std::cout << "Parsing file: " << filepath << std::endl;
        #endif // DEBUG
    
        std::string contents = LoadFile(filepath);

        u64 index = 0;
        u32 lineNumber = 1;

        Value doc = ParseNext(contents, index, filepath, lineNumber);
        return doc;
    }

} // namespace JSON