Genio
================================================================================
![Screenshot](https://github.com/nexus6-haiku/Genio/blob/main/data/screenshot/Genio.png)
    Genio.png

Introduction
--------------------------------------------------------------------------------
Genio is a fork of Ideam an IDE for [Haiku](https://www.haiku-os.org).
The editor is based on [Scintilla for Haiku](https://sourceforge.net/p/scintilla/haiku/ci/default/tree/).  

Goals and roadmap
--------------------------------------------------------------------------------
Genio aims to be an easy, simple yet powerful IDE for Haiku inspired by VS Code and Nova.

*Main goals*
* Workspace Manager and Project folders
* Language Server Protocol via clangd: autocompletion, jump to definition, and more
* Bring the editor up-to-date and on par with other Haiku editors (Koder, Pe)
* Plug-in architecture
* Compiler error parser

*Fixes and improvements*
* Bug fixing (find/replace focus capture)
* Find in files
* Better GIT support

Recent changes as of v.0.2.0 (commit #479adf5)
--------------------------------------------------------------------------------

* Removed the old project management system (.idmpro)
* Implemented the concept of Project Folder, i.e. a folder on file system.
* Show in Tracker
* Open Terminal pointing to the selected folder out of the source tree
* "Find in files" early implementation through grep
* Open files not supported by Genio (i.e. images, binary, etc.) with default application
* Log class
* Better and more consistent way of handling cut, copy and paste
* Console I/O file matcher - File names or path can now be clicked and the file will be opened in the Editor.
	Line numbers are supported that allows to click on a compile error and open the file with cursor positioned on that line
* Other minor bug fixes and improvements (please see commit log)

Build
--------------------------------------------------------------------------------
* Install `scintilla` and `scintilla_devel` hpkgs from HaikuPorts (`scintilla_x86` versions for x86_gcc2)
* Clone `genio` sources
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
