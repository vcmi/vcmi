# File Formats

This page describes which file formats are supported by vcmi. 

In most cases, VCMI supports formats that were supported by Heroes III, with addition of new formats that are more convenient to use without specialized tools. See categories below for more details on specific formats

### Images

For images VCMI supports:
- png. Recommended for usage in mods
- bmp. While this format is supported, bmp images have no compressions leading to large file sizes
- pcx (h3 version). Note that this is format that is specific to Heroes III and has nothing in common with widely known .pcx format. Files in this format generally can only be found inside of .lod archive of Heroes III and are usually extracted as .bmp files

Transparency support:
VCMI supports transparency (alpha) channel, both in png and in bmp images. There may be cases where transparency is not fully supported. If you discover such cases, please report them.

For performance reasons, please use alpha channel only in places where transparency is actually required and remove alpha channel from image othervice

Palette support:
TODO: describe how palettes work in vcmi

### Animations

For animations VCMI supports .def format from Heroes III as well as alternative json-based. See [Animation Format](Animation_Format.md) for more details

### Sounds

For sounds VCMI currently supports:
- .ogg/vorbis format - preferred for mods. Unlike wav, vorbis uses compression which may cause some data loss, however even 128kbit is generally undistinguishable from lossless formats
- .wav format. This is format used by H3. It is supported by vcmi, but it may result in large file sizes (and as result - large mods)

Generally, VCMI will support any audio parameters, however you might want to use high-bitrate versions, such as 44100 Hz or 48000 Hz, 32 bit, 1 or 2 channels

Support for additional formats, such as ogg/opus or flac may be added in future

### Music

For music VCMI currently supports:
- .ogg/vorbis format - preferred for mods. Generally offers better quality and lower sizes compared to mp3
- .mp3 format. This is format used by H3

Support for additional formats, such as ogg/opus may be added in future

### Video

Starting from VCMI 1.6, following video container formats are supported by VCMI:

- .bik - one of the formats used by Heroes III
- .smk - one of the formats used by Heroes III. Note that these videos generally have lower quality and are only used as fallback if no other formats are found
- .ogv - format used by Heroes III: HD Edition
- .webm - modern, free format that is recommended for modding.

Supported video codecs:
- bink and smacker - formats used by Heroes III, should be used only to avoid re-encoding
- theora - used by Heroes III: HD Edition
- vp8 - modern format with way better compression compared to formats used by Heroes III
- vp9 - recommended, this format is improvement of vp9 format and should be used as a default option

Support for av1 video codec is likely to be added in future.

Supported audio codecs:
- binkaudio and smackaud - formats used by Heroes III
- vorbis - modern format with good compression level
- opus - recommended, improvement over vorbis. Any bitrate is supported, with 128 kbit probably being the best option

### Json

For most of configuration files, VCMI uses [JSON format](http://en.wikipedia.org/wiki/Json) with some extensions from [JSON5](https://spec.json5.org/) format, such as comments.

### Maps

TODO: describe

### Campaigns

TODO: describe

### Map Templates

TODO: describe

### Archives

TODO: describe

### Txt

TODO: describe
