# Faction Help

This page helps you to create from scratch a VCMI mod that adds a new faction. The faction mod structure is described [here](Faction_Format.md).

## Questioning the faction creation

Before creating a faction, be aware that creating a faction mod is lots of work. You can start [creating creatures](Creature_Help.md) in a creature mod that can be converted into a faction mod after. This way, you are sure to release something. The smallest contribution is a hero portrait that you can suggest on an existing mod. You can also restore the former version of the [Ruins faction](https://github.com/vcmi-mods/ruins-town/tree/1bea30a1d915770e2fd0f95d158030815ff462cd). You would only have to remake the similar parts to the new version.

## Make a playable faction mod

Before creating your content, retrieve the content of an existing faction mod like [Highlands town](https://github.com/vcmi-mods/highlands-town). To download the project, click on the *Code* button and click on *Download ZIP*. The first thing to do is to change all the faction identifiers in the files following the [faction format](Faction_Format.md) and manage to play with the faction and the original without any conflict. To play to a faction, you have to add all the files in your *Mods* folder. When it works, you will be able to modify the content step by step.

Keep in mind that the most important part of a faction mod, above the animations, the graphisms and the musics, is the concept because if you have to change it, you have to change everything else. All the remaining content can be improved by the community.

## Town screen

### Background

Beware to direct all the shadows to the same direction. The easiest way to create the background is to use a text-to-image AI. The free most powerful AI at the moment is *Flux* available here: <https://huggingface.co/spaces/multimodalart/FLUX.1-merged>

In the *Advanced Options*, set the width to 800px and set the height to 374px.

### Buildings

To render a building upon the background, I recommend to use an inpainting AI like *BRIA Inpaint*: <https://huggingface.co/spaces/briaai/BRIA-2.3-Inpainting>

The idea is to select the area where you want to add the building. As a prompt, describe the new building. The advantage is a perfect match between the background and the building. Keep in mind that to correctly integrate a building image, it must contain the image of the background on its edges. It simulates the semi-transparency.

You can also animate the building or the background using *Stable Video Diffusion*: <https://huggingface.co/spaces/multimodalart/stable-video-diffusion>

## Map dwellings

You may want to get the same render as in the town, so you have to change the angle and the shadows. If you handle a 3D model software, you can start with *Stable Fast 3D*: <https://huggingface.co/spaces/stabilityai/stable-fast-3d>

The map dwellings are not rendered on the map with vanishing points but in isometric. You can [get an orthogonal render in Blender](https://blender.stackexchange.com/a/135384/2768).

Without 3D, you can use *Zero 1-to-3*: <https://huggingface.co/spaces/cvlab/zero123-live>

You can get higher resolution using this Video AI that can control the motion of the camera: <https://huggingface.co/spaces/TencentARC/MotionCtrl_SVD>

The buildings on the map are more satured than on town screen. If you have to reduce the size of an image, do not use interpolation (LANCZOS, Bilinear...) to get more details, not a blurred image. If you need to increase the resolution or the quality of your template image, use *SUPIR*: <https://huggingface.co/spaces/Fabrice-TIERCELIN/SUPIR>

## Map buildings

The AIs badly understand the sun direction and the perspective angles. To generate the buildings on the adventure map:

1. Open the HOMM3 map editor
1. Put items all around a big empty area
1. Make a screenshot
1. Go on an AI like *BRIA Inpaint*: <https://huggingface.co/spaces/briaai/BRIA-2.3-Inpainting>
1. Inpaint the (big) empty middle with the brush
1. Use a prompt like: `A dark house, at the center of the image, map, isometric, parallel perspective, sunlight from the bottom right`

## Music

Here are unused available themes:

* [Synthetic Horizon](https://github.com/Fabrice-TIERCELIN/forge/raw/theme/content/music/factions/theme.ogg)

1. Prompt: `Dystopy, Cinematic classical, Science fiction, 160 bpm, Best quality, Futuristic`
1. Initially created for: *Forge town*

* [Quantum Overture](https://github.com/Fabrice-TIERCELIN/asylum-town/raw/theme/asylum-town/content/Music/factions/AsylumTown.ogg)

1. Prompt: `Clef shifting, Fantasy, Mystical, Overworldly, Cinematic classical`
1. Initially created for: *Asylum town*

* [Warrior s March](https://github.com/vcmi-mods/ruins-town/assets/20668759/964f27de-6feb-4ef6-9d25-455f52938cef)

1. Prompt: `Powerful percussions, Drums, Battle Anthem, Rythm, Warrior, 160 bpm, Celtic, New age, Instrumental, Accoustic, Medieval`
1. Initially created for: *Ruins town*

* [Clan of Echoes](https://github.com/Fabrice-TIERCELIN/ruins-town/raw/theme/ruins-town/content/music/ruins.ogg)

1. Prompt: `new age, medieval, celtic, warrior, battle, soundtrack, accoustic, drums, rythm`
1. Initially created for: *Ruins town*

* [Enchanted Reverie](https://github.com/Fabrice-TIERCELIN/grove/raw/theme/Grove/content/Music/factions/GroveTown.ogg)

1. Prompt: `Classical music, Soundtrack, Score, Instrumental, 160 bpm, ((((fantasy)))), mystic`
1. Initially created for: *Grove town*

* [World Discovery](https://github.com/vcmi-mods/asylum-town/assets/20668759/34438523-8a44-44ca-b493-127501b474a6)

1. Prompt: `Clef shifting, fantasy, mystical, overworldly, Cinematic classical`
1. Initially created for: *Asylum town*

* [Enchanted Ballad](https://github.com/vcmi-mods/fairy-town/assets/20668759/619e6e33-d940-4899-8c76-9c1e8d3d20aa)

1. Prompt: `Females vocalize, Cinematic classical, Harp, Fairy tale, Princess, 160 bpm`
1. Initially created for: *Fairy town*

* [Baroque Resurgence](https://github.com/Fabrice-TIERCELIN/courtyard_proposal/raw/theme/Courtyard/Content/music/factions/courtyard/CourtTown.ogg)

1. Prompt: `Baroque, Instrumental, 160 bpm, Cinematic classical, Best quality`
1. Initially created for: *Courtyard town*

* [Harvest Parade](https://github.com/Fabrice-TIERCELIN/greenhouse-town/raw/theme/Greenhouse/content/Music/town.ogg)

1. Prompt: `Marching band, Best quality, Happy, Vegetables`
1. Initially created for: *Green town*

Those themes have been generated using *[Udio](https://udio.com)*.

## Screenshots

Most of the time, the first screenshot is the townscreen because it's the most specific content.

## Recycle

Some mods contain neutral heroes or creatures. You can integrate them in your faction mod. Don't forget to remove the content from the original mod.
