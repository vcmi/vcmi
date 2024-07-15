# CMake options

<!-- Pure markdown doesn't support code blocks within tables -->
<table>
    <thead>
        <tr>
            <th>Option</th>
            <th>Effect</th>
        </tr>
    </thead>
    <tbody>
        <tr>
            <td>-D CMAKE_BUILD_TYPE=Debug</td>
            <td>Enables debug info and disables optimizations</td>
        </tr>
        <tr>
            <td>-D CMAKE_EXPORT_COMPILE_COMMANDS=ON</td>
            <td>Creates <code>compile_commands.json</code> for <a href="https://clangd.llvm.org/"><code>clangd</code></a> language server
            <br><br>
            For clangd to find the JSON, create a file named <code>.clangd</code>
            <pre>.
├── vcmi -> contains sources and is under git control
├── build -> contains build output, makefiles, object files,...
└── .clangd</pre>
            with the following content
            <pre>CompileFlags:<br>	CompilationDatabase: build</pre></td>
        </tr>
        <tr>
            <td>-D ENABLE_CCACHE:BOOL=ON</td>
            <td>Speeds up recompilation</td>
        </tr>
        <tr>
            <td>-G Ninja</td>
            <td>Use Ninja build system instead of make, which speeds up the build and doesn't require a <code>-j</code> flag</td>
        </tr>
    </tbody>
</table>
