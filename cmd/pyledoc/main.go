package main

import (
	"flag"
	"fmt"
	"html/template"
	"os"
	"path/filepath"
	"sort"
	"strings"

	"pylevm/pyle"
)

const htmlTemplateStr = `
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{.Title}}</title>
    <style>
        :root {
            --sidebar-width: 300px;
            --main-bg: #1e1e1e;
            --sidebar-bg: #252526;
            --text-color: #d4d4d4;
            --border-color: #3e3e42;
            --link-color: #4e94ce;
            --link-hover-color: #74b4e8;
            --fn-name-color: #dcdcaa;
            --type-color: #4ec9b0;
            --header-color: #d4d4d4;
            --block-bg: #2d2d30;
			--pre-bg: #1a1a1a;
            --muted: #9aa0a6;
            --accent: #c678dd;
        }
        html {
            scroll-behavior: smooth;
            scroll-padding-top: 20px;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
            line-height: 1.6;
            color: var(--text-color);
            margin: 0;
            background-color: var(--main-bg);
            display: flex;
        }
        .sidebar {
            width: var(--sidebar-width);
            position: fixed;
            top: 0;
            left: 0;
            height: 100vh;
            background-color: var(--sidebar-bg);
            border-right: 1px solid var(--border-color);
            padding: 20px;
            overflow-y: auto;
            box-sizing: border-box;
        }
        .sidebar h1 {
			font-size: 1.5em;
			margin: 0 0 20px 0;
			padding-bottom: 10px;
			border-bottom: 1px solid var(--border-color);
		}
        .sidebar .search {
            position: sticky;
            top: 0;
            padding-bottom: 12px;
            background: var(--sidebar-bg);
        }
        .sidebar input[type="text"] {
            width: 100%;
            box-sizing: border-box;
            background: #1f1f1f;
            border: 1px solid var(--border-color);
            border-radius: 6px;
            color: var(--text-color);
            padding: 8px 10px;
            outline: none;
        }
        .sidebar .controls {
            display: flex;
            gap: 8px;
            margin-top: 8px;
        }
        .sidebar .controls button {
            flex: 1 0 auto;
            background: #333;
            color: var(--text-color);
            border: 1px solid var(--border-color);
            border-radius: 6px;
            padding: 6px 8px;
            cursor: pointer;
        }
        .sidebar .controls button:hover {
            background: #3a3a3a;
        }
        .sidebar h2 {
            margin-top: 20px;
            font-size: 1.1em;
            color: var(--header-color);
			border-bottom: none;
			padding-bottom: 5px;
        }
        .sidebar ul {
            list-style: none;
            padding: 0 0 0 10px;
        }
        .sidebar li a {
            display: block;
            padding: 6px 10px;
            text-decoration: none;
            color: var(--link-color);
            border-radius: 4px;
            transition: background-color 0.2s;
			font-size: 0.9em;
        }
        .sidebar li a:hover {
            background-color: #3e3e42;
            color: var(--link-hover-color);
        }
        .sidebar li a.active {
            background-color: #3a3f44;
            color: #fff;
        }
        .content {
            margin-left: var(--sidebar-width);
            padding: 20px 40px;
            width: calc(100% - var(--sidebar-width));
            box-sizing: border-box;
			max-width: 900px;
        }
        h1, h2, h3 {
            color: var(--header-color);
        }
        h1 {
           border-bottom: 2px solid var(--border-color);
           padding-bottom: 10px;
        }
        h2 {
           border-bottom: 1px solid var(--border-color);
           padding-bottom: 10px;
           margin-top: 50px;
        }
		.section-description {
			margin-top: -10px;
			margin-bottom: 30px;
			font-style: italic;
			color: #a0a0a0;
		}
        .section-header {
            display: flex;
            align-items: center;
            justify-content: space-between;
            gap: 10px;
        }
        .section-toggle {
            background: #333;
            color: var(--text-color);
            border: 1px solid var(--border-color);
            border-radius: 6px;
            padding: 4px 8px;
            cursor: pointer;
            font-size: 0.85em;
        }
        .section.collapsed .section-body { display: none; }
        .function-block {
            background-color: var(--block-bg);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 1px 20px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
			overflow: hidden;
        }
        .function-name {
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
            font-size: 1.1em;
            color: var(--fn-name-color);
			background-color: var(--pre-bg);
			padding: 10px 20px;
			margin: 0 -20px;
			border-bottom: 1px solid var(--border-color);
			white-space: pre-wrap;
			word-break: break-all;
            display: flex;
            align-items: center;
            justify-content: space-between;
            cursor: pointer;
        }
        .fn-toggle-indicator { color: var(--muted); font-size: 0.9em; }
        .description {
            margin-top: 15px;
            white-space: pre-wrap;
        }
        .no-doc {
            color: #888;
            font-style: italic;
			padding: 15px 0;
        }
		.doc-details {
			padding-bottom: 15px;
		}
        .function-block.collapsed .doc-details { display: none; }
        .params-title, .returns-title {
            font-weight: bold;
            margin-top: 15px;
            color: var(--accent);
        }
        .param {
            margin-left: 20px;
			line-height: 1.4;
        }
        .param-name {
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
            font-weight: bold;
			color: var(--type-color);
        }
        .returns-info {
            margin-left: 20px;
        }
		.returns-info span {
			font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
			color: var(--type-color);
		}
        #backToTop {
            position: fixed;
            right: 24px;
            bottom: 24px;
            width: 44px;
            height: 44px;
            border-radius: 50%;
            border: 1px solid var(--border-color);
            background: #333;
            color: var(--text-color);
            cursor: pointer;
            display: none;
        }
        #backToTop.show { display: inline-flex; align-items: center; justify-content: center; }
    </style>
</head>
<body>
    <nav class="sidebar">
		<h1>{{.Title}}</h1>
        <div class="search">
            <input type="text" id="searchBox" placeholder="Search functions, methods, modules..." aria-label="Search">
            <div class="controls">
                <button id="expandAll" title="Expand all">Expand All</button>
                <button id="collapseAll" title="Collapse all">Collapse All</button>
            </div>
        </div>
        {{range .Sections}}
			{{if .Funcs}}
				<h2><a href="#section-{{.AnchorID}}">{{.Title}}</a></h2>
				<ul>
					{{range .Funcs}}
						<li><a href="#{{.AnchorID}}">{{.Name}}</a></li>
					{{end}}
				</ul>
			{{end}}
        {{end}}
    </nav>

    <main class="content">
        <h1>{{.Title}} Documentation</h1>

        {{range .Sections}}
        {{if .Funcs}}
        <section id="section-{{.AnchorID}}" class="section">
            <div class="section-header">
                <h2 style="margin: 0;">{{.Title}}</h2>
                <button class="section-toggle" data-target="section-{{.AnchorID}}" aria-expanded="true">Collapse</button>
            </div>
			{{if .Description}}
				<p class="section-description">{{.Description}}</p>
			{{end}}
            <div class="section-body">
            {{ $section := . }}
            {{range .Funcs}}
                <div id="{{.AnchorID}}" class="function-block" data-name="{{.Name}}" data-signature="{{.Signature}}" data-section="section-{{$section.AnchorID}}" {{if .Doc}}data-description="{{.Doc.Description}}"{{end}}>
                    <h3 class="function-name" role="button" aria-controls="{{.AnchorID}}-content" aria-expanded="false">
                        <span>{{.Signature}}</span>
                        <span class="fn-toggle-indicator">▼</span>
                    </h3>
                     {{if .Doc}}
                        <div id="{{.AnchorID}}-content" class="doc-details">
							<div class="description">{{.Doc.Description}}</div>
							{{if .Doc.Params}}
								<div class="params-title">Parameters:</div>
								{{range .Doc.Params}}
									<div class="param"><span class="param-name">{{.Name}}:</span> {{.Description}}</div>
								{{end}}
							{{end}}
							{{if .Doc.Returns}}
								<div class="returns-title">Returns:</div>
								<div class="returns-info"><span>{{.Doc.Returns}}</span></div>
							{{end}}
						</div>
                    {{else}}
                        <p class="no-doc">No documentation available.</p>
                    {{end}}
                </div>
            {{end}}
            </div>
        </section>
        {{end}}
        {{end}}
    </main>
    <button id="backToTop" title="Back to top" aria-label="Back to top">↑</button>
    <script>
    (function() {
        const q = (s, r=document) => r.querySelector(s);
        const qa = (s, r=document) => Array.from(r.querySelectorAll(s));

        // Collapsible sections
        qa('.section').forEach((section, idx) => {
            const toggleBtn = q('.section-toggle', section);
            const body = q('.section-body', section);
            const collapsedInitially = idx > 0; // collapse all but first section
            if (collapsedInitially) {
                section.classList.add('collapsed');
                toggleBtn.setAttribute('aria-expanded', 'false');
                toggleBtn.textContent = 'Expand';
            }
            toggleBtn?.addEventListener('click', () => {
                const isCollapsed = section.classList.toggle('collapsed');
                toggleBtn.setAttribute('aria-expanded', String(!isCollapsed));
                toggleBtn.textContent = isCollapsed ? 'Expand' : 'Collapse';
            });
        });

        // Collapsible function blocks
        qa('.function-block').forEach(block => {
            const header = q('.function-name', block);
            const details = q('.doc-details', block);
            block.classList.add('collapsed');
            header?.setAttribute('aria-expanded', 'false');
            header?.addEventListener('click', () => {
                const isCollapsed = block.classList.toggle('collapsed');
                header.setAttribute('aria-expanded', String(!isCollapsed));
                const indicator = q('.fn-toggle-indicator', header);
                if (indicator) indicator.textContent = isCollapsed ? '▼' : '▲';
            });
        });

        // Expand/Collapse All
        q('#expandAll')?.addEventListener('click', () => {
            qa('.section').forEach(s => s.classList.remove('collapsed'));
            qa('.section .section-toggle').forEach(b => { b.textContent = 'Collapse'; b.setAttribute('aria-expanded','true');});
            qa('.function-block').forEach(b => b.classList.remove('collapsed'));
            qa('.function-name').forEach(h => h.setAttribute('aria-expanded','true'));
            qa('.fn-toggle-indicator').forEach(i => i.textContent = '▲');
        });
        q('#collapseAll')?.addEventListener('click', () => {
            qa('.section').forEach((s, idx) => { if (idx>0) s.classList.add('collapsed'); });
            qa('.section .section-toggle').forEach((b, idx) => { b.textContent = (idx>0)?'Expand':'Collapse'; b.setAttribute('aria-expanded', (idx===0)?'true':'false');});
            qa('.function-block').forEach(b => b.classList.add('collapsed'));
            qa('.function-name').forEach(h => h.setAttribute('aria-expanded','false'));
            qa('.fn-toggle-indicator').forEach(i => i.textContent = '▼');
        });

        // Search/filter
        const searchBox = q('#searchBox');
        const filter = () => {
            const term = (searchBox?.value || '').toLowerCase().trim();
            const hasTerm = term.length > 0;
            const sectionHasVisible = new Map();

            qa('.function-block').forEach(block => {
                const name = (block.getAttribute('data-name')||'').toLowerCase();
                const sig = (block.getAttribute('data-signature')||'').toLowerCase();
                const desc = (block.getAttribute('data-description')||'').toLowerCase();
                const visible = !hasTerm || name.includes(term) || sig.includes(term) || desc.includes(term);
                block.style.display = visible ? '' : 'none';
                const sectionId = block.getAttribute('data-section');
                if (visible && sectionId) sectionHasVisible.set(sectionId, true);
                if (visible && hasTerm) {
                    // Expand matched blocks and their sections for visibility
                    block.classList.remove('collapsed');
                    const header = q('.function-name', block);
                    header?.setAttribute('aria-expanded','true');
                    const ind = q('.fn-toggle-indicator', header); if (ind) ind.textContent='▲';
                }
            });

            qa('.section').forEach(section => {
                const id = section.id;
                const anyVisible = !!sectionHasVisible.get(id) || !hasTerm; // keep open when no term
                const items = qa('.function-block', section).filter(el => el.style.display !== 'none');
                section.style.display = (items.length > 0) ? '' : 'none';
                // auto-expand sections when searching
                if (hasTerm) section.classList.remove('collapsed');
                const btn = q('.section-toggle', section);
                if (btn) { btn.textContent = (section.classList.contains('collapsed') ? 'Expand' : 'Collapse'); btn.setAttribute('aria-expanded', String(!section.classList.contains('collapsed')));}            
            });

            // Sidebar filtering
            qa('.sidebar ul').forEach(list => {
                qa('li', list).forEach(li => {
                    const link = q('a', li);
                    const txt = (link?.textContent||'').toLowerCase();
                    const show = !hasTerm || txt.includes(term);
                    li.style.display = show ? '' : 'none';
                });
            });
        };
        searchBox?.addEventListener('input', () => {
            // debounce lightly
            window.clearTimeout(searchBox._t);
            searchBox._t = window.setTimeout(filter, 50);
        });

        // Active section highlight
        const sectionLinks = new Map(qa('.sidebar h2 a').map(a => [a.getAttribute('href').slice(1), a]));
        const observer = new IntersectionObserver(entries => {
            entries.forEach(entry => {
                if (entry.isIntersecting) {
                    const id = entry.target.id;
                    qa('.sidebar li a').forEach(a => a.classList.remove('active'));
                    const secLink = sectionLinks.get(id);
                    if (secLink) secLink.classList.add('active');
                }
            });
        }, { rootMargin: '0px 0px -70% 0px', threshold: 0.1 });
        qa('section.section').forEach(sec => observer.observe(sec));

        // Ensure clicking sidebar function links expands target
        qa('.sidebar li a').forEach(a => {
            a.addEventListener('click', e => {
                const id = a.getAttribute('href').slice(1);
                const el = document.getElementById(id);
                if (!el) return;
                // Expand parent section
                const section = el.closest('.section');
                if (section) {
                    section.classList.remove('collapsed');
                    const btn = q('.section-toggle', section);
                    if (btn) { btn.textContent = 'Collapse'; btn.setAttribute('aria-expanded','true');}
                }
                // Expand function block
                el.classList.remove('collapsed');
                const header = q('.function-name', el);
                header?.setAttribute('aria-expanded','true');
                const ind = q('.fn-toggle-indicator', header); if (ind) ind.textContent='▲';
            });
        });

        // Back to top
        const back = q('#backToTop');
        const onScroll = () => {
            if (window.scrollY > 400) back.classList.add('show'); else back.classList.remove('show');
        };
        window.addEventListener('scroll', onScroll, { passive: true });
        back?.addEventListener('click', () => window.scrollTo({ top: 0, behavior: 'smooth' }));

    })();
    </script>
</body>
</html>
`

// --- Data Structures for Template ---

type TemplateData struct {
	Title    string
	Sections []Section
}

type Section struct {
	Title       string
	Description string // For modules or types
	AnchorID    string
	Funcs       []FuncDoc
}

type FuncDoc struct {
	Name      string
	Signature string
	AnchorID  string
	Doc       *pyle.DocstringObj
}

func slugify(s string) string {
	s = strings.ToLower(s)
	s = strings.ReplaceAll(s, " ", "-")
	return s
}

func exprToString(e pyle.Expr) string {
	if e == nil {
		return ""
	}
	if ve, ok := e.(*pyle.VariableExpr); ok {
		return ve.Name.Value
	}
	// Fallback for other potential type expressions
	return e.TypeString()
}

// --- Main Application Logic ---

func main() {
	outputFlag := flag.String("o", "", "Output HTML file name")
	flag.Parse()

	args := flag.Args()
	if len(args) > 1 {
		fmt.Fprintln(os.Stderr, "Usage: pyledoc [pyle_script.pyle] [-o output.html]")
		os.Exit(1)
	}

	if len(args) == 0 {
		// Mode 1: Generate docs for built-ins
		outputFile := "pyle_docs.html"
		if *outputFlag != "" {
			outputFile = *outputFlag
		}
		generateBuiltinDocs(outputFile)
	} else {
		// Mode 2: Generate docs for a user script
		inputFile := args[0]
		outputFile := *outputFlag
		if outputFile == "" {
			outputFile = strings.TrimSuffix(inputFile, filepath.Ext(inputFile)) + ".html"
		}
		generateUserDocs(inputFile, outputFile)
	}
}

// --- Doc Generation Functions ---

func generateBuiltinDocs(outputFile string) {
	var sections []Section

	// --- Process Global Built-ins ---
	var builtinFuncs []FuncDoc
	builtinNames := make([]string, 0, len(pyle.Builtins))
	for name := range pyle.Builtins {
		builtinNames = append(builtinNames, name)
	}
	sort.Strings(builtinNames)

	for _, name := range builtinNames {
		doc, _ := pyle.BuiltinDocs[name]
		anchor := "global-" + slugify(name)
		builtinFuncs = append(builtinFuncs, FuncDoc{Name: name, Signature: name, Doc: doc, AnchorID: anchor})
	}
	if len(builtinFuncs) > 0 {
		sections = append(sections, Section{Title: "Global Functions", Funcs: builtinFuncs, AnchorID: "globals"})
	}

	// --- Process Built-in Modules ---
	moduleNames := make([]string, 0, len(pyle.BuiltinModules))
	for name := range pyle.BuiltinModules {
		moduleNames = append(moduleNames, name)
	}
	sort.Strings(moduleNames)

	for _, moduleName := range moduleNames {
		module := pyle.BuiltinModules[moduleName]
		moduleDoc, _ := pyle.BuiltinModuleDocs[moduleName]
		funcDocs, _ := pyle.BuiltinMethodDocs[moduleName]
		if funcDocs == nil {
			funcDocs = make(map[string]*pyle.DocstringObj)
		}

		var funcs []FuncDoc
		funcNames := make([]string, 0, len(module))
		for name := range module {
			funcNames = append(funcNames, name)
		}
		sort.Strings(funcNames)
		for _, name := range funcNames {
			doc, _ := funcDocs[name]
			anchor := slugify(moduleName) + "-" + slugify(name)
			funcs = append(funcs, FuncDoc{Name: name, Signature: name, Doc: doc, AnchorID: anchor})
		}
		desc := ""
		if moduleDoc != nil {
			desc = moduleDoc.Description
		}
		sections = append(sections, Section{Title: "Module: " + moduleName, Description: desc, Funcs: funcs, AnchorID: "module-" + slugify(moduleName)})
	}

	// --- Process Built-in Methods ---
	typeNames := make([]string, 0, len(pyle.BuiltinMethods))
	for typeName := range pyle.BuiltinMethods {
		typeNames = append(typeNames, typeName)
	}
	sort.Strings(typeNames)

	for _, typeName := range typeNames {
		methodMap := pyle.BuiltinMethods[typeName]
		docMap, _ := pyle.BuiltinMethodDocs[typeName]
		if docMap == nil {
			docMap = make(map[string]*pyle.DocstringObj)
		}

		var funcs []FuncDoc
		funcNames := make([]string, 0, len(methodMap))
		for name := range methodMap {
			funcNames = append(funcNames, name)
		}
		sort.Strings(funcNames)
		for _, name := range funcNames {
			doc, _ := docMap[name]
			anchor := slugify(typeName) + "-" + slugify(name)
			funcs = append(funcs, FuncDoc{Name: name, Signature: name, Doc: doc, AnchorID: anchor})
		}
		sections = append(sections, Section{Title: "Type: " + strings.Title(typeName), Funcs: funcs, AnchorID: "type-" + slugify(typeName)})
	}

	renderTemplate(TemplateData{
		Title:    "Pyle Built-ins",
		Sections: sections,
	}, outputFile)
}

func generateUserDocs(inputFile, outputFile string) {
	absPath, err := filepath.Abs(inputFile)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error creating absolute path for %s: %v\n", inputFile, err)
		os.Exit(1)
	}

	source, err := os.ReadFile(absPath)
	if err != nil {
		fmt.Fprintf(os.Stderr, "Error reading file %s: %v\n", absPath, err)
		os.Exit(1)
	}

	l := pyle.NewLexer(inputFile, string(source))
	tokens, lexErr := l.Tokenize()
	if lexErr.IsErr() {
		fmt.Fprintf(os.Stderr, "Lexer error in %s: %v\n", inputFile, lexErr.Err)
		os.Exit(1)
	}

	p := pyle.NewParser(tokens)
	result := p.Parse()
	if result.IsErr() {
		fmt.Fprintf(os.Stderr, "Parser error in %s: %v\n", inputFile, result.Err)
		os.Exit(1)
	}

	ast := result.Value

	var userFuncs []FuncDoc
	for _, stmt := range ast.Statements {
		if fn, ok := stmt.(*pyle.FunctionDefStmt); ok {
			// Signature generation
			var paramParts []string
			for _, p := range fn.Params {
				part := p.Name.Value
				if p.Type != nil {
					part += ": " + exprToString(p.Type)
				}
				paramParts = append(paramParts, part)
			}
			signature := fmt.Sprintf("%s(%s)", fn.Name.Value, strings.Join(paramParts, ", "))
			if fn.ReturnType != nil {
				signature += " -> " + exprToString(fn.ReturnType)
			}
			anchor := "fn-" + slugify(fn.Name.Value)

			// Doc object generation
			doc := &pyle.DocstringObj{}
			hasDocContent := false

			// Try to get description from docstring
			bodyStmts := fn.Body.Statements
			if len(bodyStmts) > 0 {
				if strExpr, ok := bodyStmts[0].(*pyle.StringExpr); ok {
					doc.Description = strings.TrimSpace(strExpr.Value)
					hasDocContent = true
				}
			}

			// Populate params and returns from type hints
			var paramDocs []pyle.ParamDoc
			for _, p := range fn.Params {
				paramDoc := pyle.ParamDoc{Name: p.Name.Value}
				if p.Type != nil {
					paramDoc.Description = exprToString(p.Type) // Using the type as the description
				}
				paramDocs = append(paramDocs, paramDoc)
			}
			if len(paramDocs) > 0 {
				doc.Params = paramDocs
				hasDocContent = true
			}

			if fn.ReturnType != nil {
				doc.Returns = exprToString(fn.ReturnType)
				hasDocContent = true
			}

			if hasDocContent {
				userFuncs = append(userFuncs, FuncDoc{Name: fn.Name.Value, Signature: signature, Doc: doc, AnchorID: anchor})
			} else {
				userFuncs = append(userFuncs, FuncDoc{Name: fn.Name.Value, Signature: signature, Doc: nil, AnchorID: anchor})
			}
		}
	}

	renderTemplate(TemplateData{
		Title:    fmt.Sprintf("Script: %s", filepath.Base(inputFile)),
		Sections: []Section{{Title: "Script Functions", Funcs: userFuncs, AnchorID: "script-functions"}},
	}, outputFile)
}

func renderTemplate(data TemplateData, outputFile string) {
	tmpl, err := template.New("docs").Parse(htmlTemplateStr)
	if err != nil {
		fmt.Println("Error parsing template:", err)
		os.Exit(1)
	}

	f, err := os.Create(outputFile)
	if err != nil {
		fmt.Println("Error creating file:", err)
		os.Exit(1)
	}
	defer f.Close()

	if err := tmpl.Execute(f, data); err != nil {
		fmt.Println("Error executing template:", err)
		os.Exit(1)
	}

	fmt.Printf("Successfully generated documentation to %s\n", outputFile)
}
