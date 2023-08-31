VCMI allows overriding HoMM3 .def files with .json replacement. Compared
to .def this format allows:

-   Overriding individual frames from json file (e.g. icons)
-   Modern graphics formats (targa, png - all formats supported by VCMI
    image loader)
-   Does not requires any special tools - all you need is text editor
    and images.

# Format description

``` javascript
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
            ]
        },
        ...
    ],

    // Allow overriding individual frames in file
    "images" :
    [
        {
            // Group of this image. Optional, default = 0
            "group" : 0,

            // Imdex of the image in group
            "frame" : 0,

            // Filename for this frame
            "file" : "filename.png"
        }.
        ...
    ]

}
```

# Creature animation groups

Animation for creatures consist from multiple groups, with each group
representing specific one animation. VCMI uses groups as follows:

**Basic animations**

-   \[0\] Movement: Used for creature movement
-   \[1\] Mouse over: Used for random idle movements and when mouse is
    moved on the creature
-   \[2\] Idle: Basic animation that plays continuously when stack is
    not acting
-   \[3\] Hitted: Animation that plays whenever stack is hit
-   \[4\] Defence: Alternative animation that plays when stack is
    defending and was hit in melee
-   \[5\] Death: Animation that plays when stack dies
-   \[6\] Death (ranged): Alternative animation, plays when stack is
    killed by ranged attack

**Rotation animations**

-   \[7\] Turn left: Animation for rotating stack, only contains first
    part of animation, with stack turning towards viewer
-   \[8\] Turn right: Second part of animation for rotating stack
-   \[9\] (unused in vcmi, present in H3 files)
-   \[10\] (unused in vcmi, present in H3 files)

**Melee attack animations**

-   \[11\] Attack (up): Attacking animation, stack facing upwards
-   \[12\] Attack (front): Attacking animation, stack facing front
-   \[13\] Attack (down): Attacking animation, stack facing downwards

**Ranged attack animations**

-   \[14\] Shooting (up): Ranged attack animation, stack facing upwards
-   \[15\] Shooting (front): Ranged attack animation, stack facing front
-   \[16\] Shooting (down): Ranged attack animation, stack facing
    downwards

**Special animations**

-   \[17\] Special (up): Special animation, used if dedicated cast or
    group attack animations were not found
-   \[18\] Special (front): Special animation, used if dedicated cast or
    group attack animations were not found
-   \[19\] Special (down): Special animation, used if dedicated cast or
    group attack animations were not found

**Additional H3 animations**

-   \[20\] Movement start: Animation that plays before movement
    animation starts.
-   \[21\] Movement end: Animation that plays after movement animation
    ends.

**Additional VCMI animations**

-   \[22\] \[VCMI 1.0\] Dead: Animation that plays when creature is
    dead. If not present, will consist from last frame from "Death"
    group
-   \[23\] \[VCMI 1.0\] Dead (ranged): Animation that plays when
    creature is dead after ranged attack. If not present, will consist
    from last frame from "Death (ranged)" group
-   \[24\] \[VCMI 1.2\] Resurrection: Animation that plays when creature
    is resurrected. If not present, will consist from reversed version
    of "Death" animation

**Spellcasting animations**

-   \[30\] \[VCMI 1.2\] Cast (up): Used when creature casts spell facing
    upwards
-   \[31\] \[VCMI 1.2\] Cast (front): Used when creature casts spell
    facing front
-   \[32\] \[VCMI 1.2\] Cast (down): Used when creature casts spell
    facing downwards

**Group attack animations**

-   \[40\] \[VCMI 1.2\] Group Attack (up): Used when creature attacks
    multiple target, with primary target facing up (Dragon Breath
    attack, or creatures like Hydra)
-   \[41\] \[VCMI 1.2\] Group Attack (front): Used when creature attacks
    multiple target, with primary target facing front (Dragon Breath
    attack, or creatures like Hydra)
-   \[42\] \[VCMI 1.2\] Group Attack (down): Used when creature attacks
    multiple target, with primary target facing downwards (Dragon Breath
    attack, or creatures like Hydra)

# Proposed format extensions

## Texture atlas format

**TODO**

-   arbitrary texture coordinates
-   margins
-   grid-like layout

### Texture atlas format

``` javascript
{
    // Base path of all images in animation. Optional.
    // Can be used to avoid using long path to images 
    // If a path is a filename is is treated as single texture atlas
    "basepath" : "path/to/images/atlas/bitmap.png",

    // List of sequences / groups in animation
    "sequences" :
    [
        {
            // Index of group, zero-based
            "group" : 1,

            // List of files in this group
            "frames" :
            [
                {"x":0, "y":0, "w": 64, "h": 64},
                {"x":64, "y":0, "w": 64, "h": 64}
                    ...
            ]
        },
        ...
    ]
}
```

### Texture atlas grid format

``` javascript
{
    // filename is is treated as single texture atlas
    "basepath" : "path/to/images/atlas/bitmap.png",
    
    //optional, atlas is a grid with same size images
    //located left to right, top to bottom
    //[columns, rows]
    //default [1,1]
    "gridCols":3,
    "gridRows":3,

    // List of sequences / groups in animation
    // sequence index -> images count in sequence
    "sequences" :
    [
        1,3,5
    ]
}
```