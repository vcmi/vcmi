# Resource filesystem

`CResourceHandler` combines directory and archive loaders into a virtual filesystem. Resource names are case-insensitive; the last loader providing a resource overrides earlier loaders.

- Bundled paths provide game configuration and shipped data. User data is added to the initial filesystem separately.
- Initial `DATA` and `MODS` directories are scanned before configured resource mount points are built.
- Repeated directory paths within one discovery pass retain their last occurrence. `A, B, A` is scanned as `B, A`, preserving the winning source and the order of other sources.
- Paths are compared by their native strings. Symbolic links, relative and absolute spellings, and different path spellings remain separate sources.
- Each configured directory entry is processed separately. Different mount points, recursion depths, and `bundledOnly` values are not merged.

No setting is required: removing an identical scan does not change the selected resources.

## Tests

Build `vcmitest` with `ENABLE_TEST=ON` and prepare its game data as described in the platform build instructions and the test CI job. From the build's `bin` directory, run:

```sh
./vcmitest --gtest_filter='FilesystemTest.*:ResourcePathTest.*'
```

The tests cover override order, independent mounts, recursion depth, bundled sources, and lexical distinction of symbolic-link paths. The symbolic-link case is skipped if the operating system does not permit creating directory links.
