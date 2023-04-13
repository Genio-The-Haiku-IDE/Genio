=======
Genio
================================================================================
![Screenshot](https://github.com/nexus6-haiku/Genio/blob/main/data/screenshot/Genio.png)
    Genio.png

Introduction
--------------------------------------------------------------------------------
Genio is an IDE for the Haiku operating system.
Despite being in an early stage, Genio can already be used to develop and build applications.
In fact, the Genio team uses it as the main development tool to develop and build Genio.

Genio started off as a fork of Ideam an IDE for [Haiku](https://www.haiku-os.org)

The editor is based on [Scintilla for Haiku](https://sourceforge.net/p/scintilla/haiku/ci/default/tree/).  


Goals and roadmap
------------------
Genio aims to be an easy, simple yet powerful IDE for Haiku inspired by VS Code and Nova.

*Main goals*
* Workspace Manager and Project folders
* Language Server Protocol via clangd: autocompletion, jump to definition, and more
* Bring the editor up-to-date and on par with other Haiku editors (Koder, Pe)
* Plug-in architecture
* Compiler error parser

Top priority goals after the first initial commit include:

*Source file and project management*
* refactor the source file and project management module by implementing a Workspace Manager and Project folders
* remove support for project files outside of source tree (.idmpro)
	
*Editor*
* implement the Language Server Protocol via clangd to get autocompletion and jump to definition/instance

*Fixes and improvements*
* Bug fixing (find/replace focus capture)
* Find in files
* Better GIT support

Branches
------------------
Currently there are 2 branches on the Github repository:
* main - this is the main branch, which we always try to keep in a relatively stable state
* experimental/ProjectTreeView - this is the branch where the development of the new Project related stuff happens
	
Contributions
------------------
We are not ready for prime time and we will keep the branches separate until we are.
We do not accept PRs at the moment but if you want to contribute, please contact us here on GitHub.

Compiling
------------------
* ensure you have installed "llvm12_clang and llvm12_libs" packages

* cd into src/scintilla/haiku and 'make'
* cd into src/lexilla and 'make'


* Execute `make` in Genio's top directory  
The executable is created in `app` subdirectory.  

If you would like to try a clang++ build:
* Install `llvm_clang` hpkg from HaikuPorts
* Set `BUILD_WITH_CLANG` to `1` in `Makefile`


As of v.0.2.0 Genio can now be opened and built within Genio itself (.genio config file has been added to the repository).
The makefile has been updated to accept the *debug* parameter:

* debug=1 - Genio is built in debug mode
* debug=0 or parameter omitted - Genio is built in release mode

Currently debug build with clang is broken and Debugger does not detect debug info
We recommend to build Genio with gcc if you want to contribute to it and debug, otherwise please use clang.


License
----------------
Genio is available under the MIT license. See [License.md](License.md).

