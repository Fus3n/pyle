package pyle

import (
	"bytes"
	"fmt"
	"image/color"
	"os"
	"strings"

	"github.com/hajimehoshi/ebiten/v2"
	"github.com/hajimehoshi/ebiten/v2/ebitenutil"
	"github.com/hajimehoshi/ebiten/v2/text/v2"
	"github.com/hajimehoshi/ebiten/v2/vector"
)

type FontObj struct {
	FaceSource *text.GoTextFaceSource
	Size       float64
	loc        Loc
}

func (f *FontObj) String() string { return fmt.Sprintf("<font %.1fpx>", f.Size) }
func (f *FontObj) Type() string   { return "font" }
func (f *FontObj) IsTruthy() bool { return true }
func (f *FontObj) Iter() (Iterator, Error) {
	return nil, NewRuntimeError("font is not iterable", Loc{})
}
func (f *FontObj) GetAttribute(name string) (Object, bool, Error) {
	if name == "size" {
		return NumberObj{Value: f.Size, IsInt: false}, true, nil
	}
	if name == "set_size" {
		fn, err := CreateNativeFunction("set_size", func(size float64) {
			f.Size = size
		}, nil)
		if err != nil {
			return nil, false, NewRuntimeError(err.Error(), f.GetLocation())
		}
		return fn, true, nil
	}
	return nil, false, nil
}
func (f *FontObj) GetLocation() Loc  { return f.loc }
func (f *FontObj) SetLocation(l Loc) { f.loc = l }

type PyleGame struct {
	vm       *VM
	updateFn Object
	drawFn   Object
}

func (g *PyleGame) Update() error {
	res, err := g.vm.CallFunction(g.updateFn, []Object{})
	if err != nil {
		fmt.Printf("Game Update Runtime Error: %v\n", err)
		return fmt.Errorf("runtime error: %v", err)
	}

	if result, ok := res.(*ResultObject); ok && result.Error != nil {
		fmt.Printf("Game Update Logic Error: %v\n", result.Error.Message)
		return fmt.Errorf("pyle error: %s", result.Error.Message)
	}
	return nil
}

func (g *PyleGame) Draw(screen *ebiten.Image) {
	screenObj := &UserObject{Value: screen}
	res, err := g.vm.CallFunction(g.drawFn, []Object{screenObj})
	if err != nil {
		fmt.Printf("Game Draw Runtime Error: %v\n", err)
		return
	}
	if result, ok := res.(*ResultObject); ok && result.Error != nil {
		fmt.Printf("Game Draw Logic Error: %v\n", result.Error.Message)
	}
}

func (g *PyleGame) Layout(outsideWidth, outsideHeight int) (int, int) {
	return outsideWidth, outsideHeight
}

// --- Native Functions ---

func gameInit(width, height float64, title string) {
	ebiten.SetWindowSize(int(width), int(height))
	ebiten.SetWindowTitle(title)
}

func gameRun(vm *VM, updateFn, drawFn Object) (Object, Error) {
	game := &PyleGame{vm: vm, updateFn: updateFn, drawFn: drawFn}
	fmt.Println("Starting Ebiten Game Loop...")
	if err := ebiten.RunGame(game); err != nil {
		return ReturnError(fmt.Sprintf("Game Error: %v", err)), nil
	}
	return ReturnOkNull(), nil
}

func gameLoadImage(path string) *ResultObject {
	img, _, err := ebitenutil.NewImageFromFile(path)
	if err != nil {
		return ReturnError(fmt.Sprintf("Failed to load image: %v", err))
	}
	return ReturnOk(&UserObject{Value: img})
}

func gameDrawImage(screenObj, imgObj *UserObject, x, y float64) {
	screen := screenObj.Value.(*ebiten.Image)
	img := imgObj.Value.(*ebiten.Image)
	opts := &ebiten.DrawImageOptions{}
	opts.GeoM.Translate(x, y)
	screen.DrawImage(img, opts)
}

func gameDebugPrint(screenObj *UserObject, text string) {
	screen := screenObj.Value.(*ebiten.Image)
	ebitenutil.DebugPrint(screen, text)
}

func gameLoadFont(path string) *ResultObject {
	data, err := os.ReadFile(path)
	if err != nil {
		return ReturnError(fmt.Sprintf("Failed to load font: %v", err))
	}
	source, err := text.NewGoTextFaceSource(bytes.NewReader(data))
	if err != nil {
		return ReturnError(fmt.Sprintf("Failed to parse font: %v", err))
	}
	return ReturnOk(&FontObj{FaceSource: source, Size: 16})
}

func gameDrawText(vm *VM, args ...Object) (Object, Error) {
	if len(args) < 5 {
		return nil, NewRuntimeError("Usage: pylegame.draw_text(screen, font, text, x, y, [r, g, b, a])", Loc{})
	}
	screenObj := args[0].(*UserObject)
	fontObj := args[1].(*FontObj)
	textStr := args[2].(StringObj).Value
	x := args[3].(NumberObj).Value
	y := args[4].(NumberObj).Value

	r, g, b, a := 255.0, 255.0, 255.0, 255.0
	if len(args) >= 8 {
		r = args[5].(NumberObj).Value
		g = args[6].(NumberObj).Value
		b = args[7].(NumberObj).Value
		if len(args) > 8 {
			a = args[8].(NumberObj).Value
		}
	}

	screen := screenObj.Value.(*ebiten.Image)
	op := &text.DrawOptions{}
	op.GeoM.Translate(x, y)
	op.ColorScale.Scale(float32(r/255.0), float32(g/255.0), float32(b/255.0), float32(a/255.0))

	face := &text.GoTextFace{Source: fontObj.FaceSource, Size: fontObj.Size}
	text.Draw(screen, textStr, face, op)
	return NullObj{}, nil
}

func gameMeasureText(fontObj *FontObj, textStr string) Object {
	face := &text.GoTextFace{Source: fontObj.FaceSource, Size: fontObj.Size}
	w, h := text.Measure(textStr, face, face.Size)
	m := NewMap()
	_ = m.Set(StringObj{Value: "w"}, NumberObj{Value: w, IsInt: false})
	_ = m.Set(StringObj{Value: "h"}, NumberObj{Value: h, IsInt: false})
	return m
}

func gameDrawRect(screenObj *UserObject, x, y, w, h, r, g, b, a float32) {
	screen := screenObj.Value.(*ebiten.Image)
	clr := color.RGBA{uint8(r), uint8(g), uint8(b), uint8(a)}
	vector.FillRect(screen, x, y, w, h, clr, true)
}

func gameIsKeyPressed(key string) bool {
	if k, found := keyMap[strings.ToUpper(key)]; found {
		return ebiten.IsKeyPressed(k)
	}
	return false
}

var keyMap = map[string]ebiten.Key{
	"A": ebiten.KeyA, "B": ebiten.KeyB, "C": ebiten.KeyC, "D": ebiten.KeyD,
	"E": ebiten.KeyE, "F": ebiten.KeyF, "G": ebiten.KeyG, "H": ebiten.KeyH,
	"I": ebiten.KeyI, "J": ebiten.KeyJ, "K": ebiten.KeyK, "L": ebiten.KeyL,
	"M": ebiten.KeyM, "N": ebiten.KeyN, "O": ebiten.KeyO, "P": ebiten.KeyP,
	"Q": ebiten.KeyQ, "R": ebiten.KeyR, "S": ebiten.KeyS, "T": ebiten.KeyT,
	"U": ebiten.KeyU, "V": ebiten.KeyV, "W": ebiten.KeyW, "X": ebiten.KeyX,
	"Y": ebiten.KeyY, "Z": ebiten.KeyZ,
	"LEFT": ebiten.KeyArrowLeft, "RIGHT": ebiten.KeyArrowRight,
	"UP": ebiten.KeyArrowUp, "DOWN": ebiten.KeyArrowDown,
	"SPACE": ebiten.KeySpace, "ENTER": ebiten.KeyEnter, "ESC": ebiten.KeyEscape,
}

func CreateGameModule(vm *VM) Object {
	gameMod := NewModule("pylegame")
	ModuleMustRegister(gameMod, "init", gameInit, nil)
	ModuleMustRegister(gameMod, "run", gameRun, nil)
	ModuleMustRegister(gameMod, "load_image", gameLoadImage, nil)
	ModuleMustRegister(gameMod, "draw_image", gameDrawImage, nil)
	ModuleMustRegister(gameMod, "load_font", gameLoadFont, nil)
	ModuleMustRegister(gameMod, "debug_print", gameDebugPrint, nil)
	ModuleMustRegister(gameMod, "draw_text", gameDrawText, nil)
	ModuleMustRegister(gameMod, "measure_text", gameMeasureText, nil)
	ModuleMustRegister(gameMod, "is_key_pressed", gameIsKeyPressed, nil)
	ModuleMustRegister(gameMod, "draw_rect", gameDrawRect, nil)
	return gameMod
}
