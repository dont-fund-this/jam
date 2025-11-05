package main

import (
	"jam/internal/tech/efx"
)

func With() []*efx.Def {
	return []*efx.Def{
		refsWith(),
	}
}
