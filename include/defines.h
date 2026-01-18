#ifndef DEFINES_H
#define DEFINES_H

#include <corecrt.h>
#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <fstream>
#include <sstream>
#include <math.h>

using std::string;
using std::tuple;
using std::vector;

// project specific
#define PROJNAME "ymk"

#define VERSION_MAJOR 0
#define VERSION_MINOR 1
#define VERSION_PATCH 0

// unsigned int types
typedef uint8_t u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef size_t usize;

// signed int types
typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long long i64;

// floating point types
typedef float f32;
typedef double f64;

// for bit fields (accessing bit no. x)
#define BIT(x) (1 << x)

// define static assertions
#if defined(__clang__) || defined(__gcc__) || defined(__GNUC__)
    #define ST_ASSERT static_assert
#else
    #define ST_ASSERT static_assert
#endif

// static assertions for type sizes
static_assert(sizeof(u8) == 1, "expected u8 to be 1 byte.");
static_assert(sizeof(u16) == 2, "expected u16 to be 2 bytes.");
static_assert(sizeof(u32) == 4, "expected u32 to be 4 bytes.");
static_assert(sizeof(u64) == 8, "expected u64 to be 8 bytes.");

static_assert(sizeof(i8) == 1, "expected i8 to be 1 byte.");
static_assert(sizeof(i16) == 2, "expected i16 to be 2 bytes.");
static_assert(sizeof(i32) == 4, "expected i32 to be 4 bytes.");
static_assert(sizeof(i64) == 8, "expected i64 to be 8 bytes.");

static_assert(sizeof(f32) == 4, "expected f32 to be 4 bytes.");
static_assert(sizeof(f64) == 8, "expected f64 to be 8 bytes.");

// platform detection

// windows
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
    #define IPLATFORM_WINDOWS 1
    #ifndef _WIN64
        #error "64-bit is required on windows."
    #endif
// linux
#elif defined(__linux__) || defined(__gnu_linux__)
    #define IPLATFORM_LINUX 1
#elif defined(__unix__)
    #define IPLATFORM_UNIX 1
#elif defined(__POSIX__)
    #define IPLATFORM_POSIX 1
// catch any other unsupported OS
#else
    #error "platform is not supported."
#endif

#endif  // DEFINES_H