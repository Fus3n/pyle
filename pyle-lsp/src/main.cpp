#include "lsp_server.hpp"

#include <filesystem>
#include <iostream>
#include <vector>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "lsp_server.hpp"

#include <filesystem>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;
using namespace pyle::lsp;

static std::string exe_dir();

#ifdef _WIN32
static LONG WINAPI pyle_fatal_handler(PEXCEPTION_POINTERS info) {
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    if (code == 0xC0000005 || code == 0xC00000FD) {
        void* addr = info->ExceptionRecord->ExceptionAddress;
        fprintf(stderr, "[pyle-lsp] FATAL exception code=0x%08lX address=%p\n",
                (unsigned long)code, addr);
        HMODULE mod = nullptr;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&exe_dir, &mod);
        if (mod) {
            fprintf(stderr, "[pyle-lsp] module base=%p offset=0x%zux\n",
                    (void*)mod, (size_t)((char*)addr - (char*)mod));
        }
        fflush(stderr);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
#endif

static std::string exe_dir() {
#ifdef _WIN32
    char buf[MAX_PATH];
    DWORD n = GetModuleFileNameA(nullptr, buf, MAX_PATH);
    if (n == 0 || n >= MAX_PATH) return "";
    return fs::path(std::string(buf, n)).parent_path().string();
#else
    return fs::path("/proc/self/exe").parent_path().string();
#endif
}

static std::vector<std::string> detect_std_paths() {
    std::vector<std::string> out;
    std::string dir = exe_dir();
    for (const auto& candidate : {dir + "/std", dir + "/../std", fs::current_path().string() + "/std"}) {
        std::error_code ec;
        if (fs::exists(candidate, ec) && !ec) {
            out.push_back(fs::weakly_canonical(candidate).string());
        }
    }
    return out;
}

static int run_lsp(int argc, char* argv[]) {
    std::vector<std::string> std_paths;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--version" || arg == "-v") {
            std::cout << "pyle-lsp 1.0.0" << std::endl;
            return 0;
        }
        if (arg == "--help" || arg == "-h") {
            std::cout << "pyle-lsp 1.0.0 - language server for pyle\n"
                         "usage: pyle-lsp [--std-path <dir>]...\n"
                         "  --std-path  path to a std folder (repeatable; auto-detected from the exe location)\n";
            return 0;
        }
        if (arg == "--std-path" && i + 1 < argc) {
            std_paths.push_back(argv[++i]);
        }
    }

    if (std_paths.empty()) {
        if (const char* env = getenv("PYLE_STD_PATH")) {
            std::string paths(env);
            size_t start = 0;
            while (start < paths.size()) {
                size_t end = paths.find(';', start);
                if (end == std::string::npos) end = paths.size();
                std::string part = paths.substr(start, end - start);
                while (!part.empty() && (part.back() == ' ' || part.back() == '\t')) part.pop_back();
                size_t nb = part.find_first_not_of(" \t");
                if (nb != std::string::npos) {
                    part = part.substr(nb);
                    if (!part.empty()) std_paths.push_back(part);
                }
                start = end + 1;
            }
        }
    }
    if (std_paths.empty()) {
        for (const auto& d : detect_std_paths()) std_paths.push_back(d);
    }

    ModuleResolver resolver;
    resolver.set_std_paths(std_paths);
    resolver.preload_std();

    std::cerr << "[pyle-lsp] std paths: ";
    for (const auto& p : resolver.get_std_paths()) std::cerr << p << "; ";
    std::cerr << std::endl;

    LspServer server(resolver, std_paths);
    server.run();
    return 0;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    AddVectoredExceptionHandler(1, pyle_fatal_handler);
#endif
    return run_lsp(argc, argv);
}
