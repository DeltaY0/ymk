#pragma once

#include <defines.h>
#include <logger.h>
#include <error.h>

#include <parser/token.h>

namespace ymk {

class Lexer
{
private:
    string src;
    size_t cursor = 0;
    i32 line      = 1;
    i32 col       = 1;

public:
    Lexer(const string& source);

    // main method: get the next token
    Token scan_token();

    vector<Token> scan_all();

private:
    char peek(i32 offset = 0) const;
    char advance();
    bool match(char expected);
    bool is_at_end() const;

    // helpers
    void skip_whitespace();
    Token make_token(TokenType type, string text);
    Token error_token(string msg);

    // scanners
    Token scan_string();
    Token scan_number();
    Token scan_identifier();
};

}  // namespace ymk
