# Introduction

The serializer translates between objects living in our code (like int or CGameState\*) and stream of bytes. Having objects represented as a stream of bytes is useful. Such bytes can send through the network connection (so client and server can communicate) or written to the disk (savegames).

VCMI uses binary format. The primitive types are simply copied from memory, more complex structures are represented as a sequence of primitives.

## Typical tasks

### Bumping a version number

Different major version of VCMI likely change the format of the save game. Every save game needs a version identifier, that loading can work properly. Backward compatibility isn't supported for now. The version identifier is a constant named version in Connection.h and should be updated every major VCMI version or development version if the format has been changed. Do not change this constant if it's not required as it leads to full rebuilds. Why should the version be updated? If VCMI cannot detect "invalid" save games the program behaviour is random and undefined. It mostly results in a crash. The reason can be anything from null pointer exceptions, index out of bounds exceptions(ok, they aren't available in c++, but you know what I mean:) or invalid objects loading(too much elements in a vector, etc...) This should be avoided at least for public VCMI releases.

### Adding a new class

If you want your class to be serializable (eg. being storable in a savegame) you need to define a serialize method template, as described in [#User types](#User_types "wikilink")

Additionally, if your class is part of one of registered object hierarchies (basically: if it derives from CGObjectInstance, IPropagator, ILimiter, CBonusSystemNode, CPack) it needs to be registered. Just add an appropriate entry in the `RegisterTypes.h` file. See [#Polymorphic serialization](#Polymorphic_serialization "wikilink") for more information.

# How does it work

## Primitive types

They are simply stored in a binary form, as in memory. Compatibility is ensued through the following means:

- VCMI uses internally types that have constant, defined size (like int32_t - has 32 bits on all platforms)
- serializer stores information about its endianess

It's not "really" portable, yet it works properly across all platforms we currently support.

## Dependant types

### Pointers

Storing pointers mechanics can be and almost always is customized. See [#Additional features](#Additional_features "wikilink").

In the most basic form storing pointer simply sends the object state and loading pointer allocates an object (using "new" operator) and fills its state with the stored data.

### Arrays

Serializing array is simply serializing all its elements.

## Standard library types

### STL Containers

First the container size is stored, then every single contained element.

Supported STL types include:

`vector`  
`array`  
`set`  
`unordered_set`  
`list`  
`string`  
`pair`  
`map`

### Smart pointers

Smart pointers at the moment are treated as the raw C-style pointers. This is very bad and dangerous for shared_ptr and is expected to be fixed somewhen in the future.

The list of supported data types from standard library:

`shared_ptr (partial!!!)`  
`unique_ptr`

### Boost

Additionally, a few types for Boost are supported as well:

`variant`  
`optional`

## User types

To make the user-defined type serializable, it has to provide a template method serialize. The first argument (typed as template parameter) is a reference to serializer. The second one is version number.

Serializer provides an operator& that is internally expanded to `<<` when serialziing or `>>` when deserializing.

Serializer provides a public bool field `saving`that set to true during serialziation and to false for deserialziation.

Typically, serializing class involves serializing all its members (given that they are serializable). Sample:

``` cpp
/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name;
	std::string text;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		h & name;
		h & text;
	}
};
```

## Backwards compatibility

Serializer, before sending any data, stores its version number. It is passed as the parameter to the serialize method, so conditional code ensuring backwards compatibility can be added.

Yet, because of numerous changes to our game data structure, providing means of backwards compatibility is not feasible. The versioning feature is rarely used.

Sample:

``` cpp
/// The rumor struct consists of a rumor name and text.
struct DLL_LINKAGE Rumor
{
	std::string name; //introduced in version 1065
	std::string text;

	template <typename Handler>
	void serialize(Handler & h, const int version)
	{
		if(version >= 1065)
			h & name;
		else if(!h.saving)    //when loading old savegame
			name = "no name"; //set name to some default value  [could be done in class constructor as well]
	
		h & text;
	}
};
```

## Serializer classes

### Common information

Serializer classes provide iostream-like interface with operator `<<` for serialization and operator `>>` for deserialization. Serializer upon creation will retrieve/store some metadata (version number, endianess), so even if no object is actually serialized, some data will be passed.

### Serialization to file

CLoadFile/CSaveFile classes allow to read data to file and store data to file. They take filename as the first parameter in constructor and, optionally, the minimum supported version number (default to the current version). If the construction fails (no file or wrong file) the exception is thrown.

### Networking

CConnection class provides support for sending data structures over TCP/IP protocol. It provides 3 constructors: 
1. connect to given hostname at given port (host must be accepting connection) 
2. accept connection (takes boost.asio acceptor and io_service) 
3. adapt boost.asio socket with already established connection

All three constructors take as the last parameter the name that will be used to identify the peer.

## Additional features

Here is the list of additional custom features serialzier provides. Most of them can be turned on and off.

- [#Polymorphic serialization](#Polymorphic_serialization "wikilink") — no flag to control it, turned on by calls to registerType.
- [#Vectorized list member serialization](#Vectorized_list_member_serialization "wikilink") — enabled by smartVectorMembersSerialization flag.
- [#Stack instance serialization](#Stack_instance_serialization "wikilink") — enabled by sendStackInstanceByIds flag.
- [#Smart pointer serialization](#Smart_pointer_serialization "wikilink") — enabled by smartPointerSerialization flag.

### Polymorphic serialization

Serializer is to recognize the true type of object under the pointer if classes of that hierarchy were previously registered.

This means that following will work

``` cpp
Derived *d = new Derived();
Base *basePtr = d;
CSaveFile output("test.dat");
output << b;
//
Base *basePtr = nullptr;
CLoadFile input("test.dat");
input >> basePtr; //a new Derived object will be put under the pointer
```

Class hierarchies that are now registered to benefit from this feature are mostly adventure map object (CGObjectInstance) and network packs (CPack). See the RegisterTypes.h file for the full list.

It is crucial that classes are registered in the same order in the both serializers (storing and loading).

### Vectorized list member serialization

Both client and server store their own copies of game state and VLC (handlers with data from config). Many game logic objects are stored in the vectors and possess a unique id number that represent also their position in such vector.

The vectorised game objects are:

`CGObjectInstance`  
`CGHeroInstance`  
`CCreature`  
`CArtifact`  
`CArtifactInstance`  
`CQuest`

For this to work, serializer needs an access to gamestate library classes. This is done by calling a method `CSerializer::addStdVecItems(CGameState *gs, LibClasses *lib)`.

When the game ends (or gamestate pointer is invaldiated for another reason) this feature needs to be turned off by toggling its flag.

When vectorized member serialization is turned on, serializing pointer to such object denotes not sending an object itself but rather its identity. For example:

``` cpp
//Server code
CCreature *someCreature = ...;
connection << someCreature;
```

the last line is equivalent to

``` cpp
connection << someCreature->idNumber;
```

//Client code

``` cpp
CCreature *someCreature = nullptr;
connection >> someCreature;
```

the last line is equivalent to

``` cpp
CreatureID id;
connection >> id;
someCreature = VLC->creh->creatures[id.getNum()];
```

Important: this means that the object state is not serialized.

This feature makes sense only for server-client network communication.

### Stack instance serialization

This feature works very much like the vectorised object serialization. It is like its special case for stack instances that are not vectorised (each hero owns its map). When this option is turned on, sending CStackInstance\* will actually send an owning object (town, hero, garrison, etc) id and the stack slot position.

For this to work, obviously, both sides of the connection need to have exactly the same copies of an armed object and its stacks.

This feature depends on vectorised member serialization being turned on. (Sending owning object by id.)

### Smart pointer serialization

Note: name is unfortunate, this feature is not about smart pointers (like shared-ptr and unique_ptr). It is for raw C-style pointers, that happen to point to the same object.

This feature makes it that multiple pointers pointing to the same object are not stored twice.

Each time a pointer is stored, a unique id is given to it. If the same pointer is stored a second time, its contents is not serialized — serializer just stores a reference to the id.

For example:

``` cpp
Foo * a = new Foo();
Foo * b = b;

{
	CSaveFile test("test.txt");
	test << a << b;
}

Foo *loadedA, *loadedB;
{
	CLoadFile test("test.txt");
	test >> loadedA >> loadedB;
	//now both pointers point to the same object
	assert(loadedA == loadedB);
}
```

The feature recognizes pointers by addresses. Therefore it allows mixing pointers to base and derived classes. However, it does not allow serializing classes with multiple inheritance using a "non-first" base (other bases have a certain address offset from the actual object).

Pointer cycles are properly handled. This feature makes sense for savegames and is turned on for them.