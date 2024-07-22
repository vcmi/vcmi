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

VCMI_LIB_NAMESPACE_BEGIN

class CModHandler;
class CHeroHandler;
class CCreatureHandler;
class CSpellHandler;
class CSkillHandler;
class CObjectHandler;
class CObjectClassesHandler;
class CTownHandler;
class CGeneralTextHandler;
class CConsoleHandler;
class CGameState;
class BattleFieldHandler;
class ObstacleHandler;
class TerrainTypeHandler;

class CMap;

VCMI_LIB_NAMESPACE_END

class CMapHandler;
class ISoundPlayer;
class IMusicPlayer;
class CursorHandler;
class IVideoPlayer;
class CServerHandler;

//a class for non-mechanical client GUI classes
class CClientState
{
public:
	ISoundPlayer * soundh;
	IMusicPlayer * musich;
	CConsoleHandler * consoleh;
	CursorHandler * curh;
	IVideoPlayer * videoh;
};
extern CClientState * CCS;

/// CGameInfo class
/// for allowing different functions for accessing game information
class CGameInfo final : public Services
{
public:
	const ArtifactService * artifacts() const override;
	const CreatureService * creatures() const override;
	const FactionService * factions() const override;
	const HeroClassService * heroClasses() const override;
	const HeroTypeService * heroTypes() const override;
#if SCRIPTING_ENABLED
	const scripting::Service * scripts() const override;
#endif
	const spells::Service * spells() const override;
	const SkillService * skills() const override;
	const BattleFieldService * battlefields() const override;
	const ObstacleService * obstacles() const override;
	const IGameSettings * settings() const override;

	const spells::effects::Registry * spellEffects() const override;
	spells::effects::Registry * spellEffects() override;

	std::shared_ptr<const CModHandler> modh;
	std::shared_ptr<const BattleFieldHandler> battleFieldHandler;
	std::shared_ptr<const CHeroHandler> heroh;
	std::shared_ptr<const CCreatureHandler> creh;
	std::shared_ptr<const CSpellHandler> spellh;
	std::shared_ptr<const CSkillHandler> skillh;
	std::shared_ptr<const CObjectHandler> objh;
	std::shared_ptr<const TerrainTypeHandler> terrainTypeHandler;
	std::shared_ptr<const CObjectClassesHandler> objtypeh;
	std::shared_ptr<const ObstacleHandler> obstacleHandler;
	std::shared_ptr<const CGeneralTextHandler> generaltexth;
	std::shared_ptr<const CTownHandler> townh;

	std::shared_ptr<CMapHandler> mh;

	void setFromLib();

	CGameInfo();
private:
	const Services * globalServices;
};
extern CGameInfo* CGI;
