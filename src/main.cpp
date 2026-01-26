#define MINI_LOGGER_IMPLEMENTATION
#include <logger.h>
#include <defines.h>

#include <parser/lexer.h>
#include <parser/parser.h>
#include <core/toolchain.h>
#include <build/builder.h>

void print_list(const std::string& label, const std::vector<std::string>& list) {
    if (list.empty()) return;
    std::cout << "    " << label << ": [ ";
    for (const auto& i : list) std::cout << i << " ";
    std::cout << "]\n";
}

int main(int argc, char *argv[]) {
    LOG_CHANGE_PRIORITY(LOG_WARN);

    LOGFMT(PROJNAME, "core", "YMake ",
           PURPLE_TEXT("v", VERSION_MAJOR, ".", VERSION_MINOR, ".",
                       VERSION_PATCH, "\n"));

    string config_path = "build.ymk"; // Default name
    if (argc > 1) config_path = argv[1];

    std::ifstream f(config_path);
    if (!f.is_open()) {
        // Fallback to internal test string if file missing (for your debugging)
        // Or just error out. Let's use your test string logic if you prefer,
        // but for a real tool, we read files.
        std::cerr << "Could not open " << config_path << ". Using internal test data.\n";
        // ... (Insert your string source_code = R"(...)" here if you want fallback)
    }
    
    // Read whole file into string
    std::string source_code((std::istreambuf_iterator<char>(f)),
                             std::istreambuf_iterator<char>());

    if (source_code.empty()) {
        // Fallback to the DoomEngine string you had before for testing
        source_code = R"(
            workspace: DoomWorkspace
            dist: "bin"
            obj: "build/obj"
            
            c_std: c11
            cpp_std: c++20

            conf: debug {
                flags: [ "-g", "-O0" ]
            }

            project: DoomEngine {
                kind: shared
                src: [ "src/core/glob.cpp" ] 
                # Using valid file for test ^
            }
        )";
    }

    // 2. Run Pipeline
    try {
        // A. Lex
        ymk::Lexer lexer(source_code);
        auto tokens = lexer.scan_all();

        // B. Parse
        ymk::Parser parser(tokens);
        ymk::Workspace ws = parser.parse();

        // C. Build
        ymk::build::Builder builder(ws);
        builder.build("debug");

    } catch (const std::exception& e) {
        std::cerr << "BUILD FAILED: " << e.what() << "\n";
        return 1;
    }

    return 0;
}