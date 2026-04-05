#define MINI_LOGGER_IMPLEMENTATION
#include <logger.h>
#include <defines.h>

#include <parser/lexer.h>
#include <parser/parser.h>
#include <core/toolchain.h>
#include <build/builder.h>
#include <cli/cmd.h> 

#include <fstream>
#include <iostream>

void generate_template(std::vector<std::string>& input, std::map<std::string, std::string>& args) {
    std::string path = args.count("config") ? args["config"] : "build.ymk";
    std::ofstream file(path);
    
    file << "workspace: DefaultWorkspace\n"
         << "dist: bin\n"
         << "obj: build/obj\n\n"
         << "compiler: clang++\n"
         << "c_std: c11\n"
         << "cpp_std: c++20\n\n"
         << "conf: debug {\n"
         << "    flags: [ -g, -O0 ]\n"
         << "    defines: [ YMAKE_DEBUG ]\n"
         << "}\n\n"
         << "conf: release {\n"
         << "    flags: [ -O3 ]\n"
         << "    defines: [ YMAKE_NDEBUG ]\n"
         << "}\n\n"
         << "project: App {\n"
         << "    kind: exe\n"
         << "    language: cpp\n"
         << "    src: [ \"src/**/*.cpp\" ]\n"
         << "    inc: [ \"src\" ]\n"
         << "    lib_dirs: [ ]\n\n"
         << "    platform: win {\n"
         << "        links: [ user32, gdi32, opengl32 ]\n"
         << "    }\n\n"
         << "    platform: linux {\n"
         << "        links: [ pthread, GL, GLU ]\n"
         << "    }\n"
         << "}\n";
         
    LOGFMT(PROJNAME, "init", GREEN_TEXT("Success: "), "Generated default template at ", PURPLE_TEXT(path), "\n");
}

void build_project(std::vector<std::string>& input, std::map<std::string, std::string>& args) {
    std::string config_path = args.count("config") ? args["config"] : "build.ymk";
    std::string mode = args.count("mode") ? args["mode"] : "debug";

    std::ifstream f(config_path);
    if (!f.is_open()) {
        LOGFMT(PROJNAME, "build", RED_TEXT("[ERROR]: "), "Could not open config file: ", config_path, "\n");
        return;
    }

    std::string source_code((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());

    try {
        ymk::Lexer lexer(source_code);
        auto tokens = lexer.scan_all();

        ymk::Parser parser(tokens);
        ymk::Workspace ws = parser.parse();

        ymk::build::Builder builder(ws);
        builder.build(mode);

    } catch (const std::exception& e) {
        LOGFMT(PROJNAME, "core", RED_TEXT("FATAL BUILD ERROR: "), e.what(), "\n");
    }
}

int main(int argc, char *argv[]) {
    LOG_CHANGE_PRIORITY(LOG_WARN);
    
    std::vector<std::string> cli_args;
    for (int i = 1; i < argc; i++) {
        cli_args.push_back(argv[i]);
    }

    if (cli_args.empty()) {
        cli_args.push_back("help");
    }

    std::vector<ymk::cli::Command> commands;

    commands.push_back(ymk::cli::Command(
        "build", 
        "Builds the project based on the configuration",
        {
            ymk::cli::CommandArgument("config", "Path to config file", "-c", "--config", ymk::cli::ValueType::String),
            ymk::cli::CommandArgument("mode", "Build configuration mode (e.g., debug, release)", "-m", "--mode", ymk::cli::ValueType::String)
        },
        build_project
    ));

    commands.push_back(ymk::cli::Command(
        "init", 
        "Generates a default build.ymk template in the current directory",
        {
            ymk::cli::CommandArgument("config", "Path to output config file", "-c", "--config", ymk::cli::ValueType::String)
        },
        generate_template
    ));

    commands.push_back(ymk::cli::Command(
        "help", 
        "Outputs toolchain usage information",
        {},
        [&commands](std::vector<std::string>&, std::map<std::string, std::string>&) {
            ymk::cli::output_help_info(commands);
        }
    ));

    try {
        ymk::cli::CommandInfo info = ymk::cli::parse_cli(cli_args, commands);
        
        // Let's only print the engine header if we are actually building
        if (info.cmd.name == "build") {
            LOGFMT(PROJNAME, "core", "YMake ", PURPLE_TEXT("v", VERSION_MAJOR, ".", VERSION_MINOR, ".", VERSION_PATCH, "\n"));
        }
        
        info.call_function();
    } catch (const std::exception& e) {
        // We handle specific logging inside parse_cli, so we just exit cleanly here
        return 1;
    }

    return 0;
}