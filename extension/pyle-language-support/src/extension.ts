import * as vscode from 'vscode';
import { exec } from 'child_process';

const PYLE_KEYWORDS = [
	"and", "or", "not", "for", "in", "if", "else", "let", "true", "false", "while",
	"fn", "break", "continue", "const"
];

const BUILTIN_FUNCTIONS = ["echo", "scan", "len", "importpy", "perf_counter", "get_attr"];

// Fallback static list
const STATIC_PYTHON_PACKAGES = ["os", "sys", "json", "math", "random", "re", "datetime", "collections", "itertools", "functools", "subprocess", "threading", "multiprocessing", "asyncio", "http", "urllib", "requests", "flask", "django", "numpy", "pandas", "matplotlib", "scipy", "sklearn", "pytest", "unittest", "logging", "argparse", "tkinter", "sqlite3", "email", "csv", "shutil", "glob", "pathlib", "typing", "pyle"];

let PYTHON_PACKAGES: string[] = STATIC_PYTHON_PACKAGES;

async function updatePythonPackages() {
	return new Promise<string[]>((resolve) => {
		// Try to use VS Code Python extension's interpreter, else fallback to 'python'
		let pythonPath = 'python';
		const pythonExt = vscode.extensions.getExtension('ms-python.python');
		if (pythonExt && pythonExt.isActive && pythonExt.exports && pythonExt.exports.settings) {
			const execCommand = pythonExt.exports.settings.getExecutionDetails().execCommand;
			if (execCommand && execCommand.length > 0) {
				pythonPath = execCommand[0];
			}
		}
		const script = 'import pkgutil, json; print(json.dumps(sorted([m.name for m in pkgutil.iter_modules()])))';
		exec(`${pythonPath} -c "${script}"`, { timeout: 10000 }, (err, stdout) => {
			if (err) {
				resolve(STATIC_PYTHON_PACKAGES);
				return;
			}
			try {
				const pkgs = JSON.parse(stdout);
				if (Array.isArray(pkgs) && pkgs.length > 0) {
					resolve(pkgs);
				} else {
					resolve(STATIC_PYTHON_PACKAGES);
				}
			} catch {
				resolve(STATIC_PYTHON_PACKAGES);
			}
		});
	});
}

export async function activate(context: vscode.ExtensionContext) {
	PYTHON_PACKAGES = (await updatePythonPackages()).sort();

	const completionProvider = vscode.languages.registerCompletionItemProvider(
		{ language: 'pyle', scheme: 'file' },
		{
			provideCompletionItems(document, position, token, context) {
				const completions: vscode.CompletionItem[] = [];

				const text = document.getText();
				const lines = text.split(/\r?\n/);

				// Context-aware: If inside importpy("") string, suggest Python packages
				const line = document.lineAt(position.line).text;
				const before = line.slice(0, position.character);
				const importpyMatch = /importpy\(\s*"([^"]*)$/.exec(before);
				if (importpyMatch) {
					return PYTHON_PACKAGES.map(pkg => {
						const item = new vscode.CompletionItem(pkg, vscode.CompletionItemKind.Module);
						item.insertText = pkg;
						item.detail = "Python package";
						return item;
					});
				}

				// Add function definitions: fn function_name(args...)
				const fnRegex = /fn\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\(([^)]*)\)/g;
				let match;
				while ((match = fnRegex.exec(text)) !== null) {
					const fnName = match[1];
					const argsRaw = match[2].trim();
					let argsList: string[] = [];
					if (argsRaw.length > 0) {
						argsList = argsRaw.split(',').map(arg => arg.trim()).filter(arg => arg.length > 0);
					}
					const argsDisplay = argsList.length > 0 ? argsList.join(', ') : 'none';

					// Find the line number of the function definition
					const fnDefIndex = text.slice(0, match.index).split(/\r?\n/).length - 1;
					let docComment = '';
					// Look for a multi-line comment immediately after the function definition line
					if (fnDefIndex + 1 < lines.length && lines[fnDefIndex + 1].trim().startsWith('/*')) {
						let commentLines = [];
						let i = fnDefIndex + 1;
						let inComment = false;
						while (i < lines.length) {
							const line = lines[i];
							if (!inComment && line.trim().startsWith('/*')) {
								inComment = true;
								commentLines.push(line.replace(/^\s*\/\*/, '').replace(/\*\/$/, '').trim());
							} else if (inComment) {
								if (line.includes('*/')) {
									commentLines.push(line.replace(/\*\//, '').trim());
									break;
								} else {
									commentLines.push(line.replace(/^\s*\*/, '').trim());
								}
							}
							i++;
						}
						docComment = commentLines.join('\n');
					}

					let docString = `\`\`\`pyle\nfn ${fnName}(${argsList.join(', ')})\n\`\`\``;
					if (docComment) {
						docString += `\n${docComment}`;
					}
					docString += `\n**Arguments:** ${argsDisplay}`;

					const item = new vscode.CompletionItem(fnName, vscode.CompletionItemKind.Function);
					item.documentation = new vscode.MarkdownString(docString);
					item.insertText = fnName;
					item.sortText = '1_' + fnName; // Highest priority
					completions.push(item);
				}

				// Add builtin funcitons
				completions.push(
					...BUILTIN_FUNCTIONS.map(keyword => {
						const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Function);
						item.insertText = keyword;
						item.sortText = '1_' + keyword; // Lower priority
						return item;
					})
				);

				// Add variable definitions: let variable_name =
				const varRegex = /let\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*=/g;
				while ((match = varRegex.exec(text)) !== null) {
					const varName = match[1];
					const item = new vscode.CompletionItem(varName, vscode.CompletionItemKind.Variable);
					item.detail = "User-defined variable";
					item.insertText = varName;
					item.sortText = '2_' + varName; // Next highest priority
					completions.push(item);
				}

				// Add keywords
				completions.push(
					...PYLE_KEYWORDS.map(keyword => {
						const item = new vscode.CompletionItem(keyword, vscode.CompletionItemKind.Keyword);
						item.insertText = keyword;
						item.sortText = '3_' + keyword; // Lower priority
						return item;
					})
				);

				return completions;
			}
		},
		...'abcdefghijklmnopqrstuvwxyz'
	);

	context.subscriptions.push(completionProvider);
}

export function deactivate() {}
