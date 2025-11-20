# Overview
A compact Unix-like command-line shell implemented in C. The project demonstrates parsing of command lines and execution of external programs using POSIX process primitives. It is designed as an educational, easy-to-read implementation that separates parsing, execution logic, and utility code so readers can follow how a minimal shell processes user input and launches commands.

# Key features
- **Command parsing** for simple command lines and arguments.
- **Process execution** using fork and exec family calls.
- **Basic job handling** and simple foreground/background semantics where implemented.
- **Modular C codebase** with clear separation of parsing and execution responsibilities.
- **Makefile build** that compiles sources into an executable placed in bin/.

# Repository structure
- ```bin/``` — compiled executable(s).
- ```include/``` — public headers and interfaces.
- ```input_parse/``` — parsing-related source files.
- ```src/``` — core execution logic and utilities.
- ```obj/``` — object files and intermediate build artifacts.
- ```Makefile``` — build rules and targets.

# Build
- **Requirements:** GCC/clang and a POSIX-compatible development environment.
- **Build command:** run make from the repository root.
- **Output:** executable(s) appear in the bin/ directory.

# Usage
- Run the shell executable from **bin/** (e.g., ```./bin/shell``` or the executable name produced by the Makefile).
- Enter commands similar to a standard shell; the implementation focuses on demonstrating parsing and process creation rather than full POSIX shell feature parity.

# Implementation notes
- Written in idiomatic C with header/source separation for clarity.
- Designed for study and incremental extension: follow the code flow from input reading to parsing to process creation to understand how a minimal shell operates.
