You can run VCMI on iOS 12.0 and later, all devices are supported. If you wish to run on iOS 10 or 11, you should build from source, see [How to build VCMI (iOS)](How_to_build_VCMI_(iOS) "wikilink").

## Download and install VCMI

- The latest release (recommended): <https://github.com/vcmi/vcmi/releases/latest>
- Daily builds: <https://builds.vcmi.download/branch/develop/iOS/>

To run on a non-jailbroken device you need to sign the IPA file, you
have the following options:

- (Easiest way) [AltStore](https://altstore.io/) or [Sideloadly](https://sideloadly.io/) - can be installed on Windows or macOS, don't require dealing with signing on your own 
- if you're on iOS 14.0-15.4.1, you can try <https://github.com/opa334/TrollStore>
- Get signer tool [here](https://dantheman827.github.io/ios-app-signer/) and a guide [here](https://forum.kodi.tv/showthread.php?tid=245978) (it's for Kodi, but the logic is the same). Signing with this app can only be done on macOS.
- [Create signing assets on macOS from terminal](https://github.com/kambala-decapitator/xcode-auto-signing-assets). In the command replace `your.bundle.id` with something like `com.MY-NAME.vcmi`. After that use the above signer tool.
- [Sign from any OS](https://github.com/indygreg/PyOxidizer/tree/main/tugger-code-signing). You'd still need to find a way to create signing assets (private key and provisioning profile) though.

## Installing Heroes III data files

Note: if you don't need in-game videos, you can omit downloading/copying file VIDEO.VID from data folder - it will save your time and space. The same applies to the Mp3 directory.

### Installing data files with Finder or Windows explorer

To play the game, you need to upload HoMM3 data files - **Data**, **Maps** and **Mp3** directories - to the device. Use Finder (or iTunes, if you're on Windows or your macOS is 10.14 or earlier) for that. You can also add various [mods](https://wiki.vcmi.eu/Mod_list) by uploading **Mods** directory. Follow [official Apple guide](https://support.apple.com/en-us/HT210598) and place files into VCMI app. Unfortunately, Finder doesn't display copy progress, give it about 10 minutes to finish.

### Installing data files using iOS device only

If you have data somewhere on device or in shared folder or you have downloaded it, you can copy it directly on your iPhone/iPad using Files application.

Move or copy **Data**, **Maps** and **Mp3** folders into vcmi application - it will be visible in Files along with other applications' folders.

### Installing data files with Xcode on macOS

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
