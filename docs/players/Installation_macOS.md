# Installation macOS

## Step 1: Download and install VCMI

- The latest release (recommended):
  - manually: <https://github.com/vcmi/vcmi/releases/latest>
  - via Homebrew: `brew install --cask --no-quarantine vcmi/vcmi/vcmi`
- Daily builds (might be unstable)
  - Intel (x86_64) builds: <https://builds.vcmi.download/branch/develop/macOS/intel>
  - Apple Silicon (arm64) builds: <https://builds.vcmi.download/branch/develop/macOS/arm>

If the app doesn't open, right-click the app bundle - select *Open* menu item - press *Open* button. You may also need to allow running it in System Settings - Privacy & Security.

Please report about gameplay problem on forums: [Help & Bugs](https://forum.vcmi.eu/c/international-board/help-bugs) Make sure to specify what hardware and macOS version you use.

## Step 2: Installing Heroes III data files

### Step 2.a: Installing data files with GOG offline installer

If you bought HoMM3 on [GOG](https://www.gog.com/de/game/heroes_of_might_and_magic_3_complete_edition), you can download the files directly from the browser and install them in the launcher. Select the .bin file first, then the .exe file. This may take a few seconds. Please be patient.

### Step 2.b: Installing by the classic way

1. Find a way to unpack Windows Heroes III or GOG installer. For example, use `vcmibuilder` script inside app bundle or install the game with [CrossOver](https://www.codeweavers.com/crossover) or [Kegworks](https://github.com/Kegworks-App/Kegworks).
2. Place or symlink **Data**, **Maps** and **Mp3** directories from Heroes III to:`~/Library/Application\ Support/vcmi/`
