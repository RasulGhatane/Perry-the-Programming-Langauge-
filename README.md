# Perry the Programming Language?

## Overview
This project implements a **Perry the Programming Language** in C. The language supports **variables, arithmetic operations, loops, conditional statements, and printing statements** with a unique syntax. It tokenizes, parses, and executes the input script using a hash-based variable storage system.

## Features
- **Tokenization & Parsing**: Converts input scripts into tokens for execution.
- **Mathematical Operations**: Supports addition, subtraction, multiplication, division, and modulus.
- **Variables Management**: Allows defining and modifying integer variables.
- **Control Structures**:
  - `yap("text");` → Print text or evaluated expression.
  - `sigma var = expr;` → Assigns an expression to a variable.
  - `fr_fr (condition) { ... }` → Loop execution while the condition is true.
  - `based (condition) { ... }` → If statement for conditional execution.
- **Error Handling**: Detects syntax and runtime errors like undefined variables and division by zero.
- **Memory Management**: Cleans up allocated resources upon exit.

## Syntax
```c
yap("Hello, World!");          // Print a string
yap(5 + 3 * 2);                // Print an evaluated expression

sigma x = 10;                  // Assigns 10 to variable 'x'
yap(x * 2);                    // Prints 20

fr_fr (x > 0) {                // Loop while x is greater than 0
    yap(x);
    sigma x = x - 1;           // Decrease x by 1
}

based (x == 0) {               // If statement check
    yap("x is zero");
}
```

## Installation & Usage
### Prerequisites
- GCC or Clang compiler
- A C development environment

### Compilation
```sh
gcc -o interpreter perry.c -Wall -Wextra
```

### Running a Script
```sh
./interpreter index.perry
```
where `index.perry` contains code written in this custom scripting language.

## Error Handling
The interpreter provides detailed error messages with position tracking:
- **Undefined variable**: If a variable is used before being assigned.
- **Unexpected token**: If syntax does not match expected patterns.
- **Division by zero**: Prevents arithmetic errors.


