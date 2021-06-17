#pragma once

#include "containers/darray.h"
#include "json.h"
#include "lexer.h"

namespace json
{

struct Parser
{
    size_t current_token_index;
    void ParseLexedOuput(const Lexer& lexer, Document& out);

    int errorCode;
    int errorLineNumber;

    const char* GetErrorMessage() const;
};

bool ParseFile(std::string json, Document& document);

} // namespace json
