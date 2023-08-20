If you just want to play see [mod list](mod_list "wikilink").

## Creating mod

To make your own mod you need to create subdirectory in
**<data dir>/Mods/** with name that will be used as identifier for your
mod.

Main mod is file called **mod.json** and should be placed into main
folder of your mod, e.g. **Mods/myMod/mod.json**

All content of your mod should go into **Content** directory, e.g.
**Mods/myMod/Content/**. In future it will be possible to replace this
directory with single .zip archive.

Example of how directory structure of your mod may look like:

    Mods/
         myMod/
               mod.json
               Content/
                        data/    - unorganized files, mostly bitmap images (.bmp, .png, .pcx)
                        config/  - json configuration files
                        maps/    - h3m maps added or modified by mod
                        music/   - music files. Mp3 is fully supported, ogg may be added if needed
                        sounds/  - sound files, in wav format.
                        sprites/ - animation, image sets (H3 .def files or VCMI .json files)
                        video/   - video files, .bik or .smk

## Updating mod to next version of VCMI

See [Modding changelog](Modding_changelog "wikilink")

## Creating mod file

All VCMI configuration files use [JSON
format](http://en.wikipedia.org/wiki/Json) so you may want to
familiarize yourself with it first.

Mod.json is main file in your mod and must be present in any mod. This
file contains basic description of your mod, dependencies or conflicting
mods (if present), list of new content and so on.

Minimalistic version of this file:

``` javascript
{
    "name" : "My test mod",
    "description" : "My test mod that add a lot of useless stuff into the game"
}
```

See [Mod file Format](Mod_file_Format "wikilink") for its full
description.

## Overriding graphical files from Heroes III

Any graphical replacer mods fall under this category. In VCMI directory
**<mod name>/Content** acts as mod-specific game root directory. So for
example file **<mod name>/Content/Data/AISHIELD.PNG** will replace file
with same name from **H3Bitmap.lod** game archive.

Any other files can be replaced in exactly same way.

Note that replacing files from archives requires placing them into
specific location:

    H3Bitmap.lod -> Data
    H3Sprite.lod -> Sprites
    Heroes3.snd  -> Sounds
    Video.vid    -> Video

This includes archives added by expansions (e.g. **H3ab_bmp.lod** uses
same rules as **H3Bitmap.lod**)

### Replacing .def animation files

Heroes III uses custom format for storing animation: def files. These
files are used to store all in-game animations as well as for some GUI
elements like buttons and for icon sets.

These files can be replaced by another def file but in some cases
original format can't be used. This includes but not limited to:

-   Replacing one (or several) icons in set
-   Replacing animation with fully-colored 32-bit images

In VCMI these animation files can also be replaced by json description
of their content. See [Animation Format](Animation_Format "wikilink")
for full description of this format.

Example: replacing single icon

``` javascript
{
	// List of replaced images
	"images" : 
	[	  // Index of replaced frame
		{ "frame" : 0, "file" : "HPS000KN.bmp"} 
		               //name of file that will be used as replacement
	]
}
```

"High resolution main menu" mod can be used as example of file replacer
mod.

## Packaging mod into archive

For distribution it is recommended to package mod into .zip archives. To
create .zip archive you need to have file archiver like
[7zip](http://www.7-zip.org)

File structure of packaged mod should look like this

    <modname>.zip/   <- Zip archive with high compression ratio
      modname/       <- Archive contains main mod directory 
        mod.json     <- main mod file
        Content.zip/ <- Uncompressed archive with all mod data
          Data/ 
           ...       <- Identical to Content directory
          Sprites/

You can create such structure using following instructions:

-   Go to Mods/<modname>/Content directory
-   Select all files in this directory and create archive with following
    parameters:
    -   Archive name: Content.zip
    -   Format: ZIP
    -   Compression level: None/Store only
-   Move created archive into Mods/<modname> directory and remove no
    longer needed Content directory.
-   Go to Mods/ directory
-   Create archive from your mod with following parameters:
    -   Archive name: <modname>.zip
    -   Format: ZIP
    -   Compression level: Maximum

Resulting archive is recommended form for distributing mods for VCMI

## Releasing mods

Right now there are 3 ways to bring your mod to players:

-   Manual download
-   VCMI Repository
-   Private repository

### Manual download

You can upload mod into some online file hosting and add link with
description of your mod into [mod list](mod_list "wikilink").

Note: Avoid using services that require registration or remove files
after certain period of time (examples are Wikisend and 4shared).
Instead you may use any of the services listed below:

-   [MediaFire](http://mediafire.com)
-   [Dropbox](https://dropbox.com)
-   [Google Drive](https://drive.google.com)
-   Any other service that does not has aforementioned problems

### VCMI Repository

Another option is to add mod into VCMI repository. This will allow
players to install mods directly from VCMI Launcher without visiting any
3rd-party sites.

Check for more details in [Mods repository](Mods_repository "wikilink").

### Private repository

It it also possible to create your own repository. To do this you need
to own your own server capable of file hosting or use file service that
provide direct download links (e.g. Dropbox with enabled public
directory).

Providing own repository allows you to deliver any new mods or updates
almost instantly and on the same level of integration with VCMI as mods
from VCMI repository.

To create empty repository you need to:

-   Create directory that will contain repository
-   Create file named "repository.json" in it

To add mods into such repository you need to:

-   Copy packaged archive <modname>.zip into repository directory
-   Copy mod information from mod.json into repository.json
-   Add two extra fields about this mod into repository.json:
    -   "download" - public link that can be used to download the mod,
        including <http://> prefix
    -   "size" - size of mod, in kilobytes. VCMI will use this number to
        inform player on size of the mod.

Example on how mod entry should look in repository.json

``` javascript
{
  ...
  "exampleMod" // ID of the mod, lowercase version of mod directory name
  {
    "name" : "My test mod",
    "description" : "My test mod that add a lot of useless stuff into the game",
    "author" : "Anonymous",
    "contact" : "http://example.com",
    "modType" : "Graphical",
    "depends" :
    [
        "baseMod"
    ],

    "download" : "http://example.com/vcmi/repository/exampleMod.zip",
    "size" : 1234 //size, in kilobytes

    //Note that entries that refer to files, e.g. "heroes". "creatures", "artifacts" and such
    //are not necessary in repository.json and therefore can be removed
  },
  ...
}
```

When repository is ready you can share public link to repository.json
with players. New repositories can added to vcmi launcher from settings
tab.

#### Dropbox: enabling public directory

New accounts create on Dropbox no longer have Public directory enabled
by default. You can enable it using this
[link](https://www.dropbox.com/enable_public_folder)

This will give you directory named "Public" as well as option "get
public link" for all files inside this directory.