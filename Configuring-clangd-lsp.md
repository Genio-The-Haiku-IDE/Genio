# Configuring Clangd / LSP

For more advanced IDE features, Genio implements the LSP protocol. (<https://microsoft.github.io/language-server-protocol/>)
One of the most advanged C and C++ lsp server is 'clangd' included in the clang compiler suite. (more info <https://clangd.llvm.org/>)

First pre-requisite is to install these packages:

*   latest LLVM (clang) (at the time of writing is version 17)
*   gcc_syslibs_devel package

Once installed you should have the command 'clangd' ready to be used by Genio.
The clangd tool interacts with the IDE to provide all the advanced features like autocompletion, function signature information, diagnostic errors, jump to header, etc..
To obtain the best from the tool you need to configure it and your project.
At the moment this configurations must be done manually but we aim to improve Genio in order to manage these configs in a more automatic and user-friendly way.

Keep in mind that clangd tries to compile and index a set of files to provide the best information and diagnostics results.

Two main configuration point:

## The project

To let understand clangd how you want each file to be compiled, you should provide a compile database file (compile_commands.json).
The file contains, for each file of the project, the list of all the flags and include paths your compiler needs.
There are usefull tools to let you create the compile_commands.json file automatically from a building tool like make, jam or cmake.
Usually this is enough to let clangd work correctly in most cases.

### For make

The best tool so far tested is the python program called 'compiledb'. Run it as "compiledb make -Bnwk" to produce the database.

compiledb: <https://github.com/nickdiego/compiledb>  (pip3 install compiledb)

Another tool available on HaikuDepot is 'bear' (<https://github.com/rizsotto/Bear>)

### For cmake
  >
  > cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ...

or, in CMakeLists.txt

  > add set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

### For Jam (haiku version)
  >
  > jam -c
  
More info: <https://www.freelists.org/post/haiku-development/linting-and-autocomplete-How-to-generate-compile-commandsjson-alternatives> 

More info <https://clang.llvm.org/docs/JSONCompilationDatabase.html>

## The compiler itself

With the default configuration, clangd should already recognize system include paths. If you want to add more includes or finetune some settings you need to add a configuration file.
The best place to create the file is at "/boot/home/config/settings/clangd/config.yaml".
More info with all the configuration flags can be found here:  <https://clangd.llvm.org/config>
