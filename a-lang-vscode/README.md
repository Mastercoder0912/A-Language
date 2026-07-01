# A Language Syntax Highlighting for VS Code

Syntax highlighting extension for the **A Programming Language**.

## Features

- **Syntax Highlighting** for all A language constructs:
  - Keywords: `if`, `else`, `loop`, `function`, `class`, `struct`, `private`, `return`, `pass`
  - Types: `int`, `string`, `boolean`, `list`, `dict`
  - Built-in functions: `print()`, `len()`, `type()`, `input()`, `randint()`, `int()`, `string()`, `bool()`
  - Boolean constants: `True`, `False`
  - Strings, chars, and f-strings: `"text"`, `'a'`, `f"interpolated {expr}"`
  - Comments: `#`, `//`, `/* */`
  - Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `>`, `<`, `++`, `.`, `and`, `or`, etc.

- **Smart Bracket Matching**
- **Auto-closing Brackets** and Quotes
- **Smart Indentation** for control flow blocks

## Installation

### Local Development

1. Copy this extension folder to your VS Code extensions directory:
   - **Linux/Mac**: `~/.vscode/extensions/a-language-0.2.0/`
   - **Windows**: `%USERPROFILE%\.vscode\extensions\a-language-0.2.0\`

2. Restart VS Code

3. Open any `.a` file to see syntax highlighting in action

### From Source (Development)

```bash
cd a-lang-vscode
npm run validate
```

This extension does not need compiling. Then use VS Code's "Run Extension" feature
or manually copy it to your extensions folder.

## Usage

Create a file with `.a` extension and start coding in A language:

```a
// Simple Hello World
print("Hello, World!")

// Functions with default parameters
function greet(name, greeting = "Hello") {
    print(greeting + " " + name)
}

greet("Alice")
greet("Bob", "Hi")

// Classes
class Dog {
    private string name
    
    function Dog() {
        name = "Buddy"
    }
    
    function .bark() {
        print(name + " says: Woof!")
    }
}

Dog d = Dog()
d.bark()

// Structs and loops
struct Point {
    int x;
    int y;
}

Point p = Point(10, 20)
loop (3 as i) {
    print(f"Point ({p.x}, {p.y})")
}
```

## Color Theme

Colors automatically adapt to your VS Code theme. The extension uses standard VS Code scopes for:

- **Keywords** (purple/blue in most themes)
- **Types** (blue in most themes)
- **Strings** (red/orange in most themes)
- **Numbers** (green/cyan in most themes)
- **Comments** (gray in most themes)
- **Functions** (yellow in most themes)
- **Variables** (white/default in most themes)

## File Structure

```
a-lang-vscode/
├── package.json                    # Extension metadata
├── language-configuration.json      # Language rules (comments, brackets, etc.)
├── syntaxes/
│   └── a.tmLanguage.json           # TextMate grammar (syntax highlighting rules)
├── scripts/
│   └── validate-extension.js        # Local validation helper
└── README.md                        # This file
```

## Supported Constructs

### Data Types
- Primitive: `int`, `string`, `boolean`
- Collections: `list`, `dict`
- User-defined: `struct`, `class`

### Control Flow
- `if` / `else if` / `else`
- `loop` (counted and conditional)
- `pass` (no-op)
- `return`

### Functions & Methods
- Function declarations with default parameters
- Method definitions with `.` prefix
- Function calls with arguments

### Object-Oriented Features
- Struct definitions and instantiation
- Class definitions with private members
- Constructors (matching class name)
- Method calls

### String Features
- String literals: `"text"`
- Char/single-quoted literals: `'a'`
- F-strings: `f"Value: {expression}"`
- String concatenation: `"a" + "b"`

### Comments
- Line comment: `# comment`
- Line comment: `// comment`
- Block comment: `/* multi-line comment */`

## Known Limitations

- Some complex nested structures may not highlight perfectly
- Custom theme support may require manual color adjustments
- No IntelliSense (yet)

## Future Enhancements

- [ ] IntelliSense / autocomplete
- [ ] Go to definition
- [ ] Hover documentation
- [ ] Linting
- [ ] Debugging integration
- [ ] Code snippets

## License

MIT

## Contributing

For improvements or bug reports, please submit issues or pull requests.

---

Made for the 7-Day Language Challenge.
