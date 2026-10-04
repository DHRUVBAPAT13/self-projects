package main

import (
	//"fmt"
	"mycli/internal/clapi"
	//"log"
)

type config struct {
	// Define your config fields here
	Client              clapi.Client
	nextLocationAreaURL *string
	prevLocationAreaURL *string
}

func main() {

	cfg := config{
		Client: clapi.NewClient(),
	}
	startRepl(&cfg)
}
