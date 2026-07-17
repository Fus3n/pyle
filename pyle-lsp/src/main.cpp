/**
 * Pyle Language Server Protocol (LSP) Daemon
 * Provides resilient, fuzzy-parsed IDE assistance for Pyle files (.pyl/.pyle).
 */

#include "lsp_server.hpp"

int main() {
    pyle::lsp::LspServer srv;
    srv.run();
    return 0;
}