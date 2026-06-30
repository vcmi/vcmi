# VCMI Map Converter

`vcmimapconverter` is a command-line tool that loads a map through the
regular VCMI map service and saves it in VCMI map format.

The main use case is converting Heroes III `.h3m` maps to `.vmap` for
scripts, automated checks, or reproducible investigations without
starting the map editor UI.

## Building

The converter is built when `ENABLE_CLIENT` is enabled and
`ENABLE_MINIMAL_LIB` is disabled.

Build only the converter target:

```sh
cmake --build <build-dir> --target vcmimapconverter
```

## Usage

Convert a Heroes III map to VCMI map format:

```sh
vcmimapconverter --input path/to/input.h3m --output path/to/output.vmap
```

The short option form is also supported:

```sh
vcmimapconverter -i path/to/input.h3m -o path/to/output.vmap
```

Display command-line help:

```sh
vcmimapconverter --help
```

The output path must point to the target `.vmap` file. Existing files are
overwritten by the map saving code.

## Conversion Notes

The converter initializes the same game library and filesystem stack as
other VCMI tools. Run it from a normal VCMI build or install directory
where the engine can find its `config` and `Mods` directories.

Before saving, the converter repairs common loaded-map state that can be
valid after import but invalid for writing:

- unnamed events and rumors receive generated names;
- unflaggable ownable objects are normalized to neutral ownership;
- predefined spellbook markers are converted to real spellbook artifacts;
- town buildings not available in the loaded town type are removed;
- spell scroll objects without an artifact instance receive a spell;
- mine production fields are normalized from the mine type.

Logs are written to `VCMI_MapConverter_log.txt` in the normal user log
directory.
