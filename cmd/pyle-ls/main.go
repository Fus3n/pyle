package main

import (
	"fmt"
	"log"
	"sync" // Import the sync package for the mutex

	"pylevm/pyle" // Import your existing pyle language package

	"github.com/tliron/commonlog"
	"github.com/tliron/glsp"
	protocol "github.com/tliron/glsp/protocol_3_16"
	"github.com/tliron/glsp/server"

	_ "github.com/tliron/commonlog/simple"
)

const (
	lsName = "pyle-ls"
	// Using protocol constants directly is safer
	CIKFunction = protocol.CompletionItemKindFunction
	CIKVariable = protocol.CompletionItemKindVariable
	CIKKeyword  = protocol.CompletionItemKindKeyword
)

var (
	version   string = "0.0.1"
	handler   protocol.Handler

	// Added mutex for concurrent access safety
	documentsMutex sync.RWMutex
	documents      = make(map[string]string)
)

func main() {
	commonlog.Configure(1, nil)

	handler = protocol.Handler{
		Initialize:             initialize,
		Initialized:            initialized,
		Shutdown:               shutdown,
		SetTrace:               setTrace,
		TextDocumentDidOpen:    textDocumentDidOpen,
		TextDocumentDidChange:  textDocumentDidChange,
		TextDocumentDidClose:   textDocumentDidClose, // Handle file closing
		TextDocumentDidSave:    textDocumentDidSave,
		TextDocumentCompletion: textDocumentCompletion,
	}

	s := server.NewServer(&handler, lsName, false)
	s.RunStdio()
}

func initialize(context *glsp.Context, params *protocol.InitializeParams) (interface{}, error) {
	capabilities := handler.CreateServerCapabilities()
	capabilities.CompletionProvider = &protocol.CompletionOptions{
		TriggerCharacters: []string{"."},
	}
	syncKind := protocol.TextDocumentSyncKindFull
	capabilities.TextDocumentSync = &protocol.TextDocumentSyncOptions{
		OpenClose: &[]bool{true}[0],
		Change:    &syncKind,
		Save:      &protocol.SaveOptions{IncludeText: &[]bool{false}[0]},
	}

	return protocol.InitializeResult{
		Capabilities: capabilities,
		ServerInfo: &protocol.InitializeResultServerInfo{
			Name:    lsName,
			Version: &version,
		},
	}, nil
}

func initialized(context *glsp.Context, params *protocol.InitializedParams) error {
	return nil
}

func shutdown(context *glsp.Context) error {
	return nil
}

func setTrace(context *glsp.Context, params *protocol.SetTraceParams) error {
	protocol.SetTraceValue(params.Value)
	return nil
}

func textDocumentDidOpen(context *glsp.Context, params *protocol.DidOpenTextDocumentParams) error {
	documentsMutex.Lock()
	defer documentsMutex.Unlock()
	documents[params.TextDocument.URI] = params.TextDocument.Text
	go publishDiagnostics(context, params.TextDocument.URI, params.TextDocument.Text)
	return nil
}

func textDocumentDidChange(context *glsp.Context, params *protocol.DidChangeTextDocumentParams) error {
	if len(params.ContentChanges) == 0 {
		return nil
	}

	content := params.ContentChanges[0].(protocol.TextDocumentContentChangeEventWhole).Text

	documentsMutex.Lock()
	documents[params.TextDocument.URI] = content
	documentsMutex.Unlock()

	go publishDiagnostics(context, params.TextDocument.URI, content)
	return nil
}

func textDocumentDidClose(context *glsp.Context, params *protocol.DidCloseTextDocumentParams) error {
	documentsMutex.Lock()
	defer documentsMutex.Unlock()
	delete(documents, params.TextDocument.URI)
	return nil
}

func textDocumentDidSave(context *glsp.Context, params *protocol.DidSaveTextDocumentParams) error {
	return nil
}

func textDocumentCompletion(context *glsp.Context, params *protocol.CompletionParams) (interface{}, error) {
	documentsMutex.RLock()
	content, ok := documents[params.TextDocument.URI]
	documentsMutex.RUnlock()

	if !ok {
		return protocol.CompletionList{IsIncomplete: false, Items: []protocol.CompletionItem{}}, nil
	}

	log.Printf("--- Starting Completion Request for %s at L%d:%d ---", params.TextDocument.URI, params.Position.Line+1, params.Position.Character)

	items := []protocol.CompletionItem{}
	seen := make(map[string]bool)

	// Add built-in functions
	kindFunc := CIKFunction
	detailFunc := "built-in function"
	for name := range pyle.Builtins {
		if !seen[name] {
			items = append(items, protocol.CompletionItem{
				Label:  name,
				Kind:   &kindFunc,
				Detail: &detailFunc,
			})
			seen[name] = true
		}
	}

	// Add keywords
	kindKeyword := CIKKeyword
	detailKeyword := "keyword"
	for _, keyword := range pyle.GetAllKeywords() {
		if !seen[keyword] {
			items = append(items, protocol.CompletionItem{
				Label:  keyword,
				Kind:   &kindKeyword,
				Detail: &detailKeyword,
			})
			// THE FIX IS HERE:
			seen[keyword] = true
		}
	}

	log.Printf("Document content loaded successfully.")

	l := pyle.NewLexer(params.TextDocument.URI, content)
	tokens, lexErr := l.Tokenize()
	if lexErr.IsErr() {
		log.Printf("Completion failed: Lexer error: %v", lexErr.Err)
		return protocol.CompletionList{IsIncomplete: false, Items: items}, nil
	}
	log.Printf("Lexing successful. Token count: %d", len(tokens))

	p := pyle.NewParser(tokens)
	astResult := p.Parse()
	if astResult.IsErr() {
		log.Printf("Completion running with parser error: %v", astResult.Err)
	}
	ast := astResult.Value
	if ast != nil {
		log.Println("Parsing successful.")
		currentScope := findInnermostBlock(ast, params.Position)
		if currentScope != nil && currentScope.GetToken() != nil {
			log.Printf("Innermost scope found: %T at line %d", currentScope, currentScope.GetToken().Loc.Line)
		} else {
			log.Printf("Could not determine innermost scope, or scope has no token.")
		}

		pyle.Walk(ast, pyle.WalkFunc(func(node pyle.ASTNode) {
			decl, ok := node.(*pyle.VarDeclareStmt)
			if !ok {
				return
			}
			isBeforeCursor := decl.Token.Loc.Line < int(params.Position.Line)+1 ||
				(decl.Token.Loc.Line == int(params.Position.Line)+1 && decl.Names[0].Loc.ColStart-1 < int(params.Position.Character))
			if !isBeforeCursor {
				return
			}
			if isPositionInNode(params.Position, currentScope) {
				if !seen[decl.Names[0].Value] {
					log.Printf("  -> Accepted: In scope '%s'", decl.Names[0].Value)
					kindVar := CIKVariable
					// detailVar := "variable"
					specifier := "let"
					if decl.IsConst {
						specifier = "const"
					}
					items = append(items, protocol.CompletionItem{
						Label:  decl.Names[0].Value,
						Kind:   &kindVar,
						// Detail: &detailVar,
						Documentation: protocol.MarkupContent{
							Kind: protocol.MarkupKindMarkdown,
							Value: fmt.Sprintf("```pyle\n%s %s: %s\n```",  specifier, decl.Names[0].Value, decl.Initializer.TypeString()),
						},
					})
					seen[decl.Names[0].Value] = true
				}
			}
		}))
	}

	log.Printf("--- Completion Request Finished. Found %d items. ---", len(items))
	return protocol.CompletionList{
		IsIncomplete: false,
		Items:        items,
	}, nil
}

func publishDiagnostics(context *glsp.Context, uri string, content string) {
	diagnostics := []protocol.Diagnostic{}
	severity := protocol.DiagnosticSeverityError

	l := pyle.NewLexer(uri, content)
	tokens, lexErr := l.Tokenize()

	if lexErr.IsErr() {
		err := lexErr.Err
		token := err.GetToken()
		source := "pyle-ls (lexer)"
		diagnostics = append(diagnostics, protocol.Diagnostic{
			Range:    lspRangeFromLoc(token.Loc),
			Severity: &severity,
			Source:   &source,
			Message:  err.Error(),
		})
	}

	if len(tokens) > 0 && len(diagnostics) == 0 {
		p := pyle.NewParser(tokens)
		parseResult := p.Parse()
		if parseResult.IsErr() {
			err := parseResult.Err
			token := err.GetToken()
			source := "pyle-ls (parser)"
			diagnostics = append(diagnostics, protocol.Diagnostic{
				Range:    lspRangeFromLoc(token.Loc),
				Severity: &severity,
				Source:   &source,
				Message:  err.Error(),
			})
		}
	}

	context.Notify(protocol.ServerTextDocumentPublishDiagnostics, protocol.PublishDiagnosticsParams{
		URI:         uri,
		Diagnostics: diagnostics,
	})
}

func lspRangeFromLoc(loc pyle.Loc) protocol.Range {
	startChar := loc.ColStart - 1
	if startChar < 0 {
		startChar = 0
	}
	endChar := startChar + 1
	if loc.ColEnd != nil {
		endChar = *loc.ColEnd
	}

	return protocol.Range{
		Start: protocol.Position{Line: protocol.UInteger(loc.Line - 1), Character: protocol.UInteger(startChar)},
		End:   protocol.Position{Line: protocol.UInteger(loc.Line - 1), Character: protocol.UInteger(endChar)},
	}
}

func isPositionInNode(pos protocol.Position, node pyle.ASTNode) bool {
	if node == nil {
		return false
	}
	startToken := node.GetToken()
	if startToken == nil {
		return false
	}

	cursorLine := pos.Line + 1

	if block, ok := node.(*pyle.Block); ok {
		endToken := block.EndToken
		if endToken == nil {
			return int(cursorLine) >= startToken.Loc.Line
		}
		return int(cursorLine) >= startToken.Loc.Line && int(cursorLine) <= endToken.Loc.Line
	}

	return int(cursorLine) >= startToken.Loc.Line
}

func findInnermostBlock(node pyle.ASTNode, pos protocol.Position) pyle.ASTNode {
	var innermost pyle.ASTNode = node

	walker := pyle.WalkFunc(func(n pyle.ASTNode) {
		if n == nil || n == node {
			return
		}
		if isPositionInNode(pos, n) {
			if _, ok := innermost.(*pyle.Block); ok {
				if isPositionInNode(pos, n) {
					innermost = n
				}
			}
		}
	})

	pyle.Walk(node, walker)

	if _, ok := innermost.(*pyle.Block); !ok {
		pyle.Walk(innermost, pyle.WalkFunc(func(n pyle.ASTNode) {
			if blk, ok := n.(*pyle.Block); ok {
				if isPositionInNode(pos, blk) {
					innermost = findInnermostBlock(blk, pos)
				}
			}
		}))
	}

	return innermost
}