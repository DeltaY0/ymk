#include <defines.h>
#include <logger.h>

#include <core/toolchain.h>
#include <core/mt.h>

#include <build/cache.h>

namespace ymk::build
{

class Builder
{
private:
    Workspace &workspace;
    ThreadPool thread_pool;
    Cache cache;

    // builds a single project
    void build_project(Project &proj, const string &config_name);

    // helpers
    string get_obj_path(const Project &proj, const string &srcfile);

    void compile_file(
        const Project &proj,
        const Config  &conf,
        const string  &src,
        const string  &obj
    );

public:
    Builder(Workspace &ws);

    // main event
    void build(const string &config_name = "debug");
};

} // namespace ymk::build

