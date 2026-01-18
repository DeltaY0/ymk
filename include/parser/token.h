#pragma once

#include <defines.h>
#include <logger.h>

#include <string_view>

namespace ymk {

enum class TokenType
{
    EndOfFile = 0,
    Error,

    // data types
    Identifier,  // "debug", "c++20", "-Wall", "project"
    String,      // "src/main.cpp"
    Number,      // 1, 3, (pure integers)

    // structure
    Colon,     // :
    LBrace,    // {
    RBrace,    // }
    LBracket,  // [
    RBracket,  // ]
    Comma,     // ,

};

struct Token
{
    TokenType type;
    string text;
    i32 line;
    i32 col;

    // helper for debugging
    string to_string() const {
        switch(type) {
            case TokenType::EndOfFile: return "EOF";
            case TokenType::Error: return "ERROR";
            case TokenType::Identifier: return "ID(" + text + ")";
            case TokenType::String: return "STR(\"" + text + "\")";
            case TokenType::Number: return "NUM(" + text + ")";
            case TokenType::Colon: return "COLON";
            case TokenType::LBrace: return "{";
            case TokenType::RBrace: return "}";
            case TokenType::LBracket: return "[";
            case TokenType::RBracket: return "]";
            case TokenType::Comma: return "COMMA";
            default: return "UNKNOWN";
        }
    }
};

}  // namespace ymk