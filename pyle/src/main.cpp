#include <exception>
#include <iostream>
#include <fmt/printf.h>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <argparse/argparse.hpp>
#include <string>
#include <pyle/config.hpp>

#include "pyle/pyle.hpp"
#include "pyle/std/std_core.hpp" 
#include "pyle/binder.hpp"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <limits.h>
#endif

pyle::Value register_json_module(pyle::VM& vm);


std::string read_file(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

void print_assertion_status() {
    #ifdef NDEBUG
        puts("Assertions disabled");
    #else
        puts("Assertions enabled");
    #endif
}


namespace fs = std::filesystem;

fs::path get_executable_directory() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    return fs::path(buffer).parent_path();
#elif __APPLE__
    // Mac implementation (or you can use std::filesystem::canonical)
    return fs::canonical("/proc/self/exe").parent_path();
#else
    // Linux implementation
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        return fs::path(buffer).parent_path();
    }
    return fs::current_path();
#endif
}

int main(int argc, char* argv[]) {
    print_assertion_status();

    argparse::ArgumentParser program("pyle");

    program.add_argument("-v", "--version")
        .help("Prints version information")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("script")
        .help("Path to .pyle script")
        .required()
        .default_value(std::string(""));
    program.add_argument("-d", "--dissassamble")
        .help("Prints dissassambled bytecode of the given script")
        .default_value(false)
        .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n";
        std::cerr << program;
        return 1;
    }
    if (program.get<bool>("--version")) {
        fmt::printf("Pyle version %s\n", PYLE_VERSION);
        return 0;
    }

    std::string script_path = program.get<std::string>("script");
    pyle::Pyle pyle;
    pyle::register_core_natives(pyle.vm); 
    pyle::register_module(pyle.vm, "json", register_json_module);

    fs::path exe_dir = get_executable_directory();
    fs::path std_path = exe_dir / "std";

    try {
        std::string source = read_file(script_path);
        pyle.execute(source, program.get<bool>("--dissassamble"), script_path);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    return 0;

}