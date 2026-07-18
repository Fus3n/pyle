#include "lsp_server.hpp"
#include <filesystem>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    pyle::lsp::LspServer srv;

    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--std-path" && i + 1 < argc) {
            srv.document_manager().default_import_paths.push_back(argv[++i]);
        }
    }

    fs::path exe_dir = fs::path(argv[0]).parent_path();
    fs::path std_dir = exe_dir / "std";
    if (fs::exists(std_dir)) {
        srv.document_manager().default_import_paths.push_back(std_dir.string());
    }

    srv.run();
    return 0;
}
