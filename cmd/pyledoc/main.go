package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"html/template"
	"os"
	"path/filepath"
	"regexp"
	"sort"
	"strings"
	"time"

	"github.com/Fus3n/pyle/pyle"
)



var TypeRegistry = make(map[string]string)

type SearchItem struct {
	Label    string `json:"l"` 
	Parent   string `json:"p"`
	Type     string `json:"t"`
	Link     string `json:"u"`
	Desc     string `json:"d"`
}

type SiteMeta struct {
	Title           string
	GeneratedAt     string
	Nav             []NavGroup
	SearchIndexJSON template.JS
}

type NavGroup struct {
	Title string
	Items []NavItem
}

type NavItem struct {
	Label    string
	Link     string
	IsActive bool
}

type PageData struct {
	Meta       SiteMeta
	Title      string
	PageTitle  string
	
	IsHome     bool
	IsCategory bool
	
	HomeStats  []HomeStat
	Category   Category
}

type HomeStat struct {
	Type  string
	Title string
	Desc  string
	Link  string
}

type Category struct {
	Name        string
	Description string
	Type        string
	Items       []DocItem
}

type DocItem struct {
	ID          string
	Name        string
	Signature   template.HTML
	Summary     string
	Description string
	Params      []ParamDetail
	Returns     template.HTML
}

type ParamDetail struct {
	Name string
	Desc template.HTML
}



func main() {
	outputDir := flag.String("o", "docs", "Output directory")
	flag.Parse()

	args := flag.Args()
	var meta SiteMeta
	var pages []PageData

	if err := os.MkdirAll(*outputDir, 0755); err != nil {
		panic(err)
	}

	buildTypeRegistry()

	if len(args) == 0 {
		meta, pages = prepareBuiltinPages()
	} else {
		meta, pages = prepareScriptPages(args[0])
	}


	t, err := template.New("pyle").Parse(htmlTemplate)
	if err != nil {
		panic(err)
	}

	for _, p := range pages {
		p.Meta = setActiveNav(meta, p.Title)

		filename := "index.html"
		if !p.IsHome {
			switch p.Category.Type {
			case "global":
				filename = "globals.html"
			case "module":
				filename = fmt.Sprintf("mod_%s.html", slugify(p.Category.Name))
			case "type":
				filename = fmt.Sprintf("type_%s.html", slugify(p.Category.Name))
			default:
				filename = fmt.Sprintf("script_%s.html", slugify(p.Category.Name))
			}
		}

		fullPath := filepath.Join(*outputDir, filename)
		f, err := os.Create(fullPath)
		if err != nil {
			fmt.Printf("Failed: %v\n", err)
			continue
		}
		
		if err := t.Execute(f, p); err != nil {
			fmt.Printf("Failed render: %v\n", err)
		}
		f.Close()
		fmt.Printf("Generated: %s\n", fullPath)
	}
}

func buildTypeRegistry() {
	TypeRegistry = make(map[string]string)
	for typeName := range pyle.BuiltinMethods {
		TypeRegistry[typeName] = fmt.Sprintf("type_%s.html", slugify(typeName))
	}
}

func linkify(text string) template.HTML {
	if text == "" { return "" }
	re := regexp.MustCompile(`\b[a-zA-Z_][a-zA-Z0-9_]*\b`)
	processed := re.ReplaceAllStringFunc(template.HTMLEscapeString(text), func(word string) string {
		if link, ok := TypeRegistry[word]; ok {
			return fmt.Sprintf(`<a href="%s" class="type-link">%s</a>`, link, word)
		}
		return word
	})
	return template.HTML(processed)
}

func titleCase(s string) string {
	if len(s) == 0 {
		return ""
	}
	return strings.ToUpper(s[:1]) + s[1:]
}

func prepareBuiltinPages() (SiteMeta, []PageData) {
	meta := SiteMeta{
		Title:       "Standard Library",
		GeneratedAt: time.Now().Format("Jan 02, 2006"),
	}

	var pages []PageData
	var navModules, navTypes []NavItem
	var stats []HomeStat
	var searchItems []SearchItem


	var globalKeys []string
	for k := range pyle.BuiltinFunctions { globalKeys = append(globalKeys, k) }
	sort.Strings(globalKeys)

	if len(globalKeys) > 0 {
		cat := buildCategory("Global Functions", "global", "Core built-in functions.", globalKeys, pyle.BuiltinDocs)
		pages = append(pages, PageData{Title: "Global Functions", PageTitle: "Global Functions", IsCategory: true, Category: cat})
		meta.Nav = append(meta.Nav, NavGroup{Title: "Core", Items: []NavItem{{Label: "Global Functions", Link: "globals.html"}}})
		
		for _, fname := range globalKeys {
			d, _ := pyle.BuiltinDocs[fname]
			summary := ""
			if d != nil { summary = extractSummary(d.Description) }
			searchItems = append(searchItems, SearchItem{
				Label: fname, Parent: "Global", Type: "func", Link: "globals.html#" + slugify(fname), Desc: summary,
			})
		}
	}


	var modKeys []string
	// Use BuiltinMethodDocs for discovery to include pylegame/http even if not linked
	for name := range pyle.BuiltinModuleDocs {
		modKeys = append(modKeys, name)
	}
	sort.Strings(modKeys)

	for _, name := range modKeys {
		desc := ""
		if d, ok := pyle.BuiltinModuleDocs[name]; ok {
			desc = d.Description
		}

		var funcKeys []string
		if docMap, ok := pyle.BuiltinMethodDocs[name]; ok {
			for k := range docMap {
				funcKeys = append(funcKeys, k)
			}
		}
		sort.Strings(funcKeys)
		
		cat := buildCategory(name, "module", desc, funcKeys, pyle.BuiltinMethodDocs[name])
		link := fmt.Sprintf("mod_%s.html", slugify(name))
		
		pages = append(pages, PageData{Title: name, PageTitle: "Module: " + name, IsCategory: true, Category: cat})
		navModules = append(navModules, NavItem{Label: name, Link: link})
		stats = append(stats, HomeStat{Type: "Module", Title: name, Desc: desc, Link: link})

		searchItems = append(searchItems, SearchItem{Label: name, Type: "mod", Link: link, Desc: desc})
		for _, fname := range funcKeys {
			d, _ := pyle.BuiltinMethodDocs[name][fname]
			summary := ""
			if d != nil { summary = extractSummary(d.Description) }
			searchItems = append(searchItems, SearchItem{
				Label: fname, Parent: name, Type: "func", Link: link + "#" + slugify(fname), Desc: summary,
			})
		}
	}

	var typeKeys []string
	for k := range pyle.BuiltinMethods { typeKeys = append(typeKeys, k) }
	sort.Strings(typeKeys)

	for _, name := range typeKeys {
		desc := fmt.Sprintf("Methods for the %s type.", name)
		
		typeMethods := pyle.BuiltinMethods[name]
		var methodKeys []string
		for k := range typeMethods { methodKeys = append(methodKeys, k) }
		sort.Strings(methodKeys)

		cat := buildCategory(name, "type", desc, methodKeys, pyle.BuiltinMethodDocs[name])
		
		displayName := titleCase(name)
		cat.Name = displayName
		link := fmt.Sprintf("type_%s.html", slugify(name))

		pages = append(pages, PageData{
			Title:      displayName,
			PageTitle:  "Type: " + displayName,
			IsCategory: true,
			Category:   cat,
		})
		navTypes = append(navTypes, NavItem{Label: displayName, Link: link})
		stats = append(stats, HomeStat{Type: "Type", Title: displayName, Desc: desc, Link: link})
		
		searchItems = append(searchItems, SearchItem{Label: name, Type: "type", Link: link, Desc: desc})
		for _, mname := range methodKeys {
			d, _ := pyle.BuiltinMethodDocs[name][mname]
			summary := ""
			if d != nil { summary = extractSummary(d.Description) }
			searchItems = append(searchItems, SearchItem{
				Label: mname, Parent: name, Type: "func", Link: link + "#" + slugify(mname), Desc: summary,
			})
		}
	}

	if len(navModules) > 0 { meta.Nav = append(meta.Nav, NavGroup{Title: "Modules", Items: navModules}) }
	if len(navTypes) > 0 { meta.Nav = append(meta.Nav, NavGroup{Title: "Types", Items: navTypes}) }

	jsonBytes, _ := json.Marshal(searchItems)
	meta.SearchIndexJSON = template.JS(jsonBytes)

	homePage := PageData{Meta: meta, Title: "Home", IsHome: true, HomeStats: stats}
	return meta, append([]PageData{homePage}, pages...)
}

func exprToString(e pyle.Expr) string {
	if e == nil { return "" }
	if ve, ok := e.(*pyle.VariableExpr); ok {
		return ve.Name.Value
	}
	return fmt.Sprintf("%v", e) 
}

func prepareScriptPages(inputFile string) (SiteMeta, []PageData) {

	absPath, _ := filepath.Abs(inputFile)
	source, err := os.ReadFile(absPath)
	if err != nil {
		fmt.Printf("Error reading file: %v\n", err)
		os.Exit(1)
	}


	l := pyle.NewLexer(inputFile, string(source))
	tokens, _ := l.Tokenize()
	p := pyle.NewParser(tokens)
	result := p.Parse()
	ast := result.Value


	var funcNames []string

	scriptDocs := make(map[string]*pyle.DocstringObj)

	for _, stmt := range ast.Statements {
		if fn, ok := stmt.(*pyle.FunctionDefStmt); ok {
			funcNames = append(funcNames, fn.Name.Value)

			doc := &pyle.DocstringObj{}
			
			if len(fn.Body.Statements) > 0 {
				if strExpr, ok := fn.Body.Statements[0].(*pyle.StringExpr); ok {
					doc.Description = strings.TrimSpace(strExpr.Value)
				}
			}


			for _, param := range fn.Params {
				pType := "any"
				if param.Type != nil {
					pType = exprToString(param.Type)
				}
				doc.Params = append(doc.Params, pyle.ParamDoc{
					Name:        param.Name.Value,
					Description: pType,
				})
			}


			if fn.ReturnType != nil {
				doc.Returns = exprToString(fn.ReturnType)
			}

			scriptDocs[fn.Name.Value] = doc
		}
	}
	sort.Strings(funcNames)

	scriptName := filepath.Base(inputFile)
	cat := buildCategory(scriptName, "script", "User script documentation.", funcNames, scriptDocs)
	
	page := PageData{
		Title:      scriptName,
		PageTitle:  "Script: " + scriptName,
		IsHome:     true,
		IsCategory: true,
		Category:   cat,
	}


	meta := SiteMeta{
		Title:       "Script Docs",
		GeneratedAt: time.Now().Format("Jan 02, 2006"),
	}
	meta.Nav = append(meta.Nav, NavGroup{
		Title: "Scripts",
		Items: []NavItem{{Label: scriptName, Link: "index.html", IsActive: true}},
	})


	var searchItems []SearchItem
	for _, fname := range funcNames {
		d := scriptDocs[fname]
		summary := ""
		if d != nil { summary = extractSummary(d.Description) }
		searchItems = append(searchItems, SearchItem{
			Label: fname, Parent: scriptName, Type: "func", Link: "index.html#" + slugify(fname), Desc: summary,
		})
	}
	jsonBytes, _ := json.Marshal(searchItems)
	meta.SearchIndexJSON = template.JS(jsonBytes)

	page.Meta = meta

	return meta, []PageData{page}
}


func setActiveNav(meta SiteMeta, currentTitle string) SiteMeta {
	newNav := make([]NavGroup, len(meta.Nav))
	for i, g := range meta.Nav {
		newGroup := NavGroup{Title: g.Title, Items: make([]NavItem, len(g.Items))}
		for j, item := range g.Items {
			item.IsActive = (item.Label == currentTitle)
			newGroup.Items[j] = item
		}
		newNav[i] = newGroup
	}
	meta.Nav = newNav
	return meta
}

func buildCategory(name, catType, desc string, funcNames []string, docs map[string]*pyle.DocstringObj) Category {
	cat := Category{Name: name, Type: catType, Description: desc}
	for _, fname := range funcNames {
		d, _ := docs[fname]
		parent := name
		if catType == "global" { parent = "" }
		if catType == "module" { parent = "module" }
		cat.Items = append(cat.Items, buildDocItem(fname, parent, d))
	}
	return cat
}

func buildDocItem(name string, parentCtx string, doc *pyle.DocstringObj) DocItem {
	item := DocItem{ Name: name, ID: slugify(name) }
	if doc != nil {
		item.Description = doc.Description
		item.Returns = linkify(doc.Returns)
		item.Summary = extractSummary(doc.Description)
		for _, p := range doc.Params {
			item.Params = append(item.Params, ParamDetail{Name: p.Name, Desc: linkify(p.Description)})
		}
	}
	retStr := ""
	if doc != nil { retStr = doc.Returns }
	item.Signature = buildSignature(name, parentCtx, item.Params, retStr)
	return item
}

func extractSummary(desc string) string {
	if desc == "" { return "" }
	if idx := strings.Index(desc, "."); idx != -1 { return desc[:idx+1] }
	return desc
}

func buildSignature(name, parent string, params []ParamDetail, ret string) template.HTML {
	var sb strings.Builder
	sb.WriteString(`<span class="kwd">fn</span> `)
	if parent != "" && parent != "module" {
		sb.WriteString(fmt.Sprintf(`<span class="type">%s</span>.<span class="fn">%s</span>`, parent, name))
	} else {
		sb.WriteString(fmt.Sprintf(`<span class="fn">%s</span>`, name))
	}
	sb.WriteString(`<span class="punct">(</span>`)
	for i, p := range params {
		if i > 0 { sb.WriteString(`<span class="punct">, </span>`) }
		sb.WriteString(fmt.Sprintf(`<span class="arg">%s</span>`, p.Name))
	}
	sb.WriteString(`<span class="punct">)</span>`)
	if ret != "" {
		sb.WriteString(` <span class="punct">-></span> <span class="ret">` + string(linkify(ret)) + `</span>`)
	}
	return template.HTML(sb.String())
}

func slugify(s string) string {
	return strings.ToLower(strings.ReplaceAll(strings.TrimSpace(s), " ", "-"))
}

const htmlTemplate = `<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>{{if .Title}}{{.Title}} - {{end}}{{.Meta.Title}}</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;600;700&family=JetBrains+Mono:wght@400;500&display=swap" rel="stylesheet">
    <style>
        :root {
            --bg-main: #0f1115; --bg-sidebar: #16181d; --bg-card: #1c1f26;
            --bg-input: #252830; --border: #2f333d; --border-focus: #7c3aed;
            --text: #ededed; --text-sec: #949ba5; --text-muted: #5f6570;
            --accent: #7c3aed; --accent-glow: rgba(124, 58, 237, 0.15);
            
            /* Syntax */
            --s-kwd: #c678dd; --s-fn: #61afef; --s-type: #e5c07b; 
            --s-arg: #d19a66; --s-ret: #56b6c2; --s-punct: #abb2bf;
        }
        * { box-sizing: border-box; }
        body { margin: 0; font-family: 'Inter', sans-serif; background: var(--bg-main); color: var(--text); display: flex; height: 100vh; overflow: hidden; }
        a { text-decoration: none; color: inherit; }
        a.type-link { cursor: pointer; color: inherit; border-bottom: 1px dotted currentColor; transition: 0.2s; }
        a.type-link:hover { border-bottom-style: solid; }

        /* Sidebar */
        aside { width: 280px; background: var(--bg-sidebar); border-right: 1px solid var(--border); display: flex; flex-direction: column; flex-shrink: 0; position: relative; z-index: 5; }
        .brand { padding: 20px; border-bottom: 1px solid var(--border); }
        .brand h1 { margin: 0; font-size: 1.1rem; font-weight: 600; }
        .brand a { color: #fff; }
        
        .search-container { padding: 15px; position: relative; }
        #search { width: 100%; background: var(--bg-input); border: 1px solid var(--border); color: #fff; padding: 8px 12px; border-radius: 6px; outline: none; font-family: 'Inter'; }
        #search:focus { border-color: var(--border-focus); }

        /* Advanced Search Results Dropdown */
        .search-results {
            position: absolute; top: 100%; left: 10px; right: 10px;
            background: var(--bg-card); border: 1px solid var(--border); border-radius: 6px;
            max-height: 400px; overflow-y: auto;
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            display: none; /* Hidden by default */
        }
        .search-results.visible { display: block; }
        .sr-item { padding: 10px 12px; border-bottom: 1px solid var(--border); cursor: pointer; transition: background 0.1s; }
        .sr-item:last-child { border-bottom: none; }
        .sr-item:hover, .sr-item.selected { background: var(--bg-input); }
        .sr-head { display: flex; justify-content: space-between; align-items: baseline; margin-bottom: 3px; }
        .sr-name { font-family: 'JetBrains Mono', monospace; font-weight: 600; color: var(--text); font-size: 0.9rem; }
        .sr-parent { font-size: 0.75rem; color: var(--text-muted); margin-left: 8px; }
        .sr-type { font-size: 0.65rem; text-transform: uppercase; background: var(--border); padding: 2px 5px; border-radius: 3px; color: var(--text-sec); }
        .sr-desc { font-size: 0.8rem; color: var(--text-sec); white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
        .sr-empty { padding: 15px; text-align: center; color: var(--text-muted); font-size: 0.9rem; }

        /* Navigation */
        .nav-scroll { flex: 1; overflow-y: auto; padding: 10px 0; }
        .nav-header { font-size: 0.75rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.05em; margin: 25px 20px 8px 20px; }
        .nav-item { display: block; padding: 6px 20px; color: var(--text-sec); font-size: 0.9rem; border-left: 2px solid transparent; }
        .nav-item:hover { color: #fff; background: rgba(255,255,255,0.03); }
        .nav-item.active { color: #fff; border-left-color: var(--accent); background: linear-gradient(90deg, var(--accent-glow), transparent); }

        main { flex: 1; overflow-y: auto; padding: 0; }
        .container { max-width: 900px; margin: 0 auto; padding: 40px 60px; }

        .home-grid { display: grid; grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); gap: 20px; margin-top: 30px; }
        .home-card { background: var(--bg-card); border: 1px solid var(--border); border-radius: 8px; padding: 20px; transition: transform 0.2s; }
        .home-card:hover { transform: translateY(-3px); border-color: var(--accent); }
        .hc-badge { font-size: 0.65rem; text-transform: uppercase; font-weight: 700; color: var(--accent); background: var(--bg-input); padding: 2px 6px; border-radius: 4px; }
        .hc-title { display: block; font-size: 1.2rem; font-weight: 600; margin: 10px 0 5px 0; }
        .hc-desc { font-size: 0.9rem; color: var(--text-sec); line-height: 1.5; }

        .page-header { border-bottom: 1px solid var(--border); padding-bottom: 20px; margin-bottom: 40px; }
        .page-title { font-size: 2.5rem; margin: 0 0 10px 0; font-weight: 700; }
        .page-desc { font-size: 1.1rem; color: var(--text-sec); line-height: 1.6; }

        .doc-card { background: var(--bg-card); border: 1px solid var(--border); border-radius: 8px; margin-bottom: 30px; }
        .card-head { padding: 12px 20px; background: rgba(0,0,0,0.2); border-bottom: 1px solid var(--border); font-family: 'JetBrains Mono', monospace; font-size: 0.9rem; display: flex; justify-content: space-between; }
        .card-body { padding: 20px; }
        .desc-text { margin-bottom: 20px; line-height: 1.6; color: #e0e0e0; }
        
        .param-tbl { width: 100%; font-size: 0.9rem; border-collapse: collapse; }
        .param-tbl td { padding: 5px 0; vertical-align: top; }
        .p-name { width: 120px; font-family: 'JetBrains Mono', monospace; color: var(--s-arg); }
        .p-desc { color: var(--text-sec); }
        
        .lbl { font-size: 0.75rem; font-weight: 700; color: var(--text-muted); text-transform: uppercase; margin: 15px 0 8px 0; display: block; }
        .ret-type { font-family: 'JetBrains Mono', monospace; color: var(--s-ret); }
        
        .idx-tbl { width: 100%; border-collapse: collapse; margin-bottom: 30px; border: 1px solid var(--border); border-radius: 8px; overflow: hidden; font-size: 0.9rem; }
        .idx-tbl th { text-align: left; background: var(--bg-input); padding: 10px 15px; color: var(--text-muted); border-bottom: 1px solid var(--border); }
        .idx-tbl td { padding: 10px 15px; border-bottom: 1px solid var(--border); color: var(--text-sec); }
        .idx-tbl tr:last-child td { border-bottom: none; }
        .idx-link { font-family: 'JetBrains Mono', monospace; color: var(--s-fn); }
        .idx-link:hover { text-decoration: underline; }

        .kwd { color: var(--s-kwd); } .fn { color: var(--s-fn); font-weight: 600; }
        .type { color: var(--s-type); } .arg { color: var(--s-arg); }
        .ret { color: var(--s-ret); } .punct { color: var(--s-punct); }
    </style>
</head>
<body>
    <aside>
        <div class="brand"><h1><a href="index.html">{{.Meta.Title}}</a></h1></div>
        <div class="search-container">
            <input type="text" id="search" placeholder="Search (e.g. split)...">
            <div id="searchResults" class="search-results"></div>
        </div>
        <nav class="nav-scroll">
            <a href="index.html" class="nav-item {{if .IsHome}}active{{end}}">Home</a>
            {{range .Meta.Nav}}
                <div class="nav-header">{{.Title}}</div>
                {{range .Items}}
                    <a href="{{.Link}}" class="nav-item {{if .IsActive}}active{{end}}">{{.Label}}</a>
                {{end}}
            {{end}}
        </nav>
    </aside>

    <main>
        <div class="container">
            {{if .IsHome}}
                <div class="page-header">
                    <h1 class="page-title">{{.Meta.Title}}</h1>
                    <p class="page-desc">Documentation generated on {{.Meta.GeneratedAt}}</p>
                </div>
                <div class="home-grid">
                    {{range .HomeStats}}
                    <a href="{{.Link}}" class="home-card">
                        <span class="hc-badge">{{.Type}}</span>
                        <span class="hc-title">{{.Title}}</span>
                        <span class="hc-desc">{{if .Desc}}{{.Desc}}{{else}}Reference documentation.{{end}}</span>
                    </a>
                    {{end}}
                </div>
            {{else}}
                <div class="page-header">
                    <h1 class="page-title">{{.Category.Name}}</h1>
                    {{if .Category.Description}}<p class="page-desc">{{.Category.Description}}</p>{{end}}
                </div>

                <table class="idx-tbl">
                    <thead><tr><th>Name</th><th>Summary</th></tr></thead>
                    <tbody>
                        {{range .Category.Items}}
                        <tr>
                            <td><a href="#{{.ID}}" class="idx-link">{{.Name}}</a></td>
                            <td>{{.Summary}}</td>
                        </tr>
                        {{end}}
                    </tbody>
                </table>

                {{range .Category.Items}}
                <div id="{{.ID}}" class="doc-card">
                    <div class="card-head">
                        <div class="sig">{{.Signature}}</div>
                    </div>
                    <div class="card-body">
                        <div class="desc-text">{{.Description}}</div>
                        {{if .Params}}
                            <span class="lbl">Parameters</span>
                            <table class="param-tbl">
                                {{range .Params}}<tr><td class="p-name">{{.Name}}</td><td class="p-desc">{{.Desc}}</td></tr>{{end}}
                            </table>
                        {{end}}
                        {{if .Returns}}
                            <span class="lbl">Returns</span>
                            <div class="ret-type">{{.Returns}}</div>
                        {{end}}
                    </div>
                </div>
                {{end}}
            {{end}}
        </div>
    </main>

    <script>

        const searchIndex = {{.Meta.SearchIndexJSON}};
        
        const searchInput = document.getElementById('search');
        const resultsBox = document.getElementById('searchResults');
        

        function scoreItem(item, term) {
            const name = item.l.toLowerCase();
            const parent = (item.p || '').toLowerCase();
            
            if (name === term) return 100;
            if (name.startsWith(term)) return 50;
            if (name.includes(term)) return 10;
            if (parent.includes(term)) return 5;
            return 0;
        }

        function performSearch() {
            const term = searchInput.value.toLowerCase().trim();
            if (term.length < 1) {
                resultsBox.classList.remove('visible');
                return;
            }


            const matches = searchIndex
                .map(item => ({ item, score: scoreItem(item, term) }))
                .filter(res => res.score > 0)
                .sort((a, b) => b.score - a.score)
                .slice(0, 10);

            renderResults(matches);
        }

        function renderResults(matches) {
            resultsBox.innerHTML = '';
            if (matches.length === 0) {
                resultsBox.innerHTML = '<div class="sr-empty">No results found</div>';
            } else {
                matches.forEach(m => {
                    const item = m.item;
                    const div = document.createElement('div');
                    div.className = 'sr-item';
                    div.onclick = () => { window.location.href = item.u; };
                    

                    const parentHtml = item.p ? '<span class="sr-parent">in ' + item.p + '</span>' : '';
                    
                    div.innerHTML = 
                        '<div class="sr-head">' + 
                            '<div><span class="sr-name">' + item.l + '</span>' + parentHtml + '</div>' +
                            '<span class="sr-type">' + item.t + '</span>' +
                        '</div>' +
                        '<div class="sr-desc">' + (item.d || 'No description') + '</div>';
                    
                    resultsBox.appendChild(div);
                });
            }
            resultsBox.classList.add('visible');
        }


        let debounce;
        searchInput.addEventListener('input', () => {
            clearTimeout(debounce);
            debounce = setTimeout(performSearch, 100);
        });


        document.addEventListener('click', (e) => {
            if (!searchInput.contains(e.target) && !resultsBox.contains(e.target)) {
                resultsBox.classList.remove('visible');
            }
        });
        

        searchInput.addEventListener('focus', () => {
            if (searchInput.value.trim().length > 0) performSearch();
        });

    </script>
</body>
</html>
`