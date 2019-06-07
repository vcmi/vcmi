/*
 * CGameInfo.h, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#pragma once

#include <vcmi/Services.h>

#include "../lib/ConstTransitivePtr.h"

class CModHandler;
class CMapHandler;
class CArtHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CSkillHandler;
class CBuildingHandler;
class CObjectHandler;
class CSoundHandler;
class CMusicHandler;
class CObjectClassesHandler;
class CTownHandler;
class CGeneralTextHandler;
class CConsoleHandler;
class CCursorHandler;
class CGameState;
class IMainVideoPlayer;
class CServerHandler;

class CMap;


//a class for non-mechanical client GUI classes
class CClientState
{
public:
	CSoundHandler * soundh;
	CMusicHandler * musich;
	CConsoleHandler * consoleh;
	CCursorHandler * curh;
	IMainVideoPlayer * videoh;
};
extern CClientState * CCS;

/// CGameInfo class
/// for allowing different functions for accessing game informations
class CGameInfo : public Services
{
public:
	const ArtifactService * artifacts() const override;
	const CreatureService * creatures() const override;
	const FactionService * factions() const override;
	const HeroClassService * heroClasses() const override;
	const HeroTypeService * heroTypes() const override;
	const scripting::Service * scripts() const override;
	const spells::Service * spells() const override;
	const SkillService * skills() const override;

	const spells::effects::Registry * spellEffects() const override;
	spells::effects::Registry * spellEffects() override;


	ConstTransitivePtr<CModHandler> modh; //public?
	ConstTransitivePtr<CArtHandler> arth;
	ConstTransitivePtr<CHeroHandler> heroh;
	ConstTransitivePtr<CCreatureHandler> creh;
	ConstTransitivePtr<CSpellHandler> spellh;
	ConstTransitivePtr<CSkillHandler> skillh;
	ConstTransitivePtr<CObjectHandler> objh;
	ConstTransitivePtr<CObjectClassesHandler> objtypeh;
	CGeneralTextHandler * generaltexth;
	CMapHandler * mh;
	CTownHandler * townh;

	void setFromLib();

	CGameInfo();
private:
	const Services * globalServices;
};
extern const CGameInfo* CGI;
