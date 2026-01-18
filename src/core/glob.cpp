#include <core/glob.h>

#include <algorithm>

namespace ymk::fs {

vector<string> glob::resolve(const vector<string>& patterns) {
    vector<string> results;

    for (const auto& pat_str : patterns) {
        stdfs::path pattern(pat_str);
        match_pattern(pattern, results);
    }

    // sort and remove duplicates
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());

    return results;
}

void glob::match_pattern(const stdfs::path& pattern_path, vector<string>& results) {
    string full_pattern = pattern_path.string();

    // 1. Find the "Root" (The part before any wildcards)

    // find the root of dir tree
    //    ex: "src/renderer/**/*.cpp" -> root: "src/renderer"
    stdfs::path root;
    stdfs::path remaining;
    
    auto it = pattern_path.begin();
    stdfs::path current_builder;
    
    // iterate path comps until we hit a wildcard
    bool wildcard_found = false;
    while(it != pattern_path.end()) {
        string part = it->string();
        if (part.find('*') != std::string::npos) {
            wildcard_found = true;
            break; 
        }
        current_builder /= *it;
        ++it;
    }

    if (!wildcard_found) {
        // if no wildcard, check existence directly
        if (stdfs::exists(current_builder) && stdfs::is_regular_file(current_builder)) {
            results.push_back(stdfs::absolute(current_builder).string());
        }
        return;
    }

    root = current_builder;
    
    // if the root doesn't exist (e.g. "src" folder missing), just return
    if (root.empty()) root = "."; // handle case where pattern starts with wildcard
    if (!stdfs::exists(root)) return;

    // determine filename pattern (ex: "*.cpp")
    string filename_pattern = pattern_path.filename().string();

    // recursive search
    try {
        for (const auto& entry : stdfs::recursive_directory_iterator(root)) {
            if (entry.is_regular_file()) {
                if (matches(entry.path().filename().string(), filename_pattern)) {
                    results.push_back(stdfs::absolute(entry.path()).string());
                }
            }
        }
    } catch (const stdfs::filesystem_error& e) {
        LOGFMT(
            PROJNAME,
            "file/globber",
            RED_TEXT("GLOB error:\t"),
            e.what(), "\n"
        );
    }
}

bool glob::matches(const string& text, const string& pattern) {
    // exact match
    if (text == pattern) return true;

    // wildcard handler (*)
    if (pattern.size() > 1 && pattern[0] == '*') {
        std::string suffix = pattern.substr(1);
        if (text.length() >= suffix.length()) {
            return (0 == text.compare(text.length() - suffix.length(), suffix.length(), suffix));
        }
    }
    return false;
}

} // namespace ymk::fs