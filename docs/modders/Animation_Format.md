# Animation Format

VCMI allows overriding HoMM3 .def files with .json replacement. Compared to .def this format allows:

- Overriding individual frames from json file (e.g. icons)
- Modern graphics formats (targa, png - all formats supported by VCMI image loader)
- Does not requires any special tools - all you need is text editor and images.

## Format description

```json
{
    // Base path of all images in animation. Optional.
    // Can be used to avoid using long path to images 
    "basepath" : "path/to/images/directory/",

    // List of sequiences / groups in animation
    // This will replace original group with specified list of files
    // even if original animation is longer
    "sequences" :
    [
        {
            // Index of group, zero-based
            "group" : 1,

            // List of files in this group
            "frames" :
            [
                "frame1.png",
                "frame2.png"
                    ...
            ],

            // Automatically create shadow for this frame if required. Optional, 0 = None, 1 = Normal Shadow, 2 = Sheared Shadow (e.g. for adventure map)
            "generateShadow" : 1,

            // Automatically create overlay for this frame if required. Optional, 0 = None, 1 = Outline
            "generateOverlay" : 1,
        },
        ...
    ],

    // Alternative to "sequences". Allow overriding individual frames in file. Generally should not be used in the same time as "sequences"
    "images" :
    [
        {
            // Group of this image. Optional, default = 0
            "group" : 0,

            // Index of the image in group
            "frame" : 0,

            // Filename for this frame
            "file" : "filename.png",

            // Automatically create shadow for this frame if required. Optional, 0 = None, 1 = Normal Shadow, 2 = Sheared Shadow (e.g. for adventure map)
            "generateShadow" : 1,

            // Automatically create overlay for this frame if required. Optional, 0 = None, 1 = Outline
            "generateOverlay" : 1,
        }.
        ...
    ]

}
```

## Examples

### Replacing a button

This json file will allow replacing .def file for a button with png images. Buttons require following images:

1. Active state. Button is active and can be pressed by player
2. Pressed state. Player pressed button but have not released it yet
3. Blocked state. Button is blocked and can not be interacted with. Note that some buttons are never blocked and can be used without this image
4. Highlighted state. This state is used by only some buttons and only in some cases. For example, in main menu buttons will appear highlighted when mouse cursor is on top of the image. Another example is buttons that can be selected, such as settings that can be toggled on or off

```json
{
	"basepath" : "interface/MyButton", // all images are located in this directory

	"images" :
	[
		{"frame" : 0, "file" : "active.png" },
		{"frame" : 1, "file" : "pressed.png" },
		{"frame" : 2, "file" : "blocked.png" },
		{"frame" : 3, "file" : "highlighted.png" },
	]
}
```

### Replacing simple animation

This json file allows defining one animation sequence, for example for adventure map objects or for town buildings.

```json
{
	"basepath" : "myTown/myBuilding", // all images are located in this directory

	"sequences" :
	[
		{
			"group" : 0,
			"frames" : [
				"frame01.png",
				"frame02.png",
				"frame03.png",
				"frame04.png",
				"frame05.png"
				...
			]
		}
	]
}
```

### Replacing creature animation

TODO

## Creature animation groups

Animation for creatures consist from multiple groups, with each group
representing one specific animation. VCMI uses groups as follows:

**Basic animations**

- [0] Movement: Used for creature movement
- [1] Mouse over: Used for random idle movements and when mouse is moved on the creature
- [2] Idle: Basic animation that plays continuously when stack is not acting
- [3] Hitted: Animation that plays whenever stack is hit
- [4] Defence: Alternative animation that plays when stack is defending and was hit in melee
- [5] Death: Animation that plays when stack dies
- [6] Death (ranged): Alternative animation, plays when stack is killed by ranged attack

**Rotation animations**

- [7] Turn left: Animation for rotating stack, only contains first part of animation, with stack turning towards viewer
- [8] Turn right: Second part of animation for rotating stack
- [9] (unused in vcmi, present in H3 files)
- [10] (unused in vcmi, present in H3 files)

**Melee attack animations**

- [11] Attack (up): Attacking animation, stack facing upwards
- [12] Attack (front): Attacking animation, stack facing front
- [13] Attack (down): Attacking animation, stack facing downwards

**Ranged attack animations**

- [14] Shooting (up): Ranged attack animation, stack facing upwards
- [15] Shooting (front): Ranged attack animation, stack facing front
- [16] Shooting (down): Ranged attack animation, stack facing downwards

**Special animations**

- [17] Special (up): Special animation, used if dedicated cast or group attack animations were not found
- [18] Special (front): Special animation, used if dedicated cast or group attack animations were not found
- [19] Special (down): Special animation, used if dedicated cast or group attack animations were not found

**Additional H3 animations**

- [20] Movement start: Animation that plays before movement animation starts.
- [21] Movement end: Animation that plays after movement animation ends.

**Additional VCMI animations**

- [22] Dead: Animation that plays when creature is dead. If not present, will consist from last frame from "Death" group
- [23] Dead (ranged): Animation that plays when creature is dead after ranged attack. If not present, will consist from last frame from "Death (ranged)" group
- [24] Resurrection: Animation that plays when creature is resurrected. If not present, will consist from reversed version of "Death" animation

**Spellcasting animations**

- [30] Cast (up): Used when creature casts spell facing upwards
- [31] Cast (front): Used when creature casts spell facing front
- [32] Cast (down): Used when creature casts spell facing downwards

**Group attack animations**

- [40] Group Attack (up): Used when creature attacks multiple target, with primary target facing up (Dragon Breath attack, or creatures like Hydra)
- [41]  Group Attack (front): Used when creature attacks multiple target, with primary target facing front (Dragon Breath attack, or creatures like Hydra)
- [42] Group Attack (down): Used when creature attacks multiple target, with primary target facing downwards (Dragon Breath attack, or creatures like Hydra)
