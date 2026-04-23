#include <build/builder.h>
#include <core/toolchain.h>
#include <core/glob.h>
#include <error.h>

#include <iostream>
#include <filesystem>
#include <mutex>
#include <unordered_map>

namespace stdfs = std::filesystem;

namespace ymk::build {

// global map for O(1) project lookup during dependency resolution
static std::unordered_map<string, Project*> project_map;

Builder::Builder(Workspace& ws) : workspace(ws) {
    cache.load(".");

    // index projects for fast dependency lookup
    project_map.clear();
    for (auto& p : workspace.projects) {
        project_map[p.name] = &p;
    }
}

void Builder::build(const string& config_name) {
    // TODO: Implement Topological Sort here to ensure correct build order.
    // For now, we rely on the definition order in the .ymk file.
    
    for (auto& proj : workspace.projects) {
        build_project(proj, config_name);
    }
    
    // save cache at the end
    cache.save();
}

string Builder::get_obj_path(const Project& proj, const string& src) {
    // ex: src/main.cpp -> build/obj/DoomEngine/main_HASH.o
    
    // Hash the full path to avoid collisions (e.g. src/main.cpp vs lib/main.cpp)
    size_t path_hash = std::hash<string>{}(src);
    string filename = stdfs::path(src).filename().string();
    
    return workspace.obj_dir + "/" + proj.name + "/" + filename + "_" + std::to_string(path_hash) + ".o";
}

void Builder::build_project(Project& proj, const string& config_name) {
    LOGFMT(
        PROJNAME,
        "builder",
        PURPLE_TEXT("Building Project: "),
        proj.name, " [", YELLOW_TEXT(config_name), "]\n"
    );

    // -------- CONFIGURATION MERGE
    Config final_config = proj.base_config;

    // Merge global workspace config
    final_config.merge(workspace.global_base_config);

    // Merge specific config (debug/release) from Global and Project scopes
    if (workspace.global_custom_configs.count(config_name)) {
        final_config.merge(workspace.global_custom_configs.at(config_name));
    }
    if (proj.custom_configs.count(config_name)) {
        final_config.merge(proj.custom_configs.at(config_name)); 
    }

    // Ensure compiler is set (Default to clang++)
    if (final_config.compiler.empty()) {
        final_config.compiler = "clang++";
    }

    // ---------- DEPENDENCY RESOLUTION
    for (const string& dep_name : proj.deps) {
        if (project_map.find(dep_name) == project_map.end()) {
            LOGFMT(PROJNAME, "builder", RED_TEXT("[ERROR]: "), 
                   "Unknown dependency '", dep_name, "' in project ", proj.name, "\n");
            continue; 
        }
        
        Project* dep = project_map[dep_name];
        
        // a. Inherit Public Includes
        final_config.includes.insert(
            final_config.includes.end(), 
            dep->base_config.includes.begin(), 
            dep->base_config.includes.end()
        );

        // b. Link against Dependency
        // We add the project name to 'links'. The Toolchain will handle formatting (e.g. -lDoomEngine or DoomEngine.lib)
        final_config.links.push_back(dep->name);
    }

    // ------------ LINKER FLAGS (OS/Compiler Specific)
    final_config.lib_dirs.push_back(workspace.dist_dir);

    // --------- RESOLVE SOURCE FILES
    std::vector<string> sources = ymk::fs::glob::resolve(proj.src_globs);
    if (sources.empty()) {
        LOGFMT(
            PROJNAME,
            "builder/files",
            RED_TEXT("[ERROR]: "),
            "didn't find any source files matching patterns in ", proj.name, "\n"
        );
        return;
    }

    // ------- PREPARE DIRECTORIES
    stdfs::create_directories(workspace.dist_dir);
    stdfs::create_directories(workspace.obj_dir + "/" + proj.name);

    // --------- COMPILE PHASE (Multi-threaded)
    std::vector<string> object_files;
    std::mutex obj_mutex;
    i32 tasks_dispatched = 0;

    for (const auto& src : sources) {
        string obj = get_obj_path(proj, src);
        
        {
            std::lock_guard<std::mutex> lock(obj_mutex);
            object_files.push_back(obj);
        }

        // Incremental Build Check
        if (cache.needs_recompile(proj, final_config, src)) {
            tasks_dispatched++;
            thread_pool.add_task([this, &proj, final_config, src, obj]() {
                this->compile_file(proj, final_config, src, obj);
            });
        }
    }

    // Wait for all threads to finish
    if (tasks_dispatched > 0) {
        LOGFMT(PROJNAME, "compile", CYAN_TEXT("Compiling "), tasks_dispatched, " files...\n");
        thread_pool.wait_idle();
    } else {
        LOGFMT(PROJNAME, "compile", GREEN_TEXT("Project Up to date!\n"));
    }

    // --------- LINK PHASE (Sequential)
    if (!object_files.empty()) {
        string out_ext = (proj.type == ArtifactType::Exe) ? ".exe" : 
                            (proj.type == ArtifactType::SharedLib) ? ".dll" : ".lib";
        string out_bin = workspace.dist_dir + "/" + proj.name + out_ext;
        
        CompileCmd link_cmd = Toolchain::create_link_cmd(proj, final_config, object_files, out_bin);
        
        LOGFMT(PROJNAME, "link", CYAN_TEXT("Linking "), out_bin, "...\n");
        
        // Execute Linker
        int ret = std::system(link_cmd.to_string().c_str());
        if (ret != 0) {
            LOGFMT(
                PROJNAME,
                "link",
                RED_TEXT("[ERROR]: "), "linking failed.\n",
                YELLOW_TEXT("\terror code: "), ret, "\n"
            );
        }
    }
}


#ifdef _WIN32
#include <windows.h>
#endif

// Add this helper function to your code
int execute_command_safely(const std::string& cmd_str) {
#ifdef IPLATFORM_WINDOWS
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CreateProcess requires a mutable string buffer
    std::string mutable_cmd = cmd_str;

    // bInheritHandles = FALSE is what fixes your thread-safety issue!
    if (!CreateProcessA(NULL, mutable_cmd.data(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        return -1; // Failed to start process
    }

    // Wait until child process exits
    WaitForSingleObject(pi.hProcess, INFINITE);

    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    // Close process and thread handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return static_cast<int>(exit_code);
#else
    // On Linux/Mac, std::system is slightly safer but still ideally 
    // you'd use posix_spawn() for a production build system.
    return std::system(cmd_str.c_str());
#endif
}


void Builder::compile_file(const Project& proj, const Config& cfg, const string& src, const string& obj) {
    CompileCmd cmd = Toolchain::create_compile_cmd(proj, cfg, src, obj);
    
    LOGFMT(PROJNAME, "build", CYAN_TEXT("[CC] "), src, "\n");
    
    int ret = execute_command_safely(cmd.to_string().c_str());
    if (ret != 0) {
        LOGFMT(
            PROJNAME,
            "build",
            RED_TEXT("[ERROR]: "), "Compilation Failed: ", src, "\n"
        );
        // Throwing here will likely terminate the thread std::terminate unless caught.
        // It is better to just log error, or use a thread-safe failure flag in Builder to stop linking.
        // For now, we allow it to continue to try compiling other files.
    }
}

} // namespace ymk::build