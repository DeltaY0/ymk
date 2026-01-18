#include <build/cache.h>

#include <filesystem>
#include <fstream>
#include <sstream>
#include <functional>

namespace fs = std::filesystem;

namespace ymk::build
{

// helper -> run command and get output string (for hashing)
static string run_and_read(const CompileCmd& cmd, const string& temp_file) {
    string full_cmd = cmd.to_string();
    
    // redirect stdout if toolchain didn't handle file output
    i32 ret = std::system(full_cmd.c_str());
    if (ret != 0) return "";

    // read the temp file
    std::ifstream file(temp_file, std::ios::binary);
    if (!file.is_open()) return "";
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Cache::load(const string &root) {
    cache_path = root + "/.ymake.cache";
    if (!fs::exists(cache_path)) return;

    std::ifstream in(cache_path);
    string path;
    size_t hash;
    time_t time;
    
    while (in >> path >> hash >> time) {
        registry[path] = {hash, time};
    }
}

void Cache::save() {
    std::ofstream out(cache_path);
    for (const auto& [path, entry] : registry) {
        out << path << " " << entry.hash << " " << entry.timestamp << "\n";
    }
}

bool Cache::needs_recompile(const Project& proj, const Config& config, const string &src) {
    // check if file exists in cache
    bool is_new = registry.find(src) == registry.end();
    
    // NOTE: add timestamp optimization later if building is too slow

    // ----- preprocessing (temporary file)
    string temp_i = "build/temp.i"; 
    fs::create_directories("build"); // just in case

    CompileCmd cmd = Toolchain::create_preprocess_cmd(proj, config, src, temp_i);
    
    string content = run_and_read(cmd, temp_i);
    if (content.empty()) return true;

    // hash content of file
    size_t current_hash = std::hash<std::string>{}(content);

    // compare
    if (is_new) {
        update(src, current_hash); // update cache
        return true;
    }

    if (registry[src].hash != current_hash) {
        update(src, current_hash);
        return true;
    }

    // if all checks fail, that means it's up to date
    return false;
}

void Cache::update(const string &src, size_t h) {

    // update the hash DSA, write to disk one time only
    registry[src].hash = h;
    registry[src].timestamp = 0; // NOTE: add real timestamp if we want to use that logic
}

} // namespace ymk::build