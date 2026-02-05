# Installation on Windows

## Prerequisites

- Windows 7 SP1 or newer is required.
- Windows Vista and Windows XP are **no longer supported**.

## Step 1: Download and install VCMI

Download and install VCMI using one of the following options:

- **Latest release (recommended):**  
  https://github.com/vcmi/vcmi/releases/latest

- **Daily builds (unstable, for testing):**  
  https://builds.vcmi.download/branch/

Download the installer and run it. Follow the installer instructions.

After installation, start **VCMI Launcher**.

## Step 2: Obtain Heroes III data files

VCMI requires original *Heroes of Might and Magic III* data files.

### Option A (recommended): GOG offline installer

1. Download the **offline backup installer** from GOG.com.
2. The installer consists of two files:
   - `.exe`
   - `.bin`
3. Make sure both files are downloaded.

![GoG-Installer](images/gog_offline_installer.png)

### Option B: Existing installation

If you already have Heroes III installed (Complete or Shadow of Death edition), you can reuse that installation.

## Step 3: Import Heroes III data into VCMI

Start **VCMI Launcher**.

1. Select your preferred language.
2. Choose one of the following methods:

### A) Import from GOG installer

- Select the option to import GOG files.
- First select the `.exe` file. The matching `.bin` file will be located automatically if the correct installer is selected.
- Launcher will automatically extract required files.

### B) Use existing Heroes III installation

Launcher may automatically detect your Heroes III installation.

If detection fails:

1. Locate your Heroes III installation folder.
2. Copy these directories:

   - `Data`
   - `Maps`
   - `Mp3`

   into: "%USERPROFILE%\Documents\My Games\vcmi"


3. Return to Launcher and press **Scan again**.

## Step 4: Finish setup in Launcher

Follow the instructions shown in the Launcher to complete installation and optionally download recommended mods.

VCMI is now ready to play.
