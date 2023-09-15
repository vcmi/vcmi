< [Documentation](../Readme.md) / [Modding](Readme.md) / Translations

## For translators
### Adding new languages
New languages require minor changes in the source code. Please contact someone from vcmi team if you wish to translate game into a new language

### Translation of Launcher and Map Editor
TODO: write me

### Translation of game content
TODO: write me

## For modders

### Adding new translation to mod
TODO: write me

### Translation of mod information
In order to display information in Launcher in language selected by user add following block into your mod.json:
```
	"<language>" : {
		"name" : "<translated name>",
		"description" : "<translated description>",
		"author" : "<translated author>",
		"modType" : "<translated mod type>",
	},
```
List of currently supported values for language parameter:
- english
- german
- polish
- russian
- ukrainian

However, normally you don't need to use block for English. Instead, English text should remain in root section of your mod.json file, to be used when game can not find translated version.

### Translation of mod content
TODO: write me

### Searching for missing translation strings
TODO: write me

## For developers
### Adding new languages
In order to add new language it needs to be added in multiple locations in source code:
- Generate new .ts files for launcher and map editor, either by running `lupdate` with name of new .ts or by copying english.ts and editing language tag in the header.
- Add new language into Launcher UI drop-down selector and name new string using such format:
`"<native name for language> (<english name for language>)"`
- Add new language into array of possible values for said selector

Also, make full search for a name of an existing language to ensure that there are not other places not referenced here
### Updating translation of Launcher and Map Editor to include new strings
At the moment, build system will generate binary translation files (.qs) that can be opened by Qt.
However, any new or changed lines will not be added into existing .ts files.
In order to update .ts files manually, open command line shell in `mapeditor` or `launcher` source directories and execute command
```
lupdate -no-obsolete * -ts translation/*.ts
```
This will remove any no longer existing lines from translation and add any new lines for all translations.
There *may* be a way to do the same via QtCreator UI or via CMake, if you find one feel free to update this information.
### Updating translation of Launcher and Map Editor using new .ts file from translators
Generally, this should be as simple as overwriting old files. Things that may be necessary if translation update is not visible in executable:
- Rebuild subproject (map editor/launcher).
- Regenerate translations via `lupdate -no-obsolete * -ts translation/*.ts`