This article will introduce you to new VCMI mod repository system and
explain how new mods can be added there.

# How mod repository work

## Where files are hosted

Mods list hosted under main VCMI organization:
[vcmi-mods-repository](https://github.com/vcmi/vcmi-mods-repository).

Each mod hosted in it's own repository under separate organization
[vcmi-mods](https://github.com/vcmi-mods). This way if engine become
more popular in future we can create separate teams for each mod and
accept as many people as needed.

## Why Git / GitHub?

It's solve a lot of problems:

-   Engine developers get control over all mods and can easily update
    them without adding extra burden for modders / mod maintainers.
-   With tools such as [GitHub Desktop](https://desktop.github.com/)
    it's easy for non-programmers to contribute.
-   Forward and backward compatibility. Stable releases of game use
    compatible version of mods while users of daily builds will be able
    to test mods supporting bleeding edge features.
-   Tracking of changes for repository and mods. It's not big deal now,
    but once we have scripting it's will be important to keep control
    over what code included in mods.
-   GitHub also create ZIP archives for us so mods will be stored
    uncompressed and version can be identified by commit hash.

## On backward compatibility

Our mod list in vcmi-mods-repository had "develop" as primary branch.
Daily builds of VCMI use mod list file from this branch.

Once VCMI get stable release there will be branching into "1.0.0",
"1.1.0", etc. Launcher of released version will request mod list for
particular version.

Same way we can also create special stable branch for every mod under
"vcmi-mods" organization umbrella once new stable version is released.
So this way it's will be easier to maintain two versions of same mod:
for stable and latest version.

# Getting mod into repository

## Getting into vcmi-mods organization

Before your mod can be accepted into official mod list you need to get
it into repository under "vcmi-mods" organization umbrella. To do this
contact one of mod repository maintainers. If needed you can get own
team within "vcmi-mods" organization.

Link to our mod will looks like that:

    https://github.com/vcmi-mods/adventure-ai-trace

## Rules of repository

### Allowed name for mod identifier

For sanity reasons mod identifier must only contain lower-case English
characters, numbers and hyphens.

    my-mod-name
    2000-new-maps

Sub-mods can be named as you like, but we strongly encourage everyone to
use proper identifiers for them as well.

### Rewriting History

Once you submitted certain commit into official mod list you are not
allowed to rewrite history before that commit. This way we can make sure
that VCMI launcher will always be able to download older version of any
mod.

Branches such as "develop" or stable branches like "1.0.0" should be
marked as protected on GitHub.

## Submitting mods to repository

Once mod ready for general public maintainer to make PR to
[vcmi-mods-repository](https://github.com/vcmi/vcmi-mods-repository).

## Requirements

Right now main requirements for a mod to be accepted into VCMI mods list
are:

-   Mod must be complete. For work-in-progress mods it is better to use
    other way of distribution.
-   Mod must met some basic quality requirements. Having high-quality
    content is always preferable.
-   Mod must not contain any errors detectable by validation (console
    message you may see during loading)
-   Music files must be in Ogg/Vorbis format (\*.ogg extension)