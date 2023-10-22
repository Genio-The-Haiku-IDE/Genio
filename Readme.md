Genio
================================================================================
[![CodeFactor](https://www.codefactor.io/repository/github/genio-the-haiku-ide/genio/badge)](https://www.codefactor.io/repository/github/genio-the-haiku-ide/genio)
[![Codacy Badge](https://app.codacy.com/project/badge/Grade/26f32bc4ecf2440d89c1932000405a4d)](https://app.codacy.com/gh/Genio-The-Haiku-IDE/Genio/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade)
![Screenshot](https://github.com/Genio-The-Haiku-IDE/Genio/blob/main/data/screenshot/Genio-screenshot-1.0.png)
    Genio.png

# Introduction
Genio is an IDE for the [Haiku operating system](https://www.haiku-os.org).
Despite being in an early stage, Genio can already be used to develop and build applications.
In fact, the Genio team uses it as the main development tool to develop and build Genio.

Genio started off as a fork of [Ideam](https://github.com/AmosCaster/ideam), but we already implemented some new features:

* LSP (autocompletion, signature help, go to definition/implementation/declaration)
* Find in files
* Links to file and build errors in Build Log and Console I/O
* "Problems" tab
* Project browser
* Improved handling of Editor tabs:
  * Movable tabs
  * Close all/other
* New Editor commands:
  * Duplicate current line
  * Delete lines
  * Comment/uncomment lines
  * Switch between source and header

The editor is based on [Scintilla for Haiku](https://sourceforge.net/p/scintilla/haiku/ci/default/tree/).

We also took inspiration and code from the editor [Koder](https://github.com/KapiX/Koder).

* strongly recommended for full Genio experience (autocompletion, jump to definition, etc):

  * gcc_syslibs_devel
  * llvm17_clang

```
pkgman install gcc_syslibs_devel llvm17_clang
```

# Goals and roadmap
Genio aims to be an easy, simple yet powerful IDE for Haiku inspired by VS Code and Nova.

## Main goals

* Workspace Manager and Project folders
* Language Server Protocol via clangd: autocompletion, jump to definition, and more
* Bring the editor up-to-date and on par with other Haiku editors (Koder, Pe)
* Plug-in architecture
* Compiler error parser
* Refactor the source file and project management module by implementing a Workspace Manager and Project folders

## Configuring Clangd / LSP
See [Configuring-clangd-lsp.md](https://github.com/Genio-The-Haiku-IDE/Genio/blob/main/Configuring-clangd-lsp.md)

## Prerequirements
Genio requires libgit2 to implement Git features. The development files are available in `libgit2_devel`.
Execute `pkgman install libgit2_devel` from Terminal

## Compiling
Execute `make deps && make` in Genio's top directory.
The executable is created in `app` subdirectory.  

If you would like to try a clang++ build:

* Install `llvm_clang` hpkg from HaikuPorts
* Set `BUILD_WITH_CLANG` to `1` in `Makefile`

Genio can already be opened and built within Genio itself.
The makefile has been updated to accept the *debug* parameter:

* debug=1 - Genio is built in debug mode
* debug=0 or parameter omitted - Genio is built in release mode

## Contributions
We gladly accept contributions, especially for bug fixes. Feel free to submit PRs.
For code contributions, prefer Haiku API over posix, where applicable.
We (try to) stick to the Haiku style for code, although with a few differences.

## Branches
Currently there are various branches on the Github repository:

* main - this is the main branch, which we always try to keep in a relatively stable state
* feature/* - these branches are where we develop new features, before merging into main

## License
Genio is available under the MIT license. See [License.md](License.md).
