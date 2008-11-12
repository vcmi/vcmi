#ifndef GLOBAL_H
#define GLOBAL_H
#include <iostream>
#include <boost/logic/tribool.hpp>
#include <boost/cstdint.hpp>
typedef boost::uint64_t ui64; //unsigned int 64 bits (8 bytes)
typedef boost::uint32_t ui32;  //unsigned int 32 bits (4 bytes)
typedef boost::uint16_t ui16; //unsigned int 16 bits (2 bytes)
typedef boost::uint8_t ui8; //unsigned int 8 bits (1 byte)
typedef boost::int64_t si64; //signed int 64 bits (8 bytes)
typedef boost::int32_t si32; //signed int 32 bits (4 bytes)
typedef boost::int16_t si16; //signed int 16 bits (2 bytes)
typedef boost::int8_t si8; //signed int 8 bits (1 byte)
#include "int3.h"
#define CHECKTIME 1
#if CHECKTIME
#include "timeHandler.h"
#define THC
#endif

#define NAME_VER ("VCMI 0.65")
#define CONSOLE_LOGGING_LEVEL 5
#define FILE_LOGGING_LEVEL 6

#ifdef _WIN32
#define PATHSEPARATOR "\\"
#define DATA_DIR ""
#define SERVER_NAME "VCMI_server.exe"
#else
#define PATHSEPARATOR "/"
#define DATA_DIR ""
#define SERVER_NAME "./vcmiserver"
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
enum EAbilities {DOUBLE_WIDE, FLYING, SHOOTER, TWO_HEX_ATTACK, SIEGE_ABILITY, SIEGE_WEAPON, 
KING1, KING2, KING3, MIND_IMMUNITY, NO_OBSTACLE_PENALTY, NO_CLOSE_COMBAT_PENALTY, 
JOUSTING, FIRE_IMMUNITY, TWICE_ATTACK, NO_ENEMY_RETALIATION, NO_MORAL_PENALTY, 
UNDEAD, MULTI_HEAD_ATTACK, EXTENDED_RADIOUS_SHOOTER, GHOST, RAISES_MORALE,
LOWERS_MORALE, DRAGON, STRIKE_AND_RETURN, FEARLESS, REBIRTH}; //some flags are used only for battles
enum ECombatInfo{ALIVE = REBIRTH+1, SUMMONED, CLONED, HAD_MORALE, WAITING, MOVED, DEFENDING};
class CGameInfo;
extern CGameInfo* CGI;

#define HEROI_TYPE (34)
#define TOWNI_TYPE (98)

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
const int SPELL_LEVELS = 5;

#define BFIELD_WIDTH (17)
#define BFIELD_HEIGHT (11)
#define BFIELD_SIZE ((BFIELD_WIDTH) * (BFIELD_HEIGHT))

//uncomment to make it work
//#define MARK_BLOCKED_POSITIONS
//#define MARK_VISITABLE_POSITIONS

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

namespace vstd
{
	template <typename Container, typename Item>
	bool contains(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i) != c.end();
	}
	template <typename V, typename Item, typename Item2>
	bool contains(const std::map<Item,V> & c, const Item2 &i)
	{
		return c.find(i)!=c.end();
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
	template <typename Container, typename Item>
	typename Container::iterator find(const Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename T1, typename T2>
	int findPos(const std::vector<T1> & c, const T2 &s)
	{
		for(int i=0;i<c.size();i++)
			if(c[i] == s)
				return i;
		return -1;
	}
	template <typename T1, typename T2, typename Func>
	int findPos(const std::vector<T1> & c, const T2 &s, const Func &f) //Func(T1,T2) must say if these elements matches
	{
		for(int i=0;i<c.size();i++)
			if(f(c[i],s))
				return i;
		return -1;
	}
	template <typename Container, typename Item>
	typename Container::iterator find(Container & c, const Item &i)
	{
		return std::find(c.begin(),c.end(),i);
	}
	template <typename Container, typename Item>
	bool operator-=(Container &c, const Item &i)
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
		assigner(t1 &a1, t2 a2)
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

#include "CConsoleHandler.h"
extern DLL_EXPORT std::ostream *logfile;
extern DLL_EXPORT CConsoleHandler *console;
template <int lvl> class CLogger
{
public:
	CLogger<lvl>& operator<<(std::ostream& (*fun)(std::ostream&))
	{
		if(lvl < CONSOLE_LOGGING_LEVEL)
			std::cout << fun;
		if((lvl < FILE_LOGGING_LEVEL) && logfile)
			*logfile << fun;
		return *this;
	}

	template<typename T> 
	CLogger<lvl> & operator<<(const T & data)
	{
		if(lvl < CONSOLE_LOGGING_LEVEL)
			if(console)
				console->print(data,lvl);
			else
				std::cout << data << std::flush;
		if((lvl < FILE_LOGGING_LEVEL) && logfile)
			*logfile << data << std::flush;
		return *this;
	}
};

extern DLL_EXPORT CLogger<0> tlog0; //green - standard progress info
extern DLL_EXPORT CLogger<1> tlog1; //red - big errors
extern DLL_EXPORT CLogger<2> tlog2; //magenta - major warnings
extern DLL_EXPORT CLogger<3> tlog3; //yellow - minor warnings
extern DLL_EXPORT CLogger<4> tlog4; //white - detailed log info
extern DLL_EXPORT CLogger<5> tlog5; //gray - minor log info

#define HANDLE_EXCEPTION  \
	catch (const std::exception& e) {	\
	tlog1 << e.what() << std::endl;	\
	}									\
	catch (const std::exception * e)	\
	{									\
		tlog1 << e->what()<< std::endl;	\
		delete e;						\
	}									\
	catch (const std::string& e) {		\
		tlog1 << e << std::endl;	\
	}

#define HANDLE_EXCEPTIONC(COMMAND)  \
	catch (const std::exception& e) {	\
	COMMAND;							\
	tlog1 << e.what() << std::endl;	\
	}									\
	catch (const std::exception * e)	\
	{									\
		COMMAND;						\
		tlog1 << e->what()<< std::endl;	\
		delete e;						\
	}

#endif //GLOBAL_H
