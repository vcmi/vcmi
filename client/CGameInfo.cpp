/*
 * CGameInfo.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */
#include "StdInc.h"
#include "CGameInfo.h"

#include "../lib/VCMI_Lib.h"

const CGameInfo * CGI;
CClientState * CCS = nullptr;
CServerHandler * CSH;


CGameInfo::CGameInfo() = default;

void CGameInfo::setFromLib()
{
	globalServices = VLC;
	modh = VLC->modh;
	generaltexth = VLC->generaltexth;
	creh = VLC->creh;
	townh = VLC->townh;
	heroh = VLC->heroh;
	objh = VLC->objh;
	spellh = VLC->spellh;
	skillh = VLC->skillh;
	objtypeh = VLC->objtypeh;
	terrainTypeHandler = VLC->terrainTypeHandler;
	battleFieldHandler = VLC->battlefieldsHandler;
	obstacleHandler = VLC->obstacleHandler;
}

const ArtifactService * CGameInfo::artifacts() const
{
	return globalServices->artifacts();
}

const BattleFieldService * CGameInfo::battlefields() const
{
	return globalServices->battlefields();
}

const CreatureService * CGameInfo::creatures() const
{
	return globalServices->creatures();
}

const FactionService * CGameInfo::factions() const
{
	return globalServices->factions();
}

const HeroClassService * CGameInfo::heroClasses() const
{
	return globalServices->heroClasses();
}

const HeroTypeService * CGameInfo::heroTypes() const
{
	return globalServices->heroTypes();
}

#if SCRIPTING_ENABLED
const scripting::Service * CGameInfo::scripts()  const
{
	return globalServices->scripts();
}
#endif

const spells::Service * CGameInfo::spells()  const
{
	return globalServices->spells();
}

const SkillService * CGameInfo::skills() const
{
	return globalServices->skills();
}

const ObstacleService * CGameInfo::obstacles() const
{
	return globalServices->obstacles();
}

void CGameInfo::updateEntity(Metatype metatype, int32_t index, const JsonNode & data)
{
	logGlobal->error("CGameInfo::updateEntity call is not expected.");
}

spells::effects::Registry * CGameInfo::spellEffects()
{
	return nullptr;
}

const spells::effects::Registry * CGameInfo::spellEffects() const
{
	return globalServices->spellEffects();
}
