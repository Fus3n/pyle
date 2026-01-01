//go:build pyle_no_game

package pyle

// RegisterGameModule is a no-op stub for documentation generation on headless systems.
func RegisterGameModule(vm *VM) {}

// FontObj stub to satisfy potential references
type FontObj struct {
	FaceSource any
	Size       float64
	loc        Loc
}

func (f *FontObj) String() string                               { return "<font stub>" }
func (f *FontObj) Type() string                                 { return "font" }
func (f *FontObj) IsTruthy() bool                               { return true }
func (f *FontObj) Iter() (Iterator, Error)                      { return nil, nil }
func (f *FontObj) GetAttribute(name string) (Object, bool, Error) { return nil, false, nil }
func (f *FontObj) GetLocation() Loc                             { return f.loc }
func (f *FontObj) SetLocation(l Loc)                            { f.loc = l }
