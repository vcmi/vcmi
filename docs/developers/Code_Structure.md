# Code Structure

The code of VCMI is divided into several main parts: client, server, lib and AIs, each one in a separate binary file.

## The big picture

VCMI contains three core projects: VCMI_lib (dll / so), VCMI_client (executable) and VCMI_server (executable). Server handles all game mechanics and events. Client presents game state and events to player and collects input from him.

During the game, we have one (and only one) server and one or more (one for each player computer) clients.

Important: State of the game and its mechanics are synchronized between clients and server. All changes to the game state or mechanics must be done by server which will send appropriate notices to clients.

### Game state

It's basically CGameState class object and everything that's accessible from it: map (with objects), player statuses, game options, etc.

### Bonus system

One of the more important pieces of VCMI is the [bonus system](Bonus_System.md). It's described in a separate article.

### Configuration

Most of VCMI configuration files uses Json format and located in "config" directory

#### Json parser and writer

## Client

### Main purposes of client

Client is responsible for:

- displaying state of game to human player
- capturing player's actions and sending requests to server
- displaying changes in state of game indicated by server

### Rendering of graphics

Rendering of graphics relies heavily on SDL. Currently we do not have any wrapper for SDL internal structures and most of rendering is about blitting surfaces using SDL_BlitSurface. We have a few function that make rendering easier or make specific parts of rendering (like printing text). They are places in client/SDL_Extensions and client/SDL_Framerate (the second one contains code responsible for keeping appropriate framerate, it should work more smart than just SDL_Delay(milliseconds)).
In rendering, Interface object system is quite helpful. Its base is CIntObject class that is basically a base class for our library of GUI components and other objects.

## Server

### Main purposes of server

Server is responsible for:

- maintaining state of the game
- handling requests from all clients participating in game
- informing all clients about changes in state of the game that are
    visible to them

## Lib

### Main purposes of lib

VCMI_Lib is a library that contains code common to server and client, so we avoid it's duplication. Important: the library code is common for client and server and used by them, but the library instance (in opposition to the library as file) is not shared by them! Both client and server create their own "copies" of lib with all its class instances.

iOS platform pioneered single process build, where server is a static library and not a dedicated executable. For that to work, the lib had to be wrapped into special namespace that is defined by client and server targets on iOS, so that single process is able to contain 2 versions of the library. To make it more convenient, a few macros were introduced that can be found in [Global.h](https://github.com/vcmi/vcmi/blob/develop/Global.h). The most important ones are `VCMI_LIB_NAMESPACE_BEGIN` and `VCMI_LIB_NAMESPACE_END` which must be used anywhere a symbol from the lib is needed, otherwise building iOS (or any other platform that would use single process approach) fails.

Lib contains code responsible for:

- handling most of Heroes III files (.lod, .txt setting files)
- storing information common to server and client like state of the game
- managing armies, buildings, artifacts, spells, bonuses and other game objects
- handling general game mechanics and related actions (only adventure map objects; it's an unwanted remnant of past development - all game mechanics should be handled by the server)
- networking and serialization

#### Serialization

The serialization framework can serialize basic types, several standard containers among smart pointers and custom objects. Its design is based on the [boost serialization libraries](http://www.boost.org/doc/libs/1_52_0/libs/serialization/doc/index.html).
In addition to the basic functionality it provides light-weight transfer of CGObjectInstance objects by sending only the index/id.

Serialization page for all the details.

#### Wrapped namespace examples

##### Inside the lib

Both header and implementation of a new class inside the lib should have the following structure:

`<includes>`  
`VCMI_LIB_NAMESPACE_BEGIN`  
`<code>`  
`VCMI_LIB_NAMESPACE_END`

##### Headers outside the lib

Forward declarations of the lib in headers of other parts of the project need to be wrapped in the macros:

`<includes>`  
`VCMI_LIB_NAMESPACE_BEGIN`  
`<lib forward declarations>`  
`VCMI_LIB_NAMESPACE_END`  
`<other forward declarations>`  
`<classes>`

##### New project part

If you're creating new project part, place `VCMI_LIB_USING_NAMESPACE` in its `StdInc.h` to be able to use lib classes without explicit namespace in implementation files. Example: <https://github.com/vcmi/vcmi/blob/develop/launcher/StdInc.h>

## Artificial Intelligence

### Combat AI

- Battle AI is recent and used combat AI.
- Stupid AI is old and deprecated version of combat AI

### Adventure AI

- NullkillerAI module is currently developed agent-based system driven by goals and heroes.
- VCAI is old and deprecated version of adventure map AI

## Threading Model

## Long-living threads

Here is list of threads including their name that can be seen in logging or in debugging:

- Main thread (`MainGUI`). This is main thread that is created on app start. This thread is responsible for input processing (including updating screen due to player actions) and for final rendering steps. Note that on some OS'es (like Linux) name of main thread is also name of the application. Because of that, thread name is only set for logging, while debugger will show this thread with default name.

- Network thread (`runNetwork`). Name is semi-historical, since in case of single-player game no longer uses networking, but intra-process communication. In either case, this thread permanently runs boost::asio io_service, and processes any callbacks received through it, whether it is incoming packets from network, or incoming data from another thread. Following actions are also done on this thread, due to being called as part of netpack processing:
  - combat AI actions
  - UI feedback on any incoming netpacks. Note that this also includes awaiting for any animation - whether in-combat or adventure map. When animations are playing, network thread will be waiting for animations to finish.
  - Initial reaction of adventure map AI on netpack. However, AI will usually dispatch this event to AI thread, and perform actual processing in AI thread.

- Server thread (`runServer`). This thread exists for as long as player is in singleplayer game or hosting a multiplayer game, whether on map selection, or already in loaded game. Just like networking thread, this thread also permanently runs own boost::asio io_service, and processes any incoming player (either human or AI) requests, whether through network or through intraprocess communication. When standalone vcmiserver is used, entire server will be represented by this thread.

- Console thread (`consoleHandler`). This thread usually does nothing, and only performs processing of incoming console commands on standard input, which is accessible by running vcmiclient directly.

### Intel TBB

- NullkillerAI parallelizes a lot of its tasks using TBB methods, mostly parallel_for
- Random map generator actively uses thread pool provided by TBB
- Client performs image upscaling in background thread to avoid visible freezes
- AI main task (`NKAI::makeTurn`). This TBB task is created whenever AI stars a new turn, and ends when AI ends its turn. Majority of AI event processing is done in this thread, however some actions are either offloaded entirely as tbb task, or parallelized using methods like parallel_for.
- AI helper tasks (`NKAI::<various>`). Adventure AI creates such tasks whenever it receives event that requires processing without locking network thread that initiated the call.

## Short-living threads

- Autocombat initiation thread (`autofightingAI`). Combat AI usually runs on network thread, as reaction on unit taking turn netpack event. However initial activation of AI when player presses hotkey or button is done in input processing (`MainGUI`) thread. To avoid freeze when AI selects its first action, this action is done on a temporary thread

- Initializition thread (`initialize`). On game start, to avoid delay in game loading, most of game library initialization is done in separate thread while main thread is playing intro movies.

- Console command processing (`processCommand`). Some console commands that can be entered in game chat either take a long time to process or expect to run without holding any mutexes (like interface mutex). To avoid such problems, all commands entered in game chat are run in separate thread.

### Fuzzy logic

VCMI includes [FuzzyLite](http://code.google.com/p/fuzzy-lite/) library to make use of fuzzy rule-based algorithms. They are useful to handle uncertainty and resemble human behaviour who takes decisions based on rough observations. FuzzyLite is linked as separate static library in AI/FuzzyLite.lib file.
