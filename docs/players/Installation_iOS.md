# Installation iOS

You can run VCMI on iOS 12.0 and later, all devices are supported. If you wish to run on iOS 10 or 11, you should build from source, see [How to build VCMI (iOS)](../developers/Building_iOS.md).

## Step 1: Download and install VCMI

The easiest and recommended way to install on a non-jailbroken device is to install the [AltStore Classic](https://altstore.io/) or [Sideloadly](https://sideloadly.io/). We will use AltStore as an example below. Using this method means the VCMI certificate is auto-signed automatically.

i) Use [AltStore Windows](https://faq.altstore.io/altstore-classic/how-to-install-altstore-windows) or [AltStore macOS](https://faq.altstore.io/altstore-classic/how-to-install-altstore-macos) instructions to install the store depending on the operating system you are using. 

If you're having trouble enabling "sync with this iOS device over Wi-Fi" press on the rectangular shape below "Account". Windows example from iTunes shown below:

![iTunes](images/itunes.jpg)

ii) Download the VCMI-iOS.ipa file on your iOS device directly from the [latest releases](https://github.com/vcmi/vcmi/releases/latest).

iii) To install the .ipa file on your device do one of the following:

- In AltStore go to >My Apps > press + in the top left corner. Select VCMI-iOS.ipa to install,
- or drag and drop the .ipa file into your iOS device in iTunes


## Step 2: Installing Heroes III data files

If you bought HoMM3 on [GOG](https://www.gog.com/de/game/heroes_of_might_and_magic_3_complete_edition), you can download the files directly from the browser in the device. 

Launch VCMI app on the device and the launcher will prompt two files to complete the installation. Select the **.bin** file first, then the **.exe** file. This may take a few seconds. Please be patient. 


## Step 3: Configuration settings
Once you have installed VCMI and have the launcher opened, select Settings on the left bar. The following Video settings are recommended:

- Lower reserved screen area to zero.
- Increase interface Scaling to maximum. This number will depend on your device. For 11" iPad Air it was at 273% as an example

Together, the two options should eliminate black bars and enable full screen VCMI experience. Enjoy!

## Alternative Step 1: Download and install VCMI

- The latest release (recommended): <https://github.com/vcmi/vcmi/releases/latest>
- Daily builds: <https://builds.vcmi.download/branch/develop/iOS/>

To run on a non-jailbroken device you need to sign the IPA file, you have the following aternative options:

- if you're on iOS 14.0-15.4.1, you can try <https://github.com/opa334/TrollStore>. 
- Get signer tool [here](https://dantheman827.github.io/ios-app-signer/) and a guide [here](https://forum.kodi.tv/showthread.php?tid=245978) (it's for Kodi, but the logic is the same). Signing with this app can only be done on macOS.
- [Create signing assets on macOS from terminal](https://github.com/kambala-decapitator/xcode-auto-signing-assets). In the command replace `your.bundle.id` with something like `com.MY-NAME.vcmi`. After that use the above signer tool.
- [Sign from any OS (Rust)](https://github.com/indygreg/PyOxidizer/tree/main/tugger-code-signing) / [alternative project (C++)](https://github.com/zhlynn/zsign). You'd still need to find a way to create signing assets (private key and provisioning profile) though.

The easiest way to install the ipa on your device is to do one of the following:

- In AltStore go to >My Apps > press + in the top left corner. Select VCMI-iOS.ipa to install or
- Drag and drop the .ipa file into your iOS device in iTunes

Alternatively, to install the signed ipa on your device, you can use Xcode or Apple Configurator (available on the Mac App Store for free). The latter also allows installing ipa from the command line, here's an example that assumes you have only 1 device connected to your Mac and the signed ipa is on your desktop:

    /Applications/Apple\ Configurator.app/Contents/MacOS/cfgutil install-app ~/Desktop/vcmi.ipa

## Alternative Step 2: Installing Heroes III data files

Note: if you don't need in-game videos, you can omit downloading/copying file VIDEO.VID from the Data folder - it will save your time and space. The same applies to the Mp3 directory.

### Step 2.a: Installing data files with Finder or Windows explorer

To play the game, you need to upload HoMM3 data files - **Data**, **Maps** and **Mp3** directories - to the device. Use Finder (or iTunes, if you're on Windows or your macOS is 10.14 or earlier) for that. You can also add various mods by uploading **Mods** directory. Follow [official Apple guide](https://support.apple.com/en-us/HT210598) and place files into VCMI app. Unfortunately, Finder doesn't display copy progress, give it about 10 minutes to finish.

### Step 2.b: Installing data files using iOS device only

If you have data somewhere on device or in shared folder or you have downloaded it, you can copy it directly on your iPhone/iPad using Files application.

Place **Data**, **Maps** and **Mp3** folders into vcmi application - it will be visible in Files along with other applications' folders.

### Step 2.c: Installing data files with Xcode on macOS

You can also upload files with Xcode. You need to prepare "container" for that.

1. Connect your device to your Mac
2. Start Xcode
3. Open Devices and Simulators window: Cmd+Shift+2 or Menu - Window - Devices and Simulators
4. Select your device
5. Select VCMI
6. In the bottom find "three dots" or "cogwheel" button (it should be next to + - buttons) - click it - select Download Container... 
7. Place the game directories inside the downloaded container - AppData - Documents
8. Click the "three dots" / "cogwheel" button in Xcode again - Replace Container... - select the downloaded container
9. Wait until Xcode finishes copying, progress is visible (although it might be "indefinite")

## Game controls

- Tap: left click
- Tap and hold (long press): right click
- Tap in the bottom area (status bar): activate chat/console in the game

You can start game directly (avoiding the launcher) by changing setting in iOS Settings app - VCMI.

## Reporting bugs

- Please report about gameplay problem on forums: [Help & Bugs](https://forum.vcmi.eu/c/international-board/help-bugs)
- Please report iOS-specific issues in [this forum thread](https://forum.vcmi.eu/t/ios-port/820) or (better) at [GitHub](https://github.com/vcmi/vcmi/issues) with **iOS** label
