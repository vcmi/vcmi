# Translations

## List of currently supported languages

This is list of all languages that are currently supported by VCMI. If your languages is missing from the list and you wish to translate VCMI - please contact our team and we'll add support for your language in next release.

- Belarusian
- Bulgarian
- Czech
- Chinese (Simplified)
- Dutch
- English
- Filipino
- Finnish
- French
- German
- Greek
- Hungarian
- Italian
- Japanese
- Korean
- Latvian
- Norwegian
- Polish
- Portuguese (Brazilian)
- Romanian
- Russian
- Serbian
- Spanish
- Swedish
- Turkish
- Ukrainian
- Vietnamese

## Progress of the translations

You can see the current progress of the different translations here:
[Translation progress](https://github.com/vcmi/vcmi-translation-status)

The page will be automatically updated once a week.

## Translating Heroes III data

VCMI allows translating game data into languages other than English. In order to translate Heroes III in your language easiest approach is to:

- Copy existing translation, such as English translation from here: <https://github.com/vcmi-mods/h3-for-vcmi-englisation> (delete sound and video folders)
- Copy text-free images from here: <https://github.com/vcmi-mods/empty-translation>
- Rename mod to indicate your language, preferred form is "(language)-translation"
- Update mod.json to match your mod
- Translate all texts strings from `game.json`, `campaigns.json` and `maps.json`
- Replace images in data and sprites with translated ones (or delete it if you don't want to translate them)
- If unicode characters needed for language: Create a submod with a free font like here: <https://github.com/vcmi-mods/vietnamese-translation/tree/vcmi-1.4/vietnamese-translation/mods/VietnameseTrueTypeFonts>

If you can't produce some content on your own (like the images or the sounds):

- Create a `README.md` file at the root of the mod
- Write into the file the translations and the **detailled** location

This way, a contributor that is not a native speaker can do it for you in the future.

If you have already existing Heroes III translation you can:

- Install VCMI and select your localized Heroes III data files for VCMI data files
- Launch VCMI and start any map to get in game
- Press Tab to activate chat and enter `/translate`

This will export all strings from game into `Documents/My Games/VCMI/extracted/translation/` directory which you can then use to update json files in your translation.

To export maps and campaigns, use `/translate maps` command instead.

### Video subtitles

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

## Translating VCMI data

VCMI game data is translated via [Weblate](https://hosted.weblate.org/projects/vcmi/).

For usage of Weblate, please refer to [Weblate documentation](https://docs.weblate.org/en/latest/user/translating.html)

If something is not clear - feel free to ask us on Discord or forum. Translation made via Weblate will be automatically integrated into VCMI for next release

## Translating mods via Weblate

Translation for some mods is being migrated to our self-hosted [Weblate](https://weblate.vcmi.eu/projects/). If mod that you wish to translate is already there and it already has your language, then all you need to do is register and start translating.

If you wish to add a new mod on our Weblate, please contact VCMI Team for initial setup via Discord or Github.

### Initial setup

Before starting, go through this checklist to ensure that mod is ready for Weblate:

- mod must be hosted by vcmi-mods Github organization
- mod must contain mod.json in top-level directory, and not in a subdirectory
- preferrably, mod should use centralized workflow from <https://github.com/vcmi-mods/workflow>
- export English strings by running `vcmiserver --translate-mod=mod-name` and moving all generated `english.json` files to your mod
- create dummy .json for each language to which you wish to translate in every submod that has translatable strings

This operation can only be done by VCMI Team. Only mods hosted by vcmi-mods org can be translated this way.

- Create new project on Weblate:
  - Project name: name of the mod
  - Project slug: mod ID
  - Project website: mod page in vcmi-mods org
- Create new component:
  - Version control system: `Git`
  - Source code repository: `https://github.com/vcmi-mods/mod-name.git`
  - Repository branch: `vcmi-1.x`. This is branch from which Weblate will take translations
  - On next page, select `Json nested structure file` with path pattern `content/translation/*.json`
  - Repository push URL: `https://github_pat_***@github.com/vcmi-mods/horn-of-the-abyss.git` (replace *** with token from existing project)
  - Push branch: `vcmi-1.x`. This is branch to which Weblate will upload translations
  - Rest of options are the same as with addition of new submods

### Addition of new languages

This operation can be done by anyone with write access to mod repository. You can also create new pull request and ask for someone with write access to accept it.

If translation to a new language needs to be added to Weblate, please add empty dummy files for required language to mod repository on Github. This should automatically generate translation on Weblate once changes are merged.

Note that while Weblate supports basically any language, VCMI only supports limited set of languages listed in this document

### Addition of new submods

This operation can only be done by VCMI Team

If there is a new submods that needs translating, add required files (english.json and any required dummy translations) to mod repository on Github. Please use the same list of translations as for the rest of the mod.

After that, on Weblate add new component and configure it as follows. If you have not set any of these fields, they can be edited later in Component -> Settings -> Files tab.

- Source code repository: `weblate://mod-name/mod-name`
- On next page, select `Json nested structure file` with path pattern:
  - File mask: `Mods/XXX/Content/translation/*.json` for submods.
  - File mask: `Mods/XXX/Mods/YYY/Content/translation/*.json` for submods of submods
- Json indentation: `1`
- Json indentation style: `Tabs`
- Monolingual base language file: `Mods/XXX/Content/translation/english.json` for submods, or `Mods/XXX/Mods/YYY/Content/translation/english.json` for submods of submods.
- Edit base file: `off`
- Adding new translation: `Disable adding new translations`

WARNING: Do not edit or move translation files other than through Weblate. If you have to, for example due to submod reorganization, please contact VCMI Team.

## Translating mods manually

### Exporting translation

If you want to start new translation for a mod or to update existing one you may need to export it first. To do that:

- Optionally, backup your mod preset - game may modify active submods of mod being translated
- Set game language in Launcher to one that you want to target
- launch VCMI server in translation export mode:
  - (Windows) Create shortcut for VCMI_Server.exe and append `--translate-mod=XXX` to "Target" field in shortcut properties, where XXX is identifier of mod that you want to translate
  - (command-line) Open command line and run `vcmiserver --translate-mod=XXX`, where XXX is identifier of mod that you want to translate

After that, start Launcher, switch to Help tab and open "log files directory". You can find exported json's in `extracted/translation` directory.

### Exporting translation (alternative)

Alternatively, you can use vcmi client to do similar actions:

- Enable mod(s) that you want to export and set game language in Launcher to one that you want to target
- Launch VCMI and start any map to get in game
- Press Tab to activate chat and enter '/translate'

After that, start Launcher, switch to Help tab and open "log files directory". You can find exported json's in `extracted/translation` directory.

If your mod also contains maps or campaigns that you want to translate, then use `/translate maps` command instead.

If you want to update existing translation, you can use `/translate missing` command that will export only strings that were not translated

NOTE: when translating with this method, some strings may not export correctly, for example strings that were modified in multiple mods. To avoid this, you'll need to disable mods that overrride other strings and do a second re-run of this command

### Translating mod information

In order to display information in Launcher in language selected by user add following block into your `mod.json`:

```json
	"<language>" : {
		"name" : "<translated name>",
		"description" : "<translated description>",
		"author" : "<translated author>",
		"translations" : [
			"translation/<language>.json"
		]
	},
```

However, normally you don't need to use block for English. Instead, English text should remain in root section of your `mod.json` file, to be used when game can not find translated version.

### Translating in-game strings

After you have exported translation and added mod information for your language, copy exported file to `<mod directory>/Content/translation/<language>.json`.

Use any text editor (Notepad++ is recommended for Windows) and translate all strings from this file to your language

## Developers documentation

### Adding new languages

In order to add new language it needs to be added in multiple locations in source code:

- Generate new .ts files for launcher and map editor, either by running `lupdate` with name of new `.ts` or by copying `english.ts` and editing language tag in the header.
- Add new language into `lib/Languages.h` entry. This will trigger static_assert's in places that needs an update in code
- Add new language into json schemas validation list - settings schema and mod schema
- Add new language into mod json format - in order to allow translation into new language

Also, make full search for a name of an existing language to ensure that there are not other places not referenced here

### Updating translation of Launcher and Map Editor to include new strings

At the moment, build system will generate binary translation files (`.qs`) that can be opened by Qt.
However, any new or changed lines will not be added into existing .ts files.
In order to update `.ts` files manually, open command line shell in `mapeditor` or `launcher` source directories and execute command

```sh
lupdate -no-obsolete * -ts translation/*.ts
```

This will remove any no longer existing lines from translation and add any new lines for all translations. If you want to keep old lines, remove `-no-obsolete` key from the command.
There *may* be a way to do the same via QtCreator UI or via CMake, if you find one feel free to update this information.

### Updating translation of Launcher and Map Editor using new .ts file from translators

Generally, this should be as simple as overwriting old files. Things that may be necessary if translation update is not visible in executable:

- Rebuild subproject (map editor/launcher).
- Regenerate translations via `lupdate -no-obsolete * -ts translation/*.ts`

### Translating the Installer

VCMI uses an Inno Setup installer that supports multiple languages. To add a new translation to the installer, follow these steps:

1. **Download the ISL file for your language:**
   - Visit the Inno Setup repository to find the language file you need:  
     [Inno Setup Languages](https://github.com/jrsoftware/issrc/tree/main/Files/Languages).

2. **Add custom VCMI messages:**
   - Open the downloaded ISL file and include the necessary VCMI-specific custom messages.
   - Refer to the `English.isl` file in the repository for examples of required custom messages.
   - Ensure that all messages, such as `WindowsVersionNotSupported` and `ConfirmUninstall`, are correctly translated and match the functionality described in the English version.

3. **Modify the `ConfirmUninstall` message:**
   - The VCMI installer uses a custom Uninstall Wizard. Ensure the `ConfirmUninstall` message is consistent with the English version and accurately reflects the intended functionality.

4. **Modify the `WindowsVersionNotSupported` message:**
   - Translate and update this message to ensure it aligns with the intended warning in the English version.

5. **Add the new language to the installer script:**
   - Edit the `[Languages]` section of the Inno Setup script.
   - Add an entry for your language, specifying the corresponding ISL file.

Example syntax for adding a language:

```text
[Languages]
Name: "english"; MessagesFile: "{#LangPath}\English.isl"
Name: "czech"; MessagesFile: "{#LangPath}\Czech.isl"
Name: "<your-language>"; MessagesFile: "{#LangPath}\<your-language>.isl"
```
