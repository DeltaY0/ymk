#pragma once

#include <defines.h>

#include <optional>
#include <unordered_map>

using std::optional;
using std::unordered_map;

namespace ymk {

enum class ArtifactType
{
    Exe,
    StaticLib,
    SharedLib
};

struct Config
{
    string compiler;

    optional<string> c_std;
    optional<string> cpp_std;

    optional<string> optimize;

    vector<string> defines;
    vector<string> flags;
    vector<string> includes;
    vector<string> links;

    inline void merge(Config &other) {
        // TODO: impl
        return;
    }
};

struct Project
{
    string name;
    ArtifactType type;
    string language;

    Config base_config;

    // key: debug, release, windows, linux, etc...
    // value: custom partial config to merge if flag is chosen
    unordered_map<string, Config> custom_configs;

    // input files
    vector<string> src_globs;

    // other ymk projects to link with
    vector<string> deps;

    // commands
    vector<string> pre_build_cmds;
    vector<string> post_build_cmds;
};

struct Task
{
    string name;
    vector<string> deps;  // can depend on other projects or tasks
    vector<string> cmds;
};

struct Workspace
{
    string name;

    string dist_dir = "bin";
    string obj_dir  = "build";

    Config global_base_config;
    unordered_map<string, Config> global_custom_configs;

    vector<Project> projects;
    vector<Task> tasks;
};

}  // namespace ymk