#ifndef GLOBAL_H
#define GLOBAL_H
#include <iostream>
#include <boost/logic/tribool.hpp>
#include <boost/cstdint.hpp>
typedef boost::uint32_t ui32;  //unsigned int 32 bits (4 bytes)
typedef boost::uint16_t ui16; //unsigned int 16 bits (2 bytes)
typedef boost::uint8_t ui8; //unsigned int 8 bits (1 byte)
typedef boost::int32_t si32; //signed int 32 bits (4 bytes)
typedef boost::int16_t si16; //signed int 16 bits (2 bytes)
typedef boost::int8_t si8; //signed int 8 bits (1 byte)
#include "int3.h"
#define CHECKTIME 1
#if CHECKTIME
#include "timeHandler.h"
#define THC
#endif

#define NAME_VER ("VCMI 0.62")


#ifdef _WIN32
#define PATHSEPARATOR "\\"
#define DATA_DIR ""
#else
#define PATHSEPARATOR "/"
#define DATA_DIR "/progdir/"
#endif

enum Ecolor {RED, BLUE, TAN, GREEN, ORANGE, PURPLE, TEAL, PINK}; //player's colors
enum EterrainType {border=-1, dirt, sand, grass, snow, swamp, rough, subterranean, lava, water, rock};
enum Eriver {noRiver=0, clearRiver, icyRiver, muddyRiver, lavaRiver};
enum Eroad {dirtRoad=1, grazvelRoad, cobblestoneRoad};
enum Eformat { WoG=0x33, AB=0x15, RoE=0x0e,  SoD=0x1c};
enum EvictoryConditions {artifact, gatherTroop, gatherResource, buildCity, buildGrail, beatHero,
	captureCity, beatMonster, takeDwellings, takeMines, transportItem, winStandard=255};
enum ElossCon {lossCastle, lossHero, timeExpires, lossStandard=255};
enum EHeroClasses {HERO_KNIGHT, HERO_CLERIC, HERO_RANGER, HERO_DRUID, HERO_ALCHEMIST, HERO_WIZARD,
	HERO_DEMONIAC, HERO_HERETIC, HERO_DEATHKNIGHT, HERO_NECROMANCER, HERO_WARLOCK, HERO_OVERLORD,
	HERO_BARBARIAN, HERO_BATTLEMAGE, HERO_BEASTMASTER, HERO_WITCH, HERO_PLANESWALKER, HERO_ELEMENTALIST};
class CGameInfo;
extern CGameInfo* CGI;

#define HEROI_TYPE (0)
#define TOWNI_TYPE (1)

const int F_NUMBER = 9; //factions (town types) quantity
const int PLAYER_LIMIT = 8; //player limit per map
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

#define MARK_BLOCKED_POSITIONS false
#define MARK_VISITABLE_POSITIONS false

#define DEFBYPASS

#ifdef _WIN32
	#ifdef VCMI_DLL
		#define DLL_EXPORT __declspec(dllexport)
	#else
		#define DLL_EXPORT __declspec(dllimport)
	#endif
#else
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define DLL_EXPORT	__attribute__ ((visibility("default")))
	#else
		#define DLL_EXPORT
	#endif
#endif

#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {	\
	std::cerr << e.what() << std::endl;	\
	}									\
	catch (const std::exception * e)	\
	{									\
		std::cerr << e->what()<< std::endl;	\
		delete e;						\
	}


namespace vstd
{
	template <typename Container, typename Item>
	bool contains(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i) != c.end();
	}
	template <typename V, typename Item>
	bool contains(const std::map<Item,V> & c, const Item &i)
	{
		return c.find(i)!=c.end();
	}
	template <typename Container, typename Item>
	typename Container::iterator find(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename Container, typename Item>
	typename Container::iterator find(Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename Container, typename Item>
	bool operator-=(Container &c, const Item &i)
	{
		Container::iterator itr = find(c,i);
		if(itr == c.end())
			return false;
		c.erase(itr);
		return true;
	}
}
using vstd::operator-=;
#endif //GLOBAL_H
