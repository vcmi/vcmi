# VCMI Engine API Reference

This document provides a comprehensive reference for developers working with the VCMI engine APIs.

## Table of Contents

- [Core Architecture](#core-architecture)
- [Game State API](#game-state-api)
- [Object System](#object-system)
- [Bonus System API](#bonus-system-api)
- [Scripting API](#scripting-api)
- [Networking API](#networking-api)
- [Serialization API](#serialization-api)
- [Graphics and UI](#graphics-and-ui)

## Core Architecture

### Client-Server Model

VCMI uses a client-server architecture where:
- **Server** (`VCMI_server`) handles game logic, state management, and validation
- **Client** (`VCMI_client`) handles user interface, input, and rendering
- **Lib** (`VCMI_lib`) contains shared code used by both client and server

### Key Base Classes

#### CGameState
Main class containing the complete game state.

```cpp
class CGameState
{
public:
    std::unique_ptr<CMap> map;
    std::array<PlayerState, PlayerColor::PLAYER_LIMIT_I> players;
    BattleInfo *curB; // current battle
    
    // Game state queries
    const CGHeroInstance * getHero(ObjectInstanceID objid) const;
    const CGTownInstance * getTown(ObjectInstanceID objid) const;
    // ... other methods
};
```

#### CGObjectInstance
Base class for all map objects.

```cpp
class CGObjectInstance : public IObjectInterface
{
public:
    ObjectInstanceID id;
    Obj ID; // object type
    si32 subID; // object subtype
    int3 pos; // position on map
    
    virtual void onHeroVisit(const CGHeroInstance * h) const;
    virtual void initObj(vstd::RNG & rand);
    // ... other virtual methods
};
```

## Game State API

### Player Management

```cpp
// Get player information
const PlayerState * getPlayerState(PlayerColor player) const;

// Check player status
bool isPlayerActive(PlayerColor player) const;
```

### Hero Management

```cpp
// Create and manage heroes
CGHeroInstance * createHero(HeroTypeID type, PlayerColor owner);

// Hero movement and actions
void moveHero(ObjectInstanceID hid, int3 dst, ui8 teleporting);
void giveHeroNewArtifact(const CGHeroInstance *h, const CArtifact *art, ArtifactPosition pos);
```

### Town Management

```cpp
// Building construction
void buildStructure(ObjectInstanceID tid, BuildingID building, bool force = false);

// Creature recruitment
void recruitCreatures(ObjectInstanceID obj, ObjectInstanceID dst, CreatureID crid, ui32 cram, si32 level);
```

## Object System

### Creating Custom Objects

To create a custom map object:

1. Inherit from `CGObjectInstance`
2. Override required virtual methods
3. Register the object type in `RegisterTypes.h`

```cpp
class MyCustomObject : public CGObjectInstance
{
public:
    void onHeroVisit(const CGHeroInstance * h) const override;
    void initObj(vstd::RNG & rand) override;
    
    template<typename Handler>
    void serialize(Handler &h, const int version)
    {
        h & static_cast<CGObjectInstance&>(*this);
        // serialize custom data
    }
};
```

### Object Interaction

```cpp
// Visit callbacks
virtual void onHeroVisit(const CGHeroInstance * h) const;
virtual void onHeroLeave(const CGHeroInstance * h) const;

// Battle integration
virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
```

## Bonus System API

The bonus system is VCMI's core mechanism for handling all stat modifications and special abilities.

### Basic Bonus Usage

```cpp
// Create a bonus
std::shared_ptr<Bonus> bonus = std::make_shared<Bonus>();
bonus->type = Bonus::PRIMARY_SKILL;
bonus->subtype = PrimarySkill::ATTACK;
bonus->val = 5; // +5 attack

// Apply to object
bonusSystemNode->addNewBonus(bonus);
```

### Bonus Types

- `PRIMARY_SKILL` - Hero primary skills (Attack, Defense, Spell Power, Knowledge)
- `SECONDARY_SKILL_PREMY` - Secondary skill bonuses
- `STACKS_SPEED` - Stack speed modifications
- `MAGIC_RESISTANCE` - Magic resistance
- And many more...

For complete reference, see [Bonus System](Bonus_System.md) documentation.

## Scripting API

### Lua Integration

VCMI supports Lua scripting for game logic extension.

#### Basic Script Structure

```lua
local function onVisit(player, object)
    -- Script logic here
    return true
end

return {
    onVisit = onVisit
}
```

#### Available APIs

```lua
-- Game state access
local hero = game:getHero(heroId)
local town = game:getTown(townId)

-- Object manipulation
object:addReward(reward)
object:setProperty(property, value)

-- Player interaction
player:addResource(resourceType, amount)
player:giveArtifact(artifactId)
```

For detailed scripting reference, see [Lua Scripting System](Lua_Scripting_System.md).

## Networking API

### Network Packets

All communication between client and server uses serializable packet classes inheriting from `CPack`.

#### Creating Custom Packets

```cpp
class MyCustomPack : public CPack
{
public:
    ObjectInstanceID objectId;
    si32 customData;
    
    void applyGs(CGameState *gs); // Server-side application
    
    template<typename Handler>
    void serialize(Handler &h, const int version)
    {
        h & objectId;
        h & customData;
    }
};
```

#### Sending Packets

```cpp
// From client to server
sendRequest(new MyCustomPack(objectId, data));

// From server to client
sendToAllClients(new MyCustomPack(objectId, data));
```

## Serialization API

VCMI uses a custom serialization system for save games and network communication.

### Making Objects Serializable

```cpp
template<typename Handler>
void serialize(Handler &h, const int version)
{
    h & member1;
    h & member2;
    
    // Conditional serialization based on version
    if(version >= SOME_VERSION)
        h & newMember;
}
```

### Polymorphic Serialization

For objects that need polymorphic serialization:

1. Register the type in `RegisterTypes.h`
2. Ensure proper inheritance from base serializable class

For complete serialization guide, see [Serialization](Serialization.md).

## Graphics and UI

### Interface Objects

All UI elements inherit from `CIntObject`:

```cpp
class MyWidget : public CIntObject
{
public:
    void showAll(SDL_Surface * to) override; // Rendering
    void clickLeft(tribool down, bool previousState) override; // Mouse handling
    void keyPressed(const SDL_KeyboardEvent & key) override; // Keyboard handling
};
```

### Resource Management

```cpp
// Loading graphics
auto texture = GH.renderHandler().loadImage("path/to/image.png");

// Animation handling
auto animation = std::make_shared<CAnimation>("CREATURE");
animation->setCustom("MY_CREATURE", 0, 0);
```

## Common Patterns

### Error Handling

```cpp
// Use VCMI logging system
logGlobal->error("Error message: %s", errorDetails);
logGlobal->warn("Warning message");
logGlobal->info("Info message");
```

### Memory Management

- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`) for automatic memory management
- Follow RAII principles
- Be careful with circular references in bonus system

### Thread Safety

- Game state modifications must happen on the main thread
- Use proper synchronization for multi-threaded operations
- Client and server run in separate processes, not threads

## See Also

- [Code Structure](Code_Structure.md) - Overall architecture overview
- [Bonus System](Bonus_System.md) - Detailed bonus system documentation
- [Serialization](Serialization.md) - Serialization system details
- [Networking](Networking.md) - Network protocol documentation
- [Lua Scripting System](Lua_Scripting_System.md) - Scripting API details