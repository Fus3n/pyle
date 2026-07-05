#pragma once
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <fstream>


#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

namespace fs = std::filesystem;

inline fs::path get_executable_directory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
#elif __APPLE__
    return fs::canonical("/proc/self/exe").parent_path();
#else
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return fs::path(buffer).parent_path();
    }
    return fs::current_path();
#endif
}


inline std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}


inline void print_assertion_status() {
    #ifdef NDEBUG
        puts("Assertions disabled");
    #else
        puts("Assertions enabled");
    #endif
}
