**For iOS installation look here:
<https://wiki.vcmi.eu/Installation_on_iOS>**

# Download and install VCMI

-   The latest release (recommended):
    <https://github.com/vcmi/vcmi/releases/latest>
-   Daily builds (might be unstable)
    -   Intel (x86_64) builds:
        <https://builds.vcmi.download/branch/develop/macOS/intel>
    -   Apple Silicon (arm64) builds:
        <https://builds.vcmi.download/branch/develop/macOS/arm>

If the app doesn't open, right-click the app bundle - select *Open* menu
item - press *Open* button.

Please report about gameplay problem on forums: [Help &
Bugs](https://forum.vcmi.eu/c/international-board/help-bugs)

If your problem is Mac-specific: DMG corrupted or if it doesn't start
please use following topic:

-   [VCMI for macOS](https://forum.vcmi.eu/t/macos-builds/550)

Make sure to specify what hardware and macOS version you use.

# Installing Heroes III data files

1.  Find a way to unpack Windows Heroes III or GOG installer. For
    example, use `vcmibuilder` script inside app bundle or install the
    game with [CrossOver](https://www.codeweavers.com/crossover) /
    [Wineskin](https://github.com/Gcenx/WineskinServer).
2.  Copy (or symlink) **Data**, **Maps** and **Mp3** directories from
    Heroes III to:

<!-- -->

    ~/Library/Application\ Support/vcmi/

# Connect to the mod repository (optional)

-   If that's your first installation, connection to the mod repository
    will be configured automatically, you'll see mods available to
    install from VCMI launcher
    -   We recommend you to install VCMI extras to support different
        screen resolutions and active various helpful UI tools
    -   If you don't see mods available to install, go to settings tab
        of the launcher and make sure that you have correct link in
        "repositories" text field:
        -   <https://raw.githubusercontent.com/vcmi/vcmi-mods-repository/de