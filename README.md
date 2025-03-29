# AtomC-Compiler
AtomC Compiler - A simple and lightweight compiler for the AtomC language, written in C. Features lexical analysis, parsing.

## Building the project
You can build the project using the provided Makefile:

```
make
```

Or directly with gcc:

```
gcc -std=c11 -o p.exe -I include src/main.c src/lexer.c src/parser.c src/utils.c
```

## Running the compiler
To run the compiler, use:

```
./p.exe your_file.atc
```

This will perform lexical analysis on your AtomC source file.
