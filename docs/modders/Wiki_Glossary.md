# Wiki Glossary

VCMI's in-game Wiki window has a **Glossary** category that shows free-form encyclopedia entries.  Any mod can add its own entries without conflicting with entries from other mods.

## File location

Glossary entries are declared in

```text
<mod_root>/Content/config/wikiGlossary.json
```

Each entry references two translation keys that must be present in the mod's translation file (e.g. `config/translations/english.json`).

---

## wikiGlossary.json format

```json
{
    "categories": {
        "mymod.mycategory": {
            "name": "mymod.mycategory.name"
        }
    },
    "entries": {
        "mymod.wiki.myentry": {
            "name":        "mymod.wiki.myentry.name",
            "description": "mymod.wiki.myentry.description"
        },
        "mymod.wiki.myentry.custom": {
            "name":        "mymod.wiki.myentry.custom.name",
            "description": "mymod.wiki.myentry.custom.description",
            "category":    "mymod.mycategory",
            "order":       1
        },
        "mymod.wiki.myentry.custom2": {
            "name":        "mymod.wiki.myentry.custom2.name",
            "description": "mymod.wiki.myentry.custom2.description",
            "category":    "mymod.mycategory",
            "order":       2
        }
    }
}
```

### `"categories"` block (optional)

Declares additional wiki tabs that appear in the category column alongside the built-in tabs.  Each key is a **unique string ID** for the category.  The ID is used in wiki links and must not conflict with the built-in reserved IDs: `glossary`, `town`, `hero`, `creature`, `artifact`, `spell`, `skill`, `terrain`, `mod`.  Conflicting keys are logged as errors and skipped at runtime.

| Field | Required | Description |
| --- | --- | --- |
| `name` | yes | Translation key for the tab label shown in the category list. |

Custom categories always render their entries as Markdown (same renderer as the Glossary).

### `"entries"` block

`"entries"` is a **JSON object** (not an array).  Each key is the unique identifier for the entry, used in wiki links.  Values are objects with the following fields:

| Field | Required | Description |
| --- | --- | --- |
| `name` | yes | Translation key for the list title shown in the left panel. |
| `description` | yes | Translation key for the full article body shown on the right. The value supports the Markdown syntax described below. |
| `category` | no | String ID of the category this entry belongs to.  Defaults to `"glossary"` when omitted.  Must match a key in `"categories"` or `"glossary"`. |
| `order` | no | Integer that controls the position of this entry within its category's list.  Entries are sorted by ascending `order` value; entries without an `order` field are placed after all ordered entries, in alphabetical order among themselves. |

Because both `"categories"` and `"entries"` are JSON objects, `assembleFromFiles` correctly **merges** contributions from all active mods.

> **Entry ordering:**
>
> - **Glossary** entries are sorted alphabetically by display name.  The `order` field is ignored for the built-in Glossary category.
> - **Custom category** entries are sorted by the `order` integer.  Entries that omit `order` are treated as if `order` is `999999` and appear last (alphabetically among themselves).
>
> Using explicit `order` values is strongly recommended whenever a category contains more than one entry, because JSON objects do not guarantee a stable iteration order across platforms.

### Linking to a custom-category entry

```text
[display text](wiki:mymod.mycategory/mymod.wiki.myentry.custom)
```

The category part of the URI is the custom category's string ID.

---

## Description / Markdown syntax

The description value is a JSON string.  Use `\n` for newlines.  The renderer supports the following constructs.

### Headings

```text
# Heading level 1
## Heading level 2
### Heading level 3
```

Headings are left-aligned by default.  Wrap a heading in alignment tags (see below) to change its alignment.

### Horizontal rule

```text
---
```

Three or more `-`, `_`, or `*` characters on their own line.

### Paragraphs

A blank line (`\n\n`) ends the current paragraph and starts a new one.

### Lists

**Unordered** – begin a line with `-` or `*` followed by a space:

```text
- First item\n- Second item
```

**Ordered** – begin a line with an integer, a period, and a space (e.g. `1.`):

```text
1. First step\n2. Second step
```

### Alignment

Wrap one or more block elements (headings, images, animations, videos) between an opening and a matching closing tag to control their alignment:

| Opening tag | Closing tag | Effect |
| --- | --- | --- |
| `<left>` | `</left>` | Align to the left. |
| `<center>` | `</center>` | Align to the centre. |
| `<right>` | `</right>` | Align to the right. |

Alignment persists for **all** block elements between the opening and closing tag.  A closing tag resets alignment back to the element default.

Headings default to **left**; images, animations, and videos default to **left**.

### Images

```text
![alt text](filename.ext)
```

The file extension determines how the resource is loaded.

| Extension | Rendering |
| --- | --- |
| `.png` `.pcx` `.bmp` (or any non-animation image) | Static image.  Downscaled to fit the viewport width if wider. |
| `.def` `.json` (no `#N` suffix) | Animation; all frames played as a looping ~6 fps animation. |
| `.def` `.json#N` | Single static frame `N` from the animation. |
| `.bik` `.smk` `.webm` `.mp4` | Looped video; downscaled automatically if too wide. |

The resource system strips the extension at load time, so `CPRSMALL.DEF` and `CPRSMALL` both resolve to the same asset.  Always include the extension so the Markdown renderer can detect the media type.

If the `alt text` field is non-empty, right-clicking the image, animation, or video shows it as a popup tooltip.

#### Examples

```text
Static image:
![Background](DIBOXBCK.PCX)

Animation loop (all frames):
![Creature portraits](CPRSMALL.DEF)

Single animation frame:
![Portrait 0](CPRSMALL.DEF#0)

Video (looped):
![Battle intro](LBSTART.BIK)

Centred animation:
<center>
![Centred portrait](CPRSMALL.DEF#3)
</center>

Animation with right-click tooltip:
![This text appears on right-click](CPRSMALL.DEF#0)
```

### Tables

GFM-style pipe tables are supported.  The first row is automatically rendered as a header (yellow, dark background).  The second row **must** be a separator row (`|---|---|`).

```text
| Column A | Column B |
|----------|----------|
| Cell text | More text |
```

Cell content may be any media syntax:

```text
| Creature | Icon |
|----------|------|
| Frame 0  | ![f0](CPRSMALL.DEF#0) |
| Animated | ![loop](CPRSMALL.DEF)  |
```

Column widths are distributed equally.  Text cells wrap automatically.

### VCMI colour tags

All text (paragraphs, lists, table cells, headings) passes through the VCMI label renderer, so `{highlighted text}` colour tags work everywhere:

```text
The {Fire Wall} spell deals {direct damage}.
```

### Links

Wiki links navigate to another entry when clicked.  They are shown underlined in blue.

#### Text link

```text
[display text](wiki:category/identifier)
```

Placed on its own line or inline within a paragraph.

#### Linked image

An image, animation, or video wrapped in a link navigates on left-click:

```text
[![alt text](media.def)](wiki:category/identifier)
```

Right-clicking a linked image still shows the alt-text tooltip as normal.

#### Categories and identifiers

| Category string | Content |
| --- | --- |
| `glossary` | Manual glossary entries |
| `creature` | Creature list |
| `spell` | Spell list |
| `hero` | Hero list |
| `town` | Town / faction list |
| `artifact` | Artifact list |
| `skill` | Secondary skill list |
| `terrain` | Terrain type list |
| `mod` | Installed mods |

**Glossary identifier** – the key of the entry in the `"entries"` object of `wikiGlossary.json`:

```text
"entries": { "mymod.wiki.myfeature": { ... } }   →   wiki:glossary/mymod.wiki.myfeature
```

**Game-entity identifier** – the JSON key of the entity as returned by `getJsonKey()`.  For core content this is typically just the unscoped name:

```text
wiki:creature/imp        (matches "core:imp")
wiki:spell/fireball
wiki:skill/eagleEye
```

Both the scoped form (`core:imp`) and the unscoped form (`imp`) are accepted.

### Anchors

Invisible anchors mark a position inside a page so that a link can jump directly to that position.  They are never rendered; only their Y offset is recorded.

#### Standalone anchor

Place the tag on its own line (nothing else on that line):

```text
<a id="my-anchor" />
```

Both `id` and `name` attributes are accepted.  Anchor names are case-sensitive; lowercase letters, digits, and hyphens are recommended.

#### Anchor embedded in a heading

Prefix or suffix the heading line with an anchor tag on the same Markdown line:

```text
## <a id="my-section" />Section Title
## Section Title<a id="my-section" />
```

The anchor tag is stripped before the heading is rendered; the visible heading text is not affected.

#### Linking to an anchor

Append `#anchorname` to the entry identifier in any wiki link:

```text
[Jump to my section](wiki:glossary/mymod.wiki.mypage#my-section)
```

Navigation first loads the target entry, then scrolls the page to the anchor position.

---

## Minimal example

**`config/wikiGlossary.json`**:

```json
{
    "entries": {
        "mymod.wiki.myfeature": {
            "name":        "mymod.wiki.myfeature.name",
            "description": "mymod.wiki.myfeature.description"
        }
    }
}
```

**`config/translations/english.json`**:

```json
{
    "mymod.wiki.myfeature.name": "My Feature",
    "mymod.wiki.myfeature.description":
        "## Overview\n\nDescribe the feature here.\n\n---\n\n## Details\n\n- Point one\n- Point two"
}
```

The entry will appear in the Glossary list under the letter **M** and display the rendered Markdown when clicked.
