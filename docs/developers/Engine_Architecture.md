# Engine Architecture Overview

This document provides a high-level overview of VCMI's engine architecture, explaining how the major systems interact and the design principles behind them.

## Core Design Principles

### 1. Client-Server Architecture
VCMI separates game logic from presentation using a strict client-server model:
- **Server** (`VCMI_server`) - Authoritative game state, AI, validation
- **Client** (`VCMI_client`) - User interface, graphics, input handling
- **Shared Library** (`VCMI_lib`) - Common code, data structures, utilities

### 2. Deterministic Simulation
The game state must be completely deterministic for:
- Save/load compatibility
- Network multiplayer synchronization
- Replay functionality
- AI consistency

### 3. Data-Driven Design
Game content is defined through JSON configuration files:
- Heroes, creatures, spells, artifacts
- Map objects and their behaviors
- UI layouts and graphics
- Mod support through config overrides

## System Architecture

```
┌─────────────────┐    ┌─────────────────┐
│   VCMI Client   │    │   VCMI Server   │
│                 │    │                 │
│ ┌─────────────┐ │    │ ┌─────────────┐ │
│ │ UI Systems  │ │    │ │ Game Logic  │ │
│ │ - Windows   │ │    │ │ - Mechanics │ │
│ │ - Adventure │ │    │ │ - Validation│ │
│ │ - Battle    │ │◄──►│ │ - Queries   │ │
│ │ - Launcher  │ │    │ └─────────────┘ │
│ └─────────────┘ │    │ ┌─────────────┐ │
│ ┌─────────────┐ │    │ │ AI Systems  │ │
│ │ Graphics    │ │    │ │ - VCAI      │ │
│ │ - SDL2      │ │    │ │ - Nullkiller│ │
│ │ - Rendering │ │    │ │ - Battle AI │ │
│ │ - Animation │ │    │ └─────────────┘ │
│ └─────────────┘ │    └─────────────────┘
└─────────────────┘            │
         │                     │
         └──────────┬──────────┘
                    │
         ┌─────────────────┐
         │   VCMI Library  │
         │                 │
         │ ┌─────────────┐ │
         │ │ Game State  │ │
         │ │ - CGameState│ │
         │ │ - Map Data  │ │
         │ │ - Players   │ │
         │ └─────────────┘ │
         │ ┌─────────────┐ │
         │ │ Bonus System│ │
         │ │ - Modifiers │ │
         │ │ - Effects   │ │
         │ │ - Artifacts │ │
         │ └─────────────┘ │
         │ ┌─────────────┐ │
         │ │ Serializer  │ │
         │ │ - Save/Load │ │
         │ │ - Network   │ │
         │ │ - Packets   │ │
         │ └─────────────┘ │
         └─────────────────┘
```

## Core Components

### Game State (`CGameState`)
The authoritative representation of the game world:

```cpp
class CGameState
{
    std::unique_ptr<CMap> map;                    // Game map
    std::array<PlayerState, PLAYER_LIMIT> players; // Player states
    std::unique_ptr<BattleInfo> curB;             // Current battle
    si32 day;                                     // Current day
    // ... other game state
};
```

**Responsibilities:**
- Maintains all game objects and their states
- Provides query interfaces for game information
- Ensures state consistency and validation
- Handles turn progression and time management

### Map System (`CMap`)
Represents the game world layout and objects:

```cpp
class CMap
{
    int3 size;                              // Map dimensions
    TerrainTile*** terrain;                 // Terrain grid
    std::vector<CGObjectInstance*> objects; // Map objects
    std::vector<CGHeroInstance*> heroes;    // All heroes
    std::vector<CGTownInstance*> towns;     // All towns
    // ... other map data
};
```

**Key Features:**
- 3D coordinate system (x, y, underground level)
- Dynamic object placement and removal
- Pathfinding integration
- Fog of war management

### Bonus System
The core mechanic for all stat modifications and special abilities:

```cpp
class Bonus
{
    BonusType type;        // What kind of bonus
    si32 val;             // Numeric value
    si32 subtype;         // Bonus subtype
    BonusDuration duration; // How long it lasts
    // ... bonus properties
};
```

**Examples:**
- Hero primary skills: `+5 Attack`
- Artifact effects: `+2 to all skills`
- Spell effects: `+50% Magic Resistance`
- Terrain bonuses: `+1 Defense on plains`

**Benefits:**
- Unified system for all game effects
- Automatic stacking and conflict resolution
- Save/load compatible
- Easily extensible for mods

### Object System (`CGObjectInstance`)
Base class for all interactive map objects:

```cpp
class CGObjectInstance
{
    ObjectInstanceID id;    // Unique identifier
    Obj ID;                // Object type (town, hero, mine, etc.)
    si32 subID;            // Object subtype
    int3 pos;              // Map position
    
    virtual void onHeroVisit(const CGHeroInstance * h) const;
    virtual void newTurn() const;
    virtual void initObj(vstd::RNG & rand);
};
```

**Object Types:**
- **Heroes** (`CGHeroInstance`) - Movable units with armies
- **Towns** (`CGTownInstance`) - Buildable settlements
- **Dwellings** (`CGDwelling`) - Creature recruitment
- **Resources** (`CGResource`) - Collectible resources
- **Artifacts** (`CGArtifact`) - Collectible items
- **Custom Objects** - Scripted behaviors

## Communication Architecture

### Network Protocol
Client-server communication uses typed message packets:

```cpp
class CPack // Base packet class
{
    virtual void applyGs(CGameState *gs) = 0; // Server application
    virtual void executeOnClient() = 0;       // Client application
};
```

**Packet Flow:**
1. **Client Request** → Server (user actions)
2. **Server Validation** → Process or reject
3. **Server Response** → All clients (state changes)
4. **Client Update** → UI reflects new state

**Example Packets:**
- `MoveHero` - Hero movement request
- `BuyArtifact` - Purchase from market
- `NewTurn` - Turn progression notification
- `BattleStart` - Combat initiation

### Serialization System
Unified system for save/load and network communication:

```cpp
template<typename Handler>
void serialize(Handler &h, const int version)
{
    h & membervariable1;
    h & membervariable2;
    // ... serialize all members
}
```

**Features:**
- Automatic type registration
- Version compatibility handling
- Polymorphic object support
- Cross-platform compatibility

## Subsystem Details

### Battle System
Turn-based tactical combat with hex grid:

**Components:**
- `BattleInfo` - Battle state and rules
- `CStack` - Individual creature stacks
- `BattleAction` - Player/AI commands
- `BattleSpellMechanics` - Spell effects

**Flow:**
1. Battle initialization from map encounter
2. Turn-based stack activation
3. Action processing (move, attack, cast)
4. Effect resolution and damage calculation
5. Victory condition checking

### AI Framework
Pluggable AI system with multiple implementations:

**VCAI (Default AI):**
- Goal-driven decision making
- Resource and building optimization
- Army composition strategies
- Adventure map exploration

**Nullkiller AI:**
- Modern rewrite of VCAI
- Improved performance and decision quality
- Better pathfinding and strategic planning

### Graphics System
SDL2-based rendering with custom optimizations:

**Components:**
- Texture management and caching
- Animation system with frame interpolation
- UI widget library (`CIntObject` hierarchy)
- Screen resolution scaling

**Rendering Pipeline:**
1. Adventure map background
2. Object sprites (heroes, buildings, etc.)
3. UI overlays (windows, dialogs)
4. Effect animations (spells, combat)

### Configuration System
JSON-based data definition for game content:

**Structure:**
```
config/
├── creatures.json     # Creature definitions
├── heroes.json        # Hero definitions
├── spells.json        # Spell definitions
├── artifacts.json     # Artifact definitions
├── objects/           # Map object configurations
└── schemas/           # JSON validation schemas
```

**Benefits:**
- Easy modding without code changes
- Translation support
- Version control friendly
- Validation and error detection

## Data Flow Examples

### Hero Movement
1. **Client**: User clicks on map tile
2. **Client**: Sends `MoveHero` packet to server
3. **Server**: Validates movement legality
4. **Server**: Updates game state with new position
5. **Server**: Sends `MoveHero` confirmation to all clients
6. **Clients**: Update hero position in UI

### Combat Resolution
1. **Client**: Player selects attack target
2. **Client**: Sends `BattleAction` to server
3. **Server**: Calculates damage using bonus system
4. **Server**: Applies damage to target stack
5. **Server**: Sends `BattleAttack` result to all clients
6. **Clients**: Play attack animation and update HP

### Save Game Creation
1. **Client**: Requests save game creation
2. **Server**: Serializes complete `CGameState` 
3. **Server**: Writes binary data to file
4. **Server**: Confirms save completion
5. **Client**: Updates UI with save confirmation

## Extension Points

### Custom Map Objects
1. Create class inheriting from `CGObjectInstance`
2. Override virtual methods (`onHeroVisit`, etc.)
3. Register type in `RegisterTypes.h`
4. Add configuration in `config/objects/`

### New Bonus Types
1. Add enum value to `BonusType`
2. Handle in calculation code
3. Add configuration entries
4. Update UI descriptions

### Scripting Integration
1. Lua scripts for object behaviors
2. Event-driven architecture
3. Sandbox execution environment
4. Game state access APIs

## Performance Considerations

### Memory Management
- Smart pointers for automatic cleanup
- Object pooling for frequently created objects
- Copy-on-write for large data structures
- Reference counting for shared resources

### Computational Efficiency
- Pathfinding caching and optimization
- Bonus system lazy evaluation
- Graphics texture atlasing
- Battle calculation vectorization

### Network Optimization
- Packet compression for large data
- Delta updates for state changes
- Prediction for smooth movement
- Connection timeout handling

## Development Guidelines

### Adding New Features
1. **Design Review** - Discuss architecture impact
2. **Interface Definition** - Define clear APIs
3. **Implementation** - Follow coding standards
4. **Testing** - Unit and integration tests
5. **Documentation** - Update relevant docs

### Debugging Strategies
1. **Logging** - Use appropriate log levels
2. **Assertions** - Validate assumptions
3. **Reproducibility** - Ensure deterministic behavior
4. **Isolation** - Test components independently

### Maintenance Practices
1. **Code Review** - Peer review all changes
2. **Refactoring** - Improve design over time
3. **Performance Monitoring** - Profile critical paths
4. **Technical Debt** - Address accumulated issues

This architecture has evolved over many years to support the complexity of Heroes of Might & Magic III while remaining maintainable and extensible. Understanding these core concepts will help you navigate the codebase and contribute effectively to the project.