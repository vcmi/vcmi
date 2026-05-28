# Translation of Heroes III

## Translation of Heroes III text data

VCMI maintains translation of Heroes III text files to all supported languages on the Weblate. To join translation, register on [our Weblate](https://weblate.vcmi.eu) and start translating into your language.

Heroes III translation can be located at [Heroes III project in our Weblate](https://weblate.vcmi.eu/projects/heroes-of-might-and-magic-3/#languages). This link is only accessible for registered accounts. Creating a new account should automatically grant you access to this project.

This translation includes:

- Base game data, such as names of heroes, creatures, artifact, and all in-game messages that you can encounter on any map
- Translation of maps distributed with Heroes III Complete
- Translation of RoE, AB, and SoD campaigns
- As well as translation of Chronicles, designed for use with Chronicles imported into VCMI using gog installer

Changes made via Weblate will be automatically added to corresponding translation mod in 24 hours or less.

If you have already existing Heroes III translation you can:

- Install VCMI and select your localized Heroes III data files for VCMI data files
- Launch VCMI and start any map to get in game
- Press Tab to activate chat and enter `/translate`

After that, start VCMI Launcher, switch to Help tab and open "log files directory". You can find exported json's in `extracted/translation` directory. To export maps and campaigns, use `/translate maps` command instead. Once export is complete, you can import generated files into Weblate. Make sure to select correct language and component before uploading exported json file.

## Translation of images

To translate them, do the following:

- Download your translation mod, located in [VCMI Mods on Github](https://github.com/vcmi-mods)
- Copy text-free images from [empty translation](https://github.com/vcmi-mods/empty-translation)
- Replace images in data and sprites with translated ones (or delete it if you don't want to translate them now)
- Upload modified images to your translation mod

For all images recommended format is .png. See [File Formats](../modders/File_Formats.md#images) for details. Make sure to preserve image dimensions, and if present - image palette when editing text-free images.

If you can't produce some content on your own:

- Create a `README.md` file at the root of the mod
- Write into the file the translations and the **detailled** location

This way, a contributor that is not a native speaker can do it for you in the future.

## Translation of in-game audio

- Download your translation mod, located in [VCMI Mods on Github](https://github.com/vcmi-mods)
- Check [empty translation](https://github.com/vcmi-mods/empty-translation) for information on audio (filename, text, speaker character)
- Record or generate audio files
- Placed created files into `content/sounds` directory of your translation mod
- Upload changes in your translation mod

For all audio recommended format is .ogg. See [File Formats](../modders/File_Formats.md#sounds) for details.

If you can't produce some content on your own:

- Create a `README.md` file at the root of the mod
- Write into the file the translations and the **detailled** location

This way, a contributor that is not a native speaker can do it for you in the future.

## Custom fonts

VCMI built-in font covers most languages that use European scripts - Latin, Cyrillic, Greek, etc. Other languages may need additional fonts.

You can verify this by launching VCMI, starting any map in single-player, pressing Tab to open game change and typing entire alphabet. On mobile systems you can tap on chat panel to activate it (brown area below adventure map)

If there are missing glyphs - you may need to provide own fonts. For this please locate any font with free license, such as OFL, and add font similar to one of existing mods, such as Vietnamese: <https://github.com/vcmi-mods/vietnamese-translation/tree/vcmi-1.4/vietnamese-translation/mods/VietnameseTrueTypeFonts>

## Video subtitles

It's possible to add video subtitles. Create a JSON file in `video` folder of translation mod with the name of the video (e.g. `H3Intro.json`):

```json
[
    {
        "timeStart" : 5.640, // start time, seconds
        "timeEnd" : 8.120, // end time, seconds
        "text" : " ... " // text to show during this period
    },
    ...
]
```
