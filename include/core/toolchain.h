#pragma once

#include <defines.h>
#include <core/typedefs.h>

#include <optional>

namespace ymk {

enum class CompilerType {
    Unknown = 0,
    Clang,
    GCC,
    MSVC
};

struct CompileCmd {
    string program; // "clang++" or "cl.exe" etc...
    vector<string> args; // ["-c", "main.cpp", ...]

    // to get full shell string
    string to_string() const;
};

class Toolchain {
public:
    static CompilerType detect(const string &compiler_cmd);

    // preprocessing for incremental builds
    static CompileCmd create_preprocess_cmd(
        const Project &proj,
        const Config  &conf, // merged config
        const string  &srcfile,
        const string  &outfile
    );

    // generate (ex): clang++ -c src/main.cpp
    static CompileCmd create_compile_cmd(
        const Project &proj,
        const Config  &conf,
        const string  &srcfile,
        const string  &objfile
    );

    // generates the output assembly cmd
    static CompileCmd create_link_cmd(
        const Project &proj,
        const Config  &conf,
        const vector<string> &obj_files,
        const string &output_file
    );

private:

    // abstraction for different flag syntax
    static string flag_compile_only(CompilerType type);
    static string flag_out(CompilerType type, const string &path);
    static string flag_std(CompilerType type, const string& std_ver);
    static string flag_opt(CompilerType type, const string& level);
};
    
} // namespace ymk