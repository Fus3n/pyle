const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient');
const path = require('path');

function activate(context) {
    const binPath = context.asAbsolutePath(path.join('bin', 'pyle-lsp.exe'));

    const outputChannel = vscode.window.createOutputChannel('Pyle LSP');
    outputChannel.show(true);

    const serverOptions = {
        run: { command: binPath, transport: TransportKind.stdio },
        debug: { command: binPath, transport: TransportKind.stdio }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'pyle' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.pyl')
        },
        outputChannel: outputChannel
    };

    const client = new LanguageClient(
        'pyleLSP',
        'Pyle Language Server',
        serverOptions,
        clientOptions
    );

    context.subscriptions.push(outputChannel, client.start());
}

function deactivate() {
}

exports.activate = activate;
exports.deactivate = deactivate;