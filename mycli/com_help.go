package main

import "fmt"

func callbackHelp(cfg *config) error {
	fmt.Println("Welcome to the Konohagakure help menu")
	fmt.Println("Here are available commands:")

	availableCommands := getCommands()
	for _, cmd := range availableCommands {
		fmt.Printf(" - %s: %s\n", cmd.name, cmd.description)
	}

	fmt.Println("")
	return nil
}
