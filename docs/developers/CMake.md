# CMake options

* `-D CMAKE_BUILD_TYPE=Debug`
  * Enables debug info and disables optimizations
* `-D CMAKE_EXPORT_COMPILE_COMMANDS=ON`
  * Creates `compile_commands.json` for [clangd](https://clangd.llvm.org/) language server. For clangd to find the JSON, create a file named `.clangd` with this content
     ```text
    CompileFlags:
    CompilationDatabase: build
    ```
    and place it here:
    ```text
    .
    ├── vcmi -> contains sources and is under git control
    ├── build -> contains build output, makefiles, object files,...
    └── .clangd
    ```
* `-D ENABLE_CCACHE:BOOL=ON`
  * Speeds up recompilation
* `-G Ninja`
  * Use Ninja build system instead of Make, which speeds up the build and doesn't require a `-j` flag
* `-D VCMI_UPDATE_CHANNEL=none`
  * Update channel of the build: `stable` (default), `beta`, `develop` or `none`. The launcher checks for updates only in `stable` builds. CI sets it from the branch; `none` disables the check
