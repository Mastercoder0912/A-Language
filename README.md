# .a Programming Language

**.a** is an educational, interpreted programming language written entirely in C. It is designed to make learning programming easier by reducing unnecessary syntax while still supporting modern programming concepts such as classes, functions, modules, collections, file I/O, and object-oriented programming.

Beyond being beginner-friendly, **.a** also serves as a complete interpreter implementation, demonstrating every major stage of language execution—from lexing and parsing to runtime interpretation.

---

## In Action

```console
$ git clone https://github.com/Mastercoder0912/A-Language.git
Cloning into 'A-Language'...

$ cd A-Language

$ make install
Building .a...
Build complete.

$ arun examples/hello.a
Hello, World!
```

> **Coming soon:** An animated terminal demo powered by Asciinema.

---

## Who is .a for?

- Students learning programming fundamentals
- Educators teaching introductory computer science
- Developers interested in interpreter implementation
- Anyone curious about programming language design

---

# Features

## Language Features

- Easy-to-read syntax inspired by Python and Java
- Object-Oriented Programming (Classes)
- C-style Structures
- Functions with parameters and return values
- Variables and constants
- Loops and conditionals
- Lists and Dictionaries
- String interpolation (F-Strings)
- Arithmetic (including exponentiation)
- Pointer support

## Runtime Features

- Module importing
- File Input/Output
- Built-in standard library

## Implementation

- Written entirely in C
- Recursive Descent Parser
- Abstract Syntax Tree (AST)
- Tree-walk Interpreter

---

# Installing

## premade Interpreter
```bash
curl https://a-lang-installer-4deb1e2c5f3e.herokuapp.com/install/{linux, macos, or windows} | bash
```

## Source code / Make your self
Clone the repository:

```bash
git clone https://github.com/Mastercoder0912/A-Language.git
```

Move into the project:

```bash
cd A-Language
```

Compile and install:

```bash
make install
```

Run your first program:

```bash
arun examples/hello.a
```

---

# Building

Compile the interpreter manually:

```bash
make build
```

This creates:

```text
arun
```

---

# Running Programs

Execute any `.a` program using:

```bash
arun program.a
```

Example:

```bash
arun examples/hello.a
```

---

# Hello, World!

```a
print("Hello, World!")
```

---

# Quick Example

```a
const PI = 3.14159

function square(x){
    return x ^ 2
}

class Duck{
    private string name

    function Duck(name){
        this.name = name
    }

    function speak(){
        print("Quack! My name is " + name)
    }
}

Duck d = Duck("Jeff")

print(square(5))
d.speak()
```

---

# Why .a?

Many programming languages are designed for professional software development. While powerful, they can be intimidating for new programmers.

.a was created to help students focus on learning programming concepts rather than struggling with complicated syntax.

It aims to be:

- Easy to read
- Easy to write
- Consistent
- Beginner-friendly
- Powerful enough for meaningful projects
- A practical example of interpreter design

Whether you're learning your first language or studying how interpreters work internally, **.a** provides a straightforward learning experience.

---

# Project Goals

- Help students learn programming fundamentals
- Provide educators with a clean teaching language
- Offer familiar syntax for users coming from Python, Java, or C
- Demonstrate how an interpreter works from lexer to runtime

---

# Architecture

```text
Source (.a)
      │
      ▼
   Lexer
      │
      ▼
   Parser
      │
      ▼
Abstract Syntax Tree (AST)
      │
      ▼
 Interpreter
      │
      ▼
 Program Output
```

---

# Roadmap

## Completed

- Lexer
- Recursive Descent Parser
- Abstract Syntax Tree
- Runtime Environment
- Object-Oriented Programming
- Module Imports
- File I/O
- SQLite Database Access
- JSON File Reading
- OS class
- Queue Data Type
- Standard Library

## Planned

- Garbage Collection
- Multi-language Processing
- Additional Low-level Capabilities
- Expanded Standard Library

---

# Testing

Run the test suite:

```bash
make test
```

---

# Cleaning

Remove compiled files:

```bash
make clean
```

---

# Support the Project

If you find **.a** interesting or useful, consider giving the repository a ⭐ or sharing it with others. Every bit of support helps more people discover the project.

---

# License

This project is open for learning, experimentation, and educational use.
