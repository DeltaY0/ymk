#pragma once

#include <string_view>
using std::string_view;

namespace ymk::keywords {

constexpr string_view BlockProject  = "project";
constexpr string_view BlockPlatform = "platform";
constexpr string_view BlockConf     = "conf";
constexpr string_view BlockTask     = "task";
constexpr string_view BlockOn       = "on";  // For "on: pre_build"
constexpr string_view Compiler      = "compiler";

// --- workspace ---
constexpr string_view Workspace     = "workspace";
constexpr string_view WorkspaceDist = "dist";
constexpr string_view WorkspaceObj  = "obj";

// --- Properties ---
constexpr string_view KeyStd    = "std";
constexpr string_view KeyCStd   = "c_std";
constexpr string_view KeyCppStd = "cpp_std";
constexpr string_view KeyOpt    = "optimize";

constexpr string_view KeyKind = "kind";
constexpr string_view KeyLang = "language";
constexpr string_view KeySrc  = "src";
constexpr string_view KeyInc  = "inc";

// Dependencies
constexpr string_view KeyLinks = "links";  // System Libs
constexpr string_view KeyUse   = "use";    // Project References
constexpr string_view KeyDeps  = "deps";   // Task Prerequisites

constexpr string_view KeyDefines = "defines";
constexpr string_view KeyFlags   = "flags";

// Commands
constexpr string_view KeyCmd = "exec";

// --- Values ---
constexpr string_view ValExe       = "exe";
constexpr string_view ValShared    = "shared";
constexpr string_view ValStatic    = "static";
constexpr string_view ValPreBuild  = "prebuild";
constexpr string_view ValPostBuild = "postbuild";

}  // namespace ymk::keywords