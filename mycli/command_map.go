package main

import (
	"fmt"
	"mycli/internal/clapi"
)

func callbackMap(cfg *config) error {
	cliClient := clapi.NewClient()

	resp, err := cliClient.ListLocationAreas(cfg.nextLocationAreaURL)
	if err != nil {
		return err
	}

	fmt.Println("Location Areas :")
	for _, area := range resp.Results {
		fmt.Printf(" - %s\n", area.Name)
	}
	cfg.nextLocationAreaURL = resp.Next
	cfg.prevLocationAreaURL = resp.Previous
	return nil
}

func callbackMapback(cfg *config) error {
	if cfg.prevLocationAreaURL == nil {
		fmt.Println("No previous page available.")
		return nil
	}
	cliClient := clapi.NewClient()

	resp, err := cliClient.ListLocationAreas(cfg.prevLocationAreaURL)
	if err != nil {
		return err
	}

	fmt.Println("Location Areas :")
	for _, area := range resp.Results {
		fmt.Printf(" - %s\n", area.Name)
	}
	cfg.nextLocationAreaURL = resp.Next
	cfg.prevLocationAreaURL = resp.Previous
	return nil
}
