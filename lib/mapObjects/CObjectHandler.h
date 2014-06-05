#pragma once

#include "ObjectTemplate.h"

#include "../IGameCallback.h"
#include "../int3.h"
#include "../HeroBonus.h"

/*
 * CObjectHandler.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

class CGHeroInstance;
struct BattleResult;

class DLL_LINKAGE IObjectInterface
{
public:
	static IGameCallback *cb;

	IObjectInterface();
	virtual ~IObjectInterface();

	virtual void onHeroVisit(const CGHeroInstance * h) const;
	virtual void onHeroLeave(const CGHeroInstance * h) const;
	virtual void newTurn() const;
	virtual void initObj(); //synchr
	virtual void setProperty(ui8 what, ui32 val);//synchr
	
	//Called when queries created DURING HERO VISIT are resolved
	//First parameter is always hero that visited object and triggered the query
	virtual void battleFinished(const CGHeroInstance *hero, const BattleResult &result) const;
	virtual void blockingDialogAnswered(const CGHeroInstance *hero, ui32 answer) const;
	virtual void garrisonDialogClosed(const CGHeroInstance *hero) const;
	virtual void heroLevelUpDone(const CGHeroInstance *hero) const;

//unified interface, AI helpers
	virtual bool wasVisited (PlayerColor player) const;
	virtual bool wasVisited (const CGHeroInstance * h) const;

	static void preInit(); //called before objs receive their initObj
	static void postInit();//called after objs receive their initObj

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		logGlobal->errorStream() << "IObjectInterface serialized, unexpected, should not happen!";
	}
};

class DLL_LINKAGE IBoatGenerator
{
public:
	const CGObjectInstance *o;

	IBoatGenerator(const CGObjectInstance *O);
	virtual ~IBoatGenerator() {}

	virtual int getBoatType() const; //0 - evil (if a ship can be evil...?), 1 - good, 2 - neutral
	virtual void getOutOffsets(std::vector<int3> &offsets) const =0; //offsets to obj pos when we boat can be placed
	int3 bestLocation() const; //returns location when the boat should be placed

	enum EGeneratorState {GOOD, BOAT_ALREADY_BUILT, TILE_BLOCKED, NO_WATER};
	EGeneratorState shipyardStatus() const; //0 - can buid, 1 - there is already a boat at dest tile, 2 - dest tile is blocked, 3 - no water
	void getProblemText(MetaString &out, const CGHeroInstance *visitor = nullptr) const;

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & o;
	}
};

class DLL_LINKAGE IShipyard : public IBoatGenerator
{
public:
	IShipyard(const CGObjectInstance *O);
	virtual ~IShipyard() {}

	virtual void getBoatCost(std::vector<si32> &cost) const;

	static const IShipyard *castFrom(const CGObjectInstance *obj);
	static IShipyard *castFrom(CGObjectInstance *obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & static_cast<IBoatGenerator&>(*this);
	}
};

class DLL_LINKAGE CGObjectInstance : public IObjectInterface
{
public:
	mutable std::string hoverName;
	int3 pos; //h3m pos
	Obj ID;
	si32 subID; //normal subID (this one from OH3 maps ;])
	ObjectInstanceID id;//number of object in map's vector
	ObjectTemplate appearance;

	PlayerColor tempOwner;
	bool blockVisit; //if non-zero then blocks the tile but is visitable from neighbouring tile

	virtual ui8 getPassableness() const; //bitmap - if the bit is set the corresponding player can pass through the visitable tiles of object, even if it's blockvis; if not set - default properties from definfo are used
	virtual int3 getSightCenter() const; //"center" tile from which the sight distance is calculated
	virtual int getSightRadious() const; //sight distance (should be used if player-owned structure)
	bool passableFor(PlayerColor color) const;
	void getSightTiles(std::unordered_set<int3, ShashInt3> &tiles) const; //returns reference to the set
	PlayerColor getOwner() const;
	void setOwner(PlayerColor ow);
	int getWidth() const; //returns width of object graphic in tiles
	int getHeight() const; //returns height of object graphic in tiles
	virtual bool visitableAt(int x, int y) const; //returns true if object is visitable at location (x, y) (h3m pos)
	virtual int3 getVisitableOffset() const; //returns (x,y,0) offset to first visitable tile from bottom right obj tile (0,0,0) (h3m pos)
	int3 visitablePos() const;
	bool blockingAt(int x, int y) const; //returns true if object is blocking location (x, y) (h3m pos)
	bool coveringAt(int x, int y) const; //returns true if object covers with picture location (x, y) (h3m pos)
	std::set<int3> getBlockedPos() const; //returns set of positions blocked by this object
	std::set<int3> getBlockedOffsets() const; //returns set of relative positions blocked by this object
	bool isVisitable() const; //returns true if object is visitable
	bool operator<(const CGObjectInstance & cmp) const;  //screen printing priority comparing
	void hideTiles(PlayerColor ourplayer, int radius) const;
	CGObjectInstance();
	virtual ~CGObjectInstance();
	//CGObjectInstance(const CGObjectInstance & right);
	//CGObjectInstance& operator=(const CGObjectInstance & right);
	virtual const std::string & getHoverText() const;

	void setType(si32 ID, si32 subID);

	///IObjectInterface
	void initObj() override;
	void onHeroVisit(const CGHeroInstance * h) const override;
	void setProperty(ui8 what, ui32 val) override;//synchr

	friend class CGameHandler;


	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & hoverName & pos & ID & subID & id & tempOwner & blockVisit & appearance;
		//definfo is handled by map serializer
	}
protected:
	virtual void setPropertyDer(ui8 what, ui32 val);//synchr

	void getNameVis(std::string &hname) const;
	void giveDummyBonus(ObjectInstanceID heroID, ui8 duration = Bonus::ONE_DAY) const;
};

/// function object which can be used to find an object with an specific sub ID
class CGObjectInstanceBySubIdFinder
{
public:
	CGObjectInstanceBySubIdFinder(CGObjectInstance * obj);
	bool operator()(CGObjectInstance * obj) const;

private:
	CGObjectInstance * obj;
};

struct BankConfig
{
	BankConfig() {level = chance = upgradeChance = combatValue = value = rewardDifficulty = easiest = 0; };
	ui8 level; //1 - 4, how hard the battle will be
	ui8 chance; //chance for this level being chosen
	ui8 upgradeChance; //chance for creatures to be in upgraded versions
	std::vector< std::pair <CreatureID, ui32> > guards; //creature ID, amount
	ui32 combatValue; //how hard are guards of this level
	Res::ResourceSet resources; //resources given in case of victory
	std::vector< std::pair <CreatureID, ui32> > creatures; //creatures granted in case of victory (creature ID, amount)
	std::vector<ui16> artifacts; //number of artifacts given in case of victory [0] -> treasure, [1] -> minor [2] -> major [3] -> relic
	ui32 value; //overall value of given things
	ui32 rewardDifficulty; //proportion of reward value to difficulty of guards; how profitable is this creature Bank config
	ui16 easiest; //?!?

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & level & chance & upgradeChance & guards & combatValue & resources & creatures & artifacts & value & rewardDifficulty & easiest;
	}
};

class DLL_LINKAGE CObjectHandler
{
public:
	std::map<si32, CreatureID> cregens; //type 17. dwelling subid -> creature ID
	std::map <ui32, std::vector < ConstTransitivePtr<BankConfig> > > banksInfo; //[index][preset]
	std::map <ui32, std::string> creBanksNames; //[crebank index] -> name of this creature bank
	std::vector<ui32> resVals; //default values of resources in gold

	CObjectHandler();
	~CObjectHandler();

	int bankObjToIndex (const CGObjectInstance * obj);

	template <typename Handler> void serialize(Handler &h, const int version)
	{
		h & cregens & banksInfo & creBanksNames & resVals;
	}
};
