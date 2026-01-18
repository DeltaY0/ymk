#include <core/toolchain.h>

#include <sstream>
#include <algorithm>

using std::stringstream;

namespace ymk
{

// ----- CompileCmd struct
string CompileCmd::to_string() const {
    stringstream ss;

    ss << program;
    for(const auto &arg : args)
        ss << ' ' << arg;

    return ss.str();
}

// ----- helper ------
static string to_lower(string s) {
    transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return tolower(c);
    });

    return s;
}

// -------------- toolchain ---------------

CompilerType Toolchain::detect(const string& cmd) {
    string s = to_lower(cmd);

    if (s.find("clang") != string::npos) return CompilerType::Clang;
    if (s.find("g++") != string::npos || s.find("gcc") != string::npos) return CompilerType::GCC;
    if (s.find("cl") != string::npos || s.find("msvc") != string::npos) return CompilerType::MSVC;

    return CompilerType::Unknown;
}

// --- internal flag generators ---

static void add_common_flags(vector<string>& args, CompilerType type, const Config& config) {
    // ------- standards
    if (config.cpp_std.has_value()) {
        if (type == CompilerType::MSVC) args.push_back("/std:" + config.cpp_std.value());
        else args.push_back("-std=" + config.cpp_std.value());
    }
    if (config.c_std.has_value()) {
        if (type != CompilerType::MSVC) args.push_back("-std=" + config.c_std.value());
        else args.push_back("/std:" + config.cpp_std.value());
    }

    // --------- includes
    for (const auto& inc : config.includes) {
        if (type == CompilerType::MSVC) args.push_back("/I" + inc);
        else args.push_back("-I" + inc);
    }

    // -------- defines
    for (const auto& def : config.defines) {
        if (type == CompilerType::MSVC) args.push_back("/D" + def);
        else args.push_back("-D" + def);
    }

    // ------- raw flags
    for (const auto& flag : config.flags) {
        args.push_back(flag);
    }

    // -------- optimization
    if (config.optimize.has_value()) {
        string level = config.optimize.value();
        if (type == CompilerType::MSVC) args.push_back("/O" + level); // /O2
        else args.push_back("-O" + level); // -O3
    }
}

// --- implementation ---
CompileCmd Toolchain::create_preprocess_cmd(const Project& proj, const Config& config, const string& src, const string& out) {
    // We assume C++ for simplicity, but logic could check file extension
    string compiler = config.compiler;
    
    CompilerType type = detect(compiler);
    CompileCmd cmd;
    cmd.program = compiler;

    // preprocess flag
    if (type == CompilerType::MSVC) cmd.args.push_back("/P"); 
    else cmd.args.push_back("-E");

    add_common_flags(cmd.args, type, config);

    cmd.args.push_back(src);
    
    // output flag
    if (type == CompilerType::MSVC)
        cmd.args.push_back("/Fi" + out);
    else {
        cmd.args.push_back("-o");
        cmd.args.push_back(out);
    }

    return cmd;
}

CompileCmd Toolchain::create_compile_cmd(const Project& proj, const Config& config, const string& src, const string& out) {
    string compiler = config.compiler;
    CompilerType type = detect(compiler);

    CompileCmd cmd;
    cmd.program = compiler;

    // compile only flag
    if (type == CompilerType::MSVC) cmd.args.push_back("/c");
    else cmd.args.push_back("-c");

    add_common_flags(cmd.args, type, config);
    
    // for shared libs (DLLs/SOs), we need Position Independent Code on linux
    if (proj.type == ArtifactType::SharedLib && type != CompilerType::MSVC) {
        cmd.args.push_back("-fPIC");
    }

    cmd.args.push_back(src);

    // output
    if (type == CompilerType::MSVC)
        cmd.args.push_back("/Fo" + out);
    else {
        cmd.args.push_back("-o");
        cmd.args.push_back(out);
    }

    return cmd;
}

CompileCmd Toolchain::create_link_cmd(const Project& proj, const Config& config, const vector<string>& objs, const string& out) {
    string compiler = config.compiler;
    CompilerType type = detect(compiler);
    
    CompileCmd cmd;
    cmd.program = compiler;

    if (proj.type == ArtifactType::SharedLib) {
        if (type == CompilerType::MSVC) cmd.args.push_back("/DLL");
        else cmd.args.push_back("-shared");
    }

    // input objects
    for (const auto& o : objs) cmd.args.push_back(o);

    // links (Libraries)
    for (const auto& lib : config.links) {
        if (type == CompilerType::MSVC) cmd.args.push_back(lib + ".lib");
        else cmd.args.push_back("-l" + lib);
    }

    // output
    if (type == CompilerType::MSVC)
        cmd.args.push_back("/Fe" + out);
    else {
        cmd.args.push_back("-o");
        cmd.args.push_back(out);
    }

    return cmd;
}

}  // namespace ymk
