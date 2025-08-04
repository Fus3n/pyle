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
            --sidebar-width: 280px;
            --main-bg: #282c34;
            --sidebar-bg: #21252b;
            --text-color: #abb2bf;
            --border-color: #3e4451;
            --link-color: #61afef;
            --link-hover-color: #528bce;
            --fn-name-color: #7B97E5FF;
            --code-color: #e5c07b;
            --no-doc-color: #888;
            --header-color: #e6efff;
            --block-bg: #2c313a;
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
        .sidebar h2 {
            margin-top: 0;
            border-bottom: 1px solid var(--border-color);
            padding-bottom: 10px;
            color: var(--header-color);
        }
        .sidebar ul {
            list-style: none;
            padding: 0;
        }
        .sidebar li a {
            display: block;
            padding: 8px 10px;
            text-decoration: none;
            color: var(--link-color);
            border-radius: 4px;
            transition: background-color 0.2s;
        }
        .sidebar li a:hover {
            background-color: #3e4451;
            color: var(--link-hover-color);
        }
        .content {
            margin-left: var(--sidebar-width);
            padding: 20px 40px;
            width: calc(100% - var(--sidebar-width));
            box-sizing: border-box;
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
           margin-top: 40px;
        }
        .function-block {
            background-color: var(--block-bg);
            border: 1px solid var(--border-color);
            border-radius: 8px;
            padding: 1px 20px;
            margin-bottom: 20px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.1);
        }
        .function-name {
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
            font-size: 1.2em;
            color: var(--fn-name-color);
        }
        .description {
            margin-top: 10px;
            white-space: pre-wrap;
        }
        .no-doc {
            color: var(--no-doc-color);
            font-style: italic;
        }
        .params-title, .returns-title {
            font-weight: bold;
            margin-top: 15px;
            color: #c678dd;
        }
        .param {
            margin-left: 20px;
        }
        .param-name {
            font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, Courier, monospace;
            font-weight: bold;
        }
        .returns-info {
            margin-left: 20px;
            font-style: italic;
        }
    </style>
</head>
<body>
    <nav class="sidebar">
        {{range $section := .Sections}}
            <h2>{{$section.Title}}</h2>
            <ul>
                {{range $section.Funcs}}
                    <li><a href="#{{$section.Title}}-{{.Name}}">{{.Name}}</a></li>
                {{end}}
            </ul>
        {{end}}
    </nav>

    <main class="content">
        <h1>{{.Title}}</h1>

        {{range $section := .Sections}}
        <section id="section-{{$section.Title}}">
            <h2>{{$section.Title}}</h2>
            {{range $section.Funcs}}
                <div id="{{$section.Title}}-{{.Name}}" class="function-block">
                    <h3 class="function-name">{{.Signature}}</h3>
                     {{if .Doc}}
                        <div class="description">{{.Doc.Description}}</div>
                        {{if .Doc.Params}}
                            <div class="params-title">Parameters:</div>
                            {{range .Doc.Params}}
                                <div class="param"><span class="param-name">{{.Name}}:</span> {{.Description}}</div>
                            {{end}}
                        {{end}}
                        {{if .Doc.Returns}}
                            <div class="returns-title">Returns:</div>
                            <div class="returns-info">{{.Doc.Returns}}</div>
                        {{end}}
                    {{else}}
                        <p class="no-doc">No documentation available.</p>
                    {{end}}
                </div>
            {{end}}
        </section>
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
	Title string
	Funcs []FuncDoc
}

type FuncDoc struct {
	Name      string
	Signature string
	Doc       *pyle.DocstringObj
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
	// --- Process Global Built-ins ---
	var builtinFuncs []FuncDoc
	var builtinNames []string
	for name := range pyle.Builtins {
		builtinNames = append(builtinNames, name)
	}
	sort.Strings(builtinNames)
	for _, name := range builtinNames {
		doc, _ := pyle.BuiltinDocs[name]
		builtinFuncs = append(builtinFuncs, FuncDoc{Name: name, Signature: name, Doc: doc})
	}

	// --- Process Built-in Methods ---
	var methodSections []Section
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
			funcs = append(funcs, FuncDoc{Name: name, Signature: name, Doc: doc})
		}
		methodSections = append(methodSections, Section{Title: strings.Title(typeName) + " Methods", Funcs: funcs})
	}

	sections := []Section{{Title: "Global Functions", Funcs: builtinFuncs}}
	sections = append(sections, methodSections...)

	renderTemplate(TemplateData{
		Title:    "Pyle Built-in Documentation",
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
			if p.Type != nil {
				paramDocs = append(paramDocs, pyle.ParamDoc{
					Name:        p.Name.Value,
					Description: exprToString(p.Type), // Using the type as the description
				})
			}
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
			userFuncs = append(userFuncs, FuncDoc{Name: fn.Name.Value, Signature: signature, Doc: doc})
		} else {
			userFuncs = append(userFuncs, FuncDoc{Name: fn.Name.Value, Signature: signature, Doc: nil})
		}
		}
	}

	renderTemplate(TemplateData{
		Title:    fmt.Sprintf("Documentation for %s", inputFile),
		Sections: []Section{{Title: "Functions", Funcs: userFuncs}},
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