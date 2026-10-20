package main

import (
	"bufio"
	"fmt"
	"os"
)

// go run . --file file.data --query 'SPUSH mystack item'
func main() {
	file, query := "file.data", ""
	for i := 1; i+1 < len(os.Args); i += 2 {
		switch os.Args[i] {
		case "--file":
			file = os.Args[i+1]
		case "--query":
			query = os.Args[i+1]
		}
	}

	db := Db{}
	loadDb(db, file)

	if query != "" {
		fmt.Println(execute(db, query))
		saveDb(db, file)
		return
	}

	fmt.Printf("Файл данных: %s. Введите команду (exit - выход)\n", file)
	sc := bufio.NewScanner(os.Stdin)
	fmt.Print("> ")
	for sc.Scan() {
		line := sc.Text()
		if line == "exit" || line == "quit" {
			break
		}
		if line != "" {
			fmt.Println(execute(db, line))
			saveDb(db, file) // запись в файл после каждой команды
		}
		fmt.Print("> ")
	}
}
