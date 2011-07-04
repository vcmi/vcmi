#pragma once
#ifndef __GLOBAL_H__
#define __GLOBAL_H__
#include <iostream>
#include <sstream>
#include <algorithm> //std::find
#include <string> //std::find
#include <boost/logic/tribool.hpp>
#include <boost/unordered_set.hpp>
using boost::logic::tribool;
#include <boost/cstdint.hpp>
#include <assert.h>
#ifdef ANDROID
#include <android/log.h>
#include <sstream>
#endif
//filesystem version 3 causes problems (and it's default as of boost 1.46)
#define BOOST_FILESYSTEM_VERSION 2
typedef boost::uint64_t ui64; //unsigned int 64 bits (8 bytes)
typedef boost::uint32_t ui32;  //unsigned int 32 bits (4 bytes)
typedef boost::uint16_t ui16; //unsigned int 16 bits (2 bytes)
typedef boost::uint8_t ui8; //unsigned int 8 bits (1 byte)
typedef boost::int64_t si64; //signed int 64 bits (8 bytes)
typedef boost::int32_t si32; //signed int 32 bits (4 bytes)
typedef boost::int16_t si16; //signed int 16 bits (2 bytes)
typedef boost::int8_t si8; //signed int 8 bits (1 byte)
typedef si64 expType;
typedef ui32 TSpell;
typedef std::pair<ui32, ui32> TDmgRange;
typedef ui8 TBonusType;
typedef si32 TBonusSubtype;
#include "int3.h"
#include <map>
#include <vector>
#define CHECKTIME 1
#if CHECKTIME
#include "timeHandler.h"
#define THC
#endif

#define NAME_VER ("VCMI 0.85b")
extern std::string NAME; //full name
extern std::string NAME_AFFIX; //client / server
#define CONSOLE_LOGGING_LEVEL 5
#define FILE_LOGGING_LEVEL 6

/* 
 * DATA_DIR contains the game data (Data/, MP3/, ...).
 * USER_DIR is where to save games (Games/) and the config.
 * BIN_DIR is where the vcmiclient/vcmiserver binaries reside
 * LIB_DIR is where the AI libraries reside (linux only)
 */
#ifdef _WIN32
	#define DATA_DIR "."
	#define USER_DIR  "."
	#define BIN_DIR  "."
	#define LIB_DIR "."
	#define SERVER_NAME "VCMI_server.exe"
	#define LIB_EXT "dll"
#else
	#ifndef DATA_DIR
	#error DATA_DIR undefined.
	#endif
	#ifndef BIN_DIR
	#error BIN_DIR undefined.
	#endif
	#ifndef LIB_DIR
	#error LIB_DIR undefined.
	#endif
	#define SERVER_NAME "vcmiserver"
	#define LIB_EXT "so"
#endif

#ifdef _WIN32
#define PATH_SEPARATOR "\\"
#else
#define PATH_SEPARATOR "/"
#endif

/*
 * global.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

enum Ecolor {RED, BLUE, TAN, GREEN, ORANGE, PURPLE, TEAL, PINK}; //player's colors
enum EvictoryConditions {artifact, gatherTroop, gatherResource, buildCity, buildGrail, beatHero,
	captureCity, beatMonster, takeDwellings, takeMines, transportItem, winStandard=255};
enum ElossCon {lossCastle, lossHero, timeExpires, lossStandard=255};
enum ECombatInfo{ALIVE = 180, SUMMONED, CLONED, HAD_MORALE, WAITING, MOVED, DEFENDING};

class CGameInfo;
extern const CGameInfo* CGI; //game info for general use
class CClientState;
extern CClientState * CCS;

//a few typedefs for CCreatureSet
typedef si32 TSlot;
typedef si32 TQuantity; 
typedef ui32 TCreature; //creature id
const int ARMY_SIZE = 7;

const int HEROI_TYPE = 34, 
	TOWNI_TYPE = 98,
	CREI_TYPE = 54,
	EVENTI_TYPE = 26;

const int CREATURES_COUNT = 197;
const int CRE_LEVELS = 10;
const int F_NUMBER = 9; //factions (town types) quantity
const int PLAYER_LIMIT = 8; //player limit per map
const int ALL_PLAYERS = 255; //bitfield
const int HEROES_PER_TYPE=8; //amount of heroes of each type
const int SKILL_QUANTITY=28;
const int SKILL_PER_HERO=8;
const int ARTIFACTS_QUANTITY=171;
const int HEROES_QUANTITY=156;
const int SPELLS_QUANTITY=70;
const int RESOURCE_QUANTITY=8;
const int TERRAIN_TYPES=10;
const int PRIMARY_SKILLS=4;
const int NEUTRAL_PLAYER=255;
const int NAMES_PER_TOWN=16;
const int CREATURES_PER_TOWN = 7; //without upgrades
const int MAX_BUILDING_PER_TURN = 1;
const int SPELL_LEVELS = 5;
const int CREEP_SIZE = 4000; // neutral stacks won't grow beyond this number
//const int CREEP_SIZE = 2000000000;
const int WEEKLY_GROWTH = 10; //percent
const int AVAILABLE_HEROES_PER_PLAYER = 2;
const bool DWELLINGS_ACCUMULATE_CREATURES = false;
const bool STACK_EXP = true;
const bool STACK_ARTIFACT = true;

const int BFIELD_WIDTH = 17;
const int BFIELD_HEIGHT = 11;
const int BFIELD_SIZE = BFIELD_WIDTH * BFIELD_HEIGHT;

const int SPELLBOOK_GOLD_COST = 500;

//for battle stacks' positions
struct THex
{
	static const si16 INVALID = -1;
	enum EDir{RIGHT, BOTTOM_RIGHT, BOTTOM_LEFT, LEFT, TOP_LEFT, TOP_RIGHT};

	si16 hex;

	THex() : hex(INVALID) {}
	THex(si16 _hex) : hex(_hex)
	{
		//assert(isValid());
	}
	operator si16() const
	{
		return hex;
	}

	bool isValid() const
	{
		return hex >= 0 && hex < BFIELD_SIZE;
	}

	template<typename inttype>
	THex(inttype x, inttype y)
	{
		setXY(x, y);
	}

	template<typename inttype>
	THex(std::pair<inttype, inttype> xy)
	{
		setXY(xy);
	}

	template<typename inttype>
	void setX(inttype x)
	{
		setXY(x, getY());
	}

	template<typename inttype>
	void setY(inttype y)
	{
		setXY(getX(), y);
	}

	void setXY(si16 x, si16 y)
	{
		assert(x >= 0 && x < BFIELD_WIDTH && y >= 0  && y < BFIELD_HEIGHT);
		hex = x + y * BFIELD_WIDTH;
	}

	template<typename inttype>
	void setXY(std::pair<inttype, inttype> xy)
	{
		setXY(xy.first, xy.second);
	}

	si16 getY() const
	{
		return hex/BFIELD_WIDTH;
	}

	si16 getX() const
	{
		int pos = hex - getY() * BFIELD_WIDTH;
		return pos;
	}

	std::pair<si16, si16> getXY() const
	{
		return std::make_pair(getX(), getY());
	}

	//moving to direction
	void operator+=(EDir dir)
	{
		si16 x = getX(),
			y = getY();

		switch(dir)
		{
		case TOP_LEFT:
			setXY(y%2 ? x-1 : x, y-1);
			break;
		case TOP_RIGHT:
			setXY(y%2 ? x : x+1, y-1);
			break;
		case RIGHT:
			setXY(x+1, y);
			break;
		case BOTTOM_RIGHT:
			setXY(y%2 ? x : x+1, y+1);
			break;
		case BOTTOM_LEFT:
			setXY(y%2 ? x-1 : x, y+1);
			break;
		case LEFT:
			setXY(x-1, y);
			break;
		default:
			throw std::string("Disaster: wrong direction in THex::operator+=!\n");
			break;
		}
	}

	//generates new THex moved by given dir
	THex operator+(EDir dir) const
	{
		THex ret(*this);
		ret += dir;
		return ret;
	}

	std::vector<THex> neighbouringTiles() const
	{
		std::vector<THex> ret;
		const int WN = BFIELD_WIDTH;
		checkAndPush(hex - ( (hex/WN)%2 ? WN+1 : WN ), ret);
		checkAndPush(hex - ( (hex/WN)%2 ? WN : WN-1 ), ret);
		checkAndPush(hex - 1, ret);
		checkAndPush(hex + 1, ret);
		checkAndPush(hex + ( (hex/WN)%2 ? WN-1 : WN ), ret);
		checkAndPush(hex + ( (hex/WN)%2 ? WN : WN+1 ), ret);

		return ret;
	}

	//returns info about mutual position of given hexes (-1 - they're distant, 0 - left top, 1 - right top, 2 - right, 3 - right bottom, 4 - left bottom, 5 - left)
	static signed char mutualPosition(THex hex1, THex hex2)
	{
		if(hex2 == hex1 - ( (hex1/17)%2 ? 18 : 17 )) //top left
			return 0;
		if(hex2 == hex1 - ( (hex1/17)%2 ? 17 : 16 )) //top right
			return 1;
		if(hex2 == hex1 - 1 && hex1%17 != 0) //left
			return 5;
		if(hex2 == hex1 + 1 && hex1%17 != 16) //right
			return 2;
		if(hex2 == hex1 + ( (hex1/17)%2 ? 16 : 17 )) //bottom left
			return 4;
		if(hex2 == hex1 + ( (hex1/17)%2 ? 17 : 18 )) //bottom right
			return 3;
		return -1;
	}
	//returns distance between given hexes
	static si8 getDistance(THex hex1, THex hex2)
	{
		int xDst = std::abs(hex1 % BFIELD_WIDTH - hex2 % BFIELD_WIDTH),
			yDst = std::abs(hex1 / BFIELD_WIDTH - hex2 / BFIELD_WIDTH);
		return std::max(xDst, yDst) + std::min(xDst, yDst) - (yDst + 1)/2;
	}

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hex;
	}
	static void checkAndPush(int tile, std::vector<THex> & ret)
	{
		if( tile>=0 && tile<BFIELD_SIZE && (tile%BFIELD_WIDTH != (BFIELD_WIDTH - 1)) && (tile%BFIELD_WIDTH != 0) )
			ret.push_back(THex(tile));
	}

};

enum EMarketMode
{
	RESOURCE_RESOURCE, RESOURCE_PLAYER, CREATURE_RESOURCE, RESOURCE_ARTIFACT, 
	ARTIFACT_RESOURCE, ARTIFACT_EXP, CREATURE_EXP, CREATURE_UNDEAD, RESOURCE_SKILL,
	MARTKET_AFTER_LAST_PLACEHOLDER
};

namespace SpellCasting
{
	enum ESpellCastProblem
	{
		OK, NO_HERO_TO_CAST_SPELL, ALREADY_CASTED_THIS_TURN, NO_SPELLBOOK, ANOTHER_ELEMENTAL_SUMMONED,
		HERO_DOESNT_KNOW_SPELL, NOT_ENOUGH_MANA, ADVMAP_SPELL_INSTEAD_OF_BATTLE_SPELL,
		SECOND_HEROS_SPELL_IMMUNITY, SPELL_LEVEL_LIMIT_EXCEEDED, NO_SPELLS_TO_DISPEL,
		NO_APPROPRIATE_TARGET, STACK_IMMUNE_TO_SPELL, WRONG_SPELL_TARGET
	};

	enum ECastingMode {HERO_CASTING, AFTER_ATTACK_CASTING};
}

namespace Buildings
{
	enum EBuildStructure
	{
		HAVE_CAPITAL, NO_WATER, FORBIDDEN, ADD_MAGES_GUILD, ALREADY_PRESENT, CANT_BUILD_TODAY,
		NO_RESOURCES, ALLOWED, PREREQUIRES, ERROR
	};

	//Quite useful as long as most of building mechanics hardcoded
	enum EBuilding
	{
		MAGES_GUILD_1,  MAGES_GUILD_2, MAGES_GUILD_3,     MAGES_GUILD_4,   MAGES_GUILD_5, 
		TAVERN,         SHIPYARD,      FORT,              CITADEL,         CASTLE,
		VILLAGE_HALL,   TOWN_HALL,     CITY_HALL,         CAPITOL,         MARKETPLACE,
		RESOURCE_SILO,  BLACKSMITH,    SPECIAL_1,         HORDE_1,         HORDE_1_UPGR,
		SHIP,           SPECIAL_2,     SPECIAL_3,         SPECIAL_4,       HORDE_2,
		HORDE_2_UPGR,   GRAIL,         EXTRA_CITY_HALL,   EXTRA_TOWN_HALL, EXTRA_CAPITOL,
		DWELL_FIRST=30, DWELL_LAST=36, DWELL_UP_FIRST=37, DWELL_UP_LAST=43
	};
}

namespace Res
{
	enum ERes 
	{
		WOOD = 0, MERCURY, ORE, SULFUR, CRYSTAL, GEMS, GOLD, MITHRIL
	};
}

namespace Arts
{
	enum EPos
	{
		PRE_FIRST = -1, 
		HEAD, SHOULDERS, NECK, RIGHT_HAND, LEFT_HAND, TORSO, RIGHT_RING, LEFT_RING, FEET, MISC1, MISC2, MISC3, MISC4,
		MACH1, MACH2, MACH3, MACH4, SPELLBOOK, MISC5, 
		AFTER_LAST
	};
	const ui16 BACKPACK_START = 19;
	const int ID_CATAPULT = 3, ID_LOCK = 145;
}

enum EAlignment
{
	GOOD, EVIL, NEUTRAL
};

//uncomment to make it work
//#define MARK_BLOCKED_POSITIONS
//#define MARK_VISITABLE_POSITIONS

#define DEFBYPASS


#ifdef _WIN32
	#define DLL_F_EXPORT __declspec(dllexport)
#else
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define DLL_F_EXPORT	__attribute__ ((visibility("default")))
	#else
		#define DLL_F_EXPORT
	#endif
#endif

#ifdef _WIN32
#define DLL_F_IMPORT __declspec(dllimport)
#else
#if defined(__GNUC__) && __GNUC__ >= 4
#define DLL_F_IMPORT	__attribute__ ((visibility("default")))
#else
#define DLL_F_IMPORT
#endif
#endif


#ifdef VCMI_DLL
	#define DLL_EXPORT DLL_F_EXPORT
#else
	#define DLL_EXPORT DLL_F_IMPORT
#endif


template<typename T, size_t N> char (&_ArrayCountObj(const T (&)[N]))[N];  
#define ARRAY_COUNT(arr)    (sizeof(_ArrayCountObj(arr)))



//a normal std::map with consted operator[] for sanity
template<typename KeyT, typename ValT>
class bmap : public std::map<KeyT, ValT>
{
public:
	const ValT & operator[](KeyT key) const
	{
		return find(key)->second;
	}
	ValT & operator[](KeyT key) 
	{
		return static_cast<std::map<KeyT, ValT> &>(*this)[key];
	}	
	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<std::map<KeyT, ValT> &>(*this);
	}
};

namespace vstd
{
	template <typename Container, typename Item>
	bool contains(const Container & c, const Item &i) //returns true if container c contains item i
	{
		return std::find(c.begin(),c.end(),i) != c.end();
	}
	template <typename V, typename Item, typename Item2>
	bool contains(const std::map<Item,V> & c, const Item2 &i) //returns true if map c contains item i
	{
		return c.find(i)!=c.end();
	}
	template <typename V, typename Item, typename Item2>
	bool contains(const bmap<Item,V> & c, const Item2 &i) //returns true if bmap c contains item i
	{
		return c.find(i)!=c.end();
	}
	template <typename Item>
	bool contains(const boost::unordered_set<Item> & c, const Item &i) //returns true if unordered set c contains item i
	{
		return c.find(i)!=c.end();
	}
	template <typename Item, size_t N>
	bool contains(const Item (&c)[N], const Item &i) //returns true if given array contains item i
	{
		return std::find(c, c+N, i) != c+N; //TODO: find out why template is not resolved
	}
	template <typename Container1, typename Container2>
	typename Container2::iterator findFirstNot(Container1 &c1, Container2 &c2)//returns first element of c2 not present in c1
	{
		typename Container2::iterator itr = c2.begin();
		while(itr != c2.end())
			if(!contains(c1,*itr))
				return itr;
			else
				++itr;
		return c2.end();
	}
	template <typename Container1, typename Container2>
	typename Container2::const_iterator findFirstNot(const Container1 &c1, const Container2 &c2)//returns const first element of c2 not present in c1
	{
		typename Container2::const_iterator itr = c2.begin();
		while(itr != c2.end())
			if(!contains(c1,*itr))
				return itr;
			else
				++itr;
		return c2.end();
	}
	template <typename Container, typename Item>
	typename Container::iterator find(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename T1, typename T2>
	int findPos(const std::vector<T1> & c, const T2 &s) //returns position of first element in vector c equal to s, if there is no such element, -1 is returned
	{
		for(size_t i=0; i < c.size(); ++i)
			if(c[i] == s)
				return i;
		return -1;
	}
	template <typename T1, typename T2, typename Func>
	int findPos(const std::vector<T1> & c, const T2 &s, const Func &f) //Func(T1,T2) must say if these elements matches
	{
		for(size_t i=0; i < c.size(); ++i)
			if(f(c[i],s))
				return i;
		return -1;
	}
	template <typename Container, typename Item>
	typename Container::iterator find(Container & c, const Item &i) //returns iterator to the given element if present in container, end() if not
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename Container, typename Item>
	typename Container::const_iterator find(const Container & c, const Item &i)//returns const iterator to the given element if present in container, end() if not
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename Container, typename Item>
	typename Container::size_type operator-=(Container &c, const Item &i) //removes element i from container c, returns false if c does not contain i
	{
		typename Container::iterator itr = find(c,i);
		if(itr == c.end())
			return false;
		c.erase(itr);
		return true;
	}
	template <typename t1>
	void delObj(t1 *a1)
	{
		delete a1;
	}
	template <typename t1, typename t2>
	void assign(t1 &a1, const t2 &a2)
	{
		a1 = a2;
	}
	template <typename t1, typename t2>
	struct assigner
	{
	public:
		t1 &op1;
		t2 op2;
		assigner(t1 &a1, const t2 & a2)
			:op1(a1), op2(a2)
		{}
		void operator()()
		{
			op1 = op2;
		}
	};
	template <typename t1, typename t2>
	assigner<t1,t2> assigno(t1 &a1, const t2 &a2)
	{
		return assigner<t1,t2>(a1,a2);
	}
	template <typename t1, typename t2, typename t3>
	bool equal(const t1 &a1, const t3 t1::* point, const t2 &a2)
	{
		return a1.*point == a2;
	}
	template <typename t1, typename t2>
	bool equal(const t1 &a1, const t2 &a2)
	{
		return a1 == a2;
	}
}
using vstd::operator-=;

template <typename t1, typename t2>
t1 & amax(t1 &a, const t2 &b) //assigns greater of (a, b) to a and returns maximum of (a, b)
{
	if(a >= b)
		return a;
	else
	{
		a = b;
		return a;
	}
}

template <typename t1, typename t2>
t1 & amin(t1 &a, const t2 &b) //assigns smaller of (a, b) to a and returns minimum of (a, b)
{
	if(a <= b)
		return a;
	else
	{
		a = b;
		return a;
	}
}

template <typename t1, typename t2, typename t3>
t1 & abetw(t1 &a, const t2 &b, const t3 &c) //makes a to fit the range <b, c>
{
	amax(a,b);
	amin(a,c);
	return a;
}

template <typename t1, typename t2, typename t3>
bool isbetw(const t1 &a, const t2 &b, const t3 &c) //checks if a is between b and c
{
	return a > b && a < c;
}

template <typename t1, typename t2, typename t3>
bool iswith(const t1 &a, const t2 &b, const t3 &c) //checks if a is within b and c
{
	return a >= b && a <= c;
}

template <typename T> 
void delNull(T* &ptr) //deleted pointer and sets it to NULL
{
	delete ptr;
	ptr = NULL;
}

#include "CConsoleHandler.h"
extern DLL_EXPORT std::ostream *logfile;
extern DLL_EXPORT CConsoleHandler *console;

class CLogger //logger, prints log info to console and saves in file
{
	const int lvl;
#ifdef ANDROID
	std::ostringstream buf;
	int androidloglevel;
	void outputAndroid()
	{
		int pos = buf.str().find("\n");
		while( pos >= 0 )
		{
			__android_log_print(androidloglevel, "VCMI", "%s", buf.str().substr(0, pos).c_str() );
			buf.str( buf.str().substr(pos+1) );
			pos = buf.str().find("\n");
		}
	}
#endif

public:
	CLogger& operator<<(std::ostream& (*fun)(std::ostream&))
	{
#ifdef ANDROID
		buf << fun;
		outputAndroid();
#else
		if(lvl < CONSOLE_LOGGING_LEVEL)
			std::cout << fun;
		if((lvl < FILE_LOGGING_LEVEL) && logfile)
			*logfile << fun;
#endif
		return *this;
	}

	template<typename T> 
	CLogger & operator<<(const T & data)
	{
#ifdef ANDROID
		buf << data;
		outputAndroid();
#else
		if(lvl < CONSOLE_LOGGING_LEVEL)
		{
			if(console)
				console->print(data,lvl);
			else
				std::cout << data << std::flush;
		}
		if((lvl < FILE_LOGGING_LEVEL) && logfile)
			*logfile << data << std::flush;
#endif
		return *this;
	}

	CLogger(const int Lvl) : lvl(Lvl)
	{
#ifdef ANDROID
		androidloglevel = ANDROID_LOG_INFO;
		switch(lvl) {
			case 0: androidloglevel = ANDROID_LOG_INFO; break;
			case 1: androidloglevel = ANDROID_LOG_FATAL; break;
			case 2: androidloglevel = ANDROID_LOG_ERROR; break;
			case 3: androidloglevel = ANDROID_LOG_WARN; break;
			case 4: androidloglevel = ANDROID_LOG_INFO; break;
			case 5: androidloglevel = ANDROID_LOG_DEBUG; break;
			case 6: case -2: androidloglevel = ANDROID_LOG_VERBOSE; break;
		}
#endif
	}
};

extern DLL_EXPORT CLogger tlog0; //green - standard progress info
extern DLL_EXPORT CLogger tlog1; //red - big errors
extern DLL_EXPORT CLogger tlog2; //magenta - major warnings
extern DLL_EXPORT CLogger tlog3; //yellow - minor warnings
extern DLL_EXPORT CLogger tlog4; //white - detailed log info
extern DLL_EXPORT CLogger tlog5; //gray - minor log info
extern DLL_EXPORT CLogger tlog6; //teal - AI info

//XXX pls dont - 'debug macros' are usually more trouble than it's worth
#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {	\
	tlog1 << e.what() << std::endl;		\
	throw;								\
	}									\
	catch (const std::exception * e)	\
	{									\
		tlog1 << e->what()<< std::endl;	\
		throw;							\
	}									\
	catch (const std::string& e) {		\
		tlog1 << e << std::endl;		\
		throw;							\
	}

#define HANDLE_EXCEPTIONC(COMMAND)  \
	catch (const std::exception& e) {	\
		COMMAND;						\
		tlog1 << e.what() << std::endl;	\
		throw;							\
	}									\
	catch (const std::string &e)	\
	{									\
		COMMAND;						\
		tlog1 << e << std::endl;	\
		throw;							\
	}


#if defined(linux) && defined(sparc) 
/* SPARC does not support unaligned memory access. Let gcc know when
 * to emit the right code. */
struct unaligned_Uint16 { ui16 val __attribute__(( packed )); };
struct unaligned_Uint32 { ui32 val __attribute__(( packed )); };

static inline ui16 read_unaligned_u16(const void *p)
{
        const struct unaligned_Uint16 *v = (const struct unaligned_Uint16 *)p;
        return v->val;
}

static inline ui32 read_unaligned_u32(const void *p)
{
        const struct unaligned_Uint32 *v = (const struct unaligned_Uint32 *)p;
        return v->val;
}

#else
#define read_unaligned_u16(p) (* reinterpret_cast<const Uint16 *>(p))
#define read_unaligned_u32(p) (* reinterpret_cast<const Uint32 *>(p))
#endif

//for explicit overrides
#ifdef _MSC_VER
	#define OVERRIDE override
#else
	#define OVERRIDE 	//is there any working counterpart?
#endif

#define BONUS_TREE_DESERIALIZATION_FIX if(!h.saving && h.smartPointerSerialization) deserializationFix();

#endif // __GLOBAL_H__
