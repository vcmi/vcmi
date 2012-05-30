#pragma once

#include "../../lib/AI_Base.h"
#include "../../CCallback.h"

struct HeroMoveDetails;

class CEmptyAI : public CGlobalAI
{
	CCallback *cb;

public:
	void init(CCallback * CB) override;
	void yourTurn() override;
	void heroKilled(const CGHeroInstance *) override;
	void heroCreated(const CGHeroInstance *) override;
	void heroMoved(const TryMoveHero&) override;
	void heroPrimarySkillChanged(const CGHeroInstance * hero, int which, si64 val) override {};
	void showSelDialog(std::string text, std::vector<CSelectableComponent*> & components, int askID) override {};
	void tileRevealed(const boost::unordered_set<int3, ShashInt3> &pos) override {};
	void tileHidden(const boost::unordered_set<int3, ShashInt3> &pos) override {};
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, int soundID, bool selection, bool cancel) override {};
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, boost::function<void()> &onEnd) override {};
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, boost::function<void(ui32)> &callback) override;
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, boost::function<void(ui32)> &callback) override {}; //TODO
};

#define NAME "EmptyAI 0.1"
