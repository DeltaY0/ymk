# YMake (`ymk`)

**YMake** is a fast, simple, and highly readable build system for C and C++ projects.

Designed as a lightweight alternative to the verbosity of CMake and the complexity of traditional Makefiles, YMake utilizes a custom configuration language to handle multi-threaded compilation, dependency resolution, and cross-platform builds with absolute minimal boilerplate.

## ✨ Features

* **Custom Configuration Language:** Clean, declarative syntax with zero obscure macros.
* **Multi-Threaded Compilation:** Automatically dispatches independent compile tasks across available CPU cores.
* **Incremental Builds:** Smart caching tracks file hashes and timestamps, only recompiling exactly what changed.
* **Cross-Platform Scopes:** Define `win` and `linux` specific libraries and flags right next to each other.
* **Smart Globbing:** Easily resolve source files with wildcards (e.g., `src/**/*.cpp`).
* **Project Dependencies:** Seamlessly link nested projects and libraries using the `use:` directive.

## 🚀 Getting Started

You don't need another build system to build YMake. You can bootstrap the toolchain immediately using the provided shell scripts.

### 1. Build YMake from Source

Clone the repository and run the bootstrap script for your operating system. This will compile the source code and generate the `ymk` executable in the `bin/` directory.

**Windows:**
```cmd
.\build.bat
```

**Linux / macOS:**
```bash
./build.sh
```

*(Tip: Add the `bin/` folder to your system's `PATH` variable so you can run `ymk` from any directory!)*

### 2. CLI Usage

Once installed, YMake provides a streamlined CLI for project initialization and building:

```bash
# Generate a default build.ymk template in the current directory
ymk init

# Build the project (defaults to debug mode)
ymk build

# Build a specific configuration mode
ymk build -m release

# Display all available commands and arguments
ymk help
```

## 🛠️ Configuration Syntax (`build.ymk`)

YMake reads from `build.ymk`. The configuration allows you to define global workspace settings, custom compiler configurations, and individual project targets.

### Example: A Complete Graphics Application

```text
# Global Workspace Settings
workspace: OpenGLWorkspace
dist: bin
obj: build/obj

# Compiler Toolchain
compiler: clang++
c_std: c11
cpp_std: c++20

# Global Configurations
conf: debug {
    flags: [ -g, -O0 ]
    defines: [ YMAKE_DEBUG ]
}

conf: release {
    flags: [ -O3 ]
    defines: [ YMAKE_NDEBUG ]
}

# The Main Application Target
project: GraphicsApp {
    kind: exe
    language: cpp
    
    # Smart source globbing
    src: [ "src/**/*.cpp" ]
    
    # Include directories and external library paths
    inc: [ "src", "vendor/freeglut/include" ]
    libdirs: [ "vendor/freeglut/lib/x64" ]

    # OS-Specific Linking
    platform: win {
        links: [ freeglut, opengl32, glu32, user32, gdi32 ]
    }

    platform: linux {
        links: [ glut, GL, GLU, pthread ]
    }
    
    # Execute commands before the build starts
    on: prebuild {
        exec: "echo Compiling GraphicsApp..."
    }
}
```

## 🏗️ Internal Architecture

YMake is built entirely from scratch in C++ and is structured into several highly decoupled modules:

### 1. Lexer and Parser
*(Located in `src/parser/`)*

Instead of relying on JSON or YAML parsers, YMake implements a custom Lexer and Recursive Descent Parser. It tokenizes the `.ymk` configuration file and constructs an internal Abstract Syntax Tree (AST) representing the `Workspace`, `Projects`, and platform-specific configurations.

### 2. Toolchain Abstraction
*(Located in `src/core/toolchain.cpp`)*

The toolchain module abstracts away the underlying compiler (Clang, GCC, or MSVC). It dynamically intercepts user configurations and generates the exact shell arguments required for the current environment. For instance, it automatically translates linking commands between MSVC's `/LIBPATH` and GCC's `-L` and `-l` formats, completely shielding the user from compiler-specific quirks.

### 3. Multi-Threaded Builder
*(Located in `src/build/builder.cpp` & `src/core/mt.h`)*

To ensure maximum compilation speed, YMake utilizes a custom Thread Pool. The builder analyzes the project dependencies, queues up the independent `.cpp` files as asynchronous compile tasks, and dispatches them concurrently across all available CPU threads. The final linking phase is sequenced sequentially only after all threads successfully report idle.

### 4. Cache Management
*(Located in `src/build/cache.cpp`)*

YMake implements a stateful caching system to support fast, incremental builds. By hashing file contents and storing metadata, the builder intelligently skips recompilation for source files that haven't been modified since the last successful build, drastically reducing iteration times.

### 5. CLI Router
*(Located in `src/cli/`)*

The command-line interface uses a modular routing pattern to map incoming terminal flags and arguments directly to internal C++ callback functions, ensuring easy extensibility for future commands.

## 🚧 Current Limitations & TODOs

YMake is currently under active development. A few features are still being ironed out:

* **Running Artifacts:** The CLI currently only builds the project. A `ymk run` (or `build and run`) command is planned to automatically execute the compiled artifact.
* **OS-Specific Linking Automation:** Platform blocks (`win {}` vs `linux {}`) do not yet automatically resolve based on the host OS. However, this does allow for manual cross-compiling if specific arguments are passed.
* **C & C++ Interop:** Currently, you cannot use both `c_std` and `cpp_std` simultaneously without Clang throwing conflicting argument warnings. You must specify only the standard for the primary language you are compiling.

## 📜 License

*MIT License* - Created by Yassin Ibrahim (DeltaY).
