# Genio

[![CodeFactor](https://www.codefactor.io/repository/github/genio-the-haiku-ide/genio/badge)](https://www.codefactor.io/repository/github/genio-the-haiku-ide/genio)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/26f32bc4ecf2440d89c1932000405a4d)](https://app.codacy.com/gh/Genio-The-Haiku-IDE/Genio/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
![Screenshot](https://github.com/Genio-The-Haiku-IDE/Genio/blob/main/artwork/screenshot/Genio-screenshot-2.0.png)
![Screenshot-Dark](https://github.com/Genio-The-Haiku-IDE/Genio/blob/main/artwork/screenshot/Genio-screenshot-dark-2.0.png)


## Introduction

Genio is a native and fully functional IDE for the [Haiku operating system](https://www.haiku-os.org)

Some of the features of the Genio IDE are:

*   LSP Server support (autocompletion, signature help, go to definition/implementation/declaration, quick fix, format)
*   Multi-project browser
*   Integrated source control with GIT (including opening a remote project)
*   Find in files
*   Links to file and build errors in Build Log and Console I/O
*   "Problems" tab
*   Build on save / Save on build
*   User templates for quickly creating new files and projects
*   Rich editor with many features:
    *   Multiple tabs
    *   Syntax highlighting for many languages
    *   Highlight/Trim whitespace
    *   Comment/uncomment lines
    *   Duplicate current line
    *   Delete lines
    *   Switch between source and header
*   Full screen and Focus mode

Genio started off as a fork of [Ideam](https://github.com/AmosCaster/ideam), and
 the editor is based on [Scintilla for Haiku](https://sourceforge.net/p/scintilla/haiku/ci/default/tree/).

We also took inspiration and code from the editor [Koder](https://github.com/KapiX/Koder).

*   strongly recommended for full Genio experience (autocompletion, jump to definition, etc):
    *   gcc_syslibs_devel
    *   llvm17_clang

```bash
pkgman install gcc_syslibs_devel llvm17_clang
```

## Goals and roadmap

Genio aims to be an easy, simple yet powerful IDE for Haiku inspired by VS Code and Nova.

*   Plug-in architecture
*   Bring the editor up-to-date and on par with other Haiku editors (Koder, Pe)
*   Compiler error parser

## Configuring LSP

For more advanced IDE features, Genio implements the LSP protocol. (<https://microsoft.github.io/language-server-protocol/>)

* For C and C++ projects you can use clangd. See [Configuring-clangd-lsp.md](https://github.com/Genio-The-Haiku-IDE/Genio/blob/main/Configuring-clangd-lsp.md)
* For Python projects use can install and use [Python LSP Server](https://github.com/python-lsp/python-lsp-server)

## Building Genio

### Prerequirements

Genio requires Scintilla and Lexilla to implement various functionalities.
It also requires libgit2 to implement Git features and libyaml_cpp to read yaml files.
The needed development files are available in `libgit2_devel`, `lexilla_devel` and
`yaml_cpp_devel`, respectively.
Execute `pkgman install libgit2_devel lexilla_devel yaml_cpp_devel` from Terminal.

If you would like to try a clang++ build:

*   Install `llvm_clang` hpkg from HaikuPorts
*   Set `BUILD_WITH_CLANG` to `1` in `Makefile`

### Compiling

Execute `make deps && make` in Genio's top directory.
The executable is created in `app` subdirectory.

Genio can already be opened and built within Genio itself.
The makefile has been updated to accept the *debug* parameter:

*   debug=1 - Genio is built in debug mode
*   debug=0 or parameter omitted - Genio is built in release mode

## Contributions

We gladly accept contributions, especially for bug fixes. Feel free to submit PRs.
For code contributions, prefer Haiku API over posix, where applicable.
We (try to) stick to the Haiku style for code, although with a few differences.

## License

Genio is available under the MIT license. See [License.md](License.md).
