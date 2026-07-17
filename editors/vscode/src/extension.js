const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient');
const path = require('path');

function activate(context) {
    // Path to your compiled C++ LSP binary
    const binPath = context.asAbsolutePath(path.join('bin', 'pyle-lsp.exe'));

    // Server options: how to start the LSP
    const serverOptions = {
        run: { command: binPath, transport: TransportKind.stdio },
        debug: { command: binPath, transport: TransportKind.stdio }
    };

    // Client options: when should the LSP activate, and what files does it handle
    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'pyle' }],
        synchronize: {
            // Notify the server if any .pyl files change in the workspace
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.pyl')
        }
    };

    // Create the language client
    const client = new LanguageClient(
        'pyleLSP',
        'Pyle Language Server',
        serverOptions,
        clientOptions
    );

    // Start the client. This will spawn the pyle-lsp.exe process automatically.
    context.subscriptions.push(client.start());
}

function deactivate() {
    // The LanguageClient handles killing the process automatically
}

exports.activate = activate;
exports.deactivate = deactivate;