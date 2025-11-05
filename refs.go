package main

import (
	"embed"
	"jam/internal/tech/efx"
)

//go:embed all:refs
var refs embed.FS

func refsImpl(b *efx.Bag) (interface{}, error) {
	return refs, nil
}

func refsWith() *efx.Def {
	return &efx.Def{
		Sid: "app.refs",
		Tag: "refs",
		Fun: refsImpl,
	}
}
