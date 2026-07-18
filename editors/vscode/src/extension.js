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

function activate(context) {
    const binPath = context.asAbsolutePath(path.join('bin', 'pyle-lsp.exe'));
    const outputChannel = vscode.window.createOutputChannel('Pyle LSP');
    outputChannel.show(true);

    function buildServerOptions() {
        const config = vscode.workspace.getConfiguration('pyle');
        let exePath = config.get('executablePath') || '';
        if (!exePath) exePath = findFirstPyle();

        const args = [];
        if (exePath) {
            const stdPath = findPyleStdPath(exePath);
            if (stdPath) args.push('--std-path', stdPath);
        } else {
            outputChannel.appendLine('[pyle] pyle.exe not found. Use Pyle: Select Interpreter Path to set it.');
        }

        return {
            run: { command: binPath, args, transport: TransportKind.stdio },
            debug: { command: binPath, args, transport: TransportKind.stdio }
        };
    }

    function startClient() {
        if (!fs.existsSync(binPath)) {
            const msg = `LSP binary not found at ${binPath}. Build and copy pyle-lsp.exe there, or update the extension path.`;
            outputChannel.appendLine('[pyle] ERROR: ' + msg);
            vscode.window.showErrorMessage(msg);
            return;
        }

        const config = vscode.workspace.getConfiguration('pyle');
        const exePath = config.get('executablePath') || findFirstPyle();
        if (!exePath) {
            outputChannel.appendLine('[pyle] pyle.exe not found in PATH and pyle.executablePath not set.');
            setTimeout(() => {
                vscode.window.showWarningMessage(
                    'Pyle interpreter not found. Use Pyle: Select Interpreter Path to locate pyle.exe.',
                    'Select Interpreter'
                ).then(choice => {
                    if (choice === 'Select Interpreter') vscode.commands.executeCommand('pyle.selectInterpreter');
                });
            }, 1000);
        }

        const serverOptions = buildServerOptions();
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
        await stopClient();
        startClient();
    });

    context.subscriptions.push(restartCmd, selectInterpreterCmd);
    startClient();
}

async function deactivate() {
    await stopClient();
}

exports.activate = activate;
exports.deactivate = deactivate;
