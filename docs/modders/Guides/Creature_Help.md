# Creature Help

This page helps you to create a creature (i.e. a unit that fights in a battle) for a creature mod or inside a bigger mod like a [faction mod](Faction_Help.md).

## Utilities

You need to download the two utilities [`DefPreview`](https://sourceforge.net/projects/grayface-misc/files/DefPreview-1.2.1/) and [`H3DefTool`](https://sourceforge.net/projects/grayface-misc/files/H3DefTool-3.4.2/) from the internet:

- `DefPreview` converts a `.def` file to `.bmp` images
- `H3DefTool` converts `.bmp` images to a `.def` file

But you can create a configuration that directly reads your image files. Most of the existing mods are coded with `.def` files but direct images are recommended.

## Make a playable creature

First of all, retrieve an existing creature and be sure you can clone it and make it work independently without any new content. If it already fails, don't waste your time to draw the new animation. It should work first.

## Battle render

The sun is always at zenith, so the shadow is always behind. The reason is that the creature render may be mirrored. There was no strong rules in the original game but usually, the shadow is twice less higher than the creature.

We don't know the right elevation angle for the view.

### 3D render

You can render your creature using a 3D software like *Blender*. You can start with those free-licenced rigged 3D models:

- [Fantasy-bandit](https://www.cgtrader.com/free-3d-models/character/man/fantasy-bandit)
- [Monster-4](https://www.cgtrader.com/free-3d-models/character/fantasy-character/monster-4-f5757b92-dc9c-4f5e-ad0d-593203d14fe2)
- [Crypt-fiend-modular-character](https://www.cgtrader.com/free-3d-models/character/fantasy-character/crypt-fiend-modular-character-demo-scene)
- [Solus-the-knight](https://www.cgtrader.com/free-3d-models/character/man/solus-the-knight-low-poly-character)
- [Ancient-earth-golem](https://www.cgtrader.com/free-3d-models/character/fantasy-character/ancient-earth-golem-v2)
- [Shadow-golem-elemental](https://www.cgtrader.com/free-3d-models/character/fantasy-character/shadow-golem-elemental)
- [Earth-golem-elemental](https://www.cgtrader.com/free-3d-models/character/fantasy-character/earth-golem-elemental)
- [Kong-2021-rig](https://www.cgtrader.com/free-3d-models/character/sci-fi-character/kong-2021-rig)
- [Shani](https://www.cgtrader.com/free-3d-models/character/woman/shani-3d-character)

You can also create your 3D model from a single image:

- *Stable Fast 3D*: <https://huggingface.co/spaces/stabilityai/stable-fast-3d>
- *Unique3D*: <https://huggingface.co/spaces/abreza/Unique3D>

To use it in *Blender*, create a `.blend` project and import the file. To render the texture:

1. Add a *Principled BSDF* material to the object
1. Create a *Color Attribute* in the *Shader Editor* view
1. Link the Color output of the *Color Attribute* to the *Base color* input of the *Principled BSDF*

You can improve details by cropping the source image on a detail and generate a model for this detail. Once both imported in *Blender*, melt them together.

Render the images without background by selecting png RVBA and disabling background (*Film* -> *Filter* -> *Transparent*). It avoids the creatures to have an ugly dark border. Then, to correctly separate the creature from the cyan area, in *GIMP*, apply the threeshold on the transparency by clicking on *Layer* -> *Transparency* -> *Alpha threeshold*.

The global FPS of the game is 10 f/s but you can render at a higher level and configure it in the `.json` files. We are not in the 1990's.

### IA render

You can also use an AI like *Flux* to generate the main creature representation: <https://huggingface.co/spaces/multimodalart/FLUX.1-merged>

Then you can add random animations for idle states with *SVD*: <https://huggingface.co/spaces/xi0v/Stable-Video-Diffusion-Img2Vid>

Most of the time, the creatures do not move more than one pixel in an idle animation. The reason may be to avoid too much animation on screen and make the transition with the other animations always seamless. Use poses with *ControlNet* or *OpenPose*. For specific animations, I recommend to use *Cinemo* because it adds a description prompt but the resolution is smaller: <https://huggingface.co/spaces/maxin-cn/Cinemo>

Make animations seamless from one to another. To do this, you can draw the first and the last images with a prompt with *ToonCrafter*: <https://huggingface.co/spaces/ChristianHappy/tooncrafter>

Most of the time, you need to increase the resolution or the quality of your template image, so use *SUPIR*: <https://huggingface.co/spaces/Fabrice-TIERCELIN/SUPIR>

## Battle sound effect

To create the audio effects, I recommend to use *Tango 2*: <https://huggingface.co/spaces/declare-lab/tango2>

The quality is better than *Stable Audio*.

## Map render

We don't know the right elevation angle for the view but 45° elevation seems to be a good choice. For the sunlight direction, I would say 45° elevation and 45° azimut.

The map creatures are not rendered on the map with vanishing points but in isometric. You can [get an orthogonal render in Blender](https://blender.stackexchange.com/a/135384/2768). If you are creating a creature and its updated version, most of the time, the both creatures are not oriented to the same side on the map. I think that the animation on the map is usually the *Mouse Over* animation on battle.

You can see that the view angle is higher than on a battle. To change the angle from a battle sprite, you can use *Zero 1-to-3*: <https://huggingface.co/spaces/cvlab/zero123-live>

You can get higher resolution using this Video AI that can control the motion of the camera: <https://huggingface.co/spaces/TencentARC/MotionCtrl_SVD>

If you have a 3D software, you can get better quality by converting your image into 3D model and then render it from another angle using *Stable Fast 3D*: <https://huggingface.co/spaces/stabilityai/stable-fast-3d>

Follow this comment to retrieve the color: <https://huggingface.co/stabilityai/TripoSR/discussions/1#65e8a8e5e214f37d85dad366>

### Shadow render

There are no strong rules in the original game about the angle of the shadows on the map. Different buildings have inconsistent shadows. To draw the shadow, I recommend the following technique:

Let's consider that the object is a vertical cone:

| | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |

Locate the top and its projection to the ground:

| | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟥 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | 🟥 | ‍⬛ | ‍⬛ |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |

Then draw a rectangle triangle on the left:

| | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | 💟 | ‍⬛ | ‍⬛ |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |

The square top is the projection of the shadow of the top of the cone:

| | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |

Then you can draw the rest of the shadow:

| | | | | | | | | | |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 💟 | 🟪 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 💟 | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ | ‍⬛ |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | ‍⬛ | ‍⬛ | ‍⬛ | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
| 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 | 🟦 |
