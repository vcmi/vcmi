< [Documentation](../Readme.md) / Installation on macOS

**For iOS installation look here:
<https://wiki.vcmi.eu/Installation_on_iOS>**

# Step 1: Download and install VCMI

- The latest release (recommended): <https://github.com/vcmi/vcmi/releases/latest>
- Daily builds (might be unstable) 
 - Intel (x86_64) builds: <https://builds.vcmi.download/branch/develop/macOS/intel>
 - Apple Silicon (arm64) builds: <https://builds.vcmi.download/branch/develop/macOS/arm>

If the app doesn't open, right-click the app bundle - select *Open* menu item - press *Open* button.

Please report about gameplay problem on forums: [Help & Bugs](https://forum.vcmi.eu/c/international-board/help-bugs) Make sure to specify what hardware and macOS version you use.

# Step 2: Installing Heroes III data files

1.  Find a way to unpack Windows Heroes III or GOG installer. For example, use `vcmibuilder` script inside app bundle or install the game with [CrossOver](https://www.codeweavers.com/crossover) or [Wineskin](https://github.com/Gcenx/WineskinServer).
2.  Copy (or symlink) **Data**, **Maps** and **Mp3** directories from Heroes III to:`~/Library/Application\ Support/vcmi/`
