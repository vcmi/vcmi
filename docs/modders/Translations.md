< [Documentation](../Readme.md) / [Modding](Readme.md) / Translations

# Translation system

## List of currently supported languages

This is list of all languages that are currently supported by VCMI. If your languages is missing from the list and you wish to translate VCMI - please contact our team and we'll add support for your language in next release.

- Czech
- Chinese (Simplified)
- English
- Finnish
- French
- German
- Hungarian
- Italian
- Korean
- Polish
- Portuguese (Brazilian)
- Russian
- Spanish
- Swedish
- Turkish
- Ukrainian
- Vietnamese

## Translating Heroes III data

VCMI allows translating game data into languages other than English. In order to translate Heroes III in your language easiest approach is to:

- Copy existing translation, such as English translation from here: https://github.com/vcmi-mods/h3-for-vcmi-englisation
- Rename mod to indicate your language, preferred form is "(language)-translation"
- Update mod.json to match your mod
- Translate all texts strings from game.json, campaigns.json and maps.json

If you have already existing Heroes III translation you can:

- Install VCMI and select your localized Heroes III data files for VCMI data files
- Launch VCMI_Client.exe directly from game install directory
- In console window, type `convert txt`

This will export all strings from game into `Documents/My Games/VCMI/VCMI_Client_log.txt` which you can then use to update json files in your translation

## Translating VCMI data

VCMI contains several new strings, to cover functionality not existing in Heroes III. It can be roughly split into following parts:

- In-game texts, most noticeably - in-game settings menu.
- Game Launcher
- Map Editor
- Android Launcher

Before you start, make sure that you have copy of VCMI source code. If you are not familiar with git, you can use Github Desktop to clone VCMI repository.

### Translation of in-game data

In order to translate in-game data you need:
- Add section with your language to `<VCMI>/Mods/VCMI/mod.json`, similar to other languages
- Copy English translation file in `<VCMI>/Mods/VCMI/config/vcmi/english.json` and rename it to name of your language. Note that while you can copy any language other than English, other files might not be up to date and may have missing strings.
- Translate copied file to your language.

After this, you can set language in Launcher to your language and start game. All translated strings should show up in your language.

### Translation of Launcher and Editor

VCMI Launcher and Map Editor use translation system provided by Qt framework so it requires slightly different approach than in-game translations:

- Install Qt Linguist. You can find find standalone version here: https://download.qt.io/linguist_releases/
- Open `<VCMI Sources>/launcher/translation/` directory, copy english.ts file and rename it to your language
- Launch Qt Linguist, select Open and navigate to your copied file
- Select any untranslated string, enter translation in field below, and click "Done and Next" (Ctrl+Return) to navigate to next untranslated string
- Once translation has been finished, save resulting file.

Translation of Map Editor is identical, except for location of translation files. Open `<VCMI Sources>/editor/translation/` instead to translate Map Editor

TODO: how to test translation locally

### Translation of AppStream metainfo

The [AppStream](https://freedesktop.org/software/appstream/docs/chap-Metadata.html) [metainfo file](https://github.com/vcmi/vcmi/blob/develop/launcher/eu.vcmi.VCMI.metainfo.xml) is used for Linux software centers.

It can be translated using a text editor or using [jdAppStreamEdit](https://flathub.org/apps/page.codeberg.JakobDev.jdAppStreamEdit):
- Install jdAppStreamEdit
- Open `<VCMI>/launcher/eu.vcmi.VCMI.metainfo.xml`
- Translate and save the file

### Translation of Android Launcher

TODO
see https://github.com/vcmi/vcmi/blob/develop/android/vcmi-app/src/main/res/values/strings.xml

### Submitting changes

Once you have finished with translation you need to submit these changes to vcmi team using git or Github Desktop
- Commit all your changed files
- Push changes to your forked repository
- Create pull request in VCMI repository with your changes

If everything is OK, your changes will be accepted and will be part of next release.

## Translating mods

### Exporting translation

TODO

### Translating mod information
In order to display information in Launcher in language selected by user add following block into your mod.json:
```
	"<language>" : {
		"name" : "<translated name>",
		"description" : "<translated description>",
		"author" : "<translated author>",
		"translations" : [
			"config/<modName>/<language>.json"
		]
	},
```
However, normally you don't need to use block for English. Instead, English text should remain in root section of your mod.json file, to be used when game can not find translated version.

# Developers documentation

### Adding new languages
In order to add new language it needs to be added in multiple locations in source code:
- Generate new .ts files for launcher and map editor, either by running `lupdate` with name of new .ts or by copying english.ts and editing language tag in the header.
- Add new language into lib/Languages.h entry. This will trigger static_assert's in places that needs an update in code
- Add new language into json schemas validation list - settings schema and mod schema
- Add new language into mod json format - in order to allow translation into new language

Also, make full search for a name of an existing language to ensure that there are not other places not referenced here

### Updating translation of Launcher and Map Editor to include new strings

At the moment, build system will generate binary translation files (.qs) that can be opened by Qt.
However, any new or changed lines will not be added into existing .ts files.
In order to update .ts files manually, open command line shell in `mapeditor` or `launcher` source directories and execute command
```
lupdate -no-obsolete * -ts translation/*.ts
```

This will remove any no longer existing lines from translation and add any new lines for all translations. If you want to keep old lines, remove `-no-obsolete` key from the command
There *may* be a way to do the same via QtCreator UI or via CMake, if you find one feel free to update this information.

### Updating translation of Launcher and Map Editor using new .ts file from translators

Generally, this should be as simple as overwriting old files. Things that may be necessary if translation update is not visible in executable:
- Rebuild subproject (map editor/launcher).
- Regenerate translations via `lupdate -no-obsolete * -ts translation/*.ts`
