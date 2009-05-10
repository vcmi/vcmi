#ifndef __CGENIUSAI_H__
#define __CGENIUSAI_H__

#include "Common.h"
#include "BattleLogic.h"
#include "GeneralAI.h"

namespace GeniusAI {

class CGeniusAI : public CGlobalAI
{
private:
	ICallback*							m_cb;
	GeniusAI::BattleAI::CBattleLogic*	m_battleLogic;
	GeniusAI::GeneralAI::CGeneralAI		m_generalAI;
public:
	CGeniusAI();
	virtual ~CGeniusAI();

	virtual void init(ICallback * CB);
	virtual void yourTurn();
	virtual void heroKilled(const CGHeroInstance *);
	virtual void heroCreated(const CGHeroInstance *);
	virtual void heroMoved(const HeroMoveDetails &);
	virtual void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, int val) {};
	virtual void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID){};
	virtual void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel); //Show a dialog, player must take decision. If selection then he has to choose between one of given components, if cancel he is allowed to not choose. After making choice, CCallback::selectionMade should be called with number of selected component (1 - n) or 0 for cancel (if allowed) and askID.
	virtual void tileRevealed(int3 pos){};
	virtual void tileHidden(int3 pos){};
	virtual void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback);
	virtual void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, boost::function<void()> &onEnd);
	// battle
	virtual void actionFinished(const BattleAction *action);//occurs AFTER every action taken by any stack or by the hero
	virtual void actionStarted(const BattleAction *action);//occurs BEFORE every action taken by any stack or by the hero
	virtual void battleAttack(BattleAttack *ba); //called when stack is performing attack
	virtual void battleStacksAttacked(std::set<BattleStackAttacked> & bsa); //called when stack receives damage (after battleAttack())
	virtual void battleEnd(BattleResult *br);
	virtual void battleNewRound(int round); //called at the beggining of each turn, round=-1 is the tactic phase, round=0 is the first "normal" turn
	virtual void battleStackMoved(int ID, int dest, int distance, bool end);
	virtual void battleSpellCasted(SpellCasted *sc);
	virtual void battleStart(CCreatureSet *army1, CCreatureSet *army2, int3 tile, CGHeroInstance *hero1, CGHeroInstance *hero2, bool side); //called by engine when battle starts; side=0 - left, side=1 - right
	virtual void battlefieldPrepared(int battlefieldType, std::vector<CObstacle*> obstacles); //called when battlefield is prepared, prior the battle beginning
	//
	virtual void battleStackMoved(int ID, int dest, bool startMoving, bool endMoving);
	virtual void battleStackAttacking(int ID, int dest);
	virtual void battleStackIsAttacked(int ID, int dmg, int killed, int IDby, bool byShooting);
	virtual BattleAction activeStack(int stackID);
};

}

#endif // __CGENIUSAI_H__