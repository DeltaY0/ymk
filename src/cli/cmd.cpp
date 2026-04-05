#include <cli/cmd.h>

namespace ymk::cli {

CommandInfo parse_cli(const std::vector<std::string>& arguments, const std::vector<Command>& commands) {
    std::vector<std::string> args = arguments;
    Command called_cmd;
    bool cmd_found = false;

    // 1. Identify the core command
    for (const auto& cmd : commands) {
        if (args.front() == cmd.name) {
            called_cmd = cmd;
            args.erase(args.begin());
            cmd_found = true;
            break;
        }
    }

    if (!cmd_found && !args.empty()) {
        LOGFMT(PROJNAME, "cli", RED_TEXT("[ERROR]: "), "Unknown command '", args.front(), "'\n");
        throw std::runtime_error("Unknown command");
    }

    std::map<std::string, std::string> found_available_args;
    std::vector<std::string> cmd_in;
    std::vector<std::string> used_args;

    // 2. Parse the arguments matching the command's expected options
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i][0] == '-') {
            bool found = false;
            for (const auto& arg : called_cmd.args) {
                if (args[i] == arg.short_opt || args[i] == arg.long_opt) {
                    found = true;
                    if (arg.val_type == ValueType::Bool || arg.val_type == ValueType::None) {
                        found_available_args[arg.name] = "true";
                        used_args.push_back(args[i]);
                    } else if ((i + 1) < args.size() && args[i + 1][0] != '-') {
                        found_available_args[arg.name] = args[i + 1];
                        used_args.push_back(args[i]);
                        used_args.push_back(args[i + 1]);
                        i++;
                    } else {
                        LOGFMT(PROJNAME, "cli", RED_TEXT("[ERROR]: "), "Missing value for argument '", args[i], "'\n");
                        called_cmd.output_command_info();
                        throw std::runtime_error("Incorrect arguments");
                    }
                    break; // Move to next arg in the main loop
                }
            }

            if (!found) {
                LOGFMT(PROJNAME, "cli", RED_TEXT("[ERROR]: "), "Invalid argument '", args[i], "' for command '", called_cmd.name, "'\n");
                called_cmd.output_command_info();
                throw std::runtime_error("Invalid argument");
            }
        }
    }

    // 3. Collect standard input (non-flag arguments like project names)
    for (const auto& arg : args) {
        if (std::find(used_args.begin(), used_args.end(), arg) == used_args.end()) {
            cmd_in.push_back(arg);
        }
    }

    return { called_cmd, cmd_in, found_available_args };
}

void output_help_info(const std::vector<Command>& commands) {
    LLOG(BLUE_TEXT("YMake v"), VERSION_MAJOR, ".", VERSION_MINOR, ".", VERSION_PATCH, "\n\n");
    LLOG("A fast, simple build tool for C/C++.\n\n");
    LLOG(GREEN_TEXT("USAGE: \n"));

    for (const auto& cmd : commands) {
        LLOG("  ymk ", CYAN_TEXT(cmd.name));
        
        std::string padding(std::max(0, 10 - (int)cmd.name.length()), ' ');
        LLOG(padding, cmd.desc, "\n");

        for (const auto& arg : cmd.args) {
            LLOG("\t", arg.short_opt, ", ", arg.long_opt, 
                 "  \t", CYAN_TEXT("<", arg.name, ">  "), arg.desc, "\n");
        }
        LLOG("\n");
    }
}

} // namespace ymk::cli