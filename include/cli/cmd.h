#pragma once

#include <defines.h>
#include <logger.h>
#include <error.h>

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <algorithm>

namespace ymk::cli {

enum class ValueType {
    None,
    Bool,
    String,
    Int,
    Float
};

struct CommandArgument {
    std::string name;
    std::string desc;
    std::string short_opt;
    std::string long_opt;
    ValueType val_type;

    CommandArgument(std::string name, std::string desc) 
        : name{name}, desc{desc}, val_type{ValueType::String} {}
        
    CommandArgument(std::string name, std::string desc, std::string short_opt, std::string long_opt)
        : name{name}, desc{desc}, short_opt{short_opt}, long_opt{long_opt}, val_type{ValueType::String} {}
        
    CommandArgument(std::string name, std::string desc, std::string short_opt, std::string long_opt, ValueType val_type)
        : name{name}, desc{desc}, short_opt{short_opt}, long_opt{long_opt}, val_type{val_type} {}
};

struct Command {
    std::string name;
    std::string desc;
    std::vector<CommandArgument> args;
    std::function<void(std::vector<std::string>&, std::map<std::string, std::string>&)> func_to_call;

    Command() = default;
    Command(std::string name, std::string desc) 
        : name{name}, desc{desc} {}

    Command(std::string name, std::string desc, std::vector<CommandArgument> args,
            std::function<void(std::vector<std::string>&, std::map<std::string, std::string>&)> func_to_call)
        : name{name}, desc{desc}, args{args}, func_to_call{func_to_call} {}

    void call_function(std::vector<std::string>& cmd_in, std::map<std::string, std::string>& arguments) const {
        if (func_to_call) {
            func_to_call(cmd_in, arguments);
        } else {
            LOGFMT(PROJNAME, "cli", RED_TEXT("[ERROR]: "), "tried calling a command's function that doesn't exist: ", name, "\n");
            throw std::runtime_error("Couldn't call the function for a command.");
        }
    }

    void output_command_info() const {
        LLOG(YELLOW_TEXT("USAGE: \n"));
        LLOG("  ymk ", CYAN_TEXT(name), "\t", desc, "\n");

        if (!args.empty()) {
            LLOG(PURPLE_TEXT("\targuments: \n"));
            for (const auto& arg : args) {
                LLOG("\t", arg.short_opt, ", ", arg.long_opt, 
                    "\t", CYAN_TEXT("<", arg.name, ">  "), arg.desc, "\n");
            }
        }
        LLOG("\n");
    }
};

struct CommandInfo {
    Command cmd;
    std::vector<std::string> cmd_in;
    std::map<std::string, std::string> arguments;

    void call_function() { 
        cmd.call_function(cmd_in, arguments); 
    }
};

CommandInfo parse_cli(const std::vector<std::string>& arguments, const std::vector<Command>& commands);
void output_help_info(const std::vector<Command>& commands);

} // namespace ymk::cli