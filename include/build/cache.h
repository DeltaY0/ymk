#pragma once

#include <defines.h>
#include <logger.h>

#include <core/typedefs.h>
#include <core/toolchain.h>

#include <unordered_map>
using std::unordered_map;

namespace ymk::build
{

struct FileCache {
    size_t hash;
    time_t timestamp;
};

class Cache {
private:
    string cache_path;
    unordered_map<string, FileCache> registry;

public:
    // load cache from disk (ymake.cache)
    void load(const string &workspace_root);

    // save cache to disk
    void save();

    // for incremental builds
    bool needs_recompile(
        const Project &proj,
        const Config  &conf,
        const string  &srcfile
    );

    // update cache entry
    void update(const string &srcfile, size_t new_hash);
};

} // namespace ymk::build