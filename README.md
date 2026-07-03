# .a Programming Language

`.a` is an interpreted, object-oriented programming language written in C. It was designed to be simple enough for beginners while still supporting modern programming concepts such as classes, functions, loops, structures, file I/O, and modules.

The goal of `.a` is to help students and educators focus on learning programming concepts instead of struggling with complex syntax.

---

## Features

- Easy-to-read syntax inspired by Python and Java
- Object-Oriented Programming (Classes)
- C-style Structures
- Functions with parameters and return values
- Variables and constants
- Loops and conditionals
- Lists and Dictionaries
- File Input/Output
- Module importing
- Built-in standard library
- String interpolation (F-Strings)
- Arithmetic, including exponentiation
- Pointer support
- Written entirely in C

---

## Building

Compile the interpreter using:

```bash
make build
```

This creates the interpreter executable:

```text
arun
```

---

## Running Programs

Execute a `.a` program using:

```bash
arun program.a
```

Example:

```bash
arun examples/hello.a
```

---

## Testing

Run the test suite:

```bash
make test
```

---

## Cleaning

Remove compiled files:

```bash
make clean
```

---

## Hello World

```a
print("Hello, World!")
```

---

## Example

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

## Why .a?

Many programming languages are designed for professional software development. While powerful, they can be intimidating for new programmers.

`.a` was created to provide a language that is:

- Easy to read
- Easy to write
- Consistent
- Educational
- Modern enough for real programming projects

The emphasis is on understanding programming concepts without unnecessary syntax getting in the way.

---

## Project Goals

- Help students learn programming fundamentals.
- Provide educators with a clean teaching language.
- Offer a familiar syntax for users coming from Python, Java, or C.
- Demonstrate how an interpreter works from lexer to runtime.

---

## Architecture

`.a` follows a traditional interpreter pipeline:

```
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

## Project Status

Current features include:

- Lexer
- Recursive Descent Parser
- Abstract Syntax Tree (AST)
- Runtime Environment
- Interpreter
- Standard Library
- Module Imports
- File I/O

---

## License

This project is open for learning, experimentation, and educational use.
