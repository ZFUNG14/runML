# 🛠️ Mini-Language Transpiler and Executor (runml)

As part of the CITS2002 [Systems Programming unit](https://handbooks.uwa.edu.au/unitdetails?code=CITS2002), I developed runml, a C11 program that transpiles and executes code written in a custom-designed mini-language `.ml`.

### 💼 This project involved:

- Parsing and validating `.ml` source files with a simple syntax and scoping rules
- Translating the validated program into C11 source code
- Compiling the generated code using system compilers (e.g., gcc)
- Executing the resulting binary with runtime arguments
- Handling system-level features such as process creation, command-line interfaces, and temporary file management

### 💡 What I Learned

This project deepened my understanding of:

- How programming languages are built and executed, from parsing to code generation
- The use of C11 as a "high-level assembly language" for language support
- System programming concepts like process control (fork, exec, wait), standard I/O, and file handling
- Writing clean, modular code that adheres to UNIX utility design principles
- Through this, I gained practical experience in building language tools, managing system-level resources, and working within the constraints of POSIX-compliant environments.

### 🚀 How to Use

1. Firstly, clone the repository using the below command

```
git clone https://github.com/ZFUNG14/runML.git
cd runML
```

2. Next, compile `runml.c` using the below line:

   `cc -std=c11 -Wall -Werror -o runml runml.c`

This command builds the transpiler and outputs a binary called `runml`.

3. Lastly, run the transpiler with an `.ml` source file.

   Example:
   `./runml sample02.ml`

This results in:

1. Parse and validate the sample02.ml source code.
2. Generate a temporary C file (e.g., ml-12345.c).
3. Compile it into an executable.
4. Run the program and print output to the terminal.
5. Automatically clean up generated files after execution.

### 📄 Sample Files

This repository includes 9 sample `.ml` files (sample01.ml to sample09.ml) demonstrating various language features like:

- Variable assignment
- Arithmetic expressions
- `print` statements
- User-defined functions with `return`
- Function calls and parameter passing

Each file includes inline comments describing expected output for reference.

### 🚷 `.ml` Syntax:

```
  |       means a choice
  [...]   means zero or one
  (...)*  means zero or more

Program lines commence at the left-hand margin (no indentation).
Statements forming the body of a function are indented with a single tab.

program:
           ( program-item )*

program-item:
           statement
        |  function identifier ( identifier )*
           ←–tab–→ statement1
           ←–tab–→ statement2
           ....

statement:
           identifier "<-" expression
        |  print  expression
        |  return expression
        |  functioncall

expression:
          term   [ ("+" | "-")  expression ]

term:
          factor [ ("*" | "/")  term ]

factor:
          realconstant
        | identifier
        | functioncall
        | "(" expression ")"

functioncall:
          identifier "(" [ expression ( "," expression )* ] ")"
```

1. Programs are written in text files whose names end in `.ml`
2. Statements are written _one-per-line_ (with no terminating semi-colon)
3. The character `#` appearing anywhere on a line introduces a **comment** which extends until the end of that line
4. Only a **real numbers** datatype is supported. Example: 2.71828
5. Identifiers (variable and function names) only consist of 1..12 lowercase alphabetic characters.
6. There will be **at most 50 unique identifiers** appearing in any program
7. Variables do not need to be defined before being used in an expression, and are automatically initialised to the _real value 0.0_
8. The variables `arg0`, `arg1`, and so on, provide access to the program's command-line arguments which provide real-valued numbers
9. A function must have been defined before it is called in an expression
10. Each statement in a function's body (one-per-line) is indented with a tab character
11. Functions may have **zero-or-more** formal parameters
12. A function's parameters and any other identifiers used in a function body are local to that function, and become unavailable when the function's execution completes
13. Programs execute their statements from top-to-bottom and function calls are the only form of control-flow

Note: **The `runml` transpiler is built to strictly follow the mini-language specification below. Any `.ml` file that does not conform to these rules will not be processed successfully. Errors will be reported during validation.**

### ⚠️ Limitations

- **No Expression Validation**: While basic syntax is checked, the contents of arithmetic expressions are assumed to be valid (per project spec).
- **No Control Flow**: The language does not support conditionals `if` or loops `while`, `for`. All flow is managed through function calls.
- **Limited Identifier Rules**:
  - Identifiers must be 1–12 lowercase alphabetic characters.
  - Maximum of 50 unique identifiers per `.ml` file.
- **Static Memory for Identifiers**: No dynamic memory allocation used for identifier management.
- **Function Order Dependency**: Functions must be declared before they are called.
