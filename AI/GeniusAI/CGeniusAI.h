#ifndef __CGENIUSAI_H__
#define __CGENIUSAI_H__

#include "Common.h"
#include "BattleLogic.h"
#include "GeneralAI.h"
#include "../../lib/CondSh.h"
#include <set>
#include <list>
#include <queue>

class CBuilding;

namespace geniusai {

enum BattleState
{
	NO_BATTLE,
	UPCOMING_BATTLE,
	ONGOING_BATTLE,
	ENDING_BATTLE
};


class Priorities;

class CGeniusAI : public CGlobalAI
{
private:
	ICallback*							m_cb;
	geniusai::BattleAI::CBattleLogic*	m_battleLogic;
	geniusai::GeneralAI::CGeneralAI		m_generalAI;
	geniusai::Priorities *				m_priorities;

	
	CondSh<BattleState> m_state; //are we engaged into battle?

	struct AIObjectContainer
	{
		AIObjectContainer(const CGObjectInstance * o):o(o){}
		const CGObjectInstance * o;
		bool operator<(const AIObjectContainer& b)const;
	};
	
	class HypotheticalGameState
	{
	public:
		struct HeroModel
		{
			HeroModel(){}
			HeroModel(const CGHeroInstance * h);
			int3 pos;
			int3 previouslyVisited_pos;
			int3 interestingPos;
			bool finished;
			int remainingMovement;
			const CGHeroInstance * h;
		};
		struct TownModel
		{
			TownModel(const CGTownInstance *t);
			const CGTownInstance *t;
			std::vector<std::pair<ui32, std::vector<ui32> > > creaturesToRecruit;
			CCreatureSet creaturesInGarrison;				//type, num
			bool hasBuilt;
		};
		HypotheticalGameState(){}
		HypotheticalGameState(CGeniusAI & ai);

		void update(CGeniusAI & ai);
		CGeniusAI * AI;
		std::vector<const CGHeroInstance *> AvailableHeroesToBuy;
		std::vector<int> resourceAmounts;
		std::vector<HeroModel> heroModels;
		std::vector<TownModel> townModels;
		std::set< AIObjectContainer > knownVisitableObjects;
	};

	class AIObjective
	{
	public: 
		enum Type
		{
			//hero objectives
			visit,				//done TODO: upon visit friendly hero, trade
			attack,				//done
			//flee,
			dismissUnits,
			dismissYourself,
			rearangeTroops,
			finishTurn,			//done	//uses up remaining motion to get somewhere interesting.

			//town objectives
			recruitHero,		//done
			buildBuilding,		//done
			recruitCreatures,	//done
			upgradeCreatures	//done
		};
		CGeniusAI * AI;
		Type type;
		virtual void fulfill(CGeniusAI &,HypotheticalGameState & hgs)=0;
		virtual HypotheticalGameState pretend(const HypotheticalGameState &) =0;
		virtual void print() const=0;
		virtual float getValue() const=0;	//how much is it worth to the AI to achieve
	};

	class HeroObjective: public AIObjective
	{
	public:
		HypotheticalGameState hgs;
		int3 pos;
		const CGObjectInstance * object;
		mutable std::vector<HypotheticalGameState::HeroModel *> whoCanAchieve;
		
		//HeroObjective(){}
		//HeroObjective(Type t):object(NULL){type = t;}
		HeroObjective(const HypotheticalGameState &hgs,Type t,const CGObjectInstance * object,HypotheticalGameState::HeroModel *h,CGeniusAI * AI);
		bool operator < (const HeroObjective &other)const;
		void fulfill(CGeniusAI &,HypotheticalGameState & hgs);
		HypotheticalGameState pretend(const HypotheticalGameState &hgs){return hgs;};
		float getValue() const;
		void print() const;
	private:
		mutable float _value;
		mutable float _cost;
	};

	//town objectives
		//recruitHero
		//buildBuilding
		//recruitCreatures
		//upgradeCreatures

	class TownObjective: public AIObjective
	{
	public:
		HypotheticalGameState hgs;
		HypotheticalGameState::TownModel * whichTown;
		int which;				//which hero, which building, which creature, 

		TownObjective(const HypotheticalGameState &hgs,Type t,HypotheticalGameState::TownModel * tn,int Which,CGeniusAI * AI);
		
		bool operator < (const TownObjective &other)const;
		void fulfill(CGeniusAI &,HypotheticalGameState & hgs);
		HypotheticalGameState pretend(const HypotheticalGameState &hgs){return hgs;};
		float getValue() const;
		void print() const;
	private:
		mutable float _value;
		mutable float _cost;
	};

	class AIObjectivePtrCont
	{
	public:
		AIObjectivePtrCont():obj(NULL){}
		AIObjectivePtrCont(AIObjective * obj):obj(obj){};
		AIObjective * obj;
		bool operator < (const AIObjectivePtrCont & other) const{return obj->getValue()<other.obj->getValue();}
	};
	HypotheticalGameState trueGameState;
	AIObjective * getBestObjective();
	void addHeroObjectives(HypotheticalGameState::HeroModel &h, HypotheticalGameState & hgs);
	void addTownObjectives(HypotheticalGameState::TownModel &h, HypotheticalGameState & hgs);
	void fillObjectiveQueue(HypotheticalGameState & hgs);
	
	void reportResources();
	void startFirstTurn();
	std::map<int,bool> isHeroStrong;//hero
	std::set< AIObjectContainer > knownVisitableObjects;
	std::set<HeroObjective> currentHeroObjectives;	//can be fulfilled right now
	std::set<TownObjective> currentTownObjectives;
	std::vector<AIObjectivePtrCont> objectiveQueue;

public:
	CGeniusAI();
	virtual ~CGeniusAI();

	virtual void init(ICallback * CB);
	virtual void yourTurn();
	virtual void heroKilled(const CGHeroInstance *);
	virtual void heroCreated(const CGHeroInstance *);
	virtual void heroMoved(const TryMoveHero &);
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val) {};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void tileRevealed(int3 pos);
	virtual void tileHidden(int3 pos);
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd);
	virtual void playerBlocked(int reason);

	virtual void objectRemoved(const CGObjectInstance *obj); //eg. collected resource, picked artifact, beaten hero
	virtual void newObject(const CGObjectInstance * obj); //eg. ship built in shipyard
	
	
	// battle
	virtual void actionFinished(const BattleAction *action);//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action);//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(BattleAttack *ba); //called when stack is performing attack
	virtual void battleStacksAttacked(std::set<BattleStackAttacked> & bsa); //called when stack receives damage (after battleAttack())
	virtual void battleEnd(BattleResult *br);
	virtual void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(int ID, int dest, int distance, bool end);
	virtual void battleSpellCast(SpellCast *sc);
	virtual void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	//
	virtual void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving);
	virtual void battleStackAttacking(int ID, int dest);
	virtual void battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting);
	virtual BattleAction activeStack(int stackID);
	void battleResultsApplied();
	friend class Priorities;
};
}

#endif // __CGENIUSAI_H__
