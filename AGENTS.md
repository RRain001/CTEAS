# AGENTS.md

## Project overview

This project is CTEAS, a tritium environmental assessment software package.

It contains:

- Python GUI and business logic.
- YAML-based project/configuration files.
- Native compiled executables with `.x` suffix.
- Fortran/C/C++ source files used to build some native executables.
- Docker-based Linux native binary build workflow.
- Platform-specific binary directories for Linux x86_64, Linux ARM64, and later Windows x86_64.

The project was originally developed on Linux x86_64. Existing `.x` files are platform-specific and must not be assumed to work on Linux ARM64 or Windows.

## Current development goal

The current goal is not to rewrite the whole software.

The current goal is to:

1. Inventory the existing project.
2. Identify all `.x` native binaries.
3. Map `.x` binaries to their source files where possible.
4. Rebuild Python virtual environment dependencies.
5. Add Docker-based build scripts for Linux x86_64 and Linux ARM64 native binaries.
6. Organize platform-specific binaries under `bin/<platform>/`.
7. Decouple GUI code from hard-coded native executable paths.
8. Keep the existing GUI behavior stable.

## Supported platforms

Primary target platforms:

- `linux-x86_64`
- `linux-arm64`
- `windows-x86_64` later

Do not work on macOS packaging or macOS-specific logic unless explicitly requested.

Do not work on Windows ARM64.

## Platform-specific binary layout

Use this layout for native executables:

```text
bin/
  linux-x86_64/
    *.x

  linux-arm64/
    *.x

  windows-x86_64/
    *.exe