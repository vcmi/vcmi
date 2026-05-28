---
# YAML header
render_macros: false
---
# Translation Maintenance

NOTE: Operations described here require admin access to either VCMI on Github or to Weblate and can only be done by VCMI Team

### Engine support for new languages

In order to add new language it needs to be added in multiple locations in source code:

- Generate new .ts files for launcher and map editor, either by running `lupdate` with name of new `.ts` or by copying `english.ts` and editing language tag in the header.
- Add new language into `lib/Languages.h` entry. This will trigger static_assert's in places that needs an update in code
- Add new language into json schemas validation list - settings schema and mod schema
- Add new language into mod json format - in order to allow translation into new language

Also, make full search for a name of an existing language to ensure that there are no other places not referenced here

### Heroes III data translation support

To create translation mod for a new language,  easiest approach is to:

- Create new mod, using mod.json from existing translation such as [English](https://github.com/vcmi-mods/h3-for-vcmi-englisation) as base
- Mod ID must be `<language>-translation`, mod name must be `<Language> translation`.
- Change "language" field to language ID of new language, make sure that "modType" is set to "Translation"
- Add empty Json files ( `{}` ) named `game.json`, `campaigns.json`, `maps.json`, and `chronicles.json` to directory `content/translation/`
- Upload mod files to mod repository

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

### Addition of new mods to Weblate

Requires admin access to Weblate. All fields can be edited later in Weblate project settings

- Create new project on Weblate:
  - Project name: name of the mod
  - Project slug: mod ID
  - Project website: mod page in vcmi-mods org
- Create new component:
  - Version control system: `Git`
  - Source code repository: `https://github.com/vcmi-mods/mod-name.git`
  - Repository branch: `vcmi-1.x`. This is branch from which Weblate will take translations
  - On next page, select `Json nested structure file` with path pattern `content/translation/*.json`
  - Repository push URL: `https://github_pat_***@github.com/vcmi-mods/horn-of-the-abyss.git` (replace *** with token from existing project. WARNING: DO NOT share exact URL anywhere, or specifically - private github PAT)
  - Push branch: `vcmi-1.x`. This is branch to which Weblate will upload translations
  - Rest of options are the same as with addition of new submods

With this configuration, Weblate will automatically push changes every 24 hours after first change to `vcmi-1.x` branch

### Addition of new submods to Weblate

Requires admin access to Weblate

On Weblate add new component and configure it as follows. If you have not set any of these fields, they can be edited later in Component -> Settings -> Files tab.

- Source code repository: `weblate://mod-name/mod-name`
- On next page, select `Json nested structure file` with path pattern:
  - File mask: `mods/XXX/content/translation/*.json` for submods.
  - File mask: `mods/XXX/mods/YYY/content/translation/*.json` for submods of submods
- Json indentation: `1`
- Json indentation style: `Tabs`
- Monolingual base language file: `mods/XXX/content/translation/english.json` for submods, or `mods/XXX/mods/YYY/content/translation/english.json` for submods of submods.
- Edit base file: `off`
- Adding new translation: `Disable adding new translations`
