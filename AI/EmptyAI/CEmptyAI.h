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
	void heroGotLevel(const CGHeroInstance *hero, int pskill, std::vector<ui16> &skills, int queryID) override; 
	void commanderGotLevel (const CCommanderInstance * commander, std::vector<ui32> skills, int queryID) override;
	void showBlockingDialog(const std::string &text, const std::vector<Component> &components, ui32 askID, const int soundID, bool selection, bool cancel) override; 
	void showGarrisonDialog(const CArmedInstance *up, const CGHeroInstance *down, bool removableUnits, int queryID) override;
};

#define NAME "EmptyAI 0.1"
