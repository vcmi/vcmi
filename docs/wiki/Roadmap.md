## Game engine features
### [Support remaining H3 features](https://github.com/vcmi/vcmi/labels/h3feature)
- **Description**: Original goal of vcmi project, as of right now almost all H3 features are supported. However, most of remaining features often require somewhat extensive changes.
- **Current estimation**: vcmi-1.2 (Battles), unknown(Full support)
- **Priority**: medium
- **Current assignee**: [IvanSavenko](https://github.com/IvanSavenko)
### [SDL abstraction layer & hardware acceleration](https://github.com/vcmi/vcmi/issues/1263)
- **Description**: While hardware acceleration is not critically important for vcmi, most of preceding steps are necessary for general code improvement.
- **Current estimation**: vcmi-1.3 (Testing), unknown (Full support)
- **Priority**: medium
- **Current assignee**: [IvanSavenko](https://github.com/IvanSavenko)

## Mobile Platforms
### [Adopt interface to touchscreen devices](https://github.com/vcmi/vcmi/labels/touchscreen)
- TODO: who among devs is interested in this? When do we expect any progress on this?
### Closer and frequent integration with distributing platforms (google store, altstore, ideally appstore)
- TODO: what exactly is "Closer and frequent integration"? What blocks us from doing this? 
- TODO: Who among devs is interested in this? When do we expect any progress on this?
### Port VCMI launcher to Android
- TODO: What is current state with Android Launcher? What blocks us from doing this?
- TODO: Who among devs is interested in this? When do we expect any progress on this?
## Hota/HDMod features
- [Better online mode](https://github.com/vcmi/vcmi/issues/1361)
- [Quick battle](https://github.com/vcmi/vcmi/pull/1108)
- Simultaneous turns
- [Support Hota map format](https://github.com/vcmi/vcmi/issues/1259)

## Modding System
### Flexibility for configurable objects 
  - TODO: describe what else needs to be exposed to modding system
### [Sod as mod](https://github.com/vcmi/vcmi/milestone/3)
  - Details and solution description are [here](https://github.com/vcmi/vcmi/issues/1158)
### [Map editor](https://github.com/vcmi/vcmi/labels/editor)
  - [List of tasks](https://github.com/Nordsoft91/vcmi/issues) to be done in originator's fork
### Implement other modding tools
  - Resource viewer embedded into map editor
  - Campaign editor
  - RMG template editor
### Scripting
- Description: Fundaments for scripting support is made, the scripting features just need to be gradually implemented
- Potential assignee: None?
### Missing WoG features
- Descriptions: Implement some missing WoG features like reviving commanders for money
- Potential assignee: None?

## AI
- [Battle AI](https://github.com/vcmi/vcmi/labels/battleai)
  - Tactics ability
  - Utilize more spells
  - Proper moment for retreat/surrender
  - Use some battle tricks (surround shooters, etc)
- [Adventure AI](https://github.com/vcmi/vcmi/labels/nullkillerai)

## Easy of development
- Automatic test system
  - Run unit tests on GitHub actions - implement resources stub for it
- Automatic data installation
  - Installation wizard from launcher which copies necessary resources automatically
- Better documentation for code and for modders

## Exclusive Content
- High quality mods at scale of add-ons, which should contain
  - New content (factions, units, spells, terrains whatever), 
  - Unique scenarios, campaigns, story
  - New game mechanics (random idea - add level above ground “sky” with “skyboat” transport)

## Community
- marketing and finding manpower - linux & open source enthusiast communities could be good start to find h3 fan devs