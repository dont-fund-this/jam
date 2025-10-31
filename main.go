package main

import (
	"embed"
	"jam/internal/core"
)

//go:embed all:assets
var assets embed.FS

func main() {
	core.Walk(assets)
}

// this is a troubling file
// this file feels like it does altogether too many thing
// revisit the hot wet trash here and reduce to
// ideally have a single reason to ever change
// why does it need to know about 'a s s e t s'
// why does it need to know anything at all about "embed" or any concrete impl detail
// aim to reduce this do a single thing
// tag failure on delivery with
// angry accept as a walking and chewing gum file, for now.
// it walks, and chews gum, And knows how the gum is made.
// that unnatural and distracting when you are walking and chewing gum.
