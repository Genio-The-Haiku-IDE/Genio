Genio
================
![Screenshot](https://github.com/nexus6-haiku/Genio/blob/main/data/screenshot/Genio.png)
    Genio.png

Introducing
------------------
Genio is a fork of Ideam an IDE for [Haiku](https://www.haiku-os.org).
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
Currently there are 3 branches on the Github repository:
* main - is functionally equivalent to the latest version of Ideam (v.0.7.9) at the time we forked it, except it is fully rebranded
* workspace - this is the branch where the development on the workspace manager and project folders happen
* experimental/lsp-client - this is the branch where the development of the Language Server Protocol and related features happen (e.g. autocompletion)
	
Contributions
------------------
Genio is in a very early stage. There is currently no difference between Ideam and the main branch except from a branding perspective.
We are not ready for prime time and we will keep the branches separate until we are.
We do not accept PRs at the moment but if you want to contribute, please contact us here on GitHub.

Compiling
------------------
* Install `scintilla` and `scintilla_devel` hpkgs from HaikuPorts (`scintilla_x86` versions for x86_gcc2)
* Clone `genio` sources
* Execute `make` in Genio's top directory  
The executable is created in `app` subdirectory.  

If you would like to try a clang++ build:
* Install `llvm_clang` hpkg from HaikuPorts
* Set `BUILD_WITH_CLANG` to `1` in `Makefile`

License
----------------
Genio is available under the MIT license. See [License.md](License.md).
