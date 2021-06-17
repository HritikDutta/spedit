#include "parser.h"

#include <string>
#include <string_view>
#include "containers/darray.h"
#include "platform/fileio.h"
#include "error_strings.h"
#include "json.h"
#include "lexer.h"

namespace json
{

static String EscapeToken(Parser& parser, const Token& token)
{
    String escaped;

    auto& view = token.value;

    for (size_t i = 0; i < view.size() && parser.errorCode == 0; i++)
    {
        if (view[i] == '\\')
        {
            i++;
            switch (view[i])
            {
                case 'b' : escaped.push_back('\b'); break;
                case 'f' : escaped.push_back('\f'); break;
                case 'n' : escaped.push_back('\n'); break;
                case 'r' : escaped.push_back('\r'); break;
                case 't' : escaped.push_back('\t'); break;
                case '\"': escaped.push_back('\"'); break;
                case '\\': escaped.push_back('\\'); break;
                default:
                {
                    parser.errorCode = 11;
                    parser.errorLineNumber = token.lineNumber;
                } break;
            }

            continue;
        }
        
        escaped.push_back(view[i]);
    }

    return std::move(escaped);
}

static void ParseNext(Parser& parser, const Lexer& lexer, Document& out);

static void ParseArray(Parser& parser, const Lexer& lexer, Document& out)
{
    size_t myIndex = out.dependencyTree.size();
    out.dependencyTree.emplace_back(DependencyNode::Type::ARRAY);

    // Skip the first [
    parser.current_token_index++;

    while (parser.errorCode == 0)
    {
        if (parser.current_token_index >= lexer.tokens.size())
        {
            parser.errorCode = 9;
            parser.errorLineNumber = lexer.tokens[parser.current_token_index - 1].lineNumber;
            break;
        }

        if (lexer.tokens[parser.current_token_index].type == Token::Type::SQUARE_BRACKET_CLOSE)
            break;
        
        auto& node = out.dependencyTree[myIndex];
        node._array.emplace_back(out.dependencyTree.size());
        ParseNext(parser, lexer, out);

        if (parser.errorCode != 0)
            break;

        if (parser.current_token_index >= lexer.tokens.size())
        {
            parser.errorCode = 8;
            parser.errorLineNumber = lexer.tokens[parser.current_token_index - 1].lineNumber;
            break;
        }

        if (lexer.tokens[parser.current_token_index].type == Token::Type::SQUARE_BRACKET_CLOSE)
            break;

        if (lexer.tokens[parser.current_token_index].type != Token::Type::COMMA)
        {
            parser.errorLineNumber = lexer.tokens[parser.current_token_index].lineNumber;
            parser.errorCode = 2;
            break;
        }

        parser.current_token_index++;
    }
}

static void ParseObject(Parser& parser, const Lexer& lexer, Document& out)
{
    size_t myIndex = out.dependencyTree.size();
    out.dependencyTree.emplace_back(DependencyNode::Type::OBJECT);

    // Skip the first {
    parser.current_token_index++;
    
    while (parser.errorCode == 0)
    {
        if (parser.current_token_index >= lexer.tokens.size())
        {
            parser.errorCode = 8;
            parser.errorLineNumber = lexer.tokens[parser.current_token_index - 1].lineNumber;
            break;
        }

        if (lexer.tokens[parser.current_token_index].type == Token::Type::CURLY_BRACKET_CLOSE)
            break;

        auto& node = out.dependencyTree[myIndex];

        auto& keyToken = lexer.tokens[parser.current_token_index++];
        if (keyToken.type != Token::Type::STRING)
        {
            parser.errorLineNumber = keyToken.lineNumber;
            parser.errorCode = 4;
            break;
        }

        {   // Check for semi colon
            auto& token = lexer.tokens[parser.current_token_index++];
            if (token.type != Token::Type::COLON)
            {
                parser.errorLineNumber = token.lineNumber;
                parser.errorCode = 5;
                break;
            }
        }

        String keyString = EscapeToken(parser, keyToken);
        if (parser.errorCode != 0)
            break;

        node._object[keyString] = out.dependencyTree.size(); 
        ParseNext(parser, lexer, out);

        if (parser.errorCode != 0)
            break;

        if (parser.current_token_index >= lexer.tokens.size())
        {
            parser.errorCode = 8;
            parser.errorLineNumber = lexer.tokens[parser.current_token_index - 1].lineNumber;
            break;
        }

        if (lexer.tokens[parser.current_token_index].type == Token::Type::CURLY_BRACKET_CLOSE)
            break;

        if (lexer.tokens[parser.current_token_index].type != Token::Type::COMMA)
        {
            parser.errorLineNumber = lexer.tokens[parser.current_token_index].lineNumber;
            parser.errorCode = 3;
            break;
        }

        parser.current_token_index++;
    }
}

static void ParseNext(Parser& parser, const Lexer& lexer, Document& out)
{
    if (parser.current_token_index >= lexer.tokens.size())
    {
        parser.errorCode = 6;
        parser.errorLineNumber = lexer.tokens[parser.current_token_index - 1].lineNumber;
        return;
    }

    auto& token = lexer.tokens[parser.current_token_index];

    switch (token.type)
    {
        case Token::Type::STRING:
        {
            size_t resourceIndex = out.resources.size();

            {   // Push Resource
                String value = EscapeToken(parser, token);
                out.resources.emplace_back(std::move(value));
            }

            {   // Push Node
                auto& node = out.dependencyTree.emplace_back(DependencyNode::Type::DIRECT);
                node._index = resourceIndex;
            }
        } break;

        case Token::Type::INTEGER:
        {
            size_t resourceIndex = out.resources.size();

            {   // Push Resource
                String numString = String { token.value.data(), token.value.size() };
                int64_t value = _atoi64(numString.c_str());
                out.resources.emplace_back(value);
            }

            {   // Push Node
                auto& node = out.dependencyTree.emplace_back(DependencyNode::Type::DIRECT);
                node._index = resourceIndex;
            }
        } break;

        case Token::Type::FLOAT:
        {
            size_t resourceIndex = out.resources.size();

            {   // Push Resource
                String numString = String { token.value.data(), token.value.size() };
                double value = atof(numString.c_str());
                out.resources.emplace_back(value);
            }

            {   // Push Node
                auto& node = out.dependencyTree.emplace_back(DependencyNode::Type::DIRECT);
                node._index = resourceIndex;
            }
        } break;

        case Token::Type::INDENTIFIER:
        {
            size_t resourceIndex = out.resources.size();
            
            {   // Push Resource
                if (token.value == "true")
                    out.resources.emplace_back(true);
                else if (token.value == "false")
                    out.resources.emplace_back(false);
                else if (token.value == "null")
                    resourceIndex = 0;    // Set to the common null resource
                else
                {
                    parser.errorLineNumber = token.lineNumber;
                    parser.errorCode = 1;
                    break;
                }
            }
            
            {   // Push Node
                auto& node = out.dependencyTree.emplace_back(DependencyNode::Type::DIRECT);
                node._index = resourceIndex;
            }
        } break;

        // Array
        case Token::Type::SQUARE_BRACKET_OPEN:
        {
            ParseArray(parser, lexer, out);
        } break;

        // Array
        case Token::Type::CURLY_BRACKET_OPEN:
        {
            ParseObject(parser, lexer, out);
        } break;

        default:
        {
            parser.errorCode = 7;
            parser.errorLineNumber = lexer.tokens[parser.current_token_index].lineNumber;
        } break;
    }

    parser.current_token_index++;
}

void Parser::ParseLexedOuput(const Lexer& lexer, Document& out)
{
    current_token_index = 0;
    errorCode = 0;

    out.dependencyTree.clear();
    out.resources.clear();

    // This is a null element
    // If user tries to access an object property that wasn't in the file,
    // then the value will point to this element
    auto& node = out.dependencyTree.emplace_back(DependencyNode::Type::DIRECT);
    node._index = 0;
    out.resources.emplace_back();

    if (lexer.tokens.size() > 0)
    {
        ParseNext(*this, lexer, out);

        // Check if more tokens are remaining after parsing
        if (errorCode == 0 && current_token_index < lexer.tokens.size())
        {
            errorLineNumber = lexer.tokens[current_token_index].lineNumber;
            errorCode = 10;
        }
    }
}

const char* Parser::GetErrorMessage() const
{
    return parser_error_strings[errorCode];
}

bool ParseFile(std::string json, Document& document)
{
    json::Lexer lexer;
    lexer.MoveContent(std::move(json));
    lexer.Lex();

    if (lexer.errorCode != 0)
        return false;

    json::Parser parser;
    parser.ParseLexedOuput(lexer, document);

    if (parser.errorCode != 0)
        return false;

    return true;
}

} // namespace json
