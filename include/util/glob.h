#pragma once

#include <defines.h>
#include <logger.h>
#include <error.h>

#include <filesystem>

namespace stdfs = std::filesystem;

namespace ymk::fs {

class glob
{
public:
    // interface
    static vector<string> resolve(const vector<string> &patterns);

private:
    static void match_pattern(const stdfs::path& pattern, vector<string> &results);
    static bool matches(const string &text, const string &pattern);

};

} // namespace ymk::fs