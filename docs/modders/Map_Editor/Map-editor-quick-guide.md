# Interface

<img width="738" alt="Снимок экрана 2022-09-07 в 06 31 12" src="https://user-images.githubusercontent.com/9308612/188775648-8551107d-9f0b-4743-8980-56c2c1c58bbc.png">

# Create the map

<img width="145" alt="Снимок экрана 2022-09-07 в 07 28 05" src="https://user-images.githubusercontent.com/9308612/188782248-b7895eb1-86a9-4f06-9309-43c6b3a3a728.png">

## New map

Create the new map by pressing **New** button from the toolbar

### Empty map

To create empty map, define its size by choosing option from drop-down list or enter required size manually in the text fields and press Ok button. Check **Two level map** option to create map with underground.
`Note: there are no limits on map size but be careful with sizes larger predefined XL size. It will be processed quite long to create even empty map. Also, it will be difficult to work with the huge maps because of possible performance issues`

Other parameters won't be used for empty map.

<img width="413" alt="Снимок экрана 2022-09-07 в 07 31 07" src="https://user-images.githubusercontent.com/9308612/188782588-c9f1a164-df1e-46bc-a6d3-24ff1e5396c2.png">

### Random map

To generate random map, check the **Random map** option and configure map parameters. You can select template from the drop-down list.

<img width="439" alt="Снимок экрана 2022-09-07 в 07 36 34" src="https://user-images.githubusercontent.com/9308612/188783256-7e498238-14a0-4377-8c09-d702d54ee7d9.png">

Templates are dynamically filtered depending on parameters you choose.

* [Default] template means that template will be randomly chosen
* If you see empty list it means that there are no templates fit your settings. It could be related to the settings chosen or it can mean that there are no templates install.

<img width="412" alt="Снимок экрана 2022-09-07 в 07 37 35" src="https://user-images.githubusercontent.com/9308612/188783360-1c042782-328d-4692-952f-58fa5110d0d0.png">

## Map load & save

To load the map, press open and select map file from the browser.

You can load both *.h3m and *.vmap formats but for saving *.vmap is allowed only.

# Views

There are 3 buttons switching views <img width="131" alt="Снимок экрана 2022-09-07 в 06 48 08" src="https://user-images.githubusercontent.com/9308612/188777527-b7106146-0d8c-4f14-b78d-f13512bc7bad.png">

### Ground/underground

**"U/G"** switches you between ground and underground

### Grid view

**Grid** show/hide grid

<img width="153" alt="Снимок экрана 2022-09-07 в 06 49 39" src="https://user-images.githubusercontent.com/9308612/188777723-934d693b-247d-42a6-815c-aecd0d34f653.png">

### Passability view

**Pass** show/hide passability map

<img width="190" alt="Снимок экрана 2022-09-07 в 06 51 46" src="https://user-images.githubusercontent.com/9308612/188778010-a1d45d59-7333-4432-b83f-57190fbe09f4.png">

# Setup terrain

1. Select brush you want
<img width="124" alt="Снимок экрана 2022-09-07 в 06 38 46" src="https://user-images.githubusercontent.com/9308612/188776299-fd688696-a98d-4f89-8bef-e81c90d3724b.png">

2. Select area you'd like to change

`Note: left mouse button selects tiles, right button removes selection`

![Снимок экрана 2022-09-07 в 06 39 53](https://user-images.githubusercontent.com/9308612/188776895-ce1607fb-694b-4edb-b0cc-263a0c5d04b8.png)

3. Press terrain you want

<img width="116" alt="Снимок экрана 2022-09-07 в 06 45 49" src="https://user-images.githubusercontent.com/9308612/188777176-1167561a-f23c-400a-a57b-be7fc90accae.png">

### Drawing roads and rivers

Actually, the process to draw rivers or roads is exactly the same as for terrains. You need to select tiles and then choose road/river type from the panel. 

<img width="232" alt="Снимок экрана 2022-09-13 в 21 25 18" src="https://user-images.githubusercontent.com/9308612/189968965-7adbd76c-9ffc-46e5-aeb7-9a79214cc506.png">

To erase roads or rivers, you need to select tiles to be cleaned and press empty button.

<img width="130" alt="Снимок экрана 2022-09-13 в 21 26 54" src="https://user-images.githubusercontent.com/9308612/189969309-87ff7818-2915-4b38-b9db-ec4a616d18e7.png">

_Erasing works either for roads or for rivers, e.g. empty button from the roads tab erases roads only, but not rivers. You also can safely select bigger area, because it won't erase anything on tiles without roads/rivers accordingly_

## About brushes
* Buttons "1", "2", "4" - 1x1, 2x2, 4x4 brush sizes accordingly
* Button "[]" - non-additive rectangle selection
* Button "O" - lasso brush (not implemented yet)
* Button "E" - object erase, not a brush


## Fill obstacles

Map editor supports automatic obstacle placement.
Obstacle types are automatically selected for appropriate terrain types

To do that, select area (see Setup terrains) and press **Fill** button from the toolbar
<img width="222" alt="Снимок экрана 2022-09-07 в 06 56 05" src="https://user-images.githubusercontent.com/9308612/188778572-f80b3706-1713-40c7-a5aa-e4c3b9fc6649.png">

<img width="335" alt="Снимок экрана 2022-09-07 в 06 57 23" src="https://user-images.githubusercontent.com/9308612/188778694-507009a1-4bb0-4f37-afc7-4aa26a6f3eeb.png">

<img width="350" alt="Снимок экрана 2022-09-07 в 06 57 41" src="https://user-images.githubusercontent.com/9308612/188778728-f66adcbe-9cf2-41f2-8947-f60b38901317.png">

`Note: obstacle placer may occupy few neighbour tiles outside of selected area`

# Manipulating objects

## Adding new objects

1. Find the object you'd like to place in the object browser

<img width="188" alt="Снимок экрана 2022-09-07 в 07 00 59" src="https://user-images.githubusercontent.com/9308612/188779107-dc2ab212-9ed2-48ee-ba0d-7856b94a87b1.png">

2. You can also see selected object in preview area in the left part of application window

<img width="130" alt="Снимок экрана 2022-09-07 в 07 02 19" src="https://user-images.githubusercontent.com/9308612/188779263-a78577f7-8278-4927-a44e-18a7d85e2f64.png">

3. Hold mouse at object you want to place and move it to the map. You will see transparent object. Release object at point to confirm its creation

<img width="181" alt="Снимок экрана 2022-09-07 в 07 03 02" src="https://user-images.githubusercontent.com/9308612/188779350-653bad29-c54f-471b-93aa-90096da6d98c.png">

4. Press somewhere on the map to locate object.

**Right click over the scene - cancel object placement**

## Removing objects

1. **Make sure that no one terrain brush is selected.** To de-select brush click on selected brush again.

2. Select object you'd like to remove by simple clicking on it. You can also select multiple objects by moving mouse while left button is pressed
<img width="170" alt="Снимок экрана 2022-09-07 в 07 09 15" src="https://user-images.githubusercontent.com/9308612/188780096-6aefcad5-e092-44c6-8647-975e95f9a8a3.png">

3. Press **"E"** button from the brush panel or press **delete** on keyboard

## Changing object's properties

1. **Make sure that no one terrain brush is selected.** To de-select brush click on selected brush again.

2. Select object you'd like to remove by simple clicking on it. **You cannot review and modify properties for several objects, multiple selection is not supported.**

3. Go to the **inspector** tab. You will see object's properties

<img width="197" alt="Снимок экрана 2022-09-07 в 07 12 45" src="https://user-images.githubusercontent.com/9308612/188780504-40713019-3cdd-4077-9e2f-e3417c960efe.png">

4. You are able to modify properties which are not gray
`Note: sometimes there are empty editable fields`

### Assigning player to the object

Objects with flags can be assigned to the player. Find Owner property in the inspector for selected object, press twice to modify right cell. Type player number from **0 to 7 or type NEUTRAL** for neutral objects.

# Set up the map

You can modify general properties of the map

## Map name and description

1. Open **Map** menu on the top and select **General**

<img width="145" alt="Снимок экрана 2022-09-07 в 07 17 02" src="https://user-images.githubusercontent.com/9308612/188780943-70578b79-001b-45a2-bdfd-94577db40a6b.png">

2. You will see a new window with text fields to edit map name and description

3. Pressing **Ok** will save the changes, closing the window will discard the changes

<img width="307" alt="Снимок экрана 2022-09-07 в 07 18 02" src="https://user-images.githubusercontent.com/9308612/188781063-50ce65f8-aab4-43c3-a308-22acafa7d0a2.png">

## Player settings

Open **Map** menu on the top and select **Player settings" 

<img width="141" alt="Снимок экрана 2022-09-07 в 07 20 40" src="https://user-images.githubusercontent.com/9308612/188781357-4a6091cf-f175-4649-a000-620f306d2c46.png">

You will see a window with player settings. Combobox players defines amount of players on the map. To review settings for particular player scroll the internal window. There are bunch on settings for each player you can change.

`Important: at least one player must be controlled as Human/CPU. Maps without human players won't be started in the game at most cases`

<img width="641" alt="Снимок экрана 2022-09-07 в 07 21 05" src="https://user-images.githubusercontent.com/9308612/188781400-7d5ba463-4f82-4dba-83ff-56210995e2c7.png">
