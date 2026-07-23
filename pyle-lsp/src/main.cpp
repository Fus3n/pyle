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
    for (const auto& candidate : {exe_dir / "std", exe_dir.parent_path() / "std"}) {
        if (fs::exists(candidate / "core")) {
            srv.document_manager().default_import_paths.push_back(candidate.string());
            break;
        }
    }

    srv.document_manager().preload_core_types(srv.document_manager().default_import_paths);

    srv.run();
    return 0;
}
