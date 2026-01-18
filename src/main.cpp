#define MINI_LOGGER_IMPLEMENTATION
#include <logger.h>
#include <defines.h>

#include <parser/lexer.h>
#include <parser/parser.h>
#include <util/glob.h>

void print_list(const std::string& label, const std::vector<std::string>& list) {
    if (list.empty()) return;
    std::cout << "    " << label << ": [ ";
    for (const auto& i : list) std::cout << i << " ";
    std::cout << "]\n";
}

int main() {
    LOG_CHANGE_PRIORITY(LOG_WARN);

    LOGFMT(PROJNAME, "core", "YMake ",
           PURPLE_TEXT("v", VERSION_MAJOR, ".", VERSION_MINOR, ".",
                       VERSION_PATCH, "\n"));

    string src_code = R"(
        workspace: DoomWorkspace
        dist: "bin"
        obj: "build/obj"

        compiler: clang++
        c_std: c11
        cpp_std: c++20

        conf: debug {
            defines: [ YMAKE_DEBUG, _CRT_SECURE_NO_WARNINGS ]
            flags: [ -g, -O0, -Wall ]
        }

        conf: release {
            defines: [ YMAKE_NDEBUG ]
            flags: [ -O3, -march=native ]
        }

        project: DoomEngine {
            kind: shared
            language: cpp
            
            src: [ "DOOM/src/**/*.cpp", "DOOM/vendor/glad/glad.c", "src/**/*.cpp" ]
            inc: [ "DOOM/src", "DOOM/vendor" ]

            platform: win {
                links: [ user32, gdi32, vulkan-1 ]
                defines: [ YM_PLATFORM_WIN ]
            }
        }

        project: Sandbox {
            kind: exe
            language: cpp
            use: [ DoomEngine ]
            src: [ "Sandbox/src/**/*.cpp" ]
            
            on: prebuild {
                exec: "glslc Sandbox/assets/shader.vert -o bin/shader.spv"
            }
        }

        task: run {
            deps: [ Sandbox ]
            exec: "./bin/Sandbox.exe"
        }
    )";

    // 3. Run Lexer
    ymk::Lexer lexer(src_code);
    auto tokens = lexer.scan_all();
    
    // 4. Run Parser
    try {
        ymk::Parser parser(tokens);
        ymk::Workspace ws = parser.parse();

        // 5. Verify Output (The "Robot Arm" Results)
        std::cout << "--------------------------------------\n";
        std::cout << "PARSE SUCCESSFUL\n";
        std::cout << "--------------------------------------\n";
        
        std::cout << "Workspace Name: " << ws.name << "\n";
        std::cout << "Global C++ Std: " << ws.global_base_config.cpp_std.value_or("None") << "\n";
        
        // Check Global Debug Config
        if (ws.global_custom_configs.count("debug")) {
            std::cout << "Global Debug Flags: ";
            for (auto& f : ws.global_custom_configs["debug"].flags) std::cout << f << " ";
            std::cout << "\n";
        }

        if (ws.global_custom_configs.count("release")) {
            std::cout << "Global Release Flags: ";
            for (auto& f : ws.global_custom_configs["release"].flags) std::cout << f << " ";
            std::cout << "\n";
        }

        std::cout << "Compiler: " << ws.compiler << "\n";

        std::cout << "\n--- Projects (" << ws.projects.size() << ") ---\n";
        for (const auto& p : ws.projects) {
            std::cout << "Project: " << p.name << " (" << (p.type == ymk::ArtifactType::Exe ? "Exe" : "Shared") << ")\n";
            print_list("Src", p.src_globs);
            print_list("Inc", p.base_config.includes);
            print_list("Use", p.deps);
            
            // Check Hooks
            if (!p.pre_build_cmds.empty()) {
                std::cout << "    Pre-Build: " << p.pre_build_cmds[0] << "\n";
            }

            // Check Platform Specifics
            if (p.custom_configs.count("win")) {
                print_list("Win Links", p.custom_configs.at("win").links);
            }
            std::cout << "\n";
        }

        std::cout << "--- Tasks (" << ws.tasks.size() << ") ---\n";
        for (const auto& t : ws.tasks) {
            std::cout << "Task: " << t.name << "\n";
            print_list("Deps", t.deps);
            print_list("Cmds", t.cmds);
        }

        std::cout << "\n--- Resolved Files ---\n";
        for (const auto& p : ws.projects) {
            std::cout << "Project: " << p.name << "\n";
            
            vector<string> files = ymk::fs::glob::resolve(p.src_globs);
            
            if (files.empty()) {
                std::cout << "    [!] No files found matching patterns.\n";

                LOGFMT(
                    PROJNAME,
                    "file/glob",
                    RED_TEXT("GLOB ERROR:\t"),
                    "no files found matching patterns.\n"
                )
            }

            for (const auto& f : files) {
                std::cout << "    Found: " << f << "\n";
            }
        }

    } catch (const std::exception& e) {
        LOGFMT(
            PROJNAME,
            "parser",
            RED_TEXT("PARSE error:\t"),
            e.what(), "\n"
        )
    }

    return 0;
}