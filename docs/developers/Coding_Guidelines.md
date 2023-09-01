## C++ Standard

VCMI implementation bases on C++17 standard. Any feature is acceptable as long as it's will pass build on our CI, but there is list below on what is already being used.

Any compiler supporting C++17 should work, but this has not been thoroughly tested. You can find information about extensions and compiler support at [1](http://en.cppreference.com/w/cpp/compiler_support).

## Style Guidelines

In order to keep the code consistent, please use the following conventions. From here on 'good' and 'bad' are used to attribute things that would make the coding style match, or not match. It is not a judgment call on your coding abilities, but more of a style and look call. Please try to follow these guidelines to ensure prettiness.

### Indentation

Use tabs for indentation. If you are modifying someone else's code, try to keep the coding style similar.

### Where to put braces

Inside a code block put the opening brace on the next line after the current statement:

Good:

``` cpp
if(a) 
{
	code();
	code();
}
```

Bad:

``` cpp
if(a) {
	code();
	code();
}
```

Avoid using unnecessary open/close braces, vertical space is usually limited:

Good:

``` cpp
if(a)
	code();
```

Bad:

``` cpp
if(a) {
	code();
}
```

Unless there are either multiple hierarchical conditions being used or that the condition cannot fit into a single line.

Good:

``` cpp
if(a)
{
	if(b)
		code();
}
```

Bad:

``` cpp
if(a)
	if(b)
		code();
```

If there are brackets inside the body, outside brackets are required.

Good:

``` cpp
if(a)
{
	for(auto elem : list)
	{
		code();
	}
}
```

Bad:

``` cpp
if(a)
	for(auto elem : list)
	{
		code();
	}
```

If "else" branch has brackets then "if" should also have brackets even if it is one line.

Good:

``` cpp
if(a)
{
	code();
}
else
{
	for(auto elem : list)
	{
		code();
	}
}
```

Bad:

``` cpp
if(a)
	code();
else
{
	for(auto elem : list)
	{
		code();
	}
}
```

If you intentionally want to avoid usage of "else if" and keep if body indent make sure to use braces.

Good:

``` cpp
if(a)
{
	code();
}
else
{
	if(b)
		code();
}
```

Bad:

``` cpp
if(a)
	code();
else
	if(b)
		code();
```

When defining a method, use a new line for the brace, like this:

Good:

``` cpp
void method()
{
}
```

Bad:

``` cpp
void Method() {
}
```

### Use whitespace for clarity

Use white space in expressions liberally, except in the presence of parenthesis.

**Good:**

``` cpp
if(a + 5 > method(blah('a') + 4))
	foo += 24;
```

**Bad:**

``` cpp
if(a+5>method(blah('a')+4))
foo+=24;
```

Between if, for, while,.. and the opening brace there shouldn't be a whitespace. The keywords are highlighted, so they don't need further separation.

### Where to put spaces

Use a space before and after the address or pointer character in a pointer declaration.

Good:

``` cpp
CIntObject * images[100];
```

Bad:

``` cpp
CIntObject* images[100]; or
CIntObject *images[100];
```

Do not use spaces before parentheses.

Good:

``` cpp
if(a)
	code();
```

Bad:

``` cpp
if (a)
	code();
```

Do not use extra spaces around conditions inside parentheses.

Good:

``` cpp
if(a && b)
	code();

if(a && (b || c))
	code();
```

Bad:

``` cpp
if( a && b )
	code();

if(a && ( b || c ))
	code();
```

Do not use more than one space between operators.

Good:

``` cpp
if((a && b) || (c + 1 == d))
	code();
```

Bad:

``` cpp
if((a && b)  ||  (c + 1 == d))
	code();

if((a && b) || (c + 1  ==  d))
	code();
```

### Where to use parentheses

When allocating objects, don't use parentheses for creating stack-based objects by zero param c-tors to avoid c++ most vexing parse and use parentheses for creating heap-based objects.

Good:

``` cpp
std::vector<int> v; 
CGBoat btn = new CGBoat();
```

Bad:

``` cpp
std::vector<int> v(); // shouldn't compile anyway 
CGBoat btn = new CGBoat;
```

Avoid overuse of parentheses:

Good:

``` cpp
if(a && (b + 1))
	return c == d;
```

Bad:

``` cpp
if((a && (b + 1)))
	return (c == d);
```

### Class declaration

Base class list must be on same line with class name.

``` cpp
class CClass : public CClassBaseOne, public CClassBaseOne
{
	int id;
	bool parameter;

public:
	CClass();
	~CClass();
};
```

When 'private:', 'public:' and other labels are not on the line after opening brackets there must be a new line before them.

Good:

``` cpp
class CClass
{
	int id;

public:
	CClass();
};
```

Bad:

``` cpp
class CClass
{
	int id;
public:
	CClass();
};
```

Good:

``` cpp
class CClass
{
protected:
	int id;

public:
	CClass();
};
```

Bad:

``` cpp
class CClass
{

protected:
	int id;

public:
	CClass();
};
```

### Constructor base class and member initialization

Constructor member and base class initialization must be on new line, indented with tab with leading colon.

``` cpp
CClass::CClass()
	: CClassBaseOne(true, nullptr), id(0), bool parameters(false)
{
	code();
}
```

### Switch statement

Switch statements have the case at the same indentation as the switch.

Good:

``` cpp
switch(alignment)
{
case EAlignment::EVIL:
	do_that();
	break;
case EAlignment::GOOD:
	do_that();
	break;
case EAlignment::NEUTRAL:
	do_that();
	break;
default:
	do_that();
	break;
}
```

Bad:

``` cpp
switch(alignment)
{
	case EAlignment::EVIL:
		do_that();
	break;
	default:
		do_that();
	break;
}

switch(alignment)
{
	case EAlignment::EVIL:
		do_that();
		break;
	default:
		do_that();
		break;
}

switch(alignment)
{
case EAlignment::EVIL:
{
	do_that();
}
break;
default:
{	
	do_that();
}
break;
}
```

### Lambda expressions

Good:

``` cpp
auto lambda = [this, a, &b](int3 & tile, int index) -> bool
{
	do_that();
};
```

Bad:

``` cpp
auto lambda = [this,a,&b](int3 & tile, int index)->bool{do_that();};
```

Empty parameter list is required even if function takes no arguments.

Good:

``` cpp
auto lambda = []()
{
	do_that();
};
```

Bad:

``` cpp
auto lambda = []
{
	do_that();
};
```

Do not use inline lambda expressions inside if-else, for and other conditions.

Good:

``` cpp
auto lambda = []()
{
	do_that();
};
if(lambda)
{
	code();
}
```

Bad:

``` cpp
if([]()
{
	do_that();
})
{
	code();
}
```

Do not pass inline lambda expressions as parameter unless it's the last parameter.

Good:

``` cpp
auto lambda = []()
{
	do_that();
};
obj->someMethod(lambda, true);
```

Bad:

``` cpp
obj->someMethod([]()
{
	do_that();
}, true);
```

Good:

``` cpp
obj->someMethod(true, []()
{
	do_that();
});
```

### Serialization

Serialization of each element must be on it's own line since this make debugging easier.

Good:

``` cpp
template <typename Handler> void serialize(Handler & h, const int version)
{
	h & identifier;
	h & description;
	h & name;
	h & dependencies;
}
```

Bad:

``` cpp
template <typename Handler> void serialize(Handler & h, const int version)
{
	h & identifier & description & name & dependencies;
}
```

Save backward compatibility code is exception when extra brackets are always useful.

Good:

``` cpp
template <typename Handler> void serialize(Handler & h, const int version)
{
	h & identifier;
	h & description;
	if(version >= 123)
	{
		h & totalValue;
	}
	else if(!h.saving)
	{
		totalValue = 200;
	}
	h & name;
	h & dependencies;
}
```

Bad:

``` cpp
template <typename Handler> void serialize(Handler & h, const int version)
{
	h & identifier;
	h & description;
	if(version >= 123)
		h & totalValue;
	else if(!h.saving)
		totalValue = 200;
	h & name;
	h & dependencies;
}
```

### File headers

For any new files, please paste the following info block at the very top of the source file:

``` cpp
/*
 * Name_of_File.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
```

The above notice have to be included both in header and source files (.h/.cpp).

### Code order in files

For any header or source file code must be in following order:

1.  Licensing information
2.  pragma once preprocessor directive
3.  include directives
4.  Forward declarations
5.  All other code

``` cpp
/*
 * Name_of_File.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include "Header.h"

class CGObjectInstance;
struct CPackForClient;
```

### Where and how to comment

If you comment on the same line with code there must be one single space between code and slashes.

Good:

``` cpp
if(a)
{
	code(); //Do something
}
else // Do something.
{
	code(); // Do something.
}
```

Bad:

``` cpp
if(a)
{
	code();//Do something
}
else           // Do something.
{
	code();   // TODO:
}
```

If you add single-line comment on own line slashes must have same indent as code around:

Good:

``` cpp
// Do something
if(a)
{
	//Do something
	for(auto item : list)
		code();
}
```

Bad:

``` cpp
			// Do something
if(a)
{
//Do something
	for(auto item : list)
		code();
}
```

Avoid comments inside multi-line if-else conditions. If your conditions are too hard to understand without additional comments this usually means that code need refactoring. Example given below is need improvement though. **FIXME**

Good:

``` cpp
bool isMyHeroAlive = a && b || (c + 1 > 15);
bool canMyHeroMove = myTurn && hero.movePoints > 0;
if(isMyHeroAlive && canMyHeroMove)
{
	code();
}
```

Bad:

``` cpp
if((a && b || (c + 1 > 15)) //Check if hero still alive
	&& myTurn && hero.movePoints > 0) //Check if hero can move
{
	code();
}
```

You should write a comment before the class definition which describes shortly the class. 1-2 sentences are enough. Methods and class data members should be commented if they aren't self-describing only. Getters/Setters, simple methods where the purpose is clear or similar methods shouldn't be commented, because vertical space is usually limited. The style of documentation comments should be the three slashes-style: ///.

``` cpp
/// Returns true if a debug/trace log message will be logged, false if not.
/// Useful if performance is important and concatenating the log message is a expensive task.
bool isDebugEnabled() const;
bool isTraceEnabled() const;
```

The above example doesn't follow a strict scheme on how to comment a method. It describes two methods in one go. Comments should be kept short.

If you need a more detailed description for a method you can use such style:

``` cpp
/// <A short one line description>
///
/// <Longer description>
/// <May span multiple lines or paragraphs as needed>
///
/// @param  Description of method's or function's input parameter
/// @param  ...
/// @return Description of the return value
```

A good essay about writing comments:
[2](http://ardalis.com/when-to-comment-your-code)

### Casing

Local variables and methods start with a lowercase letter and use the camel casing. Classes/Structs start with an uppercase letter and use the camel casing as well. Macros and constants are written uppercase.

### Line length

The line length for c++ source code is 120 columns. If your function declaration arguments go beyond this point, please align your arguments to match the opening brace. For best results use the same number of tabs used on the first line followed by enough spaces to align the arguments.

### Warnings

Avoid use of #pragma to disable warnings. Compile at warning level 3. Avoid commiting code with new warnings.

### File/directory naming

Compilation units(.cpp,.h files) start with a uppercase letter and are named like the name of a class which resides in that file if possible. Header only files start with a uppercase letter. JSON files start with a lowercase letter and use the camel casing.

Directories start with a lowercase letter and use the camel casing where necessary.

### Logging

Outdated. There is separate entry for [Logging API](Logging_API "wikilink")

If you want to trace the control flow of VCMI, then you should use the macro LOG_TRACE or LOG_TRACE_PARAMS. The first one prints a message when the function is entered or leaved. The name of the function will also be logged. In addition to this the second macro, let's you specify parameters which you want to print. You should print traces with parameters like this:

``` cpp
LOG_TRACE_PARAMS(logGlobal, "hero '%s', spellId '%d', pos '%s'.", hero, spellId, pos);
```

When using the macro every "simple" parameter should be logged. The parameter can be a number, a string or a type with a ostream operator\<\<. You should not log contents of a whole text file, a byte array or sth. like this. If there is a simple type with a few members you want to log, you should write an ostream operator\<\<. The produced message can look like this:

`{BattleAction: side '0', stackNumber '1', actionType 'Walk and attack', destinationTile '{BattleHex: x '7', y '1', hex '24'}', additionalInfo '7', selectedStack '-1'}`

The name of the type should be logged first, e.g. {TYPE_NAME: members...}. The members of the object will be logged like logging trace parameters. Collection types (vector, list, ...) should be logged this way: \[{BattleHex: ...}, {...}\] There is no format which has to be followed strictly, so if there is a reason to format members/objects in a different way, then this is ok.

## Best practices

### Avoid code duplication

Avoid code duplication or don't repeat yourself(DRY) is the most important aspect in programming. Code duplication of any kind can lead to inconsistency and is much harder to maintain. If one part of the system gets changed you have to change the code in several places. This process is error-prone and leads often to problems. Here you can read more about the DRY principle: [<http://en.wikipedia.org/wiki/Don%27t_repeat_yourself>](http://en.wikipedia.org/wiki/Don%27t_repeat_yourself)

### Do not use uncommon abbrevations

Do not use uncommon abbrevations for class, method, parameter and global object names.

Bad:

``` cpp
CArt * getRandomArt(...)
class CIntObject
```

Good:

``` cpp
CArtifact * getRandomArtifact(...)
class CInterfaceObject
```

### Loop handling

Use range-based for loops. It should be used in any case except you absolutely need iterator, then you may use a simple for loop.

The loop counter should be of type int, unless you are sure you won't need negative indices -- then use size_t.

### Include guards

Use #pragma once instead of the traditional #ifndef/#define/#endif include guards.

### Pre compiled header file

The header StdInc.h should be included in every compilation unit. It has to be included before any C macro and before any c++ statements. Pre compiled header should not be changed, except any important thing is missing. The StdInc includes most Boost libraries and nearly all standard STL and C libraries, so you don’t have to include them by yourself.

### Enumeration handling

Do not declare enumerations in global namespace. It is better to use strongly typed enum or to wrap them in class or namespace to avoid polluting global namespace:

``` cpp
enum class EAlignment
{
	GOOD,
	EVIL,
	NEUTRAL
};

namespace EAlignment
{
	enum EAlignment
	{
		GOOD, EVIL, NEUTRAL
	};
}
```

### Avoid senseless comments

If the comment duplicates the name of commented member, it's better if it wouldn't exist at all. It just increases maintenance cost. Bad:

``` cpp
size_t getHeroesCount(); //gets count of heroes (surprise?)
```

### Class handling

There is no definitive rule which has to be followed strictly. You can freely decide if you want to pack your own classes, where you are programming on, all in one file or each in one file. It's more important that you feel comfortable with the code, than consistency overall the project. VCMI has several container class files, so if you got one additional class to them than just add it to them instead of adding new files.

### Functions and interfaces

Don't return const objects or primitive types from functions -- it's pointless. Also, don't return pointers to non-const game data objects from callbacks to player interfaces.

Bad:

``` cpp
const std::vector<CGObjectInstance *> guardingCreatures(int3 pos) const;
```

Good:

``` cpp
std::vector<const CGObjectInstance *> guardingCreatures(int3 pos) const;
```

## Sources

[Mono project coding guidelines](http://www.mono-project.com/Coding_Guidelines)