#include <parser/parser.h>

#include <parser/keywords.h>

namespace ymk {

// ------------ init --------------------

Parser::Parser(const vector<Token>& tokens) : tokens(tokens) {
    // initially make active the global config
    active_config = &workspace.global_base_config;
}

Workspace Parser::parse() {
    while(!is_at_end()) {
        parse_statement();
    }
    return workspace;
}

// ---------- core parsing logic ------------

void Parser::parse_statement() {
    // key: value

    // stmt starts with an identifier
    const Token& key_tkn =
        consume(TokenType::Identifier, "expected identifier.");

    string key = key_tkn.text;

    consume(TokenType::Colon, "expected ':' after key.");

    // is this a block or a property?
    // property =>  key: value
    // block    =>  key: value {...}

    bool is_a_block = check(TokenType::Identifier) &&
                      (current + 1 < tokens.size() &&
                       tokens[current + 1].type == TokenType::LBrace);

    if(is_a_block) {
        string name =
            consume(TokenType::Identifier, "expected name for block.").text;
        consume(TokenType::LBrace, "expected '{' to start block.");

        parse_block(key, name);  // recurse
        return;
    }

    // otherwise it's just a property (e.g cpp_std: c++20)
    parse_property(key);
}

void Parser::parse_block(const string& type, const string& name) {
    // step 1 => save state
    Project* prev_proj = active_project;
    Config* prev_conf  = active_config;

    // ---------------------------------------------------------
    // HANDLE TASKS
    // ---------------------------------------------------------
    if(type == keywords::BlockTask) {
        Task t;
        t.name = name;
        workspace.tasks.push_back(t);

        while(!check(TokenType::RBrace) && !is_at_end()) {
            Token tkey = consume(TokenType::Identifier, "expected task property");
            consume(TokenType::Colon, "expected ':'");

            // 1. Handle Dependencies
            if(tkey.text == keywords::KeyDeps) {
                workspace.tasks.back().deps = parse_value_list();
            } 
            // 2. Handle Commands
            else if(tkey.text == keywords::KeyCmd) {
                if(check(TokenType::String)) {
                    workspace.tasks.back().cmds.push_back(advance().text); 
                } else if(check(TokenType::LBracket)) {
                    workspace.tasks.back().cmds = parse_value_list();
                }
            } 
            // 3. Unknown Key Safety Net
            else {
                LOGFMT(PROJNAME, "parser", YELLOW_TEXT("WARNING: "), "Ignored task key '", tkey.text, "'\n");
                // Consume value safely to avoid crash
                if (check(TokenType::LBracket)) parse_value_list();
                else if (check(TokenType::String)) advance();
            }
        }
        consume(TokenType::RBrace, "expected '}'");
        return;
    }

    // ---------------------------------------------------------
    // HANDLE HOOKS
    // ---------------------------------------------------------
    if(type == keywords::BlockOn) {
        if(!active_project) {
            LOGFMT(PROJNAME, "parser", RED_TEXT("PARSING ERROR: "),
                   YELLOW_TEXT("'on:' block "), "must be inside a project.\n");
        }

        vector<string>* target_vec = nullptr;
        if(name == keywords::ValPreBuild) target_vec = &active_project->pre_build_cmds;
        else if(name == keywords::ValPostBuild) target_vec = &active_project->post_build_cmds;

        while(!check(TokenType::RBrace) && !is_at_end()) {
            Token tkey = consume(TokenType::Identifier, "expected command key (exec)");
            consume(TokenType::Colon, "expected ':' ");

            if(tkey.text == keywords::KeyCmd && target_vec) {
                if(check(TokenType::String)) {
                    // Use advance().text to get the token content
                    target_vec->push_back(advance().text); 
                }
            } 
            else {
                LOGFMT(PROJNAME, "parser", YELLOW_TEXT("WARNING: "), "Ignored command key '", tkey.text, "'\n");
                if (check(TokenType::String)) advance(); 
            }
        }

        consume(TokenType::RBrace, "expected '}'");
        return;
    }

    // ---------------------------------------------------------
    // STANDARD BLOCKS (Project/Platform)
    // ---------------------------------------------------------
    if(type == keywords::BlockProject) {
        Project proj;
        proj.name = name;
        workspace.projects.push_back(proj);
        active_project = &workspace.projects.back();
        active_config  = &active_project->base_config;
    } else if(type == keywords::BlockPlatform) {
        if(active_project) active_config = &active_project->custom_configs[name];
        else active_config = &workspace.global_custom_configs[name];
    } else if(type == keywords::BlockConf) {
        if(active_project) active_config = &active_project->custom_configs[name];
        else active_config = &workspace.global_custom_configs[name];
    }

    while(!check(TokenType::RBrace) && !is_at_end()) {
        parse_statement();
    }

    consume(TokenType::RBrace, "expected '}'");

    active_project = prev_proj;
    active_config  = prev_conf;
}

void Parser::parse_property(const string& key) {
    // workspace globals
    if(key == keywords::Workspace) {
        workspace.name = parse_value_string();
        return;
    }

    if(key == keywords::WorkspaceDist) {
        workspace.dist_dir = parse_value_string();
        return;
    }
    if(key == keywords::WorkspaceObj) {
        workspace.obj_dir = parse_value_string();
        return;
    }

    if(key == keywords::Compiler) {
        workspace.global_base_config.compiler = parse_value_string();

        if(active_config)
            active_config->compiler = workspace.global_base_config.compiler;

        return;
    }

    // project properties
    if(active_project) {
        if(key == keywords::KeyKind) {
            string val = parse_value_string();
            if(val == keywords::ValExe) active_project->type = ArtifactType::Exe;
            else if(val == keywords::ValShared) active_project->type = ArtifactType::SharedLib;
            else if(val == keywords::ValStatic) active_project->type = ArtifactType::StaticLib;
            return;
        }
        if(key == keywords::KeyLang) { active_project->language = parse_value_string(); return; }
        if(key == keywords::KeySrc) { active_project->src_globs = parse_value_list(); return; }
        if(key == keywords::KeyUse) { active_project->deps = parse_value_list(); return; }
    }

    // configuration properties
    if(active_config) {
        if(key == keywords::KeyCStd) { active_config->c_std = parse_value_string(); return; }
        if(key == keywords::KeyCppStd) { active_config->cpp_std = parse_value_string(); return; }
        
        if(key == keywords::KeyStd) {
            string val = parse_value_string();
            if(val.find("c++") != string::npos || val.find("C++") != string::npos)
                active_config->cpp_std = val;
            else active_config->c_std = val;
            return;
        }

        if(key == keywords::KeyOpt) { active_config->optimize = parse_value_string(); return; }
        if(key == keywords::KeyDefines) { active_config->defines = parse_value_list(); return; }
        if(key == keywords::KeyFlags) { active_config->flags = parse_value_list(); return; }
        if(key == keywords::KeyInc) { active_config->includes = parse_value_list(); return; }
        if(key == keywords::KeyLinks) { active_config->links = parse_value_list(); return; }
    }

    LOGFMT(PROJNAME, "parser", YELLOW_TEXT("WARNING: "), "Unknown property '", key, "'\n");
    
    // Peek to see if it's a list or a single value
    if(check(TokenType::LBracket)) {
        parse_value_list();
    } else {
        parse_value_string();
    }
}

string Parser::parse_value_string() {
    if(match(TokenType::String)) return prev().text;
    if(match(TokenType::Identifier)) return prev().text;
    if(match(TokenType::Number)) return prev().text;

    // error
    LOGFMT(PROJNAME, "parse", RED_TEXT("ERROR:\t"), "parsing value string returns '' instead\n");
    return "";
}

vector<string> Parser::parse_value_list() {
    // [item, item, item]
    vector<string> list;
    consume(TokenType::LBracket, "expected '['");

    if(check(TokenType::RBracket)) {
        advance();
        return list;
    }

    do{
        list.push_back(parse_value_string());
    }
    while(match(TokenType::Comma));

    consume(TokenType::RBracket, "expected ']'");
    return list;
}

// ---------------- helpers --------------------

const Token& Parser::peek() const {
    return tokens[current];
}

const Token& Parser::prev() const {
    return tokens[current - 1];
}

bool Parser::is_at_end() const {
    return peek().type == TokenType::EndOfFile;
}

const Token& Parser::advance() {
    if(!is_at_end()) current++;
    return prev();
}

bool Parser::check(TokenType type) const {
    if(is_at_end()) return false;
    return peek().type == type;
}

bool Parser::match(TokenType type) {
    if(check(type)) {
        advance();
        return true;
    }

    return false;
}

const Token& Parser::consume(TokenType type, const string& msg) {
    if(check(type)) return advance();

    i32 lineno = tokens[current].line;
    i32 colno = tokens[current].col;

    LOGFMT(
        PROJNAME, "parser",
        RED_TEXT("PARSE ERROR:\t"),
        YELLOW_TEXT("ln: "), lineno, ", ", YELLOW_TEXT("col: "), colno, "\n"
        "\t", msg, "\n"
    );

    YTHROW("parse error");
}

}  // namespace ymk
