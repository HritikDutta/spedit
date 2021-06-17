#include "lexer.h"

#include <cstdint>
#include <string>
#include <string_view>
#include "containers/darray.h"
#include "error_strings.h"

namespace json
{

void Lexer::CopyContent(const std::string& c)
{
    content = c;
}

void Lexer::MoveContent(std::string&& c)
{
    content = std::move(c);
}

inline bool IsWhitespace(char ch)
{
    return ch == ' '  ||
           ch == '\t' ||
           ch == '\r' ||
           ch == '\n';
}

inline bool IsDigit(char ch)
{
    return (ch >= '0' && ch <= '9');
}

inline bool IsAlphabet(char ch)
{
    return (ch >= 'a' && ch <= 'z') ||
           (ch >= 'A' && ch <= 'Z');
}

static inline void EatSpaces(Lexer& lexer)
{
    while (IsWhitespace(lexer.content[lexer.current_index]))
    {
        lexer.current_line += (lexer.content[lexer.current_index] == '\n');
        lexer.current_index++;
    }
}

static inline std::string_view GetStringToken(Lexer& lexer, const std::string_view& content_view)
{
    // Skip the 1st '"'
    if (content_view[lexer.current_index] == '\"')
        lexer.current_index++;
    
    uint64_t start = lexer.current_index;
    while (content_view[lexer.current_index] != '\"')
    {
        if (content_view[lexer.current_index] == '\n' ||
            content_view[lexer.current_index] == '\0')
        {
            lexer.errorLineNumber = lexer.current_line;
            lexer.errorCode = 1;
            break;
        }

        lexer.current_index += (content_view[lexer.current_index] == '\\');
        lexer.current_index++;
    }

    // Skip the 2nd '"'
    lexer.current_index++;

    return content_view.substr(start, lexer.current_index - start - 1);
}

static inline std::string_view GetNumberToken(Lexer& lexer, const std::string_view& content_view, Token::Type& type)
{
    bool isNegative = (content_view[lexer.current_index] == '-');
    bool dotEncountered = false;

    uint64_t start = lexer.current_index;

    lexer.current_index += isNegative;

    // Also checking for - in between a number for error checking
    while (IsDigit(content_view[lexer.current_index]) ||
           content_view[lexer.current_index] == '.'   ||
           content_view[lexer.current_index] == '-')
    {
        if (content_view[lexer.current_index] == '-')
        {
            lexer.errorLineNumber = lexer.current_line;
            lexer.errorCode = 2;
            break;
        }

        if (content_view[lexer.current_index] == '.')
        {
            if (dotEncountered)
            {
                lexer.errorLineNumber = lexer.current_line;
                lexer.errorCode = 3;
                break;
            }

            dotEncountered = true;
        }
        lexer.current_index++;
    }

    type = (dotEncountered) ? Token::Type::FLOAT : Token::Type::INTEGER;

    return content_view.substr(start, lexer.current_index - start);
}

static inline std::string_view GetIdentifierToken(Lexer& lexer, const std::string_view& content_view)
{
    uint64_t start = lexer.current_index;

    while (IsAlphabet(content_view[lexer.current_index]))
        lexer.current_index++;

    return content_view.substr(start, lexer.current_index - start);
}

void Lexer::Lex()
{
    current_index = 0;
    current_line  = 1;

    errorLineNumber = 0;
    errorCode = 0;

    tokens.clear();
    tokens.reserve(std::max((size_t) 2, content.size() / 3)); // Just an estimate

    std::string_view view = content;

    bool keepLexing = true;
    while (keepLexing && errorCode == 0)
    {
        EatSpaces(*this);

        switch (content[current_index])
        {
            case '\0':
            {
                keepLexing = false;
            } break;

            // Punctuations
            case (char) Token::Type::SQUARE_BRACKET_OPEN:
            case (char) Token::Type::SQUARE_BRACKET_CLOSE:
            case (char) Token::Type::CURLY_BRACKET_OPEN:
            case (char) Token::Type::CURLY_BRACKET_CLOSE:
            case (char) Token::Type::COLON:
            case (char) Token::Type::COMMA:
            {
                auto type = (Token::Type) content[current_index];
                tokens.emplace_back(type, current_line, view.substr(current_index, 1));
                current_index++;
            } break;

            // Strings
            case '\"':
            {
                tokens.emplace_back(Token::Type::STRING, current_line, GetStringToken(*this, view));
            } break;

            default:
            {
                char startChar = content[current_index];
                if (startChar == '-' || startChar == '.' || IsDigit(startChar))
                {
                    Token::Type type;
                    std::string_view numView = GetNumberToken(*this, view, type);
                    tokens.emplace_back(type, current_line, std::move(numView));
                }
                else
                    tokens.emplace_back(Token::Type::INDENTIFIER, current_line, GetIdentifierToken(*this, view));

            } break;
        }
    }
}

const char* Lexer::GetErrorMessage() const
{
    return lexer_error_strings[errorCode];
}

} // namespace json