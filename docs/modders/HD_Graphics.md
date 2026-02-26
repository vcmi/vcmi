# HD Graphics

It's possible to provide alternative high-definition graphics within mods. They will be used if any upscaling filter is activated.

## Preconditions

It's still necessary to add 1x standard definition graphics as before. HD graphics are seperate from usual graphics. This allows to partitially use HD for a few graphics in mod. And avoid handling huge graphics if upscaling isn't enabled.

Currently following scaling factors are possible to use: 2x, 3x, 4x. You can also provide multiple of them (increases size of mod, but improves loading performance for player). It's recommend to provide 2x and 3x images.

If user for example selects 3x resolution and only 2x exists in mod then the 2x images are upscaled to 3x (same for other combinations > 1x).

## Mod

For upscaled images you have to use following folders (next to `sprites`, `data` and `video` folders):

- `sprites2x`, `sprites3x`, `sprites4x` for sprites
- `data2x`, `data3x`, `data4x` for images
- `video2x`, `video3x`, `video4x` for videos

The sprites should have the same name and folder structure as in `sprites`, `data` and `video` folder. All images that are missing in the upscaled folders are scaled with the selected upscaling filter instead of using prescaled images.

### Shadows / Overlays / Player-colored images

It's also possible (but not necessary) to add high-definition shadows: Just place a image next to the normal upscaled image with the suffix `-shadow`. E.g. `TestImage.png` and `TestImage-shadow.png`.
In future, such shadows will likely become required to correctly exclude shadow from effects such as Clone spell.

Shadow images are used only for animations of following objects:

- All adventure map objects
- All creature animations in combat

Same for overlays with `-overlay`. But overlays are **necessary** for some animation graphics. They will be colorized by VCMI.

Currently needed for:

- Flaggable adventure map objects. Overlay must contain a transparent image with white flags on it and will be used to colorize flags to owning player
- Creature battle animations, idle and mouse hover group. Overlay must contain a transparent image with white outline of creature for highlighting on mouse hover

For images that are used for player-colored interface, it is possible to provide custom images for each player. For example `HeroScr4-red.png` will be used for hero window of red player.

- Currently needed for all UI elements that are player-colored in HoMM3.
- Can NOT be used for player-owned adventure objects. Use `-overlay` images for such objects.
- Possible suffixes are `red`, `blue`, `tan`, `green`, `orange`, `purple`, `teal`, `pink`, `neutral` (used only for turn order queue in combat)

It is possible to use such additional images for both upscaled (xbrz) graphics, as well as for original / 1x images. When using this feature for original / 1x image, make sure that your base image (without suffix) is rgb/rgba image, and not indexed / with palette
