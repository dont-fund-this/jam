package main

import (
	"embed"
	"jam/internal/tech/efx"
)

//go:embed all:assets
var assets embed.FS

func With() *efx.Def {
	return efx.With(efx.Def{
		SID: "main.efs.assets",
		Tag: "assets",
		Fun: func(_ *efx.Bag, _ ...interface{}) (interface{}, error) {
			return assets, nil
		},
	})
}
