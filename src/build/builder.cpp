#include <build/builder.h>

#include <error.h>

#include <core/glob.h>

#include <mutex>
#include <filesystem>
namespace stdfs = std::filesystem;

namespace ymk::build
{

Builder::Builder(Workspace& ws) : workspace(ws) {
    // init cache
    cache.load(".");
}

void Builder::build(const string& config_name) {

    // TODO: topological sort for dependencies.
    for (auto& proj : workspace.projects) {
        build_project(proj, config_name);
    }
    
    // save cache at the end
    cache.save();
}

string Builder::get_obj_path(const Project& proj, const string& src) {
    // ex: src/main.cpp -> build/obj/DoomEngine/main.o

    // convert to full path for proper unique hashing
    size_t path_hash = std::hash<string>{}(src);
    string filename = stdfs::path(src).filename().string();
    
    return workspace.obj_dir + "/" + proj.name + "/" + filename + "_" + std::to_string(path_hash) + ".o";
}

void Builder::build_project(Project& proj, const string& config_name) {
    LOGFMT(
        PROJNAME,
        "builder",
        PURPLE_TEXT("Building Project: "),
        proj.name, " [", CYAN_TEXT(config_name), "]", "\n"
    );

    // --------- merge configs
    Config final_config = proj.base_config;

    // merge global workspace config
    final_config.merge(workspace.global_base_config); 

    // merge specific config (debug/release)
    if (proj.custom_configs.count(config_name)) {
        final_config.merge(proj.custom_configs.at(config_name)); 
    }

    // ---------- resolve files
    std::vector<string> sources = fs::glob::resolve(proj.src_globs);
    if (sources.empty()) {
        LOGFMT(
            PROJNAME,
            "builder/files",
            RED_TEXT("[ERROR]: "),
            "didn't find any source files in path: ",
            YELLOW_TEXT(proj.src_globs), "\n"
        );

        return;
    }

    // --------- create out dirs
    stdfs::create_directories(workspace.dist_dir);
    stdfs::create_directories(workspace.obj_dir + "/" + proj.name);

    // ---------- compile phase (threaded)
    std::vector<string> object_files;
    std::mutex obj_mutex;
    int tasks_dispatched = 0;

    for (const auto& src : sources) {
        string obj = get_obj_path(proj, src);
        
        {
            std::lock_guard<std::mutex> lock(obj_mutex);
            object_files.push_back(obj);
        }

        // check cache
        if (cache.needs_recompile(proj, final_config, src)) {
            tasks_dispatched++;
            thread_pool.add_task([this, &proj, final_config, src, obj]() {
                this->compile_file(proj, final_config, src, obj);
            });
        }
    }

    // wait for compilation
    if (tasks_dispatched > 0) {
        LOGFMT(
            PROJNAME,
            "compile",
            CYAN_TEXT("Compiling "), tasks_dispatched, " files...\n"
        );

        thread_pool.wait_all();
    } else {
        LOGFMT(
            PROJNAME,
            "compile",
            GREEN_TEXT("Project Up to date!\n")
        )
    }

    // ---- Link Phase (sequential)
    // only link if we have objects
    if (!object_files.empty()) {
        string out_ext = (proj.type == ArtifactType::Exe) ? ".exe" : 
                            (proj.type == ArtifactType::SharedLib) ? ".dll" : ".lib";
        string out_bin = workspace.dist_dir + "/" + proj.name + out_ext;
        
        CompileCmd link_cmd = Toolchain::create_link_cmd(proj, final_config, object_files, out_bin);
        
        std::cout << "  Linking " << out_bin << "...\n";

        LOGFMT(
            PROJNAME,
            "link",
            CYAN_TEXT("Linking "), out_bin, "...\n"
        )

        i32 ret = std::system(link_cmd.to_string().c_str());
        if (ret != 0) {
            LOGFMT(
                PROJNAME,
                "link",
                RED_TEXT("[ERROR]: "), "linking failed.\n",
                YELLOW_TEXT("\terror code: "), ret
            );
        }
    }
}

void Builder::compile_file(const Project& proj, const Config& cfg, const string& src, const string& obj) {
    CompileCmd cmd = Toolchain::create_compile_cmd(proj, cfg, src, obj);
    
    LOGFMT(
        PROJNAME,
        "build",
        CYAN_TEXT("[CC] "), src, "\n"
    )
    
    int ret = std::system(cmd.to_string().c_str());
    if (ret != 0) {
        LOGFMT(
            PROJNAME,
            "build",
            RED_TEXT("[ERROR]: "), "Compilation Failed: ", src, "\n"
        );

        YTHROW("Compilation Failed");
    }
}

} // namespace ymk::build
