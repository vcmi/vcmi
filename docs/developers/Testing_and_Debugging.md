# Testing and Debugging Guide

This guide covers testing strategies, debugging techniques, and troubleshooting for VCMI development.

## Table of Contents

- [Testing Framework](#testing-framework)
- [Unit Testing](#unit-testing)
- [Integration Testing](#integration-testing)
- [Manual Testing](#manual-testing)
- [Debugging Techniques](#debugging-techniques)
- [Performance Profiling](#performance-profiling)
- [Common Issues](#common-issues)

## Testing Framework

VCMI uses Google Test (gtest) for unit testing and custom frameworks for integration testing.

### Building with Tests

```bash
# Configure with tests enabled
cmake .. -DENABLE_TEST=ON -DCMAKE_BUILD_TYPE=Debug

# Build tests
cmake --build . --target vcmitest

# Run all tests
ctest --verbose
```

### Test Directory Structure

```
test/
├── googletest/          # Google Test framework
├── battle/             # Battle system tests
├── bonus/              # Bonus system tests
├── map/                # Map loading tests
├── serializer/         # Serialization tests
└── vcai/               # AI tests
```

## Unit Testing

### Writing Unit Tests

Create test files in the appropriate `test/` subdirectory:

```cpp
// test/bonus/BonusTest.cpp
#include <gtest/gtest.h>
#include "../../lib/bonus/Bonus.h"

TEST(BonusTest, BasicBonus)
{
    auto bonus = std::make_shared<Bonus>(Bonus::PRIMARY_SKILL, 5, PrimarySkill::ATTACK);
    
    EXPECT_EQ(bonus->type, Bonus::PRIMARY_SKILL);
    EXPECT_EQ(bonus->val, 5);
    EXPECT_EQ(bonus->subtype, PrimarySkill::ATTACK);
}

TEST(BonusTest, BonusStacking)
{
    CBonusSystemNode node;
    
    auto bonus1 = std::make_shared<Bonus>(Bonus::PRIMARY_SKILL, 3, PrimarySkill::ATTACK);
    auto bonus2 = std::make_shared<Bonus>(Bonus::PRIMARY_SKILL, 2, PrimarySkill::ATTACK);
    
    node.addNewBonus(bonus1);
    node.addNewBonus(bonus2);
    
    EXPECT_EQ(node.getBonuses(Selector::type(Bonus::PRIMARY_SKILL))->size(), 2);
}
```

### Test Fixtures

For complex setup, use test fixtures:

```cpp
class GameStateTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        gameState = std::make_unique<CGameState>();
        // Initialize test game state
    }
    
    void TearDown() override
    {
        gameState.reset();
    }
    
    std::unique_ptr<CGameState> gameState;
};

TEST_F(GameStateTest, HeroCreation)
{
    auto hero = gameState->createHero(HeroTypeID(0), PlayerColor::RED);
    ASSERT_NE(hero, nullptr);
    EXPECT_EQ(hero->tempOwner, PlayerColor::RED);
}
```

### Mocking

For testing components that depend on external systems:

```cpp
class MockGameCallback : public IGameCallback
{
public:
    MOCK_METHOD(void, giveResource, (PlayerColor player, Res::ERes which, int val));
    MOCK_METHOD(void, giveCreatures, (const CArmedInstance *objid, const CGHeroInstance * h, const CCreatureSet &creatures, bool remove));
    // ... other mock methods
};

TEST(HeroTest, GainExperience)
{
    MockGameCallback mockCb;
    CGHeroInstance hero;
    
    EXPECT_CALL(mockCb, giveResource(PlayerColor::RED, Res::EXPERIENCE, 1000));
    
    hero.gainExp(1000);
}
```

## Integration Testing

### Map Loading Tests

Test map loading and validation:

```cpp
TEST(MapTest, LoadValidMap)
{
    auto map = std::make_unique<CMap>();
    CMapLoaderH3M loader;
    
    EXPECT_NO_THROW(loader.loadMap("test/maps/valid_map.h3m", map.get()));
    EXPECT_GT(map->width, 0);
    EXPECT_GT(map->height, 0);
}
```

### Battle System Tests

Test battle mechanics:

```cpp
TEST(BattleTest, BasicCombat)
{
    // Set up battle
    BattleInfo battle;
    auto attacker = battle.getBattleHero(BattleSide::ATTACKER);
    auto defender = battle.getBattleHero(BattleSide::DEFENDER);
    
    // Test combat calculations
    auto damage = BattleInfo::calculateDmg(attacker, defender, true, false, battle.getStack(0));
    EXPECT_GT(damage.first, 0);
}
```

### Serialization Tests

Test save/load functionality:

```cpp
TEST(SerializationTest, SaveLoadGameState)
{
    CGameState originalState;
    // Initialize state...
    
    // Save to memory buffer
    std::ostringstream oss;
    COSer<CSaveFile> saver(oss);
    saver << originalState;
    
    // Load from buffer
    std::istringstream iss(oss.str());
    CISer<CLoadFile> loader(iss);
    CGameState loadedState;
    loader >> loadedState;
    
    // Verify state matches
    EXPECT_EQ(originalState.day, loadedState.day);
    EXPECT_EQ(originalState.players.size(), loadedState.players.size());
}
```

## Manual Testing

### Test Game Setup

1. **Create test scenarios** in `test/testdata/`:
```
testdata/
├── maps/               # Test maps
├── saves/              # Test save files
└── configs/            # Test configurations
```

2. **Use debug commands** in-game:
```
vcmiistari            # Enable debug mode
vcmiarmenelos        # Give all resources
vcmiainur            # Give all artifacts
```

### Cheat Codes for Testing

Enable cheats in game and use:
- `vcmiistari` - Enable other cheats
- `vcmiarmenelos` - Add 100000 of each resource
- `vcmiainur` - Give all artifacts
- `vcmiangband` - Give all spells
- `vcmilostsoul` - Lose game
- `vcmiwinwin` - Win game

### Custom Test Maps

Create minimal test maps for specific features:

```json
// minimal_test.json (map template)
{
    "size": { "width": 36, "height": 36, "depth": 1 },
    "players": ["red", "blue"],
    "objects": [
        {
            "type": "town",
            "subtype": "castle",
            "pos": [17, 17, 0],
            "template": "avctcas0.def",
            "owner": "red"
        }
    ]
}
```

## Debugging Techniques

### Debug Builds

Always use debug builds for development:

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug \
         -DCMAKE_CXX_FLAGS="-g3 -O0 -fno-omit-frame-pointer" \
         -DENABLE_DEBUG_CONSOLE=ON
```

### Logging

Use VCMI's logging system extensively:

```cpp
#include "lib/logging/CLogger.h"

// Different log levels
logGlobal->error("Critical error: %s", errorMessage);
logGlobal->warn("Warning: %s", warningMessage);
logGlobal->info("Info: %s", infoMessage);
logGlobal->debug("Debug: %s", debugMessage);
logGlobal->trace("Trace: %s", traceMessage);

// Category-specific logging
logAi->info("AI decision: %s", decision);
logNetwork->debug("Network packet: %s", packet);
```

### GDB Debugging (Linux/macOS)

```bash
# Start with GDB
gdb ./client/vcmiclient

# Set breakpoints
(gdb) break CGHeroInstance::gainExp
(gdb) break BattleInfo::calculateDmg

# Run with arguments
(gdb) run --testgame

# Examine variables
(gdb) print hero->name
(gdb) print *this

# Stack trace
(gdb) bt
(gdb) frame 3
```

### Visual Studio Debugging (Windows)

1. **Set up debugging:**
   - Generate VS project: `cmake .. -G "Visual Studio 16 2019"`
   - Set vcmiclient as startup project
   - Configure debugging arguments in project properties

2. **Useful debugging features:**
   - Set conditional breakpoints
   - Use the Autos/Locals windows
   - Watch expressions for complex objects
   - Memory dumps for pointer debugging

### Memory Debugging

#### Valgrind (Linux/macOS)

```bash
# Check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./client/vcmiclient

# Check for threading issues
valgrind --tool=helgrind ./client/vcmiclient
```

#### AddressSanitizer

```bash
# Build with AddressSanitizer
cmake .. -DCMAKE_CXX_FLAGS="-fsanitize=address -g"

# Run to detect memory errors
./client/vcmiclient
```

## Performance Profiling

### CPU Profiling

#### Using perf (Linux)

```bash
# Record performance data
perf record -g ./client/vcmiclient

# Analyze results
perf report

# Profile specific function
perf record -g -e cpu-clock --call-graph dwarf ./client/vcmiclient
```

#### Using Instruments (macOS)

1. Open Instruments app
2. Choose "Time Profiler" template
3. Target vcmiclient executable
4. Analyze call trees and hot spots

### Memory Profiling

#### Heap profiling with gperftools

```bash
# Build with profiling
cmake .. -DCMAKE_CXX_FLAGS="-lprofiler"

# Run with heap profiler
HEAPPROFILE=/tmp/vcmi.hprof ./client/vcmiclient

# Analyze heap usage
google-pprof --gv ./client/vcmiclient /tmp/vcmi.hprof.0001.heap
```

### Benchmarking

Create performance benchmarks for critical code:

```cpp
#include <chrono>

void benchmarkBattleCalculation()
{
    auto start = std::chrono::high_resolution_clock::now();
    
    // Run calculation 1000 times
    for(int i = 0; i < 1000; ++i)
    {
        auto damage = BattleInfo::calculateDmg(attacker, defender, true, false, stack);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Average calculation time: " << duration.count() / 1000.0 << " μs" << std::endl;
}
```

## Common Issues

### Build Issues

#### Missing Dependencies
```bash
# Check what's missing
cmake .. 2>&1 | grep -i "not found"

# Install missing packages (Ubuntu)
sudo apt install libsdl2-dev libboost-all-dev
```

#### CMake Cache Issues
```bash
# Clear cache and reconfigure
rm -rf CMakeCache.txt CMakeFiles/
cmake ..
```

### Runtime Issues

#### Segmentation Faults
1. **Use GDB** to get stack trace
2. **Check null pointers** - common in object interactions
3. **Verify object lifetimes** - especially with smart pointers
4. **Check array bounds** - use vector::at() instead of operator[]

#### Memory Leaks
1. **Use smart pointers** instead of raw pointers
2. **Break circular references** in bonus system
3. **Check for proper cleanup** in destructors

#### Performance Issues
1. **Profile first** - don't guess where bottlenecks are
2. **Check for N+1 queries** in database-like operations
3. **Optimize hot paths** identified by profiler
4. **Use const references** to avoid unnecessary copies

### Network Issues

#### Desynchronization
1. **Check RNG usage** - ensure deterministic behavior
2. **Verify packet ordering** - order matters for game state
3. **Test save/load** - state must be perfectly reproducible

#### Connection Problems
1. **Check firewall settings**
2. **Verify port availability**
3. **Test with localhost** first

## Continuous Integration

### GitHub Actions

VCMI uses GitHub Actions for CI. Check `.github/workflows/` for:
- Build verification on multiple platforms
- Unit test execution
- Code quality checks

### Local CI Simulation

```bash
# Run the same checks locally
cmake --build . --target test
ctest --output-on-failure

# Static analysis
clang-tidy src/*.cpp
cppcheck src/
```

## Best Practices

### Testing
1. **Write tests first** for new features (TDD)
2. **Test edge cases** and error conditions
3. **Keep tests independent** - no shared state
4. **Use descriptive test names** - explain what's being tested

### Debugging
1. **Reproduce issues reliably** before debugging
2. **Use version control** to identify when issues were introduced
3. **Document solutions** for future reference
4. **Share knowledge** with the team through comments and docs

### Performance
1. **Measure before optimizing** - profile first
2. **Optimize algorithms** before micro-optimizations
3. **Consider memory locality** for cache efficiency
4. **Test performance impact** of changes

Remember: Good testing and debugging practices save significant time in the long run and improve code quality for the entire project.