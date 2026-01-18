#include <parser/lexer.h>

#include <cctype>

namespace ymk {

Lexer::Lexer(const string& source) : src(source), cursor(0), line(1), col(1) {}

char Lexer::peek(i32 offset) const {
    if(cursor + offset >= src.length()) return '\0';

    return src[cursor + offset];
}

char Lexer::advance() {
    char c = src[cursor++];
    col++;
    return c;
}

bool Lexer::match(char expected) {
    return false;
}

bool Lexer::is_at_end() const {
    return cursor >= src.length();
}

Token Lexer::make_token(TokenType type, string text) {
    // col - length because we already advanced
    return Token{type, text, line, col - (i32)text.length()};
}

void Lexer::skip_whitespace() {
    while(true) {
        char c = peek();
        switch(c) {
            case ' ':
            case '\r':
            case '\t':
                advance();  // ignore white space in any kind
                break;
            case '\n':  // reset to new line
                line++;
                col = 1;
                cursor++;
                break;
            case '#':  // comment
                while(peek() != '\n' && !is_at_end()) advance();
                break;
            default: return;
        }
    }
}

Token Lexer::error_token(string msg) {
    return Token{TokenType::Error, msg, line, col};
}

Token Lexer::scan_string() {
    string literal;

    // assume we already consumed the opening qoute
    while(peek() != '"' && !is_at_end()) {
        if(peek() == '\n') line++;
        literal += advance();
    }

    if(is_at_end()) return error_token("unterminated string");

    advance();  // consume the closing "
    return make_token(TokenType::String, literal);
}

Token Lexer::scan_number() {
    string literal;
    literal += src[cursor - 1];  // previous digit (which was consumed)

    while(isdigit(peek())) {
        literal += advance();
    }

    return make_token(TokenType::Number, literal);
}

Token Lexer::scan_identifier() {
    string literal;
    literal += src[cursor - 1];  // first character (was already consumed)

    while(isalnum(peek()) || peek() == '_' || peek() == '+' || peek() == '-' || peek() == '=') {
        literal += advance();
    }

    return make_token(TokenType::Identifier, literal);
}

Token Lexer::scan_token() {
    skip_whitespace();
    if(is_at_end()) return make_token(TokenType::EndOfFile, "");

    // consume first character of the token
    char c = advance();

    // IDs and numbers
    if(isalpha(c) || c == '_' || c == '-' || c == '=') return scan_identifier();
    if(isdigit(c)) return scan_number();

    // strings
    if(c == '"') return scan_string();

    // control
    switch(c) {
        case ':': return make_token(TokenType::Colon, ":");
        case '{': return make_token(TokenType::LBrace, "{");
        case '}': return make_token(TokenType::RBrace, "}");
        case '[': return make_token(TokenType::LBracket, "[");
        case ']': return make_token(TokenType::RBracket, "]");
        case ',': return make_token(TokenType::Comma, ",");
    }

    return error_token(string("unexpected character: ") + c);
}

vector<Token> Lexer::scan_all() {
    vector<Token> tokens;

    while(true) {
        Token token = scan_token();
        tokens.push_back(token);
        if(token.type == TokenType::EndOfFile) break;
    }

    return tokens;
}

}  // namespace ymk
