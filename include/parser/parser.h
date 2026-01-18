#pragma once

#include <defines.h>
#include <logger.h>
#include <error.h>

#include <core/typedefs.h>
#include <parser/lexer.h>

namespace ymk {

class Parser
{
public:
    // constructor
    Parser(const vector<Token>& tokens);

    // entry point
    Workspace parse();

private:
    // state
    const vector<Token>& tokens;
    size_t current = 0;  // current token pointer

    // output object
    Workspace workspace;

    // current project. if nullptr -> write to global
    Project* active_project = nullptr;

    // if not null, load config here
    Config* active_config = nullptr;

    // helpers
    const Token& peek() const;
    const Token& prev() const;
    bool is_at_end() const;
    const Token& advance();
    bool check(TokenType type) const;
    bool match(TokenType type);
    const Token& consume(TokenType type, const string& msg);

    // grammar
    void parse_statement();

    // key: value
    void parse_property(const string& key);

    // type: name {...}
    void parse_block(const string& type, const string& name);

    // values
    string parse_value_string();
    vector<string> parse_value_list();
};

}  // namespace ymk