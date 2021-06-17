#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include "containers/darray.h"

namespace json
{

struct Token
{
    enum struct Type
    {
        NONE,
        INDENTIFIER,
        STRING,
        INTEGER,
        FLOAT,

        // Punctuations
        SQUARE_BRACKET_OPEN  = '[',
        SQUARE_BRACKET_CLOSE = ']',
        CURLY_BRACKET_OPEN   = '{',
        CURLY_BRACKET_CLOSE  = '}',
        COLON                = ':',
        COMMA                = ',',

        ILLEGAL,
        NUM_TYPES
    };

    Type type { Type::NONE };
    uint64_t  lineNumber;
    std::string_view value;

    Token(Type type, uint64_t lineNumber, std::string_view&& value)
    :   type(type), lineNumber(lineNumber), value(std::move(value)) {}
};

struct Lexer
{
    std::string content;
    gn::darray<Token> tokens;

    uint64_t current_index;
    uint64_t current_line;

    uint64_t errorLineNumber;
    int errorCode;

    void CopyContent(const std::string& c);
    void MoveContent(std::string&& c);

    void Lex();
    
    const char* GetErrorMessage() const;
};


} // namespace json
