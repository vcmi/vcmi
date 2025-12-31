# Installation iOS

You can run VCMI on iOS 12.0 and later, all devices are supported. If you wish to run on iOS 10 or 11, you should build from source, see [How to build VCMI (iOS)](../developers/Building_iOS.md).

## Step 1: Download and install VCMI

The easiest and recommended way to install on a non-jailbroken device is to install the [AltStore Classic](https://altstore.io/) or [Sideloadly](https://sideloadly.io/). We will use AltStore as an example below. Using this method means the VCMI certificate is auto-signed automatically.

a) Install AltStore

Follow the official instructions for your operating system:

- [AltStore Windows Classic](https://faq.altstore.io/altstore-classic/how-to-install-altstore-windows)
- [AltStore macOS](https://faq.altstore.io/altstore-classic/how-to-install-altstore-macos)

**Troubleshooting Tip:** If you encounter issues enabling "Sync with this iOS device over Wi-Fi," click the rectangular icon under "Account" in iTunes. See the example below:

![iTunes](https://vcmi.eu/players/images/itunes.jpg)

b) Download the VCMI-iOS.ipa file on your iOS device directly from the [latest releases](https://github.com/vcmi/vcmi/releases/latest).

c) Install the IPA File

To install the IPA file, choose one of the following methods:

- In AltStore:
  1. Go to **My Apps**.
  2. Press the **+** button in the top-left corner.
  3. Select `VCMI-iOS.ipa` to install.
- Alternatively, drag and drop the IPA file into your iOS device using iTunes.

## Step 2: Installing Heroes III data files

d) If you purchased HoMM3 on [GOG](https://www.gog.com/de/game/heroes_of_might_and_magic_3_complete_edition), you can download the required files directly on your device.

gog.com download page:
![GoG-Installer](images/gog_offline_installer.png)

e) Launch the VCMI app on your device. The launcher will prompt you to upload two files to complete the installation.

f) First, select the **.bin** file, then select the **.exe** file.

![image](https://github.com/user-attachments/assets/9a02a76f-bb2e-45ad-b2fe-ffd97112021f)

g) The process may take a few seconds. Please be patient.

## Step 3: Configuration settings

e) After installing VCMI, open the launcher and go to **Settings** in the left sidebar. Adjust the following video settings for the best experience:

- Set **Reserved Screen Area** to **0%**.
- Increase **Interface Scaling** to the maximum value suitable for your device (e.g., 250% for an 11" iPad Air).
- Set **Upscaling filter** to **xBRZ x2** if not selected by default. This will enable the VCMI HD upscaling that is similar in effect to HOMM3 HD mod. Higher xBRZ selections may tank your iOS device perfomance without providing a tangible benefit.

These settings will eliminate black bars and enable a full-screen VCMI experience. Enjoy!

## Alternative Step 1: Download and Install VCMI

- **Latest Release (Recommended):** <https://github.com/vcmi/vcmi/releases/latest>
- **Daily Builds:** <https://builds.vcmi.download/branch/develop/iOS/>

To run on a non-jailbroken device, you need to sign the IPA file. Here are your options:

- **For iOS 14.0–15.4.1:** Use [TrollStore](https://github.com/opa334/TrollStore).
- **Sign with a Tool:** Use the [iOS App Signer](https://dantheman827.github.io/ios-app-signer/) and refer to this [Kodi guide](https://forum.kodi.tv/showthread.php?tid=245978) (the process is similar).
- **Create Signing Assets on macOS:** Follow [this guide](https://github.com/kambala-decapitator/xcode-auto-signing-assets). Replace `your.bundle.id` with a unique identifier, such as `com.MY-NAME.vcmi`.
- **Sign on Any OS:** Use [Rust](https://github.com/indygreg/PyOxidizer/tree/main/tugger-code-signing) or an [alternative project in C++](https://github.com/zhlynn/zsign). Note: You will need signing assets (a private key and provisioning profile).

### Installing the IPA File

- In AltStore: Go to **My Apps**, press the **+** button, and select `VCMI-iOS.ipa` to install.
- Using iTunes: Drag and drop the IPA file into your iOS device.

Alternatively, use Xcode or Apple Configurator (available for free on the Mac App Store). Apple Configurator also allows command-line installations. Here’s an example:

```sh
/Applications/Apple\ Configurator.app/Contents/MacOS/cfgutil install-app ~/Desktop/vcmi.ipa
```

## Alternative Step 2: Installing Heroes III Data Files

**Note:** To save space and time, you can skip downloading the `VIDEO.VID` file from the **Data** folder if you don’t need in-game videos. You can also skip the **Mp3** directory.

### Step 2.a: Using Finder or Windows Explorer

1. Upload the following directories to your device:
   - **Data**
   - **Maps**
   - **Mp3** (optional)

2. Use Finder (macOS) or iTunes (Windows/macOS 10.14 or earlier). Mods can also be added by uploading the **Mods** directory.

3. Follow [Apple’s guide](https://support.apple.com/en-us/HT210598) to place the files into the VCMI app. **Finder does not display copy progress in old macOS versions**, so wait about 10 minutes for the process to finish.

### Step 2.b: Using the Files App

If the data files are on your iOS device (e.g., in a shared folder), copy them directly using the **Files** app.

1. Place the **Data**, **Maps**, and **Mp3** folders into the VCMI application folder.
2. The VCMI app folder will appear alongside other app folders in the **Files** app.

### Step 2.c: Installing data files with Xcode on macOS

You can also upload files with Xcode. You need to prepare "container" for that.

1. Connect your device to your Mac
2. Start Xcode
3. Open Devices and Simulators window: Cmd+Shift+2 or Menu - Window - Devices and Simulators
4. Select your device
5. Select VCMI
6. In the bottom find "three dots" or "cogwheel" button (it should be next to + - buttons) - click it - select Download Container...
7. Place the game directories inside the downloaded container -> AppData -> Documents
8. Click the "three dots" / "cogwheel" button in Xcode again -> Replace Container... -> select the downloaded container
9. Wait until Xcode finishes copying, progress is visible (although it might be "indefinite")

## Game controls

- Tap: left click
- Tap and hold (long press): right click
- Tap in the bottom area (status bar): activate chat/console in the game

You can start game directly (avoiding the launcher) by changing setting in iOS Settings app - VCMI.

## Troubleshooting: Keeping your Alternative Store updated

If iTunes does not connect to your device over WiFi despite enabling the Sync over WiFi option, every 7 days you need to open Alt Store or the alternative. Clicking Update All or click Update App on both VCMI and the store separately works. Do the following on PC:

1. Search for `cmd`. Right click on it and ‘Run as administrator’.
2. Copy the command below. It stops the ‘Apple Mobile Device service’.

```sh
net stop "Apple Mobile Device Service"
```

3. Copy the command below and restart the ‘Apple Mobile Device Service’.

```sh
net start "Apple Mobile Device Service"
```

In case you don't update the store in the alloted time and it expires, it won't load. Reloading the store is easy enough. Simply

- Connect you iOS device to your PC/MAC
- Complete step 9 of the AltStore Manual (trust your Apple ID) again
- Install Altstore on your device
- Update VCMI and play as normal

![image](https://github.com/user-attachments/assets/836c9a2e-7830-46eb-ab54-ef9dbb34c8f4)

You do not need to delete any files on your PC/MAC/iOS device or reboot anything to revive the Alt store or VCMI.

## Reporting bugs

- Please report about gameplay problem on forums: [Help & Bugs](https://forum.vcmi.eu/c/international-board/help-bugs)
- Please report iOS-specific issues in [this forum thread](https://forum.vcmi.eu/t/ios-port/820) or (better) at [GitHub](https://github.com/vcmi/vcmi/issues) with **iOS** label
