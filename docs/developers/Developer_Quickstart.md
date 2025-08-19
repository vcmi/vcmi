# Developer Quickstart Guide

This guide will help you get started with VCMI development as quickly as possible.

## Prerequisites

Before starting, make sure you have:
- Basic knowledge of C++ (C++17 or later)
- Git for version control
- CMake 3.21+ for building
- A code editor or IDE (Qt Creator recommended, see [Development with Qt Creator](Development_with_Qt_Creator.md))

## Quick Setup (5 minutes)

### 1. Clone the Repository

```bash
git clone --recurse-submodules https://github.com/vcmi/vcmi.git
cd vcmi
```

### 2. Install Dependencies

Choose your platform:

#### Windows
Follow the [Windows building guide](Building_Windows.md) or use [Conan](Conan.md):
```bash
conan install . -pr:b default -pr:h windows-msvc --build=missing
```

#### macOS
```bash
# Using Conan (recommended)
conan install . -pr:b default -pr:h macos-intel --build=missing  # Intel
conan install . -pr:b default -pr:h macos-arm --build=missing    # Apple Silicon

# Or using Homebrew
brew install cmake sdl2 sdl2_image sdl2_mixer sdl2_ttf boost qt5
```

#### Linux
```bash
# Ubuntu/Debian
sudo apt install cmake build-essential libsdl2-dev libsdl2-image-dev \
    libsdl2-mixer-dev libsdl2-ttf-dev libboost-all-dev qtbase5-dev

# Or use Conan
conan install . -pr:b default -pr:h linux-gcc --build=missing
```

### 3. Build the Project

```bash
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . --parallel
```

### 4. Test Your Build

```bash
# Run basic tests (if available)
ctest

# Run VCMI client (make sure you have Heroes 3 data files)
./client/vcmiclient
```

## Development Workflow

### Project Structure

```
vcmi/
├── client/          # Game client (UI, rendering, input)
├── server/          # Game server (logic, AI, validation)  
├── lib/             # Shared library (game mechanics, data)
├── AI/              # AI implementations
├── launcher/        # Game launcher utility
├── mapeditor/       # Map editor
├── test/            # Unit tests
├── config/          # Game configuration files
└── docs/            # Documentation
```

### Key Directories in Detail

- **`lib/`** - Core game mechanics, serialization, map objects
- **`client/windows/`** - Game UI windows and dialogs  
- **`client/battle/`** - Battle interface components
- **`server/`** - Game state management and AI
- **`scripting/`** - Lua scripting system

### Making Your First Change

Let's add a simple debug message when the game starts:

1. **Edit the client main file:**
```bash
# Open client/vcmiclient.cpp in your editor
```

2. **Add a debug message:**
```cpp
// Find the main() function and add near the beginning:
logGlobal->info("Hello from my VCMI build!");
```

3. **Rebuild and test:**
```bash
cmake --build . --parallel
./client/vcmiclient
```

You should see your message in the log output.

## Common Development Tasks

### Adding a New Map Object

1. **Create the object class** in `lib/mapObjects/`:
```cpp
// MyObject.h
class MyObject : public CGObjectInstance
{
public:
    void onHeroVisit(const CGHeroInstance * h) const override;
    
    template<typename Handler>
    void serialize(Handler &h, const int version)
    {
        h & static_cast<CGObjectInstance&>(*this);
    }
};
```

2. **Register the object** in `lib/serializer/RegisterTypes.h`:
```cpp
s.template registerType<MyObject>(NEXT_AVAILABLE_ID);
```

3. **Add object configuration** in `config/objects/`:
```json
{
    "myObject": {
        "handler": "myObject",
        "types": {
            "object": {
                "rmg": {
                    "value": 100,
                    "rarity": 50
                }
            }
        }
    }
}
```

### Adding a New Bonus Type

1. **Define the bonus** in `lib/Bonus.h`:
```cpp
enum BonusType
{
    // ... existing bonuses
    MY_NEW_BONUS,
    // ...
};
```

2. **Handle the bonus** in relevant calculation code
3. **Add configuration** in `config/bonusnames.json`

### Adding UI Elements

1. **Create the interface class** inheriting from `CIntObject`
2. **Override rendering and input methods**
3. **Add to parent window or interface**

## Debugging

### Enable Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

### Useful Debug Options
```bash
# Enable additional logging
cmake .. -DENABLE_DEBUG_CONSOLE=ON

# Enable debugging symbols
cmake .. -DCMAKE_CXX_FLAGS="-g3 -O0"
```

### Using GDB (Linux/macOS)
```bash
gdb ./client/vcmiclient
(gdb) run
(gdb) bt  # backtrace on crash
```

### Using Visual Studio Debugger (Windows)
1. Generate Visual Studio project: `cmake .. -G "Visual Studio 16 2019"`
2. Open `VCMI.sln` in Visual Studio
3. Set breakpoints and debug normally

## Testing

### Running Unit Tests
```bash
cd build
ctest --verbose
```

### Manual Testing
1. Copy Heroes 3 data files to appropriate directory
2. Run `vcmilauncher` to configure
3. Start a new game to test changes

### Creating Tests
Add test files in `test/` directory following existing patterns:
```cpp
#include <gtest/gtest.h>

TEST(MyTest, TestSomething)
{
    EXPECT_EQ(1 + 1, 2);
}
```

## Code Style

### Follow the Guidelines
- Read [Coding Guidelines](Coding_Guidelines.md) thoroughly
- Use tabs for indentation
- Follow naming conventions:
  - Classes: `CMyClass` or `MyClass`
  - Variables: `camelCase`
  - Constants: `ALL_CAPS`

### Useful Tools
```bash
# Format code with clang-format
clang-format -i file.cpp

# Check style
clang-tidy file.cpp
```

## Common Gotchas

### 1. Client vs Server Code
- **Client**: UI, graphics, input handling
- **Server**: Game logic, state changes, validation
- **Never** modify game state directly from client code

### 2. Serialization
- Always update `serialize()` methods when adding new members
- Bump serialization version for breaking changes
- Test save/load compatibility

### 3. Memory Management
- Use smart pointers (`std::shared_ptr`, `std::unique_ptr`)
- Be careful with circular references in bonus system
- Don't store raw pointers to game objects

### 4. Threading
- Game state is single-threaded
- UI updates must happen on main thread
- Use proper synchronization for background tasks

## Getting Help

### Documentation
- [API Reference](API_Reference.md) - Engine API documentation
- [Code Structure](Code_Structure.md) - Architecture overview
- [Bonus System](Bonus_System.md) - Bonus system details

### Community
- **Discord**: <https://discord.gg/chBT42V>
- **Forums**: <https://forum.vcmi.eu/>
- **GitHub Issues**: For bug reports and feature requests

### Code Navigation
- Use an IDE with good C++ support (Qt Creator, CLion, Visual Studio)
- Enable `CMAKE_EXPORT_COMPILE_COMMANDS` for language servers
- Use `git grep` to search codebase: `git grep "functionName"`

## Next Steps

Once you're comfortable with the basics:

1. **Pick an issue** from GitHub issues labeled "good first issue"
2. **Read architecture docs** to understand the bigger picture
3. **Study existing code** similar to what you want to implement
4. **Start small** - simple fixes and improvements first
5. **Ask for help** when you get stuck

### Recommended Reading Order
1. [Code Structure](Code_Structure.md) - Understand the architecture
2. [Bonus System](Bonus_System.md) - Core gameplay mechanic
3. [Serialization](Serialization.md) - Save/load and networking
4. [API Reference](API_Reference.md) - Detailed API documentation

Happy coding! 🎮