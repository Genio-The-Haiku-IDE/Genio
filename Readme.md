Genio
================
![Screenshot](https://raw.github.com/AmosCaster/ideam/master/data/screenshot/screenshot-0.7.4-eng.png)
    screenshot-0.7.4-eng.png

Introducing
----------------

Genio is a fork of Ideam an IDE for [Haiku](https://www.haiku-os.org).
The editor is based on [Scintilla for Haiku](https://sourceforge.net/p/scintilla/haiku/ci/default/tree/).  

Compiling
----------------

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
