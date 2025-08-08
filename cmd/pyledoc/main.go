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
        }
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
        .params-title, .returns-title {
            font-weight: bold;
            margin-top: 15px;
            color: #c678dd;
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
    </style>
</head>
<body>
    <nav class="sidebar">
		<h1>{{.Title}}</h1>
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
        <section id="section-{{.AnchorID}}">
            <h2>{{.Title}}</h2>
			{{if .Description}}
				<p class="section-description">{{.Description}}</p>
			{{end}}
            {{range .Funcs}}
                <div id="{{.AnchorID}}" class="function-block">
                    <h3 class="function-name">{{.Signature}}</h3>
                     {{if .Doc}}
                        <div class="doc-details">
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
        </section>
		{{end}}
        {{end}}
    </main>
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