const vscode = require('vscode');
const { LanguageClient, TransportKind } = require('vscode-languageclient');
const path = require('path');
const fs = require('fs');
const { execSync } = require('child_process');

function findAllPyleInPath() {
    try {
        const isWin = process.platform === 'win32';
        const cmd = isWin ? 'where pyle.exe' : 'which -a pyle';
        const result = execSync(cmd, { encoding: 'utf8', timeout: 3000 }).trim();
        return result.split('\n').map(l => l.trim()).filter(l => l.length > 0);
    } catch { return []; }
}

function findFirstPyle() {
    const list = findAllPyleInPath();
    return list.length > 0 ? list[0] : null;
}

function findPyleStdPath(pyleExePath) {
    const exeDir = path.dirname(pyleExePath);
    const stdDir = path.join(exeDir, 'std');
    if (fs.existsSync(stdDir)) return stdDir;
    return null;
}

function lspExecutableName() {
    return process.platform === 'win32' ? 'pyle-lsp.exe' : 'pyle-lsp';
}

let client = null;
let clientDisposable = null;

async function stopClient() {
    if (client) {
        try { await client.stop(); } catch { }
        client = null;
    }
    if (clientDisposable) {
        try { clientDisposable.dispose(); } catch { }
        clientDisposable = null;
    }
}

function findLspBinary(context, interpreterPath) {
    const config = vscode.workspace.getConfiguration('pyle');
    const explicit = config.get('lspPath') || '';
    if (explicit && fs.existsSync(explicit)) return explicit;

    const exe = lspExecutableName();
    if (interpreterPath) {
        const dir = path.dirname(interpreterPath);
        const sibling = path.join(dir, exe);
        if (fs.existsSync(sibling)) return sibling;
        const subfolder = path.join(dir, 'pyle-lsp', exe);
        if (fs.existsSync(subfolder)) return subfolder;
    }

    return null;
}

function findInterpreter() {
    const config = vscode.workspace.getConfiguration('pyle');
    const exePath = config.get('executablePath') || '';
    return exePath || findFirstPyle();
}

function activate(context) {
    const outputChannel = vscode.window.createOutputChannel('Pyle LSP');

    function startClient() {
        if (client) return;

        const interpreterPath = findInterpreter();
        const lspBinary = findLspBinary(context, interpreterPath);

        if (!lspBinary) {
            outputChannel.appendLine('[pyle] pyle-lsp not found; language server not started.');
            vscode.window.showWarningMessage(
                'Pyle language server (pyle-lsp) not found. Set its location to enable Pyle language features.',
                'Select LSP Binary'
            ).then(choice => {
                if (choice === 'Select LSP Binary') vscode.commands.executeCommand('pyle.selectLsp');
            });
            return;
        }

        if (!interpreterPath) {
            outputChannel.appendLine('[pyle] pyle.exe not found; running without std-path.');
            vscode.window.showWarningMessage(
                'Pyle interpreter not found. Set its location for std library support.',
                'Select Interpreter'
            ).then(choice => {
                if (choice === 'Select Interpreter') vscode.commands.executeCommand('pyle.selectInterpreter');
            });
        }

        outputChannel.appendLine(`[pyle] interpreter: ${interpreterPath || '(none)'}`);
        outputChannel.appendLine(`[pyle] lsp binary:  ${lspBinary}`);

        const args = [];
        if (interpreterPath) {
            const stdPath = findPyleStdPath(interpreterPath);
            if (stdPath) args.push('--std-path', stdPath);
            outputChannel.appendLine(`[pyle] std:        ${stdPath || '(none)'}`);
        }

        const serverOptions = { run: { command: lspBinary, args, transport: TransportKind.stdio } };
        const clientOptions = {
            documentSelector: [{ scheme: 'file', language: 'pyle' }],
            synchronize: {
                fileEvents: [
                    vscode.workspace.createFileSystemWatcher('**/*.pyl'),
                    vscode.workspace.createFileSystemWatcher('**/*.pyl.d'),
                    vscode.workspace.createFileSystemWatcher('**/*.pyle')
                ]
            },
            outputChannel: outputChannel
        };

        client = new LanguageClient('pyleLSP', 'Pyle Language Server', serverOptions, clientOptions);
        clientDisposable = client.start();
        context.subscriptions.push(clientDisposable);
    }

    const restartCmd = vscode.commands.registerCommand('pyle.restartLSP', async () => {
        await stopClient();
        startClient();
    });

    const selectInterpreterCmd = vscode.commands.registerCommand('pyle.selectInterpreter', async () => {
        const config = vscode.workspace.getConfiguration('pyle');
        const current = config.get('executablePath') || '';
        const detected = findAllPyleInPath();

        const items = [];
        for (const p of detected) {
            const isCurrent = p === current;
            items.push({
                label: (isCurrent ? '$(check) ' : '') + p,
                description: isCurrent ? '(currently set)' : '',
                detail: findPyleStdPath(p) ? `std: ${findPyleStdPath(p)}` : 'no std folder',
                path: p
            });
        }
        if (current && !detected.includes(current)) {
            items.push({
                label: '$(check) ' + current,
                description: '(currently set)',
                detail: findPyleStdPath(current) ? `std: ${findPyleStdPath(current)}` : 'no std folder',
                path: current
            });
        }
        if (detected.length === 0 && !current) {
            items.unshift({
                label: '$(warning) No pyle.exe found in PATH',
                description: '',
                detail: 'Use Browse to locate it manually, or add pyle.exe to PATH'
            });
        }
        items.push({
            label: '$(file-directory) Browse for pyle.exe...',
            description: '',
            detail: 'Pick a custom location',
            path: ''
        });

        const pick = await vscode.window.showQuickPick(items, {
            title: 'Select Pyle Interpreter',
            placeHolder: current || 'Pick a pyle.exe installation'
        });
        if (!pick) return;

        let selected;
        if (pick.label.includes('Browse')) {
            const opts = {
                canSelectFiles: true, canSelectFolders: false, canSelectMany: false,
                filters: { Executables: ['exe', 'cmd', 'bat'], All: ['*'] },
                title: 'Select pyle.exe interpreter'
            };
            const result = await vscode.window.showOpenDialog(opts);
            if (!result || result.length === 0) return;
            selected = result[0].fsPath;
        } else {
            selected = pick.path;
        }

        await config.update('executablePath', selected, vscode.ConfigurationTarget.Global);
        vscode.window.showInformationMessage(`Pyle interpreter set to: ${selected}`);
    });

    const selectLspCmd = vscode.commands.registerCommand('pyle.selectLsp', async () => {
        const config = vscode.workspace.getConfiguration('pyle');
        const current = config.get('lspPath') || '';
        const interpreterPath = findInterpreter();

        const items = [];
        const candidates = [];
        if (current && fs.existsSync(current)) candidates.push(current);
        if (interpreterPath) {
            const dir = path.dirname(interpreterPath);
            candidates.push(path.join(dir, lspExecutableName()));
            candidates.push(path.join(dir, 'pyle-lsp', lspExecutableName()));
        }

        const seen = new Set();
        for (const c of candidates) {
            if (!c || seen.has(c) || !fs.existsSync(c)) continue;
            seen.add(c);
            items.push({
                label: (c === current ? '$(check) ' : '') + c,
                description: c === current ? '(currently set)' : '',
                path: c
            });
        }
        items.push({
            label: '$(file-directory) Browse for pyle-lsp.exe...',
            description: '',
            detail: 'Pick a custom location',
            path: ''
        });

        const pick = await vscode.window.showQuickPick(items, {
            title: 'Select Pyle LSP Server Binary',
            placeHolder: current || 'Pick a pyle-lsp.exe'
        });
        if (!pick) return;

        let selected;
        if (pick.label.includes('Browse')) {
            const opts = {
                canSelectFiles: true, canSelectFolders: false, canSelectMany: false,
                filters: { Executables: ['exe'], All: ['*'] },
                title: 'Select pyle-lsp.exe'
            };
            const result = await vscode.window.showOpenDialog(opts);
            if (!result || result.length === 0) return;
            selected = result[0].fsPath;
        } else {
            selected = pick.path;
        }

        await config.update('lspPath', selected, vscode.ConfigurationTarget.Global);
        vscode.window.showInformationMessage(`Pyle LSP binary set to: ${selected}`);
    });

    const configWatcher = vscode.workspace.onDidChangeConfiguration(e => {
        if (!e.affectsConfiguration('pyle.executablePath') && !e.affectsConfiguration('pyle.lspPath')) return;
        stopClient().then(() => startClient());
    });

    context.subscriptions.push(restartCmd, selectInterpreterCmd, selectLspCmd, configWatcher);
    startClient();
}

async function deactivate() {
    await stopClient();
}

exports.activate = activate;
exports.deactivate = deactivate;
