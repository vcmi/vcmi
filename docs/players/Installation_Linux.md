< [Documentation](../Readme.md) / Installation on Linux

VCMI requires data from original Heroes 3: Shadow of Death or Complete editions. Data from native Linux version made by LOKI will not work.

# Step 1: Binaries installation

## Ubuntu

### Latest stable build from PPA (recommended)

Up-to-date releases can be found in our PPA here: <https://launchpad.net/~vcmi/+archive/ubuntu/ppa>

To install VCMI from PPA use:
```
    sudo apt-add-repository ppa:vcmi/ppa
    sudo apt update
    sudo apt install vcmi
```

### Unstable testing build from PPA

We also provide latest, unstable builds mostly suitable for testing here: <https://launchpad.net/~vcmi/+archive/ubuntu/vcmi-latest>

In order to install from this PPA use:
```
    sudo add-apt-repository ppa:vcmi/vcmi-latest
    sudo apt update
    sudo apt install vcmi
```
### From Ubuntu repository

VCMI stable builds available in "multiverse" repository. Learn how to enable it in [Ubuntu wiki](https://help.ubuntu.com/community/Repositories/Ubuntu).
Once enabled, you can install VCMI using Ubuntu Store or in terminal using following commands:
```
    sudo apt update
    sudo apt install vcmi
```
Note that version available in Ubuntu is outdated. Install via PPA is preferred.

## Debian

Stable VCMI version is available in "contrib" repository. Learn how to enable it in [Debian wiki](https://wiki.debian.org/SourcesList).
To install VCMI from repository:
```
    sudo apt-get update
    sudo apt-get install vcmi
```
## Flatpak (distribution-agnostic)

Latest public release build can be installed via Flatpak.

Depending on your distribution, you may need to install flatpak itself. You can find guide for your distribution here: <https://www.flatpak.org/setup/>
Once you have flatpak, you can install VCMI package which can be found here: <https://flathub.org/apps/details/eu.vcmi.VCMI>

## Other distributions

For other distributions, VCMI can be installed from 3rd-party repositories listed below. Note that these repositories are not supported by vcmi team and may not be up to date.

-   Archlinux [vcmi](https://aur.archlinux.org/packages/vcmi/) [vcmi-git](https://aur.archlinux.org/packages/vcmi-git/)
-   openSUSE [1 Click Install](https://software.opensuse.org/download.html?project=games&package=vcmi)

If you are interested in providing builds for other distributions, please let us know.

## Compiling from source

Please check following developer guide: [How to build VCMI (Linux)](How_to_build_VCMI_(Linux) "wikilink")

# Step 2: Installing Heroes III data files

To install VCMI you will need Heroes III: Shadow of Death or Complete edition.

## Install data using vcmibuilder script (recommended for non-Flatpak installs)

To install Heroes 3 data using automated script you need any of:

- Offline Installer downloaded from gog.com (both .exe and .bin files are required)
- Directory with preinstalled game 
- One or two CD's or CD images

Run the script using options appropriate to your input files:
```
vcmibuilder --cd1 /path/to/iso/or/cd --cd2 /path/to/second/cd
vcmibuilder --gog /path/to/gog.com/installer.exe
vcmibuilder --data /path/to/h3/data
```
You should use only one of these commands.

On flatpak install, it's also possible to run the script, but any path seems to be interpreted from within the Flatpak sandbox:

```
flatpak run --command=vcmibuilder eu.vcmi.VCMI --data /path/to/h3/data`
```

## Install data using gog.com offline installer

Download both files for the "offline backup game installers" and extract them using innoextract tool
```
innoextract --output-dir=~/Downloads/HoMM3 "setup_heroes_of_might_and_magic_3_complete_4.0_(28740).exe"
```
(note that installer file name might be different)

Once innoextract completes, start VCMI Launcher and choose to copy existing files. Select the ~/Downloads/HoMM3 directory. Once copy is complete, you can delete both offline installer files as well as ~/Downloads/HoMM3.

## Install manually using existing Heroes III data

Copy "Data", "Maps" and "Mp3" from Heroes III to `$HOME/.local/share/vcmi/`
Or, in case of flatpak install to `$HOME/.var/app/eu.vcmi.VCMI/data/vcmi/`
On some distributions $XDG_DATA_HOME could differ so instead you may need to use: `$XDG_DATA_HOME/vcmi/`

# Step 3: Launching game

VCMI should be available via desktop environment menu or launcher (Games/Strategy/VCMI)

To start the game type in console: `vcmilauncher`
Or, to start game directly avoiding Launcher: `vcmiclient`

# Reporting bugs

Please report any issues with packages according to [Bug Reporting Guidelines](Bug_Reporting_Guidelines.md)